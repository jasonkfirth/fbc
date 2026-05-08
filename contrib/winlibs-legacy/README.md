Legacy Winlibs Import Libraries
===============================

This directory contains a curated set of Windows import/static libraries.
Most 32-bit libraries were copied from the old FreeBASIC Windows library
packages at:

    https://sourceforge.net/projects/fbc/files/Older versions/0.90.1/Binaries - Windows/Libraries/

It also contains directly relevant runtime DLL/import-library pairs from
official upstream binary packages, such as FreeImage, IUP, SDL, and selected
archived MSYS2 GTK packages, where those DLLs are needed by examples and have a
clear redistributable source.  A few 64-bit static libraries are rebuilt from
upstream source when that is cleaner than carrying an obsolete DLL dependency,
such as big_int 1.0.7, GDSL 1.8, QuickLZ 1.5.0, SQLite 2.8.17, and
TinyPTC 0.8.
The 64-bit DispHelper archive is rebuilt from the upstream single-file source
so the manual COM examples can link without depending on an old 32-bit archive.
The 64-bit FastCGI archive is rebuilt from the archived 2.4.1 snapshot with a
small pointer-sized handle patch kept under patches/ so the old Win32 backend
does not truncate handles when built for Win64.
The 64-bit TinyPTC archive is rebuilt with its old 32-bit MMX assembly path
disabled and GCC-friendly fixed-width integer typedefs.

These files are kept separately from the normal runtime/library layout so the
MSYS2 package builder can opt in to them later without confusing them with
libraries built by this tree or installed by MSYS2.

Only archives that matched current example build failures were copied.  Most
matching runtime DLLs are not included here because the official FreeBASIC
library zips inspected for this directory did not contain them.  These import
libraries are enough to let examples compile and link when the DLL is supplied
elsewhere, but they do not make those old third-party DLLs redistributable by
themselves.

The runtime DLL names imported by these archives are listed in
runtime-dlls.tsv.  Local and online searches found only partial copies of the
older BASS, FMOD, Allegro 4, CGUI, Newton, SQLite, SpiderMonkey, and wx-c DLLs
in unrelated game or application install trees, not in the official
FreeBASIC library zips, so those DLLs were not staged from those sources.  The
SpiderMonkey import library is kept from the old FreeBASIC 0.18.5 package so
the examples can still be compiled, but the imported js32.dll still needs a
clean redistributable source before it can be bundled.  The staged GTK
libraries come from the MSYS2 mingw/i686 archive because recent pacman metadata
does not always offer the 32-bit GTK stack even though archived packages remain
available.  If redistributable DLLs are added later, place the 32-bit files
under bin/win32 and the MSYS2 package builder will bundle them alongside the
win32 toolchain.

No matching 64-bit import libraries for the remaining legacy third-party
examples were found in the referenced FreeBASIC 1.10.1 winlibs tree.  Selected
64-bit static libraries were rebuilt from source where the old API still
matched the shipped bindings.

See manifest.tsv for file sizes and SHA-256 hashes.
