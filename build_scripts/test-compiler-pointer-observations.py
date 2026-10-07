"""Project: FreeBASIC semantic-sidecar tests
File: test-compiler-pointer-observations.py
Purpose: Verify dereference/index inputs and allocation prototype statements.
Responsibilities: Typed identity, macros, targets, encodings and code identity.
This file intentionally does NOT execute the pointer fixtures.
"""
import argparse
from pathlib import Path
import subprocess
import sys
import tempfile


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fbc", required=True, type=Path)
    parser.add_argument("--linter-root", type=Path, default=Path(r"C:\fblint"))
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    sys.path.insert(0, str(root / "tests" / "semantic-sidecar"))
    from sidecar import Model

    count = 0
    fixture_names = ("test_pointer_dereference_semantics.bas", "test_memory_operator_pairs.bas", "pointer-index-inputs.bas")
    with tempfile.TemporaryDirectory(prefix="fbc-pointer-observations-") as directory:
        scratch = Path(directory)
        for target in ("win64", "win32", "linux-x86_64"):
            for encoding in ("utf-8", "utf-8-sig", "utf-16", "utf-16-be", "utf-32", "utf-32-be"):
                for fixture_name in fixture_names:
                    fixture_path = (root / "tests" / "semantic-sidecar" / fixture_name) if fixture_name == fixture_names[2] else (args.linter_root / "fixtures" / fixture_name)
                    source = fixture_path.read_text()
                    data = source.encode(encoding)
                    if encoding == "utf-16-be":
                        data = b"\xfe\xff" + data
                    elif encoding == "utf-32-be":
                        data = b"\x00\x00\xfe\xff" + data
                    path = scratch / fixture_name
                    path.write_bytes(data)
                    output = scratch / "observations.c"
                    model_path = scratch / "observations.fbcsem"
                    base = [str(args.fbc.resolve()), "-prefix", str(root), "-target", target,
                            "-r", "-gen", "gcc", "-o", str(output)]
                    result = subprocess.run(base + ["-semantic-model", str(model_path), str(path)],
                                            capture_output=True, text=True, timeout=60)
                    assert result.returncode == 0, result.stdout + result.stderr
                    model = Model.read(model_path)
                    assert model.capabilities[1]["parsed-pointer-dereferences"] == "available"
                    assert model.capabilities[1]["procedure-prototype-statements"] == "available"
                    assert model.capabilities[1]["parsed-pointer-indexes"] == "available"
                    assert model.capabilities[1]["pointer-index-lvalues"] == "available"
                    if fixture_name == fixture_names[0]:
                        observations = {identity: props for (domain, identity), props in model.properties.items()
                                        if domain == "expression" and "pointer-dereference-count" in props}
                        assert len(observations) == 11, observations
                        assert sorted(props["pointer-dereference-count"] for props in observations.values()) == ["1"] * 10 + ["2"]
                        expressions = {int(row[1]): row for row in model.records["E"]}
                        assert {int(expressions[identity][4]) for identity in observations} == set(range(14, 24)) | {25}
                    elif fixture_name == fixture_names[1]:
                        prototypes = [props for (domain, _), props in model.properties.items()
                                      if domain == "symbol" and any(key.startswith("procedure-prototype-statement-") for key in props)]
                        assert len(prototypes) == 11, prototypes
                        # Three macro prototypes have no editable DCL range.
                        declarations = [row for row in model.records["DCL"] if row[3] == "procedure-prototype"]
                        assert len(declarations) == 8, declarations
                    else:
                        observations = {identity: props for (domain, identity), props in model.properties.items()
                                        if domain == "expression" and "pointer-index-operand" in props}
                        assert len(observations) == 16, observations
                        expressions = {int(row[1]): row for row in model.records["E"]}
                        # Ordinary arrays, string indexing, overloaded [] and
                        # inactive tokens cannot establish a pointer receipt.
                        assert {int(expressions[identity][4]) for identity in observations} == set(range(22, 29)) | set(range(37, 42))
                        stores = {identity for identity in observations if int(expressions[identity][4]) in range(37, 42)}
                        linked = {(int(row[4]), int(row[2])) for row in model.records["H"]
                                  if row[1] == "node" and row[3] == "expression" and int(row[4]) in stores
                                  and row[5] == "source-expression" and int(model.nodes[int(row[2])][4]) == 20}
                        assert {expression for expression, _ in linked} == stores, linked
                        # Five targets and one RHS retain the same typed
                        # dereference identity through assignment lowering.
                        assignment_targets = {int(row[1]) for row in model.records["N"]
                                              if row[3] == "left" and int(model.nodes.get(int(row[2]), [0] * 5)[4]) == 2}
                        assert len({node for _, node in linked} & assignment_targets) == 5, linked
                        indexes = [int(props["pointer-index-index"]) for props in observations.values()]
                        assert any(model.constants.get(("expression", item)) == ("float64-bits", "0x3FF8000000000000") for item in indexes)
                    observed_code = output.read_bytes()
                    result = subprocess.run(base + [str(path)], capture_output=True, text=True, timeout=60)
                    assert result.returncode == 0, result.stdout + result.stderr
                    assert output.read_bytes() == observed_code, "Pointer observations changed generated code"
                    count += 1
        for mode in ("-semantic-model-bindings", "-semantic-model-expressions"):
            result = subprocess.run(base + [mode, str(model_path), str(path)],
                                    capture_output=True, text=True, timeout=60)
            assert result.returncode == 0, result.stdout + result.stderr
            model = Model.read(model_path, bindings_only=mode == "-semantic-model-bindings",
                               expressions_only=mode == "-semantic-model-expressions")
            assert model.capabilities[1]["parsed-pointer-dereferences"] == "unavailable"
            assert model.capabilities[1]["procedure-prototype-statements"] == "unavailable"
            assert model.capabilities[1]["parsed-pointer-indexes"] == "unavailable"
            assert model.capabilities[1]["pointer-index-lvalues"] == "unavailable"
            count += 1
    print(f"Compiler pointer observation checks passed: {count} cases.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
# end of test-compiler-pointer-observations.py
