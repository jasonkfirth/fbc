# FreeBASIC 1.20.6 documentation

This directory collects the documentation added or substantially revised from
the 1.10.0 release through the current 1.20.6 tree. The generated language
manual remains under `doc/manual`; these guides cover source architecture,
release, package, port, and runtime subjects that do not fit naturally in the
language reference.

## Start here

- [Native procedure policy](c-emitter-noinline.md) records contextual NoInline,
  preserved C2FB attributes and complete allocator comparisons.
- [Call termination](c-emitter-call-termination.md) records conditional GNU
  error contracts, native comparisons and the whoami warning repair.
- [Process adapters](c-emitter-process-adapters.md) records native Windows
  preprocessing, portable command arguments and registered DLL regressions.
- [Volatile updates](c-emitter-volatile-updates.md) records scalar and pointer
  writes, mapped storage widths and native access-event checks.
- [Volatile read addresses](c-emitter-volatile-read-addresses.md) records
  read-only load adapters, owning-module refresh and native value checks.
- [Coreutils change-reporting formats](c-emitter-change-formats.md) records
  original-C comparisons, private caller contracts and native type controls.
- [Shared case/default lowering](c-emitter-shared-default-switches.md) records
  direct converter dispatch, preserved fallthrough and native value checks.
- [Coreutils borrowed slots](c-emitter-coreutils-readonly-slots.md) records
  callback qualifiers, regeneration profiles and native value checks.
- [SQLite insertion phases](c-emitter-sqlite-insertion-phases.md) records
  the GCC-clean SQLite census, checked phase boundaries and native regressions.
- [SQLite read-only vectors](c-emitter-sqlite-readonly-vectors.md) records
  constructor qualifiers, schema rows, legacy callbacks and native ABI checks.
- [SQLite constraint lookups](c-emitter-sqlite-constraints.md) records checked
  WHERE terms, cached RHS values, cleanup and native regressions.
- [VT100 runtime symbol](c-emitter-vt100-symbol.md) records the ordinary C name,
  preserved application overrides and complete native warning reduction.
- [Native platform owners](c-emitter-native-platform-owners.md) records Haiku
  archive selection, native SDK execution and Windows UTF-16 qualification.
- [SQLite DROP COLUMN boundaries](c-emitter-sqlite-drop-column.md) records
  missing-token handling, cleanup ownership and native precision types.
- [SQLite virtual-table transactions](c-emitter-sqlite-vtab-transactions.md)
  records checked-instance ownership, transaction growth and reference checks.
- [Coreutils forwarded formats](c-emitter-forwarded-formats.md) records complete
  copy-output helper execution and the private `cp` format-call proof.
- [Coreutils numeric formats](c-emitter-numfmt-formats.md) records both floating
  builders, current runtime array cleanup and retained native type diagnostics.
- [Coreutils sequence formats](c-emitter-sequence-formats.md) records floating
  sequence normalization, native arguments, endpoint rounding and failure checks.
- [Coreutils configured formats](c-emitter-configured-formats.md) records the
  line-number and split-filename parser, output and retained-diagnostic checks.
- [Object-pointer equality](c-emitter-pointer-equality.md) records differing
  pointer views, single evaluation and retained source diagnostics.
- [Coreutils idle-time ranges](c-emitter-idle-formats.md) resolves the two extra
  GCC O0 format-overflow observations through actual helper executions.
- [Coreutils time-format contracts](c-emitter-time-formats.md) records the four
  completed native format, capacity, callback and timestamp-fallback checks.
- [Typed union member selection](c-emitter-union-members.md) records overlapping
  pointer-member recovery, retained const-removal diagnostics and native checks.
- [Current native runtime replay](c-emitter-runtime-current-refresh.md) records
  the complete MT/non-MT builds, individual execution outcomes and format owners.

- [Obstack allocation callbacks](c-emitter-obstack-callbacks.md) records the
  four-warning library repair, regeneration checks and native failure controls.

- [SQLite error-path connection](c-emitter-sqlite-error-context.md) records the
  three-warning library repair and exact before/after callback regression.

- [Checked ASM register and stack effects](c-emitter-asm-register-effects.md) records
  32-bit PIC operands, x86-64 spill protection, leaf CALL alignment and native controls.
- [ASM storage ownership](c-emitter-asm-storage-ownership.md) records private global
  retention, naked local-static aliases, initializer placement and native/LTO controls.

- [Checked numeric parsers](c-emitter-numeric-parsers.md) records COM/date range
  repairs, bounded date spans, native fault controls and public API checks.

- [Terminal masks and escape bytes](c-emitter-terminal-masks.md) records the
  native-width flag repair, exact runtime objects and OPEN COM execution checks.

- [Runtime record alignment](c-emitter-runtime-alignment.md) records shared
  layout guards, native warning reductions and exact runtime object comparisons.

- [Native ABI observations](c-emitter-abi-observations.md) records layout guards,
  native lifetime controls and the dominant Clang diagnostic contracts.

- [Console coordinate initialization](c-emitter-console-initialization.md)
  records Unix LOCATE's paired diagnostics and value/callback qualification.

- [Native byte-memory arguments](c-emitter-byte-memory.md) records containing
  object addresses, real-overrun controls and current production qualification.

- [Conditional native formats](c-emitter-iif-formats.md) records IIF result
  contracts, whole-port warning counts and native value/diagnostic qualification.

- [Native character adapters](c-emitter-wide-adapters.md) records the shared
  runtime conversion repair, full object comparisons and locale controls.

- [Managed field cleanup dependencies](c-emitter-cleanup-dependencies.md) records
  the selected compiler, complete Templari paired census and native cleanup checks.

- [Private format pointers](c-emitter-local-formats.md) records the selected
  compiler, native alias controls and complete production warning reconciliation.

- [C2FB enum argument widths](c-emitter-enum-widths.md) records the shared
  C-int correction, native type controls and all 43 remaining format-source owners.

- [Qualified pointer typedefs](c-emitter-qualified-typedefs.md) records the C2FB
  type and signed-byte repairs, GCC-clean converter and native value checks.

- [Plural translation formats](c-emitter-plural-formats.md) records the selected
  compiler, both source-format contracts, 64 native executions and current totals.

- [Literal suffixes and native format checks](c-emitter-literal-offsets.md) records
  the current development compiler, seven removed warnings per native tool and
  the corrected C2FB ABS result width.

- [Coreutils formatter ownership](c-emitter-coreutils-formatter.md) records the
  shared-helper link repair and native checks for the largest remaining format cluster.
- [Templari callback and GCC qualification](c-emitter-templari-callback.md) records
  the worker signature repair, 651 production profiles and two qualified API casts.
- [FBSound owning build contexts](c-emitter-fbsound-contexts.md) records 236
  GCC-clean profiles and 52 newly qualified source paths.
- [C2FB pointer qualifiers](c-emitter-pointer-qualifiers.md) records the
  eight-warning library repair and exact native object comparisons.
- [Evaluated translation results](c-emitter-format-results.md) records the
  560-warning reduction per native tool and corrected CRT formatter signatures.
- [SQLite native integer types](c-emitter-sqlite-native-types.md) records the
  88-warning library repair, native ABI controls and complete production replays.
- [External declarations](c-emitter-external-prototypes.md) records exact extern
  coalescing, preserved conflicting declarations and the 135-warning reduction.
- [Unused C parameters](c-emitter-unused-parameters.md) records GCC draft-C23
  capability selection, the 343-warning port reduction and development promotion.
- [Additional project GCC checks](c-emitter-additional-projects.md) records
  Templari, Midisoft, Rotator3d, FS4, HART, SQLite and coreutils production queues.
- [Recursive expression source locations](c-emitter-parser-source-locations.md)
  records four recovered compiler crashes, stack use and the selected compiler.
- [Library CRT constants](c-emitter-library-crt-constants.md) records the Linux
  RAND_MAX guard, 940 unchanged Windows readers and explicit Linux port failures.
- [Library providers and source coverage](c-emitter-library-providers.md) records
  typed libpq handles, ST/MT build selection and complete library source accounting.
- [Typed library sorting](c-emitter-library-sort.md) records corrected CRT
  callback types, reentrant custom sorting and retained FBC diagnostics.
- [C-long aliases and variadic anchors](c-emitter-c-long-varargs.md) records
  corrected C++ scalar type codes, C23 builtin selection and native ABI values.
- [Forward pointer results and private TLS names](c-emitter-forward-results.md)
  records completed return types, native acceptance and internal TLS identifiers.

- [Checked ASM procedure references](c-emitter-asm-reference-analysis.md) records
  the 61 removed Win32 unused-function observations and current-source selection.

- [Static locals and evaluated values](c-emitter-static-locals.md) records unread
  debug storage, preserved floating evaluation and original-context GCC repairs.

- [ASM procedure retention](c-emitter-asm-procedures.md) records native entry
  preservation, inactive-reference controls and ordinary CRT/DJGPP C aliases.

- [Canonical corpus contexts](c-emitter-census-reproducibility.md) records the
  retired private include path, native reuse rules and reproduction requirements.

- [Runtime-backed TLS arrays](c-emitter-runtime-tls-arrays.md) records computed
  storage, bounds checks, module initialization and terminating C epilogues.

- [Terminating callbacks](c-emitter-noreturn-callbacks.md) records the exact
  signature conversion, native address checks and preserved mismatch diagnostics.

- [Intel assembly storage addresses](c-emitter-asm-addresses.md) records the
  selected Win32 operand-limit repair, live parameter aliases, floating bit
  preservation, complete recount and remaining qualification boundaries.

- [Runtime library warning audit](library-warning-audit.md) separates complete
  per-object warning captures from current native runtime checks and pending
  source recounts across runtime, graphics and sound libraries.

- [DOS qualification](dos-qualification.md) covers the DOS ZIP build, bounded
  emulator runner, x87 preflight, unit-suite CI and remaining acceptance work.

- [Qualified pointer slots](c-emitter-const-slots.md) records the new source
  diagnostic, safe value temporaries, GCC-clean fb-ext-lib expansion and
  the selected compiler's complete broad qualification.

- [Pointer fields carried as integer words](c-emitter-pointer-field-words.md)
  records eight removed GCC observations, original array-test execution and
  complete current-compiler queue qualification.

- [Tiko widget lifecycle guards](c-emitter-tiko-widget-guards.md) records three
  source checks, the headless regression and the complete 126-context recount.

- [Unread debug storage](c-emitter-debug-locals.md) explains preserved source
  locals, discarded TYPEOF scratch and unchanged source/data-flow diagnostics.

- [Debug parameters and private bodies](c-emitter-debug-parameters.md) records
  named formal storage, executable private debug entries, preserved source
  warnings and the original-flags native rechecks.

- [Static typed thread callbacks](c-emitter-thread-callbacks.md) records native
  adapters, preserved address casts, GAS64 narrowing and the completed retained census.

- [Guarded label payload qualification](c-emitter-label-guard.md) binds the
  unchanged real fixture's GCC bounds warning to both native runtime passes.

- [Tiko C warning qualification](c-emitter-tiko-context.md) records the recovered
  maintained build context, four source guards and unchanged native diagnostics.

- [Hosted CI recovery](ci-recovery.md) explains runner allocation failures,
  master run scheduling, selectable macOS ARM64 images and scoped native
  macOS/Windows ARM64 qualification, independent test steps and committed
  documentation checks, including conservative documentation-only scheduling,
  independent FreeBSD suites and deterministic TCP short-read qualification.

- [Window metrics and software keyboards](../doc/window-keyboard.md) documents
  browser and Android usable geometry, density, rotation, IME requests and
  observation, including the shared gfxlib2/gfxlib3 `ScreenControl` contract.

- [C emitter diagnostic audit](c-emitter-audit.md) describes unsuppressed GCC
  and Clang checks, structural analysis, reproducible receipts, emitter repairs,
  DATA/READ checks, termination contracts, public prototypes, label targets,
  temporary storage, private C type names and
  control-flow regressions. The [ranked findings](c-emitter-findings.md) identify the open
  contracts and scoped verification results; warning cleanup remains active.
- [Effective callback conventions and temporary names](c-emitter-callback-conventions.md)
  records stored-address conversions, retained ABI boundaries and reproducible C.
- [Native compiler label flags](c-emitter-label-flags.md): 178 fewer Clang observations with unchanged Win32/Win64 record sizes and complete corpus coverage.
- [Current main qualification](c-emitter-current-main.md): actual current compiler build, both backend suites and complete corpus binding.
- [USTRING conversions](c-emitter-ustring-conversions.md): checked byte/scalar boundaries, native file I/O and unchanged runtime ABI.
- [String-capacity source-package replay](c-emitter-source-replay.md): cold source builds, exact relocated BASIC self-hosting and unchanged installed binaries.
- [String storage capacity](c-emitter-string-capacity.md): checked growth, allocation-failure ownership, bounded heap tests and 34 fewer GCC/Clang observations each.
- [UTF conversion spans](c-emitter-utf-spans.md): checked byte counts, supplementary scalars, caller-owned buffers and 104 GCC/96 Clang fewer runtime observations.
- [Console writer spans](c-emitter-console-spans.md) covers checked Unix geometry, counted wide output, resize ownership and cross-platform runtime controls.
- [Unix process and printer buffers](c-emitter-unix-transient-buffers.md) records checked transient storage, native warning reductions and process/printer regression results.
- [Filesystem and OS-query sizes](c-emitter-filesystem-sizes.md) records executable-path truncation checks, native count conversions and free-memory limits.
- [Coreutils stat formats](c-emitter-stat-formats.md) records seven native format-builder contracts, output comparisons and format-type diagnostics.
- [Coreutils word-count formats](c-emitter-wc-formats.md) records all selected fields, native argument checks, output failures and the current runtime replay.
- [Coreutils shared formatter](c-emitter-coreutils-formatter.md) records all six provider identities, native value/allocation matrices and retained dynamic-format diagnostics.

- [Stored translation formats](c-emitter-stored-translations.md) records saved literal contracts, pointer-copy proofs and retained native diagnostics.
- [Unsigned string conversions](c-emitter-string-scalars.md) records native parser widths, raw wide-character values and cross-platform regression checks.
- [Unix driver contracts](c-emitter-unix-driver-contracts.md) records callback signatures, shared focus ownership and legacy termcap arguments.
- [Keyboard and line input](c-emitter-keyboard-input.md) records X11 callback types, scan-code mapping ownership and native input conversions.
- [Formats across calls](c-emitter-call-formats.md) records private literal provenance, nonlocal-return boundaries and development-main promotion.
- [Unsigned variadic arguments](c-emitter-vararg-promotions.md) records native promotions, argument-value checks, preserved diagnostics and the Coreutils source refresh.
- [Linux native API boundaries](c-emitter-native-api-boundaries.md) records GCC-clean MT/non-MT runtime builds and public runtime executions.
- [Native runtime padding](c-emitter-native-padding.md) records named alignment storage, native layout equivalence and rebuilt runtime regressions.
- [Native I/O counts](c-emitter-native-io-counts.md) records bounded POSIX counts, socket-length types, pthread reserves and exact-binary qualification reuse.
- [Runtime C declaration contracts](c-emitter-runtime-prototypes.md): 108 owned interfaces, unchanged object bytes, checked Win32 decorations and portable C/C++ clients.
- [RND seed-vector ownership](c-emitter-rnd-state.md): private helper placement, preserved sequences, empty fills and 2,072 fewer native Clang observations.
- [Little-endian word helpers](c-emitter-utf-words.md): fixed byte copies, 8,296 fewer native Clang observations and real encoded file I/O.
- [String length contracts](c-emitter-string-length-contracts.md): signed lengths, const descriptor reads, rebuilt normal archives and 224 self-host identities.
- [Owned runtime macros](c-emitter-runtime-macros.md): 5,319 fewer Clang observations, exact runtime objects and cumulative source delivery.
- [Runtime C/ABI symbol bindings](c-emitter-runtime-symbol-bindings.md): 8,326 fewer Clang observations, unchanged code/exports and cumulative source delivery.
- [Runtime C type tags](c-emitter-runtime-type-tags.md): 32,249 fewer Clang observations, unchanged ABI/object bytes and a compact source update.
- [Shared string header conversions](c-emitter-string-header-conversions.md): allocation limits, ownership flags and 7,259 fewer native warnings per C tool.
- [Experimental source rebuild](c-emitter-portable-build.md): consolidated compiler/core runtime sources, source-only archive, cold native build and qualification limits.
- [GObject type callbacks](c-emitter-glib-type-callbacks.md) records shared
  signature adapters, macro expansion repairs and native registration tests.
- [C argument and storage widths](c-emitter-argument-widths.md) records real
  example contract repairs, original-source diagnostics and native values.
- [Compiler-owned debug bindings](c-emitter-debug-bindings.md) records scoped
  storage and descriptor cleanup, debugger type controls and GCC promotion gates.
- [Debug scratch cleanup](c-emitter-debug-scratch.md) records current complete
  counts, retained debug bindings and native/runtime qualification.
- [Compiler source warnings](compiler-source-warnings.md) describes source feedback
  retained during C cleanup, warning levels, exclusions and regression commands.
- [Current C warning status](c-emitter-current-status.md) gives the latest
  selected queue counts, excluded profiles and links to the qualification receipts.
- [GOSUB and handler label ownership](c-emitter-gosub-labels.md) records callback
  references, AST lifetime boundaries and five fewer warnings per C compiler.
- [Current JRPG contexts and retained debug bodies](c-emitter-newjrpg-context.md)
  records two source pointer repairs and the pending 241-program object sweep.
- [Read-only scalar conversions](c-emitter-qualified-conversions.md) records
  correct Single helpers, LLVM intrinsic types and the pending complete recount.
- [Shared C startup registrations](c-emitter-startup-groups.md) records the class-wide
  MinGW repair, callback order, semantic facts and native/object qualification.
- [Native C pointer values](c-emitter-pointer-values.md) and [named byte-copy slots](c-emitter-member-address.md)
  record address formation, guarded storage identity and the shared warning-class reduction.
- [C record field layout](c-emitter-record-layout.md) records offset assertions, native ABI controls
  and the rejected explicit-padding experiment.
- [C external prototype reachability](c-emitter-external-prototypes.md) records discarded declarations,
  retained public and callback roots, Windows intrinsics and whole-queue diagnostics.
- [Current paired C warning census](c-emitter-current-queues.md) records all four
  completed queues, warning classes and exact frontend/build-context boundaries.
- [Fixed managed object arrays](c-emitter-fixed-object-arrays.md) records the
  remaining real parser guard repaired through ordinary compiler array cleanup.
- [Generated scalar copy ownership](c-emitter-copy-owners.md) records five real
  cleanup guards removed, packed-copy controls and preserved diagnostic counts.
- [Current ordinary C compile timing](c-emitter-current-timing.md) records the fresh
  GCC deadline pass, retired capture prototype and real scalar-copy ownership guards.
- [Managed user-destructor ownership](c-emitter-managed-destructors.md) records partial
  field retirement, interrupted destruction and fresh ERROR-suite qualification.
- [User-constructor cleanup ownership](c-emitter-constructor-owners.md) records the private
  partial-construction extension, current-layout rebase and retained promotion limits.
- [Anonymous C record layout](c-emitter-anonymous-layout.md) records container packing,
  native C declarations, LTO compatibility and permanent regression coverage.
- [Packed descriptor trailing layout](c-emitter-packed-tail.md) records exact record sizes,
  array strides, initialized storage and C/native call qualification.
- [Current paired C queue recount](c-emitter-packed-queues.md) records both compiler cuts,
  completed diagnostics and the unresolved ordinary GCC timing gate.
- [Packed descriptor layout and access](c-emitter-packed-descriptors.md) records the compiler/library
  repair, unchanged public storage ABI and complete affected Windows native cohort.
- [Managed-field ownership](c-emitter-managed-fields.md) records partial generated cleanup,
  heap accounting and the remaining nested packed-layout failure.
- [Cleanup error state](c-emitter-cleanup-error-state.md) records preserved handled errors,
  nested callbacks, actual array/destructor cleanup and permanent runtime regressions.
- [Object-array retirement](c-emitter-array-objects.md) records bounded reverse destruction,
  native/BASIC parity, real object lifetimes and unsuppressed diagnostic checks.
- [Native C array values](c-emitter-array-values.md) records unsigned scaling before actual reads
  and writes, GAS64 comparisons, instrumentation and whole-queue diagnostics.
- [C continuation dispatch](c-emitter-dispatch-values.md) records header placement,
  the shared warning reduction, native comparisons and the moving Optical input boundary.
- [Owned C continuations](c-emitter-owned-continuations.md) records current lifetime qualification
  and the mixed GAS/C failure that prevents promotion of the shared dispatch repair.
- [Current ERROR object lifetimes](c-emitter-owner-lifetimes.md) records constructor completion,
  normal retirement and fresh mixed-backend checks on the latest private owner graph.
- [Native handler activation ownership](c-emitter-native-owners.md) records recursive
  saved-handler identity, automatic native retirement and threaded qualification.
- [Compiler-emitted ERROR continuations](c-emitter-c-resume-lowering.md) records source-scope
  transfers, real-project warning-class reductions and remaining promotion contracts.
- [Exampleageddon C warning queue](c-emitter-exampleageddon.md) records the full example
  inventory, ordinary/checked diagnostics and compiler-only repairs.
- [Natural C record padding](c-emitter-padding-contract.md) records retained ABI diagnostics
  and the rejected explicit-padding and alignment controls.
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
- [Current changelog](../changelog.txt) records the 1.20.6 changes.
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

## Contextual identifiers and example compilation

[Contextual THREAD and existing build contexts](c-emitter-thread-context.md)
records the shared compiler repair, 126 unchanged recovered examples,
unsuppressed object diagnostics and remaining full-census limits.

## C record dependencies

[Pointer-only record dependencies](c-emitter-record-pointers.md) documents
shared declaration pruning, fixed points and mixed C ABI checks.

[Deferred prototype record views](c-emitter-prototype-views.md) documents
forward dependencies, stable prototype aliases and completed packed access views.

[Deferred record declaration scope](c-emitter-record-scope.md) documents
outer tag completion, local symbol reuse and unchanged-program regression checks.

[Variant library native contracts](c-emitter-variant-abi.md) records the shared
Windows layout, text conversion and ownership repairs, with executable
regressions and separate Win32 object qualification.

- [Unused C parameters after store removal](c-emitter-unused-parameters.md): GCC reductions, ABI controls and promotion priority.

- [Main GCC promotion](c-emitter-main-gcc-promotion.md): current sources, complete GCC counts, matching runtime and preservation limits.


[Windows CRT layout and ordinary C names](c-emitter-crt-contracts.md) records
native SDK contracts, current paired warning counts, preserved experimental
policies and the missing clean external-rebuild proof.

<!-- end of README.md -->
