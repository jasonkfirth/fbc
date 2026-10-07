"""Project: FreeBASIC compiler observations
File: test-compiler-call-arguments.py
Purpose: Verify selected caller expression receipts independently of the linter.
Responsibilities: Targets, encodings, original values and generated-code identity.
This file intentionally does NOT execute unresolved calls or install compilers.
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
    source = (root / "tests" / "semantic-sidecar" / "call-argument-inputs.bas").read_text()
    count = 0
    with tempfile.TemporaryDirectory(prefix="fbc-call-inputs-") as temporary:
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
                assert model.capabilities[1]["selected-call-argument-inputs"] == "available"
                expressions = {int(row[1]): row for row in model.records["E"]}
                receipts = {identity: int(properties["call-argument-expression"])
                            for (domain, identity), properties in model.properties.items()
                            if domain == "node" and "call-argument-expression" in properties}
                assert receipts, "No selected argument receipts"
                input_lines = {int(expressions[identity][4]) for identity in receipts.values()}
                assert set(range(22, 29)) <= input_lines, input_lines
                # Original floating values remain floating even when the
                # chosen formal converts them to Integer before export.
                assert any(model.constants.get(("expression", identity)) == ("float64-bits", "0x3FF8000000000000")
                           for identity in receipts.values()), receipts
                parsed_sources = {}
                for row in model.records["H"]:
                    if row[1] == "expression" and row[5] == "parsed-expression-source":
                        parsed_sources.setdefault(int(row[2]), []).append(int(row[4]))
                pending = list(receipts.values())
                retained = set()
                while pending:
                    identity = pending.pop()
                    if identity in retained:
                        continue
                    retained.add(identity)
                    pending.extend(parsed_sources.get(identity, []))
                assert any(model.expression_operands.get(identity, [None] * 4)[3] == "multiply"
                           for identity in retained), receipts
                for node_id in receipts:
                    assert model.properties["node", node_id]["default-argument"] == "0"
                observed_code = output.read_bytes()
                result = subprocess.run(command + [str(path)], capture_output=True, text=True, timeout=60)
                assert result.returncode == 0, result.stdout + result.stderr
                assert output.read_bytes() == observed_code, "Argument observations changed generated code"
                count += 1
        for mode in ("-semantic-model-bindings", "-semantic-model-expressions"):
            result = subprocess.run(base + [mode, str(model_path), str(path)], capture_output=True, text=True, timeout=60)
            assert result.returncode == 0, result.stdout + result.stderr
            model = Model.read(model_path, bindings_only=mode == "-semantic-model-bindings",
                               expressions_only=mode == "-semantic-model-expressions")
            assert model.capabilities[1]["selected-call-argument-inputs"] == "unavailable"
            count += 1
    print(f"Compiler selected call argument checks passed: {count} cases.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
# end of test-compiler-call-arguments.py
