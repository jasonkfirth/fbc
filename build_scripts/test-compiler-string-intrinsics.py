"""Project: FreeBASIC compiler observations
File: test-compiler-string-intrinsics.py
Purpose: Verify original string intrinsic inputs independently of the linter.
Responsibilities: Folded inputs, ownership, encodings, targets and code identity.
This file intentionally does NOT execute string fixtures or install a compiler.
"""
from __future__ import annotations
import argparse
from collections import Counter
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
    source = (root / "tests" / "semantic-sidecar" / "parsed-string-intrinsics.bas").read_text()
    count = 0
    with tempfile.TemporaryDirectory(prefix="fbc-string-inputs-") as temporary:
        directory = Path(temporary)
        base = [str(args.compiler.resolve()), "-prefix", str(root), "-r", "-gen", "gcc"]
        for target in ("win32", "win64", "linux-x86_64"):
            for encoding, marker in (("utf-8", b""), ("utf-8", b"\xef\xbb\xbf"),
                                     ("utf-16-le", b"\xff\xfe"), ("utf-16-be", b"\xfe\xff"),
                                     ("utf-32-le", b"\xff\xfe\x00\x00"), ("utf-32-be", b"\x00\x00\xfe\xff")):
                path, output, model_path = directory / "inputs.bas", directory / "inputs.c", directory / "inputs.semantic"
                path.write_bytes(marker + source.encode(encoding))
                command = base + ["-target", target, "-o", str(output)]
                result = subprocess.run(command + ["-semantic-model", str(model_path), str(path)],
                                        capture_output=True, text=True, timeout=60)
                assert result.returncode == 0, result.stdout + result.stderr
                model = Model.read(model_path)
                assert model.capabilities[1]["parsed-string-intrinsics"] == "available"
                receipts = {identity: properties for (domain, identity), properties in model.properties.items()
                            if domain == "expression" and "string-intrinsic-kind" in properties}
                assert Counter(properties["string-intrinsic-kind"] for properties in receipts.values()) == {
                    "chr": 5, "wchr": 1, "uchr": 1, "trim": 3, "ltrim": 1, "rtrim": 1}
                assert len(model.relations("parsed-string-intrinsic")) == len(receipts)
                expressions = {int(row[1]): row for row in model.records["E"]}
                original_values = []
                patterns = []
                for identity, properties in receipts.items():
                    statement = model.statement_owners["expression", identity]
                    arguments = [int(properties[f"string-intrinsic-argument-{ordinal}"])
                                 for ordinal in range(1, int(properties["string-intrinsic-count"]) + 1)]
                    assert all(model.statement_owners["expression", argument] == statement for argument in arguments)
                    if properties["string-intrinsic-kind"] == "chr":
                        original_values.append([model.constants.get(("expression", argument)) for argument in arguments])
                    if properties["string-intrinsic-any"] == "1":
                        patterns.append(model.constants["expression", arguments[1]])
                assert [("signed", "937")] in original_values
                assert [("signed", "0"), None, ("signed", "256")] in original_values
                assert original_values.count([("signed", "300")]) == 2
                assert [("signed", "299")] in original_values
                # Unicode source decoding can select a wide literal. Compare
                # the decoded units while retaining each model's actual type.
                assert ("bytes", "666678") in patterns or (
                    "wide-units", "000000660000006600000078") in patterns, (target, encoding, patterns)
                assert len(patterns) == 3
                macro = [identity for identity in receipts if int(expressions[identity][2]) == 0]
                assert len(macro) == 1
                emitted = output.read_bytes()
                result = subprocess.run(command + [str(path)], capture_output=True, text=True, timeout=60)
                assert result.returncode == 0, result.stdout + result.stderr
                assert output.read_bytes() == emitted, "String observations changed generated C"
                count += 1
        for option, keyword in (("-semantic-model-expressions", "expressions_only"),
                                ("-semantic-model-bindings", "bindings_only")):
            result = subprocess.run(base + [option, str(model_path), "-o", str(output), str(path)],
                                    capture_output=True, text=True, timeout=60)
            assert result.returncode == 0, result.stdout + result.stderr
            compact = Model.read(model_path, **{keyword: True})
            assert compact.capabilities[1]["parsed-string-intrinsics"] == "unavailable"
            assert not any(key.startswith("string-intrinsic-") for properties in compact.properties.values() for key in properties)
            assert not compact.relations("parsed-string-intrinsic")
            count += 1
    print(f"Compiler string intrinsic checks passed: {count} cases; generated C byte-identical.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
# end of test-compiler-string-intrinsics.py
