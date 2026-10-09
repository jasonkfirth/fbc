"""Project: FreeBASIC compiler semantic tests
File: test-compiler-credential-contexts.py
Purpose: Verify exact named destinations, caller inputs and formal defaults.
Responsibilities: Targets, encodings, compact modes and damaged receipt rejection.
This file intentionally does NOT identify sensitive names or implement a linter rule.
"""

from __future__ import annotations

import argparse
from pathlib import Path
import subprocess
import sys
import tempfile


def remove_detail(rows: list[list[str]], property_name: str) -> list[list[str]]:
    damaged = [row[:] for row in rows]
    index = next(index for index, row in enumerate(damaged)
                 if row[0] == "K" and row[3] == property_name)
    del damaged[index]
    damaged[-1][12] = str(int(damaged[-1][12]) - 1)
    return damaged


def require_rejection(model_type: type, rows: list[list[str]], message: str) -> None:
    try:
        model_type("\n".join("\t".join(row) for row in rows) + "\n")
    except ValueError:
        return
    raise AssertionError(message)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fbc", required=True, type=Path)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    args = parser.parse_args()
    sys.path.insert(0, str(args.root / "tests" / "semantic-sidecar"))
    from sidecar import Model

    source = (args.root / "tests" / "semantic-sidecar" / "credential-context-inputs.bas").read_text()
    count = 0
    with tempfile.TemporaryDirectory(prefix="fbc-credential-contexts-") as directory:
        scratch = Path(directory)
        for target in ("win64", "win32", "linux-x86_64"):
            for encoding in ("utf-8", "utf-8-sig", "utf-16", "utf-16-be", "utf-32", "utf-32-be"):
                data = source.encode(encoding)
                if encoding == "utf-16-be":
                    data = b"\xfe\xff" + data
                elif encoding == "utf-32-be":
                    data = b"\x00\x00\xfe\xff" + data
                source_path = scratch / "contexts.bas"
                output_path = scratch / "contexts.c"
                model_path = scratch / "contexts.fbcsem"
                source_path.write_bytes(data)
                command = [str(args.fbc.resolve()), "-prefix", str(args.root), "-target", target,
                           "-r", "-o", str(output_path), "-semantic-model", str(model_path), str(source_path)]
                result = subprocess.run(command, capture_output=True, text=True, timeout=60)
                if result.returncode:
                    raise AssertionError(f"{target}/{encoding}: {result.stdout}{result.stderr}")
                model = Model.read(model_path)
                assert model.capabilities[1]["source-assignment-targets"] == "available"
                assert model.capabilities[1]["formal-default-inputs"] == "available"

                assignments = [(identity, properties) for (domain, identity), properties in model.properties.items()
                               if domain == "expression" and "source-assignment-symbol" in properties]
                assignment_lines = {int(next(row for row in model.records["E"]
                                             if int(row[1]) == identity)[4])
                                    for identity, _ in assignments}
                assert {18, 20, 21, 23, 24} <= assignment_lines, (target, encoding, assignment_lines)
                assert all(properties["source-assignment-kind"] in ("assignment", "initializer")
                           for _, properties in assignments)

                defaults = [(identity, int(properties["formal-default-expression"]))
                            for (domain, identity), properties in model.properties.items()
                            if domain == "symbol" and "formal-default-expression" in properties]
                declared_names = {identity: properties.get("declaration-name", "").lower()
                                  for (domain, identity), properties in model.properties.items()
                                  if domain == "symbol"}
                assert sorted(declared_names[identity] for identity, _ in defaults) == [
                    "ordinary", "ordinary", "password"
                ], (target, encoding, defaults)
                assert all(("expression", expression) in model.constants for _, expression in defaults)

                call_formals = []
                for (domain, node_id), properties in model.properties.items():
                    if domain != "node" or "call-argument-expression" not in properties:
                        continue
                    formal = int(model.nodes[node_id][9])
                    if formal in declared_names:
                        call_formals.append(declared_names[formal])
                assert "api_token" in call_formals and "ordinary" in call_formals

                observed_output = output_path.read_bytes()
                result = subprocess.run([item for item in command if item not in ("-semantic-model", str(model_path))],
                                        capture_output=True, text=True, timeout=60)
                assert result.returncode == 0, result.stdout + result.stderr
                assert output_path.read_bytes() == observed_output, "Semantic observations changed generated code"
                count += 1

                rows = [row[:] for row in model.rows]
                require_rejection(Model, remove_detail(rows, "source-assignment-kind"),
                                  "Incomplete source assignment receipt accepted")
                count += 1
                require_rejection(Model, remove_detail(rows, "formal-default-expression"),
                                  "Missing source formal input accepted")
                count += 1
                damaged = [row[:] for row in rows]
                default_row = next(row for row in damaged
                                   if row[0] == "K" and row[3] == "formal-default-expression")
                default_row[4] = str(len(model.records["E"]) + 1)
                require_rejection(Model, damaged, "Dangling formal default expression accepted")
                count += 1

        for option in ("-semantic-model-bindings", "-semantic-model-expressions"):
            result = subprocess.run([str(args.fbc.resolve()), "-prefix", str(args.root), "-r",
                                     "-o", str(output_path), option, str(model_path), str(source_path)],
                                    capture_output=True, text=True, timeout=60)
            assert result.returncode == 0, result.stdout + result.stderr
            model = Model.read(model_path, bindings_only=option.endswith("bindings"),
                               expressions_only=option.endswith("expressions"))
            assert model.capabilities[1]["source-assignment-targets"] == "unavailable"
            assert model.capabilities[1]["formal-default-inputs"] == "unavailable"
            assert not any("source-assignment-symbol" in properties or
                           "formal-default-expression" in properties
                           for properties in model.properties.values())
            count += 1
    print(f"Compiler credential-context checks passed: {count} cases.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

# end of test-compiler-credential-contexts.py
