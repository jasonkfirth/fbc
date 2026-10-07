#!/usr/bin/env python3
"""FreeBASIC compiler build validation.

File: check-compiler-structure.py
Purpose: Check compiler source coverage, documentation, and host policy selection.
Responsibilities: Inspect source contracts and exercise the canonical make graph.
This file intentionally does NOT compile or execute BASIC programs.
"""

from __future__ import annotations

import argparse
from pathlib import Path
import re
import subprocess
import tempfile


def source_graph(root: Path, host: str) -> dict[str, list[str]]:
    """Use the actual make fragment so a second discovery implementation cannot drift."""
    with tempfile.TemporaryDirectory(prefix="fbc-source-graph-") as temporary:
        makefile = Path(temporary) / "GNUmakefile"
        makefile.write_text(
            "srcdir := " + str(root / "src") + "\n"
            "TARGET_OS := " + host + "\n"
            "include " + str(root / "mk/compiler-sources.mk") + "\n"
            "all:\n"
            "\t$(info SOURCES=$(FBC_SRC))\n"
            "\t$(info HEADERS=$(FBC_BI))\n"
            "\t$(info GROUPS=$(FBC_SOURCE_GROUPS))\n"
            "\t@:\n",
            encoding="utf-8",
        )
        command = ["make", "--no-print-directory", "-s", "-f", str(makefile)]
        result = subprocess.run(command, text=True, capture_output=True, check=False)
        if result.returncode:
            raise ValueError(result.stderr.strip())
        return {
            # GNU make can return mixed separators on a native Windows host.
            # Compare path identities in the same form as compiler_files().
            key: [str(Path(item)) for item in value.split()] if key in ("SOURCES", "HEADERS") else value.split()
            for key, value in (line.split("=", 1) for line in result.stdout.splitlines() if "=" in line)
        }


def compiler_files(root: Path) -> list[Path]:
    compiler = root / "src/compiler"
    return sorted(
        path for path in compiler.rglob("*")
        if path.is_file() and "obj" not in path.relative_to(compiler).parts
        and path.suffix in (".bas", ".bi")
    )


def handwritten_c_sources(compiler: Path) -> list[Path]:
    """Inspect working files too, so new helpers cannot bypass the language rule."""
    sources: list[Path] = []
    for path in sorted(compiler.rglob("*.c")):
        if "obj" in path.relative_to(compiler).parts:
            continue
        try:
            with path.open("rb") as source:
                prelude = source.read(256)
        except FileNotFoundError:
            # Another compiler process may remove its temporary C emission.
            continue
        # Native Windows text output uses CRLF; Unix hosts use LF. Both
        # spellings must retain the exact emitter prelude and owning module.
        generated = path.with_suffix(".bas").is_file() and prelude.startswith((
            b"typedef   signed char       int8;\n",
            b"typedef   signed char       int8;\r\n",
        ))
        if not generated:
            sources.append(path)
    return sources


def validate(root: Path) -> tuple[int, int]:
    compiler = root / "src/compiler"
    files = compiler_files(root)
    if not files:
        raise ValueError("No compiler source files found")
    failures: list[str] = []
    # The C backend generates .c files alongside BASIC modules during -r.
    # Recognize the emitter's fixed type prelude and owning BASIC module.
    # Source policy must not depend on whether a file has been added to Git.
    for path in handwritten_c_sources(compiler):
        failures.append(str(path.relative_to(root)) + ": handwritten compiler source must be BASIC")
    runtime_headers: set[Path] = set()
    for path in files:
        relative = path.relative_to(compiler).as_posix()
        text = path.read_text(encoding="utf-8")
        header = text.split("\n\n", 1)[0]
        for field in ("Project:", "File: " + relative, "Purpose:", "Responsibilities:",
                      "This file intentionally does NOT contain:"):
            if field not in header:
                failures.append(relative + ": header missing " + field)
        if not text.rstrip().endswith("'' end of " + relative):
            failures.append(relative + ": missing identifying footer")
        if path.parent == compiler:
            failures.append(relative + ": source has no owning subsystem")
        for include in re.findall(r'(?im)^\s*#\s*include\s+(?:once\s+)?"([^"\n]+)"', text):
            if include == "driver/fbc-private.bi" and path.parent != compiler / "driver":
                failures.append(relative + ": includes driver implementation state")
            if include == "tooling/semantic-private.bi" and path.parent != compiler / "tooling":
                failures.append(relative + ": includes semantic exporter implementation helpers")
            if include == "driver/fbc-platform.bi" and relative != "driver/fbc-platform.bas":
                failures.append(relative + ": target hook bodies have more than one owner")
            candidates = (compiler / include, path.parent / include, root / "inc" / include)
            if not any(candidate.is_file() for candidate in candidates):
                failures.append(relative + ": unresolved include " + include)
            elif (root / "inc" / include).is_file() and not (compiler / include).is_file():
                runtime_headers.add((root / "inc" / include).resolve())
            elif (path.parent / include).is_file() and not (compiler / include).is_file():
                # External library includes belong to inc/. Internal includes
                # should make their subsystem dependency explicit.
                if (path.parent / include).resolve().is_relative_to(compiler.resolve()):
                    failures.append(relative + ": internal include is not rooted at src/compiler: " + include)

    # Compiler objects also depend on public runtime declarations. Follow
    # their includes so the graph cannot retain stale C aliases after an ABI
    # header changes. This only inspects quoted includes, as above.
    pending = list(runtime_headers)
    while pending:
        header = pending.pop()
        for include in re.findall(r'(?im)^\s*#\s*include\s+(?:once\s+)?"([^"\n]+)"',
                                  header.read_text(encoding="utf-8")):
            candidates = (header.parent / include, root / "inc" / include)
            target = next((path.resolve() for path in candidates if path.is_file()), None)
            if target is not None and target not in runtime_headers:
                runtime_headers.add(target)
                pending.append(target)

    hosts = {"linux", "dos", "riscos"}
    hosts.update(path.name for path in (compiler / "driver/platforms").iterdir() if path.is_dir())
    hosts.update(path.name for path in (compiler / "platform").iterdir() if path.is_dir())
    selected_sources: set[str] = set()
    selected_headers: set[str] = set()
    for host in sorted(hosts):
        current = source_graph(root, host)
        sources = current["SOURCES"]
        selected_sources.update(sources)
        selected_headers.update(current["HEADERS"])
        missing_runtime_headers = runtime_headers - {Path(path).resolve() for path in current["HEADERS"]}
        for path in sorted(missing_runtime_headers):
            failures.append(host + ": omitted compiler runtime include: " + str(path.relative_to(root)))
        names = [Path(path).name for path in sources]
        if len(names) != len(set(names)):
            failures.append(host + ": selected duplicate source basenames")
        for path in (compiler / "platform" / host).glob("*.bas"):
            matching = [source for source in sources if Path(source).name == path.name]
            if matching != [str(path)]:
                failures.append(host + ": host replacement not selected: " + path.name)
        for source in sources:
            source_path = Path(source)
            if "platform" in source_path.relative_to(compiler).parts and source_path.parent != compiler / "platform" / host:
                failures.append(host + ": selected policy from another host: " + source)
    for path in files:
        selected = selected_sources if path.suffix == ".bas" else selected_headers
        if str(path) not in selected:
            failures.append(path.relative_to(compiler).as_posix() + ": omitted from the make source graph")

    # A newly created C helper must fail even beside a BASIC module. Generated
    # output is allowed only with its owning source; an orphan is not an input.
    with tempfile.TemporaryDirectory(prefix="fbc-source-language-") as temporary:
        fixture = Path(temporary)
        (fixture / "module.bas").write_text("end\n", encoding="utf-8")
        helper = fixture / "module.c"
        helper.write_text("int helper(void) { return 0; }\n", encoding="utf-8")
        if handwritten_c_sources(fixture) != [helper]:
            failures.append("Structure check accepted a new handwritten C compiler helper")
        for newline in (b"\n", b"\r\n"):
            helper.write_bytes(b"typedef   signed char       int8;" + newline)
            if handwritten_c_sources(fixture):
                failures.append("Structure check rejected generated C with its BASIC source")
        (fixture / "module.bas").unlink()
        if handwritten_c_sources(fixture) != [helper]:
            failures.append("Structure check accepted C without its BASIC source")

    # A duplicate in two ordinary groups must stop make before VPATH can pick
    # one arbitrarily. The fixture has distinct owning directories.
    with tempfile.TemporaryDirectory(prefix="fbc-duplicate-graph-") as temporary:
        fixture = Path(temporary)
        (fixture / "src/compiler/core").mkdir(parents=True)
        (fixture / "src/compiler/support").mkdir()
        (fixture / "mk").mkdir()
        (fixture / "mk/compiler-sources.mk").write_bytes((root / "mk/compiler-sources.mk").read_bytes())
        for group in ("core", "support"):
            (fixture / "src/compiler" / group / "collision.bas").write_text("end\n", encoding="utf-8")
        try:
            source_graph(fixture, "linux")
        except ValueError as error:
            if "Duplicate compiler source basenames" not in str(error):
                failures.append("Duplicate graph rejected for an unrelated reason: " + str(error))
        else:
            failures.append("Make accepted duplicate compiler source basenames")

    if failures:
        raise ValueError("\n".join(failures))
    return len(files), len(hosts)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    options = parser.parse_args()
    try:
        files, hosts = validate(options.root.resolve())
    except (OSError, ValueError) as error:
        print(str(error))
        return 1
    print(f"Compiler structure: {files} documented files; {hosts} host source graphs checked")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

# end of check-compiler-structure.py
