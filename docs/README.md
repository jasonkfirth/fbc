# FreeBASIC 1.20.5 documentation

This directory collects the documentation added or substantially revised from
the 1.10.0 release through the current 1.20.5 tree. The generated language
manual remains under `doc/manual`; these guides cover source architecture,
release, package, port, and runtime subjects that do not fit naturally in the
language reference.

## Start here

- [C emitter diagnostic audit](c-emitter-audit.md) describes unsuppressed GCC
  and Clang checks, structural analysis, reproducible receipts, emitter repairs,
  DATA/READ checks, termination contracts, public prototypes, label targets,
  temporary storage, private C type names and
  control-flow regressions. The [ranked findings](c-emitter-findings.md) identify the open
  contracts and scoped verification results; warning cleanup remains active.
- [C error-frame lowering](c-error-frame-lowering.md) preserves the global-handler
  failure, frame-ownership prototype and next compiler/runtime implementation boundary.
- [Static C aggregate initializers](c-emitter-initializers.md) explains named members,
  typed zero defaults, descriptor dimension tables and 128-bit constant separators.
- [C block declarations and packed copies](c-emitter-blocks.md) records declaration
  staging, evaluation order, lexical scope and named addresses for byte copies.
- [Semantic numeric conversion inputs](semantic-numeric-conversions.md) explains
  compiler-owned conversion snapshots, stream closure, internal carriers and
  constant-value safety.
- [Boolean and loop semantic inputs](semantic-boolean-loop-inputs.md) describes
  CONST destinations, original enum inputs and proved coordinate prefixes.
- [Source code guide](source-code/README.md) explains the source trees,
  compiler phases, runtime libraries, graphics, sound, target tools and tests.
  The manual's matching pages start at
  [DevSourceCode.wakka](../doc/manual/cache/DevSourceCode.wakka) and are linked
  from its main and developer tables of contents.
- [Graphics maintenance guide](gfx-maintenance.md) maps gfxlib2 and gfxlib3
  ownership, locking, image contracts, backend modules and validation workflows.
- [Graphics restructuring verification](gfx-restructure-verification.md)
  records native regression gates and their scope.
- [Runtime and graphics locking](gfx-locking.md) explains input callbacks,
  allocation and mode shutdown, with a native lock-order regression.
- [Runtime maintenance guide](../src/rtlib/README.md) maps runtime subsystems,
  platform drivers, source selection, ABI contracts, ownership and regression workflows.
- [Runtime restructuring verification](rtlib-restructure-verification.md)
  records the refactor inputs and native qualification scope.
- [Current changelog](../changelog.txt) records the 1.20.5 changes.
- [1.20.4 release notes](release-1.20.4.md) summarize the user-visible work
  completed since the 1.20.2 development baseline.
- [1.10 to 1.20 development history](release-history-1.10-to-1.20.md) records
  the actual version boundaries and groups the work from all 804 audited
  commits after the 1.10.0 tag.
- [Package repository and installation](packages.md) explains the packages at
  `deb.fbxl.net`, installation choices, target packages, and checksums.
- [NuttX and RISC-V](nuttx.md) describes the QEMU, ESP32-P4, and RP2350-PiZero
  development workflows.
- [Portable serial control](serial.md) documents the `fbcom.bi` API layered on
  top of `OPEN COM`.
- [Compatibility runtime and compiler services](compatibility-services.md)
  documents the byte-string, pathname-attribute, bitmap-font measurement,
  packed-fill, file-copy, indexed-readback, and concurrent-build work.
- [Platform services and native graphics drops](../doc/platform-services.md)
  describes checked notifications, cooperative idle polling, Windows and X11
  file/text drops, examples, and their platform qualification limits.
- [Checked secure randomness](../doc/secure-random.md)
  documents the caller-owned API contract, provider and refusal behavior, DOS
  device protocol, and focused verification boundaries.
- [gfxlib3](gfxlib3/README.md) describes the opt-in render-threaded GPU graphics
  runtime and links to its architecture, parity, performance, and verification
  records.
- [sfxlib examples](../examples/sfxlib/README.md) introduces generated sound,
  sample playback, MIDI, output capture, diagnostics, and stress examples.

The complete per-change record is in [changelog.txt](../changelog.txt). The
generated manual is the authoritative reference for established language
keywords and the built-in graphics and sound command syntax.

## Platform guides

- [AROS](aros.md)
- [Classic AmigaOS](amiga.md)
- [RISC OS](riscos.md)
- [Windows CE](wince.md)
- [Linux MIPS](mips.md)
- [NuttX](nuttx.md)

Cross-target example results and known external-library limitations are kept
beside the examples:

- [JavaScript, Android, and Xbox notes](../examples/cross-target-notes.md)
- [Cygwin notes](../examples/cygwin-example-notes.md)
- [Windows notes](../examples/windows-example-notes.md)

## Build and qualification records

- [Make system review](make-system-review.md) tracks the full build-system
  audit, confirmed defects, corrections, and remaining review coverage.
- [Build consolidation](build-consolidation.md) records native integration,
  configuration tracking, source hygiene and compiler publication checks.
- [Compiler log tests and retries](compiler-test-harness.md) documents failure
  details, retry artifact ownership, and the focused harness regression.
- [Compiler semantic model](../doc/compiler-semantic-model.md) defines the
  schema 27 sidecar, source and macro origins, physical written-type root
  anchors, bounded observations, and the
  binary/text reader contract, including direct native field reads without an
  intermediate TSV record. It also documents requested source-observation
  groups, exact macro variable/member node links with counted closure,
  direct ERASE/REDIM target observations, native/stable datatype IDs and the
  dedicated semantic-build default. Generated
  unnamed parameters without source ownership are excluded from hint closure. The eleven
  optional groups also cover include requests/scopes, accepted builtin uses,
  original literal adjacency, preprocessor event counts and independently requested
  array-element closure. Written declaration types and native byte conversions
  are independently requested through `type-shapes` and `native-bytes`; unrelated
  exports omit those families. Windows byte paths
  are converted only when opening files; wire identities and source hashes remain
  intact. Declaration initializer inputs belong to source variables; nested NEW
  temporaries retain their original expression capture without being exported as
  source declarations. DIM, VAR and ordinary assignment cases are checked by the
  semantic-sidecar initializer regression.
  Full models also expose the compiler-selected automatic cleanup decision for
  variables. Original storage forests retain grouped built-in power while keeping
  arbitrary source calls opaque; focused branch-observation tests compare native
  output with observation enabled and disabled.
  Declaration observations retain selected typedefs, alias declaration sites,
  original string indices and actual record receivers before lowering. The
  maintained declaration regression covers inherited fields, implicit writes,
  member-valued defaults, generic casts, wide integer initializers, native/stable
  datatype encoding and emission parity.
  Written type observations retain callback nesting, named alias boundaries,
  explicit conventions and actual array ownership. Native byte observations
  retain selected numeric pack/unpack operations, including folded decoders.
  Both have counted closure; the compiler contains no complexity score or
  byte-order policy. CVI/MKI width validation rejects unsupported helpers before
  lowering, while preserving the documented 16/32/64-bit forms.
- [Cross-build matrix](../build_scripts/cross-build-matrix/README.md)
- [Emulated native matrix](../build_scripts/emulated-native-matrix/README.md)
- [gfxlib3 test plan](gfxlib3/test-plan.md)
- [gfxlib3 verification record](gfxlib3/verification.md)

The build records are evidence for maintainers. They are not a promise that
every optional target or external dependency is available on every host.

The [1.20.5 external library audit](external-library-qualification-1.20.5.md)
records fresh Windows, DOS, Linux, Haiku and FreeBSD Exampleageddon results,
focused library executions and the evidence for retaining the bindings.

The [library repair follow-up](external-library-repairs-1.20.5.md) records
native ABI fixes, QuickLZ qualification on FreeBSD and Haiku, and complete
Debian optional SDK coverage with an automatic packaging guard.

## Building the generated manual

Build the FreeBASIC documentation tools first, then generate every manual
format from the checked-in Wakka cache:

```sh
make -C doc
make -C doc/manual clean
make -C doc/manual all
```

The complete build requires `zip`, `gzip`, and Microsoft's HTML Help Compiler.
Pass the compiler location as `HHC=/path/to/hhc` if it is not on `PATH`. The
resulting versioned CHM, HTML, text, fbhelp, and Wakka archives are written to
`doc/manual/`. A release build is not complete until each archive exists and
the generated `html/00index.html` links to the new manual pages.

<!-- end of README.md -->
