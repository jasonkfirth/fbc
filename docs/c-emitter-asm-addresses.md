<!--
Project: FreeBASIC C backend qualification
File: c-emitter-asm-addresses.md
Purpose: Explain Intel assembly operand storage and its current qualification.
Responsibilities: Describe address inputs, parameter homes and native checks.
This file intentionally does NOT claim full foreign-target or corpus acceptance.
-->

# Intel assembly storage addresses

The Win32 `RotoZoomASM` body in fb-ext-lib names 24 distinct variables. The
previous emitter assigned each a read/write memory operand. GCC counts each
such operand twice, exceeding its 30-operand limit before producing an object.
The BASIC implementation and its arithmetic do not need to change.

The compiler change and both regression sources live in the ordinary source
tree. A fresh compiler/runtime build can run the maintained gate directly:

```sh
python -B build_scripts/test-compiler-backends.py --fbc bin/fbc.exe \
  --toolchain-prefix . --backend gcc --backend clang
```

Substitute the newly built compiler and matching prefix when using a separate
build directory or installed layout. The ignored audit receipts below retain
the local corpus history; they are not required to run this public regression.

## Storage and ABI ownership

`backend/c/c-asm-addresses.bi` owns the per-procedure list of assembly-visible
scalar parameter homes. Its symbols are borrowed until procedure emission
finishes, and its list nodes are released before those symbols expire. The
existing reachable-storage walk collects them, so unreachable assembly cannot
create a new unused C variable. This compiler state is used serially within
one emitter process; separate compiler processes own separate lists.

Private non-TLS static variables use unchanged address inputs, with the
existing memory clobber describing writes through those addresses. Automatic
variables retain real read/write memory operands. Replacing every operand
with an address input is insufficient: unoptimized GCC needs registers to
form automatic addresses, while the assembly clobbers those registers.
Automatic operands precede static address inputs, and both lists are
deduplicated before template and label indices are rendered. RotoZoom's
21 static variables and three automatic variables consequently count as 27
GCC operands. The regression reaches the limit with 24 statics and three
automatic operands.

GCC and Clang print address inputs differently in Intel templates. The emitter
writes two preprocessor-selected templates with identical constraints. Only
the GCC template adds its address modifier to generated numeric placeholders.
Literal percent signs, labels and unique local-label suffixes keep their
existing handling.

An assembly-visible scalar parameter gets one local source home on procedure
entry. The native signature retains its ABI and passes its argument under a
reserved C identifier. All BASIC references, including escaped addresses,
refer to the same local home throughout the call. There is no temporary
copy around an assembly statement and no copyback afterward. This avoids
GCC's incoming-stack argument pointer when Win32 stack realignment coincides
with assembly that clobbers the general registers.

Floating homes copy their object representations through volatile unsigned
bytes with fixed array bounds and constant offsets. Ordinary C assignment
can quiet a signaling NaN through x87, and optimized `memcpy` can become the
same assignment. The byte accesses prevent floating evaluation during this
entry transfer. Their volatile qualification belongs to the transfer, not to
the BASIC variable. Integer values and pointer carriers keep ordinary entry
assignment. Existing byte-backed CLongDouble transport remains separate.

The last named variadic argument retains its native storage so that
`__builtin_va_start` still names the actual fixed argument. Managed and
aggregate parameters, naked functions, other assembly syntax and other CPU
families retain their existing contracts. This repair does not remove GCC's
operand limit for arbitrary assembly, infer instruction effects, or repair
the existing input-only automatic operands in `asm goto`.

## Qualification and remaining warnings

The selected development compiler is the qualified assembly-address build
listed in [current status](c-emitter-current-status.md). Its complete recount
preserves GCC totals and every original source warning. The retained Clang
total increases by one pre-C99 declaration-placement advisory in fbsound;
the generated parameter prologue remains valid C99 and later. This advisory
is still counted, not suppressed or converted into a BASIC source warning.

The original Win32 failure is retained in
`build/c-emitter-forward-views-20261105/asm-operand-inspect-20261113.json.zlib`.
The v5 candidate,
`c3f2b1e625e7fde905a22808944ca5c6a23f73bb4a096d8ee17ddfdd79fba846`,
reaches an exact GCC-clean 224-unit fixed point. The complete GCC/Clang backend
suite passes in 459.380 seconds without a whole-suite deadline, recorded in
`asm-address-gates-v5-20261113.json.zlib`. The final runner, including the
mandatory-PIC target selection, passes again in 279.607 seconds in
`asm-address-gates-v6-20261113.json.zlib`. The ordinary source graph also passes
for 411 documented files and 21 host policies. Strict lint reports no errors
or warnings in the two compiler files and the BASIC regression.

`asm-address-values-v7-20261113.json.zlib` records 32 executed builds from the
actual v5 compiler: Win32/Win64, both C emitters, GCC/Clang,
GNU C11/C23 and O0/O2. Generated C and the native helper produce zero warnings
under the original unsuppressed diagnostic flags. The checks cover persistent
static updates, live ByRef aliases, unchanged caller ByVal arguments,
unreachable assembly, and exact Single/Double signaling-NaN, quiet-NaN,
negative-zero and finite bit patterns. A C observer runs inside the BASIC
assembly block to check its published parameter address before return.
Both old-compiler GCC controls fail at the operand limit as expected.
The earlier native prototypes remain separate receipts. Eight semantic-model
checks validate both transports at both widths through both emitters, with
byte-identical C whether observation is enabled or disabled, in
`asm-address-semantic-v1-20261113.json.zlib`.

`asm-address-modules-v5-20261113.json.zlib` records 16 successful object builds
of the original blur, rotate, rotozoom and scale2x modules with GCC/Clang at
both widths. Win32 RotoZoomASM retains the exports of the original valid Clang
object. All eight Win64 comparisons retain exact exports and instructions.
These are object checks; no transform, image loader or display is executed,
and no library archive is replaced.

Foreign Clang objects cover Linux at both Intel widths and FreeBSD x86-64,
with PIC/non-PIC and O0/O2. Eleven cases compile without warnings. The twelfth,
i386 PIC at O0, reports the same three register-allocation errors with the
old pointer-slot compiler and the candidate. The small static-only PIC
control passes at both optimization levels with both compiler versions.
These results are retained in `asm-address-foreign-v2-20261113.json.zlib`,
`asm-address-foreign-baseline-v1-20261113.json.zlib` and
`asm-address-pic-baseline-v1-20261113.json.zlib`. They are object checks,
not foreign executions. On Haiku/OpenBSD/Android i386, where PIC is mandatory,
the new runner omits only this known unsupported O0 fixture and still requires
its optimized counterpart. Existing backend fixtures remain required.

The newly accepted Win32 RotoZoom module still has 61 unused-function and
four unused-but-set-variable warnings under each native compiler. General
assembly currently keeps the procedure and storage analyses opaque. These
observations remain visible pending a separate reference-analysis repair;
they are not part of the earlier Win64 expansion's zero-GCC count. Four
Win32 modules also retain one Clang structure-padding observation each.

Owned scratch reuses the existing audit directory. Self-host scratch is
capped at 32 MiB and peaks at 10,675,032 bytes; scalar execution uses one
disposable directory, peaking at 170,131 bytes below its 16 MiB cap. Installed
release compilers and runtime archives remain unchanged.
`asm-address-source-cut-v1-20261113.json.zlib` retains the base Git revision,
build flags and exact bytes of all four uncommitted pinned build inputs,
including the independent DOS input context. The complete retained census and
three separate library expansions are closed independently in
`asm-address-cohort-close-v1-20261113.json.zlib`, preserving all 7,319 source
diagnostic lines. Selection and eighteen selected-path value executions are
recorded in `asm-address-selected-publish-v2-20261113.json.zlib`; the existing
pointer-slot image provides rollback without another backup tree.
The observer's isolated DJGPP prefix amendment is retained in
`asm-address-helper-prefix-amendment-v1-20261113.json.zlib`: its old bytes are
reconstructed exactly, all four native Windows preprocessor outputs are
identical, and its DOS-prefixed external calls receive an object check.

<!-- end of c-emitter-asm-addresses.md -->
