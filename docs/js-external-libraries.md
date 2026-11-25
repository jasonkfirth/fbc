<!--
    Project: FreeBASIC JS SDK
    File: js-external-libraries.md
    Purpose: Document the packaged browser libraries and how to verify them.
    This file does not describe native library installation.
-->

The Windows and Debian/Ubuntu fbc-js package recipes build these libraries
with the same Emscripten installation used by the compiler.

| Library | Packaged version | BASIC include |
| --- | --- | --- |
| raylib | 5.5 | `raylib.bi` |
| SDL2 | 2.32.10 | `SDL2/SDL.bi` |
| SDL3 | 3.4.18 | `SDL3/SDL.bi` |
| SDL2_image | 2.8.8 | `SDL2/SDL_image.bi` |
| SDL2_ttf | 2.24.0 | `SDL2/SDL_ttf.bi` |
| FreeType | 2.14.3 | `freetype2/freetype.bi` |
| SQLite | 3.53.4 | `sqlite3.bi` |
| libpng | 1.6.59 | `png.bi` |
| IJG libjpeg | 9f | `jpeglib.bi` |
| bzip2 | 1.0.8 | `bzlib.bi` |
| Expat | 2.8.0 | `expat.bi` |
| zlib | 1.4 | `zlib.bi` |

They install the archives and their licenses beside `libfb.a`. The
Debian/Ubuntu wrapper fetches the pinned sources before assembling the
source package. The library compilation in `debian/rules` runs offline
and uses CMake and Ninja for SDL3, FreeType, and Expat.

The SDL1 example uses Emscripten's built-in SDL1 compatibility layer.
Choose one SDL generation per program because the C symbol names overlap.
SDL2_image uses its bundled decoders. SDL2_ttf uses the packaged FreeType
without HarfBuzz. The SQLite build runs without threads and omits loadable
native extensions. The examples use a virtual filesystem, so persisting
database writes across reloads requires an application storage backend.
Native sockets and the desktop SDL companion libraries need their own
browser ports or transport. The profile covers the 2D renderer. GPU,
camera, and physical gamepad support need separate qualification.

`raylib.bi` selects raylib 5.5 on JS. The compiler adds Emscripten's
GLFW 3 implementation when the program links raylib. Native projects
linked against raylib 5.5 can include `raylib55.bi` directly. The
original `raylib.bi` native declarations still correspond to raylib 3.0.
Regenerate the 5.5 declarations with
`python build_scripts/generate-raylib55-binding.py RAYLIB_SOURCE inc/raylib55.bi`.
The browser audio build applies a small fix to raylib's bundled miniaudio.
Its Web Audio callback takes the floating-point heap view from Emscripten's
generated runtime, where the view stays available after memory grows.

`zlib.bi` selects the packaged zlib 1.4 version on JS. Emscripten uses
64-bit file offsets, so gzip seeking and the checksum combination APIs
take `LongInt` offsets even though the target has 32-bit pointers.

For a normal browser build, run `fbc-js-app examples/graphics/raylib/drawing.bas`.
On Windows use `fbc-js-app.cmd`. Serve the output directory over HTTP.
The example draws 60 frames and exits. Raylib 5.5's web
`WindowShouldClose()` yields through Asyncify, but does not report
a desktop-style close event.

To run the library profile in Chromium from the source checkout, make
`playwright-core` available to Node and set `FBC_BROWSER` to a local
Chromium executable. With a packaged fbc-js and its Emscripten tools on
the path, run the following from the repository root.

```sh
python3 build_scripts/exampleageddon-freebasic.py \
  --target-os js --no-prefix --fbc "$(command -v fbc-js)" \
  --include-dir /usr/include/freebasic-js \
  --example-profile build_scripts/exampleageddon-js.json --only-profile \
  --outdir out/exampleageddon-js --jobs 2 --run-timeout 75 \
  --run-prefix node --run-prefix "$PWD/build_scripts/exampleageddon-js-browser.cjs" \
  --fbc-arg=-Wl --fbc-arg=-sEXIT_RUNTIME=1
```

On Windows, pass the absolute paths to the package's `fbc-js.cmd`, its
`include/freebasic-js` directory and the browser runner. The browser
runner serves each generated page on loopback and checks the program's
exit status. It enables autoplay in its own Chromium instance so the
audio example can start without a manual click.

<!-- end of js-external-libraries.md -->
