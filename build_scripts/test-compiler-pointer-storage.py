"""Project: FreeBASIC semantic-sidecar tests
File: test-compiler-pointer-storage.py
Purpose: Verify address inputs and storage selections before AST lowering.
Responsibilities: Targets, encodings, damaged receipts and unchanged code generation.
This file intentionally does NOT execute its unsafe allocation fixtures.
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

    source = (args.root / "tests" / "semantic-sidecar" / "pointer-storage-inputs.bas").read_text()
    count = 0
    with tempfile.TemporaryDirectory(prefix="fbc-pointer-storage-") as directory:
        scratch = Path(directory)
        for target in ("win64", "win32", "linux-x86_64"):
            for encoding in ("utf-8", "utf-8-sig", "utf-16", "utf-16-be", "utf-32", "utf-32-be"):
                data = source.encode(encoding)
                if encoding == "utf-16-be":
                    data = b"\xfe\xff" + data
                elif encoding == "utf-32-be":
                    data = b"\x00\x00\xfe\xff" + data
                path = scratch / "storage.bas"
                path.write_bytes(data)
                output = scratch / "storage.c"
                model_path = scratch / "storage.fbcsem"
                base = [str(args.fbc.resolve()), "-prefix", str(args.root), "-target", target,
                        "-r", "-o", str(output)]
                result = subprocess.run(base + ["-semantic-model", str(model_path), str(path)],
                                        capture_output=True, text=True, timeout=60)
                assert result.returncode == 0, result.stdout + result.stderr
                model = Model.read(model_path)
                addresses = [props for props in model.properties.values() if "pointer-address-kind" in props]
                allocations = [props for props in model.properties.values() if "memory-new-kind" in props]
                releases = [props for props in model.properties.values() if "memory-release-kind" in props]
                assert len(addresses) == 8 and len(allocations) == 4 and len(releases) == 3
                assert sum(props["pointer-address-temporary"] == "1" for props in addresses) == 2
                assert sum(props["memory-new-placement"] == "1" for props in allocations) == 1
                assert sum(props["memory-new-clear"] == "0" for props in allocations) == 1
                assert sorted(props["memory-new-elements"] for props in allocations) == ["1", "1", "3", "4"]
                for feature in ("parsed-pointer-addresses", "parsed-storage-families", "parsed-expression-origins"):
                    assert model.capabilities[1][feature] == "available"
                observed_code = output.read_bytes()
                result = subprocess.run(base + [str(path)], capture_output=True, text=True, timeout=60)
                assert result.returncode == 0, result.stdout + result.stderr
                assert output.read_bytes() == observed_code, "Storage observations changed generated code"
                count += 1
                rows = model_path.read_text().splitlines()
                for domain, prefix in (("expression", "pointer-address-"), ("symbol", "memory-new-")):
                    receipt = next(row.split("\t") for row in rows if row.startswith(f"K\t{domain}\t")
                                   and row.split("\t")[3] == prefix + "kind")
                    properties = [row for row in rows if row.startswith(f"K\t{domain}\t{receipt[2]}\t{prefix}")]
                    for property_row in properties:
                        damaged = [row for row in rows if row != property_row]
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
                            raise AssertionError("Incomplete storage receipt was accepted: " + property_row)
                        count += 1
        for mode in ("-semantic-model-bindings", "-semantic-model-expressions"):
            result = subprocess.run(base + [mode, str(model_path), str(path)],
                                    capture_output=True, text=True, timeout=60)
            assert result.returncode == 0, result.stdout + result.stderr
            model = Model.read(model_path, bindings_only=mode == "-semantic-model-bindings",
                               expressions_only=mode == "-semantic-model-expressions")
            for feature in ("parsed-pointer-addresses", "parsed-storage-families", "parsed-expression-origins"):
                assert model.capabilities[1][feature] == "unavailable"
            assert not any("pointer-address-kind" in props or "memory-new-kind" in props or
                           "memory-release-kind" in props for props in model.properties.values())
            count += 1
    print(f"Compiler pointer storage checks passed: {count} cases.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
# end of test-compiler-pointer-storage.py
