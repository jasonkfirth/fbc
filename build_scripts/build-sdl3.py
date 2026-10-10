#!/usr/bin/env python3
"""Project: FreeBASIC SDL3 bindings
File: build-sdl3.py
Purpose: Fetch and build the upstream libraries used by the SDL3 examples.
Responsibilities: Verify pinned archives, preserve sources, and install locally.
This file intentionally does NOT contain: system package installation or compiler changes.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tarfile
import urllib.request


# Release archives include their own licenses and decoder sources. Keep the
# complete archives, rather than replacing them with unversioned Git checkouts.
LIBRARIES = (
    ("libsdl-org/SDL", "release-3.4.18", "SDL3-3.4.18",
     "9c75cf16330322c217dedd2e0609f1124f1b54b8633e763467b4684d0f4334a3"),
    ("libsdl-org/SDL_image", "release-3.4.8", "SDL3_image-3.4.8",
     "e8223b424bc7541cf84ffff5cf7e4f24ec3711c60462cb1bd6dc1d7bd7f25477"),
    ("libsdl-org/SDL_mixer", "release-3.2.4", "SDL3_mixer-3.2.4",
     "182a07c745375e113dc740d43964ff21b0be29f29f59876c4dbc4db3d32f6901"),
    ("libsdl-org/SDL_ttf", "release-3.2.2", "SDL3_ttf-3.2.2",
     "63547d58d0185c833213885b635a2c0548201cc8f301e6587c0be1a67e1e045d"),
    ("libsdl-org/SDL_net", "release-3.2.0", "SDL3_net-3.2.0",
     "098522fc26d4e302ef9348aee6e76e67fe504dfefd7f596236568f8330570c41"),
    ("icculus/SDL_sound", "v3.2.0", "SDL3_sound-3.2.0",
     "3348f3ecc653e21f2b7ed8e409f07c67b179fbbfba3b99c85698e6e5d7b5924c"),
)

BUILD_OPTIONS = {
    "SDL": ("SDL_SHARED=ON", "SDL_STATIC=OFF", "SDL_TEST_LIBRARY=ON",
            "SDL_TESTS=OFF", "SDL_EXAMPLES=OFF"),
    "SDL_image": ("SDLIMAGE_VENDORED=OFF", "SDLIMAGE_SAMPLES=OFF", "SDLIMAGE_TESTS=OFF"),
    "SDL_mixer": ("SDLMIXER_VENDORED=OFF", "SDLMIXER_EXAMPLES=OFF", "SDLMIXER_TESTS=OFF"),
    "SDL_ttf": ("SDLTTF_VENDORED=OFF", "SDLTTF_SAMPLES=OFF"),
    "SDL_net": ("SDLNET_SAMPLES=OFF",),
    "SDL_sound": ("SDLSOUND_BUILD_STATIC=OFF", "SDLSOUND_BUILD_TEST=OFF",
                  "SDLSOUND_BUILD_DOCS=OFF"),
}


def digest(path: Path) -> str:
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def fetch(work: Path, repository: str, tag: str, name: str, expected: str) -> Path:
    archive_path = work / "downloads" / (name + ".tar.gz")
    source = work / "sources" / name
    url = f"https://github.com/{repository}/releases/download/{tag}/{name}.tar.gz"
    if not archive_path.exists():
        pending = archive_path.with_suffix(".partial")
        try:
            with urllib.request.urlopen(url, timeout=60) as response, pending.open("wb") as output:
                while block := response.read(1024 * 1024):
                    output.write(block)
            if digest(pending) != expected:
                raise ValueError("Archive checksum mismatch: " + name)
            pending.replace(archive_path)
        finally:
            pending.unlink(missing_ok=True)
    if digest(archive_path) != expected:
        raise ValueError("Archive checksum mismatch: " + name)
    if not source.exists():
        # The data filter rejects paths and links that escape the extraction
        # directory. It is available in Python 3.12 and newer.
        with tarfile.open(archive_path) as archive:
            archive.extractall(work / "sources", filter="data")
    if not (source / "CMakeLists.txt").is_file():
        raise ValueError("Incomplete source directory: " + str(source))
    return source


def invoke(command: list[str], log: Path) -> None:
    with log.open("w") as output:
        output.write("command: " + repr(command) + "\n")
        output.flush()
        result = subprocess.run(command, stdout=output, stderr=subprocess.STDOUT)
    if result.returncode:
        raise RuntimeError("Command failed; see " + str(log))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--workdir", type=Path,
                        default=Path(__file__).resolve().parents[1] / "out/sdl3")
    parser.add_argument("--jobs", type=int, default=min(os.cpu_count() or 1, 8))
    parser.add_argument("--download-only", action="store_true")
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error("--jobs must be positive")
    work = args.workdir.resolve()
    for folder in ("downloads", "sources", "build", "prefix"):
        (work / folder).mkdir(parents=True, exist_ok=True)
    records = []
    for repository, tag, name, expected in LIBRARIES:
        source = fetch(work, repository, tag, name, expected)
        library = repository.split("/")[-1]
        family = "SDL3" + library[3:]
        records.append(dict(repository=repository, tag=tag, name=name, sha256=expected))
        if args.download_only:
            print(name, "verified", flush=True)
            continue
        build = work / "build" / family
        configure = ["cmake", "-S", str(source), "-B", str(build),
                     "-DCMAKE_BUILD_TYPE=Release", "-DBUILD_SHARED_LIBS=ON",
                     "-DCMAKE_INSTALL_PREFIX=" + str(work / "prefix"),
                     "-DCMAKE_PREFIX_PATH=" + str(work / "prefix")]
        configure += ["-D" + option for option in BUILD_OPTIONS[library]]
        invoke(configure, work / ("configure-" + family + ".log"))
        invoke(["cmake", "--build", str(build), "--parallel", str(args.jobs)],
               work / ("build-" + family + ".log"))
        invoke(["cmake", "--install", str(build)], work / ("install-" + family + ".log"))
        print(name, "installed in", work / "prefix", flush=True)
    (work / "upstream.json").write_text(json.dumps(records, indent=2) + "\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

# end of build-sdl3.py
