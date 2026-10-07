"""Project: FreeBASIC compiler observations
File: test-compiler-diagnostics.py
Purpose: Verify compiler-selected failure context independently of the linter.
Responsibilities: Rejected modules, encodings, targets and artifact protection.
This file intentionally does NOT execute buffer or loop fixtures.
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
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    sys.path.insert(0, str(root / "tests" / "semantic-sidecar"))
    from diagnostics import Diagnostics
    from sidecar import Model
    sources = {
        "valid": ('#lang "fb"\nDim buffer As ZString Ptr\nLine Input "prompt"; *buffer, 8\n'
                  'For counter As Integer = 1 To 2\nNext counter\n', ""),
        "capacity": ('#lang "fb"\nDim buffer As WString Ptr\nLine Input "prompt"; *buffer\n',
                     "line-input-unknown-buffer-capacity"),
        "counter": ('#lang "fb"\nDim other As Integer\nFor counter As Integer = 1 To 2\nNext other\n',
                    "for-next-variable-mismatch"),
        "binding": ('#lang "fb"\nFor counter As Integer = 1 To 2\nNext unknown_counter\n',
                    "for-next-counter-binding"),
    }
    count = 0
    with tempfile.TemporaryDirectory(prefix="fbc-diagnostics-") as temporary:
        directory = Path(temporary).resolve()
        assert directory.parent == Path(tempfile.gettempdir()).resolve()
        path, output, artifact, model_path = (directory / name for name in
                                             ("input.bas", "input.c", "input.fbcdia", "input.semantic"))
        base = [str(args.compiler.resolve()), "-prefix", str(root), "-r", "-gen", "gcc"]
        for target in ("win32", "win64", "linux-x86_64"):
            for encoding, marker in (("utf-8", b""), ("utf-8", b"\xef\xbb\xbf"),
                                     ("utf-16-le", b"\xff\xfe"), ("utf-16-be", b"\xfe\xff"),
                                     ("utf-32-le", b"\xff\xfe\x00\x00"), ("utf-32-be", b"\x00\x00\xfe\xff")):
                for name, (source, kind) in sources.items():
                    path.write_bytes(marker + source.encode(encoding))
                    command = base + ["-target", target, "-o", str(output)]
                    result = subprocess.run(command + ["-semantic-diagnostics", str(artifact), str(path)],
                                            capture_output=True, text=True, timeout=60)
                    assert (result.returncode != 0) == bool(kind), (name, result.stdout, result.stderr)
                    diagnostics = Diagnostics.read(artifact, result.returncode)
                    assert diagnostics.modules[0][2] == str(path)
                    selected = [row for row in diagnostics.records if row[3] == "error"]
                    assert [row[5] for row in selected] == ([kind] if kind else []), selected
                    assert all(row[11] == "1" and int(row[12]) >= 3 for row in selected)
                    if not kind:
                        emitted = output.read_bytes()
                        ordinary = subprocess.run(command + [str(path)], capture_output=True, text=True, timeout=60)
                        assert ordinary.returncode == 0, ordinary.stdout + ordinary.stderr
                        assert output.read_bytes() == emitted, "Diagnostics changed generated C"
                    count += 1
        path.write_text(sources["valid"][0], encoding="utf-8")
        result = subprocess.run(base + ["-semantic-model", str(model_path), "-semantic-diagnostics", str(artifact),
                                       "-o", str(output), str(path)], capture_output=True, text=True, timeout=60)
        assert result.returncode == 0, result.stdout + result.stderr
        Model.read(model_path)
        Diagnostics.read(artifact, 0)
        count += 1
        original = path.read_bytes()
        result = subprocess.run(base + ["-semantic-diagnostics", str(path), "-o", str(output), str(path)],
                                capture_output=True, text=True, timeout=60)
        assert result.returncode != 0 and path.read_bytes() == original, "Diagnostic output replaced its source"
        count += 1
        artifact.write_bytes(b"preserve existing output")
        original = artifact.read_bytes()
        result = subprocess.run(base + ["-semantic-model", str(artifact), "-semantic-diagnostics", str(artifact),
                                       "-o", str(output), str(path)], capture_output=True, text=True, timeout=60)
        assert result.returncode != 0 and artifact.read_bytes() == original, "Sidecars replaced one another"
        count += 1
    print(f"Compiler diagnostic checks passed: {count} cases; successful generated C byte-identical.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
# end of test-compiler-diagnostics.py
