"""Project: FreeBASIC semantic-sidecar tests
File: test-compiler-compound-expressions.py
Purpose: Verify original compound operands and the selected result type.
Responsibilities: Targets, encodings and unchanged side-effect code generation.
This file intentionally does NOT execute the fixture or reconstruct source.
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

    source = (root / "tests" / "semantic-sidecar" / "compound-expression-inputs.bas").read_text()
    count = 0
    with tempfile.TemporaryDirectory(prefix="fbc-compound-expression-") as directory:
        scratch = Path(directory)
        for target in ("win64", "win32", "linux-x86_64"):
            for encoding in ("utf-8", "utf-8-sig", "utf-16", "utf-16-be", "utf-32", "utf-32-be"):
                data = source.encode(encoding)
                if encoding == "utf-16-be":
                    data = b"\xfe\xff" + data
                elif encoding == "utf-32-be":
                    data = b"\x00\x00\xfe\xff" + data
                path = scratch / "compound.bas"
                path.write_bytes(data)
                output = scratch / "compound.c"
                model_path = scratch / "compound.fbcsem"
                base = [str(args.fbc.resolve()), "-prefix", str(root), "-target", target,
                        "-r", "-gen", "gcc", "-o", str(output)]
                result = subprocess.run(base + ["-semantic-model", str(model_path), str(path)],
                                        capture_output=True, text=True, timeout=60)
                assert result.returncode == 0, result.stdout + result.stderr
                model = Model.read(model_path)
                assert model.capabilities[1]["parsed-compound-operators"] == "available"
                operations = [row for row in model.records["EX"] if row[2] == "binary"]
                assert sorted(row[3] for row in operations) == ["add", "integer-divide", "modulo", "modulo", "multiply"]
                expressions = {row[1]: row for row in model.records["E"]}
                destinations = [(identity, props) for (domain, identity), props in model.properties.items()
                                if domain == "expression" and props.get("assignment-kind") == "assignment"]
                assert len(destinations) == 5
                assert all(int(props["assignment-target-dtype"]) & 31 == 11 for _, props in destinations)
                for operation in operations:
                    expression = expressions[operation[1]]
                    assert expression[2] == "0" and operation[7] == "1"
                    assert int(expressions[operation[4]][12]) & 31 == 11
                    if operation[3] == "multiply":
                        assert int(expressions[operation[5]][12]) & 31 == 16
                        assert int(expression[12]) & 31 == 16
                    else:
                        assert int(expression[12]) & 31 == 8
                observed_code = output.read_bytes()
                result = subprocess.run(base + [str(path)], capture_output=True, text=True, timeout=60)
                assert result.returncode == 0, result.stdout + result.stderr
                assert output.read_bytes() == observed_code, "Compound observations changed generated code"
                count += 1
        for mode in ("-semantic-model-bindings", "-semantic-model-expressions"):
            result = subprocess.run(base + [mode, str(model_path), str(path)],
                                    capture_output=True, text=True, timeout=60)
            assert result.returncode == 0, result.stdout + result.stderr
            model = Model.read(model_path, bindings_only=mode == "-semantic-model-bindings",
                               expressions_only=mode == "-semantic-model-expressions")
            assert model.capabilities[1]["parsed-compound-operators"] == "unavailable"
            count += 1
    print(f"Compiler compound expression checks passed: {count} cases.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
# end of test-compiler-compound-expressions.py
