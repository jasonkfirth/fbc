#!/usr/bin/env python3
"""Project: FreeBASIC SDL addon SDK
File: build-sdl-addons.py
Purpose: Build the pinned addon libraries for native checks and Windows targets.
Responsibilities: Keep builds isolated, use real dependencies, and retain results.
This file intentionally does NOT contain: compiler changes or invented import stubs.
"""

from __future__ import annotations

import argparse
from concurrent.futures import ThreadPoolExecutor
import json
import os
from pathlib import Path
import re
import shutil
import subprocess


ROOT = Path(__file__).resolve().parents[1]
PROFILES = {"win32": ("i686-w64-mingw32", "mingw32"),
            "win64": ("x86_64-w64-mingw32", "mingw64"),
            "win32-aarch64": ("aarch64-w64-mingw32", "clangarm64")}
PACKAGES = ("SDL_rtf", "SDL2_rtf", "SDL3_rtf", "SDL_gpu", "SDL2_gpu", "SDL3_gfx",
            "SDL2_bgi", "SDL3_bgi", "SDL_FontCache", "SDL2_FontCache", "SDL_Pango", "SDL_sound", "SDL2_sound",
            "SDL2_gfx", "SDL2_net", "SDL3_shadercross")


def source(work, name):
    directories = [p for p in (work / "sources" / name).iterdir() if p.is_dir()]
    if len(directories) != 1:
        raise ValueError("Ambiguous pinned source: " + name)
    return directories[0]


def run(command, environment, log, directory=ROOT):
    with log.open("w") as output:
        output.write("command: " + repr(command) + "\n")
        output.flush()
        result = subprocess.run(command, cwd=directory, env=environment,
                                stdout=output, stderr=subprocess.STDOUT, timeout=300)
    if result.returncode:
        raise RuntimeError("Command failed: " + str(log))


# -------------------------------------------------------------------------
# Target dependencies and toolchains
# -------------------------------------------------------------------------


def prepare_arm_core(work, stage, toolchain, environment):
    """Import the exports of the pinned upstream ARM64 DLLs, without stubs."""
    directory = work / "build/win32-aarch64/imports"
    directory.mkdir(parents=True, exist_ok=True)
    for family in (ROOT / "out/sdl3/prefix/include").glob("SDL3*"):
        if family.is_dir():
            shutil.copytree(family, stage / "include" / family.name, dirs_exist_ok=True)
    for package in json.loads(Path(__file__).with_name("sdl-addon-packages.json").read_text())["core_arm64"]:
        origin = work / "sources" / package["name"].removesuffix(".zip")
        libraries = list(origin.rglob("*.dll"))
        if not libraries:
            raise ValueError("Missing pinned ARM64 runtime: " + str(origin))
        for dll in libraries:
            shutil.copy2(dll, stage / "bin" / dll.name)
            if not dll.name.startswith("SDL3"):
                continue
            exports = subprocess.run(["llvm-readobj", "--coff-exports", str(dll)],
                                     capture_output=True, text=True, check=True).stdout
            names = re.findall(r"^  Name: (.+)$", exports, re.M)
            if not names:
                raise ValueError("ARM64 DLL has no named exports: " + str(dll))
            definition = directory / (dll.stem + ".def")
            definition.write_text("; Project: FreeBASIC SDL ARM64 SDK\n"
                                  "; File: " + definition.name + "\n"
                                  "; Purpose: Import the named exports of the matching upstream DLL.\n"
                                  "; Responsibilities: Preserve export names exactly.\n"
                                  "; This file intentionally does NOT contain: implementations.\n"
                                  "LIBRARY " + dll.name + "\nEXPORTS\n" +
                                  "".join("  " + name + "\n" for name in names) +
                                  "; End of " + definition.name + "\n")
            run([str(toolchain / "bin/aarch64-w64-mingw32-dlltool"), "-d", str(definition),
                 "-l", str(stage / "lib" / ("lib" + dll.stem + ".dll.a"))],
                environment, directory / (dll.stem + ".log"))


def prepare(work, target, copy_dependencies=True):
    stage = work / "stage" / target
    for folder in ("lib", "bin", "include"):
        (stage / folder).mkdir(parents=True, exist_ok=True)
    if target == "linux-x86_64":
        if copy_dependencies:
            shutil.copytree(ROOT / "out/sdl3/prefix", stage, dirs_exist_ok=True)
            for library in (work / "sources/DXC-Linux/lib").glob("*.so"):
                shutil.copy2(library, stage / "lib" / library.name)
        return stage, "cc", os.environ.copy()
    triple, msys = PROFILES[target]
    if target in ("win32", "win64"):
        if copy_dependencies:
            shutil.copytree(ROOT / "out/sdl-windows/sdk" / target, stage, dirs_exist_ok=True)
        toolchain = (ROOT / "out/sdl-windows/toolchain/usr").resolve()
        compiler = str(toolchain / "bin" / (triple + "-gcc"))
    else:
        toolchain = next((work / "toolchain/llvm-mingw").iterdir())
        compiler = str(toolchain / "bin" / (triple + "-clang"))
    environment = os.environ.copy()
    environment["PATH"] = str(toolchain / "bin") + os.pathsep + environment["PATH"]
    dependency = work / "stage/msys" / msys / msys
    if copy_dependencies and dependency.exists():
        # Preserve the established SDL1/SDL2/SDL3 DLL versions on x86/x64.
        # The ARM64 SDL2 SDK and dependency imports come from MSYS2.
        for folder in ("include", "lib", "bin", "share"):
            if not (dependency / folder).exists():
                continue
            for path in (dependency / folder).rglob("*"):
                if path.is_file():
                    out = stage / folder / path.relative_to(dependency / folder)
                    replace_runtime = (folder == "bin" and path.suffix.lower() == ".dll" and
                                       not path.name.lower().startswith(("sdl", "libsdl")))
                    if not out.exists() or replace_runtime:
                        out.parent.mkdir(parents=True, exist_ok=True)
                        shutil.copy2(path, out)
    # sdl12-compat's development headers preserve the old SDL1 ABI and also
    # work on ARM64. They are needed by the old surface-based addon sources.
    if not (stage / "include/SDL/SDL.h").exists():
        shutil.copytree(Path("/usr/include/SDL"), stage / "include/SDL", dirs_exist_ok=True)
    if copy_dependencies:
        llvm = next((work / "toolchain/llvm-mingw").iterdir())
        # SPIRV-Cross uses LLVM's C++ runtime even when the remaining x86
        # libraries were built with GCC. Keep its exact runtime DLLs too.
        for name in ("libc++.dll", "libunwind.dll", "libwinpthread-1.dll"):
            runtime = llvm / triple / "bin" / name
            if target == "win32-aarch64":
                # The matching LLVM 23.1.3 MSYS2 runtimes are ordinary ARM64
                # images. The toolchain also carries hybrid ARM64X images.
                runtime = dependency / "bin" / name
            shutil.copy2(runtime, stage / "bin" / name)
        if target == "win32-aarch64":
            prepare_arm_core(work, stage, toolchain, environment)
        for name in ("SDL_image.h", "SDL_mixer.h", "SDL_ttf.h", "SDL_net.h"):
            header = Path("/usr/include/SDL") / name
            if header.is_file() and not (stage / "include/SDL" / name).exists():
                shutil.copy2(header, stage / "include/SDL" / name)
        arch = {"win32": "x86", "win64": "x64", "win32-aarch64": "arm64"}[target]
        dxc = work / "sources/DXC-Windows"
        if (dxc / "bin" / arch).is_dir():
            for name in ("dxcompiler.dll", "dxil.dll"):
                shutil.copy2(dxc / "bin" / arch / name, stage / "bin" / name)
            shutil.copy2(dxc / "lib" / arch / "dxcompiler.lib", stage / "lib/libdxcompiler.dll.a")
    return stage, compiler, environment


def pkg_flags(*names):
    result = subprocess.run(["pkg-config", "--cflags", "--libs", *names],
                            capture_output=True, text=True, check=True)
    import shlex
    return shlex.split(result.stdout)


def repair_sources(work):
    """Apply small compatibility repairs to isolated upstream sources."""
    font = source(work, "SDL_FontCache") / "SDL_FontCache.c"
    text = font.read_text()
    if 'SDL_SetAlpha(glyph_surf, 0, SDL_ALPHA_OPAQUE)' not in text:
        text = re.sub(r'^([ \t]*)(SDL_SetSurfaceBlendMode\((\w+), SDL_BLENDMODE_NONE\));$',
                      lambda m: '#if SDL_VERSION_ATLEAST(2,0,0)\n' + m[0] + '\n#else\n' +
                      m[1] + 'SDL_SetAlpha(' + m[3] + ', 0, SDL_ALPHA_OPAQUE);\n#endif', text, flags=re.M)
        font.write_text(text)
    gpu = source(work, "sdl-gpu") / "include/SDL_gpu.h"
    text = gpu.read_text()
    if '__aarch64__' not in text:
        gpu.write_text(text.replace('defined(__x86_64)', '(defined(__x86_64) || defined(__aarch64__))')
                       .replace('defined(_M_X64)', '(defined(_M_X64) || defined(_M_ARM64))'))
    glew = source(work, "sdl-gpu") / "src/externals/glew/glew.c"
    text = glew.read_text()
    old = '  GLuint length = _glewStrLen(s);\n  GLubyte* result = (GLubyte*)malloc(length * sizeof(GLubyte));\n  GLuint i=0;'
    new = ('  GLuint length;\n  GLubyte* result;\n  GLuint i=0;\n'
           '  /* Include the terminator and reject invalid allocation sizes. */\n'
           '  if (s == NULL) return NULL;\n  length = _glewStrLen(s);\n'
           '  if ((size_t)length == (size_t)-1) return NULL;\n'
           '  result = (GLubyte*)malloc((size_t)length + 1);\n'
           '  if (result == NULL) return NULL;')
    if old in text:
        glew.write_text(text.replace(old, new, 1))
    interface = source(work, "sdl-gpu") / "include/SDL_gpu_RendererImpl.h"
    text = interface.read_text()
    repaired = text.replace('SDL_Surface* surface, GPU_Rect *surface_rect',
                            'SDL_Surface* surface, const GPU_Rect *surface_rect')
    if repaired != text:
        interface.write_text(repaired)
    pango = source(work, "SDL_Pango") / "src/SDL_Pango.h"
    text = pango.read_text()
    # Modern FreeType uses a different include guard for this public type.
    text = text.replace('#ifdef __FT2_BUILD_UNIX_H__', '#if defined(__FT2_BUILD_UNIX_H__) || defined(FT_FREETYPE_H)')
    if text != pango.read_text():
        pango.write_text(text)


# -------------------------------------------------------------------------
# Focused C library builds
# -------------------------------------------------------------------------


def build(package, target, work):
    directory = work / "build" / target / package
    directory.mkdir(parents=True, exist_ok=True)
    record = {"library": package, "target": target}
    try:
        stage, compiler, environment = prepare(work, target, False)
        if package in ("SDL2_sound", "SDL2_gfx", "SDL2_net") and target != "linux-x86_64":
            # The verified MSYS2 packages contain these libraries for all
            # three Windows architectures, including their codec builds.
            library = stage / "lib" / ("lib" + package + ".dll.a")
            if not library.is_file():
                raise ValueError("Missing Windows addon library: " + str(library))
            record.update(status="built", library_path=str(library))
            return record
        if package == "SDL2_sound":
            if target == "linux-x86_64":
                origin = source(work, "SDL_sound2")
                run(["cmake", "-S", str(origin), "-B", str(directory), "-DCMAKE_BUILD_TYPE=Release",
                     "-DCMAKE_INSTALL_PREFIX=" + str(stage), "-DSDLSOUND_BUILD_SHARED=ON",
                     "-DSDLSOUND_BUILD_STATIC=OFF", "-DSDLSOUND_BUILD_TEST=OFF", "-DSDLSOUND_BUILD_DOCS=OFF"],
                    environment, directory / "configure.log")
                run(["cmake", "--build", str(directory), "-j3"], environment, directory / "build.log")
                run(["cmake", "--install", str(directory)], environment, directory / "install.log")
                library = stage / "lib/libSDL2_sound.so"
            if not library.is_file():
                raise ValueError("Missing SDL2_sound library: " + str(library))
            record.update(status="built", library_path=str(library))
            return record
        family = "SDL3" if package.startswith("SDL3") else "SDL2" if package.startswith("SDL2") else "SDL"
        definitions = []
        includes = []
        extra_libs = []
        if package.endswith("_rtf"):
            folder = "SDL_rtf" if family == "SDL3" else "SDL_rtf-2" if family == "SDL2" else "SDL_rtf-1"
            origin = source(work, folder)
            src = origin / "src" if family == "SDL3" else origin
            files = [src / name for name in ("SDL_rtf.c", "SDL_rtfreadr.c", "rtfactn.c", "rtfreadr.c")]
            includes = [origin / "include", src]
            definitions = ["DLL_EXPORT", "BUILD_SDL", "SDL_BUILD_MAJOR_VERSION=" + ("3" if family == "SDL3" else "2"),
                           "SDL_BUILD_MINOR_VERSION=0", "SDL_BUILD_MICRO_VERSION=0"]
        elif package.endswith("_bgi"):
            origin = source(work, package)
            files = [origin / "src" / (package + ".c")]
            includes = [origin / "src"]
        elif package in ("SDL2_gfx", "SDL3_gfx"):
            origin = source(work, package)
            files = [origin / (family + name) for name in ("_framerate.c", "_gfxPrimitives.c", "_imageFilter.c", "_rotozoom.c")]
            includes = [origin]
            definitions = ["DLL_EXPORT"]
        elif package == "SDL2_net":
            origin = source(work, package)
            files = [origin / name for name in ("SDLnet.c", "SDLnetTCP.c", "SDLnetUDP.c", "SDLnetselect.c")]
            includes = [origin]
        elif package.endswith("_FontCache"):
            origin = source(work, "SDL_FontCache")
            files = [origin / "SDL_FontCache.c"]
            includes = [origin]
            extra_libs = [family + "_ttf"]
            if family == "SDL":
                definitions = ["FC_USE_SDL_GPU=1"]
                includes += [source(work, "sdl-gpu") / "include"]
                extra_libs += ["SDL_gpu"]
        elif package.endswith("_gpu"):
            origin = source(work, "sdl-gpu")
            src = origin / "src"
            files = [src / name for name in ("SDL_gpu.c", "SDL_gpu_matrix.c", "SDL_gpu_renderer.c", "SDL_gpu_shapes.c",
                     "renderer_OpenGL_1_BASE.c", "renderer_OpenGL_1.c", "renderer_OpenGL_2.c", "renderer_OpenGL_3.c",
                     "renderer_OpenGL_4.c", "renderer_GLES_1.c", "renderer_GLES_2.c", "renderer_GLES_3.c",
                     "externals/glew/glew.c", "externals/stb_image/stb_image.c", "externals/stb_image_write/stb_image_write.c")]
            includes = [origin / "include", src / "externals/glew", src / "externals/glew/GL",
                        src / "externals/stb_image", src / "externals/stb_image_write"]
            definitions = ["GLEW_STATIC", "SDL_GPU_DISABLE_GLES"]
            extra_libs = ["GL" if target == "linux-x86_64" else "opengl32"]
        elif package == "SDL_Pango":
            origin = source(work, package)
            files = [origin / "src/SDL_Pango.c"]
            includes = [origin / "src"]
            if target == "linux-x86_64":
                extra_libs = pkg_flags("pangoft2")
            else:
                includes += [stage / "include/pango-1.0", stage / "include/glib-2.0", stage / "lib/glib-2.0/include",
                             stage / "include/freetype2", stage / "include/harfbuzz"]
                extra_libs = ["pangoft2-1.0", "pango-1.0", "gobject-2.0", "glib-2.0", "freetype"]
        elif package == "SDL_sound":
            origin = source(work, "SDL_sound1")
            files = [origin / name for name in ("SDL_sound.c", "audio_convert.c", "decoders/wav.c",
                     "decoders/aiff.c", "decoders/au.c", "decoders/raw.c", "decoders/voc.c", "decoders/shn.c")]
            includes = [origin]
            definitions = ["HAVE_ASSERT_H=1", "SOUND_SUPPORTS_WAV", "SOUND_SUPPORTS_AIFF", "SOUND_SUPPORTS_AU",
                           "SOUND_SUPPORTS_RAW", "SOUND_SUPPORTS_VOC", "SOUND_SUPPORTS_SHN", "DLL_EXPORT"]
        elif package == "SDL3_shadercross":
            origin = source(work, "SDL_shadercross")
            dependency = source(work, "SPIRV-Cross")
            if not (stage / "lib/libspirv-cross-c-shared.dll.a").exists() and target != "linux-x86_64" or \
               not (stage / "lib/libspirv-cross-c-shared.so").exists() and target == "linux-x86_64":
                dep_build = work / "build" / target / "SPIRV-Cross"
                dep_build.mkdir(parents=True, exist_ok=True)
                configure = ["cmake", "-S", str(dependency), "-B", str(dep_build), "-DCMAKE_BUILD_TYPE=Release",
                             "-DCMAKE_INSTALL_PREFIX=" + str(stage), "-DSPIRV_CROSS_SHARED=ON", "-DSPIRV_CROSS_STATIC=OFF",
                             "-DSPIRV_CROSS_CLI=OFF", "-DSPIRV_CROSS_ENABLE_TESTS=OFF"]
                if target != "linux-x86_64":
                    llvm = next((work / "toolchain/llvm-mingw").iterdir())
                    triple = PROFILES[target][0]
                    configure += ["-DCMAKE_SYSTEM_NAME=Windows", "-DCMAKE_C_COMPILER=" + str(llvm / "bin" / (triple + "-clang")),
                                  "-DCMAKE_CXX_COMPILER=" + str(llvm / "bin" / (triple + "-clang++"))]
                run(configure, environment, dep_build / "configure.log")
                run(["cmake", "--build", str(dep_build), "-j3"], environment, dep_build / "build.log")
                run(["cmake", "--install", str(dep_build)], environment, dep_build / "install.log")
            files = [origin / "src/SDL_shadercross.c"]
            includes = [origin / "include", dependency]
            definitions = ["DLL_EXPORT", "SDL_SHADERCROSS_DXC"]
            extra_libs = ["spirv-cross-c-shared", "dxcompiler"]
            if target != "linux-x86_64":
                # Build this C entry-point library with the same runtime as
                # its LLVM-built C++ dependency. Its public interface is C.
                llvm = next((work / "toolchain/llvm-mingw").iterdir())
                compiler = str(llvm / "bin" / (PROFILES[target][0] + "-clang"))
        else:
            raise ValueError("Unknown package: " + package)
        command = [compiler, "-shared", "-O2", "-fno-strict-aliasing"]
        command += ["-D" + define for define in definitions]
        for include in includes:
            command += ["-I", str(include)]
        if target == "linux-x86_64":
            command += ["-fPIC", "-I", str(stage / "include")]
            command += pkg_flags("sdl" if family == "SDL" else "sdl2") if family != "SDL3" else []
            command += [str(path) for path in files] + ["-L" + str(stage / "lib"), "-l" + family]
            for lib in extra_libs:
                command += [lib if lib.startswith("-") or lib.startswith("/") else "-l" + lib]
            command += ["-lm", "-Wl,-rpath," + str(stage / "lib")]
            library = stage / "lib" / ("lib" + package + ".so")
        else:
            command += ["-I", str(stage / "include"), "-I", str(stage / "include" / family)]
            if package.endswith("_FontCache"):
                command += ["-I", str(stage / "include" / family)]
            command += [str(path) for path in files] + ["-L" + str(stage / "lib"), "-l" + family]
            command += ["-l" + name for name in extra_libs]
            library = stage / "bin" / (package + ".dll")
            command += ["-Wl,--export-all-symbols", "-Wl,--out-implib," + str(stage / "lib" / ("lib" + package + ".dll.a"))]
        command += ["-o", str(library)]
        run(command, environment, directory / "build.log")
        record.update(status="built", library_path=str(library), log=str(directory / "build.log"))
    except Exception as error:
        record.update(status="failed", error=str(error), log=str(directory / "build.log"))
    return record


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--workdir", type=Path, default=ROOT / "out/sdl-addons")
    parser.add_argument("--targets", nargs="+", choices=("linux-x86_64", *PROFILES),
                        default=["linux-x86_64", *PROFILES])
    parser.add_argument("--select", help="Build only this library name")
    parser.add_argument("--jobs", type=int, default=4)
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error("--jobs must be positive")
    work = args.workdir.resolve()
    repair_sources(work)
    for target in args.targets:
        prepare(work, target)
    selected = [p for p in PACKAGES if not args.select or p == args.select]
    # Source-side FontCache uses the matching GPU backend on SDL1.
    ordinary = [p for p in selected if p != "SDL_FontCache"]
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        results = list(pool.map(lambda item: build(*item, work), [(p, t) for t in args.targets for p in ordinary]))
    if "SDL_FontCache" in selected:
        results += [build("SDL_FontCache", t, work) for t in args.targets]
    name = "build-results" + ("-" + args.select if args.select else "") + ".json"
    (work / name).write_text(json.dumps(results, indent=2) + "\n")
    failed = [r for r in results if r["status"] != "built"]
    print(json.dumps({"checks": len(results), "failed": failed}, indent=2))
    return bool(failed)


if __name__ == "__main__":
    raise SystemExit(main())

# End of build-sdl-addons.py
