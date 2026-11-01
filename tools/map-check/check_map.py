#!/usr/bin/env python3
"""
tools/map-check/check_map.py

Verifies docs/MAP.md:
1. Every file or directory path in docs/MAP.md exists on disk (checking column 1 paths
   and all file/folder path references across all rows).
2. Every major subsystem directory in the repository is represented in docs/MAP.md.
"""

import os
import re
import sys

def resolve_candidate(repo_root: str, token: str, section: str, first_col: str) -> bool:
    cleaned = token.strip()
    if not cleaned:
        return True

    # Normalize slashes
    norm = cleaned.replace("/", os.sep)

    # 1. Direct path from repo root
    cand = os.path.join(repo_root, norm)
    if os.path.exists(cand):
        return True

    # 2. Relative to first column path in table
    first_col_clean = first_col.strip().rstrip("/")
    if first_col_clean:
        cand = os.path.join(repo_root, first_col_clean.replace("/", os.sep), norm)
        if os.path.exists(cand):
            return True

    # 3. Relative to section base directory
    if "Core (`src/server/game/`" in section:
        for base in ["src/server/game", "src/common"]:
            cand = os.path.join(repo_root, base.replace("/", os.sep), norm)
            if os.path.exists(cand):
                return True
    elif "Scripts (`src/server/scripts/`" in section:
        for base in ["src/server/scripts", "src/server/scripts/Custom"]:
            cand = os.path.join(repo_root, base.replace("/", os.sep), norm)
            if os.path.exists(cand):
                return True
    elif "Playerbots (`modules/mod_playerbots/`" in section:
        for base in ["modules/mod_playerbots", "modules/mod_playerbots/src", "modules/mod_playerbots/src/strategy"]:
            cand = os.path.join(repo_root, base.replace("/", os.sep), norm)
            if os.path.exists(cand):
                return True
    elif "Tools and tests (`tools/` and `tests/`" in section:
        for base in ["tools", "tests"]:
            cand = os.path.join(repo_root, base.replace("/", os.sep), norm)
            if os.path.exists(cand):
                return True
    elif "Top level" in section:
        for base in [".github/workflows", "sql", "addons", "lua_scripts", "src/server/scripts"]:
            cand = os.path.join(repo_root, base.replace("/", os.sep), norm)
            if os.path.exists(cand):
                return True

    return False


def is_path_reference(token: str) -> bool:
    """Determine if a backticked token looks like a file or directory reference."""
    token = token.strip()
    if not token or token.startswith(".") or token.startswith("/"):
        return False
    # If it ends with / or has path separator, it's definitely a path
    if token.endswith("/") or "/" in token or "\\" in token:
        # Ignore braces expansion like {world,characters,master}
        if "{" in token:
            return False
        return True
    # If it has a file extension (.h, .cpp, .py, .json, .md, .dist, .yml, .ps1)
    exts = (".h", ".cpp", ".py", ".json", ".md", ".dist", ".yml", ".ps1", ".txt", ".cmake")
    if any(token.endswith(ext) for ext in exts):
        return True
    return False


def main():
    repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
    map_path = os.path.join(repo_root, "docs", "MAP.md")

    if not os.path.isfile(map_path):
        print(f"Error: {map_path} not found.")
        sys.exit(1)

    with open(map_path, "r", encoding="utf-8") as f:
        lines = f.readlines()

    current_section = ""
    paths_to_verify = []

    for line in lines:
        line_s = line.strip()
        if line_s.startswith("## "):
            current_section = line_s
        elif line_s.startswith("| `"):
            parts = line_s.split("|")
            if len(parts) >= 3:
                first_col = parts[1].strip()
                # Extract all backticked items in the entire row
                items = re.findall(r"`([^`]+)`", line_s)
                for item in items:
                    # Every item in first column is always checked
                    if f"`{item}`" in first_col or is_path_reference(item):
                        paths_to_verify.append((current_section, first_col, item))

    print(f"Verifying {len(paths_to_verify)} path items from docs/MAP.md...")

    missing_from_disk = []
    for section, first_col, item in paths_to_verify:
        if not resolve_candidate(repo_root, item, section, first_col):
            missing_from_disk.append((item, section))

    # Check repository coverage in docs/MAP.md
    with open(map_path, "r", encoding="utf-8") as f:
        map_content = f.read()

    missing_from_map = []

    # 1. Custom scripts
    custom_dir = os.path.join(repo_root, "src", "server", "scripts", "Custom")
    if os.path.isdir(custom_dir):
        for entry in sorted(os.listdir(custom_dir)):
            if os.path.isdir(os.path.join(custom_dir, entry)) and entry != "__pycache__":
                expected = f"Custom/{entry}"
                if expected not in map_content:
                    missing_from_map.append(f"src/server/scripts/Custom/{entry}")

    # 2. Tools
    tools_dir = os.path.join(repo_root, "tools")
    if os.path.isdir(tools_dir):
        for entry in sorted(os.listdir(tools_dir)):
            if os.path.isdir(os.path.join(tools_dir, entry)) and entry != "__pycache__":
                expected = f"tools/{entry}"
                if expected not in map_content:
                    missing_from_map.append(f"tools/{entry}")

    # 3. Tests
    tests_dir = os.path.join(repo_root, "tests")
    if os.path.isdir(tests_dir):
        for entry in sorted(os.listdir(tests_dir)):
            if os.path.isdir(os.path.join(tests_dir, entry)) and entry != "__pycache__":
                expected = f"tests/{entry}"
                if expected not in map_content:
                    missing_from_map.append(f"tests/{entry}")

    # 4. Addons
    addons_dir = os.path.join(repo_root, "addons")
    if os.path.isdir(addons_dir):
        for entry in sorted(os.listdir(addons_dir)):
            if os.path.isdir(os.path.join(addons_dir, entry)) and entry != "__pycache__":
                expected = f"{entry}/"
                if expected not in map_content:
                    missing_from_map.append(f"addons/{entry}")

    # 5. Docs
    docs_dir = os.path.join(repo_root, "docs")
    if os.path.isdir(docs_dir):
        for entry in sorted(os.listdir(docs_dir)):
            if entry.endswith(".md"):
                expected = f"docs/{entry}"
                if expected not in map_content:
                    missing_from_map.append(f"docs/{entry}")

    print("Results:")
    print(f"  MAP.md paths missing from disk: {len(missing_from_disk)}")
    for token, sec in missing_from_disk:
        print(f"    - '{token}' ({sec})")

    print(f"  Major paths missing from MAP.md: {len(missing_from_map)}")
    for p in missing_from_map:
        print(f"    - {p}")

    total_issues = len(missing_from_disk) + len(missing_from_map)
    if total_issues == 0:
        print("OK: docs/MAP.md coverage check passed with 0 missing paths.")
        sys.exit(0)
    else:
        print(f"FAIL: {total_issues} path coverage issues found.")
        sys.exit(1)

if __name__ == "__main__":
    main()
