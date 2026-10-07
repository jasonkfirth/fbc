"""Project: FreeBASIC compiler observations
File: test-compiler-file-transfers.py
Purpose: Check original typed GET/PUT inputs independently of the linter.
Responsibilities: Shapes, ownership, encodings, targets and generated code identity.
This file intentionally does NOT execute disk operations or install a compiler.
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
    source = (root / "tests" / "semantic-sidecar" / "file-transfer-inputs.bas").read_text()
    count = 0
    with tempfile.TemporaryDirectory(prefix="fbc-file-inputs-") as temporary:
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
                assert model.capabilities[1]["file-transfer-inputs"] == "available"
                receipts = {}
                expressions = {int(row[1]): row for row in model.records["E"]}
                for (domain, identity), properties in model.properties.items():
                    if "file-transfer-kind" not in properties:
                        continue
                    assert domain == "expression"
                    expression = expressions[int(properties["file-transfer-operand"])]
                    line = int(expression[4])
                    receipts[line] = (properties["file-transfer-kind"], properties["file-transfer-storage"])
                    assert int(expression[12]) != 0
                    assert model.statement_owners[domain, identity] in model.statements
                expected = {line: ("get" if line in (29, 31, 38) else "put",
                                   "array" if line in (30, 31, 35) else "scalar")
                            for line in range(28, 39)}
                assert receipts == expected, receipts
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
            assert compact.capabilities[1]["file-transfer-inputs"] == "unavailable"
            assert not any(key.startswith("file-transfer-") for properties in compact.properties.values() for key in properties)
            count += 1
    print(f"Compiler file transfer checks passed: {count} cases; generated C byte-identical.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
# end of test-compiler-file-transfers.py
