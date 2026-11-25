#!/usr/bin/env python3
"""FreeBASIC JS SDK: prepare and build the third-party WebAssembly archives.

Package wrappers fetch the pinned source before packaging. The build command
uses the package's own Emscripten tools and works offline from that source.
This script does not build or install the FreeBASIC runtime.
"""

import argparse
import hashlib
import shutil
import subprocess
import tarfile
import tempfile
import urllib.request
import zipfile
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path


RAYLIB_COMMIT = "c1ab645ca298a2801097931d1079b10ff7eb9df8"
TILENGINE_COMMIT = "79895a48f0e316e2ba9086e8726d875efa315f6e"
RAYLIB_MODULES = ("rcore", "rshapes", "rtextures", "rtext", "rmodels", "raudio", "utils")
ZLIB_MODULES = ("adler32", "compress", "crc32", "deflate", "gzclose", "gzlib",
                "gzread", "gzwrite", "infback", "inffast", "inflate", "inftrees",
                "trees", "uncompr", "zutil")

# SDL2's source ships a complete SDL_config_emscripten.h. Select its
# browser drivers and generic threads as in Emscripten's SDL2 port.
SDL2_WEB_GROUPS = """.
    atomic audio audio/disk audio/dummy audio/emscripten cpuinfo dynapi
    events file filesystem/emscripten haptic haptic/dummy hidapi joystick
    joystick/emscripten loadso/dlopen locale locale/emscripten main/dummy
    misc misc/emscripten power power/emscripten render render/opengles2
    render/software sensor sensor/dummy stdlib thread thread/generic
    timer timer/unix video video/dummy video/emscripten video/yuv2rgb""".split()

# Pinned versions share the same source on Windows and Debian. Keep the
# unpacked trees separate because SDL2 and SDL3 export overlapping names.
SDL_URL = "https://codeload.github.com/libsdl-org/"
EXTRA_SOURCES = {
    "tilengine": ("Tilengine-" + TILENGINE_COMMIT,
                  "https://codeload.github.com/megamarc/Tilengine/tar.gz/" + TILENGINE_COMMIT,
                  "c01105f5eb3fcb27db68365dcca4b743914f7e14ddf371b8090167824b61348d"),
    "sdl2": ("SDL-release-2.32.10", SDL_URL + "SDL/tar.gz/refs/tags/release-2.32.10",
             "03f9d7c191a837525c9cda6406af2f2e48be02b5e7eb03d949cc9f1e9ca41c8b"),
    "sdl3": ("SDL-release-3.4.18", SDL_URL + "SDL/tar.gz/refs/tags/release-3.4.18",
             "c4b08b950bd29d83caae0ab8d884298d27046a9d30700fb42841c50992665c90"),
    "sdl2_image": ("SDL_image-release-2.8.8", SDL_URL + "SDL_image/tar.gz/refs/tags/release-2.8.8",
                   "2bd2c17cab3744ebfe7351ded8c70dc2078d8c7c24eb9f4ebb1b26a623e2a747"),
    "sdl2_ttf": ("SDL_ttf-release-2.24.0", SDL_URL + "SDL_ttf/tar.gz/refs/tags/release-2.24.0",
                 "2c45241a56203a59d66ec6b4eae9457e5675fc609376566a257391fd29d341a2"),
    "freetype": ("freetype-VER-2-14-3", "https://codeload.github.com/freetype/freetype/tar.gz/refs/tags/VER-2-14-3",
                 "dc49de6b01a266eef4876a4dd34d9842c475d3e28ff2eff63bd2fb760ab56261"),
    "sqlite": ("sqlite-amalgamation-3530400", "https://sqlite.org/2026/sqlite-amalgamation-3530400.zip",
               "1e71ddf93849c6a6ecf58b827c0692073d2dd7ee40196158068f7b29f422e87d"),
    "png": ("libpng-1.6.59", "https://codeload.github.com/pnggroup/libpng/tar.gz/refs/tags/v1.6.59",
            "2540302a1844ad2b2b501977abecfa850f265f97b78f065a712ab4074a89f5b5"),
    "jpeg": ("jpeg-9f", "https://storage.googleapis.com/webassembly/emscripten-ports/jpegsrc.v9f.tar.gz",
             "04705c110cb2469caa79fb71fba3d7bf834914706e9641a4589485c1f832565b"),
    "bzip2": ("bzip2-1.0.8", "https://sourceware.org/pub/bzip2/bzip2-1.0.8.tar.gz",
              "ab5a03176ee106d3f0fa90e381da478ddae405918153cca248e682cd0c4a2269"),
    "expat": ("libexpat-R_2_8_0", "https://codeload.github.com/libexpat/libexpat/tar.gz/refs/tags/R_2_8_0",
              "b6cfb7fee2d674587d94899d1f2dc039a7cbb65d167f0ba601d0b73a58f9db9e"),
}


def prepare_source(destination, prefix, url, expected_digest, selected_paths, local_tarball):
    destination = destination.resolve()
    destination.parent.mkdir(parents=True, exist_ok=True)
    if destination.exists():
        raise FileExistsError(f"source destination already exists: {destination}")
    with tempfile.TemporaryDirectory(prefix="fb-js-src-", dir=destination.parent) as work:
        temp = Path(work)
        tarball = local_tarball or temp / "source.tar.gz"
        if local_tarball is None:
            with urllib.request.urlopen(url, timeout=60) as response, tarball.open("wb") as output:
                shutil.copyfileobj(response, output)
        digest = hashlib.sha256()
        with tarball.open("rb") as data:
            for chunk in iter(lambda: data.read(1024 * 1024), b""):
                digest.update(chunk)
        if digest.hexdigest() != expected_digest:
            raise ValueError(f"{prefix} source checksum mismatch")
        def selected(name):
            if not name.startswith(prefix + "/"):
                return False
            if ".." in name.split("/") or "\\" in name:
                raise ValueError(f"unexpected archive entry: {name}")
            return selected_paths(name[len(prefix) + 1:])

        if zipfile.is_zipfile(tarball):
            with zipfile.ZipFile(tarball) as archive:
                archive.extractall(temp, [name for name in archive.namelist() if selected(name)])
        else:
            with tarfile.open(tarball) as archive:
                # Platform-specific framework symlinks are not needed for
                # the web build and are not portable to Windows packaging.
                members = [item for item in archive if selected(item.name)
                           and (item.isfile() or item.isdir())]
                archive.extractall(temp, members)
        (temp / prefix).rename(destination)


def prepare_raylib_audio(src, work):
    # This miniaudio revision assumes Emscripten exports the float heap on
    # Module. Newer Emscripten keeps it private unless explicitly exported.
    # EM_ASM already runs in that scope and can read the live heap directly.
    header = (src / "external/miniaudio.h").read_text(encoding="utf-8")
    old = "new Float32Array(Module.HEAPF32.buffer,"
    if header.count(old) != 1:
        raise ValueError("raylib miniaudio heap reference did not match the pinned source")
    patched = work / "external/miniaudio.h"
    patched.parent.mkdir()
    patched.write_text("/* FreeBASIC web build: use Emscripten's internal heap view. */\n"
                       + header.replace(old, "new Float32Array(HEAPF32.buffer,"),
                       encoding="utf-8", newline="\n")
    source = work / "raudio.c"
    shutil.copyfile(src / source.name, source)
    return source


def build_archive(source, destination, relative_src, modules, flags, name, license_name, emcc, emar,
                  *, license_path="LICENSE", config_header=None, jobs=1):
    src = source.resolve() / relative_src
    if not modules:
        raise ValueError(f"no source modules for {name}")
    destination.mkdir(parents=True, exist_ok=True)
    # Stage into a temporary directory on the destination volume. An
    # interrupted compilation cannot replace a complete installed archive.
    with tempfile.TemporaryDirectory(prefix=name + "-", dir=destination) as work:
        temp = Path(work).resolve()
        if config_header:
            header_path, header_name = config_header
            shutil.copyfile(source / header_path, temp / header_name)
        def compile_module(module):
            obj = temp / (module.replace("/", "_") + ".o")
            c_source = src / (module + ".c")
            if name == "libraylib.a" and module == "raudio":
                c_source = prepare_raylib_audio(src, temp)
            subprocess.run([emcc, "-c", str(c_source), "-o", str(obj),
                            "-O2", "-I", str(src), "-I", str(temp), *flags], check=True)
            with obj.open("rb") as data:
                if data.read(4) != b"\x00asm":
                    raise ValueError(f"{module} was compiled for a foreign target")
            return str(obj)

        with ThreadPoolExecutor(max_workers=jobs) as pool:
            objects = list(pool.map(compile_module, modules))
        archive = temp / name
        # Use a response file so archive member lists still fit when emar
        # is started through a Windows batch launcher.
        response = temp / "members.rsp"
        response.write_text("".join(Path(obj).name + "\n" for obj in objects), encoding="ascii")
        subprocess.run([emar, "rcs", str(archive), "@members.rsp"], cwd=temp, check=True)
        verify_wasm_archive(archive)
        if license_path is None:
            (temp / license_name).write_text("SQLite is in the public domain.\n"
                                            "https://sqlite.org/copyright.html\n", encoding="ascii")
        else:
            shutil.copyfile(source / license_path, temp / license_name)
        (temp / license_name).replace(destination / license_name)
        archive.replace(destination / archive.name)
    print(f"Built {name} WebAssembly archive", flush=True)


def prepare_extras(directory, downloads, only):
    for name in only or EXTRA_SOURCES:
        prefix, url, digest = EXTRA_SOURCES[name]
        destination = directory / name
        if destination.is_dir():
            continue
        local_tarball = downloads / name if downloads else None
        # Exclude platform projects, binaries, and upstream test data from
        # Debian source packaging. Keep the files CMake and the C build use.
        if name in ("sdl2", "sdl3"):
            web_paths = lambda path: "/" not in path or path.startswith(("include/", "src/", "cmake/"))
        elif name == "tilengine":
            web_paths = lambda path: path == "LICENSE" or path.startswith(("include/", "src/"))
        elif name == "sdl2_image":
            web_paths = lambda path: path == "LICENSE.txt" or path.startswith(("src/", "include/"))
        elif name == "sdl2_ttf":
            web_paths = lambda path: path in ("LICENSE.txt", "SDL_ttf.c", "SDL_ttf.h")
        elif name == "expat":
            web_paths = lambda path: path.startswith(("expat/lib/", "expat/cmake/")) or path in (
                "expat/CMakeLists.txt", "expat/ConfigureChecks.cmake", "expat/expat_config.h.cmake", "expat/COPYING",
                "expat/expat.pc.cmake", "expat/Changes")
        elif name == "freetype":
            web_paths = lambda path: "/" not in path or path.startswith(("src/", "include/", "builds/", "docs/"))
        elif name == "png":
            web_paths = lambda path: ("/" not in path and Path(path).suffix in (".c", ".h")) or path in (
                "LICENSE", "scripts/pnglibconf.h.prebuilt")
        else:
            web_paths = lambda path: "/" not in path and (path in ("LICENSE", "README", "jconfig.txt") or
                                                          Path(path).suffix in (".c", ".h"))
        prepare_source(destination, prefix, url, digest, web_paths, local_tarball)


def verify_wasm_archive(archive):
    members = 0
    archive_size = archive.stat().st_size
    with archive.open("rb") as data:
        if data.read(8) != b"!<arch>\n":
            raise ValueError(f"invalid archive: {archive}")
        while header := data.read(60):
            if len(header) != 60 or header[58:] != b"`\n":
                raise ValueError(f"invalid archive member: {archive}")
            name = header[:16].strip()
            size = int(header[48:58].strip())
            member_end = data.tell() + size + size % 2
            if size < 0 or member_end > archive_size:
                raise ValueError(f"truncated archive member: {archive}")
            if name not in (b"/", b"//", b"/SYM64/"):
                if size < 4 or data.read(4) != b"\x00asm":
                    raise ValueError(f"non-WebAssembly member {name!r} in {archive}")
                members += 1
            data.seek(member_end)
    if not members:
        raise ValueError(f"no WebAssembly members: {archive}")


def build_cmake_archive(source, destination, archive_name, license_path, license_name, options, args):
    with tempfile.TemporaryDirectory(prefix=archive_name + "-", dir=destination) as work:
        build = Path(work).resolve()
        subprocess.run([args.emcmake, args.cmake, "-G", "Ninja", "-S", str(source), "-B", str(build),
                        "-DCMAKE_BUILD_TYPE=Release", *options], check=True)
        subprocess.run([args.cmake, "--build", str(build), "--parallel", str(args.jobs)], check=True)
        archive = build / archive_name
        verify_wasm_archive(archive)
        shutil.copyfile(source / license_path, build / license_name)
        (build / license_name).replace(destination / license_name)
        archive.replace(destination / archive_name)
    print(f"Built {archive_name} WebAssembly archive", flush=True)


def build_extras(args):
    source = args.source.resolve()
    destination = args.destination.resolve()
    destination.mkdir(parents=True, exist_ok=True)
    selected = args.only or EXTRA_SOURCES
    sdl_options = ["-DSDL_STATIC=ON", "-DSDL_SHARED=OFF", "-DSDL_TEST_LIBRARY=OFF",
                   "-DSDL_TESTS=OFF", "-DSDL_EXAMPLES=OFF", "-DSDL_INSTALL=OFF"]
    def direct(name, relative_src, modules, flags, library, license_name, **kwargs):
        build_archive(source / name, destination, relative_src, modules, flags, library,
                      license_name, args.emcc, args.emar, jobs=args.jobs, **kwargs)

    for name in selected:
        if name == "tilengine":
            modules = sorted(path.stem for path in (source / name / "src").glob("*.c") if path.stem != "Test")
            # Tilengine's source includes SDL2/SDL.h. The SDK builds SDL and
            # libpng from pinned sources, so reuse those headers without
            # fetching Emscripten's ports at package build time.
            with tempfile.TemporaryDirectory(prefix="tilengine-headers-", dir=destination) as work:
                include = Path(work)
                shutil.copytree(source / "sdl2/include", include / "SDL2")
                shutil.copyfile(source / "png/scripts/pnglibconf.h.prebuilt", include / "pnglibconf.h")
                direct(name, "src", modules, ["-std=gnu99", "-I", str(source / name / "include"),
                                               "-I", str(include), "-I", str(source / "png"),
                                               "-I", str(args.zlib_source.resolve())],
                       "libTilengine.a", "TILENGINE-LICENSE")
            (destination / "TILENGINE-SOURCE").write_text(
                "Tilengine source used for this archive:\n"
                "https://github.com/megamarc/Tilengine/tree/" + TILENGINE_COMMIT + "\n", encoding="ascii")
        elif name == "sdl2":
            src = source / name / "src"
            modules = [(path.relative_to(src).with_suffix("")).as_posix()
                       for group in SDL2_WEB_GROUPS for path in sorted((src / group).glob("*.c"))]
            direct(name, "src", modules, ["-I", str(source / name / "include")],
                   "libSDL2.a", "SDL2-LICENSE", license_path="LICENSE.txt")
        elif name == "sdl3":
            build_cmake_archive(source / name, destination, "libSDL3.a", "LICENSE.txt",
                                "SDL3-LICENSE", sdl_options, args)
        elif name == "sdl2_image":
            modules = sorted(path.stem for path in (source / name / "src").glob("IMG*.c"))
            direct(name, "src", modules,
                   ["-I", str(source / name / "include"), "-I", str(source / "sdl2/include"),
                    "-DSDL_IMAGE_USE_COMMON_BACKEND", "-DUSE_STBIMAGE",
                    *["-DLOAD_" + codec for codec in ("BMP", "GIF", "JPG", "LBM", "PCX", "PNG", "PNM",
                                                      "QOI", "SVG", "TGA", "XCF", "XPM", "XV")]],
                   "libSDL2_image.a", "SDL2-IMAGE-LICENSE", license_path="LICENSE.txt")
        elif name == "sdl2_ttf":
            direct(name, ".", ["SDL_ttf"], ["-I", str(source / "sdl2/include"),
                                            "-I", str(source / "freetype/include")],
                   "libSDL2_ttf.a", "SDL2-TTF-LICENSE", license_path="LICENSE.txt")
        elif name == "freetype":
            build_cmake_archive(source / name, destination, "libfreetype.a", "docs/FTL.TXT",
                                "FREETYPE-LICENSE", ["-DBUILD_SHARED_LIBS=OFF"] +
                                ["-DFT_DISABLE_" + dep + "=ON" for dep in
                                 ("ZLIB", "BZIP2", "PNG", "HARFBUZZ", "BROTLI")], args)
        elif name == "sqlite":
            direct(name, ".", ["sqlite3"], ["-DSQLITE_THREADSAFE=0", "-DSQLITE_OMIT_LOAD_EXTENSION"],
                   "libsqlite3.a", "SQLITE-LICENSE", license_path=None)
        elif name == "png":
            modules = sorted(path.stem for path in (source / name).glob("png*.c")
                             if path.stem != "pngtest")
            direct(name, ".", modules, ["-I", str(args.zlib_source.resolve())], "libpng.a",
                   "LIBPNG-LICENSE", config_header=("scripts/pnglibconf.h.prebuilt", "pnglibconf.h"))
        elif name == "jpeg":
            # The IJG makefile lists the library sources apart from tools
            # and platform-specific memory managers. Use the no-backing-store
            # manager for the browser, which has no native temporary files.
            modules = """jaricom jcapimin jcapistd jcarith jccoefct jccolor jcdctmgr
                jchuff jcinit jcmainct jcmarker jcmaster jcomapi jcparam jcprepct
                jcsample jctrans jdapimin jdapistd jdarith jdatadst jdatasrc
                jdcoefct jdcolor jddctmgr jdhuff jdinput jdmainct jdmarker
                jdmaster jdmerge jdpostct jdsample jdtrans jerror jfdctflt
                jfdctfst jfdctint jidctflt jidctfst jidctint jquant1 jquant2
                jutils jmemmgr jmemnobs""".split()
            direct(name, ".", modules, [], "libjpeg.a", "LIBJPEG-LICENSE",
                   license_path="README", config_header=("jconfig.txt", "jconfig.h"))
        elif name == "bzip2":
            direct(name, ".", "blocksort huffman crctable randtable compress decompress bzlib".split(),
                   [], "libbz2.a", "BZIP2-LICENSE")
        elif name == "expat":
            build_cmake_archive(source / name / "expat", destination, "libexpat.a", "COPYING",
                                "EXPAT-LICENSE", ["-DEXPAT_" + feature + "=OFF" for feature in
                                ("SHARED_LIBS", "BUILD_TOOLS", "BUILD_TESTS", "BUILD_EXAMPLES", "BUILD_DOCS",
                                 "ENABLE_INSTALL")], args)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    prepare = commands.add_parser("prepare")
    prepare.add_argument("source", type=Path)
    prepare.add_argument("--tarball", type=Path)
    build = commands.add_parser("build")
    build.add_argument("source", type=Path)
    build.add_argument("destination", type=Path)
    build.add_argument("--emcc", default="emcc")
    build.add_argument("--emar", default="emar")
    for command in (prepare, build):
        command.add_argument("--library", choices=("raylib", "zlib"), default="raylib")
    extra_prepare = commands.add_parser("prepare-extras")
    extra_prepare.add_argument("source", type=Path)
    extra_prepare.add_argument("--downloads", type=Path)
    extra_build = commands.add_parser("build-extras")
    extra_build.add_argument("source", type=Path)
    extra_build.add_argument("destination", type=Path)
    extra_build.add_argument("--zlib-source", required=True, type=Path)
    extra_build.add_argument("--emcc", default="emcc")
    extra_build.add_argument("--emar", default="emar")
    extra_build.add_argument("--emcmake", default="emcmake")
    extra_build.add_argument("--cmake", default="cmake")
    extra_build.add_argument("--jobs", type=int, default=2)
    for command in (extra_prepare, extra_build):
        command.add_argument("--only", choices=EXTRA_SOURCES, action="append")
    args = parser.parse_args()
    if args.command == "prepare-extras":
        prepare_extras(args.source, args.downloads, args.only)
    elif args.command == "build-extras":
        if args.jobs < 1:
            parser.error("--jobs must be positive")
        build_extras(args)
    elif args.command == "prepare":
        if args.library == "raylib":
            prepare_source(args.source, "raylib-" + RAYLIB_COMMIT,
                           "https://codeload.github.com/raysan5/raylib/tar.gz/" + RAYLIB_COMMIT,
                           "be3443c3fb97f7cbd18ae3f0ea2a73e6c1486128df33b639dce7e44070fc285d",
                           lambda path: path.startswith("src/") or path in
                           ("LICENSE", "parser/output/raylib_api.json"), args.tarball)
        else:
            prepare_source(args.source, "zlib-1.4", "https://zlib.net/fossils/zlib-1.4.tar.gz",
                           "02dd159783677a749c04adbdd02b55b8a2bcc7e054f51b76b549199a916ad584",
                           lambda path: "/" not in path and
                           (path == "LICENSE" or Path(path).suffix in (".c", ".h")), args.tarball)
    elif args.library == "raylib":
        build_archive(args.source, args.destination, "src", RAYLIB_MODULES,
                      ["-DPLATFORM_WEB", "-DGRAPHICS_API_OPENGL_ES2",
                       "-I", str(args.source.resolve() / "src/external/glfw/include")],
                      "libraylib.a", "RAYLIB-LICENSE", args.emcc, args.emar)
    else:
        build_archive(args.source, args.destination, ".", ZLIB_MODULES, ["-DHAVE_UNISTD_H"],
                      "libz.a", "ZLIB-LICENSE", args.emcc, args.emar)

# end of build-js-libraries.py
