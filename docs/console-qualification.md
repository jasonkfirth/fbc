# Xbox and Wii qualification, 13 November 2026

Both cross-compilers and target libraries were rebuilt. Wii passes the complete
selected unit manifest and required Exampleageddon examples. Xbox passes its
required examples; its full unit qualification remains open.

| Target | Exampleageddon inventory | Compiled | Required executions | fbctests |
| --- | ---: | ---: | ---: | --- |
| Wii | 1,775 | 1,304 | 654/654 passed | 685 suites, 2,451 tests, 1,152,074 assertions; zero failures |
| Xbox | 1,775 | 1,321 | 655/655 passed | Incomplete; five failure diagnostics recorded before timeout |

The shared runner retains its existing classifications. External libraries,
foreign platforms, interactive programs and other excluded classes remain
visible in each CSV. The reports combine full sweeps with actual focused
retests. Every replacement pass was checked against its executable hash and
fresh guest exit nonce. First-sweep reports and raw runtime receipts are retained.

The final Wii unit run used the rebuilt runtime, including the NOW error-path
unlock repair. Its XML matches the earlier passing run byte for byte, while its
executable has its own recorded hash. All selected Wii BASIC module objects had
already been rebuilt with the corrected compiler. The guest returned zero under
Dolphin's JIT core, and the exported XML was checked against the guest receipt.

## Source and changes

Qualification uses the frozen source based on
`838154812bab6a77f7477a942bfde12f3fbe88ac` plus the recorded source overlays.
The final audit checks 13,771 source files with no mismatches. These receipts
cover that snapshot; concurrent canonical-tree edits require separate testing.

The work includes:

- Wii software TLS, procedure-lifetime storage for over-aligned locals, and
  native thread-context lookup. Alignment coverage includes scoped declarations
  and repeated control-flow entry.
- Xbox nested-record alignment, CRT initialization and shutdown ordering,
  calendar functions and TIMER units. CRT long-double bindings follow each
  console SDK's native ABI.
- Xbox directory iteration, wildcard matching, relative rename paths and
  condition variables. Waiters register before releasing their mutex, and each
  owns a native stack event until signaling has finished.
- Defined errors for missing Xbox process targets and unsupported directory
  changes, with temporary-string cleanup on failed EXEC calls.
- Console guest argument and exit adapters, explicit BASIC main selection, and
  bounded emulator controllers. The Xbox controller owns its complete process
  tree and retries transient sharing failures while a status file is published.

The eight-worker condition regression passes 128 rounds on both consoles.
Missing, empty and oversized Xbox launch paths return the expected errors.
Shared Exampleageddon contract tests pass all ten cases. The added console
regressions pass source lint with `--no-semantic`; semantic lint is a separate
qualification scope.

## Outstanding Xbox unit gate

The final XML-only unit attempt recorded two promoted-argument formatting
assertions and three Unicode directory assertions, then exceeded its runtime
bound without producing complete XML. Earlier verbose attempts stalled in the
100-thread test, and another attempt stopped while creating an encoded file.
Captured production stacks include SDK stdio mutex waiters and sleeping workers;
the full-suite stall remains unresolved.

A standalone native SDK thread probe reproduces a thread-creation stall with
no emitted BASIC objects or FreeBASIC runtime linked. Under the default affinity
it reports 75 thread creations within 70 ms, then reaches the 180-second host
timeout. With the alternate all-cores affinity, the first 100-thread round
finishes in 135 ms; the second round stops progressing after reporting 75
creations and also times out. Both phases stop before worker stdio output.
The link map, source hashes, failed guest receipts and exact private-settings
restoration are checked in
[the native thread evidence](../out/codex-console-20261113/native-thread-evidence.json).
The alternate option follows the emulator's
[official settings definition](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/master/src/common/Settings.cpp).

Independent native C probes reproduce the two library/filesystem boundaries:

- The SDK formatter returns the same incorrect result from native C and emitted
  BASIC calls. Floating-point formatting is missing from this installed PDCLib.
- Native CreateFileA and FindFirstFileA/FindNextFileA return question marks for
  the UTF-8 filename bytes under Cxbx-Reloaded CI-585c49a. The probe uses the SDK
  directly, independently of FreeBASIC's directory adapter.

SDK formatting support and validation with a working xemu firmware/disk setup
or Xbox hardware remain necessary for a passing Xbox unit gate. The parent-tree
asset search located Dolphin but did not locate a usable xemu BIOS/MCPX/HDD set.
A final filename search covered the supplied workspace's parent, the fbc
checkout's parent, and E:. Its sole denied directory was System Volume
Information; unrelated QEMU firmware and disk images were retained as search
findings only.
The follow-up search includes case-insensitive filenames, likely archive names,
the Windows-redirected Downloads folder, the games tree and the installed Xbox
SDK. It also finds no usable firmware/disk set.

## Evidence

The SDKs were used in place. Work lives under
`out/codex-console-20261113`, with separate target objects and reusable test media.

- [Final evidence manifest](../out/codex-console-20261113/console-qualification-final.json)
- [Wii unit receipt and totals](../out/codex-console-20261113/wii-units/verified-summary.json)
- [Xbox unit receipt and observed failures](../out/codex-console-20261113/xbox-units/verified-summary.json)
- [Wii Exampleageddon report](../out/codex-console-20261113/wii-examples/report.md)
- [Xbox Exampleageddon report](../out/codex-console-20261113/xbox-examples/report.md)
- [Frozen source audit](../out/codex-console-20261113/frozen-source-audit.json)

The final manifest records compiler, runtime archive, SDK library, controller,
program and exported-artifact hashes. The task directory also retains the build
scripts, compiler wrappers and raw guest logs used for these results.

<!-- end of console-qualification.md -->
