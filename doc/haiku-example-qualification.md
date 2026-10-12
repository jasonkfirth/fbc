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
