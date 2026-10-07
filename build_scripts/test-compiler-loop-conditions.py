"""Project: FreeBASIC compiler observations
File: test-compiler-loop-conditions.py
Purpose: Verify original loop predicates and complete grammar observations.
Responsibilities: Encodings, targets, reader rejection and code identity.
This file intentionally does NOT execute loop fixtures.
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
    sys.path.insert(0, str(root / "tests" / "semantic-sidecar"))
    from sidecar import DETAIL_TAGS, Model
    source = (root / "tests/semantic-sidecar/loop-condition-inputs.bas").read_text(encoding="utf-8")
    count = mutations = 0
    with tempfile.TemporaryDirectory(prefix="fbc-loop-conditions-") as temporary:
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
                kinds = Counter(value for properties in model.properties.values() for key, value in properties.items() if key.startswith("loop-condition-kind:"))
                assert kinds == {"while": 3, "do-while": 1, "do-until": 1, "do": 3,
                                 "loop": 3, "loop-while": 1, "loop-until": 1}, kinds
                assert len(model.relations("loop-condition")) == 7
                assert model.capabilities[1]["loop-condition-inputs"] == "available"
                emitted = output.read_bytes()
                result = subprocess.run(command + [str(path)], capture_output=True, text=True, timeout=60)
                assert result.returncode == 0, result.stdout + result.stderr
                assert output.read_bytes() == emitted, "Loop observations changed generated C"
                if count == 0:
                    rows = [row.split("\t") for row in model_path.read_text(encoding="ascii").splitlines()]
                    predicate = next(i for i, row in enumerate(rows) if row[0] == "K" and row[3].startswith("loop-condition-expression:") and row[4] != "0")
                    bare = next(i for i, row in enumerate(rows) if row[0] == "K" and row[3].startswith("loop-condition-expression:") and row[4] == "0")
                    kind = next(i for i, row in enumerate(rows) if row[0] == "K" and row[3].startswith("loop-condition-kind:"))
                    relation = next(i for i, row in enumerate(rows) if row[0] == "H" and row[5] == "loop-condition")
                    other = next(row[4] for row in rows if row[0] == "K" and row[3].startswith("loop-condition-expression:") and row[4] not in ("0", rows[predicate][4]))
                    def changed(index: int, column: int, value: str) -> list[list[str]]:
                        altered = [row.copy() for row in rows]
                        altered[index][column] = value
                        return altered
                    cases = [rows[:predicate] + rows[predicate + 1:], rows[:kind] + rows[kind + 1:],
                             rows[:relation] + rows[relation + 1:], rows[:-1] + [rows[relation].copy(), rows[-1].copy()],
                             changed(predicate, 4, "0"), changed(bare, 4, rows[predicate][4]),
                             changed(predicate, 4, other), changed(kind, 4, "loop-until"),
                             changed(kind, 4, "unknown"), changed(predicate, 3, "loop-condition-unknown:1"),
                             changed(predicate, 1, "expression"), changed(predicate, 2, "99999999"),
                             changed(relation, 6, "0"), changed(relation, 4, other),
                             changed(relation, 6, "99999999"),
                             changed(predicate, 3, "loop-condition-expression:99999999")]
                    capability = next(i for i, row in enumerate(rows) if row[0] == "CAP" and row[2] == "loop-condition-inputs")
                    cases.append(changed(capability, 3, "unavailable"))
                    conflicting = rows[:-1] + [rows[predicate].copy(), rows[-1].copy()]
                    conflicting[-2][4] = other
                    cases.append(conflicting)
                    for altered in cases:
                        # Keep ordinary completion totals valid. Rejection must
                        # come from the broken observation, not stale counts.
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
                            raise AssertionError(f"Independent reader accepted loop mutation {mutations}")
                        result = subprocess.run([str(args.native_reader.resolve()), str(mutation), "invalid"], capture_output=True, text=True, timeout=60)
                        assert result.returncode == 0, (mutations, result.stdout + result.stderr)
                        mutations += 1
                count += 1
        for option, keyword in (("-semantic-model-expressions", "expressions_only"), ("-semantic-model-bindings", "bindings_only")):
            result = subprocess.run(base + [option, str(model_path), "-o", str(output), str(path)], capture_output=True, text=True, timeout=60)
            assert result.returncode == 0, result.stdout + result.stderr
            compact = Model.read(model_path, **{keyword: True})
            assert compact.capabilities[1]["loop-condition-inputs"] == "unavailable"
            assert not any(key.startswith("loop-condition-") for properties in compact.properties.values() for key in properties)
            assert not compact.relations("loop-condition")
            count += 1
    print(f"Compiler loop condition checks passed: {count} cases; generated C byte-identical; {mutations} reader mutations.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
# end of test-compiler-loop-conditions.py
