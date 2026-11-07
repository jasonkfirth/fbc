<!--
Project: FreeBASIC runtime ABI qualification
File: c-emitter-runtime-symbol-bindings.md
Purpose: Record ordinary C identifiers with unchanged runtime linker symbols.
Responsibilities: Explain prefix ownership, consumer coverage, COFF import hints
and the cumulative source delivery without conflating emitted-C corpus counts.
This file intentionally does NOT claim another-host or full-campaign completion.
-->

# Runtime C identifiers and ABI labels

Nine runtime C spellings now use the ordinary fb_ namespace. Their legacy linker
names remain unchanged through FB_RTLIB_SYMBOL in fb_abi.h. This includes the
shared context, error buffer, DATA cursor, encoding tables and Object entry points.
The compiler supplies its native C label prefix, including the extra underscore
on Win32. The helper owns data and Cdecl bindings; it does not invent non-Cdecl
decoration. Public layouts, signatures, enum values and BASIC declarations remain
unchanged. No diagnostic option is disabled.

The consistent token update changes 190 existing in-tree C/header files and adds
one small ABI header. The token lexer preserves string and character literals,
comments and unrelated dirty bytes. Thus quoted ABI names and inline assembly
keep their existing spellings. The independently encoded minimal NuttX Object
provider keeps its own legacy definitions. No BASIC source, application example
or PNG loader is changed. Existing internal graphics/sound clients and three
coupled C test fixtures follow the shared header's ordinary names.

Encoding-table declarations now live in one shared header, rather than being
repeated in individual readers. The Object RTTI record also has a shared
declaration. These changes remove three missing-variable-declaration observations
per runtime variant along with the repeated reserved-name and dollar warnings.

| Native runtime profile | GCC before / after | Clang before | Clang after |
| --- | ---: | ---: | ---: |
| ST, 522 members | 1,067 / 1,067 | 26,705 | 22,546 |
| MT, 522 members | 1,067 / 1,067 | 26,712 | 22,553 |
| Startup | 2 / 2 | 47 | 39 |

All 2,090 direct native checks pass under the same aggressive, unsuppressed
flags. The total reduction is 8,326 Clang observations. GCC is unchanged; no
warning class increases. Instructions, data, relocations and exported symbols
match the preceding runtime throughout.

## The two COFF byte differences

There are 1,043 exact GCC object identities. The ST/MT copy-constructor objects
change only the optional type hint on the undefined base-constructor symbol:
function type 0x20 becomes unspecified type zero. GCC omits this .def hint for an
asm-labelled imported function. The definition remains a typed function and the
tail-jump relocation still names the same ABI symbol. The earlier CRT alias
qualification recorded the same compiler behavior.

The comparison permits this field only for the two named Object Cdecl labels,
only on undefined external symbols, and only in the 0x20-to-zero direction.
Every other object byte must match. Produced objects are retained as emitted;
the comparison never edits them. Both normal archives include the actual new
objects and retain all exports and member order. Relinking the compiler with
the new ST archive yields the exact preceding executable, 86700330. Its duplicate
output is retired after comparison; installed binaries remain untouched.

## Consumer and platform checks

The target-prefix prototype uses the production header: sixteen Win32/Win64
objects with GCC/Clang and twenty-four freestanding Clang objects across Linux
x86-64, ARMv7, AArch64, RISC-V32/64 and Darwin x86-64. Plain and weak definitions
are identical before and after. These are prefix/object checks, not foreign SDK
or runtime execution claims.

Fourteen affected production/mock consumers have Win32/Win64 GCC/Clang coverage.
Fifty-two object pairs are byte-identical. Four objects from the string-limit
test differ because standard assert() prints the renamed C expression. Its
assertions remain enabled and all four before/after executions pass. Eight mixed
C/C++ executions use old-header C objects, new-header callers and the actual
updated ST/MT archives. Twelve additional Win32 objects cover those current
fixtures. Sixteen canonical provider checks retain eight exact object pairs.

Two measurement-wrapper issues were corrected without changing production
behavior: the pthread fixture retains its original SDK include order, and
fixtures which include production C select those files' actual preimages for
the before comparison. The runtime structural guard passes 126 source-graph
cases across 1,252 sources. It also prevents legacy ABI spellings from returning
as ordinary C names in common headers, while allowing quoted binding labels.

The new header's identifying comment was corrected after native compilation.
Everything after that comment and its source line count remain identical. The
actual-header prefix checks and structural guard pass after the correction.
The amendment receipt records both exact versions rather than claiming a second
full native sweep for a comment-only change.

## Source delivery and retained limits

The cumulative runtime-symbol-source-update.zip in the existing E: qualification
area updates 134 files and the runtime manifest. All 609 composed inputs match
the current qualification provider. Apply it over the original portable-source.zip
in a fresh short directory before building. It includes the preceding string and
type-tag repairs and replaces the earlier type-tag-only update. The original
archive stays unchanged; the update contains no executable or object cache.

```powershell
python -B verify-runtime-warning-update.py
python -B build.py --tool-dir C:/path/to/ucrt64/bin
python -B verify-smoke.py --tool-dir C:/path/to/ucrt64/bin
```

Another-host rebuild, external corpus replay from the bundle, wider SDK execution
and the experimental compiler's existing promotion limits remain unfinished.
The emitted-C corpus census remains separate and unchanged. The source update
retains the frozen compiler/bootstrap cut before unrelated concurrent frontend
edits. The overall compiler warning campaign remains active.

The 160 MiB retained-component bound still applies. One retired, explicitly
unaccepted include-wrapper SQLite receipt is stored as a lossless zlib file;
its original bytes restore exactly. Previously rejected scratch-file deletions
were not retried. No installed compiler, runtime archive or application binary
was replaced.

Evidence: runtime-symbol-native.json/SQLite, runtime-symbol-consumers.json/SQLite,
runtime-symbol-link.json/SQLite, runtime-symbol-mixed.json/SQLite,
runtime-symbol-canonical.json/SQLite, runtime-symbol-cross.json/SQLite,
runtime-symbol-documentation-amendment.json and runtime-symbol-source-update.json
in E:/fbc-c-audit-20261105-forward-views-r2-qualification/crt-candidate/canonical.

<!-- end of c-emitter-runtime-symbol-bindings.md -->
