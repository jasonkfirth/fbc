#!/usr/bin/env python3
"""
Project: FreeBASIC test tooling
File: check-core-language-test-coverage.py
Purpose: Find language tokens/directives with no spelling in fbctests sources.
Responsibilities: Read generated unit/log inventories and compare them with
    the compiler keyword table and preprocessor directive set.
This file intentionally does NOT contain semantic test assertions.
"""

from __future__ import annotations

import argparse
import csv
from pathlib import Path
import re
import sys


INVENTORY_FILES = (
    "unit-tests.inc",
    "log-tests-fb.inc",
    "log-tests-fblite.inc",
    "log-tests-qb.inc",
    "log-tests-deprecated.inc",
)

# These names are internal parser tokens emitted by compiler-assisted
# serialization. User programs call the public Serialize/Deserialize API.
INTERNAL_TOKENS = {"FB_TK_SERIALIZE", "FB_TK_DESERIALIZE"}
PREFERRED_KEYWORD_WITNESSES = {
    "FB_TK_ANY": "functions/any-byref.bas",
    "FB_TK_CLASS": "structs/class-reserved.bas",
    "FB_TK_CVA_COPY": "functions/va_cva_api.bas",
    "FB_TK_GOSUB": "quirk/gosub.bas",
    "FB_TK_IMPORT": "bindings/import-data.bas",
    "FB_TK_IMPLEMENTS": "structs/implements-reserved.bas",
    "FB_TK_LIB": "pp/libpath-main.bas",
    "FB_TK_RESUME": "fblite/resume.bas",
    "FB_TK_SERIALIZE": "serialization/roundtrip.bas",
    "FB_TK_DESERIALIZE": "serialization/roundtrip.bas",
    "FB_TK_THREAD": "threads/thread-local.bas",
    "FB_TK_THREADCALL": "threads/threadcall.bas",
    "FB_TK_UNLOCK": "file/unlock-compile.bas",
}
PREPROCESSOR_DIRECTIVES = (
    ("#IF", r"(?im)^\s*#\s*if(?![a-z])"),
    ("#IFDEF", r"(?im)^\s*#\s*ifdef\b"),
    ("#IFNDEF", r"(?im)^\s*#\s*ifndef\b"),
    ("#ELSEIF", r"(?im)^\s*#\s*elseif(?![a-z])"),
    ("#ELSEIFDEF", r"(?im)^\s*#\s*elseifdef\b"),
    ("#ELSEIFNDEF", r"(?im)^\s*#\s*elseifndef\b"),
    ("#ELSE", r"(?im)^\s*#\s*else(?![a-z])"),
    ("#ENDIF", r"(?im)^\s*#\s*endif\b"),
    ("#DEFINE", r"(?im)^\s*#\s*define\b"),
    ("#MACRO", r"(?im)^\s*#\s*macro\b"),
    ("#ENDMACRO", r"(?im)^\s*#\s*endmacro\b"),
    ("#UNDEF", r"(?im)^\s*#\s*undef\b"),
    ("#INCLUDE", r"(?im)^\s*#\s*include\b"),
    ("#INCLIB", r"(?im)^\s*#\s*inclib\b"),
    ("#LIBPATH", r"(?im)^\s*#\s*libpath\b"),
    ("#PRAGMA", r"(?im)^\s*#\s*pragma\b"),
    ("#CMDLINE", r"(?im)^\s*#\s*cmdline\b"),
    ("#LANG", r"(?im)^\s*#\s*lang\b"),
    ("#PRINT", r"(?im)^\s*#\s*print\b"),
    ("#ERROR", r"(?im)^\s*#\s*error\b"),
    ("#ASSERT", r"(?im)^\s*#\s*assert\b"),
    ("#LINE", r"(?im)^\s*#\s*line\b"),
    ("'$INCLUDE", r"(?im)^\s*'\s*\$\s*include\b"),
    ("'$DYNAMIC", r"(?im)^\s*'\s*\$\s*dynamic\b"),
    ("'$STATIC", r"(?im)^\s*'\s*\$\s*static\b"),
    ("'$LANG", r"(?im)^\s*'\s*\$\s*lang\b"),
)
PREFERRED_DIRECTIVE_WITNESSES = {
    "#LIBPATH": "pp/libpath-main.bas",
    "#LINE": "pp/line-directive.bas",
    "#LANG": "quirk/hash-lang.bas",
    "'$INCLUDE": "fblite/meta-include.bas",
    "'$DYNAMIC": "fblite/options-smoke.bas",
    "'$STATIC": "fblite/options-smoke.bas",
    "'$LANG": "quirk/meta-lang.bas",
}
CORE_MANUAL_FEATURES = (
    ("ASSERT", r"(?<![a-z0-9_])assert\s*\(", "quirk/runtime-assertions.bas"),
    ("ASSERTWARN", r"(?<![a-z0-9_])assertwarn\s*\(", "quirk/runtime-assertions.bas"),
    ("STOP", r"(?<![a-z0-9_])stop(?![a-z0-9_])", "quirk/legacy-system-commands.bas"),
    ("INP", r"(?<![a-z0-9_])inp\s*\(", "quirk/legacy-system-commands.bas"),
    ("OUT", r"(?<![a-z0-9_])out\s+", "quirk/legacy-system-commands.bas"),
    ("WAIT", r"(?<![a-z0-9_])wait\s+", "quirk/legacy-system-commands.bas"),
    ("BEEP", r"(?<![a-z0-9_])beep(?![a-z0-9_])", "quirk/legacy-system-commands.bas"),
    ("SLEEP", r"(?<![a-z0-9_])sleep\s+", "threads/thread-local.bas"),
    ("LPRINT", r"(?<![a-z0-9_])lprint\b", "fblite/legacy-printer.bas"),
    ("LPOS", r"(?<![a-z0-9_])lpos\s*\(", "fblite/legacy-printer.bas"),
    ("OPTION()", r"(?<![a-z0-9_])option\s*\(", "optimizations/vector.bas"),
)

KEYWORD_ROW = re.compile(
    r'\(\s*@"([^"\n]+)"\s*,\s*(FB_TK_[A-Z0-9_]+)\s*,\s*'
    r'(FB_TKCLASS_[A-Z]+)'
)
INVENTORY_ENTRY = re.compile(r"\+=\s+(\.\/[^\s]+)")
MODE_PRIORITY = {
    "FBCUNIT": 0,
    "COMPILE_AND_RUN_OK": 1,
    "MULTI_MODULE_OK": 2,
    "COMPILE_ONLY_OK": 3,
    "COMPILE_AND_RUN_FAIL": 4,
    "MULTI_MODULE_FAIL": 5,
    "COMPILE_ONLY_FAIL": 6,
}


def read_test_sources(root: Path) -> tuple[set[Path], dict[Path, set[str]], list[str]]:
    tests_dir = root / "tests"
    missing = [name for name in INVENTORY_FILES if not (tests_dir / name).is_file()]
    if missing:
        return set(), {}, missing

    sources: set[Path] = set()
    modes: dict[Path, set[str]] = {}
    for inventory in INVENTORY_FILES:
        for line in (tests_dir / inventory).read_text(encoding="utf-8").splitlines():
            match = INVENTORY_ENTRY.search(line)
            if not match:
                continue
            path = tests_dir / match.group(1)[2:]
            if inventory == "unit-tests.inc":
                mode = "FBCUNIT"
            else:
                mode_match = re.search(r"SRCLIST_([A-Z_]+)\s*\+=", line)
                mode = mode_match.group(1) if mode_match else "UNKNOWN"
            if path.suffix == ".bas" and path.is_file():
                resolved = path.resolve()
                sources.add(resolved)
                modes.setdefault(resolved, set()).add(mode)
            elif path.suffix == ".bmk" and path.is_file():
                for source in read_bmk_sources(path):
                    resolved = source.resolve()
                    sources.add(resolved)
                    modes.setdefault(resolved, set()).add(mode)
    return sources, modes, []


def read_bmk_sources(path: Path) -> set[Path]:
    """Add the BASIC files named by a simple multi-module test manifest."""
    sources: set[Path] = set()
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        match = re.match(r"\s*(?:MAIN|SRCS)\s*:?=\s*(.*?)\s*$", line)
        if not match:
            continue
        for name in match.group(1).split():
            candidate = path.parent / name
            if candidate.suffix == ".bas" and candidate.is_file():
                sources.add(candidate)
    return sources


def source_code(path: Path) -> str:
    code = path.read_text(encoding="utf-8", errors="replace").lower()
    code = re.sub(r'"(?:""|[^"])*"', " ", code)
    code = re.sub(r"/\'.*?\'/", " ", code, flags=re.DOTALL)
    return re.sub(r"'[^\n]*", " ", code)


def source_label(path: Path, tests_dir: Path) -> str:
    path = path.resolve()
    tests_dir = tests_dir.resolve()
    try:
        return path.relative_to(tests_dir).as_posix()
    except ValueError:
        return path.as_posix()


def find_witness(
    pattern: str,
    source_text: list[tuple[Path, str]],
    source_modes: dict[Path, set[str]],
    tests_dir: Path,
    preferred: str = "",
    word: str = "",
    source_tokens: dict[Path, set[str]] | None = None,
) -> str:
    def matches_feature(path: Path, code: str) -> bool:
        if word and source_tokens is not None:
            return word in source_tokens[path]
        return re.search(pattern, code) is not None

    matches = [(path, code) for path, code in source_text if matches_feature(path, code)]
    if preferred:
        for path, _ in matches:
            if source_label(path, tests_dir) == preferred:
                return preferred
    if not matches:
        return ""

    def rank(path: Path) -> tuple[int, str]:
        modes = source_modes.get(path, {"UNKNOWN"})
        mode_rank = min((MODE_PRIORITY.get(mode, 99) for mode in modes), default=99)
        return mode_rank, source_label(path, tests_dir)

    return source_label(min((path for path, _ in matches), key=rank), tests_dir)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--strict", action="store_true", help="return failure for uncovered tokens")
    parser.add_argument("--output", type=Path, help="write language-feature-to-test witness TSV")
    args = parser.parse_args()
    root = args.root.resolve()

    sources, source_modes, missing_inventories = read_test_sources(root)
    if missing_inventories:
        names = ", ".join(missing_inventories)
        print(f"Missing generated test inventory: {names}", file=sys.stderr)
        print("Run make -C tests unit-tests and make -C tests log-tests first.", file=sys.stderr)
        return 2

    tests_dir = root / "tests"
    keyword_file = root / "src/compiler/symbols/symb-keyword.bas"
    if not keyword_file.is_file():
        print(f"Missing keyword table: {keyword_file}", file=sys.stderr)
        return 2

    source_raw = [
        (path, path.read_text(encoding="utf-8", errors="replace").lower())
        for path in sorted(sources)
    ]
    source_text = [(path, source_code(path)) for path, _ in source_raw]
    source_tokens = {
        path: set(re.findall(r"[a-z_][a-z0-9_]*", code))
        for path, code in source_text
    }
    uncovered: list[tuple[str, str, str]] = []
    rows_out: list[tuple[str, str, str, str, str, str]] = []
    uncovered_directives: list[str] = []
    uncovered_manual_features: list[str] = []
    rows = KEYWORD_ROW.findall(keyword_file.read_text(encoding="utf-8", errors="replace"))
    for spelling, token, token_class in rows:
        words = spelling.lower().split()
        pattern = r"(?<![a-z0-9_])" + r"\s+".join(map(re.escape, words)) + r"(?![a-z0-9_])"
        preferred = PREFERRED_KEYWORD_WITNESSES.get(token, "")
        witness = find_witness(
            pattern, source_text, source_modes, tests_dir, preferred,
            word=words[0] if len(words) == 1 else "", source_tokens=source_tokens
        )
        if not witness:
            if token in INTERNAL_TOKENS:
                public_name = "serialize" if token == "FB_TK_SERIALIZE" else "deserialize"
                public_pattern = r"(?<![a-z0-9_])" + public_name + r"(?![a-z0-9_])"
                witness = find_witness(
                    public_pattern, source_text, source_modes, tests_dir,
                    "serialization/roundtrip.bas", word=public_name,
                    source_tokens=source_tokens
                )
        if not witness:
            uncovered.append((spelling, token, token_class))
            witness = "MISSING"
        modes = source_modes.get((tests_dir / witness).resolve(), set()) if witness != "MISSING" else set()
        rows_out.append(("keyword", spelling, token, token_class, witness, ",".join(sorted(modes))))

    for spelling, pattern in PREPROCESSOR_DIRECTIVES:
        preferred = PREFERRED_DIRECTIVE_WITNESSES.get(spelling, "")
        witness = find_witness(pattern, source_raw, source_modes, tests_dir, preferred)
        if not witness:
            uncovered_directives.append(spelling)
            witness = "MISSING"
        modes = source_modes.get((tests_dir / witness).resolve(), set()) if witness != "MISSING" else set()
        rows_out.append(("preprocessor", spelling, "", "directive", witness, ",".join(sorted(modes))))

    for spelling, pattern, preferred in CORE_MANUAL_FEATURES:
        witness = find_witness(pattern, source_text, source_modes, tests_dir, preferred)
        if not witness:
            uncovered_manual_features.append(spelling)
            witness = "MISSING"
        modes = source_modes.get((tests_dir / witness).resolve(), set()) if witness != "MISSING" else set()
        rows_out.append(("manual feature", spelling, "", "CatPgMisc", witness, ",".join(sorted(modes))))

    if args.output:
        output = args.output if args.output.is_absolute() else root / args.output
        output.parent.mkdir(parents=True, exist_ok=True)
        with output.open("w", encoding="utf-8", newline="") as stream:
            writer = csv.writer(stream, delimiter="\t", lineterminator="\n")
            writer.writerow(("feature type", "spelling", "token", "class", "test source", "test mode"))
            writer.writerows(rows_out)
        print(f"Language feature witness matrix: {output}")

    print(f"Standard fbctests BASIC sources: {len(sources)}")
    print(f"Compiler keyword table entries checked: {len(rows)}")
    print(f"Preprocessor directive forms checked: {len(PREPROCESSOR_DIRECTIVES)}")
    print(f"Additional CatPgMisc forms checked: {len(CORE_MANUAL_FEATURES)}")
    if uncovered:
        print("No source occurrence:")
        for spelling, token, token_class in uncovered:
            print(f"  {spelling} ({token}, {token_class})")
    if uncovered_directives:
        print("No preprocessor source occurrence:")
        for spelling in uncovered_directives:
            print(f"  {spelling}")
    if uncovered_manual_features:
        print("No CatPgMisc source occurrence:")
        for spelling in uncovered_manual_features:
            print(f"  {spelling}")
    if uncovered or uncovered_directives or uncovered_manual_features:
        return 1 if args.strict else 0

    print("All keyword rows, preprocessor forms and selected CatPgMisc entries have test witnesses.")
    print("This is a discovery check; it does not prove runtime semantics.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

# end of check-core-language-test-coverage.py
