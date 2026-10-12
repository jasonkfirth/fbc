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

## Native checks

The following commands assume `bin/fbc` is the private native Linux compiler,
its runtimes use the checkout's normal layout, and the historical SDK prefix
is `out/legacy-sdk/prefix`:

```sh
python3 build_scripts/test-serialization.py --fbc bin/fbc \
    --cc /usr/bin/gcc --clang /usr/bin/clang --backend gcc --backend gas64

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

The runner reports every compile failure and the original classification.
`--fail-on-self-contained` gates its unattended self-contained subset; a zero
exit does not qualify missing external libraries or interactive examples.
Retain the CSV, failure logs and focused SDK receipts together. Native amd64
checks do not qualify a 32-bit SDK, physical sound devices or accelerated GPU
hardware. Container/Xvfb checks must be reported with that scope.

<!-- end of linux-example-qualification.md -->
