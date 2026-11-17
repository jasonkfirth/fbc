<!--
Project: FreeBASIC C backend verification
File: c-emitter-asm-register-effects.md
Purpose: Record checked Intel register effects and native PIC qualification.
Responsibilities: Explain clobber ownership, stable frames and observed controls.
The project queue receipts own their separate whole-graph warning totals.
-->

# Current private storage follow-up

The [naked procedure storage repair](c-emitter-asm-storage-ownership.md) advances
development main to `dffcd9a1`. Its expanded 504-execution matrix includes the
previous register/stack regressions and new private global/local-static controls.
Four re-emitted leaf-CALL modules match the previously executed native inputs.
The sections below retain the register/stack repair's preceding source cuts.


# Checked x86 assembly register and stack effects

The preceding 32-bit qualification selects `8f51b482`, which builds the complete 224-unit compiler
with zero GCC
warnings and reaches an exact emitted-C fixed point. Its source context includes
the concurrent `winnt.bi` edit. A second complete self-build verifies that context
and retains the same compiler image and all 224 native objects. A further
224-unit emission refresh verifies the next concurrent SDK edit with identical
native C. The complete normal backend suite passes, and all 703 fbctests
contexts retain GCC 0 / Clang 771. The compiler retains Clang 1,340.

## Native operand allocation

The previous emitter declared all six allocatable 32-bit general registers
clobbered in every ordinary assembly block. At `-O0`, position-independent
address operands could then require registers that the native compiler could
never allocate. Some PIC forms also omitted the implicit EBX write from CPUID.

The checked instruction path now describes its general-register effects through
an explicit instruction whitelist. Partial register names contribute their full
register, including AH, AX and EAX. CPUID, XGETBV and one-operand IMUL contribute
their implicit writes. Registers read in explicit operands also contribute their
roots conservatively. The native memory and condition-code contracts continue
to describe the entire joined assembly block.

This effect proof has its own whitelist, independently of procedure-name
reachability. A newly recognized procedure-reference opcode therefore requires
its register effects to be described before receiving the reduced clobber set.
Opaque instructions, procedure-address operands and indirect branches retain
the conservative clobber path. Checked branches stay within a joined template's
resolved local labels.

## Automatic storage during PUSH and POP

Native compilers may address automatic variables relative to ESP when frame
pointer omission is enabled. A PUSH inside the assembly template changes that
address before a following memory operand is evaluated.

Checked 32-bit blocks containing stack operations and automatic memory operands
now receive a live level-zero frame-address input before the template. This
establishes stable frame-relative operand addressing. The source regression
observes both a local variable and its parameter while the stack is displaced.
The native qualification explicitly requests `-fomit-frame-pointer` for these
cases and independently observes their returned values.

## Recorded native checks

The permanent regression is
`tests/compiler-support/c-asm-register-effects.bas`, registered in the normal
backend suite at ordinary and optimized settings. It checks partial-register
aliases, an implicit EDX write, private static addressing, and stack-local
access. The existing CPU-query and x87 control-word fixture supplies independent
CPU and floating-control observations.

Both actual compiler revisions emit the fixtures through the GCC and Clang C
producers at `-O0` and `-O2`, with ordinary and debug variants. Native GCC and
Clang compile and execute the emitted modules at both pointer widths.

| Native platform | Repaired executions | Observations | Original compile failures | Original execution failures |
| --- | ---: | ---: | ---: | ---: |
| Windows | 64 | 6,816 | 0 | 16 |
| Linux | 224 | 23,856 | 24 | 80 |

All repaired executions pass and all generated modules have zero GCC warnings
under the full unsuppressed diagnostic flags. Linux covers the native tool's
default mode, explicit PIE and explicit PIC, including FreeBASIC's own `-pic`
mode. Linking uses `-Wl,-z,text`. The earlier C-only pilot remains a separate
experimental receipt. The final matrix uses the actual compiler's emitted C.

Sixteen ordinary driver executions also pass across Win32 and Win64, both C
producers, ordinary/debug builds and `-O0`/`-O2`. Eight further comparisons emit
the existing opaque-opcode and separate-template regressions for 32-bit Linux
through both producers and PIC settings. Each retains the original conservative
clobber list and byte-identical C. Development main's default entry executes the
permanent regression successfully through both C producers.

## Reproduction and bounds

The complete retained replay closes 6,419 configured profiles at GCC 12 /
Clang 15,862, including 6,345 accepted C profiles and the recorded exclusions
and provider/refusal dispositions. Sixteen profiles affected by the concurrent
SDK edit are re-emitted in the same database, with their intermediate records
preserved. All current source hashes agree and there are no new frontend
regressions. Templari's complete 668-profile paired graph retains GCC 2 /
Clang 53,213. Both GitHub ports retain exact C across all 1,053 contexts, and
the four additional E-drive projects retain GCC 0 across 440 objects.
The later Templari source update receives a further 61-profile refresh with
the same totals. Exampleageddon retains three deliberate null-dereference
observations in each of its 1,775-profile checked and default configurations.
Later concurrent runtime edits are recorded separately from the last fully
qualified Linux MT runtime cut, GCC 147 / Clang 10,148.

Controllers and receipts are under
`build/c-emitter-forward-views-20261105`.

- `asm-pic-registers-selfhost-v2-20261115.py` owns the canonical self-build.
- `asm-pic-registers-packet-v2-20261115.py` records exact C from both compilers.
- `asm-pic-registers-worker-v2-20261115.py` runs the serial native matrix.
- `asm-pic-registers-entry-v2-20261115.py` checks ordinary driver builds.
- `asm-pic-registers-fallback-v1-20261115.py` checks opaque C identities.
- `asm-pic-registers-source-refresh-v1-20261115.py` verifies current compiler
  inputs through all 224 fixed-point C identities.
- `asm-pic-registers-selected-v2-20261115.json.zlib` records development-main
  selection, its public entry checks, protected products and the rollback image.
- `asm-pic-registers-native-{windows,linux}-v2-20261115.json.zlib` preserve raw
  commands, diagnostics, executable status and original controls.
- `asm-pic-registers-qualification-v2-20261115.json.zlib` binds the source cut,
  native observations, complete project queues and protected products.

Native scratch usage peaks at 152,799 bytes on Windows and 54,354 bytes on Linux.
Both scratch directories are removed after completion. Strict source-only lint
on the two compiler files and the regression reports zero errors and warnings.
Installed compiler and runtime products retain their separate ownership.

The retained replay's measured owned peak is 30,668,909 bytes within its
256 MiB archival bound. The 64-bit stack-manipulation/automatic-storage case
has its own follow-up check beyond this checked 32-bit repair.

## Current 64-bit stack qualification

Development main selects `bc2e5878`, extending the stack qualification to x86-64. The previous
selected compiler, `8f51b482`, fails 48 execution controls in the expanded
matrix. PUSH can overwrite automatic storage or native spills below RSP.
Disabling the red zone protects both explicit automatic operands and spills
whose source assembly names no BASIC variable.

Ordinary x86-64 inline assembly now requests `-mno-red-zone` for its own C
module. The driver copies that requirement before releasing the backend
context. Plain modules retain their native defaults in either source order.
A module-level comment publishes the same requirement for standalone emitted-C
consumers. The native test helper reads that exact comment with bounded input
storage. File-scope naked procedures retain their existing ownership.

A frame-address input alone exposes another fault in leaf assembly CALLs.
The corrected emitter reserves a 16-byte aligned local object and retains it
through a memory output, with a memory-only frame-address input. This establishes
the native call alignment and stable automatic addressing. The constraints need
no general-register setup between source assembly fragments. The output-only
object has no uninitialized C read. The existing checked 32-bit frame treatment
remains qualified by the expanded matrix.

The permanent `c-asm-register-effects.bas` regression now exercises 64-bit PUSH,
opaque spill behavior and a BASIC-label assembly branch. The existing
`c-asm-real-addresses` BASIC/C pair adds a leaf CALL with an independent naked
callee that records its entry stack alignment. Its Windows caller reserves
the ABI shadow area. Keeping the leaf free of automatic locals and ordinary
calls makes the frame-alignment requirement observable.

The actual candidate completes a 224-unit emitted-C fixed point with zero GCC
warnings. Its Windows/Linux native matrix passes 288 executions and 49,968 value
observations, and sixteen ordinary driver executions pass. All generated matrix
modules have zero GCC warnings. Clang retains the same sixteen startup
registration observations as the preceding compiler. Sixteen additional Linux
CALL-alignment observations pass with zero native warnings. Early C-only pilots
and the rejected frame-only compiler have separate receipts.

The complete backend suite passes with an externally observed leaf CALL that
remains observable under native inlining. All 703 fbctests contexts have GCC 0 /
Clang 771, and the compiler has GCC 0 / Clang 1,340. Eight opaque 32-bit controls
retain exact C. Both ordinary development-main entries execute the register
regression successfully. The immutable `8f51b482` image supplies rollback.

The complete retained replay has 6,419 profiles at GCC 12 / Clang 15,862.
Templari has 668 paired profiles at GCC 2 / Clang 53,213 after four profiles
refresh a concurrently changed header. All 1,053 Linux port profiles close at
coreutils GCC 168 / Clang 1,803 and SQLite GCC 21 / Clang 7,696. Twenty-five
changed coreutils modules receive paired native builds with their original
diagnostics. The four other E-drive projects complete 440 objects at GCC 0.
The fresh Linux MT runtime cut covers 1,328 current inputs at GCC 147 /
Clang 10,150, including thirty concurrent source updates and twelve passing
public API executions. Windows-only runtime inputs retain their platform
qualification.

Controllers and receipts use the `asm-stack64` prefix under the same audit
root. The native ASM matrix owns under 153 KB of temporary files per platform.
The current runtime rebuild peaks at 14,907,884 bytes and removes its scratch
directory after both native builds. The retained replay records its own bound.
Strict source-only lint covers eight changed BASIC/compiler files with zero
errors and one advisory for the regression's required PRINT result marker.
Compiler-backed lint also reports unavailable input-closure/type-hint facts.

A separate pre-existing naked-procedure storage fault remains open. File-scope
assembly can write a private BASIC global that the native optimizer removes or
constant-folds because the write is absent from its C effects. The original
compiler reproduces the missing-storage link failure. The CALL-alignment probe
uses an independently provided external observer object. Native storage
retention and private-global read/write visibility need a separate repair
and execution controls.

GCC documents the red-zone effect of inline PUSH and CALL and requires a
restored stack pointer at assembly exit.
[GNU GCC extended assembly](https://gcc.gnu.org/onlinedocs/gcc-15.2.0/gcc/Extended-Asm.html)

<!-- end of c-emitter-asm-register-effects.md -->
