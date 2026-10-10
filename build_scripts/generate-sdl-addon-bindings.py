#!/usr/bin/env python3
"""Project: FreeBASIC SDL addon bindings
File: generate-sdl-addon-bindings.py
Purpose: Translate the pinned addon C interfaces into FreeBASIC includes.
Responsibilities: Preserve notices, calling conventions, and header implementations.
This file intentionally does NOT contain: library downloads or native builds.
"""

from __future__ import annotations

import argparse
from pathlib import Path
import re
import subprocess


ROOT = Path(__file__).resolve().parents[1]
SPECS = (
    ("SDL", "SDL_rtf", "SDL_rtf-1", "SDL_rtf.h", "SDL_rtf"),
    ("SDL2", "SDL_rtf", "SDL_rtf-2", "SDL_rtf.h", "SDL2_rtf"),
    ("SDL3", "SDL_rtf", "SDL_rtf", "include/SDL3_rtf/SDL_rtf.h", "SDL3_rtf"),
    ("SDL3", "SDL_shadercross", "SDL_shadercross", "include/SDL3_shadercross/SDL_shadercross.h", "SDL3_shadercross"),
    ("SDL", "SDL_gpu", "sdl-gpu", "include/SDL_gpu.h", "SDL_gpu"),
    ("SDL2", "SDL_gpu", "sdl-gpu", "include/SDL_gpu.h", "SDL2_gpu"),
    ("SDL2", "SDL_bgi", "SDL2_bgi", "src/SDL2_bgi.h", "SDL2_bgi"),
    ("SDL3", "SDL_bgi", "SDL3_bgi", "src/SDL3_bgi.h", "SDL3_bgi"),
    ("SDL", "SDL_Pango", "SDL_Pango", "src/SDL_Pango.h", "SDL_Pango"),
    ("SDL", "SDL_sound", "SDL_sound1", "SDL_sound.h", "SDL_sound"),
    ("SDL2", "SDL_sound", "SDL_sound2", "src/SDL_sound.h", "SDL2_sound"),
    ("SDL2", "SDL_FontCache", "SDL_FontCache", "SDL_FontCache.h", "SDL2_FontCache"),
    ("SDL", "SDL_FontCache", "SDL_FontCache", "SDL_FontCache.h", "SDL_FontCache"),
) + tuple(("SDL3", name, "SDL3_gfx", name + ".h", "SDL3_gfx") for name in
          ("SDL3_framerate", "SDL3_gfxPrimitives", "SDL3_gfxPrimitives_font", "SDL3_imageFilter", "SDL3_rotozoom"))


def source_directory(work, name):
    directories = sorted(path for path in (work / "sources" / name).iterdir() if path.is_dir())
    if len(directories) != 1:
        raise ValueError("Expected one pinned source directory for " + name)
    return directories[0]


def notice(header):
    text = header.read_text(encoding="latin-1")
    match = re.search(r'/\*(.*?)\*/', text, re.S)
    if not match:
        raise ValueError("Header has no copyright notice: " + str(header))
    return "\n".join(("'' " + line.rstrip()).rstrip() for line in match[1].strip().splitlines())


def clean(body, family, name):
    body = body.replace('as byte', 'as boolean') if family == "SDL3" else body
    body = re.sub(r'^#include once "(?:stdbool|SDL_endian|SDL_gpu_version)\.bi"\n', '', body, flags=re.M)
    body = body.replace('GPU_bool bool', 'GPU_bool boolean')
    body = body.replace('#macro SOUND_VERSION(', '#macro SOUND_VERSION_(')
    if name == 'SDL_sound':
        body = body.replace('type Sound_SampleFlags as long', 'type Sound_SampleFlags as ulong')
        body = body.replace('SOUND_SAMPLEFLAG_EAGAIN = 1 shl 31', 'SOUND_SAMPLEFLAG_EAGAIN = 1u shl 31')
    body = re.sub(r'^#include once "SDL2/SDL_(?:keycode|mouse)\.bi"\n', '', body, flags=re.M)
    if name == 'SDL_FontCache':
        body = body.replace('extern "C"', 'extern "C"\n\ntype FC_Font as FC_Font_', 1)
    if name == "SDL_gpu":
        start = body.index('\n#if ', body.index('#define GPU_bool'))
        end = body.index('\nconst GPU_FALSE', start)
        body = body[:start] + '\n#ifdef __FB_64BIT__\nconst SDL_GPU_BITNESS = 64\n#else\nconst SDL_GPU_BITNESS = 32\n#endif\n' + body[end:]
        body = body.replace('defined(__FB_64BIT__) and ((defined(__FB_LINUX__) and (not defined(__FB_ARM__))) or defined(__FB_WIN32__))',
                            'defined(__FB_64BIT__)')
        body = re.sub(r'(?:private )?function GPU_GetCompiledVersion\(\) as SDL_version.*?end function',
                      'private function GPU_GetCompiledVersion cdecl() as SDL_version\n'
                      '\tdim version as SDL_version = (0, 12, 0)\n\treturn version\nend function', body, flags=re.S)
        body = re.sub(r'^dim shared (GPU_RENDERER_\w+) as const GPU_RendererEnum = (.*)$',
                      r'const \1 as GPU_RendererEnum = \2', body, flags=re.M)
        body = body.replace('extern "C"', 'extern "C"\n\ntype GPU_RendererImpl as GPU_RendererImpl_', 1)
    if name == "SDL_bgi":
        if family == "SDL3":
            body = body.replace('WM_WHEEL SDL_MOUSEWHEEL', 'WM_WHEEL SDL_EVENT_MOUSE_WHEEL')
        body = body.replace('''\n'' TODO: #define SDL_BGI_VERSION 3.0.4''', '\n#define SDL_BGI_VERSION "3.0.4"')
        body = re.sub(r'^#define SEL_FUNC[^\n]*\n', '', body, flags=re.M)
        body = body.replace('#define random(', '#define bgi_random(')
        # These are BGI event values, not Windows message identifiers. Keep
        # them in the BGI namespace without redefining Windows' global names.
        body = re.sub(r'^#define (WM_\w+) (.*)$', r'const \1 = \2', body, flags=re.M)
        # Keep BGI's original procedure names in a namespace. Several overlap
        # with BASIC graphics commands and must not alter the global language.
        body = body.replace('extern "C"', 'namespace bgi\nextern "C"', 1)
        body += '\nend namespace\n'
        body += '\nnamespace bgi\n'
        body += 'private function initwindow cdecl(byval window_width as long, byval window_height as long, _\n'
        body += '\tbyval title as zstring ptr = 0) as long\n'
        body += '\tif title = 0 then return _initwin_1(window_width, window_height)\n'
        body += '\treturn _initwin_2(window_width, window_height, title)\nend function\nend namespace\n'
        seen = set()
        lines = []
        for line in body.splitlines():
            if line.startswith('declare ') and line in seen:
                continue
            seen.add(line)
            lines.append(line)
        body = '\n'.join(lines) + '\n'
    allowed = ('GPU_PAD_', 'SDL3_FRAMERATE_SCOPE', 'SDL3_GFXPRIMITIVES_SCOPE',
               'SDL3_IMAGEFILTER_SCOPE', 'SDL3_ROTOZOOM_SCOPE', 'SDL_BGI_VERSION', 'initwindow')
    for line in body.splitlines():
        if 'TODO:' in line and not any(name in line for name in allowed):
            raise ValueError("Untranslated declaration: " + line)
    body = re.sub(r'^.*TODO:.*\n', '', body, flags=re.M)
    if name == "SDL_Pango":
        # The C header defines these constants, rather than importing them
        # from the DLL. Keep them local so separate BASIC modules can include
        # this header without introducing duplicate global definitions.
        body = re.sub(r'^extern\s+.*MATRIX.*\n', '', body, flags=re.M)
        matrices = re.findall(r'^dim shared (_MATRIX_\w+) as const SDLPango_Matrix = \(([^\n]+)\)\n', body, re.M)
        body = re.sub(r'^dim shared .*MATRIX.*\n', '', body, flags=re.M)
        helpers = []
        for symbol, values in matrices:
            numbers = values.split(', ')
            # BASIC varies its first array subscript fastest, so transpose
            # the source initializer to retain C's exact sixteen-byte layout.
            rows = ['{' + ', '.join(numbers[row + col * 4] for col in range(4)) + '}' for row in range(4)]
            function = 'SDLPango_' + symbol.removeprefix('_')
            helpers += ['private function ' + function + ' cdecl() as const SDLPango_Matrix ptr',
                        '\tstatic matrix as SDLPango_Matrix = ({ ' + ', '.join(rows) + ' })',
                        '\treturn @matrix', 'end function',
                        '#define ' + symbol.removeprefix('_') + ' ' + function + '()']
        body = body.replace('end extern', '\n'.join(helpers) + '\n\nend extern', 1)
        # These entry points are declared conditionally by the C header when
        # its Pango/FreeType headers have already been included. Their symbols
        # are always present in this library, so expose the complete interface.
        additions = ('declare sub SDLPango_CopyFTBitmapToSurface(byval bitmap as const FT_Bitmap ptr, _\n'
                     '\tbyval surface as SDL_Surface ptr, byval matrix as const SDLPango_Matrix ptr, byval rect as SDL_Rect ptr)\n'
                     'declare function SDLPango_GetPangoFontMap(byval context as SDLPango_Context ptr) as PangoFontMap ptr\n'
                     'declare function SDLPango_GetPangoFontDescription(byval context as SDLPango_Context ptr) as PangoFontDescription ptr\n'
                     'declare function SDLPango_GetPangoLayout(byval context as SDLPango_Context ptr) as PangoLayout ptr\n')
        body = body.replace('extern "C"', '#include once "pango/pango.bi"\n#include once "freetype2/freetype.bi"\n\nextern "C"', 1)
        body = body.replace('end extern', additions + '\nend extern', 1)
    return body


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--workdir', type=Path, default=ROOT / 'out/sdl-addons')
    parser.add_argument('--fbfrog', type=Path, default=Path('/home/jkfirth/fb_corpus/fbfrog/fbfrog'))
    args = parser.parse_args()
    work = args.workdir.resolve()
    raw = work / 'raw'
    raw.mkdir(exist_ok=True)
    version = raw / 'version.h'
    version.write_text('/* Project: SDL bindings. File: version.h.\n'
                       '   Purpose: Select the SDL API version for preprocessing.\n'
                       '   Responsibilities: Supply one version predicate.\n'
                       '   This file intentionally does NOT contain: SDL declarations. */\n'
                       '#define SDL_VERSION_ATLEAST(x,y,z) SDL_BINDING_MAJOR\n/* End of version.h */\n')
    for family, name, package, path, library in SPECS:
        source = source_directory(work, package)
        header = source / path
        # SDL_gpu's old architecture test overlooks 64-bit ARM. This same
        # repair is used for its native library build and generated padding.
        if name == 'SDL_gpu' and '__aarch64__' not in header.read_text():
            text = header.read_text().replace('defined(__x86_64)',
                    '(defined(__x86_64) || defined(__aarch64__))').replace('defined(_M_X64)',
                    '(defined(_M_X64) || defined(_M_ARM64))')
            header.write_text(text)
        destination = raw / (family + '-' + name + '.bi')
        command = [str(args.fbfrog), '-target', 'linux', '-target', 'windows',
                   '-fbfroginclude', 'stdbool.h', '-define', 'SDL_DECLSPEC', '',
                   '-define', 'DECLSPEC', '', '-define', 'SDLCALL', '',
                   '-define', 'SDL_MAJOR_VERSION', '1' if family == 'SDL' else '2']
        if name == 'SDL_gpu':
            command += ['-define', 'SDL_BINDING_MAJOR', '0' if family == 'SDL' else '1',
                        '-include', str(version), '-incdir', str(source / 'include')]
        if name == 'SDL_FontCache' and family == 'SDL':
            command += ['-define', 'FC_USE_SDL_GPU', '1']
        for include in ('SDL.h', 'SDL3/SDL.h', 'SDL2/SDL.h', 'SDL_ttf.h', 'SDL_gpu.h',
                        'SDL_endian.h', 'SDL_begin_code.h', 'SDL_close_code.h',
                        'SDL3/SDL_begin_code.h', 'SDL3/SDL_close_code.h', 'begin_code.h',
                        'close_code.h', 'stdio.h', 'stdarg.h', 'math.h'):
            if include != header.name:
                command += ['-removeinclude', include]
        command += ['-include', str(header), '-emit', str(header), str(destination), '-dontemit', '*']
        result = subprocess.run(command, capture_output=True, text=True, timeout=60)
        (raw / (family + '-' + name + '.log')).write_text(result.stdout + result.stderr)
        if result.returncode:
            raise RuntimeError("fbfrog failed for " + str(header))
        body = clean(destination.read_text(), family, name)
        filename = name + '.bi'
        heading = ("'' Project: FreeBASIC SDL addon bindings\n'' File: " + filename + '\n'
                   "'' Purpose: Declare the pinned " + library + ' C interface.\n'
                   "'' Responsibilities: Preserve public types, callbacks, and header helpers.\n"
                   "'' This file intentionally does NOT contain: the external library implementation.\n"
                   "'' Translated from upstream; this is an altered source version.\n''\n" + notice(header) + '\n\n')
        includes = '#inclib "' + library + '"\n#include once "SDL.bi"\n'
        if name == 'SDL_FontCache':
            includes += '#include once "SDL_ttf.bi"\n'
            if family == 'SDL':
                includes += '#include once "SDL_gpu.bi"\n'
        body = body.replace('#pragma once', '#pragma once\n\n' + includes, 1)
        if 'extern "C"' not in body:
            body = body.replace(includes, includes + '\nextern "C"\n', 1) + '\nend extern\n'
        output = ROOT / 'inc' / family / filename
        output.write_text(heading + body + "\n'' End of " + filename + '\n')
        print(output.relative_to(ROOT), flush=True)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

# End of generate-sdl-addon-bindings.py
