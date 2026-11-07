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
import runpy
import shutil
import subprocess
import tempfile


def source_graph(root: Path, host: str, make: str = "make") -> dict[str, list[str]]:
    """Use the actual make fragment so a second discovery implementation cannot drift."""
    with tempfile.TemporaryDirectory(prefix="fbc-source-graph-") as temporary:
        makefile = Path(temporary) / "GNUmakefile"
        makefile.write_text(
            "srcdir := " + str(root / "src") + "\n"
            "TARGET_OS := " + host + "\n"
            "include " + str(root / "mk/compiler-sources.mk") + "\n"
            "all:\n"
            "\t$(info SOURCES=$(FBC_SRC))\n"
            "\t$(info HEADERS=$(sort $(FBC_BI) $(FBC_SEMANTIC_WIRE_BI)))\n"
            "\t$(info GROUPS=$(FBC_SOURCE_GROUPS))\n"
            "\t@:\n",
            encoding="utf-8",
        )
        # Included dependency rules can become make's default target. Always
        # select our reporting recipe instead of building a compiler object.
        command = [make, "--no-print-directory", "-s", "-f", str(makefile), "all"]
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
                prelude = source.read(4096)
        except FileNotFoundError:
            # Another compiler process may remove its temporary C emission.
            continue
        # Deferred startup attributes precede declarations so a complete unit
        # can choose one registration per priority. Accept only that emitted
        # macro form, then require the ordinary scalar prelude and BASIC owner.
        startup = re.match(
            rb"(?:#define fb_c_startup_attr_[cd]_[0-9]{1,5} __attribute__\(\("
            rb"(?:used|(?:constructor|destructor)(?:\([0-9]{1,5}\))?)\)\)\r?\n)+", prelude)
        if startup:
            prelude = prelude[startup.end():]
        # A consistent stdio binding may precede the fixed scalar prelude.
        # Match its narrow emitted form; unrelated C is still not a source input.
        binding = re.match(
            rb"typedef (?:void FILE|(?:struct|union) fb_type_[A-Za-z0-9_]+ FILE "
            rb"__attribute__\(\(aligned\(1\)\)\));\r?\n", prelude)
        if binding:
            prelude = prelude[binding.end():]
        # Native Windows text output uses CRLF; Unix hosts use LF.
        generated = path.with_suffix(".bas").is_file() and prelude.startswith((
            b"typedef   signed char       int8;\n",
            b"typedef   signed char       int8;\r\n",
        ))
        if not generated:
            sources.append(path)
    return sources


def validate(root: Path, make: str = "make") -> tuple[int, int]:
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
                      "This file intentionally does NOT"):
            if field not in header:
                failures.append(relative + ": header missing " + field)
        # An exclusion list and a full sentence express the same ownership rule.
        # Keep the existing list form, and require a verb/object for sentences.
        if not re.search(r"This file intentionally does NOT (?:contain:|[a-z]+ [A-Za-z][^\n]+)", header):
            failures.append(relative + ": header missing an exclusion list or sentence")
        # Both native FreeBASIC comment forms can identify the owning file.
        footers = ("'' end of " + relative, "/' end of " + relative + " '/")
        if not text.rstrip().endswith(footers):
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
        current = source_graph(root, host, make)
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
            scalar = b"typedef   signed char       int8;" + newline
            for binding in (b"", b"typedef void FILE;" + newline,
                            b"typedef struct fb_type_4FILE FILE __attribute__((aligned(1)));" + newline,
                            b"typedef union fb_type_4FILE FILE __attribute__((aligned(1)));" + newline):
                helper.write_bytes(binding + scalar)
                if handwritten_c_sources(fixture):
                    failures.append("Structure check rejected generated C with its BASIC source")
                for attribute in (b"used", b"constructor(101)", b"destructor"):
                    prefix = b"#define fb_c_startup_attr_c_101 __attribute__((" + attribute + b"))" + newline
                    helper.write_bytes(prefix + binding + scalar)
                    if handwritten_c_sources(fixture):
                        failures.append("Structure check rejected deferred startup attributes")
                    helper.write_bytes(prefix + binding + b"int helper(void) { return 0; }")
                    if handwritten_c_sources(fixture) != [helper]:
                        failures.append("Structure check accepted unrelated C after startup attributes")
            helper.write_bytes(b"typedef void FILE;" + newline + b"int helper(void) { return 0; }")
            if handwritten_c_sources(fixture) != [helper]:
                failures.append("Structure check accepted unrelated C after a stdio typedef")
        helper.write_bytes(b"typedef void FILE;\ntypedef   signed char       int8;\n")
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
            source_graph(fixture, "linux", make)
        except ValueError as error:
            if "Duplicate compiler source basenames" not in str(error):
                failures.append("Duplicate graph rejected for an unrelated reason: " + str(error))
        else:
            failures.append("Make accepted duplicate compiler source basenames")

    if failures:
        raise ValueError("\n".join(failures))
    # Each opcode keeps its real callback signature through storage and dispatch.
    # Run the binding guard here so the ordinary structure check owns this rule.
    callbacks = runpy.run_path(str(root / "build_scripts/check-x86-emitter-callbacks.py"))
    callback_source = compiler / "backend/x86"
    try:
        callbacks["check"](callback_source)
        callbacks["self_test"](callback_source)
    except RuntimeError as error:
        raise ValueError("X86 callback structure: " + str(error)) from error
    return len(files), len(hosts)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--make", default=shutil.which("gmake") or shutil.which("make") or "make")
    options = parser.parse_args()
    try:
        files, hosts = validate(options.root.resolve(), options.make)
    except (OSError, ValueError) as error:
        print(str(error))
        return 1
    print(f"Compiler structure: {files} documented files; {hosts} host source graphs checked")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

# end of check-compiler-structure.py
