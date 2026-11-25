#!/usr/bin/env python3
"""FreeBASIC browser corpus: stage FreeBASIC and FreeBASIC-NG compiler sources.

fb-corpus-compiler-stage.py selects a tracked revision and writes its browser
build profile. It keeps the compiler's regular entry point and translation
units, and supplies host constants for the existing JS code-generation target.
The build driver owns compiler invocation and artifact packaging.
"""

import argparse
from io import BytesIO
import json
from pathlib import Path
import subprocess
import tarfile


COMPILERS = {
    "freebasic-fbc": ("freebasic-compiler", "https://github.com/freebasic/fbc"),
    "freebasic-ng": ("freebasic-ng-compiler", "https://github.com/metaneutrons/freebasic-ng"),
}


def git(project, *arguments):
    return subprocess.check_output(["git", "-C", str(project), *arguments])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("project", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--project-name", choices=COMPILERS, default="freebasic-fbc")
    args = parser.parse_args()
    revision = git(args.project, "rev-parse", "HEAD").decode("ascii").strip()
    slug, origin = COMPILERS[args.project_name]
    project = args.output / args.project_name
    project.mkdir(parents=True)
    tree = git(args.project, "archive", "--format=tar", revision, "src/compiler", "doc/gpl.txt", "readme.txt")
    with tarfile.open(fileobj=BytesIO(tree)) as archive:
        archive.extractall(project, filter="data")
    (project / "SOURCE.txt").write_text(
        f"Compiler source {origin}/tree/{revision}/src/compiler\n"
        "The browser profile adds a JS host to the upstream compiler's existing JS target.\n"
        "It also preserves the fatal allocation diagnostic and exit status on that host.\n",
        encoding="utf-8",
    )
    entry = "src/compiler/fbc.bas"
    units = [source.relative_to(project).as_posix() for source in sorted((project / "src/compiler").glob("*.bas"))
             if source.relative_to(project).as_posix() != entry]
    host = '#elseif defined(__FB_LINUX__)\nconst FB_HOST_EXEEXT'
    cpu = 'const FB_DEFAULT_CPUTYPE32 = FB_DEFAULT_CPUTYPE_X86\n\tconst FB_DEFAULT_CPUTYPE64 = FB_DEFAULT_CPUTYPE_X86_64\n'
    case = {
        "slug": slug,
        "project": project.name,
        "entry": entry,
        "source_origin": origin,
        "units": units,
        "files": ["src/compiler/*.bi", "readme.txt"],
        "notices": ["SOURCE.txt"],
        "flags": ["-m", "fbc", "-d", "ENABLE_STANDALONE", "-d", "FB_NO_SFXLIB",
                  "-Wl", "-sASYNCIFY=0",
                  "-Wl", "-sSTACK_SIZE=4MB", "-Wl", "-sEXPORTED_RUNTIME_METHODS=FS"],
        "timeout": 900,
        "replacements": [
            ["src/compiler/fb.bi", host,
             '#elseif defined(__FB_JS__)\n'
             'const FB_HOST_EXEEXT        = ""\n'
             'const FB_HOST_PATHDIV       = "/"\n'
             'const FB_DEFAULT_TARGET     = FB_COMPTARGET_JS\n' + host],
            ["src/compiler/fb.bi", cpu,
             cpu + '#elseif defined(__FB_JS__)\n'
                   '\tconst FB_DEFAULT_CPUTYPE32 = FB_DEFAULT_CPUTYPE_ASMJS\n'
                   '\tconst FB_DEFAULT_CPUTYPE64 = FB_DEFAULT_CPUTYPE_ASMJS\n'],
            # This compiler has no BASIC error handler. Bare ERROR still emits a
            # computed goto, which Clang rejects in a function with no label addresses.
            ["src/compiler/list.bas", "\t'' Abort right now, the rtlib will show the error\n\terror(4)",
             '\tprint "runtime error 4: Out of memory"\n\tend 4'],
        ],
    }
    (args.output / (slug + ".json")).write_text(json.dumps([case], indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()

# end of fb-corpus-compiler-stage.py
