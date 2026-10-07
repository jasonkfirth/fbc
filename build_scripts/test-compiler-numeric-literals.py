"""Project: FreeBASIC semantic-sidecar tests
File: test-compiler-numeric-literals.py
Purpose: Verify parser literal origin independently of constant values.
Responsibilities: Radices, converted values, targets, encodings and code identity.
This file intentionally does NOT execute the fixture or infer literal origins.
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

    source = """' Project: FreeBASIC numeric literal observations
' File: literals.bas
' Purpose: Verify actual parsed literal kinds and canonical radices.
' Responsibilities: Folded, named, macro and inactive inputs.
' This file intentionally does NOT execute its operations.
#lang "fb"
#define CompareValue(value) ((value) < 100)
Const As Integer limitValue = 100
Dim As Integer inputValue, resultValue
Dim As Single roundedValue = 16777217.0f
resultValue = inputValue < limitValue
resultValue = inputValue < (50 + 50)
resultValue = inputValue And &HFF
resultValue = inputValue And &O77
resultValue = inputValue And &B101
resultValue = CompareValue(inputValue)
#if 0
    resultValue = inputValue < 999
#endif
' end of literals.bas
"""
    count = 0
    with tempfile.TemporaryDirectory(prefix="fbc-numeric-literals-") as directory:
        scratch = Path(directory)
        for target in ("win64", "win32", "linux-x86_64"):
            for encoding in ("utf-8", "utf-8-sig", "utf-16", "utf-16-be", "utf-32", "utf-32-be"):
                data = source.encode(encoding)
                if encoding == "utf-16-be":
                    data = b"\xfe\xff" + data
                elif encoding == "utf-32-be":
                    data = b"\x00\x00\xfe\xff" + data
                path = scratch / "literals.bas"
                path.write_bytes(data)
                output = scratch / "literals.c"
                model_path = scratch / "literals.fbcsem"
                base = [str(args.fbc.resolve()), "-prefix", str(root), "-target", target,
                        "-r", "-gen", "gcc", "-o", str(output)]
                result = subprocess.run(base + ["-semantic-model", str(model_path), str(path)],
                                        capture_output=True, text=True, timeout=60)
                assert result.returncode == 0, result.stdout + result.stderr
                model = Model.read(model_path)
                assert model.capabilities[1]["parsed-numeric-literals"] == "available"
                literals = [props for (domain, _), props in model.properties.items()
                            if domain == "expression" and "numeric-literal-kind" in props]
                # The #if condition is parsed; its inactive body is skipped.
                assert len(literals) == 9, literals
                assert sorted(props["numeric-literal-base"] for props in literals) == ["binary"] + ["decimal"] * 6 + ["hex", "octal"]
                assert all(props["numeric-literal-text-complete"] == "1" for props in literals)
                assert sum(props["numeric-literal-kind"] == "float" for props in literals) == 1
                assert all(props["numeric-literal-text"] != "999" for props in literals)
                observed_code = output.read_bytes()
                result = subprocess.run(base + [str(path)], capture_output=True, text=True, timeout=60)
                assert result.returncode == 0, result.stdout + result.stderr
                assert output.read_bytes() == observed_code, "Literal observations changed generated code"
                count += 1
        for mode in ("-semantic-model-bindings", "-semantic-model-expressions"):
            result = subprocess.run(base + [mode, str(model_path), str(path)],
                                    capture_output=True, text=True, timeout=60)
            assert result.returncode == 0, result.stdout + result.stderr
            model = Model.read(model_path, bindings_only=mode == "-semantic-model-bindings",
                               expressions_only=mode == "-semantic-model-expressions")
            assert model.capabilities[1]["parsed-numeric-literals"] == "unavailable"
            count += 1
    print(f"Compiler numeric literal checks passed: {count} cases.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
# end of test-compiler-numeric-literals.py
