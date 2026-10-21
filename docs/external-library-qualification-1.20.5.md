<!--
Project: FreeBASIC example qualification
File: external-library-qualification-1.20.5.md
Purpose: Record the five-target external library audit on 2026-10-21.
Responsibilities: Measured results, removal decision, provenance and limits.
This file contains no claim of universal library or physical-device support.
-->

# External library qualification with FreeBASIC 1.20.5

The [repair follow-up](external-library-repairs-1.20.5.md) records the subsequent
binding/example fixes and Debian optional SDK additions. The original sweep
counts and receipts below remain unchanged.

All five sweeps covered the same 1,773 example source files. Compiler identities,
counts, remaining failures and source fingerprints are recorded in
[the result manifest](external-library-qualification-results.json).

| Target | All compile/link passes | External compile/link passes | Self-contained runs | Focused library checks |
| --- | ---: | ---: | ---: | ---: |
| Windows | 1682/1773 | 317/319 | 654/654 | 20 passed |
| DOS | 1282/1773 | 22/313 | 645/653 | 7 passed |
| Linux | 1656/1773 | 313/313 | 654/654 | 31 passed |
| Haiku | 1488/1773 | 158/313 | 651/654 | 22 passed |
| FreeBSD | 1583/1773 | 250/313 | 654/654 | 26 passed |

The counts include the focused overlays recorded in the manifest. Windows has
six additional examples classified as external rather than platform-specific.
Exampleageddon deliberately skips external examples at runtime. The focused
checks above are additional executions; they do not mean that every linked
external example was run. None of the selected final library checks failed.

## Removal decision

No libraries, bindings or examples were removed. Every one of the 313
non-Windows external example rows compiles and links on Linux with the
1.20.5 compiler and the matching available SDK. These results do not support
claiming that those libraries cannot work with FreeBASIC 1.20.5. No earlier
compiler release was tested in this audit.

A missing target SDK, an unavailable library architecture, a target binding
gap and a runtime failure are different findings. A failure on one target
does not establish that a library is unusable on its other targets.

## Qualification details and limits

- Windows used a 1.20.5 compiler and the qualified native runtime from the
  2026-10-20 build. Seven initial self-contained link failures came from an
  older installed runtime preceding that archive in the library search.
  All seven compiled and ran after correcting the search order. The two
  remaining external build failures are BASSMOD examples without a Win64 SDK.
- Linux used the native 1.20.5 candidate. Thirteen legacy SDK examples select
  x86 rather than x86-64. They were rechecked with the existing x86 compatibility
  runtime and SDK; the compiler remained 1.20.5. This does not qualify an entire
  newly packaged x86 1.20.5 distribution. The final four wx-c links also required
  the legacy SDK library search path. All 313 external examples then linked.
  A subsequent [fresh link confirmation](external-library-linux-link-confirmation.json)
  rebuilt all 313 external examples and verified every output as an executable
  ELF image: 300 x86-64 and 13 i386. This uses the matching private legacy SDKs;
  installing only the Debian optional package set does not reproduce those SDKs.
- FreeBSD used the native 1.20.5 candidate. Remaining failures include absent
  optional SDKs, the gettext link dependency in FB_Calc, and the C++ helper
  recipes selecting libstdc++ on a libc++ host. These were retained as actual
  target limitations, not global library incompatibility.
- Haiku used a checksum-verified, privately extracted 1.20.5 release package.
  Its three unbuilt self-contained examples reference newer idle and secure
  random runtime entry points absent from that package. Other recorded gaps
  include missing optional SDKs and the missing Haiku target branch in ffi.bi.
  The libffi call and closure examples ran successfully on Windows, Linux and
  FreeBSD, so this is a target binding gap.
- DOS used the checksum-verified published 1.20.5 runtime, genuine DJGPP
  cross-tools, and actual execution inside DOSBox-X. Five thread examples do
  not build with that single-thread package; three more need the newer idle
  and secure random entry points. All 645 built self-contained programs ran.
  A native DOS compiler preflight and 40-example compile batch are separate
  receipts. The complete compilation sweep was cross-built on Windows.
- The DOS QuickLZ decompression case initially could not read the compressed
  fixture handed between two work directories. A focused run of the same
  executable compressed and decompressed a file directly, returned zero for
  both operations, and reproduced every input byte. The original failure and
  the focused overlay are both retained.
- Focused checks cover controlled data, local APIs and loopback HTTP. They
  do not qualify all GUI interactions, audio devices, remote database servers,
  physical DOS hardware or every API in a linked library. Windows terminal
  input examples were deliberately left untested by the headless checker.

## Evidence and resource ownership

The local receipt root is `out/ea1205/`. It contains `inputs.json`,
`summary.json`, `external-matrix.csv`, all five full result tables, compile
diagnostics, focused runtime logs and the separate QuickLZ result. Unix
receipts are also retained as compact `qualification.tar.gz` archives.
The source snapshot pins 3,890 files; none changed between the initial hash
and the final verification. Existing dirty source changes and earlier
qualification workspaces were preserved. Generated product cleanup is
recorded separately in `out/ea1205/cleanup.json` and the result manifest.
Cleanup released 1.10 GiB from this audit's generated products and download.

<!-- end of external-library-qualification-1.20.5.md -->
