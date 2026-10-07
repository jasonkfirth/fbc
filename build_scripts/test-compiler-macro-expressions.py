"""Project: FreeBASIC semantic-sidecar tests
File: test-compiler-macro-expressions.py
Purpose: Verify typed expanded operators and logical invocation anchors.
Responsibilities: Targets, encodings, provenance and byte-identical code generation.
This file intentionally does NOT execute the fixture or infer expansion text.
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

    source = (root / "tests" / "semantic-sidecar" / "macro-expression-inputs.bas").read_text()
    count = 0
    with tempfile.TemporaryDirectory(prefix="fbc-macro-expression-") as directory:
        scratch = Path(directory)
        for target in ("win64", "win32", "linux-x86_64"):
            for encoding in ("utf-8", "utf-8-sig", "utf-16", "utf-16-be", "utf-32", "utf-32-be"):
                data = source.encode(encoding)
                if encoding == "utf-16-be":
                    data = b"\xfe\xff" + data
                elif encoding == "utf-32-be":
                    data = b"\x00\x00\xfe\xff" + data
                path = scratch / "macro.bas"
                path.write_bytes(data)
                output = scratch / "macro.c"
                model_path = scratch / "macro.fbcsem"
                base = [str(args.fbc.resolve()), "-prefix", str(root), "-target", target,
                        "-r", "-gen", "gcc", "-o", str(output)]
                result = subprocess.run(base + ["-semantic-model", str(model_path), str(path)],
                                        capture_output=True, text=True, timeout=60)
                assert result.returncode == 0, result.stdout + result.stderr
                model = Model.read(model_path)
                assert model.capabilities[1]["parsed-macro-expressions"] == "available"
                operations = [row for row in model.records["EX"] if row[2] in ("binary", "unary")]
                assert sorted(row[3] for row in operations) == ["add", "add", "modulo", "negate"]
                expressions = {row[1]: row for row in model.records["E"]}
                origins = {row[2] for row in model.records["MR"] if row[1] == "expression"}
                for operation in operations:
                    expression = expressions[operation[1]]
                    assert expression[2] == "0" and operation[7] == "0"
                    assert expression[1] in origins, "Expanded operation lost actual token provenance"
                observed_code = output.read_bytes()
                result = subprocess.run(base + [str(path)], capture_output=True, text=True, timeout=60)
                assert result.returncode == 0, result.stdout + result.stderr
                assert output.read_bytes() == observed_code, "Macro observations changed generated code"
                count += 1
        for mode in ("-semantic-model-bindings", "-semantic-model-expressions"):
            result = subprocess.run(base + [mode, str(model_path), str(path)],
                                    capture_output=True, text=True, timeout=60)
            assert result.returncode == 0, result.stdout + result.stderr
            model = Model.read(model_path, bindings_only=mode == "-semantic-model-bindings",
                               expressions_only=mode == "-semantic-model-expressions")
            assert model.capabilities[1]["parsed-macro-expressions"] == "unavailable"
            count += 1
    print(f"Compiler macro expression checks passed: {count} cases.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
# end of test-compiler-macro-expressions.py
