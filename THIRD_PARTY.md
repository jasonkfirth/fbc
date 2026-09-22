<!--
    Project: FreeBASIC
    -------------------

    File: THIRD_PARTY.md

    Purpose:

        Record third-party software, license notices, and external build
        dependencies associated with the FreeBASIC source tree.

    Responsibilities:

        - identify third-party implementation code distributed in this tree
        - point to the complete license text kept with each component
        - distinguish bundled code from optional external libraries and tools

    This file intentionally does NOT contain:

        - the full FreeBASIC GPL, LGPL, or FDL license texts
        - a replacement for notices preserved in individual source files
        - license terms for libraries installed separately by a user
-->

# Third-party software and license notices

This file records third-party software that is distributed with, built for,
or referenced by FreeBASIC. It is an inventory and navigation aid. The
copyright and license notices in the referenced source files are authoritative
and must remain with those files.

FreeBASIC's own licensing is described in [`readme.txt`](readme.txt). In
summary, the compiler is GPLv2 or later, the runtime and graphics libraries
are LGPLv2 or later with a static-linking exception, and the documentation is
GNU FDL. Those project licenses are not repeated here.

## Bundled third-party implementation code

These components contain implementation code in the FreeBASIC source tree.

| Component | Version in this tree | Use | License and notice |
| --- | --- | --- | --- |
| `dr_mp3` by David Reid | 0.7.4 | MP3 decoding in `sfxlib` | Public domain or MIT-0, as stated at the end of [`dr_mp3.h`](src/sfxlib/third_party/dr_mp3.h). The file also preserves the `minimp3` attribution and public-domain notice for the portions derived from it. |
| `dr_wav` by David Reid | 0.14.6 | WAV loading and writing in `sfxlib` | Public domain or MIT-0, as stated at the end of [`dr_wav.h`](src/sfxlib/third_party/dr_wav.h). |
| `stb_vorbis` by Sean Barrett and contributors | 1.22 | Ogg Vorbis decoding in `sfxlib` | MIT License or public domain, as stated at the end of [`stb_vorbis.c`](src/sfxlib/third_party/stb_vorbis.c). |
| PDMLWP by Paolo De Marino, based on work by Sengan Short and Josh Turpen | DOS contribution | Pre-emptive lightweight threads for DOS | GNU Library General Public License v2 with the project-specific credit exception described in [`copying`](contrib/dos/pdmlwp/copying) and [`copying.lib`](contrib/dos/pdmlwp/copying.lib). The required credits are listed in [`thanks`](contrib/dos/pdmlwp/thanks). |

The decoder source files above are used by the platform-specific `sfxlib`
implementations. Their upstream license blocks are deliberately kept in the
source files rather than paraphrased here.

## libffi

The FreeBASIC runtime uses **libffi** for `Threadcall`. Depending on the
target and package, libffi may be supplied by the host system or downloaded
and built by a packaging script. The libffi implementation is not vendored in
the main source tree, but this repository carries its license notice in
[`doc/libffi-license.txt`](doc/libffi-license.txt).

libffi is distributed under the MIT/Expat license. When a binary package
includes a libffi archive, headers, or library, that package must include the
corresponding libffi notice.

Upstream project: [sourceware.org/libffi](https://sourceware.org/libffi/)

## FreeBASIC bindings for external libraries

The `inc/` directory contains FreeBASIC declarations translated from many
external C APIs. These are interface bindings, not implementations of the
external libraries. A binding does not grant permission to redistribute the
corresponding library binary.

The binding files retain the upstream copyright and license notices where
those notices were supplied by the original project. In particular, consult
the following local notices when redistributing the related bindings:

| Binding or API family | Notice location |
| --- | --- |
| FLTK | [`inc/FLTK/LICENSE`](inc/FLTK/LICENSE) and the headers under [`inc/FLTK/`](inc/FLTK/) |
| libpng | The notices in [`inc/png.bi`](inc/png.bi), [`inc/png12.bi`](inc/png12.bi), [`inc/png14.bi`](inc/png14.bi), [`inc/png15.bi`](inc/png15.bi), and [`inc/png16.bi`](inc/png16.bi) |
| PCRE and PCRE2 | The notices in [`inc/pcre-common.bi`](inc/pcre-common.bi), [`inc/pcreposix.bi`](inc/pcreposix.bi), [`inc/pcre2.bi`](inc/pcre2.bi), and [`inc/pcre2posix.bi`](inc/pcre2posix.bi) |
| QuickLZ | The license and commercial-use terms at the top of [`inc/quicklz.bi`](inc/quicklz.bi) |
| raylib and raymath | The zlib/libpng license in [`inc/raylib.bi`](inc/raylib.bi) and [`inc/raymath.bi`](inc/raymath.bi) |
| PortAudio | The notice at the top of [`inc/portaudio.bi`](inc/portaudio.bi) |
| PCG32 code used by the built-in PRNG bindings | The Apache 2.0 notice in [`inc/fbprng.bi`](inc/fbprng.bi) |

The remaining binding directories follow the same rule. Each `.bi` file must
be read as the primary notice for the API it declares. This includes, among
others, Allegro, GTK, OpenAL, SDL, OpenGL, X11, XCB, FreeType, Cairo, GSL,
IUP, libxml2, libxslt, Lua, ODE, VLC, wxWidgets, ZeroMQ, and platform SDK
bindings.

## External build and platform tools

The FreeBASIC build and release documentation names the following external
tools and platform libraries. They are normally obtained separately and are
not relicensed by FreeBASIC:

| Tool or library | Project |
| --- | --- |
| DJGPP | [delorie.com](https://www.delorie.com/) |
| GCC | [gcc.gnu.org](https://gcc.gnu.org/) |
| GNU binutils | [gnu.org/software/binutils](https://www.gnu.org/software/binutils/) |
| GNU debugger | [gnu.org/software/gdb](https://www.gnu.org/software/gdb/) |
| GoRC | [godevtool.com](http://godevtool.com/) |
| MinGW | [osdn.net/projects/mingw](https://osdn.net/projects/mingw/) |
| MinGW-w64 | [mingw-w64.org](https://mingw-w64.org/) |
| OpenXDK | [openxdk.sourceforge.net](https://openxdk.sourceforge.net/) |
| TDM-GCC | [jmeubank.github.io/tdm-gcc](https://jmeubank.github.io/tdm-gcc/) |
| WinLibs | [winlibs.com](https://www.winlibs.com/) |

When a standalone package bundles one of these tools or a target SDK, the
package maintainer must preserve that toolchain's own license and attribution
files. The terms can differ between a compiler, its runtime libraries, its
headers, and a particular binary distribution.

## Distribution checklist

When publishing a source or binary package containing FreeBASIC:

1. Keep the license and attribution blocks in the bundled third-party source
   files.
2. Include [`doc/libffi-license.txt`](doc/libffi-license.txt) when libffi is
   included in the package.
3. Keep the notices for any external binding that is copied into the package.
4. Include the upstream license files for any toolchain or system library
   that is bundled rather than installed separately.
5. Do not describe an optional external library as part of the FreeBASIC
   license unless its own license explicitly permits that treatment.

<!-- end of THIRD_PARTY.md -->
