#!/usr/bin/env python3
"""Project: FreeBASIC semantic sidecar audit
File: audit-compiler-semantic-gaps.py
Purpose: Preserve small, reproducible examples of semantic export coverage.
Responsibilities: Run isolated probes and retain sources, diagnostics, and facts.
This file intentionally does NOT treat an observed omission as correct behavior.
"""

from __future__ import annotations

import argparse
from collections import Counter
from dataclasses import dataclass
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import sys


@dataclass(frozen=True)
class Case:
    name: str
    body: str
    options: tuple[str, ...] = ()
    expected_status: int = 0
    encoding: str = "utf-8"
    expected_full_valid: bool = True


CASES = (
    Case("operators", """type BaseType extends Object
end type
type DerivedType extends BaseType
end type
dim object_value as BaseType
if object_value is DerivedType then print 1
dim integer_value as integer = 1
dim values(0 to 1) as integer
dim pointer_value as integer ptr = @integer_value
dim text_value as string = "a"
print *pointer_value, cast(double, integer_value), cptr(integer ptr, 0)
print len(text_value), sizeof(integer_value), values(0)
dim allocated_value as integer ptr = new integer(2)
delete allocated_value
"""),
    Case("prototype_names", """declare function API(byval explicit_name as integer, byref borrowed_name as double) as long
"""),
    Case("special_definitions", """type Tracked
 value as integer
 declare constructor(byval argument as integer)
 declare destructor()
end type
constructor Tracked(byval argument as integer)
 this.value = argument
end constructor
destructor Tracked()
end destructor
operator +(byref lhs as Tracked, byref rhs as Tracked) as integer
 return lhs.value + rhs.value
end operator
dim value as Tracked = 1
print value + value
"""),
    Case("copyback", """declare sub Mutate(byref text as string)
dim text_value as string * 8
Mutate(text_value)
"""),
    Case("static_cleanup", """type Tracked
 value as integer
 declare constructor()
 declare destructor()
end type
constructor Tracked()
end constructor
destructor Tracked()
end destructor
sub Owner()
 static item as Tracked
end sub
Owner()
"""),
    Case("global_cleanup", """type Tracked
 value as integer
 declare constructor()
 declare destructor()
end type
constructor Tracked()
end constructor
destructor Tracked()
end destructor
dim shared item as Tracked
"""),
    Case("data", """data_anchor:
data 1, 2.5, !"A\\0B"
dim item as integer
read item
restore data_anchor
"""),
    Case("assembly", """sub AssemblyProbe()
 dim item as integer
 asm
  mov eax, item
  mov item, eax
 end asm
end sub
AssemblyProbe()
"""),
    Case("macros", """#define ADD(lhs, rhs) lhs + rhs
#define WRAP(value) ADD(value, 2)
dim item as integer = WRAP(1)
#undef ADD
"""),
    Case("options", """#lang "fblite"
option base 1
dim values(2) as integer
option base 0
dim other_values(2) as integer
print lbound(values), lbound(other_values)
"""),
    Case("bydesc", "declare sub ArrayAPI(values(any, any) as integer)\n"),
    Case("file_effects", """dim number as integer
open "audit.txt" for output as #number
print #number, 1
close #number
"""),
    Case("diagnostic_recovery", """dim value as integer
print value + unresolved_name
print value
""", expected_status=1),
    Case("node_plain", "dim value as integer = 1\nprint value\n"),
    Case("node_debug", "dim value as integer = 1\nprint value\n", ("-g",)),
    Case("implicit_declaration", "#lang \"fblite\"\nprint implicit_value\n"),
    Case("forward_alias", """type LinkType as FutureType ptr
type FutureType
 value as integer
end type
type CountType as integer
type Counter as CountType
dim item as Counter
"""),
    Case("base_constructor", """type ParentType extends Object
 value as integer
 declare constructor()
 declare constructor(byval number as integer)
end type
constructor ParentType()
end constructor
constructor ParentType(byval number as integer)
 this.value = number
end constructor
type ChildType extends ParentType
 declare constructor()
end type
constructor ChildType()
 base(5)
end constructor
dim item as ChildType
"""),
    Case("constructor_chain", """type Tracked
 value as integer
 declare constructor(byval number as integer)
 declare constructor()
end type
constructor Tracked(byval number as integer)
 this.value = number
end constructor
constructor Tracked()
 constructor(1)
end constructor
dim item as Tracked
"""),
    Case("module_priority", """sub InitializeEarly() constructor 201
end sub
sub InitializeLate() constructor 202
end sub
sub FinalizeModule() destructor 201
end sub
"""),
    Case("runtime_checks", """dim values(0 to 2) as integer
dim index_value as integer
print values(index_value)
dim pointer_value as integer ptr
print *pointer_value
""", ("-exx",)),
    Case("unicode_index", """dim text_value as ustring = "a"
print text_value[0]
text_value[0] = 65
"""),
    Case("varargs", """#include once "crt/stdarg.bi"
sub VarargProbe cdecl(byval marker as long, ...)
 dim arguments as cva_list
 cva_start(arguments, marker)
 print cva_arg(arguments, long)
 cva_end(arguments)
end sub
VarargProbe(1, 2)
"""),
    Case("conditional", """#define ENABLED 1
#if ENABLED
dim selected_value as integer
#else
dim skipped_value as integer
#endif
print selected_value
"""),
    Case("legacy_macro", '#define TEXT "\u00e9"\ndim value as string = TEXT\n',
         encoding="latin-1", expected_full_valid=False),
    Case("legacy_literal", 'dim value as string = "\u00e9"\n',
         encoding="latin-1", expected_full_valid=False),
)


SOURCE_HEADER = """' Project: FreeBASIC semantic sidecar audit
' File: probe.bas
' Purpose: Supply a disposable source for one coverage observation.
' Responsibilities: Exercise compiler-owned decisions without running the program.
' This file intentionally does NOT alter production sources.

"""


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write_source(path: Path, body: str, encoding: str = "utf-8") -> None:
    header = SOURCE_HEADER.replace("File: probe.bas", "File: " + path.name)
    path.write_text(header + body + "\n' end of " + path.name + "\n", encoding=encoding)


def invoke(command: list[str], folder: Path, log: Path) -> dict:
    result = subprocess.run(command, cwd=folder, stdin=subprocess.DEVNULL,
                            text=True, capture_output=True, timeout=60)
    log.write_text("$ " + shlex.join(command) + "\n" + result.stdout + result.stderr,
                   encoding="utf-8")
    return {"status": result.returncode, "command": command, "log": str(log)}


def model_facts(model) -> dict:
    """Summarize exported facts without inferring a missing source operation."""
    procedures = []
    for identity, row in model.signatures.items():
        metadata = model.types[identity]
        if metadata[18] == "runtime":
            continue
        procedures.append({"identity": identity, "kind": row[2],
                           "name": metadata[2], "origin": metadata[18],
                           "declarations": sum(binding[1] == str(identity) and binding[2] == "declaration"
                                               for binding in model.records["B"])})
    calls = Counter()
    for row in model.records["N"]:
        if model.properties["node", int(row[1])].get("kind") == "call":
            calls[model.symbol_name(row[9])] += 1
    return {
        "records": {tag: len(rows) for tag, rows in model.records.items()},
        "operators": dict(Counter(row[1] for row in model.records["O"])),
        "implicit_calls": dict(Counter(row[4] for row in model.records["I"])),
        "node_lines": sorted({int(row[11]) for row in model.records["N"]}),
        "node_kinds": sorted({properties["kind"] for (domain, _), properties in model.properties.items()
                              if domain == "node"}),
        "calls": dict(calls), "procedures": procedures,
        "parameters": [{"identity": int(row[2]), "name": model.symbol_name(row[2]),
                        "procedure": model.symbol_name(row[1]), "mode": row[4], "rank": int(row[6])}
                       for row in model.records["G"] if model.types[int(row[1])][18] != "runtime"],
        "dependencies": [row[1] for row in model.records["D"]],
        "footer": model.footer,
    }


def run_case(case: Case, common: list[str], output: Path, reader) -> list[dict]:
    folder = output / case.name
    folder.mkdir()
    source = folder / "probe.bas"
    write_source(source, case.body, case.encoding)
    results = []
    for mode, option in (("full", "-semantic-model"), ("expressions", "-semantic-model-expressions")):
        path = folder / (mode + ".tsv")
        entry = {"case": case.name, "mode": mode, "source_sha256": digest(source),
                 **invoke(common + list(case.options) + [option, str(path), str(source)],
                          folder, folder / (mode + ".log"))}
        entry["expected_status"] = case.expected_status
        entry["expected_valid_model"] = mode == "expressions" or (case.expected_status == 0 and case.expected_full_valid)
        emission = source.with_suffix(".c")
        if emission.exists():
            entry["emission_sha256"] = digest(emission)
        try:
            model = reader.read(path, expressions_only=mode == "expressions", allow_recovery=mode == "expressions")
            entry["valid_model"] = True
            entry.update(model_facts(model))
        except (ValueError, OSError) as error:
            entry["valid_model"] = False
            entry["reader_error"] = str(error)
        results.append(entry)
        print(case.name, mode, "status", entry["status"], "accepted", entry["valid_model"], flush=True)
    return results


def reader_mutations(output: Path, reader) -> list[dict]:
    """Keep record totals correct while changing a class-specific payload."""
    text = (output / "copyback/full.tsv").read_text()
    model = reader(text)
    results = []
    for name, key, replacement in (("unknown_node_kind", "kind", "pretend-kind"),
                                    ("invalid_argument_bytes", "bytes", "nonnumeric")):
        rows = [line.split("\t") for line in text.splitlines()]
        row = next(row for row in rows if row[0] == "K" and row[1] == "node" and row[3] == key
                   and (key != "bytes" or model.properties["node", int(row[2])]["kind"] == "argument"))
        entry = {"case": name, "mode": "reader-mutation", "original_property": row.copy(),
                 "node_kind": model.properties["node", int(row[2])]["kind"]}
        row[4] = replacement
        try:
            reader("\n".join("\t".join(row) for row in rows) + "\n")
            entry["accepted"] = True
        except ValueError as error:
            entry["accepted"] = False
            entry["reader_error"] = str(error)
        results.append(entry)
    return results


def run_transport(common: list[str], output: Path, reader) -> list[dict]:
    """Only these freshly created files are allowed to alias an output path."""
    folder = output / "transport"
    folder.mkdir()
    results = []
    cases = ["same_path", "hardlink", "symlink"]
    if Path("/dev/full").exists():
        cases.append("dev_full")
    for name in cases:
        source = folder / (name + ".bas")
        write_source(source, "dim important_value as integer = 17\nprint important_value\n")
        destination = folder / (name + ".tsv")
        if name == "same_path":
            destination = source
        elif name == "hardlink":
            os.link(source, destination)
        elif name == "symlink":
            destination.symlink_to(source.name)
        elif name == "dev_full":
            destination = Path("/dev/full")
        original = source.read_bytes()
        entry = {"case": name, "mode": "transport",
                 **invoke(common + ["-semantic-model-expressions", str(destination), str(source)],
                          folder, folder / (name + ".log"))}
        entry["source_changed"] = source.read_bytes() != original
        entry["original_source_hex"] = original.hex()
        if name != "dev_full":
            try:
                model = reader.read(destination, expressions_only=True, allow_recovery=True)
                entry["valid_model"] = True
                entry.update(model_facts(model))
            except (ValueError, OSError) as error:
                entry["valid_model"] = False
                entry["reader_error"] = str(error)
        results.append(entry)
        print(name, "status", entry["status"], "source changed", entry["source_changed"], flush=True)
    return results


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", type=Path, required=True)
    parser.add_argument("--prefix", type=Path, required=True)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--output", type=Path, required=True)
    options = parser.parse_args()
    compiler = options.compiler.resolve()
    prefix = options.prefix.resolve()
    root = options.root.resolve()
    output = options.output.resolve()
    # Refusing an existing destination keeps the deliberately destructive alias
    # probes confined to files this invocation has just created.
    output.mkdir(parents=True)
    sys.path.insert(0, str(root / "tests/semantic-sidecar"))
    from sidecar import Model, SCHEMA
    if SCHEMA != "19":
        parser.error("These dated probes require the schema 19 reader")
    common = [str(compiler), "-prefix", str(prefix), "-i", str(prefix / "inc"),
              "-gen", "gcc", "-r", "-maxerr", "20"]
    manifest = {"compiler": str(compiler), "compiler_sha256": digest(compiler),
                "reader_sha256": digest(root / "tests/semantic-sidecar/sidecar.py"),
                "probe_script_sha256": digest(Path(__file__)), "schema": SCHEMA, "prefix": str(prefix)}
    results = []
    for case in CASES:
        results.extend(run_case(case, common, output, Model))
    results.extend(run_transport(common, output, Model))
    results.extend(reader_mutations(output, Model))
    failures = [entry for entry in results if "expected_status" in entry
                and (entry["status"] != entry["expected_status"] or
                     entry["valid_model"] != entry["expected_valid_model"])]
    (output / "results.json").write_text(json.dumps({"manifest": manifest, "results": results,
                                                     "unexpected_results": failures}, indent=2) + "\n")
    print("Saved", len(results), "observations to", output)
    print("An accepted sidecar validates its structure, not the completeness of its semantics.")
    return bool(failures)


if __name__ == "__main__":
    raise SystemExit(main())

# end of audit-compiler-semantic-gaps.py
