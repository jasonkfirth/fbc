<!--
    Project: FreeBASIC compiler
    File: doc/haiku-example-qualification.md

    Purpose:

        Record the native Haiku Exampleageddon result and the third-party
        libraries available to FreeBASIC examples on the qualification VM.

    Responsibilities:

        - identify the tested Haiku and compiler versions
        - separate compile/link evidence from runtime evidence
        - list available libraries and known compatibility gaps

    This file intentionally does NOT claim interactive or physical-device
    acceptance for examples that were only compiled.
-->

# Haiku Exampleageddon and library qualification

- Snapshot date: 2026-10-12
- Guest: Haiku R1 beta 6, `hrev59866+79`, amd64
- VM: VM135 `codex-haiku`, 2 virtual CPUs, 4 GiB RAM
- Compiler: FreeBASIC 1.20.4-3, native `haiku-x86_64` build
- Compiler FFI support: enabled

Package availability and versions below describe the HaikuPorts repository
checked on the snapshot date. They can change as HaikuPorts is updated.

## Exampleageddon result

The native sweep covered all 1,767 `.bas` files. It compiled 1,487 and did
not compile 280. The 653 examples classified as self-contained all compiled
and ran: there were no self-contained failures, run failures, or timeouts.
The sweep exited successfully with `--fail-on-self-contained`.

The 280 compile failures include platform-specific sources, helper modules,
legacy APIs and libraries that are absent or incompatible with the installed
SDK. Exampleageddon skips failed compiles and skips external-library,
interactive, graphics, network, benchmark and audio classes at runtime. Thus
the zero exit is the complete self-contained gate, not proof that every
third-party example runs on Haiku.

Of 309 examples classified as external-library examples, 156 compiled in the
full sweep and 153 did not. After that sweep, a focused rerun with the shared
source directory transferred correctly compiled both PCRE CRegex sources;
the full-sweep totals above remain the totals in the saved report. The full
report and CSV are in the qualification workspace under
`out/exampleageddon-haiku-vm135/run-final/`.

The runner can be repeated from the staged source tree with the native
compiler and the same Haiku package profile:

```sh
python3 build_scripts/exampleageddon-freebasic.py \
    --root "$PWD" \
    --fbc "$PWD/bin/fbc" \
    --include-dir "$PWD/inc" \
    --prefix "$PWD" \
    --target-os haiku \
    --outdir out/exampleageddon-haiku-vm135/run-final \
    --jobs 2 \
    --compile-timeout 60 \
    --run-timeout 15 \
    --fail-on-self-contained
```

Keep build and Exampleageddon concurrency at two jobs on this VM. Runtime
passes apply only to the self-contained class; the other classes were not
used as GUI, audio-device, network-service, or hardware acceptance tests.

## Libraries with compile or link evidence

Each count is the number of examples compiled and linked in the full corpus
for that library family. It does not imply complete API coverage or runtime
qualification. The PCRE row includes the later focused rerun noted above.

| Area | HaikuPorts development packages | Exampleageddon evidence |
| --- | --- | --- |
| Databases | `libmysqlclient_devel`, `postgresql18_devel`, `sqlite_devel` | MySQL 1/1, PostgreSQL 1/1 and SQLite 3 1/1 compiled and linked. SQLite 2 is unavailable. |
| HTTP, XML and compression | `curl_devel`, `expat_devel`, `libxml2_devel`, `bzip2_devel`, `libzip_devel`, `zlib_devel` | cURL 1/1, Expat 1/1, libxml2 1/1, bzip2 1/1, libzip 1/1 and zlib 1/1 compiled and linked. |
| Images and fonts | `freeimage_devel`, `gd_devel`, `libjpeg_turbo_devel`, `libpng16_devel`, `freetype_devel`, `cairo_devel` | FreeImage 2/2, GD 1/1, JPEG 1/1, PNG 1/1, FreeType 2/2 and Cairo 3/3 compiled and linked. |
| OpenGL | `mesa_devel`, `glu_devel`, `glfw_devel` (GLFW 3) | 33/34 OpenGL-family sources compiled and linked. One legacy NeHe example needs GLFW 2's `glfwOpenWindow`, which is not provided by the installed GLFW 3 library. |
| SDL 1.2 | `libsdl_devel`, `sdl_gfx_devel`, `sdl_image_devel`, `sdl_mixer_devel`, `sdl_net_devel`, `sdl_ttf_devel` | 15/20 compiled and linked. SDL_FontCache, SDL_gpu, SDL_Pango, SDL_rtf and SDL_sound were missing. |
| SDL 2 | `libsdl2_devel`, `sdl2_gfx_devel`, `sdl2_image_devel`, `sdl2_mixer_devel`, `sdl2_net_devel`, `sdl2_sound_devel`, `sdl2_ttf_devel` | 7/11 compiled and linked. SDL2_bgi, SDL2_FontCache, SDL2_gpu and SDL2_rtf were missing. |
| SDL 3 | `libsdl3_devel`, `sdl3_image_devel`, `sdl3_ttf_devel` | 39/63 compiled and linked in the full external-library classification. Other samples need unavailable add-ons or APIs newer than the installed SDK. No SDL3 graphics, mixer or network development package was found. |
| GUI, utility and math | `gtk3_devel`, `glib2_devel`, `gsl_devel`, `ode_devel`, `lua5.3_devel`, `portaudio_devel`, `libcaca_devel`, `ncurses6_devel`, `bcunit_devel` | GLib 7/7, GSL 1/1, ODE 1/1, Lua 1/1, PortAudio 1/1, libcaca 2/2, curses 1/1 and CUnit 2/2 compiled and linked. Only 4 GTK-family sources compiled; many examples target GTK 2 APIs. |
| PCRE | `libpcre_devel` | The focused rerun compiled and linked `CRegex.bas` and `test.bas` after the runner began syncing their shared parent helper. |

## Libraries compiled and run on Haiku

Each row records a focused example run on the Haiku VM, in addition to a
successful native compile and link. These runs qualify the named interfaces
and input paths only; they do not establish full binding coverage.

| Library and HaikuPorts package | Example programs | Runtime result |
| --- | --- | --- |
| SQLite 3 (`sqlite_devel` 3.53.4.0-1) | `examples/database/sqlite3_test.bas` | Opened an in-memory database and returned `4` for `select 2+2`. SQLite 2 was unavailable. |
| Lua 5.3 (`lua5.3_devel` 5.3.6-5) | `examples/other-languages/Lua/minmax.bas` with `minmax.lua` | Loaded the script and reported the expected minimum and maximum. |
| GSL (`gsl_devel` 2.6-2) | `examples/manual/libraries/gsl1.bas`, `gsl2.bas` | Both numerical and matrix examples ran. The header now links `gslcblas` on Haiku because the package leaves CBLAS references for the application link step. |
| GMP (`gmp_devel` 6.3.0-1) | `examples/manual/libraries/gmp.bas` | Computed `2^65536`. |
| LZO (`lzo_devel` 2.10-3) | `examples/manual/libraries/lzo.bas` | Safe decompression restored the original bytes after compression. |
| Expat (`expat_devel` 2.9.0-1) | `examples/xml/expat.bas` | Parsed a controlled XML fixture through the streaming callbacks. |
| libxml2 (`libxml2_devel` 2.15.3-1) | `examples/xml/libxml.bas` | Parsed a controlled XML fixture through the text-reader API. |
| zlib (`zlib_devel` 1.3.2-1) | `examples/compression/zlib.bas` | Compression, inflate, dictionary and gzip checks passed; the example reported the expected runtime/header version difference. |
| bzip2 (`bzip2_devel` 1.0.8-3) | `examples/compression/bz2compress.bas` | Compressed the repository's `examples/fblogo.bmp` fixture. |
| libzip (`libzip_devel` 1.11.4-1) | `examples/manual/libraries/zip2.bas`, `examples/compression/libzip.bas` | Created and extracted both entries; extracted bytes matched their source text. |
| BCUnit (`bcunit_devel` 3.0.2-1) | `examples/misc/CUnit/simple.bas` | Two tests and five assertions passed. HaikuPorts supplies this CUnit-compatible API as `libbcunit`. |
| GD (`gd_devel` 2.3.3-8) | `examples/files/GD/gd.bas` | Generated non-empty GIF, PNG and JPEG files. |
| PCRE 1 (`libpcre_devel` 8.45-3) | `examples/regex/PCRE/pcredemo.bas`, `preg_class.bas` | Both regex examples ran; `preg_class.bas` used its sibling `match-progress.bi` helper. |
| GLib (`glib2_devel` 2.88.1-8) | `g_Actions.bas`, `g_Arguments.bas`, `g_Assert.bas`, `g_Filenames.bas`, `g_SplitArguments.bas` in `examples/misc/glib/` | The five named utility examples ran. Other GLib examples are not included in this runtime claim. |
| SDL 1.2 (`libsdl_devel` 1.2.15-19) | `examples/graphics/SDL1/core/video_info.bas` | SDL initialization and video-info queries completed in the headless VM. No window or rendering behavior was tested. SDL 2 and SDL 3 remain unqualified on Haiku. |
| cURL (`curl_devel` 8.22.0-3) | `examples/manual/libraries/curl.bas` | Fetched and checked the exact response from a local loopback HTTP fixture. |

Useful package versions in this snapshot include HaikuPorts `curl_devel`
8.22.0-3, `expat_devel` 2.9.0-1, `libxml2_devel` 2.15.3-1,
`libiconv_devel` 1.18-1, `libpcre_devel` 8.45-3, `freetype_devel`
2.14.3-1, `mesa_devel` 22.0.5-3, `bcunit_devel` 3.0.2-1,
`libsdl_devel` 1.2.15-19, `libsdl2_devel` 2.32.10-3 and `libsdl3_devel`
3.4.16-1.

## Compatibility fixes and focused checks

- Haiku's GNU libiconv exports `libiconv_open`, `libiconv` and
  `libiconv_close`; `inc/crt/iconv.bi` maps the portable declarations to
  those symbols. The Exampleageddon iconv API probe passed at runtime.
- Haiku uses 32-bit `fd_mask` words and a 1,024-descriptor `fd_set` (128
  bytes). Haiku-specific select and timeval declarations were added under
  `inc/crt/sys/haiku/`. Independent C ABI checks matched the FreeBASIC layout;
  select boundary and null checks passed.
- HaikuPorts provides the compatible CUnit API as `libbcunit`, so
  `inc/CUnit/CUnit.bi` selects `bcunit` on Haiku. Both CUnit examples linked.
- The Exampleageddon transfer path now includes sibling shared sources when
  individual examples are compiled and preserves generated remote artifacts
  between dependent commands. This fixed false failures for SDL, PCRE and
  FMOD source helpers. It does not supply libraries that HaikuPorts lacks.
- Focused checks also linked OpenGL/GLU samples, SDL 1.2 core OpenGL, cURL,
  and the local CHttp library. These are compile/link checks unless explicitly
  identified above as runtime checks.

## Packages or bindings still needing separate work

- The checked HaikuPorts repository had no development packages for SQLite 2,
  PDFlib, IUP, GTK 2, Allegro 5, TRE, Newton, GDSL, TinyPTC, CGUI or
  cryptlib. QuickLZ was also unavailable.
- Allegro 4's HaikuPorts build depends on X11 and did not link in this native
  Haiku setup. The Allegro 5 package was unavailable.
- GTK 3 is present, but the bulk of the GTK examples use older GTK 2 APIs;
  the four successful compiles do not qualify that family as a whole.
- OpenAL examples need `-lalut`, which was not present. The installed OpenAL
  package alone is insufficient for those examples.
- Aspell compiled but did not run because no `en_US` word list was installed.
  Installing HaikuPorts `aspell_en` failed when package refresh/download was
  denied, so Aspell is not runtime-qualified and is not listed as Haiku
  supported in its library page.
- BASS and FMOD are proprietary SDKs and were not available. The focused FMOD
  probe reached the expected missing `-lfmod` link failure.
- The libffi example did not compile because the current binding refers to
  the undefined `FFI_GNUW64` constant. FFI support in the native compiler was
  enabled, but that does not qualify this binding.

Interactive graphics, GUI, audio, networking, and physical-device behavior
remain unqualified by the headless self-contained gate. Use focused native
checks when adding a package or repairing one of these bindings, and record
compile/link, runtime, and device evidence separately.

<!-- end of haiku-example-qualification.md -->
