#!/usr/bin/env python3
"""FreeBASIC graphics build validation.

File: check-gfx-source-graph.py
Purpose: Protect graphics ownership and platform override selection.
Responsibilities: Query Make, check source coverage, includes and dependencies.
This file intentionally does NOT compile sources or implement graphics APIs.
"""

import argparse
from pathlib import Path
import re
import subprocess
import tempfile


TARGETS = (
    ("win32", "x86"), ("win32", "x86_64"), ("win32", "aarch64"),
    ("cygwin", "x86_64"), ("wince", "arm"),
    ("linux", "x86"), ("linux", "x86_64"), ("linux", "arm"),
    ("linux", "aarch64"), ("android", "arm"), ("android", "aarch64"),
    ("darwin", "x86_64"), ("darwin", "aarch64"),
    ("freebsd", "x86_64"), ("netbsd", "x86_64"),
    ("openbsd", "x86_64"), ("dragonfly", "x86_64"),
    ("solaris", "x86_64"), ("illumos", "x86_64"),
    ("haiku", "x86_64"), ("dos", "x86"), ("xbox", "x86"),
    ("riscos", "arm"), ("nuttx", "riscv32"), ("wii", "powerpc"),
    ("amiga", "m68k"), ("aros", "x86_64"), ("js", "x86"),
)


def query(root: Path, make: str, work: Path, target: str, arch: str,
          switches: tuple[str, ...] = ()) -> dict[str, list[Path]]:
    makefile = work / "GNUmakefile"
    variables = ("GFX_SRC", "GFX3_SRC", "GFX2_GENERIC_DIRS", "GFX3_GENERIC_DIRS",
                 "LIBFBGFX_H", "LIBFBGFX3_H")
    makefile.write_text(
        "rootdir := " + root.as_posix() + "\n"
        "mkpath := " + (root / "mk").as_posix() + "\n"
        "srcdir := " + (root / "src").as_posix() + "\n"
        "TARGET_OS := " + target + "\nTARGET_ARCH := " + arch + "\n"
        "ARM_VER := v7\n"
        "include " + (root / "mk/source-graph.mk").as_posix() + "\n"
        ".PHONY: report\nreport:\n" +
        "".join(f"\t$(info {name}=$({name}))\n" for name in variables) + "\t@:\n",
        encoding="utf-8",
    )
    # An explicit target prevents included object dependency rules from
    # accidentally selecting a build during this read-only query.
    result = subprocess.run(
        [make, "--no-print-directory", "-s", "-f", str(makefile), "report", *switches],
        cwd=root, capture_output=True, text=True, timeout=30,
    )
    if result.returncode:
        raise ValueError(result.stderr.strip() or "Make source graph query failed")
    rows = dict(line.split("=", 1) for line in result.stdout.splitlines()
                if any(line.startswith(name + "=") for name in variables))
    if set(rows) != set(variables):
        raise ValueError("Make did not return the complete graphics source graph")
    return {name: [Path(value).resolve() for value in values.split()]
            for name, values in rows.items()}


def check_sources(root: Path, rows: dict[str, list[Path]], ownership: bool = False) -> None:
    for library, sources, groups, headers in (
        ("gfxlib2", "GFX_SRC", "GFX2_GENERIC_DIRS", "LIBFBGFX_H"),
        ("gfxlib3", "GFX3_SRC", "GFX3_GENERIC_DIRS", "LIBFBGFX3_H"),
    ):
        selected = rows[sources]
        if len({path.stem for path in selected}) != len(selected):
            raise ValueError("Ambiguous archive members in " + library)
        for source in selected:
            if not source.is_file():
                raise ValueError("Selected source does not exist: " + str(source))
        base = root / "src" / library
        if list(base.glob("*.c")):
            raise ValueError(library + " shared implementations must have a subsystem owner")
        for directory in rows[groups]:
            if not directory.is_dir():
                raise ValueError("Missing subsystem: " + str(directory))
            owned_sources = directory.glob("*.c") if ownership else ()
            for source in owned_sources:
                text = source.read_text(encoding="utf-8-sig")
                if not text.startswith("/*") or source.name not in text[:1600]:
                    raise ValueError("Missing source ownership header: " + str(source))
                if not text.rstrip().endswith("/* end of " + source.name + " */"):
                    raise ValueError("Missing source footer: " + str(source))
        # Dependency lists must cover private headers and textual fragments,
        # even on a clean build which has not yet emitted compiler .d files.
        required_headers = set(base.glob("*.h"))
        required_headers.update((base / "drivers").rglob("*.h"))
        required_headers.update((base / "drivers").rglob("*.inc"))
        for directory in rows[groups]:
            required_headers.update(directory.glob("*.h"))
            required_headers.update(directory.glob("*.inc"))
        missing = required_headers - set(rows[headers])
        if missing:
            raise ValueError("Untracked graphics dependencies: " + str(sorted(missing)))


def check_includes(root: Path) -> None:
    for library in ("gfxlib2", "gfxlib3"):
        base = root / "src" / library
        for source in base.rglob("*"):
            if "obj" in source.relative_to(base).parts or source.suffix not in (".c", ".h", ".cpp", ".s", ".inc"):
                continue
            for name in re.findall(r'^\s*#\s*include\s*"([^"\n]+)"',
                                   source.read_text(encoding="utf-8-sig"), re.M):
                if Path(name).name in ("gfx3_opengl_private.h", "gfx3_vulkan_private.h"):
                    if source.parent != base / "backend":
                        raise ValueError("Backend-private types escaped their owner: " + str(source))
                # RP2350's board adapter includes the NuttX SDK's hardware
                # headers. They are supplied by that SDK, not by this tree.
                if library == "gfxlib2" and source.name == "gfx_rp2350_dvi.c" and (
                    name.startswith("hardware/") or name in (
                        "rp23xx_dmac.h", "rp23xx_gpio.h", "rp23xx_pio.h"
                    )
                ):
                    continue
                if not (source.parent / name).is_file():
                    raise ValueError("Unresolved quoted include: " + str(source) + ": " + name)


def check_ownership(root: Path, rows: dict[str, list[Path]]) -> None:
    native2 = {target for target, _ in TARGETS} | {"unix", "x86", "x86_64", "arm", "aarch64"}
    native3 = {"android", "darwin", "linux", "win32"}
    for library, groups, native in (("gfxlib2", "GFX2_GENERIC_DIRS", native2),
                                    ("gfxlib3", "GFX3_GENERIC_DIRS", native3)):
        base = root / "src" / library
        owned = set(rows[groups])
        if not (base / "drivers").is_dir():
            raise ValueError("Missing native driver owner: " + str(base))
        for name in native:
            if (base / name).exists():
                raise ValueError("Native adapter must live below drivers/: " + str(base / name))
        for source in base.rglob("*"):
            if not source.is_file() or source.suffix not in (".c", ".cpp", ".s"):
                continue
            relative = source.relative_to(base)
            if relative.parts[0] == "obj":
                continue
            driver = len(relative.parts) >= 3 and relative.parts[0] == "drivers" and relative.parts[1] in native
            if source.parent not in owned and not driver:
                raise ValueError("Source has no selected subsystem owner: " + str(source))


def validate(root: Path, make: str) -> int:
    count = 0
    with tempfile.TemporaryDirectory(prefix="fbc-gfx-source-graph-") as temporary:
        work = Path(temporary).resolve()
        for target, arch in TARGETS:
            rows = query(root, make, work, target, arch)
            check_sources(root, rows, ownership=(count == 0))
            if count == 0:
                check_ownership(root, rows)
            if target == "riscos":
                for name in ("draw/gfx_window.c", "drivers/riscos/gfx_wimp.c"):
                    if rows["GFX_SRC"].count((root / "src/gfxlib2" / name).resolve()) != 1:
                        raise ValueError("RISC OS must retain shared WINDOW and native Wimp")
            if target == "wince":
                # A missing native directory still leaves a valid generic
                # graph, but it cannot link without the driver registration
                # and Windows CE's screen implementation.
                for name in ("gfx_display.c", "gfx_driver.c", "gfx_input.c",
                             "gfx_joystick.c", "gfx_screen.c", "gfx_xpad.c"):
                    source = (root / "src/gfxlib2/drivers/wince" / name).resolve()
                    if rows["GFX_SRC"].count(source) != 1:
                        raise ValueError("Windows CE must retain its native backend: " + name)
            count += 1
        for switches in (("DISABLE_X11=1",), ("DISABLE_OPENGL=1",),
                         ("DISABLE_X11=1", "DISABLE_OPENGL=1")):
            check_sources(root, query(root, make, work, "linux", "x86_64", switches))
            count += 1
    check_includes(root)
    print(f"GFX_STRUCTURE_PASS graphs={count}: ownership, archive names, includes and dependencies")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--make", default="make")
    options = parser.parse_args()
    try:
        return validate(options.root.resolve(strict=True), options.make)
    except (OSError, ValueError, subprocess.TimeoutExpired) as error:
        print(str(error))
        return 1


if __name__ == "__main__":
    raise SystemExit(main())

# end of check-gfx-source-graph.py
