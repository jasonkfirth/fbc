<!--
Project: FreeBASIC maintenance
File: maintenance-refinements.md
Purpose: Record completion and qualification of the five maintenance refinements.
Responsibilities: Explain changed contracts, executable evidence and remaining limits.
This file intentionally does NOT claim a release or a complete platform-driver audit.
-->

# Maintenance refinements, 20 October 2026

All five refinements are implemented and qualified. They extend the earlier
[build consolidation](build-consolidation.md) in the existing uncommitted tree.
The [machine-readable results](maintenance-refinements-results.json) pin the
tested maintained inputs, native receipts, linked libraries and generated
bootstrap output. The base revision remains
`6016fb099cab5e1c37f6b8c74f30782e1af94bcd`.

## Changed behavior

1. **Warning reports have an automatic oracle.** `make warning-tests` compiles
   copied fixtures in disposable storage and compares every target profile with
   its reference. Missing, additional or changed reports fail without rewriting
   expectations. `make warning-tests-update` explicitly replaces references
   after all selected compiler commands succeed. The historical shell entry
   point delegates to the same runner. Checkout CRLF and LF forms compare as
   logical text; compiler failures and timeouts remain failures.
2. **Mode lifetime follows runtime-before-graphics locking.** Screen entry
   points and shutdown acquire the runtime lock before graphics, matching input
   hook dispatch. Failure cleanup keeps the pair until storage and hooks agree.
   The shared legacy LZW table uses graphics synchronization, avoiding a hidden
   reverse acquisition from image operations. The
   [locking contract](gfx-locking.md) traces callbacks, temporary allocation,
   teardown and caller obligations. The WinCE replacement follows the same
   order. Its seven existing graphics source files are now exempted from the
   broad `winCE/` ignore rule, matching the runtime source exception.
3. **Documentation checks are maintained entry points.** `make source-docs-test`
   checks local guide links and native navigation and participates in quick/full
   workflows. `make source-docs-render-test FBDOC=/path/to/fbdoc` uses the real
   converter in disposable storage, checking HTML, fbhelp and text output,
   source trees, configuration prose, navigation and malformed markup.
4. **Incomplete source scans fail.** The artifact guard collects `os.walk`
   traversal errors and reports failure before it can print a pass marker.
   Its regression injects a `PermissionError` through `os.scandir`, avoiding
   permission tests which administrators or root could bypass.
5. **Build configuration covers more of its inputs.** Selected tool binaries
   and visible wrapper files contribute content checksums. Bootstrap objects
   and links have successful-output records, and failed links preserve the
   previous compiler. Emission uses copied inputs, records successful options,
   and rejects stale or unfinished output before parallel object compilation.
   `BUILD_ROOT=build/name` separates compiler/library objects, executables,
   archives and bootstrap output. Language-suite, quick/full, install and
   package entry points are rejected with this option and require separate
   worktrees.

The [Make guide](../mk/README.md) and native build/source-tools manual pages
describe the public commands and ownership rules.

## Final qualification

| Check | Windows | Debian Linux | FreeBSD |
| --- | --- | --- | --- |
| Production Make contract regressions | 47/47 | 47/47 | 47/47 |
| Warning reports compared with references | 340/340 | 340/340 | 340/340 |
| Native lock-order, callback, allocation and shutdown regression | Passed | Passed | Passed |
| Older screen implementation rejected by the same regression | Passed | Passed | Passed |
| Graphics ownership/source graphs | 31/31 | 31/31 | 31/31 |
| Source documentation and artifact checks | Passed | Passed | Passed |

The contract suite includes changed-warning rejection and explicit reference
updates, deterministic scan errors, same-path wrapper changes, bootstrap flag
changes and failed publication, stale/unfinished emission, parallel guard
ordering, and two independent native archive builds with different definitions.
Their cleanup preserves the other output tree and an unrelated source artifact.

Additional Windows checks replaced a native compiler-forwarding executable at
the same path and observed archive behavior change from 7 to 9 without changing
the Make command. The public Make warning and documentation targets passed.
Fresh converter output passed for seven source/configuration pages in HTML,
fbhelp and text; 168 Markdown links and 55 native links were checked.

The complete compiler/library dry run contained 1,638 concrete compile/link
outputs, all under its selected isolated root or its named compiler temporary.
It created no output directory. A subsequent real startup-object build created
its object and successful configuration record inside that root. A shared
quick-test request was rejected there.

Native bootstrap emission produced all 186 C files selected by the authoritative
compiler graph, totaling 14,295,114 bytes. Temporary copied inputs and pending
markers were removed. The emission record also passed under the minimal-runtime
child configuration. One generated output set is retained with its hash manifest;
contract fixtures, warning scratch and rendered manuals clean themselves up.
Unix checks reused the previous owned native workspaces. The transferred overlay
was about 365 KB, with no additional source snapshot or product rebuild.

Final Unix workers returned zero and verified their exact input manifest both
before and after qualification. A superseded Windows run overlapped edits;
the frozen final run passed all 47 cases. Initial Unix runner links omitted
Xpm, and a Linux summary used nonportable `tail -1` syntax. Both qualification
helpers were corrected and the final native workers completed successfully.

## Scope and retained boundaries

These are focused production-fragment and native C checks. They do not repeat
the earlier full compiler/runtime rebuild, 2,441-test language suite or example
sweep. The locking runner compiles the current screen and codec implementations
and links retained native MT libraries, recording their exact identities.

The null-driver regression does not qualify visible presentation, physical
input or every native driver's worker lifecycle. WinCE source is retained and
follows the documented contract; no WinCE SDK/runtime qualification is claimed.
One Make process must own each physical output root. Opaque wrapper dependencies
and SDK headers/libraries outside the dependency graph still require component
cleanup. Pre-generated bootstrap bundles without an emission record remain
supported through their selected target directory.

No maintained BASIC source or warning expectation was changed. The root
`bin/fbc.exe` remains unchanged, with SHA-256
`17c0fe049c35ec18764f7a5bb3e40c33d3e763a90f686dcd79e088a00492be4c`.
Changes remain uncommitted.

<!-- end of maintenance-refinements.md -->
