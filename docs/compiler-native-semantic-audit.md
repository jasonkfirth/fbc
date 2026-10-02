<!--
Project: FreeBASIC compiler verification
File: compiler-native-semantic-audit.md
Purpose: Record native-emitter audits and checked sidecar publication.
Responsibilities: Identify verified coverage, fixes, build integration, and limits.
This file intentionally does NOT claim complete language semantics or cross-platform execution.
-->

# Native emitter and sidecar publication audit

Verified October 1, 2026, following the
[compiler-source audit](compiler-source-semantic-audit.md).

## Native emitter coverage

All 164 modules selected by the Linux compiler Make graph pass with export
disabled, full export, and compact export on both native assembly backends:

| Compiler | gas64, Linux x86-64 | gas, Linux x86 |
| --- | ---: | ---: |
| Optimized | 164/164 | 164/164 |
| With assertions | 164/164 | 164/164 |

These are 656 module/backend cases and 1,968 compiler invocations. Full and
compact models pass the independent reader, metadata and reference closure,
physical source bounds, dependency closure, and expression comparisons.
The emitted assembly has identical bytes across all three export modes.

After the publication changes, the optimized compiler also passes the entire
GCC/LLVM/Clang module matrix: 492 cases and 1,476 invocations. That makes 1,148
verified cases and 3,444 invocations in this follow-up. Exact compiler identities
and source hashes are retained separately for each run; later focused driver
and preprocessor protection checks are recorded through the Make entrypoint.

## Publication defects closed

G01 and G02 in the [gap audit](semantic-sidecar-gap-audit.md) are addressed:

- The exporter stages output privately in the destination directory instead
  of truncating the requested destination before reading input.
- Sources, includes, preincludes, object/library inputs, resources, and
  requested compiler artifacts remain protected through publication.
  Filesystem identities detect hardlinks as well as canonical path aliases.
  Protection continues after the public dependency-record limit.
- Existing destination symlinks and special files are rejected. `/dev/full`
  reports a compiler error instead of silently succeeding.
- Every write, flush, close, and final replacement is checked. A failed
  operation preserves old output and makes the compiler return failure.
- Interrupted output stays private. Successful output replaces an existing
  ordinary model after the stream has closed successfully.

The existing incomplete/recovery snapshot behavior for parser errors remains
available. Consumers must still check the invocation status and the correct
completion or recovery footer. Publication safety does not establish the
remaining semantic capabilities listed in the gap audit.

## Regression and build checks

The expanded schema 20 contract suite passes all 54 methods with both compiler
builds. Separate gas and gas64 selections each pass 53 methods and skip the
LLVM-specific literal test. The writer fixture deterministically injects
write, flush, close, and replacement failures and verifies old contents and
staging cleanup. Native tests exercise source/include/preinclude aliases,
hardlinks, symlinks, generated-code/executable/preprocessor collisions, ordinary
replacement, special devices, and interruption.

One concurrent test run hit a temporary-directory quota while creating the
dependency-limit fixture. The same suite passed with its temporary files in
the project artifact directory. That attempt is retained as an environment
failure, not a compiler failure.

The writer and source revision observer are now BASIC modules. Native metadata
layouts remain behind the existing runtime filesystem interface, using a
fixed-width snapshot shared with `fbc-int/file-info.bi`. The compiler build and
bootstrap graph select BASIC sources only. The structure check passes for 233
documented BASIC/header files and 21 host source graphs. Publication and source
revision fixtures are also BASIC and exercise the production implementations.

The DOS implementation uses DJGPP's bounded `_fixpath` interface, as documented
in the [DJGPP library reference](https://www.delorie.com/djgpp/doc/libc/libc_328.html).
DOS staging directory names fit its short filename convention. Windows uses
native file identities and replacement without deleting the destination first.
Those branches were reviewed; this report does not claim their runtime validation.

## Reproduce

```sh
python3 build_scripts/test-compiler-semantic-self.py --fbc bin/fbc \
    --backend gas64 --output out/semantic-gas64 --compress-artifacts

python3 build_scripts/test-compiler-semantic-self.py --fbc bin/fbc \
    --backend gas --target-triplet i686-linux-gnu \
    --output out/semantic-gas --compress-artifacts
```

Evidence is retained under `out/compiler-semantic-native-20261001/`:

- `final-gas64-*`, `final-gas-*`, and `final-c-family` model/emission matrices;
- `verified-summary.json`, checked case totals and compiler identities;
- `contracts-verified-*`, `contracts-gas*`, and publication fixture results;
- `make-final.log`, bootstrap emission, cleanup, and structure evidence;
- frozen sources, headers, validators, Make files, and compiler binaries.

These matrices stop at compiler emission. They do not prove assembled programs
or every inactive host branch. Linux publication and its failure paths were
separately executed, and all earlier capability limits remain explicit.

<!-- end of compiler-native-semantic-audit.md -->
