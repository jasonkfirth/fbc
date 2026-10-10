#!/usr/bin/env python3
"""Project: FreeBASIC SDL3 binding tests
File: test-sdl3-bindings.py
Purpose: Compare translated interfaces with the pinned C headers and libraries.
Responsibilities: Check layouts, constants, cross-target emission, and native calls.
This file intentionally does NOT contain: dependency installation or hardware tests.
"""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import wave


ROOT = Path(__file__).resolve().parents[1]
HEADERS = (
    ("SDL3/SDL.h", "SDL.bi"),
    ("SDL3/SDL_main.h", "SDL_main.bi"),
    ("SDL3/SDL_vulkan.h", "SDL_vulkan.bi"),
    ("SDL3/SDL_test.h", "SDL_test.bi"),
    ("SDL3_image/SDL_image.h", "SDL_image.bi"),
    ("SDL3_mixer/SDL_mixer.h", "SDL_mixer.bi"),
    ("SDL3_ttf/SDL_ttf.h", "SDL_ttf.bi"),
    ("SDL3_ttf/SDL_textengine.h", "SDL_textengine.bi"),
    ("SDL3_net/SDL_net.h", "SDL_net.bi"),
    ("SDL3_sound/SDL_sound.h", "SDL_sound.bi"),
)
PREFIXES = ("SDL_", "SDLTest_", "TTF_", "Sound_", "IMG_", "NET_", "MIX_")


def inline_functions(work: Path, output: Path) -> dict:
    expected = {}
    implementation_details = {"SDL_size_mul_check_overflow_builtin",
                              "SDL_size_add_check_overflow_builtin"}
    for header in (work / "sources").glob("*/include/**/*.h"):
        # Doxygen examples are not implementations. Preserve string literals
        # while removing comments so documentation cannot inflate the count.
        code = re.sub(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|/\*.*?\*/|//[^\n]*',
                      lambda m: "" if m[0].startswith(("/*", "//")) else m[0],
                      header.read_text(), flags=re.S)
        names = {name for name in re.findall(r'^\s*SDL_FORCE_INLINE\b[^;{}]+?\b(\w+)\s*\([^;{}]*\)\s*\{',
                                            code, re.M) if name.startswith(PREFIXES)}
        for name in names - implementation_details:
            expected[name] = header.name
    if not expected:
        raise RuntimeError("Upstream inline function inventory is empty; check the source directory")
    definitions = {}
    for header in (ROOT / "inc/SDL3").glob("*.bi"):
        for match in re.finditer(r'^private (function|sub) (SDL_\w+) (cdecl\([^\n]+)',
                                 header.read_text(), re.M):
            definitions[match[2]] = (header.name, match[1], match[3])
    missing = sorted(expected.keys() - definitions.keys())
    mapping = {name: {"c_header": expected[name], "basic_header": definitions[name][0]}
               for name in sorted(expected) if name in definitions}
    # Taking addresses prevents a value-equivalent macro from concealing a
    # missing C-callable helper. These checks also run on Win32's distinct ABI.
    probe = ["'' Project: SDL3 inline helper tests", "'' File: inline-addresses.bas",
             "'' Purpose: Check C-callable addresses of the translated inline helpers.",
             "'' Responsibilities: Preserve signatures on each target profile.",
             "'' This file intentionally does NOT contain: runtime or SDL implementations.",
             '#include once "SDL3/SDL.bi"']
    for index, name in enumerate(sorted(mapping)):
        _, kind, signature = definitions[name]
        probe += [f"type inline_pointer_{index} as {kind} {signature}",
                  f"dim inline_address_{index} as inline_pointer_{index} = @{name}"]
    probe += ["SDL_CPUPauseInstruction()", "'' end of inline-addresses.bas"]
    (output / "inline-addresses.bas").write_text("\n".join(probe) + "\n")
    return dict(functions=len(expected), missing=missing, implementations=mapping,
                compiler_specific_variants=sorted(implementation_details))


def public_functions(ast: dict) -> dict:
    expected = {node["name"] for node in ast["inner"]
                if node.get("kind") == "FunctionDecl"
                and node.get("name", "").startswith(PREFIXES)
                and node.get("storageClass") != "static"}
    translated = set()
    for header in (ROOT / "inc/SDL3").glob("*.bi"):
        for match in re.finditer(r'^\s*declare (?:sub|function) (\w+)(?:.*?alias "([^"]+)")?\(',
                                 header.read_text(), re.M):
            translated.add(match[2] or match[1])
    return dict(functions=len(expected), missing=sorted(expected - translated))


def invoke(command: list[str], directory: Path, environment: dict[str, str],
           log: Path, timeout: int = 120) -> str:
    result = subprocess.run(command, cwd=directory, env=environment, text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=timeout)
    log.write_text("command: " + repr(command) + "\n" + result.stdout)
    if result.returncode:
        raise RuntimeError("Command failed; see " + str(log))
    return result.stdout


def values(output: str) -> dict[str, int]:
    return {name: int(value.strip()) for line in output.splitlines()
            for name, value in [line.rsplit("|", 1)]}


def layout_sources(ast: dict, output: Path) -> int:
    records = {}
    constants = set()

    def visit(node):
        if node.get("kind") == "EnumConstantDecl" and node.get("name", "").startswith(PREFIXES):
            constants.add(node["name"])
        for child in node.get("inner", []):
            visit(child)

    visit(ast)
    for node in ast["inner"]:
        if (node.get("kind") == "RecordDecl" and node.get("completeDefinition")
                and node.get("name", "").startswith(PREFIXES)):
            records[node["name"]] = [(field["name"], "[" in field["type"]["qualType"])
                                     for field in node.get("inner", [])
                                     if field["kind"] == "FieldDecl" and field.get("name")]
    c = ["/* Project: SDL3 ABI tests. File: probe.c. Purpose: Report C layout values.",
         "   Responsibilities: Measure pinned public declarations.",
         "   This file intentionally does not implement SDL. */",
         "#include <stdio.h>", "#include <stddef.h>", "#include <stdint.h>",
         '#include "headers.c"', "int main(void) {"]
    basic = ["'' Project: SDL3 ABI tests. File: probe.bas. Purpose: Report BASIC layout values.",
             "'' Responsibilities: Measure translated declarations.",
             "'' This file intentionally does not implement SDL."]
    basic += ['#include once "SDL3/' + header + '"' for _, header in HEADERS]
    for index, (name, fields) in enumerate(sorted(records.items())):
        c.append(f'printf("size|{name}|%zu\\n", sizeof({name}));')
        c.append(f'printf("align|{name}|%zu\\n", _Alignof({name}));')
        basic += [f'print "size|{name}|"; sizeof({name})', f"type alignment_{index}",
                  "pad as ubyte", f"value as {name}", "end type",
                  f'print "align|{name}|"; offsetof(alignment_{index}, value)']
        for field, is_array in fields:
            fb_field = ("mod_" if field == "mod" else field) + ("(0)" if is_array else "")
            c.append(f'printf("offset|{name}.{field}|%zu\\n", offsetof({name}, {field}));')
            basic.append(f'print "offset|{name}.{field}|"; offsetof({name}, {fb_field})')
    for name in sorted(constants):
        # Sign extension exposes mistranslated unsigned enums such as the
        # SDL_sound EAGAIN flag. Comparing just the low word would hide them.
        c.append(f'printf("constant|{name}|%llu\\n", (unsigned long long)(uint64_t)({name}));')
        basic.append(f'print "constant|{name}|"; culngint({name})')
    c += ["return 0;", "}", "/* end of probe.c */"]
    basic.append("'' end of probe.bas")
    (output / "probe.c").write_text("\n".join(c) + "\n")
    (output / "probe.bas").write_text("\n".join(basic) + "\n")
    return len(records)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--workdir", type=Path, default=ROOT / "out/sdl3")
    parser.add_argument("--fbc", type=Path, default=ROOT / "bin/fbc")
    parser.add_argument("--cc", default="cc")
    parser.add_argument("--clang", default="clang")
    parser.add_argument("--font", type=Path,
                        default=Path("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"))
    args = parser.parse_args()
    work = args.workdir.resolve()
    prefix = work / "prefix"
    output = work / "validation"
    output.mkdir(parents=True, exist_ok=True)
    environment = os.environ.copy()
    environment["LD_LIBRARY_PATH"] = str(prefix / "lib") + os.pathsep + environment.get("LD_LIBRARY_PATH", "")
    environment["SDL_AUDIODRIVER"] = "dummy"
    report = {}
    try:
        c_headers = ["/* Project: SDL3 ABI tests. File: headers.c.",
                     "   Purpose: Load authoritative declarations. Responsibilities: Include pinned headers.",
                     "   This file intentionally does not implement SDL. */", "#define SDL_MAIN_HANDLED"]
        c_headers += ["#include <" + c + ">" for c, _ in HEADERS]
        c_headers.append("/* end of headers.c */")
        (output / "headers.c").write_text("\n".join(c_headers) + "\n")
        with (output / "headers-ast.json").open("w") as stream:
            subprocess.run([args.clang, "-I", str(prefix / "include"), "-Xclang", "-ast-dump=json",
                            "-fsyntax-only", str(output / "headers.c")], stdout=stream, check=True)
        ast = json.loads((output / "headers-ast.json").read_text())
        report["functions"] = public_functions(ast)
        if report["functions"]["missing"]:
            raise RuntimeError("Public C functions are missing from the BASIC declarations")
        report["inline_functions"] = inline_functions(work, output)
        if report["inline_functions"]["missing"]:
            raise RuntimeError("Public C inline functions are missing BASIC implementations")
        count = layout_sources(ast, output)
        fbc = [str(args.fbc.resolve()), "-i", str(ROOT / "inc"), "-p", str(prefix / "lib")]
        invoke([args.cc, "-I", str(prefix / "include"), str(output / "probe.c"), "-o",
                str(output / "probe-c")], ROOT, environment, output / "compile-c.log")
        invoke(fbc + [str(output / "probe.bas"), "-x", str(output / "probe-fb")], ROOT,
               environment, output / "compile-fb.log")
        expected = values(invoke([str(output / "probe-c")], output, environment, output / "probe-c.log"))
        actual = values(invoke([str(output / "probe-fb")], output, environment, output / "probe-fb.log"))
        differences = [(name, value, actual.get(name)) for name, value in expected.items()
                       if value != actual.get(name)]
        report["layout"] = dict(records=count, checks=len(expected), differences=differences)
        print(f"C ABI: {count} records, {len(expected)} checks, {len(differences)} differences", flush=True)
        if differences:
            raise RuntimeError("C and BASIC ABI probes differ")
        for target in ("linux-x86", "linux-x86_64", "win32", "win64", "linux-arm", "linux-aarch64"):
            invoke(fbc + ["-target", target, "-gen", "gcc", "-r", str(output / "probe.bas"), "-o",
                          str(output / ("headers-" + target + ".c"))], ROOT, environment,
                   output / ("emit-" + target + ".log"))
            invoke(fbc + ["-target", target, "-gen", "gcc", "-r",
                          str(ROOT / "tests/sdl3/platform-declarations.bas"), "-o",
                          str(output / ("platform-" + target + ".c"))], ROOT, environment,
                   output / ("emit-platform-" + target + ".log"))
            invoke(fbc + ["-target", target, "-gen", "gcc", "-r", str(output / "inline-addresses.bas"),
                          "-o", str(output / ("inline-" + target + ".c"))], ROOT, environment,
                   output / ("emit-inline-" + target + ".log"))
        report["emission"] = "six target profiles passed; cross-target binaries were not executed"
        report["platform_declarations"] = "Android, iOS, and GDK signatures emitted for all six profiles"
        with wave.open(str(output / "fixture.wav"), "wb") as stream:
            stream.setnchannels(1)
            stream.setsampwidth(2)
            stream.setframerate(8000)
            stream.writeframes(struct.pack("<400h", *([0] * 400)))
        invoke([args.cc, "-I", str(prefix / "include"), "-c", str(ROOT / "tests/sdl3/native-abi.c"),
                "-o", str(output / "native-abi.o")], ROOT, environment, output / "compile-bridge.log")
        for backend in ("gcc", "gas64"):
            executable = output / ("binding-smoke-" + backend)
            invoke(fbc + ["-mt", "-gen", backend, str(ROOT / "tests/sdl3/binding-smoke.bas"),
                          str(output / "native-abi.o"), "-x", str(executable)], ROOT, environment,
                   output / ("compile-smoke-" + backend + ".log"))
            invoke([str(executable), str(output / "fixture.wav"), str(output / "fixture.png"),
                    str(args.font.resolve())], output, environment, output / ("smoke-" + backend + ".log"))
            print(backend, "native smoke passed", flush=True)
        report["runtime"] = "GCC and gas64 passed"
    except (OSError, RuntimeError, subprocess.SubprocessError) as error:
        report["error"] = str(error)
        print(error, flush=True)
    (output / "results.json").write_text(json.dumps(report, indent=2) + "\n")
    return 1 if "error" in report else 0


if __name__ == "__main__":
    raise SystemExit(main())

# end of test-sdl3-bindings.py
