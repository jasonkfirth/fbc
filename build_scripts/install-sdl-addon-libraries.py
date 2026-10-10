#!/usr/bin/env python3
"""Project: FreeBASIC SDL addon SDK
File: install-sdl-addon-libraries.py
Purpose: Install verified Windows import libraries and their matching runtime files.
Responsibilities: Check machine types, retain notices, and record installed hashes.
This file intentionally does NOT contain: synthetic exports or application execution.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess


ROOT = Path(__file__).resolve().parents[1]
LIBRARIES = ("SDL", "SDL_image", "SDL_mixer", "SDL_net", "SDL_ttf", "SDL_sound", "SDL_gfx",
             "SDL_gpu", "SDL_rtf", "SDL_FontCache", "SDL_Pango", "SDL2", "SDL2_image", "SDL2_mixer",
             "SDL2_net", "SDL2_ttf", "SDL2_sound", "SDL2_gfx", "SDL2_gpu", "SDL2_rtf", "SDL2_FontCache",
             "SDL2_bgi", "SDL3", "SDL3_image", "SDL3_mixer", "SDL3_net", "SDL3_ttf", "SDL3_sound",
             "SDL3_gfx", "SDL3_bgi", "SDL3_rtf", "SDL3_shadercross")
MACHINES = {"win32": "IMAGE_FILE_MACHINE_I386", "win64": "IMAGE_FILE_MACHINE_AMD64",
            "win32-aarch64": "IMAGE_FILE_MACHINE_ARM64"}
SYSTEM_DLLS = set("advapi32 bcrypt comctl32 comdlg32 crypt32 dnsapi dsound dwrite gdi32 imm32 "
                 "iphlpapi kernel32 msimg32 msvcrt netapi32 ntdll ole32 oleaut32 opengl32 rpcrt4 "
                 "setupapi shell32 shlwapi user32 userenv usp10 version winmm ws2_32 wsock32".split())


def digest(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def verify_machine(path, target):
    headers = subprocess.run(["llvm-readobj", "--file-headers", str(path)],
                             capture_output=True, text=True, check=True).stdout
    machines = set(re.findall(r"Machine: (IMAGE_FILE_MACHINE_\w+)", headers))
    if machines != {MACHINES[target]}:
        raise ValueError("Wrong or missing architecture: " + str(path))


def verify_dependencies(directory, roots):
    """Check the transitive DLL imports reachable from the SDL libraries."""
    files = {p.name.lower(): p for p in directory.glob("*.dll")}
    pending = [p.name.lower() for p in roots]
    visited = set()
    system = set()
    exports = {}
    symbols_checked = 0
    while pending:
        name = pending.pop()
        if name in visited:
            continue
        visited.add(name)
        imports = subprocess.run(["llvm-readobj", "--coff-imports", str(files[name])],
                                 capture_output=True, text=True, check=True).stdout
        for block in re.findall(r"Import \{\n(.*?)\n\}", imports, re.S):
            match = re.search(r"^  Name: (.+\.dll)$", block, re.M | re.I)
            if not match:
                raise ValueError("Cannot read DLL import in " + name)
            dependency = match.group(1).lower()
            if dependency in files:
                pending.append(dependency)
                if dependency not in exports:
                    table = subprocess.run(["llvm-readobj", "--coff-exports", str(files[dependency])],
                                           capture_output=True, text=True, check=True).stdout
                    exports[dependency] = set(re.findall(r"^  Name: (.+)$", table, re.M))
                # A DLL with the right filename can still be an incompatible
                # version. Check the named imports against its actual exports.
                symbols = re.findall(r"^  Symbol: (.+) \(\d+\)$", block, re.M)
                missing = set(symbols) - exports[dependency]
                if missing:
                    raise ValueError("Missing exports in " + dependency + " needed by " + name +
                                     ": " + ", ".join(sorted(missing)))
                symbols_checked += len(symbols)
            elif dependency.startswith(("api-ms-win-", "ext-ms-win-")) or dependency[:-4] in SYSTEM_DLLS:
                system.add(dependency)
            else:
                raise ValueError("Missing dependency " + dependency + " imported by " + name)
    return {"runtime_dlls": sorted(visited), "windows_dlls": sorted(system),
            "named_imports_checked": symbols_checked}


def install(work, target, destination):
    stage = work / "stage" / target
    destination.mkdir(parents=True, exist_ok=True)
    runtime = destination / "sdl-dlls"
    runtime.mkdir(exist_ok=True)
    entries = []
    roots = []
    for name in LIBRARIES:
        archive = stage / "lib" / ("lib" + name + ".dll.a")
        strings = subprocess.run(["strings", str(archive)], capture_output=True, text=True, check=True).stdout
        names = {line for line in strings.splitlines() if line.lower().endswith(".dll")
                 and "/" not in line and "\\" not in line}
        if len(names) != 1:
            raise ValueError("Cannot identify the real DLL imported by " + str(archive))
        dll = stage / "bin" / next(iter(names))
        if not dll.is_file():
            raise ValueError("Matching DLL is missing: " + str(dll))
        verify_machine(archive, target)
        verify_machine(dll, target)
        shutil.copy2(archive, destination / archive.name)
        shutil.copy2(dll, runtime / dll.name)
        roots.append(dll)
        # Paths are relative to this target's SDK directory so the manifest
        # also describes a copied or packaged SDK on another machine.
        entries.append({"library": name, "target": target, "archive": archive.name,
                        "dll": (Path("sdl-dlls") / dll.name).as_posix(),
                        "archive_sha256": digest(archive), "dll_sha256": digest(dll)})
    for dll in (stage / "bin").glob("*.dll"):
        verify_machine(dll, target)
        shutil.copy2(dll, runtime / dll.name)
    # Public Pango/FreeType headers also select their own libraries. Install
    # the dependency imports so applications can link using this SDK alone.
    for archive in (stage / "lib").glob("*.dll.a"):
        if archive.name not in {"lib" + name + ".dll.a" for name in LIBRARIES}:
            verify_machine(archive, target)
            shutil.copy2(archive, destination / archive.name)
    for name in (*LIBRARIES, "SDLmain", "SDL2main", "SDL3_test", "SDL2_test"):
        archive = stage / "lib" / ("lib" + name + ".a")
        if archive.is_file():
            verify_machine(archive, target)
            shutil.copy2(archive, destination / archive.name)
    notices = destination / "sdl-licenses"
    notices.mkdir(exist_ok=True)
    for path in (stage / "share/licenses").rglob("*"):
        if path.is_file():
            output = notices / path.relative_to(stage / "share/licenses")
            output.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(path, output)
    # Keep original source notices beside binary SDK files. The pinned source
    # manifest remains the map back to each exact archive used for the build.
    for tree in (work / "sources", ROOT / "out/sdl3/sources"):
        for folder in tree.glob("*/*" if tree == work / "sources" else "*"):
            if folder.is_dir():
                for path in folder.iterdir():
                    if path.is_file() and path.name.upper().startswith(("LICENSE", "COPYING")):
                        shutil.copy2(path, notices / (folder.name + "-" + path.name))
    for backend in ("DXC-Windows", "DXC-Linux"):
        for path in (work / "sources" / backend).glob("LICENSE*"):
            if path.is_file():
                shutil.copy2(path, notices / (backend + "-" + path.name))
    llvm = next((work / "toolchain/llvm-mingw").iterdir())
    shutil.copy2(llvm / "LICENSE.TXT", notices / "LLVM-LICENSE.TXT")
    triple = {"win32": "i686-w64-mingw32", "win64": "x86_64-w64-mingw32",
              "win32-aarch64": "aarch64-w64-mingw32"}[target]
    for path in (llvm / triple / "share/mingw32").glob("COPYING*"):
        shutil.copy2(path, notices / ("MinGW-" + path.name))
    (destination / "sdl-libraries.json").write_text(json.dumps(entries, indent=2) + "\n")
    (destination / "sdl-dependencies.json").write_text(
        json.dumps(verify_dependencies(runtime, roots), indent=2) + "\n")
    return entries


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--workdir", type=Path, default=ROOT / "out/sdl-addons")
    parser.add_argument("--libdir", type=Path, default=ROOT / "lib")
    args = parser.parse_args()
    entries = []
    for target in MACHINES:
        entries += install(args.workdir.resolve(), target, args.libdir.resolve() / target)
    (args.workdir / "library-manifest.json").write_text(json.dumps(entries, indent=2) + "\n")
    print(len(entries), "Windows import archives installed with matching DLLs")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

# End of install-sdl-addon-libraries.py
