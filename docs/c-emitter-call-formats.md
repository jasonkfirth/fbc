<!--
Project: FreeBASIC C backend verification
File: c-emitter-call-formats.md
Purpose: Record literal format provenance across ordinary calls.
Responsibilities: Explain complete-body checks, native diagnostics and promotion.
Application execution retains its separately recorded qualification.
-->

# Literal formats across calls, November 16

[Stored translations](c-emitter-stored-translations.md) have an additional
ordinary-call boundary because translation storage can change through aliases.

Development main is `52606515`, with full SHA256
`5260651597fe8180e05eef139e9b57425848f426f91c0213588b64d873b8297f`.
The compiler retains a proven private literal binding across ordinary calls.
Complete-body checks exclude escaped pointer slots, writable aliases and opaque
operations. Direct stores update the known value, and labels, memory writes and
saved-continuation calls invalidate it. The native formatter receives the actual
evaluated pointer through the existing format-argument helper.

The initial candidate omitted the saved-continuation boundary. A diagnostic
control reproduces that omission, and the final compiler retains invalidation
for the native setjmp-family names. Its complete backend suite passes. The
regression also retains wrong-type errors after ordinary calls, dynamic-format
warnings, BYREF and alias boundaries, control-flow joins and static-storage
boundaries. Native value tests check exact output, return values, pointer
identity and single evaluation of intervening calls and formatter arguments.

## Native and compiler qualification

Sixty-four Windows/Linux executions cover both revisions, both C producers,
both native compilers, 32/64-bit targets and O0/O2. They pass 2,240 complete
formatted-output comparisons, together with pointer-identity and call-count
checks. Windows 32-bit Clang uses the actual MinGW32 headers and GCC32 linker.
The native flags retain their unsuppressed audit settings.

All 224 compiler units retain a GCC-clean emitted-C fixed point. The complete
normal backend suite passes in 736.201 seconds, and all 703 fbctests profiles
remain GCC-clean. Their Clang totals remain 1,340 and 771. Development-main
selection is atomic, retains the preceding `dffcd9a1` image and passes two normal
entry-point executions with assertions enabled. All ten protected binaries and
library products retain their preceding hashes.

## Production and corpus results

The complete coreutils/SQLite replay contains 1,053 profiles. Only the coreutils
`ls` module changes its emitted C. Both native tools produce byte-identical
original/current objects for that module. Its `FILE_ESCAPE` formatter retains
the literal contract across the intervening `to_uchar` call.

Coreutils reaches GCC 167 / Clang 1,803. GCC removes one nonliteral-format
observation. Clang replaces its nonliteral observation with a format argument
signedness observation for the explicitly promoted byte. That warning remains
counted and retains its format-contract review scope. SQLite remains GCC 21 /
Clang 7,696. Three finite-format compiler sites and seventeen dynamic-format
runtime contracts remain open.

The complete retained replay passes 6,419 profiles at GCC 12 / Clang 15,862.
Its 6,345 accepted profiles, 46 PNG exclusions, six included providers, two
closed assembler contexts and twenty known frontend refusals retain their
preceding dispositions. Only the compiler emitter module changes C identity.
Exampleageddon's 1,775 checked and 1,775 default contexts retain GCC 3 each and
Clang 2,712 / 2,591 respectively.

Templari's qualified recipe supplies 395 client and 281 server contexts. All 676
contexts have identical paired C-producer output, totaling 1,352 emissions.
The qualified source cut reaches GCC 2 / Clang 53,783. The graph grows by two
contexts overall, with eight new context keys and six retired keys. Forty-three
matched contexts have changed opened inputs. The remaining 625 contexts keep
their inputs and identical C. This is
compilation qualification. Application execution retains its separate evidence.

The refreshed additional-project queue remains GCC-clean for Midisoft 146,
Rotator3d 16, FS4 125 and HART 153 standalone contexts. Its 441 configured
contexts preserve the owning-source disposition for the included HART entry.

A later shared GUI synchronization changes 813 opened inputs across these
project and corpus copies. These counts describe their last-qualified source
cuts. Replay of the synchronized copies remains pending. The compiler, fbctests
and Linux runtime source checks remain current.

## Reproduction and receipts

The registered repository regression is part of the normal backend suite.

```sh
python3 -B build_scripts/test-compiler-backends.py \
  --root /path/to/fbc --fbc /path/to/fbc/build/main-gcc/bin/fbc \
  --toolchain-prefix /path/to/toolchain --backend gcc --backend clang
```

The focused diagnostic entry point is
`tests/compiler-support/c-format-locals.py`. Its paired BASIC/C fixture supplies
the runtime value checks. Source-only library lint reports zero errors and zero
warnings for the changed emitter and BASIC fixture.

Receipts under `build/c-emitter-forward-views-20261105` include
`call-format-selfhost-v2-20261115.json.zlib`,
`call-format-suite-v2-20261115.json.zlib`,
`call-format-selected-v1-20261115.json.zlib`, the paired native value receipts,
`call-format-port-replay-v1-20261115.sqlite3`,
`call-format-retained-v1-20261115.sqlite3`,
`call-format-templari-v1-20261115.sqlite3` and
`call-format-extra-projects-v1-20261116.sqlite3`.

The Templari exporter initially reused the preceding archive's filename. The
676-context input archive is preserved under its own filename, and the future
controller uses a unique output name. Recovery verifies 1,184 of the preceding
674-context cut's 1,185 raw source versions. The older server `src/external.bas`
version is unavailable. Its recorded hash and emitted C remain preserved.
`call-format-archive-recovery-v1-20261116.json.zlib` records this limit and the
reconstructed archive's new container hash. The current 676-context archive
contains all of its qualified opened inputs.

<!-- end of c-emitter-call-formats.md -->
