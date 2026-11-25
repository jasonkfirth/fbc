#!/usr/bin/env python3
"""FreeBASIC browser corpus. Prepare the L-BASIC frontend and LLVM emitter.

fb-corpus-lbasic-frontend.py merges pinned upstream sources using their macro
preprocessor, then adapts the QB64 host calls and syntax for fbc-js. It uses
the parser tables produced by the browser generator checks. The L-BASIC
parser, AST and emitters still own compilation. The LLVM libraries and the
browser link step are handled by fb-corpus-lbasic-llvm.py.
"""

import argparse
from hashlib import sha256
from io import BytesIO
import json
from pathlib import Path
import re
import runpy
import subprocess
import sys
import tarfile
from tempfile import TemporaryDirectory


REVISION = "db13d92c05d2521cbd3673d7629bba755dbdaf51"
NATIVE_INCLUDES = {"emitters/llvm/llvm.bi", "emitters/llvm/llvm.bm", "spawn.bm"}
GENERATOR_TABLES = {
    "lbasic-state-generator": ("ts.rules", {"ts_data.bi", "ts_data.bm"}),
    "lbasic-token-generator": ("tokens.list", {"token_data.bi", "token_registrations.bm"}),
}
INCLUDE = re.compile(r"^\s*\$include\s*:\s*'([^']+)'\s*$", re.IGNORECASE)
PRESERVED_TEXT = re.compile(r'("(?:[^"\n]|"")*"|\x27[^\n]*)')


def merge_includes(source, compiler):
    for line in source.read_text(encoding="utf-8").splitlines():
        include = INCLUDE.fullmatch(line)
        if not include:
            yield line
        elif include[1] not in NATIVE_INCLUDES:
            child = (source.parent / include[1]).resolve()
            if not child.is_relative_to(compiler):
                raise ValueError(f"Compiler include escapes the staged tree: {child}")
            yield from merge_includes(child, compiler)


def replace_once(source, original, replacement):
    if source.count(original) != 1:
        raise ValueError(f"Expected one occurrence of {original!r}")
    return source.replace(original, replacement, 1)


def adapt_directives(source):
    source = re.sub(r"(?im)^\$if VERSION < 2\.0 then\n\s*\$error [^\n]+\n\$end if\n", "", source)
    source = re.sub(r"(?im)^(\s*)\$(if|elseif) (.*?) then\s*$", r"\1#\2 \3", source)
    source = re.sub(r"(?im)^(\s*)\$end if\s*$", r"\1#endif", source)
    source = re.sub(r"(?im)^(\s*)\$else\s*$", r"\1#else", source)
    return re.sub(r"(?im)^(\s*)\$let ([a-z0-9_]+) = (.*)$", r"\1#define \2 \3", source)


def adapt_host(source, llvm=False):
    source = re.sub(r"(?im)^\s*(?:\$console|\$screenhide|option _explicitarray|_dest _console|_dest 0)\s*$", "", source)
    # The parser's file-based entry point uses paths in Emscripten's filesystem.
    source = re.sub(r'(?s)if environ\$\("LLVM_ROOT"\).*?(?=target_platform_settings = runtime_platform_settings)',
                    'runtime_platform_settings.id = "WebAssembly"\n'
                    'runtime_platform_settings.posix_paths = TRUE\n'
                    'runtime_platform_settings.rtlib_dir = "/runtime"\n', source, count=1)
    # INPUT_SRC_MEM has no callers in this revision. MEMFS provides ordinary files.
    source = re.sub(r"(?ms)^sub add_input_mem\(.*?^end sub\n", "", source, count=1)
    source = re.sub(r"(?ms)^        case INPUT_SRC_MEM\n.*?(?=^    end select)", "", source, count=1)
    source = re.sub(r"(?m)^            (?:elseif .*? = INPUT_SRC_MEM then|case INPUT_SRC_MEM)\n[^\n]+\n", "", source)
    for obsolete in ("const INPUT_SRC_MEM = 3\n", "    mem as _mem\n", "    cur_pos as _offset\n", "chdir _startdir$\n"):
        source = replace_once(source, obsolete, "")
    source = re.sub(r"(?im)^\s*(?:old_dest = _dest|_dest old_dest|_screenshow)\s*$", "", source)
    source = re.sub(r"(?im)^(?:[ \t]*resume interactive_recovery|[ \t]*if .* then resume interactive_recovery)\n", "", source)
    source = replace_once(source, "on error goto error_handler\n", "")
    source = re.sub(r"(?ms)^interactive_recovery:.*?(?=^sub fatalerror)", "", source, count=1)
    source = replace_once(source, 'logging_file_handle = freefile\ndump_file_handle = logging_file_handle\n'
                          'open_file "SCRN:", logging_file_handle, TRUE',
                          "logging_file_handle = 0\ndump_file_handle = 0")
    # FreeBASIC can return file-open errors directly without a QB64 resume label.
    source = re.sub(r"(?ms)^sub open_file\(.*?^end sub", '''sub open_file(filename$, handle, is_output)
    dim as long result
    if is_output then
        result = open(filename$ for output as #handle)
    else
        result = open(filename$ for input as #handle)
    end if
    if result <> 0 then fatalerror "Could not open file " + filename$
end sub''', source, count=1)
    source = replace_once(source, "        close #1\n        exit sub", "        close #dump_file_handle\n        exit sub")
    source = replace_once(source, "    Error_message$ = msg$\n    error 101",
                          '    fatalerror "Parser: Line" + str$(ps_actual_linenum) + ": " + msg$')
    source = re.sub(r'(Error_message\$ = "Node"[^\n]+): error 101', r"\1: fatalerror Error_message$", source)
    if not llvm:
        source = replace_once(source, "    ll_build\n", '    fatalerror "LLVM backend is unavailable on this host"\n')
    source = source.replace("_startdir$", "curdir")
    source = source.replace("_commandcount", "argument_count")
    return replace_once(source, "    for i = 1 TO argument_count", """    dim as long argument_count = 0
    do while len(command$(argument_count + 1)) <> 0
        argument_count += 1
    loop
    for i = 1 to argument_count""")


def adapt_language(source):
    source = re.sub(r"(?i)\bas _offset\b", "as uinteger", source)
    source = re.sub(r"(?i)\bDIM SHARED t_states~%(\([^\n]+\))", r"DIM SHARED t_states\1 as ushort", source)
    source = source.replace("t_states~%", "t_states")
    source = re.sub(r"(?i)\bredim _preserve\b", "redim preserve", source)
    source = re.sub(r"(?m)^function symtab_hash~&\(k\$, max\)$", "function symtab_hash(k$, max) as ulong", source)
    source = source.replace("symtab_hash~&", "symtab_hash")
    source = source.replace("    hash~& = 5381", "    dim as ulong hash = 5381").replace("hash~&", "hash")
    source = re.sub(r"(?m)^(\s*)h~& = symtab_hash\(", r"\1dim as ulong h = symtab_hash(", source).replace("h~&", "h")
    source = source.replace('_instrrev(l - 1, ast_constants(expected_type), "|")',
                            'instrrev(ast_constants(expected_type), "|", l - 1)')
    source = re.sub(r"(?i)_instrrev\(", "instrrev(", source).replace("_trim$", "trim$")
    source = re.sub(r"(?mi)^(\s*case )(?=>)", r"\1is ", source)
    source = re.sub(r"(?ms)(^sub type_sig_merge\([^\n]+\).*?)^end function", r"\1end sub", source, count=1)
    source = source.replace("<= SYMTAB_MAX_LOADING then exit function", "<= SYMTAB_MAX_LOADING then exit sub")
    fragments = PRESERVED_TEXT.split(source)
    for index in range(0, len(fragments), 2):
        code = re.sub(r"(?i)\bvar\b", "variable_id", fragments[index])
        code = re.sub(r"(?i)\bcast\b", "cast_node", code)
        fragments[index] = re.sub(r"(?i)(?<!redim )\bpreserve\b", "keep_elements", code)
    source = "".join(fragments)
    source = re.sub(r"(?ms)(^sub tok_read_next_token\b.*?)^end function",
                    lambda match: match[1].replace("exit function", "exit sub") + "end sub", source, count=1)
    source = source.replace("if c = 13 then _continue", "if c = 13 then continue for")
    source = re.sub(r"(?ms)^function tok_next_ts\$.*?^end function",
                    lambda match: re.sub(r"\bcommand\b", "transition", match[0]), source, count=1)
    source = source.replace("symtab(i).typ,symtab(i).v1", "symtab(i).typ; symtab(i).v1")

    # The upstream tokenizer includes the generated DATA inside a procedure.
    # FreeBASIC requires module-level DATA and RESTORE can still use its label.
    table = re.search(r"(?im)^\s*tokeng_data:\n(?:DATA [^\n]+\n)+", source)
    if table is None:
        raise ValueError("Missing generated tokenizer DATA")
    source = source[:table.start()] + "\n" + source[table.end():] + "\n" + table[0].strip() + "\n"
    prototypes = ["declare " + item.strip() for item in re.findall(r"(?im)^(?:sub|function) [^\n]+", source)]
    source = replace_once(source, "\nparse_cmd_line_args\n", "\n" + "\n".join(prototypes) + "\n\nparse_cmd_line_args\n")
    return """#define _fileexists(file) (len(dir(file)) <> 0)
#define _strcmp(lhs, rhs) (((lhs) < (rhs)) - ((lhs) > (rhs)))
#define _shr(value, bits) ((value) shr (bits))
#define _shl(value, bits) ((value) shl (bits))
""" + source


def copy_tables(work, compiler):
    reports = {row["slug"]: row for row in
               json.loads((work / "generator-browser-results.json").read_text(encoding="utf-8"))}
    hashes = {}
    for slug, (rules, tables) in GENERATOR_TABLES.items():
        row = reports.get(slug, {})
        if row.get("status") != "passed" or set(row["outputs"]) != tables:
            raise ValueError(f"Missing verified browser generator outputs for {slug}")
        if sha256((compiler / "parser" / rules).read_bytes()).hexdigest() != row.get("rulesSha256"):
            raise ValueError(f"The checked generator used a different revision of {rules}")
        hashes.update(row["outputs"])
    for name, digest in hashes.items():
        data = (work / "logs" / ("browser-" + name)).read_bytes()
        if sha256(data).hexdigest() != digest:
            raise ValueError(f"Generated table changed after the browser check: {name}")
        (compiler / "parser" / name).write_bytes(data)


def build_foundation(root, toolchain):
    driver = runpy.run_path(str(Path(__file__).with_name("fb-corpus-js.py")))
    env = driver["configure_tools"](toolchain)
    foundation = root / "runtime/foundation"
    archive = root / "runtime/foundation.a"
    with TemporaryDirectory(prefix="foundation-", dir=root) as work:
        objects = []
        for c_file in sorted(foundation.glob("*.c")):
            obj = Path(work) / (c_file.stem + ".o")
            subprocess.run([env["EMCC"], "-std=c17", "-c", str(c_file), "-o", str(obj)], env=env, check=True)
            objects.append(str(obj))
        subprocess.run([env["EMAR"], "rcs", str(archive), *objects], env=env, check=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("repository", type=Path)
    parser.add_argument("generator_work", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--toolchain", type=Path, required=True)
    parser.add_argument("--llvm", action="store_true", help="stage the AST-to-LLVM bitcode emitter")
    args = parser.parse_args()
    args.output.mkdir(parents=True)
    root = args.output.resolve() / "L-BASIC"
    root.mkdir()
    snapshot = subprocess.check_output(["git", "-C", str(args.repository), "archive", "--format=tar", REVISION,
                                        "compiler", "tools/prep.py", "runtime/foundation", "tests", "LICENSE"])
    with tarfile.open(fileobj=BytesIO(snapshot)) as archive:
        archive.extractall(root, filter="data")
    compiler = root / "compiler"
    copy_tables(args.generator_work, compiler)
    merged = root / "merged.bas"
    if args.llvm:
        emitter = runpy.run_path(str(Path(__file__).with_name("fb-corpus-lbasic-llvm-source.py")))
        lines = emitter["merge"](compiler / "lbasic.bas", compiler, INCLUDE, PRESERVED_TEXT)
    else:
        lines = merge_includes(compiler / "lbasic.bas", compiler)
    merged.write_text("\n".join(lines) + "\n", encoding="utf-8")
    prepared = root / "prepared.bas"
    subprocess.run([sys.executable, str(root / "tools/prep.py"), "-D", '@LLVM_INSTALL@|"system"',
                    "-D", f'@VERSION@|"{REVISION[:9]}"', str(merged), str(prepared)], check=True)
    source = adapt_language(adapt_host(adapt_directives(prepared.read_text(encoding="utf-8")), llvm=args.llvm))
    if args.llvm:
        source = replace_once(source, "\nparse_cmd_line_args\n", '''
parse_cmd_line_args
if options.build_stages <> BUILD_PARSE and options.build_stages <> (BUILD_PARSE or BUILD_IR) then
    fatalerror "Choose -e parse or -e ir"
end if
''')
    entry = "lbasic-llvm.bas" if args.llvm else "lbasic-browser.bas"
    (root / entry).write_text(source, encoding="utf-8")
    build_foundation(root, args.toolchain)
    origin = "https://github.com/flukiluke/L-BASIC"
    stage = "LLVM bitcode emitter" if args.llvm else "parse-and-dump stage"
    (root / "SOURCE.txt").write_text(f"Original L-BASIC frontend {origin}/tree/{REVISION}\n"
                                    f"Browser build of the compiler's {stage}.\n", encoding="utf-8")
    if args.llvm:
        return
    case = {
        "slug": "lbasic-frontend",
        "project": root.name,
        "entry": "lbasic-browser.bas",
        "source_origin": origin,
        "files": ["runtime/foundation/*.c", "runtime/foundation/*.h", "tests/for.test", "tests/udt.test"],
        "assets": {"runtime/foundation.a": "/runtime/foundation.a"},
        "notices": ["SOURCE.txt"],
        "flags": ["-lang", "fblite", "-Wl", "-sASYNCIFY=0", "-Wl", "-sEXPORTED_RUNTIME_METHODS=FS",
                  "-Wl", "-sSTACK_SIZE=2MB"],
    }
    (args.output / "lbasic-frontend.json").write_text(json.dumps([case], indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()

# end of fb-corpus-lbasic-frontend.py
