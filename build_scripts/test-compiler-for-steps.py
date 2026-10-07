"""Project: FreeBASIC compiler observations
File: test-compiler-for-steps.py
Purpose: Check original scalar STEP receipts independently of the linter.
Responsibilities: Values, ownership, encodings, targets and code identity.
This file intentionally does NOT execute loops or install a compiler.
"""
from __future__ import annotations
import argparse
from pathlib import Path
import subprocess
import sys
import tempfile


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--compiler", required=True, type=Path)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    sys.path.insert(0, str(root / "tests" / "semantic-sidecar"))
    from sidecar import Model
    source = (root / "tests" / "semantic-sidecar" / "scalar-for-steps.bas").read_text()
    count = 0
    with tempfile.TemporaryDirectory(prefix="fbc-for-steps-") as temporary:
        directory = Path(temporary)
        base = [str(args.compiler.resolve()), "-prefix", str(root), "-r", "-gen", "gcc"]
        for target in ("win32", "win64", "linux-x86_64"):
            for encoding, marker in (("utf-8", b""), ("utf-8", b"\xef\xbb\xbf"),
                                     ("utf-16-le", b"\xff\xfe"), ("utf-16-be", b"\xfe\xff"),
                                     ("utf-32-le", b"\xff\xfe\x00\x00"), ("utf-32-be", b"\x00\x00\xfe\xff")):
                path = directory / "inputs.bas"
                output = directory / "inputs.c"
                model_path = directory / "inputs.semantic"
                path.write_bytes(marker + source.encode(encoding))
                command = base + ["-target", target, "-o", str(output)]
                result = subprocess.run(command + ["-semantic-model", str(model_path), str(path)],
                                        capture_output=True, text=True, timeout=60)
                assert result.returncode == 0, result.stdout + result.stderr
                model = Model.read(model_path)
                assert model.capabilities[1]["scalar-for-step-inputs"] == "available"
                assert model.capabilities[1]["scalar-for-step-sites"] == "available"
                assert model.capabilities[1]["enum-for-step-inputs"] == "available"
                assert model.capabilities[1]["scalar-for-bound-inputs"] == "available"
                assert model.capabilities[1]["scalar-for-selected-steps"] == "available"
                receipts = {}
                selected = {}
                for (domain, identity), properties in model.properties.items():
                    for key, value in properties.items():
                        if domain == "symbol" and key.startswith("for-step-expression:"):
                            statement = int(key[20:])
                            line = int(model.statements[statement][13])
                            receipts[line] = int(value)
                            for role in ("start", "limit"):
                                bound = int(properties[f"for-{role}-expression:{statement}"])
                                assert model.statement_owners["expression", bound] == statement
                            selected[line] = int(properties[f"for-step-selected-expression:{statement}"])
                            if selected[line]:
                                assert model.statement_owners["expression", selected[line]] == statement
                            assert properties[f"for-counter:{statement}"] in ("local", "existing")
                            site = model.physical_locations.get(("statement", statement, "for-step"))
                            macro_site = any(row[1] == "statement" and int(row[2]) == statement and row[4] == "for-step"
                                             for row in model.records["MR"])
                            assert (site is not None or macro_site) == bool(int(value))
                assert set(receipts) == {12, 14, 16, 18, 20, 22, 24, 37, 39, 41, 46, 48, 50}, receipts
                assert receipts[16] == 0
                assert receipts[39] == 0
                assert model.constants["expression", receipts[37]] == ("signed", "0")
                assert model.constants["expression", receipts[41]] == ("signed", "1")
                assert model.constants["expression", receipts[12]] == ("signed", "-1")
                assert model.constants["expression", receipts[14]] == ("signed", "-2")
                assert model.constants["expression", receipts[18]] == ("signed", "0")
                assert model.constants["expression", receipts[20]] == ("float64-bits", "0x3FF8000000000000")
                assert model.constants["expression", receipts[24]] == ("signed", "-2147483649")
                assert selected[22] == 0
                assert model.constants["expression", selected[16]] == ("signed", "1")
                assert model.constants["expression", selected[46]] == ("signed", "0")
                assert model.constants["expression", selected[48]] == ("signed", "-128")
                assert model.constants["expression", selected[50]] == ("unsigned", "0")
                emitted = output.read_bytes()
                result = subprocess.run(command + [str(path)], capture_output=True, text=True, timeout=60)
                assert result.returncode == 0, result.stdout + result.stderr
                assert output.read_bytes() == emitted, "Observations changed generated C"
                count += 1
        for option, keyword in (("-semantic-model-expressions", "expressions_only"),
                                ("-semantic-model-bindings", "bindings_only")):
            result = subprocess.run(base + [option, str(model_path), "-o", str(output), str(path)],
                                    capture_output=True, text=True, timeout=60)
            assert result.returncode == 0, result.stdout + result.stderr
            compact = Model.read(model_path, **{keyword: True})
            assert compact.capabilities[1]["scalar-for-step-inputs"] == "unavailable"
            assert compact.capabilities[1]["scalar-for-step-sites"] == "unavailable"
            assert compact.capabilities[1]["enum-for-step-inputs"] == "unavailable"
            assert compact.capabilities[1]["scalar-for-bound-inputs"] == "unavailable"
            assert compact.capabilities[1]["scalar-for-selected-steps"] == "unavailable"
            assert not any(key.startswith("for-step-") for properties in compact.properties.values() for key in properties)
            count += 1
    print(f"Compiler scalar FOR step checks passed: {count} cases; generated C byte-identical.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
# end of test-compiler-for-steps.py
