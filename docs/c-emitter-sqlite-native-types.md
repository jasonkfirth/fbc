<!--
Project: FreeBASIC C-backend verification
File: c-emitter-sqlite-native-types.md
Purpose: Record SQLite native integer declarations and their qualification.
Responsibilities: Bind the library repair to source, target and native evidence.
-->

# SQLite native C integer types

The maintained `fb_src/sqlite3_types.bi` header in the requested SQLite port
now uses the SDK's `CLong`, `CULong` and `size_t` declarations. Native C type
identity matters even when integer widths match. SQLite's fixed-width
`sqlite3_int64`, `sqlite3_uint64`, `i64` and `u64` aliases retain their explicit
64-bit representations.

The development compiler remains `acc4a892`. The library repair is in the local
audited checkout at `build/c-emitter-forward-views-20261105/github-projects/freebasic-sqlite`,
based on `c7e29f0daf37a5880b63c7778a8f8b93055cdb5d`. Its portable patch and source
receipt accompany the qualification. Publication to the GitHub port remains
pending.

## Complete production graphs

| Target | Make-selected units | GCC before | GCC after | Clang before | Clang after |
| --- | ---: | ---: | ---: | ---: | ---: |
| Linux x86-64 | 555 | 111 | 23 | 7,784 | 7,696 |
| Windows x86-64 | 548 | 23 | 23 | Separate queue | Separate queue |

All 1,103 units are emitted again through the selected compiler. Every original
FBC warning line is preserved. The Make source lists and production flags match
the pre-repair recipes. Native Linux GCC 14.2 and Clang 19.1.7 use the original
aggressive diagnostic flags. Windows GCC 16.2 uses its original flags.

The repair removes all 88 GCC builtin-declaration mismatches. These comprise
47 `memset`, 27 `memcpy`, six `memmove`, five `memcmp`, two `strlen` and one
`strncmp` observations. Clang removes 88 incompatible-library declarations.
Its remaining such observation is the Unix port's `strerror` declaration.

All 555 Linux GCC objects match their preceding bytes exactly. Clang retains
554 exact object identities. Its `sqlite3_bulk_where_begin.bas` object shrinks
by 48 bytes after the corrected declarations enable builtin optimization.
An isolated `-fno-builtin` comparison produces identical before/after objects
and disassembly for that unit. The complete diagnostic gates retain their
original flags.

The remaining 23 GCC observations on each target comprise eighteen possible
null dereferences, three qualifier casts and two containing-object tail-clear
bounds observations. They remain open. Coreutils retains 776 GCC observations,
including 702 nonliteral formats across 116 units. Reviewed examples include
translated-message temporaries and runtime format construction.

## Native regression entry point

From `fb_src`, run `make check-c-abi-types FBC=/path/to/fbc CC=/path/to/cc`.
The compiler and native C tool must target the same host. The Python helper
also accepts `--target`, repeated `--fbc-arg` and `--cc-arg` options, and an
optional executable runner for an explicitly configured cross-target check.

The BASIC fixture exports five identity functions. The C observer supplies
independent `long`, `unsigned long`, `size_t` and fixed-width prototypes before
including the emitted definitions. It checks zero, signed extrema, unsigned
maxima and the high size bit. C owns startup, allowing the Win32 check to run
with the available native C toolchain.

Both C producers pass on Windows and Linux, at 32 and 64 bits, through both
native C compilers. Sixteen native executions pass. Sixteen controls using the
original header reject incompatible C prototypes. The ordinary Make target
also passes through the selected SDK compiler. Strict source-only lint of the
header and fixture reports zero findings on Windows and Linux.

## Evidence and resource bounds

The existing `build/c-emitter-forward-views-20261105` area contains the
`sqlite-native-types-*-v1-20261114` emission databases, native Linux result,
boundary-value controls, object-difference inspection, portable patch and
qualification receipt. Original and repaired C bytes are compressed and
hash-pinned. One temporary C/object slot is reused by each worker. Linux
compilation runs serially at reduced priority within a 128 MiB owned-data cap.

Installed release binaries and runtime archives retain their recorded hashes.
The broader warning-cleanup goal remains active.

<!-- end of c-emitter-sqlite-native-types.md -->
