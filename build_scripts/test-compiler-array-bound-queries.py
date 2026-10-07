"""Project: FreeBASIC compiler observations
File: test-compiler-array-bound-queries.py
Purpose: Verify array bound query identity through folding and conversion.
Responsibilities: Targets, encodings, reader closure and unchanged code emission.
This file intentionally does NOT execute array access fixtures.
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
    source = (root / "tests/semantic-sidecar/array-bound-query-inputs.bas").read_text(encoding="utf-8")
    compiler, reader = str(args.compiler.resolve()), str(args.native_reader.resolve())
    count = mutations = 0
    with tempfile.TemporaryDirectory(prefix="fbc-array-bound-queries-") as temporary:
        directory = Path(temporary)
        path, output, model_path = directory / "input.bas", directory / "input.c", directory / "input.semantic"
        base = [compiler, "-prefix", str(root), "-r", "-gen", "gcc"]
        for target in ("win32", "win64", "linux-x86_64"):
            for encoding, marker in (("utf-8", b""), ("utf-8", b"\xef\xbb\xbf"),
                                     ("utf-16-le", b"\xff\xfe"), ("utf-16-be", b"\xfe\xff"),
                                     ("utf-32-le", b"\xff\xfe\x00\x00"), ("utf-32-be", b"\x00\x00\xfe\xff")):
                path.write_bytes(marker + source.encode(encoding))
                command = base + ["-target", target, "-o", str(output)]
                result = subprocess.run(command + ["-semantic-model", str(model_path), str(path)], capture_output=True, text=True, timeout=60)
                assert result.returncode == 0, result.stdout + result.stderr
                model = Model.read(model_path)
                result = subprocess.run([reader, str(model_path), "valid"], capture_output=True, text=True, timeout=60)
                assert result.returncode == 0, result.stdout + result.stderr
                assert model.capabilities[1]["array-bound-query-inputs"] == "available"
                groups = {identity: props for (domain, identity), props in model.properties.items()
                          if domain == "expression" and "array-bound-symbol" in props}
                assert len(groups) == 11, len(groups)
                assert Counter(props["array-bound-kind"] for props in groups.values()) == {"lower": 3, "upper": 8}
                assert len(model.relations("array-bound-query")) == len(groups)
                expressions = {int(row[1]): row for row in model.records["E"]}
                floating = next(identity for identity, props in groups.items()
                                if expressions[int(props["array-bound-dimension"])][12] == "16")
                assert int(expressions[int(groups[floating]["array-bound-selected-dimension"])][12]) & 511 == 8
                # A folded constant and a runtime call both retain query membership.
                assert {expressions[identity][8] for identity in groups} >= {"16", "9"}
                emitted = output.read_bytes()
                result = subprocess.run(command + [str(path)], capture_output=True, text=True, timeout=60)
                assert result.returncode == 0, result.stdout + result.stderr
                assert output.read_bytes() == emitted, "Array bound observations changed generated C"
                if count == 0:
                    rows = [row.split("\t") for row in model_path.read_text(encoding="ascii").splitlines()]
                    positions = {key: next(i for i, row in enumerate(rows) if row[0] == "K" and row[3] == key)
                                 for key in ("array-bound-kind", "array-bound-symbol", "array-bound-dimension", "array-bound-selected-dimension")}
                    owner = rows[positions["array-bound-kind"]][2]
                    relation = next(i for i, row in enumerate(rows) if row[0] == "H" and row[5] == "array-bound-query")
                    capability = next(i for i, row in enumerate(rows) if row[0] == "CAP" and row[2] == "array-bound-query-inputs")
                    selected_float = next(i for i, row in enumerate(rows) if row[0] == "K" and row[2] == str(floating) and row[3] == "array-bound-selected-dimension")
                    foreign = next(props["array-bound-dimension"] for identity, props in groups.items()
                                   if model.statement_owners["expression", identity] != model.statement_owners["expression", int(owner)])
                    scalar = next(row[1] for row in model.records["S"] if row[2].upper() == "DIMENSIONINDEX")
                    def changed(index: int, column: int, value: str) -> list[list[str]]:
                        altered = [row.copy() for row in rows]
                        altered[index][column] = value
                        return altered
                    cases = [rows[:index] + rows[index + 1:] for index in (*positions.values(), relation)]
                    cases += [changed(positions["array-bound-kind"], 4, "invalid"),
                              changed(positions["array-bound-kind"], 3, "array-bound-unknown"),
                              changed(positions["array-bound-symbol"], 4, scalar),
                              changed(positions["array-bound-symbol"], 4, "0"),
                              changed(positions["array-bound-symbol"], 4, "99999999"),
                              changed(positions["array-bound-symbol"], 1, "symbol"),
                              changed(positions["array-bound-dimension"], 4, "0"),
                              changed(positions["array-bound-dimension"], 4, owner),
                              changed(positions["array-bound-dimension"], 4, foreign),
                              changed(positions["array-bound-selected-dimension"], 4, "0"),
                              changed(positions["array-bound-selected-dimension"], 4, owner),
                              changed(positions["array-bound-selected-dimension"], 4, foreign),
                              changed(selected_float, 4, groups[floating]["array-bound-dimension"]),
                              changed(relation, 6, "1"), changed(relation, 2, scalar),
                              changed(relation, 1, "node"), changed(capability, 3, "unavailable"),
                              rows[:-1] + [rows[relation].copy(), rows[-1].copy()]]
                    conflict = rows[:-1] + [rows[positions["array-bound-kind"]].copy(), rows[-1].copy()]
                    conflict[-2][4] = "upper"
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
                            raise AssertionError(f"Independent reader accepted bound query mutation {mutations}")
                        result = subprocess.run([reader, str(mutation), "invalid"], capture_output=True, text=True, timeout=60)
                        assert result.returncode == 0, (mutations, result.stdout + result.stderr)
                        mutations += 1
                count += 1
        for option, keyword in (("-semantic-model-expressions", "expressions_only"), ("-semantic-model-bindings", "bindings_only")):
            result = subprocess.run(base + [option, str(model_path), "-o", str(output), str(path)], capture_output=True, text=True, timeout=60)
            assert result.returncode == 0, result.stdout + result.stderr
            compact = Model.read(model_path, **{keyword: True})
            assert compact.capabilities[1]["array-bound-query-inputs"] == "unavailable"
            assert not any(key.startswith("array-bound-") for props in compact.properties.values() for key in props)
            assert not compact.relations("array-bound-query")
            count += 1
    print(f"Compiler array bound query checks passed: {count} cases; generated C byte-identical; {mutations} reader mutations.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
# end of test-compiler-array-bound-queries.py
