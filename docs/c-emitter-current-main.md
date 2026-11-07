<!--
Project: FreeBASIC current main-source qualification
File: c-emitter-current-main.md
Purpose: Record current compiler freshness and its complete emitted-C queues.
Responsibilities: Bind actual source, diagnostic counts and reproducibility limits.
This file intentionally does NOT claim experimental promotion or zero warnings.
-->

# Current main compiler, November 6, 2026

Compiler 3b1b1ab5 builds the actual current checkout, including preserved
parser-quirk-string.bas and semantic-model.bas changes outside the frozen package.
This qualification does not modify those sources or install the resulting image.

The actual Make-selected Windows graph contains 224 units. All are freshly
emitted with the previous main compiler. Exactly two C inputs change; both pass
fresh aggressive, unsuppressed GCC and Clang checks. The other 222 qualified
objects are reused after exact C and object comparisons. The rebuilt compiler
then regenerates all 224 C inputs byte for byte. Both complete backend suites
pass. Strict lint on the two changed source files reports zero findings.

Compiler-source diagnostics remain zero GCC and 1,506 Clang: 1,493 natural
record-padding observations and 13 startup registration observations. This batch
closes the current-source build gap; it does not remove warnings.

| Queue | Selected profiles | C-passed profiles | GCC | Clang |
| --- | ---: | ---: | ---: | ---: |
| Broad corpus/projects/compiler | 2,295 | 2,251 | 2,139 | 7,144 |
| Exampleageddon | 3,550 | 3,488 | 3,495 | 8,916 |

Every eligible application profile is freshly emitted with the rebuilt compiler.
The broad queue reuses the separately completed 224-unit compiler qualification.
Native results are reused only for identical C, pinned tools and complete flags,
with no C preprocessing dependency. Two existing DOS assembly profiles retain
their C target limitation. Twenty example frontend limits, six included-owner
profiles, 46 PNG exclusions and 32 concurrent Optical exclusions retain their
existing classifications. No application is executed by this census. The two
queues overlap and count diagnostic observations, not distinct defects.

All 10,901 census source identities are checked against an explicit current cut.
The delta from the earlier census comprises two compiler BASIC files, 115
native-library C/header files and the already qualified record-dependency test
runner repair. Native-only files are pinned separately; this emitted-C sweep
does not compile or qualify their library behavior. The recent native runtime
repairs retain their independent receipts and counts. The compiler link uses
its previously matched main runtime prefix.

An initial audit used a different scratch C basename, changing generated module
constructor names and needlessly triggering native recompilation. All 96 affected
preflight profiles match the previous C byte for byte after that name substitution.
Their commands, diagnostics and controller preimage remain in a compact receipt;
the redundant full C copies were retired. The accepted sweep reuses unit.c;
remaining C differences receive fresh native checks. Another guard preflight mistook macro spellings inside
C string literals for preprocessing dependencies; its corrected token scan passes.

Reproduction uses current-main-build.py and current-main-queues.py in the existing
build/c-emitter-forward-views-20261105 area, the pinned checkout, tools and earlier
qualified object graph. The compiler C text views in JSON normalize line endings;
recorded emission commands reconstruct the byte-exact native inputs. The queue
controller proves the two reconstructed objects and executable hashes again.
This is a local incremental current-main qualification. The portable source
archive remains the earlier experimental cut. Another-host cold rebuilding and
external corpus replay from that archive remain unverified.

Temporary compiler, C, object and response slots are retired. Retained storage
stays below 160 MiB, with a separate 192 MiB transient integration ceiling.
Four installed compiler/archive pins remain unchanged. No valid examples,
PNG source or unrelated dirty work are changed. The full campaign remains active.

Evidence: current-main-build.json.zlib, current-main-queues.json.zlib and
current-main-lint.json.zlib in the existing E: qualification area; independent
verification is build/c-emitter-forward-views-20261105/current-main-final-verification.json.

<!-- end of c-emitter-current-main.md -->
