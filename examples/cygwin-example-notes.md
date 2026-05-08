# Cygwin example compile notes

Project: FreeBASIC Cygwin example sweep

Purpose: Record examples that do not compile from the packaged Cygwin build
without optional external libraries.

This file intentionally does not describe runtime behavior.  The sweep links
the examples with the staged Cygwin compiler, but it does not run GUI, audio,
database, networking, or scripting examples.

## Summary

The Cygwin sweep was run with `.build-cygwin/stage/usr/bin/fbc.exe`.  Core
examples now compile, including `network/http-get.bas`,
`manual/procs/mydll.bas`, and `threads/timer-lib/timer.bas`.

The remaining out-of-box failures are external library issues.  The final
sweep reported 173 `Failed to compile` entries plus a small number of explicit
make target failures, all caused by missing `-l...` libraries rather than
Cygwin compiler, runtime, or header failures.

## External-Library Failures

| Examples | Out-of-box status |
| --- | --- |
| `examples/compression/bz2compress.bas`, `libzip.bas`, `QuickLZ.bas`, `zlib.bas` | Do not compile out of the box; missing compression libraries such as `bz2`, `zip`, `z`, or `quicklz`. |
| `examples/console/caca/*.bas`, `examples/console/curses/curses.bas` | Do not compile out of the box; missing console libraries such as `caca` or `pdcurses`. |
| `examples/database/*.bas` | Do not compile out of the box; missing database client libraries such as `mysqlclient`, `pq`, `sqlite`, or `sqlite3`. |
| `examples/files/DevIL`, `FreeImage`, `GD`, `GIFLIB`, `jpeglib`, `libpng`, and `pdflib` examples | Do not compile out of the box; missing image/file libraries such as `IL`, `freeimage`, `gd`, `gif`, `jpeg`, `png`, or `pdf`. |
| `examples/graphics/Allegro*`, `cairo`, `FreeType`, `grx`, `OpenGL`, `SDL`, and `tinyptc` examples | Do not compile out of the box; missing graphics libraries such as Allegro, Cairo, FreeType, GRX, GLU/GLUT/GLFW, SDL, or TinyPTC. |
| `examples/GUI/CGUI`, `GTK+`, `IUP`, and `wx-c` examples | Do not compile out of the box; missing GUI toolkit libraries such as CGUI, GTK/GooCanvas, IUP, or wx-c. |
| `examples/manual/libraries/*` optional binding examples | Do not compile out of the box when the corresponding external library is not installed.  This includes libraries such as aspell, BASS/BASSMOD, cryptlib, FastCGI, FreeImage, GSL/GMP, IUP, SpiderMonkey, and others. |
| `examples/math/big_int`, `cryptlib`, `GSL`, `Newton`, and `ODE` examples | Do not compile out of the box; missing the matching math/physics/crypto libraries. |
| `examples/misc/CUnit`, `glib`, and `gdsl` examples | Do not compile out of the box; missing the corresponding optional libraries. |
| `examples/network/curl/CHttp/test.bas` | Does not compile out of the box; missing `curl`. |
| `examples/regex/PCRE` and `regex/TRE` examples | Do not compile out of the box; missing `pcre` or `tre`. |
| `examples/sound/BASS`, `FMOD`, `OpenAL`, and `portaudio-sine.bas` | Do not compile out of the box; missing audio libraries such as BASS, FMOD, OpenAL/ALUT, or PortAudio. |
| `examples/xml/expat.bas` and `examples/xml/libxml.bas` | Do not compile out of the box; missing `expat` or `xml2`. |

These should be treated as optional Cygwin package/backlog items.  They are not
current evidence of a broken Cygwin compiler or core FreeBASIC distribution.
