#!/usr/bin/env python3
"""Project: FreeBASIC SDL3 bindings
File: generate-sdl3-bindings.py
Purpose: Regenerate the declarations from the pinned upstream C headers.
Responsibilities: Run fbfrog, repair language differences, and retain notices.
This file intentionally does NOT contain: a C parser or SDL implementations.
"""

from __future__ import annotations

import argparse
from pathlib import Path
import re
import subprocess

import importlib.util


ROOT = Path(__file__).resolve().parents[1]
BUILD_SPEC = importlib.util.spec_from_file_location("sdl3_build", Path(__file__).with_name("build-sdl3.py"))
BUILD_MODULE = importlib.util.module_from_spec(BUILD_SPEC)
BUILD_SPEC.loader.exec_module(BUILD_MODULE)
PACKAGES = BUILD_MODULE.LIBRARIES
INLINE_FUNCTIONS = {
    "SDL_Swap16", "SDL_Swap32", "SDL_Swap64", "SDL_SwapFloat",
    "SDL_MostSignificantBitIndex32", "SDL_HasExactlyOneBitSet32",
    "SDL_RectToFRect", "SDL_PointInRect", "SDL_RectEmpty", "SDL_RectsEqual",
    "SDL_PointInRectFloat", "SDL_RectEmptyFloat", "SDL_RectsEqualEpsilon",
    "SDL_RectsEqualFloat", "SDL_size_mul_check_overflow", "SDL_size_add_check_overflow",
}


def check_conversion(body: str) -> None:
    # Converter diagnostics must have an explicit disposition. A new SDL
    # release must not lose an unfamiliar macro or function silently.
    names = ("SDL_COMPILE_TIME_ASSERT", "SDL_compile_time_assert_bool_size",
             "SDL_RESTRICT", "SDL_INLINE", "SDL_copyp", "SDL_iconv_utf8_ucs2",
             "SDL_iconv_utf8_ucs4", "SDL_iconv_wchar_utf8", "SDL_TriggerBreakpoint",
             "SDL_CompilerBarrier", "SDL_MemoryBarrierRelease", "SDL_MemoryBarrierAcquire",
             "SDL_CPUPauseInstruction", "SDL_DEFINE_AUDIO_FORMAT", "SDL_DLNOTE_JSON_ARRAY",
             "SDL_ELF_NOTE_DLOPEN", "VK_DEFINE_HANDLE", "VK_DEFINE_NON_DISPATCHABLE_HANDLE")
    for line in body.splitlines():
        if "TODO:" not in line:
            continue
        diagnostic = line.split("TODO:", 1)[1].strip()
        if diagnostic in ("continue;", "break;"):
            continue
        if not any(re.search(r"\b" + name + r"\b", diagnostic) for name in names):
            raise ValueError("Unhandled header conversion: " + diagnostic)


def file_header(filename: str, package: str, notice: str) -> str:
    heading = (
        "Project: FreeBASIC SDL3 bindings\n"
        "File: " + filename + "\n"
        "Purpose: Declare the " + package + " C interface for FreeBASIC.\n"
        "Responsibilities: Preserve public types, constants, and calling conventions.\n"
        "This file intentionally does NOT contain: the upstream library implementation.\n\n"
        "Translated from the upstream headers; this is an altered source version.\n"
        "Regenerate with build_scripts/generate-sdl3-bindings.py.\n\n" + notice.strip()
    )
    return "\n".join(("'' " + line).rstrip() if line else "''"
                     for line in heading.splitlines()) + "\n\n"


def opaque_types(headers: list[Path], body: str) -> str:
    opaque = set()
    for header in headers:
        opaque.update(re.findall(r"typedef struct (\w+)\s+\1;", header.read_text()))
    # An incomplete C struct can only be passed by pointer. An unresolved FB
    # alias preserves that restriction without inventing a one-byte struct.
    complete = set(re.findall(r"^(?:type|union) (\w+)\s*$", body, re.M))
    return "\n".join("type " + name + " as " + name + "_"
                     for name in sorted(opaque - complete))


def clean(body: str, core: bool) -> str:
    check_conversion(body)
    body = re.sub(r'^[ \t]*#include once .*\n', '', body, flags=re.M)
    body = re.sub(r'^\s*(?:#define|#undef) SDL_begin_code_h.*\n', '', body, flags=re.M)
    body = body.replace("as byte", "as boolean").replace("type Sint8 as boolean", "type Sint8 as byte")
    body = body.replace("\tmod as SDL_Keymod", "\tmod_ as SDL_Keymod")
    body = body.replace("type SDL_PixelType as", "type SDL_PixelType_ as")
    body = body.replace('declare sub SDL_Log(', 'declare sub SDL_Log_ alias "SDL_Log"(')
    body = body.replace('type Sound_SampleFlags as long', 'type Sound_SampleFlags as ulong')
    body = re.sub(r'(SOUND_SAMPLEFLAG_\w+ = )1 shl', r'\g<1>1u shl', body)
    body = re.sub(r'^const (SDL_(?:(?:IMAGE|MIXER|TTF|NET|SOUND)_)?(?:MAJOR|MINOR|MICRO)_VERSION) =',
                  r'const \1 as long =', body, flags=re.M)
    for name in ("SDL_PRIX64", "SDL_PRIX32", "SDL_PRILLX"):
        body = re.sub(r"\b" + name + r"\b", name + "_", body)
    if core:
        removed = set(re.findall(r'^private (?:function|sub) (\w+)', body, re.M))
        if removed - INLINE_FUNCTIONS:
            raise ValueError("Inline functions need BASIC implementations: " + repr(sorted(removed - INLINE_FUNCTIONS)))
        body = re.sub(r'^\s*const NULL.*\n', '', body, flags=re.M)
        body = re.sub(r'^#define SDL_mem(?:cpy|move|set)\s.*\n', '', body, flags=re.M)
        body = re.sub(r'^private (function|sub) .*?^end \1\n', '', body, flags=re.M | re.S)
        body = re.sub(r'^#define SDL_RectsEqualFloat\(.*\n', '', body, flags=re.M)
        body = re.sub(r'^#macro SDL_(?:enabled|disabled)_assert.*?^#endmacro\n', '', body, flags=re.M | re.S)
        # Compiler annotations and inline assembly are not part of the C ABI.
        # SDL_inline.bi supplies portable versions of the useful public helpers.
        body = re.sub(r'^.*(?:SDL_TriggerBreakpoint|SDL_assert(?:_release|_paranoid|_always)?\(|SDL_ASSERT_LEVEL|SDL_FUNCTION|SDL_null_while|SDL_NULL_WHILE|__debugbreak|SDL_CompilerBarrier|SDL_MemoryBarrier(?:Acquire|Release)\(|SDL_CPUPauseInstruction|HAS_.*BSWAP|SDL_Swap(?:16|32|64|Float)(?:LE|BE)?\().*\n', '', body, flags=re.M)
        body = re.sub(r'#define SDL_(SINT|UINT)64_C\(c\) c##[UL]+',
                      lambda match: '#define SDL_' + match[1] + '64_C(c) c##' +
                      ('LL' if match[1] == 'SINT' else 'ULL'), body)
        body = body.replace('type(expression)', 'cast(type, expression)')
        body = body.replace('(sizeof(array) / sizeof(array[0]))', '(ubound(array) - lbound(array) + 1)')
        body = re.sub(r'^#define SDL_zeroa.*\n', '', body, flags=re.M)
        # BASIC promotes arithmetic to its pointer-sized Integer. C version
        # fields and audio byte counts are int expressions, including when
        # passed through varargs, so cast their final results back to Long.
        body = body.replace('((version) / 1000000)', 'clng((version) \\ 1000000L)')
        body = body.replace('(((version) / 1000) mod 1000)', 'clng(((version) \\ 1000L) mod 1000L)')
        body = body.replace('((version) mod 1000)', 'clng((version) mod 1000L)')
        body = body.replace('* 1000000)', '* 1000000L)').replace('* 1000)', '* 1000L)')
        body = body.replace('(SDL_AUDIO_BITSIZE(x) / 8)', 'clng(SDL_AUDIO_BITSIZE(x) \\ 8L)')
        body = body.replace('(SDL_AUDIO_BYTESIZE((x).format) * (x).channels)',
                            'clng(SDL_AUDIO_BYTESIZE((x).format) * (x).channels)')
        for unit in ('SECOND', 'MS', 'US'):
            body = body.replace('((NS) / SDL_NS_PER_' + unit + ')',
                                '((NS) \\ SDL_NS_PER_' + unit + ')')
        body = re.sub(r'^const (SDL_AUDIO_MASK_\w+) = (.*)$', r'const \1 as long = \2',
                      body, flags=re.M)
        body = re.sub(r'^const (SDL_(?:MS_PER_SECOND|US_PER_SECOND|NS_PER_MS|NS_PER_US)) =',
                      r'const \1 as long =', body, flags=re.M)
        body = body.replace('cbyte(SDL_RectsEqualEpsilon', 'cbool(SDL_RectsEqualEpsilon')
        # FB LongInt is C long long in the generated C, even on LP64 systems
        # where SDL's C typedef uses long. Match the BASIC variadic argument.
        format_end = body.index('\n#define SDL_PRIs32')
        format_start = body.rfind('\n#if', 0, format_end)
        body = body[:format_start] + '\n#define SDL_PRIs64 SDL_PRILLd\n' + \
               '#define SDL_PRIu64 SDL_PRILLu\n#define SDL_PRIx64 SDL_PRILLx\n' + \
               '#define SDL_PRIX64_ SDL_PRILLX_\n' + body[format_end:]
        audio_aliases = ('SDL_AUDIO_S16', 'SDL_AUDIO_S32', 'SDL_AUDIO_F32')
        pixel_aliases = {'RGBA32': 'RGBA8888', 'ARGB32': 'ARGB8888',
                         'BGRA32': 'BGRA8888', 'ABGR32': 'ABGR8888',
                         'RGBX32': 'RGBX8888', 'XRGB32': 'XRGB8888',
                         'BGRX32': 'BGRX8888', 'XBGR32': 'XBGR8888'}
        for alias in audio_aliases:
            body = re.sub(r'^\s*' + alias + r' = .*$',
                          '#ifdef __FB_BIGENDIAN__\n\t' + alias + ' = ' + alias + 'BE\n' +
                          '#else\n\t' + alias + ' = ' + alias + 'LE\n#endif', body, flags=re.M)
        for alias, big in pixel_aliases.items():
            body = re.sub(r'^\s*(SDL_PIXELFORMAT_' + alias + r' = .*)$',
                          lambda match, alias=alias, big=big: '#ifdef __FB_BIGENDIAN__\n\tSDL_PIXELFORMAT_' +
                          alias + ' = SDL_PIXELFORMAT_' + big + '\n#else\n' + match[1] + '\n#endif',
                          body, flags=re.M)
        # SDL's Windows macros pass the calling executable's CRT entry points.
        # A function address, rather than a call, is required in BASIC.
        body = body.replace('SDL_BeginThreadFunction _beginthreadex', 'SDL_BeginThreadFunction @_beginthreadex')
        body = body.replace('SDL_EndThreadFunction _endthreadex', 'SDL_EndThreadFunction @_endthreadex')
        body = re.sub(r'^\s*(?:const|#define) SDL_(Begin|End)ThreadFunction\s*(?:=\s*)?NULL\s*$',
                      lambda match: '#define SDL_' + match[1] + 'ThreadFunction 0', body, flags=re.M)
        thread_start = body.index('\n#if', body.index('type SDL_ThreadFunction'))
        thread_end = body.index('\ndeclare function SDL_CreateThreadRuntime', thread_start)
        body = body[:thread_start] + '\n#ifdef __FB_WIN32__\n' + \
               '\t#define SDL_BeginThreadFunction @_beginthreadex\n' + \
               '\t#define SDL_EndThreadFunction @_endthreadex\n#else\n' + \
               '\t#define SDL_BeginThreadFunction 0\n\t#define SDL_EndThreadFunction 0\n#endif\n' + body[thread_end:]
    body = re.sub(r'^.*TODO:.*\n', '', body, flags=re.M)
    body = re.sub(r'^#define (SDL_\w+)_h_\s*$',
                  lambda match: "'' " + "-" * 73 + "\n'' " + match[1] + ".h\n'' " + "-" * 73,
                  body, flags=re.M)
    body = re.sub(r'\n(?:[ \t]*\n){2,}', '\n\n', body)
    body = '\n'.join(line.rstrip() for line in body.splitlines()) + '\n'
    while True:
        reduced = re.sub(r'^#if[^\n]*\n[ \t\n]*#endif\n', '', body, flags=re.M)
        if reduced == body:
            break
        body = reduced
    return body


def generate(fbfrog: Path, source: Path, output: Path, raw: Path, name: str,
             headers: list[Path], core: Path) -> None:
    command = [str(fbfrog), "-target", "linux", "-target", "windows",
               "-target", "darwin", "-target", "freebsd", "-target", "openbsd",
               "-target", "netbsd", "-define", "MAC_OS_X_VERSION_MIN_REQUIRED", "1070",
               "-incdir", str(core), "-incdir", str(source / "include"),
               "-fbfroginclude", "stdbool.h", "-define", "SDL_DISABLE_ALLOCA",
               "-define", "SDL_DISABLE_OLD_NAMES", "-define", "SDL_MAIN_HANDLED",
               "-define", "SDL_BYTEORDER", "1234", "-define", "SDL_FLOATWORDORDER", "1234"]
    for include in ("inttypes.h", "endian.h", "sys/endian.h", "machine/endian.h",
                    "intrin.h", "immintrin.h", "SDL3/SDL_main_impl.h"):
        command += ["-removeinclude", include]
    for header in headers:
        command += ["-include", str(header)]
    if name == "SDL3":
        # SDL.h includes the common runtime API. Vulkan and SDL_test remain
        # separate, just as they are in C.
        command += ["-emit", "*/SDL3/*.h", str(raw / "SDL.bi")]
    else:
        for header in headers:
            command += ["-emit", "*/" + header.parent.name + "/" + header.name,
                        str(raw / (header.stem + ".bi"))]
    command += ["-dontemit", "*"]
    with (raw / (name + ".log")).open("w") as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    filenames = ["SDL.bi"] if name == "SDL3" else sorted({header.stem + ".bi" for header in headers})
    for filename in filenames:
        generated = raw / filename
        if name == "SDL3" and filename != "SDL.bi":
            continue
        body = clean(generated.read_text(), name == "SDL3")
        upstream_headers = list((source / "include").rglob("*.h"))
        aliases = opaque_types(upstream_headers, body)
        includes = '#include once "crt/long.bi"\n#include once "crt/stdarg.bi"\n'
        if name == "SDL3":
            includes += '#ifdef __FB_WIN32__\n\t#include once "crt/process.bi"\n#endif\n'
        else:
            includes = '#include once "SDL.bi"\n'
            if filename == "SDL_textengine.bi":
                includes += '#include once "SDL_ttf.bi"\n'
                aliases = ""
        body = body.replace('#pragma once', '#pragma once\n\n#inclib "' + name + '"\n\n' + includes, 1)
        body = body.replace('extern "C"', 'extern "C"\n\n' + aliases, 1)
        if name == "SDL3":
            body += '\n#include once "SDL_inline.bi"\n'
            body += '#include once "SDL_platform_api.bi"\n'
        notice = (source / "LICENSE.txt").read_text()
        destination = output / filename
        destination.write_text(file_header(filename, source.name, notice) + body.rstrip() +
                               "\n\n'' end of " + filename + "\n")
        print(destination, flush=True)


def generate_optional(fbfrog: Path, source: Path, output: Path, raw: Path) -> None:
    core = source / "include"
    for family in ("vulkan", "test"):
        filename = "SDL_" + family + ".bi"
        command = [str(fbfrog), "-target", "linux", "-target", "windows",
                   "-incdir", str(core), "-fbfroginclude", "stdbool.h",
                   "-define", "SDL_DISABLE_ALLOCA", "-define", "SDL_DISABLE_OLD_NAMES",
                   "-define", "SDL_BYTEORDER", "1234", "-define", "SDL_FLOATWORDORDER", "1234",
                   "-removeinclude", "inttypes.h", "-removeinclude", "endian.h",
                   "-include", "SDL3/SDL_" + family + ".h",
                   "-emit", "*/SDL3/SDL_" + family + "*.h", str(raw / filename),
                   "-dontemit", "*"]
        with (raw / ("SDL3_" + family + ".log")).open("w") as log:
            subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
        body = clean((raw / filename).read_text(), False)
        body = body.replace('#pragma once', '#pragma once\n\n#include once "SDL.bi"', 1)
        if family == "test":
            body = body.replace('#pragma once', '#pragma once\n\n#inclib "SDL3_test"', 1)
        else:
            start = body.index('type VkInstance')
            end = body.index('declare function SDL_Vulkan_LoadLibrary')
            handles = body[start:end]
            handles = re.sub(r'^#undef VK_DEFINE_.*\n', '', handles, flags=re.M)
            body = body[:start] + '#ifndef VK_VERSION_1_0\n' + handles + \
                   'type VkAllocationCallbacks as VkAllocationCallbacks_\n#endif\n\n' + body[end:]
        header = core / ("SDL3/SDL_" + family + ".h")
        notice = re.match(r'/\*(.*?)\*/', header.read_text(), re.S)[1].strip()
        (output / filename).write_text(file_header(filename, source.name, notice) + body.rstrip() +
                                      "\n\n'' end of " + filename + "\n")
        print(output / filename, flush=True)


def generate_wrappers(source: Path, output: Path) -> None:
    wrappers = set(re.findall(r'#include <SDL3/(SDL_\w+)\.h>',
                              (source / "include/SDL3/SDL.h").read_text()))
    wrappers.add("SDL_main")
    for stem in sorted(wrappers):
        filename = stem + ".bi"
        text = file_header(filename, source.name, "")
        text += "'' The common declarations live in SDL.bi, matching the existing\n"
        text += "'' FreeBASIC SDL bindings. This entry point also supports C-style includes.\n\n"
        text += "'' SDL.bi includes SDL_inline.bi for the header-only implementations.\n\n"
        text += '#pragma once\n\n#include once "SDL.bi"\n\n'
        text += "'' end of " + filename + "\n"
        (output / filename).write_text(text)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fbfrog", type=Path, required=True)
    parser.add_argument("--workdir", type=Path, default=ROOT / "out/sdl3")
    parser.add_argument("--output", type=Path, default=ROOT / "inc/SDL3")
    args = parser.parse_args()
    work = args.workdir.resolve()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    raw = work / "bindings-raw"
    raw.mkdir(parents=True, exist_ok=True)
    core = work / "sources" / PACKAGES[0][2] / "include"
    for _, _, package, _ in PACKAGES:
        source = work / "sources" / package
        name = package.rsplit("-", 1)[0]
        if name == "SDL3":
            headers = [core / "SDL3/SDL.h", core / "SDL3/SDL_main.h"]
        else:
            headers = sorted((source / "include").rglob("*.h"),
                             key=lambda path: (path.name == "SDL_textengine.h", path.name))
        generate(args.fbfrog.resolve(), source, output, raw, name, headers, core)
    generate_optional(args.fbfrog.resolve(), core.parent, output, raw)
    generate_wrappers(core.parent, output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

# end of generate-sdl3-bindings.py
