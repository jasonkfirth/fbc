Project: FreeBASIC SDL examples
File: readme.txt
Purpose: Describe the generation directories, shared helpers, and local checks.
Responsibilities: Give build and execution commands and explain resource ownership.
This file intentionally does NOT contain: a replacement for upstream documentation.

The examples cover SDL_rtf on SDL1, SDL2, and SDL3; SDL_gpu on SDL1 and
SDL2; SDL3_gfx; SDL_bgi on SDL2 and SDL3; SDL_Pango; SDL_FontCache on
SDL1/SDL_gpu and SDL2; SDL_sound on SDL1 and SDL2; and SDL_shadercross.
The SDL2 image2, mixer2, net2, ttf2, and gfx2 examples provide direct checks
for the existing image, mixer, networking, TrueType, and gfx bindings.

Executable examples live under ../SDL1, ../SDL2, and ../SDL3. Each
generation keeps core examples in core and companion examples in the
corresponding image, mixer, net, ttf, sound, gfx, gpu, rtf, bgi, pango,
fontcache, or shadercross subdirectory. This directory owns only the
helpers shared by those companion examples and this documentation.

The old core includes remain unchanged. BGI functions live in the bgi
namespace because names such as line and circle overlap with BASIC commands.
The SOUND_VERSION macro is named SOUND_VERSION_ to avoid FreeBASIC's
case-insensitive collision with Sound_Version.

Windows libraries
-----------------

Windows x86 files are in lib/win32, Windows x86_64 files in lib/win64,
and Windows ARM64 files in lib/win32-aarch64. The .dll.a files import real
DLL exports; they are not self-contained static implementations. Copy the
matching DLLs and their dependencies from sdl-dlls beside an application
executable. Each target has sdl-libraries.json with exact hashes and
sdl-licenses with upstream notices. Upstream-provided static .a files are
retained alongside the import archives where supplied.
Manifest archive and DLL paths are relative to that target's SDK directory.
Dependency import archives are installed too, including the text libraries
selected by the Pango and FreeType includes. sdl-dependencies.json records
the checked DLL import closure and the remaining Windows system imports.

Shadercross includes its SPIRV-Cross and DXC backends. Its example checks
reflection, SPIR-V-to-HLSL translation, and HLSL compilation back to SPIR-V.
The GPU example uses a real OpenGL context. The sound example checks WAV
decoding and rewind. The RTF font callbacks remain alive until the document
context is freed; its fonts are closed before TTF_Quit.

Local checks
------------

  python3 build_scripts/test-sdl-addons.py

This builds every example using GCC and GAS64 and gives each executable a
fresh authenticated Xvfb server. The runner checks visible output, normal
exit, decoded audio, and shader metadata. It sets FB_SDL_ADDON_FRAMES for
bounded runs; without that variable graphical examples remain interactive.

Pass a font file to fontcache1/fontcache2 or ttf2. Pass an RTF document and font
file to rtf1/rtf2/rtf3. Pass a WAV file to sound1/sound2 and a SPIR-V vertex
shader to shadercross3. The runner stages controlled fixtures for these cases.
image2 takes a PNG file; mixer2 takes a short WAV file. net2 sends one UDP
packet to its own ephemeral loopback port and checks the received bytes.
The BGI examples select compatible mode before initialization and batch
drawing before refresh(), keeping SDL rendering on the main thread.

To recreate the addon SDK after preparing the core SDKs:

  python3 build_scripts/fetch-sdl-addons.py
  python3 build_scripts/build-sdl-addons.py
  python3 build_scripts/generate-sdl-addon-bindings.py
  python3 build_scripts/install-sdl-addon-libraries.py

fetch-sdl-addons.py verifies the recorded source and dependency archives.
build-sdl-addons.py builds the focused C libraries in isolated directories
and derives the ARM64 core imports from the real upstream DLL exports.
generate-sdl-addon-bindings.py requires fbfrog; pass --fbfrog if necessary.
install-sdl-addon-libraries.py verifies archive and DLL machine types and
the transitive runtime dependencies before recording the installed SDK.
These steps also require the previously prepared SDL core SDKs; they do not
install system packages. Native codec/text dependencies must be available
through pkg-config. Native builds need SDL1, SDL2, SDL3, Pango, and OpenGL.

The Windows build checker accepts --addons, --sdk32, --sdk64,
--arm-toolchain, and --sdk-arm. Use a Linux-hosted LLVM-MinGW toolchain for
ARM64 and a prefix containing FreeBASIC's matching target runtime. The
prepared prefix is out/sdl-windows/fb-prefix. For the installed libraries:

  python3 build_scripts/test-sdl-windows.py --minimum \
    --sdk32 lib/win32 --sdk64 lib/win64 --sdk-arm lib/win32-aarch64 \
    --arm-toolchain out/sdl-addons/toolchain/llvm-mingw/llvm-mingw-20261006-ucrt-ubuntu-22.04-x86_64

--minimum links one example for every packaged SDL1, SDL2, and SDL3 library
and compiles every SDL header. --addon-only selects the 19 companion
examples. --select SDL1, --select SDL2, and --select SDL3 restrict paths to
one generation in the native companion runner or the Windows checker.

Windows checks produce and inspect executables; they do not execute them
on Windows. ARM64 uses the Clang backend; x86 and x86_64 use both GCC and
the corresponding assembler backend.
The prepared ARM64 runtime was built without libffi/Threadcall. Ordinary
SDL C calls, callbacks, and SDL thread APIs do not require Threadcall.

The SDL1 sound build enables WAV, AIFF, AU, RAW, VOC, and SHN decoders.
Its optional external codec implementations are not enabled by this build.
These examples are integration checks for the public APIs, rather than a
translation of every upstream test program.

Source repairs are confined to isolated upstream builds: SDL_gpu's ARM64
padding selection and one callback's const qualification; its bundled GLEW
string-copy allocation; SDL1 FontCache surface-alpha calls; and SDL_Pango's
modern FreeType include guard. The original downloaded archives are retained.

Minimum example coverage
------------------------

The table gives one local example for every packaged library variant.
Paths are relative to the corresponding SDL1, SDL2, or SDL3 directory.

  Library family   SDL1 example              SDL2 example      SDL3 example
  --------------   -----------------------   ---------------   -----------------------------
  core             core/pixel.bas            core/sdl2-hello.bas
                                                               core/renderer/01-clear/clear.bas
  image            image/image_test1.bas     image/image2.bas  image/showimage.bas
  mixer            mixer/music_test1.bas     mixer/mixer2.bas  mixer/basics/01-load-and-play/load-and-play.bas
  net              net/net_httpget.bas       net/net2.bas      net/datagram.bas
  ttf              ttf/ttf.bas               ttf/ttf2.bas      ttf/showfont.bas
  sound            sound/sound1.bas          sound/sound2.bas  sound/playsound_simple.bas
  gfx              gfx/gfx_line.bas          gfx/gfx2.bas      gfx/gfx3.bas
  rtf              rtf/rtf1.bas              rtf/rtf2.bas      rtf/rtf3.bas
  gpu              gpu/gpu1.bas              gpu/gpu2.bas      (SDL3 uses its core GPU API)
  FontCache        fontcache/fontcache1.bas  fontcache/fontcache2.bas
  Pango            pango/pango1.bas
  bgi                                        bgi/bgi2.bas      bgi/bgi3.bas
  shadercross                                                  shadercross/shadercross3.bas

End of readme.txt
