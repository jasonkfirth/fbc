#!/usr/bin/env python3
"""Project: FreeBASIC runtime library.
File: check-rtlib-structure.py
Purpose: Check runtime ownership, includes and the real Make source graph.
Responsibilities: Detect missing sources, ambiguous members and platform drift.
This file intentionally does NOT compile runtime code or simulate a target OS.
"""

from __future__ import annotations

import argparse
from collections import Counter
from pathlib import Path
import re
import subprocess
import tempfile


TARGETS = (
    "linux", "win32", "freebsd", "netbsd", "openbsd", "dragonfly", "darwin",
    "haiku", "solaris", "illumos", "android", "cygwin", "dos", "js", "wii",
    "xbox", "wince", "nuttx", "aros", "amiga", "riscos",
)
UNIX_TARGETS = set(TARGETS) - {"win32", "cygwin", "dos", "js", "wii", "xbox", "wince", "nuttx"}
SOURCE_SUFFIXES = {".c", ".h", ".s", ".cpp"}
# The legacy BASIC driver testers also include these macro-only C headers.
# Their documentation must be skipped by the BASIC preprocessor.
SHARED_BASIC_HEADERS = {"drivers/win32/fbportio.h", "drivers/wince/fbportio.h"}
QUOTED_INCLUDE = re.compile(r'^\s*#\s*include\s*"([^"\n]+)"', re.M)
COMMENTS = re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|/\*.*?\*/|//[^\n]*', re.S)

# These quoted names are supplied by platform SDKs, not this repository.
# Keep exceptions attached to their consumers so a misspelled private include
# cannot be hidden by an unrestricted system-header exemption.
SDK_INCLUDES = {
    "drivers/darwin/sys_getexename.c": {"mach-o/dyld.h"},
    "drivers/darwin/sys_getexepath.c": {"mach-o/dyld.h"},
    "drivers/dragonfly/sys_getexename.c": {"sys/sysctl.h"},
    "drivers/dragonfly/sys_getexepath.c": {"sys/sysctl.h"},
    "drivers/freebsd/sys_getexename.c": {"sys/sysctl.h"},
    "drivers/freebsd/sys_getexepath.c": {"sys/sysctl.h"},
    "files/file_attr.c": {"pdclib/_PDCLIB_int.h"},
    "drivers/xbox/file_hseteof.c": {"pdclib/_PDCLIB_int.h"},
    "drivers/nuttx/fb_nuttx_gosub.c": {"setjmp.h"},
    "drivers/nuttx/fb_nuttx_minrt.c": {"builtin/builtin.h"},
}


def without_comments(text: str) -> str:
    """Keep quoted names intact while ignoring examples in engineering notes."""
    return COMMENTS.sub(lambda match: "" if match.group().startswith(("/*", "//"))
                        else match.group(), text)


def graph(root: Path, make: str, work: Path, target: str, arch: str,
          disabled: bool = False, source_root: Path | None = None) -> dict[str, list[str]]:
    """Query Make itself; no compiler, package discovery or generated objects."""
    makefile = work / "graph.mk"
    makefile.write_text(
        "rootdir := " + root.as_posix() + "\n"
        "mkpath := $(rootdir)/mk\n"
        "srcdir := " + (source_root or root / "src").as_posix() + "\n"
        "TARGET_OS := " + target + "\nTARGET_ARCH := " + arch + "\n"
        "libfbobjdir := objects/base\nlibfbpicobjdir := objects/pic\n"
        "libfbmtobjdir := objects/mt\nlibfbmtpicobjdir := objects/mtpic\n"
        + ("DISABLE_X11 := 1\n" if disabled else "")
        + "include $(mkpath)/source-graph.mk\n"
        ".PHONY: report\nreport:\n"
        "\t$(info GROUPS=$(RTLIB_SOURCE_GROUPS))\n"
        "\t$(info SOURCES=$(RTLIB_SRC))\n"
        "\t$(info OBJECTS=$(RTLIB_OBJ))\n"
        "\t$(info PIC=$(RTLIB_PIC_OBJ))\n"
        "\t$(info MT=$(RTLIB_MT_OBJ))\n"
        "\t$(info MTPIC=$(RTLIB_MT_PIC_OBJ))\n\t@:\n",
        encoding="utf-8",
    )
    result = subprocess.run([make, "-s", "--no-print-directory", "-f", str(makefile), "report"],
                            cwd=root, capture_output=True, text=True, timeout=30)
    if result.returncode:
        raise ValueError(result.stderr.strip() or "Make graph query failed")
    rows = dict(line.split("=", 1) for line in result.stdout.splitlines() if "=" in line)
    if set(rows) != {"GROUPS", "SOURCES", "OBJECTS", "PIC", "MT", "MTPIC"}:
        raise ValueError("Make graph returned incomplete records")
    return {key: value.split() for key, value in rows.items()}


def expected_sources(rtl: Path, groups: list[str], target: str, arch: str,
                     disabled: bool) -> set[Path]:
    """Independent contract: whole C files are replaced by the highest layer."""
    selected: dict[str, Path] = {}
    drivers = rtl / "drivers"
    source_os = {"illumos": "solaris", "cygwin": "win32"}.get(target, target)
    layers = [[path for group in groups for path in (rtl / group).glob("*.c")]]
    if target in UNIX_TARGETS:
        layers.append(list((drivers / "unix").glob("*.c")))
    layers.extend([list((drivers / source_os).glob("*.c")),
                   list((drivers / source_os / arch).glob("*.c"))])
    for layer in layers:
        for path in layer:
            selected[path.name] = path.resolve()
    result = set(selected.values())
    for directory in ([drivers / "x86"] if arch == "x86" else []) + [drivers / source_os, drivers / source_os / arch]:
        result.update(path.resolve() for path in directory.glob("*.s"))
    if disabled:
        result -= {drivers / "unix/io_xfocus.c", drivers / "unix/scancodes_x11.c"}
    return result


def check_collision_guards(root: Path, make: str, work: Path) -> int:
    """Tiny synthetic layers prove rejection and normal replacement behavior."""
    source = work / "fixture/src"
    rtl = source / "rtlib"
    for directory in ("core", "arrays", "drivers/unix", "drivers/linux", "drivers/linux/x86_64"):
        (rtl / directory).mkdir(parents=True)
    for directory in ("core", "drivers/unix", "drivers/linux", "drivers/linux/x86_64"):
        (rtl / directory / "probe.c").write_text("/* fixture */\n")
    actual = graph(root, make, work, "linux", "x86_64", source_root=source)
    if [Path(path) for path in actual["SOURCES"]] != [rtl / "drivers/linux/x86_64/probe.c"]:
        raise ValueError("Target architecture did not replace lower layers")
    duplicate = rtl / "arrays/probe.c"
    duplicate.write_text("/* duplicate portable archive member */\n")
    try:
        graph(root, make, work, "linux", "x86_64", source_root=source)
    except ValueError as error:
        if "Duplicate rtlib source basename" not in str(error):
            raise
    else:
        raise ValueError("Make accepted duplicate portable archive members")
    duplicate.unlink()
    collision = rtl / "drivers/linux/probe.s"
    collision.write_text("/* C and assembly cannot own one archive member */\n")
    try:
        graph(root, make, work, "linux", "x86_64", source_root=source)
    except ValueError as error:
        if "Duplicate rtlib source basename" not in str(error):
            raise
    else:
        raise ValueError("Make accepted duplicate C/assembly archive members")
    return 3


def validate(root: Path, make: str) -> None:
    rtl = root / "src/rtlib"
    failures: list[str] = []
    files = sorted(path for path in rtl.rglob("*") if path.is_file()
                   and "obj" not in path.relative_to(rtl).parts and path.suffix in SOURCE_SUFFIXES)
    if not files:
        raise ValueError("Runtime source tree is empty")
    for path in files:
        relative = path.relative_to(rtl).as_posix()
        text = path.read_bytes().decode("latin-1")
        header = text.split("*/", 1)[0]
        for field in ("Project:", "File: " + relative, "Purpose:", "Responsibilities:",
                      "This file intentionally does NOT contain:"):
            if field not in header:
                failures.append(relative + ": header missing " + field)
        footer = "/* end of " + relative + " */"
        if relative in SHARED_BASIC_HEADERS:
            footer = "#if 0\n" + footer + "\n#endif"
            if not text.startswith("#if 0") or not text.split("*/", 1)[1].lstrip().startswith("#endif"):
                failures.append(relative + ": shared BASIC header needs a skipped documentation block")
        if not text.replace("\r\n", "\n").rstrip().endswith(footer):
            failures.append(relative + ": missing identifying footer")
        for name in QUOTED_INCLUDE.findall(without_comments(text)):
            candidates = [path.parent / name, rtl / name, root / "src" / name]
            if relative == "drivers/haiku/haiku_debug.cpp" and name == "fb_gfx_haiku.h":
                candidates.append(root / "src/gfxlib2/drivers/haiku" / name)
            if name not in SDK_INCLUDES.get(relative, set()) and not any(candidate.is_file() for candidate in candidates):
                failures.append(relative + ": unresolved local include " + name)
    cases = 0
    with tempfile.TemporaryDirectory(prefix="fbc-rtlib-structure-") as temporary:
        work = Path(temporary)
        initial = graph(root, make, work, "linux", "x86_64")
        groups = initial["GROUPS"]
        guide = (rtl / "README.md").read_text(encoding="utf-8")
        for group in groups:
            if "`" + group + "`" not in guide:
                failures.append("README.md: missing ownership for " + group)
        generic = {path for group in groups for path in (rtl / group).glob("*.c")}
        portable_roots = {Path(group).parts[0] for group in groups}
        for path in files:
            relative = path.relative_to(rtl)
            if relative.parts[0] in portable_roots and path.suffix == ".c" and path not in generic:
                failures.append(relative.as_posix() + ": portable source is outside the declared subsystem graph")
            elif len(relative.parts) > 1 and relative.parts[0] not in portable_roots | {"drivers", "static"}:
                failures.append(relative.as_posix() + ": source directory has no declared owner")
            elif relative.parts[0] == "drivers" and (len(relative.parts) < 3
                    or relative.parts[1] not in set(TARGETS) | {"unix", "x86"}):
                failures.append(relative.as_posix() + ": driver has no declared platform owner")
        for path in rtl.glob("*.c"):
            failures.append(path.name + ": portable source must belong to a subsystem")
        if len(generic) != len({path.name for path in generic}):
            failures.append("Portable subsystem basenames are not unique")
        for target in TARGETS:
            for arch in ("x86", "x86_64", "aarch64"):
                for disabled in (False, True):
                    label = target + "/" + arch + ("/no-x11" if disabled else "")
                    actual = graph(root, make, work, target, arch, disabled)
                    sources = [Path(name).resolve() for name in actual["SOURCES"]]
                    expected = expected_sources(rtl, groups, target, arch, disabled)
                    if set(sources) != expected or len(sources) != len(expected):
                        failures.append(label + ": selected source coverage differs from replacement policy")
                    duplicates = [name for name, count in Counter(path.stem for path in sources).items() if count > 1]
                    if duplicates:
                        failures.append(label + ": duplicate archive members " + ", ".join(duplicates))
                    for key, variant in (("OBJECTS", "base"), ("PIC", "pic"), ("MT", "mt"), ("MTPIC", "mtpic")):
                        objects = [(Path("objects") / variant / path.relative_to(rtl).with_suffix(".o")).as_posix()
                                   for path in sources]
                        if actual[key] != objects:
                            failures.append(label + ": " + key + " diverged from canonical objects")
                    cases += 1
        guard_cases = check_collision_guards(root, make, work)
    if failures:
        raise ValueError("\n".join(failures))
    print(f"RTLIB_STRUCTURE=PASS files={len(files)} groups={len(groups)} graph_cases={cases} guard_cases={guard_cases}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--make", default="make")
    options = parser.parse_args()
    try:
        validate(options.root.resolve(strict=True), options.make)
    except (OSError, ValueError, subprocess.TimeoutExpired) as error:
        print(error)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

# end of check-rtlib-structure.py
