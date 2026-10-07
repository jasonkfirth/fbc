"""Project: FreeBASIC semantic-sidecar tests
File: test-compiler-size-queries.py
Purpose: Verify selected LEN/SIZEOF inputs independently of folded results.
Responsibilities: Targets, encodings, corruption and code-generation identity.
This file intentionally does NOT execute its unevaluated pointer expressions.
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
    args.root = args.root.resolve()
    sys.path.insert(0, str(args.root / "tests" / "semantic-sidecar"))
    from sidecar import Model

    source = (args.root / "tests" / "semantic-sidecar" / "size-query-inputs.bas").read_text()
    count = 0
    with tempfile.TemporaryDirectory(prefix="fbc-size-queries-") as directory:
        scratch = Path(directory)
        for target in ("win64", "win32", "linux-x86_64"):
            for encoding in ("utf-8", "utf-8-sig", "utf-16", "utf-16-be", "utf-32", "utf-32-be"):
                data = source.encode(encoding)
                if encoding == "utf-16-be":
                    data = b"\xfe\xff" + data
                elif encoding == "utf-32-be":
                    data = b"\x00\x00\xfe\xff" + data
                path = scratch / "queries.bas"
                path.write_bytes(data)
                output = scratch / "queries.c"
                model_path = scratch / "queries.fbcsem"
                base = [str(args.fbc.resolve()), "-prefix", str(args.root), "-target", target,
                        "-r", "-o", str(output)]
                result = subprocess.run(base + ["-semantic-model", str(model_path), str(path)],
                                        capture_output=True, text=True, timeout=60)
                assert result.returncode == 0, result.stdout + result.stderr
                model = Model.read(model_path)
                assert model_path.read_text().splitlines()[0] == "FBCSEM\t27\t1.20.4"
                queries = [(identity, props) for (domain, identity), props in model.properties.items()
                           if domain == "expression" and "size-query-kind" in props]
                assert len(queries) == 11, queries
                assert sum(props["size-query-kind"] == "len" for _, props in queries) == 2
                assert sum(props["size-query-input"] == "type" for _, props in queries) == 2
                assert sum(props["size-query-input"] == "array" for _, props in queries) == 1
                assert sum(int(props["size-query-dtype"]) & 0x1E0 != 0 for _, props in queries) == 8
                origins = [row for row in model.records["H"] if row[5] == "size-query-source"]
                assert origins, "Precedence unwinding lost original size query provenance"
                assert all(int(row[4]) < int(row[2]) for row in origins)
                assert model.capabilities[1]["size-query-inputs"] == "available"
                observed_code = output.read_bytes()
                result = subprocess.run(base + [str(path)], capture_output=True, text=True, timeout=60)
                assert result.returncode == 0, result.stdout + result.stderr
                assert output.read_bytes() == observed_code, "Size observations changed generated code"
                count += 1
                rows = model_path.read_text().splitlines()
                identity = str(queries[0][0])
                for key in ("size-query-kind", "size-query-dtype", "size-query-subtype",
                            "size-query-operand", "size-query-input"):
                    prefix = "\t".join(("K", "expression", identity, key)) + "\t"
                    damaged = [row for row in rows if not row.startswith(prefix)]
                    footer = damaged[-1].split("\t")
                    footer[12] = str(int(footer[12]) - (len(rows) - len(damaged)))
                    damaged[-1] = "\t".join(footer)
                    broken = scratch / "broken.fbcsem"
                    broken.write_text("\n".join(damaged) + "\n")
                    try:
                        Model.read(broken)
                    except ValueError:
                        pass
                    else:
                        raise AssertionError("Missing size-query property was accepted: " + key)
                    count += 1
        for mode in ("-semantic-model-bindings", "-semantic-model-expressions"):
            result = subprocess.run(base + [mode, str(model_path), str(path)],
                                    capture_output=True, text=True, timeout=60)
            assert result.returncode == 0, result.stdout + result.stderr
            model = Model.read(model_path, bindings_only=mode == "-semantic-model-bindings",
                               expressions_only=mode == "-semantic-model-expressions")
            assert model.capabilities[1]["size-query-inputs"] == "unavailable"
            assert not any("size-query-kind" in props for props in model.properties.values())
            count += 1
    print(f"Compiler size-query checks passed: {count} cases.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
# end of test-compiler-size-queries.py
