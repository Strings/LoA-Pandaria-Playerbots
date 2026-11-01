"""Binary WDBC and WDB2 reader library with TrinityCore format and named schema support."""
from dataclasses import dataclass
import os
from pathlib import Path
import struct
from typing import Any, Dict, Iterator, List, Optional, Sequence, Tuple, Union

from .schemas import FieldSpec, TableSchema, get_schema

WDBC_MAGIC = b'WDBC'
WDB2_MAGIC = b'WDB2'
TRINITY_FMT_MAP = {
    'n': ('uint32', 'I', 4),
    'd': ('uint32', 'I', 4),
    'i': ('int32', 'i', 4),
    'x': ('uint32', 'I', 4),
    'f': ('float', 'f', 4),
    's': ('string', 'I', 4),
    'b': ('uint8', 'B', 1),
    'l': ('uint8', 'B', 1),
    'X': ('uint8', 'B', 1),
}
KIND_TO_STRUCT = {
    'uint32': ('I', 4),
    'int32': ('i', 4),
    'float': ('f', 4),
    'string': ('I', 4),
    'uint8': ('B', 1),
}


@dataclass(frozen=True)
class DbcHeader:
    magic: str
    record_count: int
    field_count: int
    record_size: int
    string_size: int
    table_hash: int = 0
    build: int = 0
    timestamp: int = 0
    min_index: int = 0
    max_index: int = 0
    locale: int = 0
    copy_table_size: int = 0
    data_offset: int = 20


def parse_header(raw: Union[bytes, memoryview]) -> DbcHeader:
    """Parse a 20-byte WDBC or 32/48-byte WDB2 header."""
    if len(raw) < 20:
        raise ValueError(f'File too small for DBC/DB2 header ({len(raw)} bytes)')
    magic = bytes(raw[:4])
    if magic == WDBC_MAGIC:
        rc, fc, rs, ss = struct.unpack_from('<4I', raw, 4)
        return DbcHeader(
            magic='WDBC',
            record_count=rc,
            field_count=fc,
            record_size=rs,
            string_size=ss,
            data_offset=20,
        )
    if magic == WDB2_MAGIC:
        if len(raw) < 32:
            raise ValueError(f'File too small for WDB2 header ({len(raw)} bytes)')
        rc, fc, rs, ss, th, build, ts = struct.unpack_from('<7I', raw, 4)
        offset = 32
        min_idx = max_idx = loc = unk5 = 0
        if build > 12880:
            if len(raw) < 48:
                raise ValueError(f'File too small for extended WDB2 header ({len(raw)} bytes)')
            min_idx, max_idx, loc, unk5 = struct.unpack_from('<4i', raw, 32)
            offset = 48
        if max_idx != 0:
            diff = max_idx - min_idx + 1
            if diff < 0:
                raise ValueError(f'Invalid WDB2 index range: min_index={min_idx}, max_index={max_idx}')
            offset += diff * 6
        return DbcHeader(
            magic='WDB2',
            record_count=rc,
            field_count=fc,
            record_size=rs,
            string_size=ss,
            table_hash=th,
            build=build,
            timestamp=ts,
            min_index=min_idx,
            max_index=max_idx,
            locale=loc,
            copy_table_size=unk5,
            data_offset=offset,
        )
    raise ValueError(f'Unsupported DBC/DB2 magic {magic!r}; expected WDBC or WDB2')


def schema_from_trinity_fmt(fmt: str, name: str = 'Custom') -> TableSchema:
    fields: List[FieldSpec] = []
    key_field = 'col_0'
    for idx, ch in enumerate(fmt):
        if ch not in TRINITY_FMT_MAP:
            raise ValueError(f"Unsupported TrinityCore format character {ch!r} at index {idx} in {fmt!r}")
        kind, _, _ = TRINITY_FMT_MAP[ch]
        fname = 'id' if (ch in ('n', 'd') and key_field == 'col_0') else f'col_{idx}'
        if fname == 'id':
            key_field = 'id'
        fields.append(FieldSpec(name=fname, kind=kind))
    return TableSchema(name=name, filename=name, fields=tuple(fields), key_field=key_field, trinity_fmt=fmt)


def default_schema_for_header(header: DbcHeader, name: str = 'Raw') -> TableSchema:
    fields = [FieldSpec(name='id' if i == 0 else f'col_{i}', kind='uint32') for i in range(header.field_count)]
    return TableSchema(
        name=name,
        filename=name,
        fields=tuple(fields),
        key_field='id' if header.field_count > 0 else '',
        trinity_fmt='n' + 'i' * max(0, header.field_count - 1),
    )


class DbcRecord:
    """Lazy view over a single DBC/DB2 row with typed and named field access."""
    __slots__ = ('_table', '_row_index', '_row_offset', '_values')

    def __init__(self, table: 'DbcTable', row_index: int, row_offset: int,
                 values: Optional[Tuple[Any, ...]] = None) -> None:
        self._table = table
        self._row_index = row_index
        self._row_offset = row_offset
        self._values = values

    @property
    def row_index(self) -> int:
        return self._row_index

    def _ensure_values(self) -> Tuple[Any, ...]:
        if self._values is None:
            self._values = self._table._unpack_row(self._row_offset)
        return self._values

    def _resolve_col(self, col: Union[int, str]) -> int:
        if isinstance(col, int):
            if 0 <= col < self._table.header.field_count:
                return col
            raise IndexError(f'Column index {col} out of range (0..{self._table.header.field_count - 1})')
        idx = self._table._name_to_index.get(col)
        if idx is None:
            raise KeyError(f"Unknown field {col!r} in table {self._table.name!r}")
        return idx

    def get_uint(self, col: Union[int, str]) -> int:
        idx = self._resolve_col(col)
        off = self._row_offset + self._table._field_offsets[idx]
        if self._table._field_sizes[idx] == 1:
            return self._table._raw[off]
        val, = struct.unpack_from('<I', self._table._raw, off)
        return val

    def get_int(self, col: Union[int, str]) -> int:
        idx = self._resolve_col(col)
        off = self._row_offset + self._table._field_offsets[idx]
        if self._table._field_sizes[idx] == 1:
            val, = struct.unpack_from('<b', self._table._raw, off)
            return val
        val, = struct.unpack_from('<i', self._table._raw, off)
        return val

    def get_float(self, col: Union[int, str]) -> float:
        idx = self._resolve_col(col)
        off = self._row_offset + self._table._field_offsets[idx]
        val, = struct.unpack_from('<f', self._table._raw, off)
        return val

    def get_string(self, col: Union[int, str]) -> str:
        str_offset = self.get_uint(col)
        return self._table.get_string(str_offset)

    def __getitem__(self, col: Union[int, str]) -> Any:
        idx = self._resolve_col(col)
        return self._ensure_values()[idx]

    def __getattr__(self, name: str) -> Any:
        idx = self._table._name_to_index.get(name)
        if idx is None:
            raise AttributeError(f"{type(self).__name__!r} has no attribute {name!r}")
        return self._ensure_values()[idx]

    def to_dict(self) -> Dict[str, Any]:
        vals = self._ensure_values()
        return {f.name: vals[i] for i, f in enumerate(self._table.schema.fields)}

    def __repr__(self) -> str:
        return f"DbcRecord({self.to_dict()!r})"


class DbcTable:
    """In-memory reader for a single WDBC or WDB2 table."""

    def __init__(self, raw: bytes, header: DbcHeader, schema: TableSchema,
                 path: Optional[Path] = None) -> None:
        self._raw = raw
        self.header = header
        self.schema = schema
        self.path = path
        self.name = schema.name

        if len(schema.fields) != header.field_count:
            raise ValueError(
                f"Schema {schema.name!r} has {len(schema.fields)} fields, "
                f"but {path or schema.filename} header reports {header.field_count} fields"
            )

        struct_chars: List[str] = ['<']
        field_offsets: List[int] = []
        field_sizes: List[int] = []
        string_indices: List[int] = []
        cur_offset = 0
        for idx, f in enumerate(schema.fields):
            if f.kind not in KIND_TO_STRUCT:
                raise ValueError(f"Unsupported field kind {f.kind!r} for field {f.name!r}")
            sc, sz = KIND_TO_STRUCT[f.kind]
            struct_chars.append(sc)
            field_offsets.append(cur_offset)
            field_sizes.append(sz)
            if f.kind == 'string':
                string_indices.append(idx)
            cur_offset += sz

        if cur_offset != header.record_size:
            raise ValueError(
                f"Schema {schema.name!r} record size is {cur_offset} bytes, "
                f"but {path or schema.filename} header reports {header.record_size} bytes"
            )

        records_bytes = header.record_count * header.record_size
        expected_total = header.data_offset + records_bytes + header.string_size
        if len(raw) < expected_total:
            raise ValueError(
                f"Truncated {header.magic} file {path or schema.filename}: "
                f"expected at least {expected_total} bytes, got {len(raw)}"
            )

        self._row_struct = struct.Struct(''.join(struct_chars))
        self._field_offsets = tuple(field_offsets)
        self._field_sizes = tuple(field_sizes)
        self._string_indices = tuple(string_indices)
        self._name_to_index = {f.name: i for i, f in enumerate(schema.fields)}
        self._records_start = header.data_offset
        self._records_end = header.data_offset + records_bytes
        self._string_start = self._records_end
        self._string_end = self._string_start + header.string_size
        self._string_block = raw[self._string_start:self._string_end]
        self._string_cache: Dict[int, str] = {0: ''}

        # Build primary key index O(1)
        self._key_index = schema.key_index
        self._key_offset = self._field_offsets[self._key_index] if header.field_count > 0 else 0
        self._id_to_row: Dict[int, int] = {}
        if header.field_count > 0 and header.record_count > 0:
            key_size = self._field_sizes[self._key_index]
            rec_size = header.record_size
            base = self._records_start + self._key_offset
            if key_size == 4:
                unpack_u32 = struct.Struct('<I').unpack_from
                for row_idx in range(header.record_count):
                    key_val, = unpack_u32(raw, base + row_idx * rec_size)
                    self._id_to_row[key_val] = row_idx
            else:
                for row_idx in range(header.record_count):
                    self._id_to_row[raw[base + row_idx * rec_size]] = row_idx

    @classmethod
    def from_bytes(cls, raw: bytes, schema: Optional[TableSchema] = None,
                   fmt: Optional[str] = None, name: str = 'Raw',
                   path: Optional[Path] = None) -> 'DbcTable':
        header = parse_header(raw)
        if schema is None:
            if fmt is not None:
                schema = schema_from_trinity_fmt(fmt, name=name)
            else:
                schema = get_schema(path.name if path else name) or default_schema_for_header(header, name=name)
        return cls(raw=raw, header=header, schema=schema, path=path)

    @classmethod
    def from_file(cls, path: Union[str, Path], schema: Optional[TableSchema] = None,
                  fmt: Optional[str] = None) -> 'DbcTable':
        file_path = Path(path)
        raw = file_path.read_bytes()
        return cls.from_bytes(raw, schema=schema, fmt=fmt, name=file_path.stem, path=file_path)

    def __len__(self) -> int:
        return self.header.record_count

    def __contains__(self, record_id: int) -> bool:
        return record_id in self._id_to_row

    def keys(self) -> Sequence[int]:
        return list(self._id_to_row.keys())

    def get_string(self, offset: int) -> str:
        cached = self._string_cache.get(offset)
        if cached is not None:
            return cached
        if offset <= 0 or offset >= self.header.string_size:
            return ''
        end = self._string_block.find(b'\x00', offset)
        if end == -1:
            end = self.header.string_size
        val = self._string_block[offset:end].decode('utf-8', errors='replace')
        self._string_cache[offset] = val
        return val

    def _unpack_row(self, row_offset: int) -> Tuple[Any, ...]:
        raw_vals = self._row_struct.unpack_from(self._raw, row_offset)
        if not self._string_indices:
            return raw_vals
        mutable = list(raw_vals)
        for s_idx in self._string_indices:
            mutable[s_idx] = self.get_string(mutable[s_idx])
        return tuple(mutable)

    def record_at(self, row_index: int) -> DbcRecord:
        if not 0 <= row_index < self.header.record_count:
            raise IndexError(f'Row index {row_index} out of range (0..{self.header.record_count - 1})')
        row_offset = self._records_start + row_index * self.header.record_size
        return DbcRecord(self, row_index, row_offset)

    def get(self, record_id: int) -> Optional[DbcRecord]:
        row_idx = self._id_to_row.get(record_id)
        if row_idx is None:
            return None
        return self.record_at(row_idx)

    def __getitem__(self, record_id: int) -> DbcRecord:
        rec = self.get(record_id)
        if rec is None:
            raise KeyError(f'ID {record_id} not found in {self.name}')
        return rec

    def records(self) -> Iterator[DbcRecord]:
        rec_size = self.header.record_size
        start = self._records_start
        for row_idx in range(self.header.record_count):
            yield DbcRecord(self, row_idx, start + row_idx * rec_size)

    def __iter__(self) -> Iterator[DbcRecord]:
        return self.records()

    def iter_raw_tuples(self) -> Iterator[Tuple[Any, ...]]:
        """Fast C-level struct.iter_unpack over raw rows (string columns remain uint32 string offsets)."""
        if self.header.record_count == 0:
            return iter(())
        view = memoryview(self._raw)[self._records_start:self._records_end]
        return self._row_struct.iter_unpack(view)

    def iter_tuples(self) -> Iterator[Tuple[Any, ...]]:
        """Fast iterator yielding tuples with string columns resolved to str."""
        if not self._string_indices:
            yield from self.iter_raw_tuples()
            return
        str_indices = self._string_indices
        get_s = self.get_string
        for row in self.iter_raw_tuples():
            mutable = list(row)
            for s_idx in str_indices:
                mutable[s_idx] = get_s(mutable[s_idx])
            yield tuple(mutable)


class DbcDirectory:
    """Locale-aware loader and cache for a directory of .dbc and .db2 files."""

    def __init__(self, dbc_dir: Optional[Union[str, Path]] = None, locale: str = 'enUS') -> None:
        if dbc_dir:
            resolved = Path(dbc_dir)
        elif os.environ.get('MOP_DBC_DIR'):
            resolved = Path(os.environ['MOP_DBC_DIR'])
        else:
            resolved = Path(__file__).resolve().parents[2] / 'gamedata' / 'dbc'
        self.dbc_dir = resolved
        self.locale = locale
        self._cache: Dict[str, DbcTable] = {}

    def resolve_path(self, name_or_filename: str) -> Path:
        base = name_or_filename.strip()
        stem = base.rsplit('.', 1)[0] if base.lower().endswith(('.dbc', '.db2')) else base
        ext = base[len(stem):] if base.lower().endswith(('.dbc', '.db2')) else ''

        schema = get_schema(base)
        candidates: List[str] = []
        if ext:
            candidates.append(stem + ext)
            alt_ext = '.db2' if ext.lower() == '.dbc' else '.dbc'
            candidates.append(stem + alt_ext)
        elif schema is not None:
            candidates.append(schema.filename)
            alt_ext = '.db2' if schema.filename.lower().endswith('.dbc') else '.dbc'
            candidates.append(stem + alt_ext)
        else:
            candidates.extend([stem + '.dbc', stem + '.db2'])

        search_dirs = [self.dbc_dir / self.locale, self.dbc_dir]
        # Prefer non-empty files (>= 20 bytes) across locale dir and root dbc dir
        for cand in candidates:
            for d in search_dirs:
                p = d / cand
                if p.is_file() and p.stat().st_size >= 20:
                    return p
                # Case-insensitive fallback inside directory if needed
                if d.is_dir():
                    cand_lower = cand.lower()
                    for child in d.iterdir():
                        if child.name.lower() == cand_lower and child.is_file() and child.stat().st_size >= 20:
                            return child

        raise FileNotFoundError(
            f"Could not find non-empty DBC/DB2 file for {name_or_filename!r} in "
            f"{self.dbc_dir} or {self.dbc_dir / self.locale}"
        )

    def load(self, name_or_filename: str, schema: Optional[TableSchema] = None,
             fmt: Optional[str] = None, use_cache: bool = True) -> DbcTable:
        path = self.resolve_path(name_or_filename)
        cache_key = str(path.resolve()).lower()
        if use_cache and schema is None and fmt is None and cache_key in self._cache:
            return self._cache[cache_key]
        resolved_schema = schema or ( None if fmt is not None else (get_schema(name_or_filename) or get_schema(path.name)) )
        table = DbcTable.from_file(path, schema=resolved_schema, fmt=fmt)
        if use_cache and schema is None and fmt is None:
            self._cache[cache_key] = table
        return table
