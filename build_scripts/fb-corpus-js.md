<!--
    Project: FreeBASIC browser corpus
    File: fb-corpus-js.md
    Purpose: Reproduce the catalog and verify the browser programs.
    This file does not contain host credentials or deployment inventory.
-->

# FreeBASIC browser corpus

The build profile lists each entry point and its source files. The build
driver stages those files in a temporary directory, links against a staged
`fbc-js` SDK, and keeps the generated HTML, JavaScript and Wasm with a
checksum receipt. The `assets` field embeds additional inputs in the
browser's virtual filesystem.

Use `preload` for large asset directories. Each key names a directory in
the project, and its value is the directory visible inside the browser
filesystem. The build packages tracked files into `index.data` and leaves
the source directories intact. `replacements` records small browser
adaptations against the original source, with an expected match count when
the same text appears more than once.

List additional compilation units in `units` and staged include search
paths in `include_dirs`. `files` stages inputs referenced by the source,
including headers and BASIC files that the entry point includes itself.
`exclude_files` names files to omit when a broad glob includes alternate
platform data. `notices` stages license files and includes them in the
published license page and source archive.

Set `adapter` when a browser entry point lives beside the build driver and
calls modules from the corpus project. The driver includes it in the input
hashes. The source archive places the adapter in the project directory and
includes a build profile that uses it from there.

For a demo that links a BASIC library, use `static_archive` with the
library name and a list of staged source files. The build creates `lib<name>.a`
with `fbc-js -lib`, adds its directory to the normal link search, and
includes the archive beside the browser program. The source archive
contains the BASIC headers used to link the library.

From the repository root on Windows, with the SDK and bundled Emscripten
installed, run the build:

```powershell
python build_scripts/fb-corpus-js.py --corpus C:/fb_corpus `
  --profile build_scripts/fb-corpus-js.json --output out/fb-corpus-js `
  --compiler out/js-libraries/stage/bin/fbc-js.exe --toolchain C:/freebasic-js
```

Use `--only <slug>` for each entry you want to rebuild. Inspect individual
compiler logs under `out/fb-corpus-js/logs`. Each attempted entry has a
status in `build-results.json`. A case can set a longer `timeout` in
seconds for its compiler step. The `--timeout` option overrides that value.

With `playwright-core` on `NODE_PATH` and a Chromium executable in
`FBC_BROWSER`, run a program check:

```powershell
node build_scripts/fb-corpus-js-check.cjs out/fb-corpus-js guess
```

To preview or publish, generate the static catalog with
`fb-corpus-js-site.py --corpus ... --profile ... --build ... --site ... --only ...`.
Select reviewed entries whose browser results match the build. The generator
writes a launch page, source archive, and license for each entry, along with
a catalog-wide `SHA256SUMS.txt`. Serve the output directory over HTTP so
Emscripten can load the Wasm files.

Every title on the main catalog opens its program directly. Each description
links to the upstream project and to the source and license for the browser
build. The main catalog groups the text programs below the graphical programs
and also lists them at `text.html`. Existing program directory URLs redirect
to the corresponding `play.html`.

To test the actual launch pages, set `FBC_CORPUS_SITE` to the preview URL
and `FBC_CORPUS_CONSOLE=1`. Pass the same entry slugs to the browser runner.
This mode uses the visitor's input form and reads the transcript directly.

Use `fb-corpus-js-extend.py` to add reviewed entries to a deployed catalog.
Pass the existing catalog and its browser receipt with `--base` and
`--base-preview`, and the generated addition and its preview with `--add`
and `--add-preview`. The command writes a combined catalog at `--site`, a
combined browser receipt at `--receipt`, and a deployment overlay at
`--overlay`. The overlay contains the added programs and the replacement
catalog index and manifest. Existing entry files are reused as hard links.
Pass `--replace OLD NEW` to put a verified addition in an existing entry's
place on the main page. The old player and directory URLs redirect to the
new player.

The OpenMIDISeSh editor uses `fb-corpus-openmidisesh.json` with the
OpenMIDISeSh project's parent directory as `--corpus`. The profile selects
the regular editor modules, the built-in synth, and browser workspace
storage. The File menu saves into that workspace. The browser file controls
transfer files between it and the visitor's computer.

## Editor workspaces

Set `workspace_aliases` on a graphics program that keeps files between
visits. The build links Emscripten's IDBFS and loads the program's directory
from IndexedDB before BASIC starts. Relative paths resolve within that
directory. List absolute top-level data directories such as `/bladen` as
aliases if a program builds paths from `ExePath`. The app's normal save and
open commands work inside this workspace. A second tab runs with a temporary
workspace while the original tab is open.
To ship an editable sample, put the asset at `/browser-sample/<filename>`
and use `workspace_sample` to name the file. On the first visit the runtime
copies it into the workspace. It opens the workspace copy on every visit
and offers a download of the last saved bytes.

The fbfrog player converts a pasted or uploaded C header in a dedicated
Web Worker. Each conversion launches a fresh fbfrog instance with a private
virtual filesystem. The worker returns the generated `.bi` text and the
compiler diagnostics to the page.

Clumsier uses its original 80-column terminal editor and saves into the
workspace's `prj` directory. The browser file controls copy source into and
out of that directory. Use the editor's Open and Save commands before
downloading. Imports use ASCII with up to 79 characters per line because
Clumsier's reader rewrites longer lines as BASIC continuations. Clumsier's
desktop compile command launches an external executable, so the browser
build exposes the source editor and file operations.

Dark Castle opens the original isometric editor with a generated workshop
room and textures. The editor saves its native `.map` files under `/maps`.
Use F2, then F4 to save, and F2, then F3 to open a saved room. The page also
offers the saved files for download. The build keeps full Asyncify support
because the editor's indirect keyboard wait must yield to the browser.

Prospector uses the game's character mode and builds with its own
framebuffer driver. Run `fb-corpus-prospector.py` with the Prospector
checkout, FreeBASIC source tree, staged SDK, Emscripten command shim, and a
new build directory. Then run `fb-corpus-prospector-site.py` with that build,
a new catalog staging directory, and the catalog stylesheet. The player
uses IndexedDB for saves made through the game's own Save and Quit command.

The FlightGear calculator runs one operation at a time in a Web Worker.
The form sends the original menu number and supplies the values to BASIC's
`INPUT` calls. It displays the text produced by the original calculation
units. The desktop menu and its pause for a key are still part of the
native build.

The fbrtLib formatter accepts a number and format mask. It runs the
library's `Format` function in a fresh Web Worker and displays the exact
text, including whitespace. With no arguments the same executable runs
the library's extreme-precision regression fixture.

The local `fblint` case accepts a pasted or uploaded `.bas` or `.bi` file.
After building the case, run `fb-corpus-fblint-stage.py` with the build
directory and serve its `apps` directory over HTTP. Open `/fblint/lint.html`.
Each check uses a fresh worker and reads fblint's JSONL findings. The
browser uses `--no-semantic` because compiler-backed checks launch an
installed FreeBASIC compiler. The port matches the semantic helpers'
64-bit output parameters on the 32-bit Wasm target.

Station922's preview uses the portable static file handler. A worker gives
it a complete HTTP request and a virtual site root, then reads the status,
headers and bytes emitted by the FreeBASIC code. The sample site and any
folder selected by the visitor stay in that worker's virtual filesystem.

The AZdecrypt player runs the original substitution search with its English
four-gram table in a dedicated worker. Run `fb-corpus-azdecrypt-stage.py`
with the AZdecrypt checkout and a new `azdecrypt` directory under a staging
corpus, then build `--only azdecrypt-substitution` from that staging corpus.
The adapted source and model are included in the downloadable source archive.
Each job runs a finite set of restarts with a selected seed. The worker can
be canceled without blocking the page or leaving a search running.

The Kiwi URL Inspector passes the address to the compiled Kiwi sample in a
fresh Web Worker. Kiwi's URL getters report the protocol, host with optional
port, and the remaining path, query and fragment. The browser adaptation
splits at the first separator after the authority and omits the desktop
object base, whose wait and notify methods bring in thread callbacks. The
worker reads the result from the virtual filesystem, which preserves field
boundaries that the browser console hook can omit. The page inspects the
input locally and makes no request to the address.

The ToyBASIC page passes one arithmetic expression to a fresh Wasm worker.
The program uses the original precedence parser and evaluates its tree in
FreeBASIC. The browser build limits expression length, validates operators
and numbers, and returns a specific error for malformed input. It reserves
Wasm stack space for recursive unary operators and nested parentheses
within the 256-character limit. The raw executable can still run the
original `code.toy` sample.

OpenSesh MIDI to MOD runs the OpenSesh reader and exporter from the
`midisoft-fb` snapshot. A worker mounts the input file and returns the
conversion report and the downloadable four-channel ProTracker module.
The report records pitch, duration, and voice reductions. The browser
check verifies the exported pattern data and sample lengths, rejects
malformed MIDI, and converts another file in the same page.

## Browser line input

The catalog provides `Module.fbReadLine()` for interactive text programs.
The callback returns a promise for one line of text. Asyncify lets BASIC's
`Input` and `Line Input` wait for that promise while the browser displays
the transcript and handles the form. Set `Module.fbLineInput` to the editor
so FreeBASIC's window key listener yields to the browser's key handling
while that editor has focus.

The console contains touch events within its HTML controls and transcript.
FreeBASIC's window-level touch callback emulates a mouse and cancels the
default action. Keeping console touches local lets the browser generate tap
clicks and scroll the transcript. The published-page checks exercise touch
submission alongside mouse clicks and keyboard entry.

`GetKey` waits for a key in a browser and yields during the wait. `Dir`
enumerates the Emscripten virtual filesystem, including files embedded or
preloaded by the build profile.

HACKIT uses the Linux console adapter with an fbc-js SDK that includes the
browser `WIDTH()` query fix. The player supplies one ASCII document as a
startup argument. Ctrl+S writes to the virtual filesystem, and the download
button exports those saved bytes. Download before reloading the page. The
port treats the virtual root as a directory and checks an existing file's
write access without truncating its contents.

## Graphics keyboard input

Set `graphics_keyboard` for an entry that uses the gfxlib2 canvas and
expects text or key events through the FreeBASIC runtime. The build driver
retains `Module.doNotCaptureKeyboard` alongside the Emscripten defaults.
This lets gfxlib2 turn off the legacy SDL keyboard listener and receive the
browser's printable key events.

Araxia's Windows data pack uses mixed-case filenames. Its case passes
`-sCASE_INSENSITIVE_FS=1` to Emscripten so calls written with DOS or Windows
filename semantics can find the original assets.

OpenGL examples can use `ScreenGLProc` once a WebGL context is current.
Emscripten supplies a Wasm-callable address. Shader examples that require
WebGL 2 also need appropriate linker flags and GLSL ES shader sources.

ChessbrainFB uses its original graphical board and engine. Its native UCI
input loop requires a background thread, so the browser case launches the
graphical game. Nebunoid uses its existing silent backend. The profile
keeps the original campaigns and one credited backdrop for the download.

The SDL2 image viewer and slideshow checks create their own BMP files in
the browser filesystem. They check image rotation and a slide transition.
The cases still use the checkouts' fonts for local testing.

The fb-imageproc page accepts image files the browser can decode, with a
12 MiB file limit and a maximum output side of 1,600 pixels. A fresh
worker sends packed RGBA rows to the original grayscale routines. The
adapter uses gfxlib's null display mode and respects bitmap row padding.
The worker returns RGBA bytes for display and PNG download. The same
executable draws the original sample grid when run without arguments.

The local FB_IMG2ASCII check uses its original `BLOAD` and `BSAVE` path
to convert a generated bitmap in color and monochrome. The JS target's
gfxlib driver uses legacy SDL, so this browser case takes a BMP path and
leaves JPEG and PNG decoding through SDL2_image to native targets.

The JS SDK builds `libTilengine.a` from pinned Tilengine, SDL2 and libpng
sources. The `tilengine-learning` case uses `Tilengine.bi` from the
FBTilengine checkout and links the SDK's SDL2, libpng and zlib archives.

The upstream Mode 7 sample's browser port uses that SDK to draw a tile
plane with one affine transform per scanline. The application generates
its own indexed terrain and parallax sky. The browser test verifies
forward motion, rotation, and a reset to the opening view. The source
download includes the Tilengine revision matched to the SDK archive,
the original C sample, the binding, and the FreeBASIC port.

```powershell
$stage = 'out/fb-corpus-js/tilengine-mode7-input'
python build_scripts/fb-corpus-tilengine-stage.py C:/fb_corpus/Tilengine C:/fb_corpus/FBTilengine out/js-libraries/stage $stage
python build_scripts/fb-corpus-js.py --corpus $stage --profile "$stage/tilengine-mode7.json" `
  --output out/fb-corpus-js/tilengine-mode7-work --compiler out/js-libraries/stage/bin/fbc-js.exe `
  --toolchain C:/freebasic-js --only tilengine-mode7
```

The Griffon Legend's map loader keeps a 321-by-201 integer array on the
stack. The case raises the Wasm stack limit and enables overflow checks.
The browser build reads the original assets from their usual paths and
keeps its four save slots and configuration in IndexedDB. The browser
check walks through the town, saves, reloads, and loads the saved location.

Dungeon of the Phoenix uses raylib's Web Audio backend to play the
original Ogg samples. It generates a separate atlas with letter and
symbol tiles and uses raylib's default font. The browser package omits
the original icon sheet and font files. The browser frame reads key edges before
`EndDrawing` snapshots GLFW's key state. The test enters the dungeon,
moves the player sprite, and checks for nonzero Web Audio samples.

Pyromax Dax keeps the engine's long-lived state in static storage because
the original main function's Wasm frame exceeded the default stack size.
Its texture uploader converts FreeBASIC's BGRA image rows to packed RGBA
for WebGL. The browser camera uses a translation for its fixed orientation
to avoid the bundled GLU shim's incorrect matrix argument order. Enter
skips the timed logos in the browser. The browser test opens the first
map and checks that Right moves the player sprite. The build enables
stack checks and allows extra time for Emscripten's instrumentation.

The Copper Circuit workshop stages the same engine with two original
courses, geometric artwork, and silent audio. The browser loads those
assets from a separate `.data` file. The included case lists the assets
individually so the source download can be built without Git metadata.
Generate the assets and profile from a Pyromax Dax checkout before
building the catalog case.

```powershell
$stage = 'out/fb-corpus-js/pyromax-workshop-input'
python build_scripts/fb-corpus-pyromax-stage.py C:/fb_corpus/Pyromax-Dax build_scripts/fb-corpus-js.json $stage
python build_scripts/fb-corpus-js.py --corpus $stage --profile "$stage/pyromax-workshop.json" `
  --output out/fb-corpus-js/pyromax-workshop-work --compiler out/js-libraries/stage/bin/fbc-js.exe `
  --toolchain C:/freebasic-js --only pyromax-workshop
```

The SCons-built OHRRPGCE player retains its own browser shell. The catalog
uses the shell's test RPG argument and copies the verified output files
under their original names. Its source archive contains the staged inputs
and the build command.

The cmdsqlite case uses the browser's virtual filesystem paths. SQLite
queries and conversion are synchronous, so the case turns off Emscripten's
Asyncify instrumentation for this executable. Programs that wait for
browser input still use the SDK default.

The local fb-VNC build uses the original viewer, RFB decoder, renderer,
and omaGUI connection dialog. The browser transport accepts an endpoint
that carries the RFB byte stream in binary WebSocket messages. It copies
bytes from a bounded queue and lets the existing timeout loops yield to
the browser. A page served over HTTPS uses a `wss://` endpoint.

```powershell
$stage = 'out/fb-corpus-js/fb-vnc-browser-input'
$work = 'out/fb-corpus-js/fb-vnc-browser-work'
python build_scripts/fb-corpus-fbvnc-stage.py C:/fb_corpus/fb-VNC $stage
python build_scripts/fb-corpus-js.py --corpus $stage --profile "$stage/fb-vnc-browser.json" `
  --output $work --compiler out/js-libraries/stage/bin/fbc-js.exe `
  --toolchain C:/freebasic-js --only fb-vnc-browser
node build_scripts/fb-corpus-fbvnc-check.cjs "$work/apps/fb-vnc-browser" "$work/browser-results.json"
```

The loopback check uses the same Chromium setup as the other corpus
checks. It sends a split RFB negotiation and framebuffer update, accepts
keyboard and pointer input from the viewer, then closes the socket so
the viewer can reconnect from its own dialog.

The BRPGEngine map converter accepts the compact files from
`BRpg/Res/Map`. Each cell has two 16-bit sign-magnitude tile IDs. The
port writes 32-bit signed width and height followed by the ground and
overlay IDs in row order, as read by the RPG loader. The sample is a
generated map. Set `FBC_BRPG_PROJECT` to the BRPGEngine checkout while
running the browser preview to compare two shipped maps against their
simulator payloads. The simulator copies include a trailing zero word
after the payload.

```powershell
$stage = 'out/fb-corpus-js/brpg-map-input'
python build_scripts/fb-corpus-brpg-stage.py C:/fb_corpus/BRPGEngine $stage
python build_scripts/fb-corpus-js.py --corpus $stage --profile "$stage/brpg-map.json" `
  --output out/fb-corpus-js/brpg-map-work --compiler out/js-libraries/stage/bin/fbc-js.exe `
  --toolchain C:/freebasic-js --only brpg-map
```

VisualFBEditor's browser port runs the editor's Format with Basis Word
command. It chooses a separator from the first selected line and aligns
matching lines in that selection. The FreeBASIC worker handles source
text and leaves string literals and comments alone. The original source
stays in the left editor while the result is shown and downloaded from
the right. An uploaded UTF-8 file keeps its BOM and line endings when
the displayed source is unchanged. Set `FBC_VISUALFB_PROJECT` while
running the browser preview to test the command on the original
LNGCreator source. The desktop editor requires MyFbFramework and its
desktop GUI backend.

```powershell
$stage = 'out/fb-corpus-js/visualfb-align-input'
python build_scripts/fb-corpus-visualfb-stage.py C:/fb_corpus/VisualFBEditor $stage
python build_scripts/fb-corpus-js.py --corpus $stage --profile "$stage/visualfb-align.json" `
  --output out/fb-corpus-js/visualfb-align-work --compiler out/js-libraries/stage/bin/fbc-js.exe `
  --toolchain C:/freebasic-js --only visualfb-align
```

The FreeBASIC compiler pages run the compiler's normal entry point inside a
worker. `-pp` writes preprocessed BASIC and `-r` writes C for the JS target.
The worker passes `-m main` so the C output has an executable entry point.
The worker mounts uploaded `.bi` files in `/headers` for `#include` to find.
Use the Emscripten SDK to turn the generated C into a browser executable.
The staging script records the selected Git revision and supplies the JS
host options. The build disables the newer sound language extension because
it reserves `tempo`, a variable name in the upstream compiler.
The browser profile prints the fatal allocation diagnostic and exits with
code 4. The original `ERROR(4)` emits a computed goto that Emscripten rejects
in this procedure because it contains no error-handler label address.

```powershell
$stage = 'out/fb-corpus-js/freebasic-compiler-input'
python build_scripts/fb-corpus-compiler-stage.py C:/fb_corpus/freebasic-fbc $stage
python build_scripts/fb-corpus-js.py --corpus $stage --profile "$stage/freebasic-compiler.json" `
  --output out/fb-corpus-js/freebasic-compiler-work --compiler out/js-libraries/stage/bin/fbc-js.exe `
  --toolchain C:/freebasic-js --only freebasic-compiler
```

For FreeBASIC-NG, stage `C:/fb_corpus/freebasic-ng` with `--project-name freebasic-ng`
and use `freebasic-ng-compiler` for the profile filename stem and program slug.

L-BASIC uses two source generators to build the tables used by its parser.
The staged tools use the original `ts.rules` and `tokens.list`. The state
generator stores characters in the last dimension so FreeBASIC can grow
the table while preserving complete states. It still emits the original
QB64 table format consumed by the L-BASIC compiler.

```powershell
$stage = 'out/fb-corpus-js/l-basic-gen-input'
$work = 'out/fb-corpus-js/l-basic-gen-work'
$reference = 'C:/build/lbasic-qb64/compiler/parser'
python build_scripts/fb-corpus-lbasic-stage.py C:/fb_corpus/L-BASIC $stage
python build_scripts/fb-corpus-js.py --corpus $stage --profile "$stage/lbasic-generators.json" `
  --output $work --compiler out/js-libraries/stage/bin/fbc-js.exe --toolchain C:/freebasic-js
node build_scripts/fb-corpus-lbasic-check.cjs $work $stage $reference
```

The reference directory is `compiler/parser` from a QB64 build at the same
L-BASIC revision. The upstream `tools/tsgen` and `tools/tokgen` can generate
these files independently of the compiler build. The Chromium check compares `ts_data.bi`, `ts_data.bm`,
`token_data.bi`, and `token_registrations.bm` after normalizing line endings
and the spacing around numeric fields emitted by `PRINT #`.
It also checks that each generator reports and rejects invalid input.
Outputs from a native FreeBASIC build of the staged generators can also
serve as the reference for comparing platforms.

The frontend staging step checks the hashes of those browser-generated
tables before feeding them to L-BASIC's macro preprocessor. It builds the
original C foundation archive for Wasm and stages it under `/runtime`.
The browser checks run the compiler's `-e parse` mode against arithmetic,
the upstream counter and user-defined type tests, and malformed input.
This build produces an AST dump. The upstream LLVM stage loads a native
library, initializes the x86 target, and launches the native linker for
executable output.

```powershell
$frontend = 'out/fb-corpus-js/l-basic-frontend-input'
$frontendWork = 'out/fb-corpus-js/l-basic-frontend-work'
python build_scripts/fb-corpus-lbasic-frontend.py C:/fb_corpus/L-BASIC $work $frontend --toolchain C:/freebasic-js
python build_scripts/fb-corpus-js.py --corpus $frontend --profile "$frontend/lbasic-frontend.json" `
  --output $frontendWork --compiler out/js-libraries/stage/bin/fbc-js.exe --toolchain C:/freebasic-js
node build_scripts/fb-corpus-lbasic-frontend-check.cjs $frontendWork $frontend
```

The LLVM emitter uses [LLVM 14.0.6](https://github.com/llvm/llvm-project/releases/tag/llvmorg-14.0.6).
The SDK first builds TableGen for the host, then uses it to generate headers
for LLVM's Wasm archives. The emitter uses LLVM's `wasm32-unknown-emscripten`
data layout and writes bitcode with L-BASIC's original `-e ir` path. The
`execute` check links the Chromium-produced bitcode with the staged L-BASIC
foundation archive through the SDK and checks the programs' output in Node.

```powershell
$llvm = 'D:/llvm-14.0.6.src'
$llvmWork = 'D:/lbasic-llvm-build'
$llvmStage = 'D:/lbasic-llvm-input'
$sdk = 'C:/freebasic-js'
$fbcjs = 'out/js-libraries/stage/bin/fbc-js.exe'
$llvmOptions = @($llvm, $llvmWork, '--toolchain', $sdk)
$ninja = (Get-Command ninja.exe).Source
python build_scripts/fb-corpus-lbasic-llvm.py @llvmOptions --ninja $ninja --phase host
python build_scripts/fb-corpus-lbasic-llvm.py @llvmOptions --ninja $ninja --phase wasm
python build_scripts/fb-corpus-lbasic-frontend.py C:/fb_corpus/L-BASIC $work $llvmStage --toolchain $sdk --llvm
python build_scripts/fb-corpus-lbasic-llvm.py @llvmOptions --compiler $fbcjs --phase probe
node build_scripts/fb-corpus-lbasic-llvm-check.cjs "$llvmWork/probe"
python build_scripts/fb-corpus-lbasic-llvm.py @llvmOptions --compiler $fbcjs --frontend "$llvmStage/L-BASIC" --phase emitter
node build_scripts/fb-corpus-lbasic-llvm-check.cjs "$llvmWork/emitter" "$llvmStage/L-BASIC"
python build_scripts/fb-corpus-lbasic-llvm.py @llvmOptions --frontend "$llvmStage/L-BASIC" --phase execute
```

`fb-corpus-lbasic-llvm-site.py` stages the verified compiler in the existing
`lbasic-frontend/` catalog entry. The page offers a parse dump or LLVM
bitcode. The bitcode download uses the Wasm target, and the source archive
contains the matching foundation library and the SDK link command.

Run the curated Exampleageddon library sweep with the same SDK include
directory that the corpus builds use.

```powershell
python build_scripts/exampleageddon-freebasic.py --target-os js --no-prefix `
  --fbc out/js-libraries/stage/bin/fbc-js.exe `
  --include-dir out/js-libraries/stage/include/freebasic-js `
  --example-profile build_scripts/exampleageddon-js.json --only-profile `
  --fbc-arg=-Wl --fbc-arg=-sEXIT_RUNTIME=1 --jobs 2 `
  --compile-timeout 300 --run-timeout 30 `
  --run-prefix node --run-prefix (Resolve-Path build_scripts/exampleageddon-js-browser.cjs).Path `
  --outdir out/fb-corpus-js/exampleageddon
```

The exit setting lets the browser runner observe a successful program exit.

## Updating the catalog presentation

The catalog shares the FBXL sidebar artwork served under `/oma/graphics/`.
The game pages use OMA's viewport layout and keep the guest framebuffer size
under the game's control. Console programs and tools have their own player
layouts.

Use `--restyle-from` to apply the shared styles to an existing release without
recompiling the programs. Use new stage and overlay directories on the same
volume as the previous release. The stage hard-links unchanged assets for
browser checks. The overlay contains the changed pages, the shared presentation
files, and the new checksum manifest.

```powershell
python build_scripts/fb-corpus-js-site.py --restyle-from out/fb-corpus-js/current `
  --site out/fb-corpus-js/preview --overlay out/fb-corpus-js/presentation-overlay
```

<!-- end of fb-corpus-js.md -->
