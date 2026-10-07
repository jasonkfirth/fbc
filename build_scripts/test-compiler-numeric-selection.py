"""Project: FreeBASIC semantic-sidecar tests
File: test-compiler-numeric-selection.py
Purpose: Verify selected operand types and values independently of result types.
Responsibilities: Target coercion, pre-fold values, encodings and code identity.
This file intentionally does NOT reconstruct the compiler's promotion rules.
"""
import argparse
from pathlib import Path
import subprocess
import sys
import tempfile


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fbc", required=True, type=Path)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    sys.path.insert(0, str(root / "tests" / "semantic-sidecar"))
    from sidecar import Model

    source = (root / "tests" / "semantic-sidecar" / "numeric-selected-operands.bas").read_text()
    count = 0
    with tempfile.TemporaryDirectory(prefix="fbc-numeric-selection-") as directory:
        scratch = Path(directory)
        for target in ("win64", "win32", "linux-x86_64"):
            for encoding in ("utf-8", "utf-8-sig", "utf-16", "utf-16-be", "utf-32", "utf-32-be"):
                data = source.encode(encoding)
                if encoding == "utf-16-be":
                    data = b"\xfe\xff" + data
                elif encoding == "utf-32-be":
                    data = b"\x00\x00\xfe\xff" + data
                path = scratch / "selection.bas"
                path.write_bytes(data)
                output = scratch / "selection.c"
                model_path = scratch / "selection.fbcsem"
                base = [str(args.fbc.resolve()), "-prefix", str(root), "-target", target,
                        "-r", "-gen", "gcc", "-o", str(output)]
                result = subprocess.run(base + ["-semantic-model", str(model_path), str(path)],
                                        capture_output=True, text=True, timeout=60)
                assert result.returncode == 0, result.stdout + result.stderr
                model = Model.read(model_path)
                assert model.capabilities[1]["selected-numeric-operands"] == "available"
                expressions = {row[1]: row for row in model.records["E"]}
                values = {row[2]: row for row in model.records["C"] if row[1] == "expression"}
                operations = {row[1]: row for row in model.records["EX"]}
                selections = [(identity, props) for (domain, identity), props in model.properties.items()
                              if domain == "expression" and "numeric-selected-left-dtype" in props]
                assert len(selections) == 9, selections
                for identity, props in selections:
                    operation = operations[str(identity)]
                    left = expressions[props["numeric-selected-left-expression"]]
                    right = expressions[props["numeric-selected-right-expression"]]
                    assert int(left[12]) & 511 == int(props["numeric-selected-left-dtype"]) & 511
                    assert int(right[12]) & 511 == int(props["numeric-selected-right-dtype"]) & 511
                    line = int(operation[9])
                    if line == 17:
                        expected = 9 if target == "win32" else 8
                        assert int(left[12]) & 31 == expected and int(right[12]) & 31 == expected
                    elif line == 18:
                        assert int(left[12]) & 31 == 8 and int(right[12]) & 31 == 8
                    elif line == 19:
                        assert int(left[12]) & 31 == 16 and int(right[12]) & 31 == 16
                    elif line in (20, 21):
                        value = values[right[1]]
                        assert value[3:5] == ["signed", "64" if line == 20 else "-1"], value
                observed_code = output.read_bytes()
                result = subprocess.run(base + [str(path)], capture_output=True, text=True, timeout=60)
                assert result.returncode == 0, result.stdout + result.stderr
                assert output.read_bytes() == observed_code, "Numeric observations changed generated code"
                count += 1
        for mode in ("-semantic-model-bindings", "-semantic-model-expressions"):
            result = subprocess.run(base + [mode, str(model_path), str(path)],
                                    capture_output=True, text=True, timeout=60)
            assert result.returncode == 0, result.stdout + result.stderr
            model = Model.read(model_path, bindings_only=mode == "-semantic-model-bindings",
                               expressions_only=mode == "-semantic-model-expressions")
            assert model.capabilities[1]["selected-numeric-operands"] == "unavailable"
            count += 1
    print(f"Compiler numeric selection checks passed: {count} cases.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
# end of test-compiler-numeric-selection.py
