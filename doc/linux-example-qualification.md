<!--
Project: FreeBASIC native example qualification
File: linux-example-qualification.md
Purpose: Describe repeatable Linux compiler and genuine SDK checks.
Responsibilities: Build scope, dependencies, commands and evidence boundaries.
This file contains no host credentials or physical-device acceptance.
-->

# Linux example qualification

Use a private checkout and build its compiler and runtimes together. Keep the
compiler executable, source hashes and package versions with the results. The
Windows executable cannot serve as the native Linux compiler. A source
bootstrap can be emitted for Linux from the current compiler and then compiled
on Linux with `make -j2 bootstrap-minimal`, followed by `make -j2 libs` and a
native compiler rebuild. Do not replace a working installed compiler to run
these checks.

The CT133 run uses Debian 13 amd64, GCC 14, two CPU cores and 4 GiB of RAM. Its
private workspace is `/srv/compile-testing/fbc-exampleageddon`. Keep at most two
compiler jobs active across all builds in that guest. The SDL add-on builder
uses one nested CMake job per concurrent library by default; a caller which
builds libraries sequentially can pass `build_jobs=2`.

## Development packages

Install the development package corresponding to each binding being exercised.
The common SDK checks use these Debian packages:

```sh
sudo apt-get install --no-install-recommends \
    libzip-dev libexpat1-dev libxml2-dev zlib1g-dev libbz2-dev liblzo2-dev \
    libgmp-dev libffi-dev libgsl-dev liblua5.3-dev \
    libjpeg-dev libfreeimage-dev libgd-dev libsdl1.2-dev \
    libfreetype-dev libcairo2-dev libtre-dev \
    libcurl4-openssl-dev libsqlite3-dev libaspell-dev aspell-en \
    liballegro4-dev liballegro5-dev liballegro-ttf5-dev default-jdk-headless \
    xvfb xauth x11-utils fonts-dejavu-core
```

The whole corpus also needs the native GTK2/GTK3, SDL2/add-on, database client,
image, audio and OpenGL development packages used by its individual examples.
Record the actual package versions after installation. A successful package
transaction does not establish that a binding matches that package's ABI.

Build historical SDKs from their matching upstream releases in separate
prefixes when the distribution no longer provides them. The qualification
uses PCRE 8.45 with its 8-, 16- and 32-bit APIs, GLFW 2.7.9, TinyPTC X11 0.7.3,
GDSL 1.8, big_int 1.0.7, SQLite 2.8.17, CGUI 2.1.0 and the matching GTK2
add-ons. Preserve archive hashes and any applied upstream or Debian patches.
Keep GLFW2 outside the general library search path because GLFW3 uses the
same Unix library name with an incompatible API.

Newton 3.13 uses `_POSIX_VER_64` for the Linux amd64 build and retains its
default single precision. C probes include `stdbool.h` before its legacy
header. GNU libjit revision `942c988db170d98061a9e934fb3d7b618b7d5137`
uses its genuine System V backend with `--disable-long-double`; add
`-d JIT_NFLOAT_IS_DOUBLE` to its client examples only. Native cryptlib 3.4.9.5
provides the SHA-256/SHA-1 APIs used by the original examples.

PDFlib 4.0.2 uses its bundled compression/image libraries. Keep their library
directory local to the PDF example so that old `libz`, `libpng` and `libtiff`
archives cannot shadow current SDKs for other examples. Apply the recorded
`pdflib4-open-file-cleanup.patch` and `pdflib4-tiff-lp64.patch` private fixes.
The latter keeps TIFF file fields at 32 bits when the host C `long` is 64 bits.
The native TIFF probe checks an independently encoded odd-width image and
guarded decoder output; the PDFlib probe independently parses and renders
the resulting PDFs.

`build_scripts/build-sdl3.py --jobs 2` builds the pinned SDL3 releases used by
the shipped bindings. The source archives and SDK prefix are private build
artifacts. The SDL add-on package manifest records its separate source pins.

## Legacy i386 SDKs

The archived SpiderMonkey 1.6, GRX 2.4.9 X11, wx-c 0.9.2/GTK2, BASSMOD 2.1
hotfix and FMOD 3.75 SDKs use i386 here. Build matching FreeBASIC linux-x86 runtimes privately.
Debian multiarch development packages supply their 32-bit dependencies.
Keep amd64 SDK library directories out of these examples' compiler profiles;
select `-target linux -arch 686` and each private SDK's actual library path.

SpiderMonkey uses target-generated jsautocfg.h and the private
spidermonkey16-fdlibm-errno.patch. It restricts the global errno workaround to
XP_WIN; Unix requires its normal thread-local errno header. The real engine's
C ABI, callbacks, roots and original examples pass on both i386 backends.

For GRX X11 add `-d GRX_USE_X11`, selecting actual libgrx20X.a and X11.
The default console backend remains available. test-grx.py compares all 256
cells through the actual X11 drawable as well as GRX, on a private display.
Windows retains its native GDI checks.

wx-c uses all 156 original wrapper units from the pinned wx.NET-0.9.2 archive,
linked to genuine wxWidgets 2.8.12 GTK2 Unicode libraries. test-wxc-linux.py
checks C/FreeBASIC widths and string/colour values, captures actual original
GTK2 windows and closes them normally on both i386 backends. Keep the
underlying wxWidgets shared libraries on the private loader path.

BASS amd64 original clients pass with a process-local official ALSA null PCM.
It discards audio; physical sound-card operation remains open. BASSMOD and
FMOD need OSS for ordinary playback. Use test-bassmod.py --decode-only and
test-fmod.py --metadata-only for scoped decoder/metadata checks without
/dev/dsp. Those modes compile original clients and never claim OSS playback.
Do not install or load host kernel audio drivers from the container.

Opening an output device is insufficient for playback acceptance. The archived
FMOD 3.75 binary opened ALSA through the genuine Pulse PCM plugin in CT133,
but its tracker timing failed the native C check. Supported buffer settings
of 50, 100 and 200 ms did not produce an accepted tracker result. Retain that
failure separately from the passing metadata checks; it does not establish
working playback for the original examples.

## Native checks

The following commands assume `bin/fbc` is the private native Linux compiler,
its runtimes use the checkout's normal layout, and the historical SDK prefix
is `out/legacy-sdk/prefix`:

```sh
python3 build_scripts/test-serialization.py --fbc bin/fbc \
    --cc /usr/bin/gcc --clang /usr/bin/clang --backend gcc --backend gas64 \
    --cc32 /usr/bin/gcc --prefix32 "$PWD"

python3 build_scripts/test-linux-libraries.py --fbc bin/fbc --prefix "$PWD" \
    --sdk-prefix out/legacy-sdk/prefix --output out/linux-sdk

python3 build_scripts/test-compression-xml.py --fbc bin/fbc \
    --runtime-prefix "$PWD" --output out/linux-compression

python3 build_scripts/test-curl.py --fbc bin/fbc \
    --runtime-prefix "$PWD" --output out/linux-curl

python3 build_scripts/test-sqlite3.py --fbc bin/fbc --cc /usr/bin/gcc \
    --output out/linux-sqlite3

python3 build_scripts/test-aspell.py --fbc bin/fbc --cc /usr/bin/gcc \
    --prefix "$PWD" --output out/linux-aspell
```

The SDK runner compares independent C header sizes and offsets with the
FreeBASIC declarations before executing the libffi closures and regex
wrappers. It checks guarded FreeType bitmap blending against independently
calculated pixels, including clipping and negative pitch. C probes call the
real SDKs. The receipt hashes their headers and linked libraries.

The compression/XML runner checks original example output, malformed inputs,
bounded extraction and independent archive decompression. Its oversized file
is sparse and must allocate at most 64 KiB. Linux filename cases require a
case-sensitive test filesystem. Device names, trailing dots and case variants
have different meanings from Windows; the runner checks the actual Linux
results while retaining traversal, duplicate, CRC and size-limit checks.

Examples with final console waits receive input only through an owned PTY.
The helper establishes a controlling terminal and foreground process group
before executing the real program. It never sends input to a user's console.
Native GUI probes use a separate authenticated Xvfb display and stop it when
the probe ends.

The SDL3 core checks can be run on each backend with
`test-sdl3-examples.py --fbc bin/fbc --backend gcc --select core/ --private-x`
and the corresponding `--backend gas64` command. These use the SDK's software
renderer and official dummy audio driver. GPU examples need the native Vulkan
loader and a usable Vulkan driver; Debian's `libvulkan-dev` and
`mesa-vulkan-drivers` provide the software-driver test context used here.
These runs check the actual library's APIs and bounded example behavior.
They do not establish physical camera, sound or GPU hardware operation.

The libcurl fixtures bind only to loopback and verify exact response/upload
bytes, redirect rewind, multipart forms and failure statuses. Aspell checks
installed Canadian and US English dictionaries, suggestions, EOF, byte limits
and valid/invalid UTF-8 without saving personal dictionaries.

## Interactive SDL and sound examples

```sh
python3 build_scripts/test-sdl12-examples.py --fbc bin/fbc \
    --output out/linux-sdl12 --jobs 2

python3 build_scripts/test-sdl-addons.py --fbc bin/fbc \
    --workdir out/sdl-addons --output out/linux-sdl-addons --jobs 2

python3 build_scripts/test-sfx-examples.py --fbc bin/fbc \
    --output out/linux-sfx-examples --jobs 2

python3 build_scripts/test-sfx-examples.py --fbc bin/fbc \
    --output out/linux-sfx-pulse --jobs 2 --driver pulse --private-pulse
```

The SDL runners inspect rendered windows, use input only on their own X
servers and check decoder, timer, event, error and loopback-network paths.
Their SDL audio driver is the official dummy driver. The sound runner uses
the genuine sfxlib null backend, checks bounded driver samples and verifies
original output WAV files independently. Input capture and external MIDI
hardware may be unavailable; that is distinct from software mixing/output.

The PulseAudio profile creates a separate authenticated service and clocked
null sink, with a private Unix socket and D-Bus session. It exercises the real
PulseAudio API and verifies accepted samples without changing desktop audio.
An independent monitor capture can confirm PCM beyond the library's own
diagnostic hook. Native ALSA checks can select a process-local null PCM using
ALSA_CONFIG_PATH; those accepted samples are also diagnostic evidence, not
physical speaker proof. The private service requires pulseaudio,
pulseaudio-utils and dbus-run-session.

The native write-loop regression is separate from playback acceptance:

```sh
python3 build_scripts/test-sfx-linux-write.py --cc /usr/bin/gcc \
    --bits 64 --output out/sfx-write64
python3 build_scripts/test-sfx-linux-write.py --cc /usr/bin/clang \
    --bits 32 --output out/sfx-write32
python3 build_scripts/test-sfx-linux-write.py --cc /usr/bin/clang \
    --bits 64 --ubsan --output out/sfx-write-ubsan
```

It uses genuine SDK declarations with unit-only controlled write responses to
check partial acceptance, zero-frame retry bounds, error prefixes and invalid
input sizes. These fault cases do not emulate playback or replace the SDK.

PortAudio example cleanup and callback bounds have a separate controlled
regression:

```sh
python3 build_scripts/test-portaudio-failures.py --fbc bin/fbc \
    --output out/portaudio-failures
```

Native PortAudio monitor checks must wait for the recording client and allow
for software-sink startup buffering. An independent C reference helps separate
fixture behavior from FreeBASIC callback behavior. The checked fixture uses
a private clocked Pulse sink, the genuine ALSA Pulse PCM plugin, bounded raw
PCM capture and independent tone analysis; it does not prove exact audible
durations. The archived FMOD 3.75 ALSA/Pulse route remains unqualified: its
tracker timing is inconsistent, including with tested buffer sizes.

The private Xvfb helper uses -noreset to keep the server from resetting after
the readiness client disconnects, before the example connects. Authentication
and cleanup remain active. Native gfxlib2 windows have no _NET_WM_PID property;
the composer check therefore requires its unique exact title on a private
server containing only the launched child, rather than searching the desktop.

OpenAL source and buffer cleanup has a controlled amd64 regression using the
installed OpenAL/ALUT declarations and both compiler backends:

```sh
python3 build_scripts/test-openal-failures.py --fbc bin/fbc \
    --output out/openal-failures
```

The WAV player accepts an explicit filename, checks API failures, and deletes
its source before its attached buffer. Native Linux checks use a private
PulseAudio service and an owned terminal to pause, resume and exit the exact
example while recording bounded nonzero monitor PCM. FB_EXAMPLE_AUTOPLAY=1
checks a one-second lifecycle; cold software-sink startup can delay observable
PCM beyond that interval. A native C reference exhibits the same delay.
Allow startup time before testing interactive controls or accepting PCM.
These software-sink results do not qualify physical speaker output.

## GTK windows and C argument checks

The GTK examples have a separate native window runner:

```sh
python3 build_scripts/test-gtk-examples.py --fbc bin/fbc \
    --sdk-prefix out/gtk2-sdk/prefix --output out/linux-gtk
```

The optional SDK prefix supplies archived GTK2 add-ons such as libglade and
gtkglext; installed GTK2, GTK3 and GooCanvas libraries remain genuine. The
runner stages assets, tests all three packing layouts, opens a text fixture
in the application example, and checks normal closure of each visible window.
Arrow dragging must change the rendered image and canvas items must receive
keyboard focus. Only the base and colour-selection examples intentionally
start with uniform windows. Authentication and process ownership remain
required. Accessibility bridging is disabled for this private fixture, which
has no accessibility service; software OpenGL is selected explicitly.

GObject variadic property values must use their declared C types. Boolean
properties use CTRUE (1), and double properties require Double expressions.
Native C va_arg checks cover the Windows GAS64 immediate-float register-copy
regression separately:

```sh
python3 build_scripts/test-vararg-float.py --fbc bin/fbc \
    --cc /usr/bin/gcc --output out/vararg-float
```

This native amd64 runner checks register positions, stack arguments, zero and
negative values, stored values, named float parameters and indirect calls.
On Windows, select the native GCC executable and compiler paths explicitly.
The Windows compiler fix is qualified in a private candidate; a Linux native
run does not by itself establish the Microsoft x64 ABI.

## IUP dialogs

The IUP examples have native dialog and callback checks:

```sh
python3 build_scripts/test-iup-examples.py --fbc bin/fbc \
    --sdk-prefix out/native-small-sdk/prefix --output out/linux-iup
```

Use the genuine GTK3-backed IUP library in that prefix. The runner tests all
five examples on both amd64 backends. With DISPLAY removed, each must report
initialization failure and exit with status 1. With a private authenticated
Xvfb display, the button example must open and dismiss its modal message, then
exit through its Close callback. Text edits include a decimal value, empty
input and words with spaces. RGB cases check orange, cyan and out-of-range
components clamped to black by inspecting actual rendered pixels. Input stays
on the launched process's own display and the server is reaped afterward.
These checks do not qualify physical displays or all IUP controls.

## Whole example sweep

```sh
python3 build_scripts/exampleageddon-freebasic.py --root "$PWD" \
    --fbc "$PWD/bin/fbc" --prefix "$PWD" --target-os linux --jobs 2 \
    --outdir out/linux-examples --compile-timeout 90 --run-timeout 15 \
    --fail-on-self-contained
```

Add explicit library paths and per-example profiles for the SDKs present on
the machine. Debian's libjpeg-turbo uses JPEG ABI 6, so the JPEG example needs
`-d __JPEGLIB_VER__=6`; the checked Windows SDK uses ABI 8. Keep that choice
local to the example. Use `--glfw2-libdir` for the legacy GLFW example and
`--iup-gtk3` when linking against the GTK3 IUP build.

Use absolute library paths: each example compiles in its own working directory.
For private shared-library SDKs, the compiler/linker process also needs their
transitive dependencies on LD_LIBRARY_PATH or an appropriate linker search
path. The wx-c 2.8 checks require their matching private wxWidgets libraries.
Keep the GLFW2 archive in its own directory and pass that directory through
--glfw2-libdir; adding the generic SDK prefix can instead select system GLFW3.

A global backend override needs per-example profiles for backend-specific
sources. The builtin-memory and builtin-numeric examples explicitly require
the GCC/Clang/LLVM builtin interface; a GAS64 qualification must compile those
with a compatible C backend and record the exceptions. Likewise, archived
i386-only libraries need their original target and toolchain profiles rather
than inheriting native amd64 library paths.

Set CXX to the complete C++ compiler used for local .cxx helpers, and keep its
matching runtime libraries available when executing those clients. A different
g++ earlier on PATH can be incomplete even when the selected GCC works.

Native GAS64 value regressions can be run separately from the corpus sweep:

```sh
python3 build_scripts/test-gas64-values.py --fbc bin/fbc \
    --cc /usr/bin/gcc --output out/gas64-values
```

This checks integer FRAC results returned as Single and Double, including an
overloaded UDT operator. Independent C aggregate factories exercise floating,
integer and mixed return classes through array, member and pointer stores,
with exact field values and neighboring canaries. These are native amd64 ABI
checks. They do not establish physical graphics or another architecture's ABI.

Packed C structures from 9 through 16 bytes also check every returned tail
byte and adjacent array elements. On SysV amd64 these tails occupy the second
integer return register. Partial stores must shift by bits, so consuming two
or four bytes requires a shift of 16 or 32. The regression catches incorrect
tail extraction even when the structure's first eight bytes are correct.

The runner reports every compile failure and the original classification.
`--fail-on-self-contained` gates its unattended self-contained subset; a zero
exit does not qualify missing external libraries or interactive examples.
Retain the CSV, failure logs and focused SDK receipts together. Native amd64
checks do not qualify a 32-bit SDK, physical sound devices or accelerated GPU
hardware. Container/Xvfb checks must be reported with that scope.

<!-- end of linux-example-qualification.md -->
