<!-- Project: FreeBASIC DOS qualification; File: dos-qualification.md.
Purpose: Reproduce the DOS build and language qualification.
Responsibilities: Emulator requirements, commands, receipts and acceptance scope.
This document does not implement compiler semantics or publish packages. -->

# DOS build and qualification

Run the maintained scripts from MSYS2/MINGW64 at the repository root:

```sh
export MAKE_JOBS=4
bash build_scripts/msdos-build-freebasic.sh --skip-deps --keep-buildroot
bash build_scripts/msdos-unit-tests.sh
bash build_scripts/msdos-exampleageddon.sh
```

The build script stages `out/msdos/FreeBASIC-*-dos` and creates its ZIP.
It also bundles the existing PDMLWP thread provider, its source notices and
rebuild helper. Programs still select that provider explicitly with
`-dos-threads pdmlwp`; ordinary programs use the non-threaded runtime.
`DOS_THREAD_PROVIDER=` disables these optional archives for a smaller build,
which cannot qualify the required threaded examples.
The unit script uses that build's private DOS source/object directory, compiles
the emulator, FPU control and assembly-address preflights and the complete
Make-selected DOS unit manifest, and runs the executables in DOSBox-X.
Test objects from a Windows build are not reused.
`BUILDROOT`, `OUT`, `PKGDIR`, `HOST_FBC`, `TOOLROOT`, and `DOSBOX_BIN` can select
existing build inputs. A fresh source build is required for final acceptance.

The `dos` job in `.github/workflows/extended-platforms.yml` builds the package
with `--keep-buildroot`, runs these unit checks, then runs Exampleageddon with
the packaged DOS compiler. It uploads the ZIP and qualification reports even
when tests fail. An uploaded package is not evidence that qualification passed.

Run `bash build_scripts/msdos-exampleageddon.sh` after the distribution build
to exercise the same examples locally. This wrapper uses native MinGW Python
so DOSBox-X receives Windows mount paths. It accepts the build's `OUT`,
`PKGDIR`, and `DOSBOX_BIN` settings and an optional `DOS_EXAMPLE_SDK` directory.
Additional arguments are passed to `msdos-exampleageddon.py`.

The example runner uses DOS batch files and response files. The shared runner
still owns inventory classification, multi-file sources and runtime resources.
Every source receives a result row; unsupported platform or library outcomes
remain visible. The gate requires a complete inventory, successful compilation
and execution of every self-contained example, and no attempted runtime failure.
The shared argument selector adds `-dos-threads pdmlwp` to DOS examples using
thread primitives. These examples retain their original classifications.
`--max-batches` is a development bound and cannot turn a partial run into a pass.
`--resume` requires identical source, package, emulator and runner fingerprints.
`--start-batch N` changes only the processing order: the runner wraps through
every batch and still requires the entire inventory before the gate can pass.
Each completed batch retains logs and artifact hashes, then discards its private
source copies and binaries to bound disk use.
The wrapper runs its own fresh x87 preflight, including when the CI unit lane
failed later in the language tests.
Per-example logs capture DOS stdout. The DOSBox-X shell does not support
`2>&1`, and the packaged DJGPP `REDIR.EXE` misparses its options with LFN
enabled. Compiler diagnostics on stdout are retained; diagnostics written only
to stderr are not in those logs. Exit status and artifact checks still gate
each compilation and runtime result.

## Emulator and process contracts

The portable Windows default is DOSBox-X 2026.10.01. Its release tag and asset
name use different prefixes; the build and media-test scripts derive both from
`DOSBOX_X_VERSION`. The DOS guest uses version 7.1 with long filenames enabled.
The test tree and compiler inputs require that filename support.

The default explicitly selects the normal CPU core and Pentium II model.
`DOS_TEST_CORE` selects the same core for unit preflights and Exampleageddon;
alternate cores must pass the preflights and full unit suite before use.
The CI job selects the verified October dynamic core to reduce compilation
time; both cores have completed the full DOS unit suite locally.
The March 2026 DOSBox-X dynamic core incorrectly terminates the bounded Single
loop in `tests/dos/emulator-x87.bas`; the same executable passes the normal
core and the October 2026 dynamic core. CPU-core selection must not be inherited
from a developer's interactive emulator configuration. The preflight rejects
that fault before the complete suite runs.

`build_scripts/dos-run-program.py` owns only the emulator process it launches.
It requires a fresh DOS success marker, checks the emulator exit status, limits
wall time and output growth, and records executable/emulator hashes. XML reports
can be included in the size limit with `--watch-file`. A timeout, missing marker,
output limit or DOS failure always returns a failing host status.
CWSDPMI paging is directed to the invocation's private work directory, and the
guest unloads its resident DPMI host before completing. This keeps runtime swap
files out of the immutable compiler package and makes resume fingerprints stable.

The unit report is generated only after the suite returns. `REPORT.XML` is
removed before each run, and a missing replacement fails qualification.
Reports under `out/msdos/qualification` include the guest output, XML, and host
receipt. Bounded output is essential: a failed assertion inside a broken loop
can otherwise produce hundreds of megabytes without reaching a final report.

## Acceptance scope

The frozen-source build below passes fbctests and the complete Exampleageddon
gate using its existing DOS applicability rules. The CI job runs both checks
and retains the DOS ZIP. Every manifest entry remains accounted for, including
unsupported platform/library outcomes. A DOS-compatible failure must not be
reclassified to obtain a green gate. These receipts qualify DOSBox-X with LFN
support; they do not establish a physical-DOS or classic-DOSBox result.

DOS WSTRING has a byte-wide compatibility ABI (`DISABLE_WCHAR`). USTRING remains
UTF-8 and supports Unicode scalars. Unicode round-trip tests must check the
documented replacement behavior when converting an unrepresentable scalar to
the byte-wide type; changing that ABI or suppressing Unicode tests is not a
valid precision or conversion fix.

## November 13 investigation receipt

The initial diagnostic found Single ASIN precision errors, seven additional
math comparisons and 42 text conversion assertions. These have been resolved
in the canonical sources. No tests were removed or suppressed.

The emulator has SHA-256
`7b549d74523e436d3813d8be1f3347aacb4cbf3ca08af717d3c82a1ab880ecef`.

The four previously failing Exampleageddon entries, IDs 0414, 1166, 1480 and
1481, passed bounded isolated replays with this setup. Their old sweep receipts
remain unchanged. The rebuilt compiler and complete passing sweep recorded
below supersede these focused diagnostic checks.

The passing diagnostic run completed all 2,461 tests and 1,620,888 assertions, with a
fresh XML report and zero failures. It includes IEEE libm startup behavior,
the tangent precision adapter, corrected byte-wide WSTRING expectations, and
two additional tests for lossy wide conversions and Unicode-only paths. The
test executable SHA-256 is
`19f495e5e96ebc5cac237dc3a3f57186a1979b6109702842e03b49fef0005303`.
This was the earlier candidate tree with focused canonical fixes applied;
the qualified-source receipts below come from the subsequent rebuild.

`tests/dos/math-control.c` additionally checks the exact binary64 tangent result
and preservation of all twelve precision/rounding combinations. The adapter
changes only the current thread's precision for the fdlibm call, stores the
Double result, then restores the caller's control word. BASIC startup selects
IEEE libm behavior so `0 ^ negative` has the same positive-infinity result as
constant folding and the other target runtimes.

## Qualified-source unit receipt

The November 13 source build and refreshed distribution pass the complete DOS
unit manifest: 685 suites, 2,462 tests and 1,620,924 assertions, with no errors
or failures. The final runtime-TLS build passes on both the normal and October
dynamic cores with executable SHA-256
`e245c3dc3df1a1625128d8a76a5e3da21a3819b1eeb7ae2848853fed90b43adb`.
All eight runtime preflights also pass on both cores, including the
assembly-address observer while the assembly block is active, initialized TLS
across C/GAS modules, private storage keys and checked array indexing.
The unit workflow retains its selected source
and object manifests, XML, guest output and parsed count summary.

Local normal-core evidence is under
`out/codex-dos-emitter-20261112/package/qualification`; the final dynamic-core
replay is under `package/qualification-dynamic` in that same task directory.
Both have fresh guest output, XML and a parsed count summary for the same
executable. The earlier `d9e824...` executable belongs to the pre-TLS-key build.
`qualification-inputs-float-bits.json` records the canonical/private source hashes.
Concurrent padding declarations confined to 64-bit targets produce identical
DOS-preprocessed internal headers; their equivalence receipt avoids an
unnecessary rebuild of the unchanged 32-bit code.
The provider package refresh passed the complete unit suite again with the
same unit executable hash. `qualification-inputs-provider.json` records that
source snapshot, including the provider sources and reviewed DOS-excluded
gfxlib3 updates. `qualification-source-cut.json` binds the emitter to committed
revision `e4341432091a3506ddb2386069c2478b3120651d`. Later independent,
uncommitted callback-cast edits are outside this qualification; these receipts
must not be presented as validation of those newer edits.

The earlier native compiler reproduced the new floating-argument representation
regression with exit status 9 at its signaling-NaN check. The current DOS compiler
passes all eight floating representation checks and the existing address/alias
checks. That pre-TLS-key compiler has SHA-256
`5808be1f1733247b62407dc72d122110e3960cd0a5ca915963580d17273d6141`.
The C observer uses Intel assembly and DJGPP's underscore-prefixed symbol names.

The earlier partial sweep is retained under `package/exampleageddon` and marked
superseded. A second sweep under `float-bits-qualification/exampleageddon`
found that the package omitted the existing optional thread provider; its
required thread-pool example failed compilation rather than being excluded.
All five required threaded examples passed a focused native DOS replay with
the provider. The refreshed package's sweep under
`out/codex-dos-emitter-20261112/provider-qualification/exampleageddon` then
exposed the TLS defect described below and was superseded. Its result rows
were not reused for the final compiler, and partial receipts are not acceptance.

The packaged `fbdos.bat` also sets GCC to the DJGPP installation, where its
private programs are available. The distribution smoke test calls this launcher
and compiles and runs both the default emitter and a C-backend x87 check.

## Runtime TLS qualification

The provider sweep found a further required compile failure in
`examples/manual/variable/thread-local.bas`: DJGPP COFF does not implement
native C `__thread` storage. DOS C accesses now use the existing runtime TLS
allocator, with ordinary static objects supplying their initial bytes.
Public BASIC externs use a common mangled-name key across modules. Private
objects use their static template address as an opaque key, because different
modules can have private variables with the same spelling. The runtime borrows
that key for the program lifetime and releases only its per-thread allocations.

The shared array-index repair retains byte bias, normal bounds checks and a
single evaluation of each index when TLS supplies a computed address. Native
C TLS externs and managed TLS initialization still receive explicit compiler
diagnostics. They are not replaced with shared storage.

The DOS unit workflow checks ordinary TLS, worker isolation, initialized BASIC
externs and private module identity through C and GAS, fixed-array indexing,
the storage API independently of compiler lowering, and unsupported-form
diagnostics before running the complete unit manifest. The earlier provider
sweeps are diagnostic receipts and are marked superseded. The rebuilt compiler
and archives passed a fresh complete 1,775-entry run under
`runtime-tls-qualification/exampleageddon`. The qualified compiler has
SHA-256 `bb5e8d4aba1516ba89926d6e340e7f614162f277f5d0b0202eb4adeaba3c8fa4`.
`qualification-inputs-runtime-tls.json` records its frozen source snapshot.
The final unit suite was rebuilt from all 695 selected objects before the
normal-core run; the dynamic-core replay uses that exact executable. Unsupported
managed TLS and native C TLS imports produce the expected two error 338
diagnostics without a compiler crash.

The distribution ZIP has SHA-256
`5e564bd6c9d25e622d155d1ab99864ae07f0d112c460b7b422bcdf98ebf48dda`.
`package/dos-package-receipt.json` records its 10339-file inventory, CRC check
and byte-for-byte comparison with the staged package. The CI job is present in
the canonical repository; these local receipts do not establish a hosted CI pass.

## Final Exampleageddon receipt

The official runner completed its full-inventory acceptance check with exit
status 0 after all native DOS workers stopped. It recomputed the source,
includes, package, SDK, emulator and runner fingerprints before accepting the
collected batches. All 45 batches have strict result rows and DOS completion
markers for 1,775 unique examples. Of these, 1,351 compiled and linked, and all
654 required self-contained examples ran successfully. There were no required
compile failures or attempted runtime failures. The other 424 compile failures
remain visible under their existing platform, library or other classifications.

The authoritative summary is
`out/codex-dos-emitter-20261112/runtime-tls-qualification/exampleageddon/reports/summary.json`.
The manifest, per-batch receipts, raw result rows, completion markers and logs
remain beside that report. `canonical-dos-fix-retention.json` in the task
directory records the source retention audit. The qualified snapshot is based
on `e4341432091a3506ddb2386069c2478b3120651d` plus the DOS TLS and shared
array-index fixes; these production fixes and the DOS CI job are captured in
canonical commit `838154812`. Later unrelated callback and graphics edits are
outside this compiler snapshot.

<!-- end of dos-qualification.md -->
