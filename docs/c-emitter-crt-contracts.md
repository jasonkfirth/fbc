<!--
Project: FreeBASIC C backend and Windows CRT verification
File: c-emitter-crt-contracts.md
Purpose: Record native CRT contracts and the complete paired warning recount.
Responsibilities: Explain source changes, qualification and reproducibility limits.
This file intentionally does NOT claim private promotion or whole-corpus execution.
-->

# Windows CRT contracts and C identifiers, November 5, 2026

The Win64 declaration now matches the selected UCRT SDK: FILE contains one
pointer placeholder, with size eight, and __acrt_iob_func takes a 32-bit unsigned
index. The old declaration exposed the legacy multi-field record and used a
64-bit native integer for the index. The Win32 legacy MSVCRT record and stream
table remain unchanged. Consumers embedding the old FILE record require a
rebuild; UCRT does not expose the old private MSVCRT fields.

The C encoder gives 18 recurring ordinary CRT functions legal generated C names
and retains their exact assembler/linker symbols. Accessors, string operations,
narrow/wide integer conversions and directory/environment entry points share
this rule. Unknown functions and compiler-recognized intrinsic spellings keep
their existing treatment. Both selected native tools report these 18 functions
as non-builtins at 32 and 64 bits.

The experimental compiler additionally preserves its existing RTL-only _setjmp
naming rule. That rule is outside the public helper's policy. A first overlay
omitted it, and the full recount caught new per-profile warnings. Those private
results are rejected preflight data. The corrected selector equals the public
helper plus that exact existing rule; no original provider was modified.

| Queue | Main GCC | Main Clang | Experimental GCC | Experimental Clang |
| --- | ---: | ---: | ---: | ---: |
| Broad stable body | 2,139 | 7,144 | 247 | 5,291 |
| Exampleageddon | 3,538 | 8,970 | 126 | 5,549 |

Each compiler cut removes 170 broad and 50 Exampleageddon Clang observations.
Only reserved identifiers and padding change. GCC totals and all other classes
stay unchanged, and no profile gains warnings. Counts describe overlapping
queue observations, not distinct source defects.

The main e681a3a8 compiler reproduces all 224 qualified C files from Root without
staging includes. Experimental b3c09da8 reproduces its 224 files using Root SDK
headers and the explicit private selector. Both bootstrap/final compiler images
are byte-identical within their respective fixed points. Compiler diagnostics
remain zero GCC and 1,506 Clang observations per cut.

The accepted current pair has 64 native C matrix executions across checked and
unchecked modes, GCC/Clang producers and tools, GNU11/GNU23, and O0/O2. Four
independent GAS64 controls pass. Native SDK helpers check FILE size and enclosing
field offset, standard-stream identity, errno slot/value, conversions, owned
allocation/free and a small real file round trip. The original header control
returns at the size comparison before any mismatched accessor call.

Address-only probes cover APIs whose execution would change process state or
produce sound. The 128 before/after COFF objects establish 64 code/data/relocation
and external-symbol identities. Four fresh C identities bind the corrected
experimental compiler to its retained object checks. GCC may change an undefined
asm-labelled function's COFF type from function 0x20 to unspecified zero; this
metadata distinction is recorded explicitly. Win32 execution and nominal C-tag
identity under mixed LTO are not claimed by these new CRT probes.

Four full backend suites, two current namespace suites, existing full stdio
contracts, 14 semantic contracts and the structural check pass. Source-only
strict lint has zero errors and no new findings; eight existing atomic-macro
advisories remain. The object census covers all selected profiles with original
unsuppressed diagnostics, preserving the [documented target/frontend limits](c-emitter-current-queues.md).
It does not execute the corpus examples. Installed compilers and archives remain
unchanged, and unrelated dirty work and valid examples are preserved.

The experimental build is locally reproducible using retained inputs and caches.
Of its 224 compiler units, 217 use current sources and seven use overlays. Its
recipes record 39 include directories, and its matching thin runtime archive has
external object dependencies. A standalone source/runtime bundle and a clean
rebuild on another machine have not been demonstrated. Runtime ownership and
interoperability promotion boundaries remain open. Lower experimental warning
counts must not be presented as delivered main-compiler results.

Evidence is in build/c-emitter-forward-views-20261105: crt-promotion.json,
crt-final.patch, crt-body-comparison.json and crt-experimental-reproducibility.json.
The corrected experimental work is retained in crt-candidate/private-restored
inside E:/fbc-c-audit-20261105-forward-views-r2-qualification. One additional
nested candidate retains the rejected preflight without copying the source tree.
Retained data has an 80 MiB guard and reused whole-body scratch a 96 MiB guard.
The largest compilations use a generous hang bound; 90 seconds is not acceptance.

<!-- end of c-emitter-crt-contracts.md -->
