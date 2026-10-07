"""Project: FreeBASIC compiler observations
File: test-compiler-array-subscripts.py
Purpose: Verify original array inputs before integer conversion and scaling.
Responsibilities: Targets, encodings, reader closure and code identity.
This file intentionally does NOT execute out-of-range accesses.
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
    parser.add_argument("--native-reader", required=True, type=Path)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    sys.path.insert(0, str(root / "tests/semantic-sidecar"))
    from sidecar import DETAIL_TAGS, Model
    source = (root / "tests/semantic-sidecar/array-subscript-inputs.bas").read_text(encoding="utf-8")
    count = mutations = 0
    with tempfile.TemporaryDirectory(prefix="fbc-array-subscripts-") as temporary:
        directory = Path(temporary)
        path, output, model_path = directory / "input.bas", directory / "input.c", directory / "input.semantic"
        base = [str(args.compiler.resolve()), "-prefix", str(root), "-r", "-gen", "gcc"]
        for target in ("win32", "win64", "linux-x86_64"):
            for encoding, marker in (("utf-8", b""), ("utf-8", b"\xef\xbb\xbf"),
                                     ("utf-16-le", b"\xff\xfe"), ("utf-16-be", b"\xfe\xff"),
                                     ("utf-32-le", b"\xff\xfe\x00\x00"), ("utf-32-be", b"\x00\x00\xfe\xff")):
                path.write_bytes(marker + source.encode(encoding))
                command = base + ["-target", target, "-o", str(output)]
                result = subprocess.run(command + ["-semantic-model", str(model_path), str(path)], capture_output=True, text=True, timeout=60)
                assert result.returncode == 0, result.stdout + result.stderr
                model = Model.read(model_path)
                result = subprocess.run([str(args.native_reader.resolve()), str(model_path), "valid"], capture_output=True, text=True, timeout=60)
                assert result.returncode == 0, result.stdout + result.stderr
                assert model.capabilities[1]["array-subscript-inputs"] == "available"
                groups = {identity: properties for (domain, identity), properties in model.properties.items() if domain == "expression" and "array-subscript-symbol" in properties}
                assert len(groups) == 14, len(groups)
                assert Counter(int(properties["array-subscript-rank"]) for properties in groups.values()) == {1: 12, 2: 2}
                assert len(model.relations("array-subscript")) == len(groups)
                expressions = {int(row[1]): row for row in model.records["E"]}
                assert any(expressions[int(properties["array-subscript-index:0"])][12] == "16" for properties in groups.values()), "Original Double index was lost"
                emitted = output.read_bytes()
                result = subprocess.run(command + [str(path)], capture_output=True, text=True, timeout=60)
                assert result.returncode == 0, result.stdout + result.stderr
                assert output.read_bytes() == emitted, "Subscript observations changed generated C"
                if count == 0:
                    rows = [row.split("\t") for row in model_path.read_text(encoding="ascii").splitlines()]
                    index = next(i for i, row in enumerate(rows) if row[0] == "K" and row[3] == "array-subscript-index:0")
                    symbol = next(i for i, row in enumerate(rows) if row[0] == "K" and row[3] == "array-subscript-symbol")
                    rank = next(i for i, row in enumerate(rows) if row[0] == "K" and row[3] == "array-subscript-rank")
                    selected = next(i for i, row in enumerate(rows) if row[0] == "K" and row[3] == "array-subscript-selected-index:0")
                    floating = next(identity for identity, properties in groups.items() if expressions[int(properties["array-subscript-index:0"])][12] == "16")
                    floating_selected = next(i for i, row in enumerate(rows) if row[0] == "K" and row[2] == str(floating) and row[3] == "array-subscript-selected-index:0")
                    relation = next(i for i, row in enumerate(rows) if row[0] == "H" and row[5] == "array-subscript")
                    capability = next(i for i, row in enumerate(rows) if row[0] == "CAP" and row[2] == "array-subscript-inputs")
                    foreign = next(properties["array-subscript-index:0"] for identity, properties in groups.items() if identity != int(rows[index][2]) and model.statement_owners["expression", identity] != model.statement_owners["expression", int(rows[index][2])])
                    def changed(row_index: int, column: int, value: str) -> list[list[str]]:
                        altered = [row.copy() for row in rows]
                        altered[row_index][column] = value
                        return altered
                    cases = [rows[:index] + rows[index + 1:], rows[:symbol] + rows[symbol + 1:],
                             rows[:rank] + rows[rank + 1:], rows[:relation] + rows[relation + 1:],
                             rows[:-1] + [rows[relation].copy(), rows[-1].copy()],
                             changed(index, 4, "0"), changed(index, 4, rows[index][2]),
                             changed(index, 4, foreign), changed(index, 3, "array-subscript-index:8"),
                             changed(index, 3, "array-subscript-index:00"), changed(index, 3, "array-subscript-unknown"),
                             changed(rank, 4, "0"), changed(rank, 4, "9"), changed(rank, 4, "2"),
                             changed(symbol, 4, "0"), changed(symbol, 4, "99999999"),
                             changed(symbol, 1, "symbol"), changed(relation, 6, "1"),
                             changed(relation, 2, "99999999"), changed(capability, 3, "unavailable"),
                             rows[:selected] + rows[selected + 1:], changed(floating_selected, 4, groups[floating]["array-subscript-index:0"]),
                             changed(selected, 4, "0"), changed(selected, 4, foreign),
                             changed(selected, 3, "array-subscript-selected-index:8")]
                    conflict = rows[:-1] + [rows[index].copy(), rows[-1].copy()]
                    conflict[-2][4] = foreign
                    cases.append(conflict)
                    for altered in cases:
                        altered = [row.copy() for row in altered]
                        totals = Counter(row[0] for row in altered)
                        for column, tag in enumerate(("M", "P", "S", "V", "N", "E", "B", "I", "D"), 2):
                            altered[-1][column] = str(totals[tag])
                        altered[-1][12] = str(sum(totals[tag] for tag in DETAIL_TAGS))
                        mutation = directory / "mutation.semantic"
                        mutation.write_bytes(("\n".join("\t".join(row) for row in altered) + "\n").encode("ascii"))
                        try:
                            Model.read(mutation)
                        except ValueError:
                            pass
                        else:
                            raise AssertionError(f"Independent reader accepted subscript mutation {mutations}")
                        result = subprocess.run([str(args.native_reader.resolve()), str(mutation), "invalid"], capture_output=True, text=True, timeout=60)
                        assert result.returncode == 0, (mutations, result.stdout + result.stderr)
                        mutations += 1
                count += 1
        for option, keyword in (("-semantic-model-expressions", "expressions_only"), ("-semantic-model-bindings", "bindings_only")):
            result = subprocess.run(base + [option, str(model_path), "-o", str(output), str(path)], capture_output=True, text=True, timeout=60)
            assert result.returncode == 0, result.stdout + result.stderr
            compact = Model.read(model_path, **{keyword: True})
            assert compact.capabilities[1]["array-subscript-inputs"] == "unavailable"
            assert not any(key.startswith("array-subscript-") for properties in compact.properties.values() for key in properties)
            assert not compact.relations("array-subscript")
            count += 1
    print(f"Compiler array subscript checks passed: {count} cases; generated C byte-identical; {mutations} reader mutations.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
# end of test-compiler-array-subscripts.py
