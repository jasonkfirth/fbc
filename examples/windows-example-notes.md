# Windows example compile notes

Project: FreeBASIC Windows example sweep

Purpose: Record examples that still do not compile in the packaged Windows
MinGW sweep after staging the available redistributable win32 libraries.

This file intentionally does not describe runtime behavior.  The sweep links
the examples, but it does not run GUI, audio, database, or scripting examples.
Runtime DLL availability is tracked separately in
`contrib/winlibs-legacy/runtime-dlls.tsv`.

## Win32

The win32 sweep now links the shipped example set.  DOS examples are skipped by
the Windows makefile rule, and the documented intentional-error examples are
excluded by `examples/GNUmakefile`.

The old `examples/misc/libjit` programs were removed from the shipped example
tree because they depended on GNU libjit and no clean MSYS2 MinGW package or
official Windows import-library/runtime bundle was found during this pass.

## Runtime-Only Gaps

Several old win32 import libraries now allow examples to compile, but their
runtime DLLs were not staged because the official FreeBASIC winlibs packages
did not include them.  That includes BASS, FMOD, Allegro 4, CGUI, FastCGI,
Newton, SQLite, SpiderMonkey's `js32.dll`, and wx-c.  Keep those examples in
the compile sweep, but do not claim they are runnable from a clean install
until the DLLs have a clean redistributable source.

## Win64 Backlog

The win64 sweep now links the examples that could be restored from clean
source builds: QuickLZ, SQLite 2, big_int, DispHelper, FastCGI, GDSL, TinyPTC,
IUP, and the MSYS2-provided ODE/GD paths.  The full sweep log is
`/tmp/fb-example-sweep/win64-examples-forced6.log`; TinyPTC was staged after
that sweep had already passed the graphics group, so the current failure list
comes from `/tmp/fb-example-sweep/win64-failed-recheck.log`.

| Example | Status |
| --- | --- |
| `examples/files/pdflib/test.bas` | Fails at link time: `cannot find -lpdf`.  The old PDFlib API is tied to a legacy third-party package and no clean 64-bit redistributable library was found. |
| `examples/graphics/Allegro/*.bas` | Fails at link time: `cannot find -lalleg`.  These examples use Allegro 4, while current MSYS2 ships Allegro 5. |
| `examples/GUI/CGUI/hello.bas` | Fails at link time: `cannot find -lalleg` and `cannot find -lcgui`.  CGUI depends on the old Allegro 4 stack. |
| `examples/graphics/grx/grx.bas` | Fails at link time: `cannot find -lgrx20`.  No clean win64 GRX 2.x package was found. |
| `examples/GUI/wx-c/*.bas` | Fails at link time: `cannot find -lwx-c-0-9-0-2`.  The examples target the old wx-c wrapper, not current wxWidgets. |
| `examples/manual/libraries/bass.bas` and `examples/sound/BASS/*.bas` | Fail at link time on `-lbass` or `-lbassmod`.  BASS/BASSMOD are proprietary legacy audio SDKs, so they were not bundled from random DLL copies. |
| `examples/manual/libraries/fmod*.bas` and `examples/sound/FMOD/mp3-player.bas` | Fail at link time: `cannot find -lfmod`.  These use the old FMOD 3 API, not the current FMOD SDK. |
| `examples/manual/libraries/cryptlib.bas` and `examples/math/cryptlib/hashing.bas` | Fail at link time: `cannot find -lcl`.  cryptlib is source-available but has a stricter license and a larger native build surface, so it was left for a deliberate packaging decision. |
| `examples/manual/libraries/spidermonkey*.bas` | Fail at link time: `cannot find -ljs`.  The binding is for the old SpiderMonkey/js32-era API. |
| `examples/math/Newton/test.bas` | Fails at link time: `cannot find -lNewton`.  The available modern Newton Dynamics packages do not directly match the old example ABI. |
Those are packaging/pruning backlog items rather than compiler regressions.
They should only be restored to the clean win64 sweep if we find or build a
clean 64-bit import-library/runtime set that matches the shipped bindings.
