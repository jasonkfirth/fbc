"""Project: FreeBASIC compiler semantic tests
File: test-compiler-numeric-assignments.py
Purpose: Verify original expression types and accepted numeric destinations.
Responsibilities: Target widths, encodings, export modes and damaged receipts.
This file intentionally does NOT parse BASIC or execute unsafe test arithmetic.
"""

import argparse
from pathlib import Path
import subprocess
import sys
import tempfile


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fbc", required=True, type=Path)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    args = parser.parse_args()
    sys.path.insert(0, str(args.root / "tests" / "semantic-sidecar"))
    from sidecar import Model

    source = (args.root / "tests" / "semantic-sidecar" / "numeric-assignment-targets.bas").read_text()
    count = 0
    with tempfile.TemporaryDirectory(prefix="fbc-numeric-targets-") as directory:
        scratch = Path(directory)
        for target in ("win64", "win32", "linux-x86_64"):
            for encoding in ("utf-8", "utf-8-sig", "utf-16", "utf-16-be", "utf-32", "utf-32-be"):
                data = source.encode(encoding)
                if encoding == "utf-16-be":
                    data = b"\xfe\xff" + data
                elif encoding == "utf-32-be":
                    data = b"\x00\x00\xfe\xff" + data
                path = scratch / "targets.bas"
                path.write_bytes(data)
                model_path = scratch / "targets.fbcsem"
                command = [str(args.fbc.resolve()), "-prefix", str(args.root), "-target", target,
                           "-r", "-o", str(scratch / "targets.asm"), "-semantic-model", str(model_path), str(path)]
                result = subprocess.run(command, capture_output=True, text=True, timeout=60)
                if result.returncode:
                    raise AssertionError(f"{target}/{encoding}: {result.stdout}{result.stderr}")
                model = Model.read(model_path)
                assert model_path.read_text().splitlines()[0] == "FBCSEM\t27\t1.20.4"
                expressions = {int(row[1]): row for row in model.records["E"]}
                receipts = [(identity, props) for (domain, identity), props in model.properties.items()
                            if domain == "expression" and "assignment-kind" in props]
                assert len(receipts) == 6, (target, encoding, receipts)
                assert sum(props["assignment-kind"] == "initializer" for _, props in receipts) == 4
                assert sum(props["assignment-kind"] == "assignment" for _, props in receipts) == 2
                assert sum(int(props["assignment-target-dtype"]) & 31 == 2 for _, props in receipts) == 5
                # LONG operands are retained. Arithmetic separately uses the
                # compiler's native INTEGER promotion, including Win64's width.
                assert any(int(expressions[identity][12]) & 31 == 11 and
                           int(props["assignment-target-dtype"]) & 31 == 2 for identity, props in receipts)
                assert any(int(expressions[identity][12]) & 31 == 8 and
                           int(props["assignment-target-dtype"]) & 31 == 13 for identity, props in receipts)
                assert model.capabilities[1]["numeric-assignment-targets"] == "available"
                casts = [row for row in model.records["EX"] if row[2] == "cast"]
                assert len(casts) == 3, (target, encoding, casts)
                assert all(int(row[4]) in expressions and row[5] == "0" for row in casts)
                count += 1
                for property_name in ("assignment-kind", "assignment-target-dtype"):
                    rows = [row[:] for row in model.rows]
                    index = next(i for i, row in enumerate(rows) if row[0] == "K" and row[3] == property_name)
                    del rows[index]
                    rows[-1][12] = str(int(rows[-1][12]) - 1)
                    try:
                        Model("\n".join("\t".join(row) for row in rows) + "\n")
                    except ValueError:
                        count += 1
                    else:
                        raise AssertionError("Damaged destination receipt accepted")
        for option in ("-semantic-model-bindings", "-semantic-model-expressions"):
            result = subprocess.run([str(args.fbc.resolve()), "-prefix", str(args.root), "-r",
                                     "-o", str(scratch / "targets.asm"), option, str(model_path), str(path)],
                                    capture_output=True, text=True, timeout=60)
            assert result.returncode == 0, result.stdout + result.stderr
            model = Model.read(model_path, bindings_only=option.endswith("bindings"),
                               expressions_only=option.endswith("expressions"))
            assert model.capabilities[1]["numeric-assignment-targets"] == "unavailable"
            assert not any("assignment-kind" in props for props in model.properties.values())
            count += 1
    print(f"Numeric assignment compiler checks passed: {count} cases.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

# end of test-compiler-numeric-assignments.py
