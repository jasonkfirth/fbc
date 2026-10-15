#!/usr/bin/env python3
"""FreeBASIC semantic smoke transport verification.
File: check-semantic-smoke-transport.py
Purpose: Compare default binary facts with the diagnostic text smoke artifact.
Responsibilities: Require the binary signature and independently validate both models.
This file does not infer source semantics or change compiler artifacts.
"""
import argparse
from pathlib import Path
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, required=True)
    parser.add_argument("binary", type=Path)
    parser.add_argument("text", type=Path)
    args = parser.parse_args()
    sys.path.insert(0, str(args.root / "tests/semantic-sidecar"))
    from sidecar import Model
    with args.binary.open("rb") as stream:
        if stream.read(9) != b"FBCSEMB1\n":
            raise ValueError("Default semantic smoke output is not FBCSEMB1")
    binary = Model.read(args.binary)
    text = Model.read(args.text)
    if binary.rows != text.rows:
        raise ValueError("Binary and diagnostic text semantic facts differ")
    print("Semantic smoke binary/text parity passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

# end of check-semantic-smoke-transport.py
