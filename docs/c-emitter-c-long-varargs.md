<!--
Project: FreeBASIC C backend qualification
File: c-emitter-c-long-varargs.md
Purpose: Record explicit C-long mangling and narrow variadic-anchor repairs.
Responsibilities: Explain native type contracts, diagnostics and ABI evidence.
Compiler selection and complete corpus coverage belong to current status.
-->

# C-long aliases and narrow variadic anchors

## Explicit C-long type codes

`inc/crt/long.bi` defines CLong and CULong using the target's C width and explicit
mangling aliases. C++ Itanium names encode these types as `l` and `m`. On 32-bit
targets the mangler could remap an explicit alias after its early Integer case,
then read an empty type-code entry. The resulting symbol omitted the argument's
type code and failed to link to the native provider.

The shared type-code table now supplies `l` and `m` for these remapped entries.
The existing Windows 64-bit vendor INTEGER handling runs before the table.
The regression supplies independently spelled native symbols and checks signed,
unsigned and target-width values. Both C producers emit the expected names for
Windows and Linux at 32 and 64 bits. The original compiler reproduces the Win32
link failure. Native values pass with the corrected compiler.
The shared mangler also passes actual Win32 GAS and Win64 GAS64 executions
against the independent C provider.

This defect was exposed by correcting fb-ext-lib's JPEG skip-input callback
parameter to CLong, as required by `jpeglib.bi`. The library correction removes
one strict function-cast observation. Sixteen before/after native object builds
preserve normalized symbols and complete instructions. The helper's parameter
type changes its private mangled spelling, which is normalized explicitly in
that comparison. Qualification compiles the callback module and compares code.

## Variadic argument initialization

Before C23, native va_start contracts restrict the last named argument's type.
Clang diagnoses byte, short and float anchors because default argument
promotions change their types. Changing the function's formal parameter type
would also change its ABI.

For narrow integer and Single anchors, generated C now uses a module-local
macro. C23 implementations exposing `__builtin_c23_va_start` initialize the
argument list through that builtin. An unevaluated `sizeof` acknowledges the
source anchor. Other dialects and compilers retain the existing two-argument
builtin and its diagnostics. Ordinary anchors retain their existing output.
The helper participates in the emitted-procedure dependency graph, so unused
private callers also retire their macro.

The regression retains signed byte, short and float C prototypes and calls
the BASIC procedures from both BASIC and an independently compiled C observer.
Its two C producers, two native tools, Windows widths, GNU11/GNU23 and O0/O2
matrix supplies 64 actual value executions across both repairs. Each observer
uses the other native compiler. Eight further executions check dead-helper
pruning. All 32 before/after variadic disassemblies are identical. Peak owned
scratch is 78,617 bytes.

Eight further assembly comparisons cover both C producers on Linux x86,
x86-64, ARMv7 and AArch64. Complete optimized Clang assembly matches its
preceding compiler for every case. These foreign checks produce assembly.

The C23 path replaces Clang's varargs observations with pre-C23 compatibility
observations under `-Weverything`. The total diagnostic count is preserved.
GNU11 retains its preceding diagnostics. The public backend suite probes the
native dialect and builtin, then requires `-Werror=varargs` when supported.
It also retains unused-function and unused-macro checks for the dead helper.

## Qualification

The canonical compiler reaches an exact 224-unit self-host fixed point with
zero GCC warnings. The complete updated GCC/Clang suite passes in 310.362
seconds. All 703 fbctests profiles retain zero GCC warnings and 771 Clang
observations, including the two retained C23 compatibility advisories. Twenty
additional values pass through the selected development path. Full receipts are tracked in
[current status](c-emitter-current-status.md). Strict lexical lint accepts both
BASIC fixtures and both changed compiler files. Earlier strict invocations
reported zero findings. A later required-semantic control exposes the installed
linter's rejection of the compiler's newer diagnostic capability records, so
current compiler-backed lint remains unverified. The callback module retains
one pre-existing presentation-whitespace
lint finding. Its compiler-backed lint reaches the native lookup-event limit,
so that semantic check remains unresolved.

Evidence is retained in the existing `build/c-emitter-forward-views-20261105`
work area. The current build and focused receipts are
`c-abi-selfhost-v1-20261113.json.zlib` and
`c-abi-values-v3-20261113.json.zlib`. The first value attempt identified a missing
CRT include in the new fixture. That include was corrected before the completed
matrix. The first full-suite attempt was stopped at the same boundary and
receives no pass credit. A later test correction selects the wide-value branch
at preprocessing time, avoiding constant-overflow warnings on 32-bit targets.
The completed third value matrix also requires zero BASIC warnings.

<!-- end of c-emitter-c-long-varargs.md -->
