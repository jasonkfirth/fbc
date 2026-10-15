<!--
    Project: FreeBASIC compiler
    File: doc/freebsd-example-qualification.md

    Purpose:

        Record the native FreeBSD Exampleageddon result and the third-party
        libraries that were available to FreeBASIC examples on the test VM.

    Responsibilities:

        - identify the tested FreeBSD and compiler versions
        - distinguish compile/link evidence from runtime evidence
        - list FreeBSD package names and bindings that remain unqualified

    This file intentionally does NOT contain:

        - a promise that every third-party example is portable
        - GUI, audio-device, or server runtime evidence that the VM did not provide
-->

# FreeBSD Exampleageddon and library qualification

- Snapshot date: 2026-10-12
- Guest: FreeBSD 15.1-RELEASE-p4, amd64
- Compiler: FreeBASIC 1.20.4-3, native FreeBSD x86_64 build
- Test context: VM134 `codex-freebsd`, 2 virtual CPUs, 4 GiB RAM

The package names below are the FreeBSD `pkg` names installed or checked in
the ports catalogue on the snapshot date. Package availability and versions
can change with the ports repository.

## Exampleageddon result

Exampleageddon covered 1,767 `.bas` files. It compiled 1,576 files.
The 653-example self-contained gate passed in full, with no self-contained
failures, run failures, or run timeouts. The sweep exited successfully with
`--fail-on-self-contained`.

Of the 309 examples classified as external-library examples, 248 compiled and
linked. The other 61 failed to compile or link. Exampleageddon skips that class
at runtime, so a successful link alone does not qualify a library's runtime
behavior. The report and CSV from the final sweep are kept on the VM under
`out/exampleageddon-freebsd-final-20261012-complete/`.

The remaining 191 compile failures across the full corpus include sources for
other operating systems, helper modules, and optional or legacy APIs. See the
CSV for each example and its compiler log.

The sweep can be reproduced from the staged source tree with:

```sh
FBROOT=/srv/compile-testing/fbc-exampleageddon-20261012
python3 "$FBROOT/build_scripts/exampleageddon-freebasic.py" \
  --root "$FBROOT" \
  --fbc "$FBROOT/bin/fbc" \
  --include-dir "$FBROOT/inc" \
  --prefix "$FBROOT" \
  --target-os freebsd \
  --example-profile "$FBROOT/build_scripts/exampleageddon-freebsd.json" \
  --outdir "$FBROOT/out/exampleageddon-freebsd-final-20261012-complete" \
  --jobs 2 \
  --compile-timeout 60 \
  --run-timeout 15 \
  --fail-on-self-contained
```

The profile adds `/usr/local/lib/mariadb` for
`examples/database/mysql_test.bas`. FreeBSD's MariaDB Connector/C package
places `libmariadb.so` in that directory.

## Libraries compiled and linked by Exampleageddon

These rows summarize third-party package families represented in the
compile/link sweep. They do not show runtime proof. The manual pages list
FreeBSD only for libraries with a successful example run described below.

| Area | FreeBSD packages | Example coverage |
| --- | --- | --- |
| Databases | `sqlite3`, `mariadb-connector-c`, `postgresql16-client` | SQLite 3, MariaDB Connector/C, and PostgreSQL client examples |
| Networking and HTTP | `curl`, `fcgi-devkit` | cURL CHttp and FastCGI examples |
| XML and compression | `expat`, `libxml2`, `libzip`, `lzo2`, `bzip2`; zlib is in the base system | Expat, libxml2, ZIP, LZO, bzip2, and zlib examples |
| GTK and drawing | `gtk2`, `gtk3`, `glib`, `cairo`, `goocanvas2`, `libglade2` | 60 GTK-family examples, including GTK 2/3 tutorials, GLib, Cairo, GooCanvas 2, and libglade |
| Images and fonts | `devil`, `freeimage`, `libgd`, `jpeg-turbo`, `png`, `freetype2`, `jasper`, `libmng` | DevIL, FreeImage, GD, JPEG, PNG, and FreeType examples |
| SDL 1.2 | `sdl`, `sdl_image`, `sdl_gfx`, `sdl_mixer`, `sdl_net`, `sdl_pango`, `sdl_ttf`, `sdl_sound` | Core SDL and the listed image, graphics, mixer, network, Pango, TrueType, and sound bindings |
| SDL 2 | `sdl2`, `sdl2_image`, `sdl2_gfx`, `sdl2_mixer`, `sdl2_net`, `sdl2_ttf`, `sdl2_sound` | Core SDL2 and the listed image, graphics, mixer, network, TrueType, and sound bindings |
| SDL 3 | `sdl3`, `sdl3_image`, `sdl3_mixer`, `sdl3_net`, `sdl3_ttf` | SDL3 core and the listed image, mixer, network, and TrueType examples |
| Graphics and physics | `allegro`, `allegro5`, `freeglut`, `glfw2`, `grx`, `ode-double` | Allegro 4/5, OpenGL/GLU/GLUT, legacy GLFW 2, GRX, and ODE examples |
| Math, language, and support | `aspell`, `gmp`, `gsl`, `libffi`, `lua53`, `pcre`, `libtre`, `cunit`, `libcaca`; curses is in the base system | Aspell, GMP, GSL, libffi, Lua 5.3, PCRE, TRE, CUnit, libcaca, and curses examples |
| Audio | `openal-soft`, `freealut`, `libopenmpt`, `portaudio` | OpenAL Utility Toolkit, libopenmpt, and PortAudio examples |

The FreeBASIC bindings need a few FreeBSD-specific details:

- `inc/crt/sys/freebsd/types.bi`, `inc/crt/sys/freebsd/select.bi`, and
  `inc/crt/sys/freebsd/time.bi` describe the FreeBSD C ABI. The `fd_set` and
  `timeval` sizes were checked against the guest headers and by a FreeBASIC
  smoke program.
- `inc/crt/iconv.bi` enables the POSIX iconv interface on FreeBSD, which SDL 1
  and SDL 2 examples use.
- `inc/mysql/mysql.bi` links the FreeBSD MariaDB Connector/C library as
  `mariadb`; the Exampleageddon profile adds its package-specific library path.
- The GSL binding links both `gsl` and `gslcblas` on FreeBSD.
- The Lua 5.3 binding uses FreeBSD's `lua-5.3` linker name.
- Aspell and DevIL use FreeBSD's system libc++ runtime through the `c++`
  linker name.
- GRX on FreeBSD links `grx20X`, X11, and PNG.
- The GooCanvas examples select GTK 3 on FreeBSD, matching the `goocanvas2`
  package.
- The FreeBSD run linked the GooCanvas and libglade examples. Three GTK examples
  still fail: FB_Calc and two OpenGL button demos.

## Runtime checks

These repository examples were compiled with the staged native compiler and
run on the FreeBSD guest. Their per-example logs are under
`out/freebsd-third-party-runtime-20261012/logs/`; the libzip smoke artifacts
are under `out/freebsd-third-party-runtime-20261012/zip-smoke/`.

- SQLite: `examples/database/sqlite3_test.bas` opened `:memory:`, reported
  SQLite 3.53.4 and returned `4` for `2+2`.
- Lua 5.3: `examples/other-languages/Lua/minmax.bas` loaded `minmax.lua` and
  returned the expected minimum and maximum.
- GSL: `examples/manual/libraries/gsl1.bas` and `gsl2.bas` produced the
  expected power, hypotenuse and matrix results.
- libffi: `examples/manual/libraries/ffi/helloworld.bas` and `closures.bas`
  called the example callback and returned `42`.
- GMP: `examples/manual/libraries/gmp.bas` computed and printed `2^65536`.
- LZO: `examples/manual/libraries/lzo.bas` completed its compression and
  decompression round trip.
- Expat and libxml2: `examples/xml/expat.bas` and `libxml.bas` parsed a
  controlled XML fixture.
- zlib: `examples/compression/zlib.bas` passed its checks. It reported the
  expected header/runtime version difference because FreeBSD's runtime is
  newer than the bundled binding's header version.
- bzip2: `examples/compression/bz2compress.bas` wrote a compressed fixture.
- libzip: `examples/manual/libraries/zip2.bas` created an archive and
  `examples/compression/libzip.bas` extracted both entries; both outputs were
  byte-compared with the expected text. `ldd` showed `libzip.so.5` from
  `/usr/local/lib`.
- GD: `examples/files/GD/gd.bas` wrote its GIF, PNG and JPEG outputs.
- CUnit: `examples/misc/CUnit/simple.bas` passed both tests.
- PCRE: `pcredemo.bas` and `preg_class.bas` produced their expected matches.
- TRE: `match.bas`, `replace.bas` and `subexpr.bas` produced their expected
  match and replacement results.
- GLib: `g_Actions.bas`, `g_Arguments.bas`, `g_Assert.bas`, `g_Filenames.bas`,
  `g_HashTable.bas` and `g_SplitArguments.bas` exited successfully.
  `g_VarArgMacros.bas` intentionally calls `g_error` and aborts; it is not
  counted as a passing example.
- cURL: `examples/manual/libraries/curl.bas` fetched a response from a
  temporary loopback HTTP server.
- Aspell: `examples/manual/libraries/aspell.bas` returned suggestions with
  the `en_US` dictionary installed.
- DevIL: `examples/manual/libraries/il.bas` read `fblogo.bmp` and wrote a
  copy in the test directory.
- SDL 1.2: `examples/graphics/SDL1/core/video_info.bas` exited under Xvfb; its
  driver-name field was blank in this headless setup.
- SDL 3: the core template, image, TrueType and network examples exited under
  Xvfb or the local resolver. The tests used bounded frame counts for GUI
  examples.
- GTK 2: the manual `examples/manual/libraries/gtk.bas` example linked GTK 2,
  opened and exited under Xvfb. The synthetic window close emitted a GDK
  warning while destroying the window, so interactive desktop behavior is
  not established.

GTK 3's custom-widget example stayed in its event loop after a synthetic close.
The GooCanvas arrowhead example opened without the earlier invalid boolean
property warning after its FreeBASIC varargs were corrected, but the headless
close test did not finish cleanly. SDL 2 and Allegro 4 event-loop tests did not
complete, Allegro 5 segfaulted under Xvfb, and Cairo and FreeImage screen
examples segfaulted in the same FreeBASIC graphics path. These libraries are
not listed as runtime-proven here. The VM had no physical audio device, so no
audio-device behavior was tested. MariaDB and PostgreSQL examples only
received compile/link qualification because no database servers were
configured.

## Packages or bindings still needing separate work

- IUP was not present in the FreeBSD package catalogue checked for this run;
  its examples still lack `-liup` and the GTK 2 Pango compatibility library.
- wxWidgets is available as `wx32-gtk3`, but the examples use a separate
  `-lwx-c-0-9-0-2` shim that was not available.
- FreeBSD's `glfw` and `glfw2` packages install the same
  `/usr/local/lib/libglfw.so`; only one can be installed at once. This run used
  `glfw2` for the legacy NeHe examples.
- SDL GPU, SDL FontCache, SDL BGI, SDL RTF, SDL3 graphics, and SDL3
  shader-cross and SDL3 sound examples still lack matching linkable libraries
  in the checked package set.
- The PDFlib package exists in the FreeBSD catalogue, but it was not installed
  or qualified. Resolve its licensing requirements before using it.
- The SpiderMonkey examples target an archived 1.6 API. Newer JavaScript
  engine packages do not establish compatibility with that binding.
- The `cryptlib` package exists in the catalogue, but it remains unqualified
  pending review of its license terms.
- BASS/BASSMOD, FMOD, QuickLZ, TinyPTC, and CGUI remain unqualified on this
  FreeBSD setup.

`freealut` is packaged but deprecated by its port. Its example linked after
installation; this is not a recommendation for new audio code.

<!-- end of doc/freebsd-example-qualification.md -->
