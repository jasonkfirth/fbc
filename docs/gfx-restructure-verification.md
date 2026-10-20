<!--
Project: FreeBASIC graphics libraries
File: gfx-restructure-verification.md
Purpose: Record the gfxlib maintenance refactor and its qualification scope.
Responsibilities: Identify preserved contracts, inputs and native test evidence.
This file intentionally does NOT promise qualification of untested platforms.
-->

# Graphics restructuring verification

The refactor begins at `e5d0ca9180dbcfbe461ea2c94a7d1ab20cc379ce`.
Shared gfxlib2 and gfxlib3 implementations now have explicit subsystem owners.
The default graphics ABI, image and event layouts, platform adapters and BASIC
semantics are retained.

OpenGL and Vulkan use independently compiled implementation modules and private
backend headers. Device/context creation, surface storage, command submission,
geometric drawing, image transfers and presentation have named owners. The
[maintenance guide](gfx-maintenance.md) and source maps describe the boundaries.

## Mechanical checks

- 114 original shared C files were assigned to explicit Make source groups.
- The directory move changes include paths and ownership documentation.
- All 215 extracted backend function bodies, private layouts and OpenGL shader
  strings match the starting revision after private helper renames. Make selects
  the same original implementations in 28 target/architecture comparisons.
- The permanent structure guard checks 31 target/architecture/feature graphs,
  local include resolution, private-header boundaries, dependencies, ownership
  headers and archive-member uniqueness.
- The split backend units pass native GCC syntax and warning checks.
- Synthetic collisions verify rejection of both duplicate C members and a
  C/C++ pair which would otherwise produce the same archive member.

## Initial native qualification

| Native host | Unit tests | Assertions | Language records | Exampleageddon self-contained |
| --- | ---: | ---: | ---: | --- |
| Windows x86-64 | 2,441 passed | 1,621,936 passed | 1,834 passed | 656/656 passed |
| Midcom Linux x86-64 | 2,441 passed | 1,622,047 passed | 1,834 passed | 656/656 passed |
| FreeBSD VM134 x86-64 | 2,441 passed | 1,622,044 passed | 1,834 passed | 656/656 passed |

Each language count comprises 1,740 FB, 86 QB, three FBLite and five Deprecated
records. Warning generation completes 68 fixtures for each of the five
maintained target profiles on each host.
Receipt collection verifies one passing result per language record, the full
unit totals, native stage exits and unchanged input hashes.

Windows completed the required components serially after interrupted runners
overlapped. Its receipt collector validates the retained passing unit run and
every final language result. The original failed wrapper log is retained
separately; the result does not claim that invocation exited successfully.

The full example corpus has 1,773 sources on every host:

| Host | Compile passes | Classified compile failures/timeouts | Run failures/timeouts |
| --- | ---: | ---: | ---: |
| Windows | 1,591 | 182 | 0 |
| Midcom | 1,574 | 199 | 0 |
| FreeBSD | 1,583 | 190 | 0 |

Receipts and frozen input hashes are under `out/gfx-maintenance`. A concurrent
rtlib restructuring appeared during the initial build. Qualification therefore
uses bounded snapshots of the starting revision plus the graphics changes,
preserving the concurrent checkout edits and keeping each result attributable
to a defined input set.

Each completed run verifies 9,954 actual input files. Generated warning
references, object lists and failed-only caches are excluded from source-input
hashing; these full-suite runs do not use `FAILED_ONLY=1`. Graphics archive
checks reject duplicate strong definitions and missing archive-index entries.
The initial 359 graphics inputs matched the working checkout; five pre-existing
checkout line-ending conversions were equivalent to the frozen Git source bytes.

After the final navigation comments, rebuilding the two Windows gfxlib3
archives and four Midcom gfxlib3 archives produces identical SHA-256 hashes.
FreeBSD does not build those backend archives. This closes the comment-only
delta after the native test runs without repeating the full suites.

Compact local receipts are named `out/gfx-maintenance/windows-evidence.zip`,
`midcom-evidence.zip` and `freebsd-evidence.zip`, with corresponding
`*-summary.json` files. `evidence-index.json` records their sizes, SHA-256
hashes and required gate totals. The bundles include compiler/archive hashes,
native logs and classified corpus reports, without copying build binaries.

The ordinary fbctests gate includes unit, language/log and warning tests.
Exampleageddon's required gate is the self-contained runnable subset; classified
optional-library and platform failures remain visible in its corpus report.
gfxlib3's separate graphics parity wrappers and backend fixtures extend the
default gfxlib2 coverage on supported hosts.

FreeBSD has no native gfxlib3 window adapter. Its native regression gate
qualifies gfxlib2 and the matching compiler/runtime. Source graph checks for
other targets do not substitute for native rendering or controller tests there.

## Drivers directory follow-up

Native adapters, shared Unix glue and CPU kernels now live below each library's
`drivers` directory. This moves 156 files in 28 directories and removes the OS
and CPU directories from beside the shared subsystem owners. The driver maps
link back to the shared contracts and explain special source-inclusion adapters.

Comparison against the pre-move capture verifies 357 non-document files without
algorithm changes. Every local include still resolves to the same file. All
31 target/architecture/feature source lists are unchanged after normalizing the
new directory component. The permanent guard rejects native folders at the old
root locations and covers native headers and textual fragments.

The eight Darwin compile patterns retain their Blocks flags. Haiku's runtime
header lookup, lint suppressions, NuttX reduced staging, and Amiga/DOS fixture
paths follow the new layout. Lint-path checks exercise both shared subsystem
and selected native discovery for Linux and FreeBSD; shell syntax checks pass.
The current runtime structure check also passes its 126 graph cases and three
collision guards, including Haiku's shared graphics-header lookup.

The existing native qualification workspaces are reused for forced graphics
rebuilds, retaining the qualified compiler/runtime and moving native object
directories rather than keeping another object tree. Midcom's eight graphics
archives, FreeBSD's four archives and Windows' four archives are byte-identical
to the archives from the completed native suites above. Every host verifies
9,956 unchanged input files after rebuilding; the two additional inputs are
the driver-directory source maps. The existing fbctests, Exampleageddon and
GPU-fixture results therefore apply to the rebuilt archive bytes.

Follow-up source hashes, build logs and binary-identity receipts are under
`out/gfx-drivers`. Each native identity receipt names the SHA-256 of the earlier
full-suite evidence bundle. This preserves the distinction between rebuilding
the relocated sources and repeating the full suites.

## Additional graphics checks and carried failures

The existing explicit OpenGL GPU-surface state fixture passes all seven stages
on Windows and Midcom. The explicit Vulkan GPU-surface fixture passes on both
hosts. Midcom uses a separate Xvfb display. Explicit renderer flags constrain
selection to that GPU API, including retries with fullscreen inverted.

The unchanged gfxlib3 unit wrapper runs 102 tests with 469,679 assertions.
Four assertions fail in `gfx/legacy-image-header.bas`'s custom-font PSET case.
Replacing the split backend objects with the original monolithic backend
objects reproduces those same four failures on both Windows and Midcom. The
gfxlib3 log lane passes. This refactor retains that known compatibility debt.

The standalone C infrastructure fixture also reproduces the same failed
checks and later crash with original and split Windows backend objects. The
reason for its later crash has not been isolated. The
historical Vulkan bootstrap fixture has batching and conservative surface-limit
expectations which differ from the current runtime. Neither C fixture is
reported as passing qualification here.

The warning harness needed a portable invocation for these native gates: Bash
is resolved through PATH, the toolchain environment is passed separately from
the compiler executable, and missing tools, abnormal exits and empty source
filters fail the runner. Its reference-output generation retains ordinary
compiler-error cases used by warning fixtures.

<!-- end of gfx-restructure-verification.md -->
