"""SQL update validator: filename conventions, CRLF, idempotence, ID ranges, collisions, schema and world references."""
import argparse
from dataclasses import dataclass, field
from datetime import date
import glob
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import sys
from typing import Any, Dict, Iterable, List, Optional, Sequence, Set, Tuple

ROOT = Path(__file__).resolve().parents[2]
IDENT_RE = re.compile(r'^[A-Za-z0-9_]+$')
FILENAME_RE = re.compile(r'^(?P<year>\d{4})_(?P<month>\d{2})_(?P<day>\d{2})_(?P<agent>[a-z0-9]+)_(?P<seq>\d{2,})_(?P<slug>[a-z0-9_]+)\.sql$')
LEGACY_FILENAME_RE = re.compile(r'^(?P<year>\d{4})_(?P<month>\d{2})_(?P<day>\d{2})_(?P<rest>[A-Za-z0-9_-]+)\.sql$')
DEFAULT_AGENTS = ('claude', 'codex', 'gemini')


# ---------------------------------------------------------------------------
# Generic SQL Core: Lexer, Statement Parser, Static & Idempotence Checks
# ---------------------------------------------------------------------------

@dataclass(frozen=True)
class Issue:
    file: str
    line: int
    category: str
    severity: str
    message: str

    def to_dict(self) -> Dict[str, Any]:
        return {
            'file': self.file,
            'line': self.line,
            'category': self.category,
            'severity': self.severity,
            'message': self.message,
        }


@dataclass
class Statement:
    text: str
    line: int
    kind: str
    table: str = ''
    if_not_exists: bool = False
    if_exists: bool = False
    temporary: bool = False
    is_ignore: bool = False
    is_replace: bool = False
    has_on_duplicate: bool = False
    columns: Tuple[str, ...] = ()
    rows: List[Tuple[Any, ...]] = field(default_factory=list)
    where_text: str = ''
    where_keys: Dict[str, Set[Any]] = field(default_factory=dict)
    where_unscoped: bool = False
    set_columns: Tuple[str, ...] = ()
    relative_self_columns: Tuple[str, ...] = ()
    create_columns: Dict[str, str] = field(default_factory=dict)
    primary_key: Tuple[str, ...] = ()


def check_line_endings(path: Path, raw_bytes: bytes) -> List[Issue]:
    issues: List[Issue] = []
    rel = _rel_path(path)
    crlf_count = raw_bytes.count(b'\r\n')
    bare_cr = raw_bytes.replace(b'\r\n', b'').count(b'\r')
    if crlf_count or bare_cr:
        first_line = 1
        for idx, line in enumerate(raw_bytes.splitlines(keepends=True), start=1):
            if line.endswith(b'\r\n') or line.endswith(b'\r'):
                first_line = idx
                break
        parts = []
        if crlf_count:
            parts.append(f'{crlf_count} CRLF line(s)')
        if bare_cr:
            parts.append(f'{bare_cr} CR line(s)')
        issues.append(Issue(rel, first_line, 'crlf', 'error',
                            f"File contains {', '.join(parts)}; SQL updates must use LF line endings"))
    return issues


def check_filename(path: Path, known_agents: Sequence[str] = DEFAULT_AGENTS,
                   allow_legacy_before: Optional[date] = None) -> Tuple[List[Issue], Optional[Tuple[str, str, str]]]:
    """Validate YYYY_MM_DD_<agent>_<NN>_<description>.sql and return (issues, (date_str, agent, seq))."""
    issues: List[Issue] = []
    rel = _rel_path(path)
    name = path.name
    match = FILENAME_RE.match(name)
    if not match:
        legacy = LEGACY_FILENAME_RE.match(name)
        if legacy and allow_legacy_before is not None:
            try:
                file_date = date(int(legacy.group('year')), int(legacy.group('month')), int(legacy.group('day')))
                if file_date <= allow_legacy_before:
                    return issues, None
            except ValueError:
                pass
        # Check if an unknown agent or uppercase characters caused the mismatch
        parts = name[:-4].split('_') if name.endswith('.sql') else []
        if len(parts) >= 5 and all(p.isdigit() for p in parts[:3]):
            candidate_agent = parts[3]
            if candidate_agent not in known_agents:
                issues.append(Issue(
                    rel, 1, 'filename', 'error',
                    f"Filename '{name}' does not match YYYY_MM_DD_<agent>_<NN>_<description>.sql "
                    f"(agent '{candidate_agent}' not in {tuple(known_agents)})"
                ))
                return issues, None
        issues.append(Issue(
            rel, 1, 'filename', 'error',
            f"Filename '{name}' does not match required pattern YYYY_MM_DD_<agent>_<NN>_<description>.sql"
        ))
        return issues, None

    year, month, day = int(match.group('year')), int(match.group('month')), int(match.group('day'))
    try:
        date(year, month, day)
    except ValueError as exc:
        issues.append(Issue(rel, 1, 'filename', 'error', f"Invalid date in filename '{name}': {exc}"))

    agent = match.group('agent')
    if agent not in known_agents:
        issues.append(Issue(
            rel, 1, 'filename', 'error',
            f"Unknown agent '{agent}' in filename '{name}'; expected one of {tuple(known_agents)}"
        ))
        return issues, None

    date_key = f"{match.group('year')}_{match.group('month')}_{match.group('day')}"
    return issues, (date_key, agent, match.group('seq'))


def split_sql_statements(sql_text: str) -> Tuple[List[Tuple[int, str]], List[Tuple[int, str]]]:
    """Split SQL text into (line, statement) pairs and (line, directive_or_comment_issue) pairs."""
    statements: List[Tuple[int, str]] = []
    directives: List[Tuple[int, str]] = []
    buf: List[str] = []
    stmt_start_line: Optional[int] = None
    line_no = 1
    i = 0
    n = len(sql_text)
    at_line_start = True

    while i < n:
        ch = sql_text[i]
        if ch == '\n':
            if buf:
                buf.append(ch)
            line_no += 1
            at_line_start = True
            i += 1
            continue
        if ch in ' \t\r':
            if buf:
                buf.append(ch)
            i += 1
            continue

        # Check line-start client directives like \! or SOURCE
        if at_line_start and not buf:
            rest_line = sql_text[i:sql_text.find('\n', i) if sql_text.find('\n', i) != -1 else n].strip()
            if rest_line.startswith('\\') or re.match(r'(?i)^SOURCE\b', rest_line):
                directives.append((line_no, rest_line))
                nl = sql_text.find('\n', i)
                if nl == -1:
                    break
                i = nl
                continue

        at_line_start = False

        # Line comment: -- or #
        if sql_text.startswith('--', i) and (i + 2 == n or sql_text[i + 2] in ' \t\r\n-'):
            nl = sql_text.find('\n', i + 2)
            i = n if nl == -1 else nl
            continue
        if ch == '#':
            nl = sql_text.find('\n', i + 1)
            i = n if nl == -1 else nl
            continue

        # Block comment: /* ... */
        if sql_text.startswith('/*', i):
            if sql_text.startswith('/*!', i):
                directives.append((line_no, 'Executable comment /*! ... */ is not allowed'))
            end = sql_text.find('*/', i + 2)
            if end == -1:
                directives.append((line_no, 'Unterminated block comment /*'))
                break
            comment_chunk = sql_text[i:end + 2]
            line_no += comment_chunk.count('\n')
            if buf:
                buf.append(' ')
            i = end + 2
            continue

        if stmt_start_line is None:
            stmt_start_line = line_no

        # Quoted strings and identifiers
        if ch in ("'", '"', '`'):
            quote = ch
            buf.append(ch)
            i += 1
            while i < n:
                c = sql_text[i]
                buf.append(c)
                if c == '\n':
                    line_no += 1
                if c == '\\' and quote in ("'", '"') and i + 1 < n:
                    i += 1
                    buf.append(sql_text[i])
                    if sql_text[i] == '\n':
                        line_no += 1
                    i += 1
                    continue
                if c == quote:
                    if i + 1 < n and sql_text[i + 1] == quote:
                        buf.append(sql_text[i + 1])
                        i += 2
                        continue
                    i += 1
                    break
                i += 1
            continue

        if ch == ';':
            stmt = ''.join(buf).strip()
            if stmt:
                statements.append((stmt_start_line or line_no, stmt))
            buf.clear()
            stmt_start_line = None
            i += 1
            continue

        buf.append(ch)
        i += 1

    trailing = ''.join(buf).strip()
    if trailing:
        statements.append((stmt_start_line or line_no, trailing))
    return statements, directives


def _unquote_ident(token: str) -> str:
    token = token.strip()
    if len(token) >= 2 and token[0] == '`' and token[-1] == '`':
        return token[1:-1].replace('``', '`')
    return token


def _parse_scalar(token: str) -> Any:
    t = token.strip()
    if not t:
        return ''
    if (t.startswith("'") and t.endswith("'")) or (t.startswith('"') and t.endswith('"')):
        q = t[0]
        inner = t[1:-1]
        inner = inner.replace(q + q, q).replace('\\' + q, q).replace('\\n', '\n').replace('\\r', '\r').replace('\\\\', '\\')
        return inner
    upper = t.upper()
    if upper == 'NULL':
        return None
    if re.fullmatch(r'[+-]?\d+', t):
        return int(t)
    if re.fullmatch(r'[+-]?(?:\d+\.\d*|\.\d+)(?:[eE][+-]?\d+)?', t):
        return float(t)
    if re.fullmatch(r'0[xX][0-9a-fA-F]+', t):
        return int(t, 16)
    return t


def _split_top_level_csv(text: str) -> List[str]:
    items: List[str] = []
    cur: List[str] = []
    depth = 0
    i = 0
    n = len(text)
    while i < n:
        ch = text[i]
        if ch in ("'", '"', '`'):
            q = ch
            cur.append(ch)
            i += 1
            while i < n:
                c = text[i]
                cur.append(c)
                if c == '\\' and q in ("'", '"') and i + 1 < n:
                    i += 1
                    cur.append(text[i])
                    i += 1
                    continue
                if c == q:
                    if i + 1 < n and text[i + 1] == q:
                        cur.append(text[i + 1])
                        i += 2
                        continue
                    i += 1
                    break
                i += 1
            continue
        if ch == '(':
            depth += 1
            cur.append(ch)
        elif ch == ')':
            depth = max(0, depth - 1)
            cur.append(ch)
        elif ch == ',' and depth == 0:
            items.append(''.join(cur).strip())
            cur.clear()
        else:
            cur.append(ch)
        i += 1
    tail = ''.join(cur).strip()
    if tail:
        items.append(tail)
    return items


def _extract_value_tuples(values_blob: str) -> List[Tuple[Any, ...]]:
    tuples: List[Tuple[Any, ...]] = []
    i = 0
    n = len(values_blob)
    while i < n:
        while i < n and values_blob[i] in ' \t\r\n,':
            i += 1
        if i >= n or values_blob[i] != '(':
            break
        depth = 0
        start = i + 1
        while i < n:
            ch = values_blob[i]
            if ch in ("'", '"', '`'):
                q = ch
                i += 1
                while i < n:
                    c = values_blob[i]
                    if c == '\\' and q in ("'", '"') and i + 1 < n:
                        i += 2
                        continue
                    if c == q:
                        if i + 1 < n and values_blob[i + 1] == q:
                            i += 2
                            continue
                        i += 1
                        break
                    i += 1
                continue
            if ch == '(':
                depth += 1
            elif ch == ')':
                depth -= 1
                if depth == 0:
                    inner = values_blob[start:i]
                    fields = [_parse_scalar(f) for f in _split_top_level_csv(inner)]
                    tuples.append(tuple(fields))
                    i += 1
                    break
            i += 1
    return tuples


def _strip_subqueries(where_text: str) -> str:
    """Remove parenthesized (SELECT ...) subqueries so only outer-table predicates are matched."""
    out: List[str] = []
    i = 0
    n = len(where_text)
    while i < n:
        if where_text[i] == '(':
            end = _find_matching_paren(where_text, i)
            if end != -1:
                inner = where_text[i + 1:end].lstrip()
                if re.match(r'(?i)^SELECT\b', inner):
                    out.append('()')
                    i = end + 1
                    continue
        out.append(where_text[i])
        i += 1
    return ''.join(out)


def _parse_where_keys(where_text: str) -> Dict[str, Set[Any]]:
    """Extract simple equality, IN (...), and BETWEEN A AND B predicates per column name (lowercase)."""
    keys: Dict[str, Set[Any]] = {}
    if not where_text.strip():
        return keys
    outer_where = _strip_subqueries(where_text)
    # 1. col BETWEEN a AND b
    for m in re.finditer(
        r'(?i)`?([A-Za-z_][A-Za-z0-9_]*)`?\s+BETWEEN\s+([+-]?\d+)\s+AND\s+([+-]?\d+)',
        outer_where
    ):
        col = m.group(1).lower()
        lo, hi = int(m.group(2)), int(m.group(3))
        if 0 <= hi - lo <= 50000:
            keys.setdefault(col, set()).update(range(lo, hi + 1))

    # 2. col IN (...)
    for m in re.finditer(r'(?i)`?([A-Za-z_][A-Za-z0-9_]*)`?\s+IN\s*\(([^()]+)\)', outer_where):
        col = m.group(1).lower()
        vals = {_parse_scalar(v) for v in _split_top_level_csv(m.group(2))}
        keys.setdefault(col, set()).update(vals)

    # 3. col = val (avoid matching bitwise & N = val by checking preceding non-space char)
    for m in re.finditer(
        r"(?i)(?<![&|^+\-*/])`?([A-Za-z_][A-Za-z0-9_]*)`?\s*=\s*('(?:''|\\'|[^'])*'|\"(?:\"\"|\\\"|[^\"])*\"|[+-]?\d+)",
        outer_where
    ):
        col = m.group(1).lower()
        keys.setdefault(col, set()).add(_parse_scalar(m.group(2)))

    return keys


def _parse_create_table_body(body: str) -> Tuple[Dict[str, str], Tuple[str, ...]]:
    columns: Dict[str, str] = {}
    pk: List[str] = []
    for item in _split_top_level_csv(body):
        s = item.strip()
        if not s:
            continue
        pk_match = re.match(r'(?i)^(?:CONSTRAINT\s+`?[A-Za-z0-9_]+`?\s+)?PRIMARY\s+KEY\s*\(([^)]+)\)', s)
        if pk_match:
            for col in _split_top_level_csv(pk_match.group(1)):
                col_name = _unquote_ident(col.split('(')[0]).lower()
                if col_name and col_name not in pk:
                    pk.append(col_name)
            continue
        if re.match(r'(?i)^(?:UNIQUE\s+|FULLTEXT\s+|SPATIAL\s+)?(?:KEY|INDEX|CONSTRAINT|FOREIGN\s+KEY|CHECK)\b', s):
            continue
        col_match = re.match(r'^`?([A-Za-z0-9_]+)`?\s+(.+)$', s, re.S)
        if col_match:
            col_name = col_match.group(1).lower()
            col_def = col_match.group(2)
            columns[col_name] = col_def
            if re.search(r'(?i)\bPRIMARY\s+KEY\b', col_def) and col_name not in pk:
                pk.append(col_name)
    return columns, tuple(pk)


def parse_statement(line: int, text: str) -> Statement:
    s = text.strip()

    # Transaction / session control
    if re.match(r'(?i)^(?:START\s+TRANSACTION|BEGIN|COMMIT|ROLLBACK)\b', s):
        return Statement(text=s, line=line, kind='TRANSACTION')
    if re.match(r'(?i)^SET\s+', s):
        return Statement(text=s, line=line, kind='SET')
    if re.match(r'(?i)^(?:USE|CREATE\s+DATABASE|DROP\s+DATABASE|SOURCE)\b', s):
        return Statement(text=s, line=line, kind='FORBIDDEN_DIRECTIVE')
    if re.match(r'(?i)^SELECT\b', s):
        return Statement(text=s, line=line, kind='SELECT')

    # CREATE TABLE
    m_create = re.match(
        r'(?is)^CREATE\s+(?P<temp>TEMPORARY\s+)?TABLE\s+(?P<ine>IF\s+NOT\s+EXISTS\s+)?(?P<tbl>(?:`?[A-Za-z0-9_]+`?\.)?`?[A-Za-z0-9_]+`?)\s*(?:\((?P<body>.*)\))?',
        s
    )
    if m_create:
        raw_tbl = m_create.group('tbl')
        tbl = _unquote_ident(raw_tbl).lower()
        body = m_create.group('body') or ''
        # Trim trailing ') ENGINE=...' outside outer parens
        close_idx = _find_matching_paren(s, s.find('(')) if '(' in s else -1
        if close_idx != -1:
            body = s[s.find('(') + 1:close_idx]
        cols, pk = _parse_create_table_body(body) if body else ({}, ())
        return Statement(
            text=s, line=line, kind='CREATE_TABLE', table=tbl,
            if_not_exists=bool(m_create.group('ine')),
            temporary=bool(m_create.group('temp')),
            create_columns=cols, primary_key=pk
        )

    # DROP TABLE
    m_drop = re.match(
        r'(?is)^DROP\s+(?P<temp>TEMPORARY\s+)?TABLE\s+(?P<ie>IF\s+EXISTS\s+)?(?P<tbl>`?[A-Za-z0-9_]+`?)',
        s
    )
    if m_drop:
        return Statement(
            text=s, line=line, kind='DROP_TABLE',
            table=_unquote_ident(m_drop.group('tbl')).lower(),
            if_exists=bool(m_drop.group('ie')),
            temporary=bool(m_drop.group('temp'))
        )

    # ALTER TABLE
    m_alter = re.match(r'(?is)^ALTER\s+TABLE\s+(?P<tbl>`?[A-Za-z0-9_]+`?)\s+(?P<ops>.+)$', s)
    if m_alter:
        return Statement(
            text=s, line=line, kind='ALTER_TABLE',
            table=_unquote_ident(m_alter.group('tbl')).lower()
        )

    # TRUNCATE TABLE
    m_trunc = re.match(r'(?is)^TRUNCATE\s+(?:TABLE\s+)?(?P<tbl>`?[A-Za-z0-9_]+`?)$', s)
    if m_trunc:
        return Statement(
            text=s, line=line, kind='DELETE',
            table=_unquote_ident(m_trunc.group('tbl')).lower(),
            where_unscoped=True
        )

    # DELETE FROM
    m_del = re.match(
        r'(?is)^DELETE\s+FROM\s+(?P<tbl>`?[A-Za-z0-9_]+`?)(?:\s+WHERE\s+(?P<where>.+))?$',
        s
    )
    if m_del:
        where_text = (m_del.group('where') or '').strip()
        return Statement(
            text=s, line=line, kind='DELETE',
            table=_unquote_ident(m_del.group('tbl')).lower(),
            where_text=where_text,
            where_keys=_parse_where_keys(where_text),
            where_unscoped=not bool(where_text)
        )

    # INSERT / REPLACE INTO
    m_ins = re.match(
        r'(?is)^(?P<verb>INSERT|REPLACE)\s+(?P<ign>IGNORE\s+)?INTO\s+(?P<tbl>`?[A-Za-z0-9_]+`?)\s*(?P<rest>.+)$',
        s
    )
    if m_ins:
        is_replace = m_ins.group('verb').upper() == 'REPLACE'
        is_ignore = bool(m_ins.group('ign'))
        tbl = _unquote_ident(m_ins.group('tbl')).lower()
        rest = m_ins.group('rest').strip()
        cols: Tuple[str, ...] = ()
        if rest.startswith('('):
            end_col = _find_matching_paren(rest, 0)
            if end_col != -1:
                cols = tuple(_unquote_ident(c).lower() for c in _split_top_level_csv(rest[1:end_col]))
                rest = rest[end_col + 1:].strip()

        has_on_dup = bool(re.search(r'(?i)\bON\s+DUPLICATE\s+KEY\s+UPDATE\b', rest))
        has_not_exists = bool(
            re.match(r'(?is)^SELECT\b', rest)
            and re.search(rf'(?is)\bNOT\s+EXISTS\s*\(\s*SELECT\b[^;]*\bFROM\s+`?{re.escape(tbl)}`?\b', rest)
        )
        rows: List[Tuple[Any, ...]] = []
        m_val = re.match(r'(?is)^VALUES?\s*(?P<vals>.+)$', rest)
        if m_val:
            vals_blob = m_val.group('vals')
            dup_match = re.search(r'(?i)\bON\s+DUPLICATE\s+KEY\s+UPDATE\b', vals_blob)
            if dup_match:
                vals_blob = vals_blob[:dup_match.start()]
            rows = _extract_value_tuples(vals_blob)

        return Statement(
            text=s, line=line, kind='INSERT', table=tbl,
            is_ignore=is_ignore, is_replace=is_replace,
            has_on_duplicate=has_on_dup or has_not_exists, columns=cols, rows=rows
        )

    # UPDATE
    m_upd = re.match(
        r'(?is)^UPDATE\s+(?P<tbl>`?[A-Za-z0-9_]+`?)\s+SET\s+(?P<rest>.+)$',
        s
    )
    if m_upd:
        tbl = _unquote_ident(m_upd.group('tbl')).lower()
        rest = m_upd.group('rest')
        where_idx = _find_top_level_keyword(rest, 'WHERE')
        if where_idx != -1:
            set_part = rest[:where_idx].strip()
            where_part = rest[where_idx + 5:].strip()
        else:
            set_part = rest.strip()
            where_part = ''

        set_cols: List[str] = []
        rel_cols: List[str] = []
        for assign in _split_top_level_csv(set_part):
            if '=' not in assign:
                continue
            lhs, rhs = assign.split('=', 1)
            col_name = _unquote_ident(lhs.strip()).lower()
            if '.' in col_name:
                col_name = _unquote_ident(col_name.split('.')[-1]).lower()
            set_cols.append(col_name)
            # Detect non-idempotent relative self update: col = col + 1 or col = col * 2
            rhs_clean = rhs.strip()
            if re.search(rf'(?i)\b`?{re.escape(col_name)}`?\s*[\+\-\*/]', rhs_clean):
                rel_cols.append(col_name)

        return Statement(
            text=s, line=line, kind='UPDATE', table=tbl,
            where_text=where_part,
            where_keys=_parse_where_keys(where_part),
            where_unscoped=not bool(where_part),
            set_columns=tuple(set_cols),
            relative_self_columns=tuple(rel_cols)
        )

    return Statement(text=s, line=line, kind='OTHER')


def _find_matching_paren(text: str, open_pos: int) -> int:
    if open_pos < 0 or open_pos >= len(text) or text[open_pos] != '(':
        return -1
    depth = 0
    i = open_pos
    n = len(text)
    while i < n:
        ch = text[i]
        if ch in ("'", '"', '`'):
            q = ch
            i += 1
            while i < n:
                c = text[i]
                if c == '\\' and q in ("'", '"') and i + 1 < n:
                    i += 2
                    continue
                if c == q:
                    if i + 1 < n and text[i + 1] == q:
                        i += 2
                        continue
                    i += 1
                    break
                i += 1
            continue
        if ch == '(':
            depth += 1
        elif ch == ')':
            depth -= 1
            if depth == 0:
                return i
        i += 1
    return -1


def _find_top_level_keyword(text: str, keyword: str) -> int:
    depth = 0
    i = 0
    n = len(text)
    kw_len = len(keyword)
    while i < n:
        ch = text[i]
        if ch in ("'", '"', '`'):
            q = ch
            i += 1
            while i < n:
                c = text[i]
                if c == '\\' and q in ("'", '"') and i + 1 < n:
                    i += 2
                    continue
                if c == q:
                    if i + 1 < n and text[i + 1] == q:
                        i += 2
                        continue
                    i += 1
                    break
                i += 1
            continue
        if ch == '(':
            depth += 1
        elif ch == ')':
            depth = max(0, depth - 1)
        elif depth == 0 and text[i:i + kw_len].upper() == keyword.upper():
            before_ok = (i == 0) or (not (text[i - 1].isalnum() or text[i - 1] in '_`'))
            after_pos = i + kw_len
            after_ok = (after_pos == n) or (not (text[after_pos].isalnum() or text[after_pos] in '_`'))
            if before_ok and after_ok:
                return i
        i += 1
    return -1


def check_idempotence(path: Path, statements: Sequence[Statement],
                      directives: Sequence[Tuple[int, str]],
                      known_pk_columns: Optional[Dict[str, Tuple[str, ...]]] = None) -> List[Issue]:
    issues: List[Issue] = []
    rel = _rel_path(path)
    pk_map: Dict[str, Tuple[str, ...]] = dict(known_pk_columns or {})

    for line_no, directive in directives:
        issues.append(Issue(rel, line_no, 'idempotence', 'error',
                            f"Disallowed client directive or comment: {directive}"))

    # Track preceding DELETE / DROP coverage per table within the file
    unscoped_deleted_tables: Set[str] = set()
    deleted_keys_by_table: Dict[str, Dict[str, Set[Any]]] = {}
    dropped_tables: Set[str] = set()
    temporary_tables: Set[str] = set()

    for stmt in statements:
        if stmt.kind == 'FORBIDDEN_DIRECTIVE':
            issues.append(Issue(rel, stmt.line, 'idempotence', 'error',
                                f"Disallowed database/session statement: {stmt.text[:80]}"))
            continue

        if stmt.kind == 'DROP_TABLE':
            if stmt.if_exists or stmt.temporary or stmt.table in temporary_tables:
                dropped_tables.add(stmt.table)
                temporary_tables.discard(stmt.table)
            else:
                issues.append(Issue(rel, stmt.line, 'idempotence', 'error',
                                    f"Bare DROP TABLE `{stmt.table}` without IF EXISTS is not idempotent"))
            continue

        if stmt.kind == 'CREATE_TABLE':
            if stmt.primary_key:
                pk_map[stmt.table] = stmt.primary_key
            if stmt.temporary:
                temporary_tables.add(stmt.table)
            elif not stmt.if_not_exists and stmt.table not in dropped_tables:
                issues.append(Issue(
                    rel, stmt.line, 'idempotence', 'error',
                    f"Bare CREATE TABLE `{stmt.table}` without IF NOT EXISTS is not idempotent"
                ))
            continue

        if stmt.kind == 'DELETE':
            if stmt.where_unscoped:
                unscoped_deleted_tables.add(stmt.table)
            else:
                tbl_keys = deleted_keys_by_table.setdefault(stmt.table, {})
                for col, vals in stmt.where_keys.items():
                    tbl_keys.setdefault(col, set()).update(vals)
            continue

        if stmt.kind == 'INSERT':
            if stmt.is_ignore or stmt.is_replace or stmt.has_on_duplicate:
                continue
            if stmt.table in unscoped_deleted_tables or stmt.table in dropped_tables or stmt.table in temporary_tables:
                continue
            if stmt.table not in deleted_keys_by_table:
                issues.append(Issue(
                    rel, stmt.line, 'idempotence', 'error',
                    f"Bare INSERT INTO `{stmt.table}` without preceding DELETE, ON DUPLICATE KEY UPDATE, or INSERT IGNORE"
                ))
                continue

            # Check if preceding DELETE covers the inserted rows' key column when verifiable
            del_cols = deleted_keys_by_table[stmt.table]
            if stmt.columns and stmt.rows and del_cols:
                # Find any column present in both INSERT columns and DELETE predicates
                shared_cols = [c for c in stmt.columns if c in del_cols and del_cols[c]]
                if shared_cols:
                    col = shared_cols[0]
                    col_idx = stmt.columns.index(col)
                    covered_vals = del_cols[col]
                    uncovered = [
                        row[col_idx] for row in stmt.rows
                        if col_idx < len(row) and row[col_idx] not in covered_vals
                    ]
                    if uncovered:
                        sample = ', '.join(repr(v) for v in uncovered[:5])
                        issues.append(Issue(
                            rel, stmt.line, 'idempotence', 'error',
                            f"INSERT INTO `{stmt.table}` has `{col}` value(s) not covered by preceding DELETE: {sample}"
                        ))
            continue

        if stmt.kind == 'UPDATE':
            if stmt.table in temporary_tables:
                continue
            if stmt.where_unscoped:
                issues.append(Issue(
                    rel, stmt.line, 'idempotence', 'error',
                    f"Unscoped UPDATE on `{stmt.table}` without WHERE clause"
                ))
            if stmt.relative_self_columns:
                # Relative update is only idempotent if WHERE guards the target column explicitly
                unguarded = [c for c in stmt.relative_self_columns if c not in stmt.where_keys]
                if unguarded:
                    issues.append(Issue(
                        rel, stmt.line, 'idempotence', 'error',
                        f"Relative self-update on `{stmt.table}` column(s) {unguarded} is not idempotent on re-run"
                    ))

    return issues


# ---------------------------------------------------------------------------
# WoW / UltimateMoP Adapter: ID Ranges, Cross-Agent Collisions, World DB Refs
# ---------------------------------------------------------------------------

@dataclass(frozen=True)
class SystemIdBlock:
    system: str
    is_placeholder: bool
    ranges: Dict[str, Tuple[int, int]]
    table_prefixes: Tuple[str, ...]


# Mapping of (table_name) -> (id_category, id_column_names, custom_floor)
TABLE_ID_CATEGORY: Dict[str, Tuple[str, Tuple[str, ...], int]] = {
    'quest_template': ('quests', ('id',), 90000),
    'quest_offer_reward': ('quests', ('id',), 90000),
    'quest_request_items': ('quests', ('id',), 90000),
    'quest_template_addon': ('quests', ('id',), 90000),
    'creature_template': ('creatures', ('entry',), 900000),
    'gameobject_template': ('gameobjects', ('entry',), 900000),
    'item_template': ('items', ('entry',), 190000),
    'spell_dbc': ('spells', ('id',), 310000),
    'spellmisc_dbc': ('spells', ('id',), 310000),
    'spelleffect_dbc': ('spells', ('id', 'effectspellid'), 310000),
    'gossip_menu': ('gossip', ('entry',), 90000),
    'gossip_menu_option': ('gossip', ('menu_id',), 90000),
    'npc_text': ('gossip', ('id',), 90000),
    'rbac_permissions': ('gossip', ('id',), 90000),
}

# Default primary keys for common world/auth/characters tables when not specified by CREATE TABLE
DEFAULT_TABLE_PKS: Dict[str, Tuple[str, ...]] = {
    'quest_template': ('id',),
    'quest_offer_reward': ('id',),
    'quest_request_items': ('id',),
    'quest_template_addon': ('id',),
    'creature_template': ('entry',),
    'creature_template_addon': ('entry',),
    'creature_template_model': ('creatureid', 'idx'),
    'creature_template_movement': ('creatureid',),
    'creature_template_difficulty': ('entry', 'difficulty'),
    'creature': ('guid',),
    'creature_addon': ('guid',),
    'creature_equip_template': ('creatureid', 'id'),
    'creature_text': ('entry', 'groupid', 'id'),
    'creature_queststarter': ('id', 'quest'),
    'creature_questender': ('id', 'quest'),
    'gameobject_template': ('entry',),
    'gameobject': ('guid',),
    'gameobject_queststarter': ('id', 'quest'),
    'gameobject_questender': ('id', 'quest'),
    'item_template': ('entry',),
    'hotfix_data': ('id', 'tablehash'),
    'twink_item': ('entry',),
    'spell_dbc': ('id',),
    'spellmisc_dbc': ('id',),
    'spelleffect_dbc': ('id',),
    'spell_script_names': ('spell_id', 'scriptname'),
    'spell_area': ('spell', 'area', 'quest_start', 'aura_spell', 'racemask', 'gender'),
    'npc_text': ('id',),
    'gossip_menu': ('entry', 'text_id'),
    'gossip_menu_option': ('menu_id', 'id'),
    'rbac_permissions': ('id',),
    'rbac_linked_permissions': ('id', 'linkedid'),
    'smart_scripts': ('entryorguid', 'source_type', 'id', 'link'),
    'areatrigger_scripts': ('entry',),
    'trinity_string': ('entry',),
    'heartseed_stage': ('stage',),
    'heartseed_glow': ('stage', 'path'),
    'heartseed_trait': ('id',),
    'heartseed_essence_template': ('id',),
    'heartseed_essence_source': ('essence_id', 'item_entry', 'name_pattern'),
    'heartseed_shrine': ('id',),
    'gmtools_checkpoint': ('name',),
    'gmtools_checkpoint_step': ('checkpoint', 'step'),
    'gmtools_kit': ('name',),
    'gmtools_kit_item': ('kit', 'item', 'class_mask'),
}

# Known stock RBAC permission exceptions documented in auth updates
STOCK_RBAC_ALLOWLIST: Set[int] = {1345}


def parse_id_ranges(markdown_text: str) -> List[SystemIdBlock]:
    blocks: List[SystemIdBlock] = []
    range_re = re.compile(r'(\d+)\s*[–\-]\s*(\d+)')
    for raw_line in markdown_text.splitlines():
        line = raw_line.strip()
        if not line.startswith('|') or not line.endswith('|'):
            continue
        cells = [c.strip() for c in line.strip('|').split('|')]
        if len(cells) < 8:
            continue
        if cells[0].lower() == 'system' or set(cells[0]) <= {'-', ':'}:
            continue
        sys_name = cells[0]
        is_placeholder = 'next system' in sys_name.lower() or '<system>' in cells[7].lower()
        cat_names = ('quests', 'creatures', 'gameobjects', 'items', 'spells', 'gossip')
        ranges: Dict[str, Tuple[int, int]] = {}
        for idx, cat in enumerate(cat_names, start=1):
            m = range_re.search(cells[idx])
            if m:
                ranges[cat] = (int(m.group(1)), int(m.group(2)))
        prefixes: List[str] = []
        for token in re.findall(r'`([A-Za-z0-9_]+)\*`', cells[7]):
            prefixes.append(token.lower())
        blocks.append(SystemIdBlock(
            system=sys_name,
            is_placeholder=is_placeholder,
            ranges=ranges,
            table_prefixes=tuple(prefixes)
        ))
    return blocks


def _infer_file_system(path: Path, statements: Sequence[Statement],
                       blocks: Sequence[SystemIdBlock]) -> Optional[SystemIdBlock]:
    name_lower = path.name.lower()
    active_blocks = [b for b in blocks if not b.is_placeholder]
    # Check table prefixes created or modified in the file
    for stmt in statements:
        if not stmt.table:
            continue
        for b in active_blocks:
            if any(stmt.table.startswith(pfx) for pfx in b.table_prefixes):
                return b
    # Check filename slug against prefixes
    for b in active_blocks:
        for pfx in b.table_prefixes:
            stem = pfx.rstrip('_')
            if stem and re.search(rf'(?:^|_){re.escape(stem)}(?:_|\.|$)', name_lower):
                return b
    return None


def _row_dict(columns: Tuple[str, ...], row: Tuple[Any, ...],
              fallback_cols: Tuple[str, ...] = ()) -> Dict[str, Any]:
    cols = columns if columns else fallback_cols
    return {cols[i]: row[i] for i in range(min(len(cols), len(row)))}


def check_id_ranges_and_collisions(
    file_statements: Sequence[Tuple[Path, Optional[str], Sequence[Statement]]],
    blocks: Sequence[SystemIdBlock],
    enforce_custom_for_agents: bool = True
) -> List[Issue]:
    """Validate IDs against docs/ID_RANGES.md blocks and detect cross-file/cross-agent collisions."""
    issues: List[Issue] = []
    active_blocks = [b for b in blocks if not b.is_placeholder]
    placeholder_blocks = [b for b in blocks if b.is_placeholder]

    # Track (category, id_val) -> (rel_path, agent, line, table)
    seen_category_ids: Dict[Tuple[str, int], Tuple[str, Optional[str], int, str]] = {}
    # Track (table, pk_tuple) -> (rel_path, agent, line, verb)
    seen_table_pks: Dict[Tuple[str, Tuple[Any, ...]], Tuple[str, Optional[str], int, str]] = {}
    learned_pks: Dict[str, Tuple[str, ...]] = dict(DEFAULT_TABLE_PKS)

    for _, _, stmts in file_statements:
        for st in stmts:
            if st.kind == 'CREATE_TABLE' and st.primary_key:
                learned_pks[st.table] = st.primary_key

    for path, agent, statements in file_statements:
        rel = _rel_path(path)
        file_system = _infer_file_system(path, statements, blocks)
        is_agent_file = agent is not None

        for stmt in statements:
            # 1. Check ID range allocation on INSERT into categorized tables
            if stmt.kind == 'INSERT' and stmt.table in TABLE_ID_CATEGORY:
                cat, id_cols, floor = TABLE_ID_CATEGORY[stmt.table]
                fallback = learned_pks.get(stmt.table, id_cols[:1])
                for row in stmt.rows:
                    rdict = _row_dict(stmt.columns, row, fallback)
                    for id_col in id_cols:
                        val = rdict.get(id_col)
                        if not isinstance(val, int):
                            continue
                        if val >= floor:
                            # Must fall into an active block
                            matching_block = next(
                                (b for b in active_blocks if cat in b.ranges and b.ranges[cat][0] <= val <= b.ranges[cat][1]),
                                None
                            )
                            in_placeholder = any(
                                cat in b.ranges and b.ranges[cat][0] <= val <= b.ranges[cat][1]
                                for b in placeholder_blocks
                            )
                            if matching_block is None:
                                hint = ' (lies in unallocated Next system block)' if in_placeholder else ''
                                issues.append(Issue(
                                    rel, stmt.line, 'id_range', 'error',
                                    f"`{stmt.table}`.{id_col} = {val} is not in any allocated `{cat}` range in docs/ID_RANGES.md{hint}"
                                ))
                            elif file_system is not None and matching_block.system != file_system.system:
                                exp_lo, exp_hi = file_system.ranges.get(cat, (0, 0))
                                issues.append(Issue(
                                    rel, stmt.line, 'id_range', 'error',
                                    f"`{stmt.table}`.{id_col} = {val} belongs to '{matching_block.system}', "
                                    f"but file belongs to '{file_system.system}' (expected {exp_lo}–{exp_hi})"
                                ))

                            # Record custom ID in category space for collision detection (once per distinct table/category)
                            if stmt.table in ('quest_template', 'creature_template', 'gameobject_template',
                                              'item_template', 'spell_dbc', 'npc_text', 'gossip_menu', 'rbac_permissions') and id_col == id_cols[0]:
                                key = (f"{cat}:{stmt.table}", val)
                                prev = seen_category_ids.get(key)
                                if prev is not None and prev[0] != rel:
                                    issues.append(Issue(
                                        rel, stmt.line, 'id_collision', 'error',
                                        f"Custom ID collision on `{stmt.table}`.{id_col} = {val} "
                                        f"(also defined in {prev[0]}:{prev[2]})"
                                    ))
                                else:
                                    seen_category_ids[key] = (rel, agent, stmt.line, stmt.table)
                        elif is_agent_file and enforce_custom_for_agents:
                            # Agent file inserting into a template table below custom floor
                            if stmt.table == 'rbac_permissions' and val in STOCK_RBAC_ALLOWLIST:
                                continue
                            if stmt.table in ('quest_template', 'item_template', 'spell_dbc', 'spellmisc_dbc',
                                              'spelleffect_dbc', 'npc_text', 'creature_template', 'gameobject_template'):
                                issues.append(Issue(
                                    rel, stmt.line, 'id_range', 'error',
                                    f"Agent update inserts non-reserved ID `{stmt.table}`.{id_col} = {val} below custom floor {floor}"
                                ))

            # 2. Check cross-file / cross-agent row primary-key collisions on INSERT and UPDATE
            pk_cols = learned_pks.get(stmt.table)
            if not pk_cols:
                continue

            if stmt.kind == 'INSERT':
                for row in stmt.rows:
                    rdict = _row_dict(stmt.columns, row, pk_cols)
                    if all(c in rdict for c in pk_cols):
                        pk_tuple = tuple(rdict[c] for c in pk_cols)
                        map_key = (stmt.table, pk_tuple)
                        prev = seen_table_pks.get(map_key)
                        if prev is not None and prev[0] != rel:
                            # Flag collisions between agent files
                            if is_agent_file and prev[1] is not None:
                                issues.append(Issue(
                                    rel, stmt.line, 'id_collision', 'error',
                                    f"Row key collision on `{stmt.table}` {pk_cols}={pk_tuple} with {prev[0]}:{prev[2]}"
                                ))
                        else:
                            seen_table_pks[map_key] = (rel, agent, stmt.line, 'INSERT')

            elif stmt.kind == 'UPDATE' and is_agent_file and len(pk_cols) == 1:
                pk_col = pk_cols[0]
                for pk_val in stmt.where_keys.get(pk_col, ()):
                    map_key = (stmt.table, (pk_val,))
                    prev = seen_table_pks.get(map_key)
                    if prev is not None and prev[0] != rel and prev[1] is not None and prev[1] != agent:
                        issues.append(Issue(
                            rel, stmt.line, 'id_collision', 'error',
                            f"Cross-agent modification collision on `{stmt.table}`.{pk_col}={pk_val} "
                            f"between agent '{agent}' ({rel}:{stmt.line}) and agent '{prev[1]}' ({prev[0]}:{prev[2]})"
                        ))
                    elif prev is None:
                        seen_table_pks[map_key] = (rel, agent, stmt.line, 'UPDATE')

    return issues


class WorldReferenceCatalog:
    """Holds valid entity IDs and table schemas from a world DB, DBC files, and the SQL corpus."""

    def __init__(
        self,
        quests: Optional[Set[int]] = None,
        creatures: Optional[Set[int]] = None,
        gameobjects: Optional[Set[int]] = None,
        items: Optional[Set[int]] = None,
        spells: Optional[Set[int]] = None,
        table_columns: Optional[Dict[str, Set[str]]] = None,
    ):
        self.quests: Set[int] = set(quests or ())
        self.creatures: Set[int] = set(creatures or ())
        self.gameobjects: Set[int] = set(gameobjects or ())
        self.items: Set[int] = set(items or ())
        self.spells: Set[int] = set(spells or ())
        self.table_columns: Dict[str, Set[str]] = {k.lower(): {c.lower() for c in v}
                                                   for k, v in (table_columns or {}).items()}

    @classmethod
    def from_mysql_and_dbc(
        cls,
        world_db: str,
        *,
        mysql_bin: str = '',
        db_host: str = '127.0.0.1',
        db_port: int = 3306,
        db_user: str = 'ultimatemop',
        db_password: str = '',
        dbc_dir: Optional[Path] = None,
        extra_dbs: Sequence[str] = (),
    ) -> 'WorldReferenceCatalog':
        if not IDENT_RE.fullmatch(world_db):
            raise ValueError(f'Invalid world database identifier: {world_db}')

        def run_query(sql: str, db: Optional[str] = None) -> List[str]:
            suffix = '.exe' if os.name == 'nt' else ''
            exe = str(Path(mysql_bin) / ('mysql' + suffix)) if mysql_bin else ('mysql' + suffix)
            cmd = [exe, '--no-defaults', '--protocol=TCP', '-h', db_host, '-P', str(db_port),
                   '-u', db_user, '--batch', '--skip-column-names', '--binary-mode']
            if db:
                cmd.append(db)
            env = os.environ.copy()
            if db_password:
                env['MYSQL_PWD'] = db_password
            res = subprocess.run(cmd, input=sql.encode('utf-8'), stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=env)
            if res.returncode != 0:
                raise RuntimeError(f"MySQL query failed: {res.stderr.decode(errors='replace')[-1000:]}")
            return [line.strip() for line in res.stdout.decode('utf-8', errors='replace').splitlines() if line.strip()]

        # Load schema columns for world_db + any extra_dbs (e.g. characters_gemini, auth_gemini)
        schemas = [world_db] + [d for d in extra_dbs if IDENT_RE.fullmatch(d)]
        in_list = ', '.join(f"'{s}'" for s in schemas)
        col_rows = run_query(
            f"SELECT LOWER(TABLE_NAME), LOWER(COLUMN_NAME) FROM information_schema.COLUMNS WHERE TABLE_SCHEMA IN ({in_list});"
        )
        table_columns: Dict[str, Set[str]] = {}
        for row in col_rows:
            parts = row.split('\t')
            if len(parts) == 2:
                table_columns.setdefault(parts[0], set()).add(parts[1])

        def load_ids(table: str, col: str) -> Set[int]:
            t_low, c_low = table.lower(), col.lower()
            if t_low not in table_columns or c_low not in table_columns[t_low]:
                return set()
            lines = run_query(f"SELECT `{col}` FROM `{table}`;", world_db)
            return {int(x) for x in lines if re.fullmatch(r'[+-]?\d+', x)}

        quests = load_ids('quest_template', 'ID')
        creatures = load_ids('creature_template', 'entry')
        gameobjects = load_ids('gameobject_template', 'entry')
        items = load_ids('item_template', 'entry')
        spells = load_ids('spell_dbc', 'Id')
        # Also include spell IDs known in world tables in case DBC dir is not passed
        for spell_tbl, spell_col in (
            ('spell_script_names', 'spell_id'),
            ('spell_proc', 'spellId'),
            ('spell_bonus_data', 'entry'),
            ('spell_custom_attribute', 'entry'),
            ('player_class_spells', 'spellId'),
        ):
            for s_id in load_ids(spell_tbl, spell_col):
                spells.add(abs(s_id))

        if dbc_dir is not None:
            spell_dbc_path = Path(dbc_dir) / 'Spell.dbc'
            if spell_dbc_path.is_file():
                spells.update(load_wdbc_first_uint32_ids(spell_dbc_path))

        return cls(
            quests=quests,
            creatures=creatures,
            gameobjects=gameobjects,
            items=items,
            spells=spells,
            table_columns=table_columns,
        )

    def absorb_statements(self, statements: Sequence[Statement]) -> None:
        """Register tables and entity IDs created/inserted by the SQL corpus itself."""
        for stmt in statements:
            if stmt.kind == 'CREATE_TABLE' and stmt.create_columns:
                self.table_columns.setdefault(stmt.table, set()).update(stmt.create_columns.keys())
            elif stmt.kind == 'INSERT':
                fallback = DEFAULT_TABLE_PKS.get(stmt.table, ('id',))
                for row in stmt.rows:
                    rdict = _row_dict(stmt.columns, row, fallback)
                    if stmt.table == 'quest_template' and isinstance(rdict.get('id'), int):
                        self.quests.add(rdict['id'])
                    elif stmt.table == 'creature_template' and isinstance(rdict.get('entry'), int):
                        self.creatures.add(rdict['entry'])
                    elif stmt.table == 'gameobject_template' and isinstance(rdict.get('entry'), int):
                        self.gameobjects.add(rdict['entry'])
                    elif stmt.table == 'item_template' and isinstance(rdict.get('entry'), int):
                        self.items.add(rdict['entry'])
                    elif stmt.table == 'spell_dbc' and isinstance(rdict.get('id'), int):
                        self.spells.add(rdict['id'])


def load_wdbc_first_uint32_ids(path: Path) -> Set[int]:
    with path.open('rb') as stream:
        header = stream.read(20)
        if len(header) < 20:
            raise ValueError(f'DBC header too short: {path}')
        magic, record_count, _, record_size, _ = struct.unpack('<4s4I', header)
        if magic != b'WDBC' or record_size < 4:
            raise ValueError(f'Invalid WDBC file: {path}')
        body = stream.read(record_count * record_size)
    return {struct.unpack_from('<I', body, i * record_size)[0] for i in range(record_count)}


def check_schema_and_references(
    path: Path,
    statements: Sequence[Statement],
    catalog: WorldReferenceCatalog,
    check_schema: bool = True,
) -> List[Issue]:
    """Validate column names, row arity, and references to quest givers, quests, items, creatures, and spells."""
    issues: List[Issue] = []
    rel = _rel_path(path)

    for stmt in statements:
        # 1. Schema check (columns and INSERT value arity)
        if stmt.kind == 'INSERT' and stmt.columns:
            for row_idx, row in enumerate(stmt.rows, start=1):
                if len(row) != len(stmt.columns):
                    issues.append(Issue(
                        rel, stmt.line, 'schema', 'error',
                        f"INSERT INTO `{stmt.table}` row {row_idx} has {len(row)} value(s) for {len(stmt.columns)} column(s)"
                    ))
        if check_schema and stmt.table and stmt.table in catalog.table_columns:
            valid_cols = catalog.table_columns[stmt.table]
            if stmt.kind == 'INSERT' and stmt.columns and valid_cols:
                for col in stmt.columns:
                    if col not in valid_cols:
                        issues.append(Issue(
                            rel, stmt.line, 'schema', 'error',
                            f"Unknown column `{col}` in INSERT INTO `{stmt.table}`"
                        ))
            elif stmt.kind == 'UPDATE' and valid_cols:
                for col in stmt.set_columns:
                    if col not in valid_cols:
                        issues.append(Issue(
                            rel, stmt.line, 'schema', 'error',
                            f"Unknown column `{col}` in UPDATE `{stmt.table}` SET"
                        ))
                for col in stmt.where_keys:
                    if col not in valid_cols:
                        issues.append(Issue(
                            rel, stmt.line, 'schema', 'error',
                            f"Unknown column `{col}` in UPDATE `{stmt.table}` WHERE"
                        ))

        # 2. Foreign-key reference checks on INSERT, UPDATE, and quest-giver DELETE
        if stmt.kind == 'INSERT':
            fallback = DEFAULT_TABLE_PKS.get(stmt.table, ())
            for row in stmt.rows:
                rdict = _row_dict(stmt.columns, row, fallback)
                _check_row_references(rel, stmt.line, stmt.table, rdict, catalog, issues)
        elif stmt.kind in ('UPDATE', 'DELETE'):
            _check_update_references(rel, stmt.line, stmt, catalog, issues)

    return issues


def _check_row_references(
    rel: str,
    line: int,
    table: str,
    rdict: Dict[str, Any],
    catalog: WorldReferenceCatalog,
    issues: List[Issue],
) -> None:
    def require_creature(val: Any, label: str) -> None:
        if isinstance(val, int) and val > 0 and catalog.creatures and val not in catalog.creatures:
            issues.append(Issue(rel, line, 'reference', 'error',
                                f"`{table}`.{label} references missing creature_template entry {val}"))

    def require_gameobject(val: Any, label: str) -> None:
        if isinstance(val, int) and val > 0 and catalog.gameobjects and val not in catalog.gameobjects:
            issues.append(Issue(rel, line, 'reference', 'error',
                                f"`{table}`.{label} references missing gameobject_template entry {val}"))

    def require_quest(val: Any, label: str) -> None:
        if isinstance(val, int) and val > 0 and catalog.quests and val not in catalog.quests:
            issues.append(Issue(rel, line, 'reference', 'error',
                                f"`{table}`.{label} references missing quest_template ID {val}"))

    def require_item(val: Any, label: str) -> None:
        if isinstance(val, int) and val > 0 and catalog.items and val not in catalog.items:
            issues.append(Issue(rel, line, 'reference', 'error',
                                f"`{table}`.{label} references missing item_template entry {val}"))

    def require_spell(val: Any, label: str) -> None:
        if isinstance(val, int) and val > 0 and catalog.spells and val not in catalog.spells:
            issues.append(Issue(rel, line, 'reference', 'error',
                                f"`{table}`.{label} references missing spell ID {val}"))

    # Quest givers / enders
    if table in ('creature_queststarter', 'creature_questender'):
        require_creature(rdict.get('id'), 'id (quest giver/ender)')
        require_quest(rdict.get('quest'), 'quest')
    elif table in ('gameobject_queststarter', 'gameobject_questender'):
        require_gameobject(rdict.get('id'), 'id (gameobject quest giver/ender)')
        require_quest(rdict.get('quest'), 'quest')

    # Quest companion tables
    elif table in ('quest_offer_reward', 'quest_request_items'):
        require_quest(rdict.get('id'), 'ID')
    elif table == 'quest_template_addon':
        require_quest(rdict.get('id'), 'ID')
        require_quest(rdict.get('prevquestid'), 'PrevQuestID')
        require_quest(rdict.get('nextquestid'), 'NextQuestID')
    elif table == 'quest_template':
        for col in ('startitem', 'rewarditemid1', 'rewarditemid2', 'rewarditemid3', 'rewarditemid4',
                    'rewardchoiceitemid1', 'rewardchoiceitemid2', 'rewardchoiceitemid3',
                    'rewardchoiceitemid4', 'rewardchoiceitemid5', 'rewardchoiceitemid6',
                    'requireditemid1', 'requireditemid2', 'requireditemid3',
                    'requireditemid4', 'requireditemid5', 'requireditemid6'):
            require_item(rdict.get(col), col)

    # Items & custom item tables
    elif table == 'twink_item':
        require_item(rdict.get('entry'), 'entry')
    elif table == 'gmtools_kit_item':
        require_item(rdict.get('item'), 'item')
    elif table == 'heartseed_essence_source':
        require_item(rdict.get('item_entry'), 'item_entry')
    elif table == 'hotfix_data':
        table_hash = rdict.get('tablehash')
        deleted = rdict.get('deleted', 0)
        if table_hash in (2442913102, 3562939398) and not deleted:
            require_item(rdict.get('id'), 'Id')
    elif table in ('creature_loot_template', 'gameobject_loot_template', 'item_loot_template',
                   'fishing_loot_template', 'reference_loot_template', 'spell_loot_template',
                   'disenchant_loot_template', 'prospecting_loot_template', 'milling_loot_template',
                   'pickpocketing_loot_template', 'skinning_loot_template', 'mail_loot_template'):
        if table == 'creature_loot_template':
            require_creature(rdict.get('entry'), 'entry')
        elif table == 'item_loot_template':
            require_item(rdict.get('entry'), 'entry')
        mincount_or_ref = rdict.get('mincountorref', 1)
        if isinstance(mincount_or_ref, int) and mincount_or_ref > 0:
            require_item(rdict.get('item'), 'item')
    elif table == 'npc_vendor':
        require_creature(rdict.get('entry'), 'entry')
        vtype = rdict.get('type', 1)
        if vtype == 1:
            require_item(rdict.get('item'), 'item')

    # Creatures & GameObjects
    elif table == 'creature':
        require_creature(rdict.get('id'), 'id')
    elif table == 'gameobject':
        require_gameobject(rdict.get('id'), 'id')
    elif table in ('creature_template_addon', 'creature_text'):
        require_creature(rdict.get('entry'), 'entry')
    elif table in ('creature_template_model', 'creature_equip_template'):
        require_creature(rdict.get('creatureid') or rdict.get('entry'), 'CreatureID')
    elif table == 'creature_template_movement':
        require_creature(rdict.get('creatureid'), 'CreatureId')
    elif table == 'creature_template_difficulty':
        require_creature(rdict.get('entry'), 'Entry')
    elif table == 'vehicle_template_accessory':
        require_creature(rdict.get('entry'), 'entry')
        require_creature(rdict.get('accessory_entry'), 'accessory_entry')
    elif table == 'npc_trainer':
        entry = rdict.get('entry') or rdict.get('id')
        if isinstance(entry, int) and 0 < entry < 200000:
            require_creature(entry, 'entry')
        spell = rdict.get('spell') or rdict.get('spellid')
        if isinstance(spell, int) and spell > 0:
            require_spell(spell, 'spell')
    elif table == 'achievement_reward':
        require_item(rdict.get('item'), 'item')
        require_spell(rdict.get('spell'), 'spell')
        require_creature(rdict.get('sender'), 'sender')
    elif table == 'npc_spellclick_spells':
        require_creature(rdict.get('npc_entry'), 'npc_entry')
        require_spell(rdict.get('spell_id'), 'spell_id')
    elif table == 'smart_scripts':
        source_type = rdict.get('source_type')
        entryorguid = rdict.get('entryorguid')
        if source_type == 0 and isinstance(entryorguid, int) and entryorguid > 0:
            require_creature(entryorguid, 'entryorguid')
        elif source_type == 1 and isinstance(entryorguid, int) and entryorguid > 0:
            require_gameobject(entryorguid, 'entryorguid')

    # Spells
    elif table == 'heartseed_trait':
        require_spell(rdict.get('spell_id'), 'spell_id')
    elif table == 'heartseed_essence_template':
        require_spell(rdict.get('proc_spell_id'), 'proc_spell_id')
    elif table == 'spelleffect_dbc':
        require_spell(rdict.get('effectspellid'), 'EffectSpellId')
        require_spell(rdict.get('effecttriggerspell'), 'EffectTriggerSpell')
    elif table == 'spell_script_names':
        spell_id = rdict.get('spell_id')
        if isinstance(spell_id, int) and spell_id > 0:
            require_spell(spell_id, 'spell_id')
    elif table == 'spell_area':
        require_spell(rdict.get('spell'), 'spell')
    elif table == 'spell_custom_attribute':
        require_spell(rdict.get('entry'), 'entry')
    elif table == 'spell_linked_spell':
        for col in ('spell_trigger', 'spell_effect'):
            val = rdict.get(col)
            if isinstance(val, int) and val != 0:
                require_spell(abs(val), col)
    elif table == 'playercreateinfo_action':
        if rdict.get('type') == 0:
            require_spell(rdict.get('action'), 'action')


def _check_update_references(
    rel: str,
    line: int,
    stmt: Statement,
    catalog: WorldReferenceCatalog,
    issues: List[Issue],
) -> None:
    if stmt.table == 'item_template' and catalog.items:
        for entry in stmt.where_keys.get('entry', ()):
            if isinstance(entry, int) and entry > 0 and entry not in catalog.items:
                issues.append(Issue(rel, line, 'reference', 'error',
                                    f"UPDATE `item_template` references missing item entry {entry}"))
    elif stmt.table == 'creature_template' and catalog.creatures:
        for entry in stmt.where_keys.get('entry', ()):
            if isinstance(entry, int) and entry > 0 and entry not in catalog.creatures:
                issues.append(Issue(rel, line, 'reference', 'error',
                                    f"UPDATE `creature_template` references missing creature entry {entry}"))
    elif stmt.table == 'gameobject_template' and catalog.gameobjects:
        for entry in stmt.where_keys.get('entry', ()):
            if isinstance(entry, int) and entry > 0 and entry not in catalog.gameobjects:
                issues.append(Issue(rel, line, 'reference', 'error',
                                    f"UPDATE `gameobject_template` references missing gameobject entry {entry}"))
    elif stmt.table in ('quest_template', 'quest_template_addon') and catalog.quests:
        for qid in stmt.where_keys.get('id', ()):
            if isinstance(qid, int) and qid > 0 and qid not in catalog.quests:
                issues.append(Issue(rel, line, 'reference', 'error',
                                    f"UPDATE `{stmt.table}` references missing quest ID {qid}"))
    elif stmt.table in ('creature_queststarter', 'creature_questender'):
        for cid in stmt.where_keys.get('id', ()):
            if isinstance(cid, int) and cid > 0 and catalog.creatures and cid not in catalog.creatures:
                issues.append(Issue(rel, line, 'reference', 'error',
                                    f"{stmt.kind} `{stmt.table}` references missing quest giver/ender creature {cid}"))
        for qid in stmt.where_keys.get('quest', ()):
            if isinstance(qid, int) and qid > 0 and catalog.quests and qid not in catalog.quests:
                issues.append(Issue(rel, line, 'reference', 'error',
                                    f"{stmt.kind} `{stmt.table}` references missing quest ID {qid}"))
    elif stmt.table in ('gameobject_queststarter', 'gameobject_questender'):
        for gid in stmt.where_keys.get('id', ()):
            if isinstance(gid, int) and gid > 0 and catalog.gameobjects and gid not in catalog.gameobjects:
                issues.append(Issue(rel, line, 'reference', 'error',
                                    f"{stmt.kind} `{stmt.table}` references missing quest giver/ender gameobject {gid}"))
        for qid in stmt.where_keys.get('quest', ()):
            if isinstance(qid, int) and qid > 0 and catalog.quests and qid not in catalog.quests:
                issues.append(Issue(rel, line, 'reference', 'error',
                                    f"{stmt.kind} `{stmt.table}` references missing quest ID {qid}"))
    elif stmt.table == 'creature_loot_template' and stmt.kind == 'UPDATE':
        for entry in stmt.where_keys.get('entry', ()):
            if isinstance(entry, int) and entry > 0 and catalog.creatures and entry not in catalog.creatures:
                issues.append(Issue(rel, line, 'reference', 'error',
                                    f"UPDATE `creature_loot_template` references missing creature entry {entry}"))
        mincounts = stmt.where_keys.get('mincountorref', {1})
        if any(isinstance(m, int) and m > 0 for m in mincounts):
            for item in stmt.where_keys.get('item', ()):
                if isinstance(item, int) and item > 0 and catalog.items and item not in catalog.items:
                    issues.append(Issue(rel, line, 'reference', 'error',
                                        f"UPDATE `creature_loot_template` references missing item entry {item}"))


def _rel_path(path: Path) -> str:
    try:
        return path.resolve().relative_to(ROOT).as_posix()
    except ValueError:
        return path.as_posix()


def validate_files(
    paths: Sequence[Path],
    *,
    id_ranges_path: Optional[Path] = None,
    known_agents: Sequence[str] = DEFAULT_AGENTS,
    allow_legacy_before: Optional[date] = None,
    catalog: Optional[WorldReferenceCatalog] = None,
    check_crlf: bool = True,
    check_schema: bool = True,
) -> List[Issue]:
    """Run all configured SQL validation checks across a list of SQL files."""
    issues: List[Issue] = []
    ranges_file = id_ranges_path or (ROOT / 'docs/ID_RANGES.md')
    blocks = parse_id_ranges(ranges_file.read_text(encoding='utf-8')) if ranges_file.is_file() else []

    file_statements: List[Tuple[Path, Optional[str], List[Statement]]] = []
    seen_sequences: Dict[Tuple[str, str, str], str] = {}
    learned_pks: Dict[str, Tuple[str, ...]] = dict(DEFAULT_TABLE_PKS)

    for raw_path in paths:
        path = Path(raw_path)
        rel = _rel_path(path)
        raw_bytes = path.read_bytes()
        if check_crlf:
            issues.extend(check_line_endings(path, raw_bytes))

        fn_issues, seq_meta = check_filename(path, known_agents=known_agents, allow_legacy_before=allow_legacy_before)
        issues.extend(fn_issues)
        agent_name: Optional[str] = None
        if seq_meta is not None:
            _, agent_name, _ = seq_meta
            prev_file = seen_sequences.get(seq_meta)
            if prev_file is not None and prev_file != rel:
                issues.append(Issue(
                    rel, 1, 'filename', 'error',
                    f"Duplicate agent sequence number {seq_meta[0]}_{seq_meta[1]}_{seq_meta[2]} (also used by {prev_file})"
                ))
            else:
                seen_sequences[seq_meta] = rel

        text = raw_bytes.decode('utf-8-sig', errors='replace')
        raw_stmts, directives = split_sql_statements(text)
        parsed_stmts = [parse_statement(ln, st) for ln, st in raw_stmts]
        for st in parsed_stmts:
            if st.kind == 'CREATE_TABLE' and st.primary_key:
                learned_pks[st.table] = st.primary_key

        issues.extend(check_idempotence(path, parsed_stmts, directives, known_pk_columns=learned_pks))
        file_statements.append((path, agent_name, parsed_stmts))

    if blocks:
        issues.extend(check_id_ranges_and_collisions(file_statements, blocks))

    if catalog is not None:
        for _, _, parsed_stmts in file_statements:
            catalog.absorb_statements(parsed_stmts)
        for path, _, parsed_stmts in file_statements:
            issues.extend(check_schema_and_references(path, parsed_stmts, catalog, check_schema=check_schema))

    return issues


def expand_inputs(patterns: Sequence[str]) -> List[Path]:
    found: List[Path] = []
    seen: Set[Path] = set()
    for pat in patterns:
        p = Path(pat)
        if p.is_dir():
            candidates = sorted(p.rglob('*.sql'))
        elif any(ch in pat for ch in '*?['):
            candidates = [Path(m) for m in sorted(glob.glob(pat, recursive=True)) if Path(m).is_file()]
        else:
            if not p.is_file():
                raise ValueError(f'SQL path not found: {pat}')
            candidates = [p]
        for c in candidates:
            resolved = c.resolve()
            if resolved not in seen:
                seen.add(resolved)
                found.append(c)
    return found


def main(argv: Optional[Sequence[str]] = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('paths', nargs='+', help='SQL files, directories, or glob patterns to validate')
    parser.add_argument('--id-ranges', default=str(ROOT / 'docs/ID_RANGES.md'), help='Path to docs/ID_RANGES.md')
    parser.add_argument('--agents', default=','.join(DEFAULT_AGENTS), help='Comma-separated valid agent names')
    parser.add_argument('--legacy-before', help='Allow pre-fork upstream filenames on or before YYYY-MM-DD')
    parser.add_argument('--ignore-crlf', action='store_true', help='Skip CRLF check (e.g. on autocrlf checkouts)')
    parser.add_argument('--world-db', default=os.environ.get('MOP_WORLD_DB', ''),
                        help='World database name for schema and foreign-reference checks')
    parser.add_argument('--extra-db', action='append', default=[],
                        help='Additional database(s) for schema column lookup (e.g. characters_gemini, auth_gemini)')
    parser.add_argument('--dbc-dir', default=os.environ.get('MOP_DBC_DIR', ''),
                        help='DBC folder containing Spell.dbc for client spell ID verification')
    parser.add_argument('--mysql-bin', default=os.environ.get('MOP_MYSQL_BIN', ''))
    parser.add_argument('--db-host', default=os.environ.get('MOP_DB_HOST', '127.0.0.1'))
    parser.add_argument('--db-port', type=int, default=int(os.environ.get('MOP_DB_PORT', '3306')))
    parser.add_argument('--db-user', default=os.environ.get('MOP_DB_USER', 'ultimatemop'))
    parser.add_argument('--db-password-env', default='MOP_DB_PASSWORD')
    parser.add_argument('--json', action='store_true', help='Output JSON report')
    args = parser.parse_args(argv)

    files = expand_inputs(args.paths)
    if not files:
        raise ValueError('No SQL files matched the provided paths')

    legacy_date = date.fromisoformat(args.legacy_before) if args.legacy_before else None
    agents = tuple(a.strip() for a in args.agents.split(',') if a.strip())

    catalog: Optional[WorldReferenceCatalog] = None
    if args.world_db:
        dbc_path = Path(args.dbc_dir) if args.dbc_dir else None
        catalog = WorldReferenceCatalog.from_mysql_and_dbc(
            args.world_db,
            mysql_bin=args.mysql_bin,
            db_host=args.db_host,
            db_port=args.db_port,
            db_user=args.db_user,
            db_password=os.environ.get(args.db_password_env, ''),
            dbc_dir=dbc_path,
            extra_dbs=args.extra_db,
        )

    issues = validate_files(
        files,
        id_ranges_path=Path(args.id_ranges),
        known_agents=agents,
        allow_legacy_before=legacy_date,
        catalog=catalog,
        check_crlf=not args.ignore_crlf,
    )

    if args.json:
        report = {
            'files_checked': len(files),
            'issue_count': len(issues),
            'ok': len(issues) == 0,
            'issues': [i.to_dict() for i in issues],
        }
        print(json.dumps(report, indent=2))
    else:
        for issue in issues:
            print(f"{issue.file}:{issue.line}: [{issue.category}] {issue.message}")
        print(f"Checked {len(files)} file(s): {len(issues)} issue(s) found.")

    return 1 if issues else 0


if __name__ == '__main__':
    sys.exit(main())
