#!/usr/bin/env python3
"""Project: FreeBASIC semantic sidecar audit
File: audit-compiler-semantic-model.py
Purpose: Index compiler contracts and export observation points for a coverage review.
Responsibilities: Enumerate source files, dispatch routes, classes, layouts, and hooks.
This file intentionally does NOT prove semantic completeness from text matches.
"""

from __future__ import annotations

import argparse
from collections import Counter
import csv
from fnmatch import fnmatchcase
import hashlib
import json
from pathlib import Path
import re


FUNCTION = re.compile(r"^(?:(?:private|public)\s+)?(function|sub)\s+(\w+)", re.I)
TYPE = re.compile(r"^(?:(?:private|public)\s+)?(type|union)\s+(\w+)\s*(?:''.*)?$", re.I)
FIELD = re.compile(r"^\s*([\w]+)(?:\([^\n]*?\))?\s+as\s+(.+?)(?:\s+''\s*(.*))?$", re.I)
AS_FIELD = re.compile(r"^\s*as\s+(.+?)\s+(\w+(?:\([^)]*\))?(?:\s*,\s*\w+(?:\([^)]*\))?)*)\s*$", re.I)


def blocks(path: Path, prefix: str = "") -> list[dict]:
    """Index named contract fields, retaining comments and exact source locations."""
    result = []
    containers = []
    for line_number, line in enumerate(path.read_text().splitlines(), 1):
        start = TYPE.match(line.strip())
        if start:
            active = None
            if start[2].startswith(prefix):
                active = {"name": start[2], "kind": start[1].lower(),
                          "line": line_number, "fields": []}
                result.append(active)
            containers.append(active)
            continue
        # Anonymous TYPE/UNION members do not end their containing layout.
        # FreeBASIC uses these for the narrow/wide lexer buffers in particular.
        if re.match(r"\s*(?:type|union)\s*(?:''.*)?$", line, re.I):
            containers.append(None)
            continue
        if re.match(r"\s*end (?:type|union)\b", line, re.I):
            if containers:
                containers.pop()
            continue
        active = next((row for row in reversed(containers) if row is not None), None)
        if active:
            field = FIELD.match(line)
            if field:
                active["fields"].append({"name": field[1], "type": field[2].strip(),
                                         "comment": field[3] or "", "line": line_number})
            else:
                declaration, _, comment = line.partition("''")
                field = AS_FIELD.match(declaration)
                if field:
                    for name in re.findall(r"(?:^|,)\s*(\w+)", field[2]):
                        active["fields"].append({"name": name, "type": field[1].strip(),
                                                 "comment": comment.strip(), "line": line_number})
    return result


def enum(path: Path, name: str, prefix: str) -> list[dict]:
    text = path.read_text().splitlines()
    active = False
    result = []
    for line_number, line in enumerate(text, 1):
        if re.match(r"\s*enum\s+" + re.escape(name) + r"\s*$", line, re.I):
            active = True
        elif active and re.match(r"\s*end enum\b", line, re.I):
            break
        elif active:
            found = re.match(r"\s*(" + re.escape(prefix) + r"\w+)\b", line)
            if found:
                result.append({"name": found[1], "line": line_number})
    return result


def inventory(root: Path) -> dict:
    compiler = root / "src/compiler"
    files = []
    dispatch = []
    functions = []
    observations = []
    layouts = []
    for path in sorted(compiler.rglob("*")):
        relative = path.relative_to(compiler)
        if not path.is_file() or "obj" in relative.parts or path.suffix not in (".bas", ".bi"):
            continue
        data = path.read_bytes()
        source = data.decode("utf-8")
        for layout in blocks(path):
            layout["file"] = str(relative)
            layouts.append(layout)
        current = ""
        hooks = []
        count = 0
        for line_number, line in enumerate(source.splitlines(), 1):
            found = FUNCTION.match(line)
            if found:
                current = found[2]
                functions.append({"file": str(relative), "line": line_number,
                                  "name": current, "kind": found[1].lower()})
            if re.match(r"end (?:function|sub)\b", line, re.I):
                current = ""
            if "fbSemanticModel" in line and not line.lstrip().startswith(("declare ", "''")):
                names = re.findall(r"\bfbSemanticModel\w+", line)
                for name in names:
                    row = {"file": str(relative), "line": line_number,
                           "function": current, "hook": name, "text": line.strip()}
                    observations.append(row)
                    hooks.append(name)
                    count += 1
            if relative.parts[0] in ("parser", "preprocessor") and re.match(r"\s*case\b", line, re.I):
                tokens = re.findall(r"\b(?:FB_TK_|CHAR_)\w+", line)
                if tokens:
                    dispatch.append({"file": str(relative), "line": line_number,
                                     "function": current, "tokens": tokens})
        files.append({"file": str(relative), "group": relative.parts[0],
                      "bytes": len(data), "lines": len(source.splitlines()),
                      "sha256": hashlib.sha256(data).hexdigest(),
                      "hooks": sorted(set(hooks)), "observation_lines": count})
    return {
        "files": files, "functions": functions, "dispatch_cases": dispatch,
        "observations": observations,
        "layouts": layouts,
        "ast_classes": enum(compiler / "ast/ast.bi", "AST_NODECLASS", "AST_NODECLASS_"),
        "operators": enum(compiler / "ast/ast-op.bi", "AST_OP", "AST_OP_"),
        "symbol_classes": enum(compiler / "symbols/symb.bi", "FB_SYMBCLASS", "FB_SYMBCLASS_"),
        "ast_layouts": [row for row in layouts if row["file"] == "ast/ast.bi"],
        "symbol_layouts": [row for row in layouts if row["file"] == "symbols/symb.bi"],
        "compiler_options": blocks(compiler / "core/fb.bi", "FBCMMLINEOPT"),
        "limitations": ["Hook presence is not proof of complete coverage.",
                        "A dispatcher can enter observed common subroutines.",
                        "A class name does not prove export of its payload.",
                        "Review field lifetimes and transformation phases before adding exports."]}


def field_coverage(result: dict, rules_path: Path) -> list[dict]:
    """Apply reviewed dispositions; the last matching rule is the most specific."""
    with rules_path.open() as stream:
        rules = list(csv.DictReader((line for line in stream if not line.startswith("#")), delimiter="\t"))
    allowed = {"exported", "partial", "derivable", "missing", "bookkeeping", "backend-detail"}
    rows = []
    for layout in result["layouts"]:
        for field in layout["fields"]:
            matches = [rule for rule in rules
                       if fnmatchcase(layout["file"], rule["file"])
                       and fnmatchcase(layout["name"], rule["contract"])
                       and fnmatchcase(field["name"], rule["field"])]
            if not matches or matches[-1]["status"] not in allowed:
                raise ValueError("Field requires review: " + layout["file"] + ":" +
                                 layout["name"] + "." + field["name"])
            rule = matches[-1]
            rows.append({"file": layout["file"], "contract": layout["name"],
                         "field": field["name"], "line": field["line"],
                         **{key: rule[key] for key in ("status", "records", "gaps", "reason")}})
    return rows


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--coverage-rules", type=Path)
    parser.add_argument("--coverage-output", type=Path)
    options = parser.parse_args()
    result = inventory(options.root.resolve())
    if options.coverage_output and not options.coverage_rules:
        parser.error("--coverage-output requires --coverage-rules")
    if options.coverage_rules:
        result["field_coverage"] = field_coverage(result, options.coverage_rules)
    options.output.mkdir(parents=True, exist_ok=True)
    (options.output / "compiler-inventory.json").write_text(json.dumps(result, indent=2) + "\n")
    with (options.output / "compiler-files.tsv").open("w") as stream:
        stream.write("file\tgroup\tlines\tobservation_lines\thooks\tsha256\n")
        for row in result["files"]:
            stream.write("\t".join((row["file"], row["group"], str(row["lines"]),
                                     str(row["observation_lines"]), ",".join(row["hooks"]), row["sha256"])) + "\n")
    with (options.output / "compiler-layouts.tsv").open("w") as stream:
        stream.write("file\tcontract\tfield\ttype\tline\tcomment\n")
        for layout in result["layouts"]:
            for field in layout["fields"]:
                stream.write("\t".join((layout["file"], layout["name"], field["name"], field["type"],
                                         str(field["line"]), field["comment"].replace("\t", " "))) + "\n")
    counts = Counter(row["group"] for row in result["files"])
    print("Indexed", len(result["files"]), "compiler files in", len(counts), "subsystems;")
    print(len(result["functions"]), "function/sub bodies;", len(result["dispatch_cases"]), "parser/preprocessor case sites;")
    print(len(result["ast_classes"]), "AST classes;", len(result["operators"]), "operators;",
          len(result["symbol_classes"]), "symbol classes.")
    print(len(result["layouts"]), "named layouts;",
          sum(len(row["fields"]) for row in result["layouts"]), "layout fields.")
    if options.coverage_rules:
        destination = options.coverage_output or options.output / "field-coverage.tsv"
        with destination.open("w") as stream:
            stream.write("# Project: FreeBASIC semantic sidecar audit\n")
            stream.write("# File: semantic-sidecar-field-coverage.tsv\n")
            stream.write("# Purpose: Account for compiler layout fields in the dated coverage review.\n")
            stream.write("# Responsibilities: Apply reviewed field dispositions with source locations and gap references.\n")
            stream.write("# This file intentionally does NOT claim execution coverage from rule matches.\n")
            writer = csv.DictWriter(stream, fieldnames=("file", "contract", "field", "line", "status",
                                                        "records", "gaps", "reason"), delimiter="\t",
                                    lineterminator="\n")
            writer.writeheader()
            writer.writerows(result["field_coverage"])
            stream.write("# end of semantic-sidecar-field-coverage.tsv\n")
        print("Field dispositions:", dict(Counter(row["status"] for row in result["field_coverage"])))
    print("Evidence:", options.output.resolve())
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

# end of audit-compiler-semantic-model.py
