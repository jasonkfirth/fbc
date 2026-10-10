Project: FreeBASIC SDL3 examples
File: readme.txt
Purpose: Describe the libraries, build commands, and validation scope.
Responsibilities: Explain startup, assets, source provenance, and compatibility.
This file intentionally does NOT contain: SDL implementations or compiler build internals.

SDL3 and companion libraries
---------------------------

These examples use SDL3's public C interfaces. SDL3 does not require C++ language
support in the BASIC compiler. The bindings are under inc/SDL3 and coexist with
the existing inc/SDL and inc/SDL2 directories. Use one SDL generation in each
translation unit, since the upstream libraries reuse type and function names.

The pinned source releases are:

    SDL3       3.4.18       SDL3_image  3.4.8
    SDL3_mixer 3.2.4        SDL3_ttf    3.2.2
    SDL3_net   3.2.0        SDL3_sound  3.2.0

SDL3_sound decodes sound files. SDL3_mixer manages mixing and playback tracks.
They provide different APIs, and both are included. The GPU and Vulkan APIs are
also available, along with SDL_test and the TTF text-engine interface.

Companion examples also live in the gfx, bgi, rtf, and shadercross
subdirectories. Their helpers and SDK instructions are under
../SDL_common. Check those examples with test-sdl-addons.py --select SDL3;
test-sdl3-examples.py checks the 60 upstream translation units described below.

Build the native libraries from the FreeBASIC repository root:

    python3 build_scripts/build-sdl3.py --jobs 6

This verifies SHA-256 hashes of complete upstream release archives and installs
under out/sdl3/prefix. It requires Python 3.12 or newer, CMake, make or Ninja, and
a native C/C++ toolchain. SDL_image, SDL_mixer, and SDL_ttf use available system
development libraries. Their configure logs list enabled formats and backends.
The builder does not replace system libraries or change the compiler build.

The archive, unpacked source, notices, and build logs remain under out/sdl3.
To fetch and verify source without compiling it, add --download-only.

Building examples
-----------------

For a simple example, run from the repository root:

    bin/fbc -mt -i inc -i examples/graphics/SDL3 -p out/sdl3/prefix/lib \
        examples/graphics/SDL3/core/renderer/02-primitives/primitives.bas \
        -x out/sdl3/primitives

On Linux, put out/sdl3/prefix/lib on LD_LIBRARY_PATH when running the executable.
On other systems, use the platform's normal library search path. Windows needs
matching SDL3 DLLs and import libraries for the executable's architecture.

The callback examples use callback-main.bi. It passes FreeBASIC's command-line
arguments through SDL_RunApp and SDL_EnterAppMainCallbacks, retaining SDL's event
loop and cleanup sequence. Ordinary examples continue to use their own main loop.

TTF showfont links the editbox helper as a second module:

    bin/fbc -mt -i inc -i examples/graphics/SDL3 -p out/sdl3/prefix/lib \
        examples/graphics/SDL3/ttf/showfont.bas \
        examples/graphics/SDL3/ttf/editbox.bas -m showfont -x out/sdl3/showfont

Examples that load data need the upstream PNG, WAV, MP3, and hex files beside
their executable. The example runner stages these files from the downloaded
sources. TTF examples accept a font filename. The testapp retains its original
font collection; FB_SDL3_TEST_FONT lets the runner supply one known local font.
No private font collection is included in the repository.

Repeatable checks
-----------------

    python3 build_scripts/test-sdl3-bindings.py
    xvfb-run -a python3 build_scripts/test-sdl3-examples.py

For a fresh private X server per executable, run without an outer xvfb-run:

    python3 build_scripts/test-sdl3-examples.py --private-x --backend gcc \
        --frames 32 --output out/sdl3/private-x-gcc
    python3 build_scripts/test-sdl3-examples.py --private-x --backend gas64 \
        --frames 32 --output out/sdl3/private-x-gas64

This mode requires Xvfb, xauth, and xdpyinfo. Each example gets its own
authenticated X server, forced SDL X11 driver, readiness check, and cleanup.
The report records the display and server PID. The camera example is attempted
and must deliver a frame; parallel checks serialize access to the V4L device.
Its bounded check exits through a quit event after presenting that frame.

To compile every example with each native backend and retain separate reports:

    python3 build_scripts/test-sdl3-examples.py --compile-only --backend gcc \
        --output out/sdl3/examples-gcc
    python3 build_scripts/test-sdl3-examples.py --compile-only --backend gas64 \
        --output out/sdl3/examples-gas64

The binding check compares C and BASIC structure sizes, alignment, field offsets,
and enum values, then tests native booleans, callbacks, structure arguments and
returns, threads, I/O, decoding, and font rendering with GCC and gas64. Header
emission is checked for Linux x86/x86-64/ARM/AArch64 and Windows x86/x86-64.
Android, iOS, and GDK signatures receive separate declaration checks.
Cross-target emission does not claim that those target binaries were executed.

The example check compiles all 60 upstream example translation units, including
the editbox helper. FB_SDL3_SMOKE_FRAMES bounds graphical runs through callback
completion or the normal quit-event path. The interactive TTF testapp quits after
one draw because it waits for input between draws. Audio uses SDL's dummy driver.
Physical camera capture remains an explicitly reported hardware check.
The networking checks create bounded
TCP, UDP, HTTP, and VoIP peers on 127.0.0.1; they do not contact public services.
The --compile-only and --select options support narrower development runs.

The existing exampleageddon runner understands the shared helpers and showfont's
second module. After building the local libraries, supply their search path:

    python3 build_scripts/exampleageddon-freebasic.py --no-prefix \
        --fbc-arg=-p --fbc-arg="$PWD/out/sdl3/prefix/lib"

Use an absolute library path in that command, since the harness compiles in an
isolated working directory. The SDL3 example runner resolves its own paths automatically.

Binding details and provenance
------------------------------

The declarations follow the existing SDL binding layout. SDL.bi owns the common
API; individual SDL_*.bi entry points include it. Optional Vulkan and SDL_test
interfaces remain separate. SDL_platform_api.bi retains Android, iOS, and GDK
declarations; those APIs require the matching native platform and SDL build.
Shader bytecode belongs to the GPU text example, not to the compiler or runtime.

FreeBASIC identifiers are case-insensitive. The following C names are adjusted:

    SDL_Log          -> SDL_Log_       (SDL_log remains the math function)
    SDL_PixelType    -> SDL_PixelType_ (SDL_PIXELTYPE remains the format macro)
    SDL_PRIX32       -> SDL_PRIX32_
    SDL_PRIX64       -> SDL_PRIX64_
    SDL_PRILLX       -> SDL_PRILLX_
    SDL_KeyboardEvent.mod -> mod_

C bool maps to the one-byte BASIC boolean type. C int maps to long, not the
pointer-sized integer type. Pointer sizes, 64-bit flags, byte order aliases,
and Windows CRT thread entry points are handled explicitly in the binding.
The 64-bit format macros match BASIC LongInt/ULongInt variadic arguments,
which the C backend represents as long long, including on LP64 systems.

FreeBASIC SizeOf(array) describes one element. The converted examples use the
complete C byte counts, and SDL_arraysize/SDL_zeroa use BASIC bounds for their
one-dimensional arrays. The compiler's existing SizeOf behavior is unchanged.

Regenerate declarations, using a locally built fbfrog:

    python3 build_scripts/generate-sdl3-bindings.py --fbfrog /path/to/fbfrog

SDL_inline.bi supplies BASIC versions of C inline helpers. Compiler-specific C
annotations, SIMD intrinsics, and ELF note construction are not BASIC interfaces.
OpenGL APIs use FreeBASIC's existing GL bindings.

The small SDL_*.bi entry points forward to SDL.bi. Their corresponding C headers
usually contain declarations; SDL_bits.h, SDL_endian.h, SDL_rect.h, and
SDL_stdinc.h also contain inline implementations. All 16 public inline helpers
are implemented in SDL_inline.bi with C calling conventions and addressable
functions. SDL's two compiler-builtin overflow variants are implementation
details; the portable entry points preserve their documented contract.
The binding checker inventories the upstream bodies, checks helper addresses on
every target profile, and compares their results against separately compiled C.

The audio byte/frame counts and version components keep the C interface's
32-bit result widths. Nanosecond conversions use integer division, including
for values beyond Double's exact range.

The native gas64 backend needs the compiler fixes included with these examples:
correct division register names, preservation of a live dividend, stable queued
stack arguments, signed immediate encoding for negative Single arguments, and
unique result-register mappings when IR reuses an operand's virtual ID.
The compiler regression tests cover the affected calls without SDL dependencies.

The examples are translations of the pinned upstream examples. Their notices
or public-domain statements remain in the files. Source and decoder licenses
remain in the upstream archives. No upstream C source has been modified.

end of readme.txt
