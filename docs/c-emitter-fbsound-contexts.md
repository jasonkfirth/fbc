<!--
Project: FreeBASIC C-backend verification
File: c-emitter-fbsound-contexts.md
Purpose: Record the completed FBSound owning-build and native warning checks.
Responsibilities: Bind Make recipes, target diagnostics and source coverage.
-->

# FBSound build contexts, November 14

The selected `cd32db5b` compiler passes all 236 FBSound object profiles with
**zero GCC warnings**. GCC and Clang accept all 472 native object builds.
The 52 test files previously lacking owning-build evidence now have completed
profiles. The wider source ledger has 1,466 owning-source proofs and 1,471
pending build contexts across its recorded compiler cuts.

| Target | Object profiles | GCC warnings | Clang warnings |
| --- | ---: | ---: | ---: |
| Windows x86-64 | 61 | 0 | 179 |
| Windows x86 | 61 | 0 | 146 |
| Linux x86-64 | 57 | 0 | 49 |
| Linux x86 | 57 | 0 | 31 |
| Total | 236 | 0 | 405 |

Windows uses GCC 16.2 and Clang 22.1.7, with `-m32` selecting their Win32 object
target. Linux uses native GCC 14.2 and Clang 19.1.7 in the two-core compile guest.
Each saved command retains the full unsuppressed diagnostic options. These
profiles overlap the retained library corpus and keep their separate totals.

## Build recipes and source identity

The source selection comes from `C:\fb_corpus\fbsound\makefile`. Windows
profiles include six static-library sources, three shared-library sources and
all 52 `tests/*.bas` entries. Each Linux target includes four library sources,
the same 52 test entries and the shared-library smoke entry's dynamic binding
configuration. Include paths, multithreading, optimization, output kind and
platform defines follow the corresponding Make targets. Linux retains PIC.

The initial captures enable all FBC warning levels. A second complete 236-unit
emission retains Make's `-w pedantic` checks together with `-w all`, and supplies
the explicit Win64 architecture option. All 236 C files match the already
compiled files exactly. Every original FBC warning line is retained. This
qualifies reuse of their native object results under the normal recipe checks.

The Linux source pins also match the source inventory captured before the
Windows builds. Every opened input and all nine protected compiler, runtime
and FBSound product hashes are rechecked at closure. FBSound's existing dirty
work is preserved.

## Clang classes and qualification scope

The 405 Clang observations comprise 174 padding observations, 192 startup
registrations, five reserved identifiers, four declaration-order observations,
two overlength strings and 28 `-Wunsafe-buffer-usage` observations. They remain
in the native receipts for subsequent class review.

These are object-only compiler diagnostics. Linking, audio-device operation,
media decoding and the separate C++ SID provider retain their own build/runtime
qualification. The source coverage proofs use the current Windows owning
profiles. The ledger records 220 owning paths on `cd32db5b` and 1,246 on the
preceding `987d6d9d` cut.

## Receipts

The existing `build/c-emitter-forward-views-20261105` work area contains the
complete records.

- `current-fbsound-win64-v1-20261114.sqlite3` and its Win32 counterpart retain
  recipes, input hashes, actual emissions and both native compiler outputs.
- `current-fbsound-linux-emission-v1-20261114.sqlite3` retains all 114 actual
  Linux emissions and their compressed C.
- `fbsound-linux-native-packet-v1-20261114.json.zlib` and
  `fbsound-linux-native-result-v1-20261114.json.zlib` bind each emitted file to
  its native compiler command, output and return status.
- `fbsound-normal-flags-v1-20261114.json.zlib` records the complete Make-warning
  replay and 236 exact C identities.
- `fbsound-qualification-v1-20261114.json.zlib` closes the 236-profile matrix.
- `fbsound-source-cut-v1-20261114.json.zlib` preserves the exact opened BASIC
  sources under portable project/SDK paths, with the matching Make recipe.
- `current-source-coverage-selected-reconciliation-v10-20261114.json.zlib`
  replaces the previous coverage view with the 52 additional owning proofs.

The host workers reuse one temporary C/object/log slot and peak below 702 KB
each, including their result databases. The native Linux worker peaks at
1,078,990 owned bytes. The completed 775,660-byte encoded upload wrapper is
retired. Compressed C, controllers and qualification receipts remain available.

<!-- end of c-emitter-fbsound-contexts.md -->
