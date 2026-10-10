#!/usr/bin/env python3
"""Project: FreeBASIC SDL addon SDK
File: fetch-sdl-addons.py
Purpose: Fetch the exact source and dependency archives recorded for this SDK.
Responsibilities: Verify SHA-256 values and extract into isolated staging directories.
This file intentionally does NOT contain: package installation or native library builds.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import shutil
import tarfile
import urllib.request
import zipfile


ROOT = Path(__file__).resolve().parents[1]


def fetch(work, url, filename, expected, destination):
    archive = work / "downloads" / filename
    archive.parent.mkdir(parents=True, exist_ok=True)
    if not archive.exists():
        pending = archive.with_suffix(archive.suffix + ".partial")
        try:
            with urllib.request.urlopen(url, timeout=60) as response, pending.open("wb") as output:
                shutil.copyfileobj(response, output)
            if hashlib.sha256(pending.read_bytes()).hexdigest() != expected:
                raise ValueError("Checksum mismatch: " + filename)
            pending.replace(archive)
        finally:
            pending.unlink(missing_ok=True)
    if hashlib.sha256(archive.read_bytes()).hexdigest() != expected:
        raise ValueError("Cached archive checksum mismatch: " + filename)
    destination.mkdir(parents=True, exist_ok=True)
    if filename.endswith(".zip"):
        with zipfile.ZipFile(archive) as package:
            # These pinned packages contain ordinary relative paths.
            for member in package.infolist():
                name = member.filename.replace("\\", "/")
                parts = Path(name).parts
                if Path(name).is_absolute() or ".." in parts:
                    raise ValueError("Unsafe ZIP member: " + member.filename)
                output = destination / name
                if member.is_dir():
                    output.mkdir(parents=True, exist_ok=True)
                else:
                    output.parent.mkdir(parents=True, exist_ok=True)
                    with package.open(member) as source, output.open("wb") as target:
                        shutil.copyfileobj(source, target)
    else:
        with tarfile.open(archive) as package:
            package.extractall(destination, filter="data")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--workdir", type=Path, default=ROOT / "out/sdl-addons")
    args = parser.parse_args()
    work = args.workdir.resolve()
    packages = json.loads(Path(__file__).with_name("sdl-addon-packages.json").read_text())
    for package in packages["sources"]:
        fetch(work, package["url"], package["archive"], package["sha256"], work / package["directory"])
    seen = set()
    for package in packages["packages"]:
        if package["url"] in seen:
            continue
        seen.add(package["url"])
        family = package["url"].split("/mingw/", 1)[1].split("/", 1)[0]
        fetch(work, package["url"], Path(package["url"]).name, package["sha256"], work / "stage/msys" / family)
    for package in packages["core_arm64"]:
        fetch(work, package["url"], package["name"], package["sha256"], work / "sources" / package["name"].removesuffix(".zip"))
    for package in packages.get("dxc", []):
        name = "DXC-Windows" if package["name"].endswith(".zip") else "DXC-Linux"
        fetch(work, package["url"], package["name"], package["sha256"], work / "sources" / name)
    print("Pinned SDL addon sources and Windows dependencies verified")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

# End of fetch-sdl-addons.py
