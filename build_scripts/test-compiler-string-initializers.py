"""Project: FreeBASIC compiler observations
File: test-compiler-string-initializers.py
Purpose: Verify fixed-character initializer associations before lowering.
Responsibilities: Element widths, fields, encodings, targets and code identity.
This file intentionally does NOT execute truncated string fixtures.
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
    parser.add_argument("--fixture", required=True, type=Path)
    parser.add_argument("--native-reader", type=Path)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    sys.path.insert(0, str(root / "tests" / "semantic-sidecar"))
    from sidecar import Model
    source = args.fixture.read_text(encoding="utf-8")
    count = 0
    mutation_count = 0
    with tempfile.TemporaryDirectory(prefix="fbc-string-initializers-") as temporary:
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
                if args.native_reader:
                    result = subprocess.run([str(args.native_reader.resolve()), str(model_path), "valid"],
                                            capture_output=True, text=True, timeout=60)
                    assert result.returncode == 0, result.stdout + result.stderr
                    if count == 0:
                        rows = [row.split("\t") for row in model_path.read_text(encoding="ascii").splitlines()]
                        type_index = next(index for index, row in enumerate(rows)
                                          if row[0] == "K" and row[3] == "string-initializer-dtype")
                        mutation_path = directory / "qualification.semantic"
                        for value in ("516", "65540"):
                            altered = [row.copy() for row in rows]
                            altered[type_index][4] = value
                            mutation_path.write_bytes(("\n".join("\t".join(row) for row in altered) + "\n").encode("ascii"))
                            result = subprocess.run([str(args.native_reader.resolve()), str(mutation_path), "invalid"],
                                                    capture_output=True, text=True, timeout=60)
                            assert result.returncode == 0, result.stdout + result.stderr
                            try:
                                Model.read(mutation_path)
                            except ValueError:
                                pass
                            else:
                                raise AssertionError("Independent reader accepted an unselected initializer dtype")
                            mutation_count += 1
                assert model.capabilities[1]["string-initializer-targets"] == "available"
                receipts = {identity: properties for (domain, identity), properties in model.properties.items()
                            if domain == "expression" and "string-initializer-symbol" in properties}
                assert len(receipts) == 15, (target, encoding, len(receipts))
                assert len(model.relations("string-initializer")) == len(receipts)
                widths = {}
                for identity, properties in receipts.items():
                    symbol = model.symbols[int(properties["string-initializer-symbol"])]
                    widths.setdefault(symbol[2], []).append(int(properties["string-initializer-bytes"]))
                    assert ("expression", identity) in model.statement_owners
                    assert int(properties["string-initializer-bytes"]) == int(symbol[9])
                assert widths["ARRAYTEXT"] == [4, 4], widths
                assert widths["LABELTEXT"] == [3]
                assert widths["SHAREDTOOSMALL"] == [4]
                assert widths["WIDETOOSMALL"] == [4 * (2 if target.startswith("win") else 4)]
                emitted = output.read_bytes()
                result = subprocess.run(command + [str(path)], capture_output=True, text=True, timeout=60)
                assert result.returncode == 0, result.stdout + result.stderr
                assert output.read_bytes() == emitted, "Initializer observations changed generated C"
                count += 1
        for option, keyword in (("-semantic-model-expressions", "expressions_only"),
                                ("-semantic-model-bindings", "bindings_only")):
            result = subprocess.run(base + [option, str(model_path), "-o", str(output), str(path)],
                                    capture_output=True, text=True, timeout=60)
            assert result.returncode == 0, result.stdout + result.stderr
            compact = Model.read(model_path, **{keyword: True})
            assert compact.capabilities[1]["string-initializer-targets"] == "unavailable"
            assert not any(key.startswith("string-initializer-") for properties in compact.properties.values() for key in properties)
            assert not compact.relations("string-initializer")
            count += 1
    print(f"Compiler string initializer checks passed: {count} cases; generated C byte-identical; {mutation_count} additional reader mutations.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
# end of test-compiler-string-initializers.py
