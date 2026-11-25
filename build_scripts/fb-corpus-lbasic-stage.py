#!/usr/bin/env python3
"""FreeBASIC browser corpus. Stage the L-BASIC parser table generators.

fb-corpus-lbasic-stage.py archives the tracked build tools and their rule
files, then records the QB64 host adaptations in a reproducible fbc-js
profile. The original generators still parse the rules and write the tables.
The L-BASIC compiler and LLVM backend have their own build stages.
"""

import argparse
from io import BytesIO
import json
from pathlib import Path
import subprocess
import tarfile


ORIGIN = "https://github.com/flukiluke/L-BASIC"
GENERATORS = {
    "tsgen": ("lbasic-state-generator", "compiler/parser/ts.rules"),
    "tokgen": ("lbasic-token-generator", "compiler/parser/tokens.list"),
}


def common_edits():
    # File opens return a status in fblite. Check it at the point of failure,
    # since the QB64 ON ERROR dispatch is not needed for this command.
    edits = [
        ("$console:only\n_dest _console\non error goto ehandler\n", ""),
        ("chdir _startdir$\n", ""),
        ("if _commandcount <> 3 then", 'if command$(3) = "" or command$(4) <> "" then'),
        ("if not _fileexists(command$(1)) then", "if len(dir(command$(1))) = 0 then"),
    ]
    for handle, mode in ((1, "input"), (2, "output"), (3, "output")):
        file_arg = f"command$({handle})"
        statement = f"{file_arg} for {mode} as #{handle}"
        edits.append((f"open {statement}", f"if open({statement}) <> 0 then\n"
                      f'    print "Cannot open "; {file_arg}\n    end 1\nend if'))
    return edits


def state_edits():
    return [
        ("redim shared classnames$(0)", """option gosub
declare sub add_class(cname$, cvalue$)
declare function get_state_id(state$)
declare function get_token_id(token$)
declare function expand_escapes$(in$)
declare function resolve_charref$(raw$)

redim shared classnames$(0)"""),
        ("ehandler:\n    print err; _errorline\n    system 1\n", ""),
        ("    dir$ =", "    directive_name$ ="),
        ("select case dir$", "select case directive_name$"),
        ('"Unknown directive " + dir$', '"Unknown directive " + directive_name$'),
        ("    state_id%% =", "    dim as long state_id ="),
        ("    new_state_id%% =", "    dim as long new_state_id ="),
        ("    token_id%% = 0", "    dim as long token_id = 0"),
        ("    v~% = (", "    dim as ushort v = ("),
        ("state_id%%", "state_id", 4),
        ("token_id%%", "token_id", 3),
        ("v~%", "v", 3),
        # QB64 stores the first subscript contiguously. With FreeBASIC's
        # layout, grow the first dimension so REDIM PRESERVE keeps complete
        # rows of 128 character transitions. Keep the signed 16-bit DATA
        # representation used by the original generator.
        ("redim shared states%(127, 1)", "redim shared states(1, 127) as short"),
        ("redim _preserve states%(127, ubound(states%, 2) + 1)",
         "redim _preserve states(ubound(states, 1) + 1, 127)"),
        ("ubound(states%, 2)", "ubound(states, 1)", 2),
        ("states%(j&, i&)", "states(i&, j&)"),
        ("states%(127, i&)", "states(i&, 127)"),
        ("states%(asc(chars$, i&), state_id)", "states(state_id, asc(chars$, i&))"),
        ("redim _preserve", "redim preserve", 5),
        ("\nsystem\n", "\nend\n"),
        ("    system 1\n", "    end 1\n", 3),
    ]


def token_edits():
    return [
        ("const FALSE = 0, TRUE = not FALSE\n", """declare sub process_return_type(prev_name$, cur_name$, ret_type$)
declare sub process_arg_list(arglist$)
declare sub split(in$)
declare sub split_args(in$)
declare function quote$(a$)
declare sub assertsize_range(min_expected, max_expected)
declare sub assertsize(expected)
declare sub fatalerror(msg$)
"""),
        ('ehandler:\n    print "Error"; err; _errorline\n    system 1\n', ""),
        ("_continue", "continue do", 3),
        ("redim _preserve", "redim preserve", 3),
        ("\nsystem\n", "\nend\n"),
        ("    system 1\n", "    end 1\n", 3),
    ]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("repository", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    git = ["git", "-C", str(args.repository)]
    revision = subprocess.check_output([*git, "rev-parse", "HEAD"], text=True).strip()
    project = args.output / "L-BASIC"
    project.mkdir(parents=True)
    members = ["LICENSE", "README.md"]
    for tool, (_, rules) in GENERATORS.items():
        members.extend((f"tools/{tool}.bas", rules))
    snapshot = subprocess.check_output([*git, "archive", "--format=tar", revision, *members])
    with tarfile.open(fileobj=BytesIO(snapshot)) as archive:
        archive.extractall(project, filter="data")
    (project / "SOURCE.txt").write_text(f"L-BASIC source {ORIGIN}/tree/{revision}\n", encoding="utf-8")

    cases = []
    for tool, (slug, rules) in GENERATORS.items():
        entry = f"tools/{tool}.bas"
        edits = common_edits() + (state_edits() if tool == "tsgen" else token_edits())
        cases.append({
            "slug": slug,
            "project": project.name,
            "entry": entry,
            "source_origin": ORIGIN,
            "files": [rules],
            "notices": ["SOURCE.txt"],
            "flags": ["-lang", "fblite", "-Wl", "-sASYNCIFY=0", "-Wl", "-sEXPORTED_RUNTIME_METHODS=FS"],
            "replacements": [[entry, *edit] for edit in edits],
        })
    (args.output / "lbasic-generators.json").write_text(json.dumps(cases, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()

# end of fb-corpus-lbasic-stage.py
