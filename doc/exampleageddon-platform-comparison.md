<!--
Project: FreeBASIC example qualification
File: exampleageddon-platform-comparison.md
Purpose: Explain the six-target comparison and how to update it from receipts.
Responsibilities: Compile/runtime distinctions, source identity and complete coverage.
This file contains no compiler installation or physical-device acceptance.
-->

# Comparing Exampleageddon targets

Run the maintained sweep on each actual target using its matching compiler,
runtime and genuine SDKs. Keep its results.csv and source/compiler hashes.
Win32 and Win64 are separate targets even when both execute on the same PC.
Use the DOS session's native DOS execution evidence, not a Windows compiler
cross-build, to establish DOS runtime success.

The comparison tool accepts one CSV per target and optional source pins:

```sh
python3 build_scripts/report-exampleageddon-platforms.py \
    --sweep win32=out/win32/results.csv --pins win32=out/win32/inputs.json \
    --sweep win64=out/win64/results.csv --pins win64=out/win64/inputs.json \
    --sweep linux=out/linux/results.csv --pins linux=out/linux/inputs.json \
    --sweep haiku=out/haiku/results.csv --pins haiku=out/haiku/inputs.json \
    --sweep freebsd=out/freebsd/results.csv --pins freebsd=out/freebsd/inputs.json \
    --sweep dos=out/dos/results.csv --pins dos=out/dos/inputs.json \
    --output out/platform-comparison
```

Pin files normally contain a source_sha256 object keyed by repository path.
An observed post-run hash must say so in source_basis; it is weaker evidence
than a hash recorded before execution. source_identity can map an included
helper to the owning program that the runner actually compiled. The comparison
retains that identity rather than comparing the owner's hash to the helper.

Use --overlay TARGET=CSV for a later focused result covering named examples
from that target's full sweep. Supply pins which distinguish the original
sweep from the focused source update. Overlay paths and hashes remain in the
JSON; the original full-sweep CSV is left intact.

The platform-matrix.csv contains every current example and all six target
columns, including classification, skip reason and compile-log path. Targets
without a receipt are not-tested. Source changes are stale-evidence. A source
match does not establish that every included header or library is unchanged.

The report also writes four filtered lists:

- failed-all-six.csv contains recorded compile or runtime failures on all six
  targets. Skipped, blocked, stale and missing checks cannot qualify for it.
- fails-on-some-target.csv contains an observed failure on at least one target.
- no-compile-pass-on-tested-targets.csv identifies candidates with no recorded
  compilation success. It is not an all-target failure claim.
- no-recorded-runtime-pass.csv includes every example without an unattended
  runtime success in the supplied sweeps. Many are interactive examples,
  library helpers or external SDK clients which were deliberately not run.

Read the group and reason columns before treating a compile failure as a bug.
Intentional compiler-error demonstrations and examples for other platforms
remain classified. Missing SDKs describe the tested environment; they do not
prove that a supported example can never work on that target. Focused native
SDK receipts supplement the sweep, but the tool does not silently reinterpret
skipped rows as either successes or failures.

<!-- end of exampleageddon-platform-comparison.md -->
