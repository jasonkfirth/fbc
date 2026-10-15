<!--
    Project: FreeBASIC compiler
    File: doc/dos-example-qualification.md

    Purpose:

        Record DOS Exampleageddon results and the third-party libraries
        available to FreeBASIC examples in the DOSBox-X qualification setup.

    Responsibilities:

        - record the full DOSBox-X Exampleageddon receipt set
        - distinguish source, link, and runtime evidence
        - list DOS library packages and their qualification level

    This file intentionally does NOT claim physical-PC or universal DOS
    compatibility from emulator results.
-->

# DOS Exampleageddon and library qualification

- Snapshot date: 2026-10-13
- Emulator: private DOSBox-X 2026.10.01 instance
- DOS version: 7.1, long filename support enabled
- Emulated CPU: Pentium II, dynamic x86 core, maximum cycles
- Compiler package: FreeBASIC 1.20.4 DOS candidate, built with the private DJGPP 3.4 toolchain
- Example compile flags: `-target dos -arch 686 -fpu x87 -w all -exx -mt -dos-threads pdmlwp`
- Test artifacts: `out/exampleageddon-dos-20261012/`

The full sweep covered all 1,767 example IDs in 45 completed batches. Each
batch has a `RESULT.TSV` and a `DONE.TXT` receipt under
`out/exampleageddon-dos-20261012/evidence/sweep/latest-batches/`. Across the
raw receipts, 1,347 examples compiled and linked and 420 did not. The raw
category counts are:

| Category | Rows | Compiled and linked | Did not compile or link |
| --- | ---: | ---: | ---: |
| External library | 313 | 22 | 291 |
| Graphics-interactive | 18 | 17 | 1 |
| Helper module | 34 | 22 | 12 |
| Intentional failure | 17 | 9 | 8 |
| Interactive | 499 | 497 | 2 |
| Long-running benchmark | 3 | 3 | 0 |
| Network | 3 | 3 | 0 |
| Platform-specific | 162 | 56 | 106 |
| Self-contained | 652 | 652 | 0 |
| sfxlib audio | 66 | 66 | 0 |

All 652 self-contained examples compiled, linked, and returned status 0 at
runtime. The DOS classifier excludes libffi-backed `THREADCALL` from that
category because the DOS runtime does not provide it. The 420 raw compile or
link failures include unavailable third-party packages, platform-specific
examples, helper modules, and intentional failure examples; this is not a
claim that all 1,767 examples are portable to DOS.

The sweep captured two DOS-applicable source defects that were fixed and
checked in focused DOSBox-X retries. Example 1025 now compiles and links after
its DOS extern aliases were corrected for FreeBASIC's leading C symbol
prefix; its embedded-data object exports `_binary_hello_txt_start/end`. The
retry did not launch this interactive example. Example 1043 now compiles and
links after its DOS timer rollover checks were corrected; it also was not
launched. Raw example 1667 fails because `timer.bi` explicitly rejects DOS
with `#error "Unsupported platform"`; the current classifier now marks this
example platform-specific. R006 rebuilt both corrected examples from the
current worktree source: both artifacts were present and neither interactive
program was launched. The original full-sweep receipts remain unchanged.

The full sweep's external-library examples are compile/link checks and are not
launched by Exampleageddon. The three network examples also compiled and
linked with Watt-32 enabled, but were not run.

The DXE checks exposed and fixed two source/build issues. `hLinkDosDxe` now
uses the import-library basename, and the DOS package preparation patches the
DJGPP linker script with the underscore-prefixed profiling section bounds
required by the COFF runtime. A 4-byte section probe linked with those bounds.
The private Watt-32 compiler candidate then linked both DOS DXE consumers
without adding `#inclib "watt"` to the examples. In B908, `dylib.bas` loaded
`mydll.dxe`, resolved the DOS alias `_AddNumbers`, called it, and exited 0.

The import-library consumer `test.bas` also compiled and linked in B908, but
the DOSBox-X run exited with error 14 because `mydll.DXE` could not resolve
`_fb_ErrorSetModName`. The DOS manual already documents that the multithreaded
runtime is not exported to DXEs. This is a known `-mt` runtime limit, not a
successful import-library runtime qualification.

The full 1,767-row sweep used the options above with the private compiler
candidate built with Watt-32, PDMLWP, DPMI yield, and low-power graphics
enabled. Its private compiler candidate has SHA-256
`57FB3BE420C648620872FEED7DBBE9B856386990EBC482E2DEFAF8FEFCD25BBD`. No
network traffic or physical PC was tested. The installed `bin/fbc.exe` was
not replaced.

The provider-enabled full sweep compiled and linked all three network
examples. The DOS branch in `examples/network/http-get.bas` uses the
FreeBASIC `OPEN TCP` file device instead of the Unix socket bindings, which
explicitly reject DOS. The two `examples/network/opentcp/` examples now compile
their TCP code on DOS instead of compiling only their former unsupported-target
message. The verbose link commands include `-lwatt`. These checks do not run
the programs; packet-driver networking and `WATTCP.CFG` behavior remain
unqualified.

## Qualification levels

| Status | Evidence |
| --- | --- |
| Runtime verified | A targeted DOSBox-X program exercised the library API and checked its output. This is emulator evidence only. |
| Linked, runtime not tested | The matching Exampleageddon example compiled and linked against a staged DOS library, but the sweep does not run external-library examples. |
| Not available in this SDK | The current DJGPP setup has no compatible package, or the example failed to compile or link. A binding file alone does not qualify the library. |
| Not tested | No targeted DOS runtime evidence or successful corpus link has been recorded. |

## Third-party libraries

The following libraries have targeted runtime evidence so far:

| Library | DOS library version | Evidence |
| --- | --- | --- |
| bzip2 | 1.0.8 | The DOSBox-X compression example produced a 710-byte `.bz2` fixture. Host-side decompression recovered the original bytes. |
| QuickLZ | 1.5.0 | Level 1 compressed an 8 KiB input to 408 bytes and decoded it back byte-for-byte. The DJGPP decoder was built with `QLZ_MEMORY_SAFE`. |
| zlib | 1.3 | Runtime self-tests, stream operations, and gzip file checks passed. The binding declares 1.2.8, so the expected runtime-version warning was reported. |

The corpus has also linked examples against these staged DOS libraries. The
full corpus sweep skips running external-library examples, so these rows have
compile/link evidence only:

| Library | DOS library version | Example |
| --- | --- | --- |
| Allegro 4 | 4.2.2 | `examples/graphics/Allegro/` |
| Curses | PDCurses 3.9 | `examples/console/curses/curses.bas` |
| Expat | 2.0.0 | `examples/manual/libraries/expat.bas` |
| JPEG | 9e | `examples/files/jpeglib/example.bas` |
| GRX | 2.4.9 | `examples/graphics/grx/grx.bas` |
| libpng | 1.6.40 | `examples/files/libpng/png.bas` |
| Lua | 5.2.2 | `examples/manual/libraries/lua.bas` |
| PCRE1 | 8.42 | `examples/regex/PCRE/` |

Watt-32 is available in the staged DJGPP library set. The compiler must be
built with `DOS_TCP_PROVIDER=watt32` for the DOS driver to add `-lwatt`
automatically. A real DOS machine also needs a compatible packet driver and a
matching `WATTCP.CFG`. Neither packet-driver networking nor socket runtime
behavior was tested in DOSBox-X, so Watt-32 is available for opt-in builds but
is not runtime-qualified here.

The complete package and binding status, including failed links and libraries
without a DOS package in this setup, is summarized in
`doc/manual/cache/DOSLibrarySupport.wakka`. The `ExtLib*.wakka` platform lists
include DOS for bzip2, QuickLZ, and zlib after runtime checks, alongside the
built-in DOS API and DJGPP C runtime. The support page records compile/link-only
results and missing packages. Do not infer DOS availability from a `.bi` file:
the target also needs a compatible DJGPP library and any runtime assets.

The emulator setup, full corpus outcomes, and every batch receipt are held
separately from the normal compiler installation. This candidate qualification
does not replace or overwrite the installed `bin/fbc.exe`.
