<!--
Project: FreeBASIC C backend verification
File: c-emitter-pointer-values.md
Purpose: Explain native pointer-value arithmetic and its C qualification.
Responsibilities: Address formation, read-only views, diagnostic receipts and limits.
This file intentionally does NOT claim bounds correctness for arbitrary pointer accesses.
-->

# Native pointer values in emitted C

BASIC pointer arithmetic forms native address values. An address can be null,
outside an allocation or wrapped at the native word size without being read or
written. The ordinary C pointer ADD/SUB path imposed a different rule: forming
such a value could already be undefined, even when the program never accessed
memory through it.

The regression reproduces that difference directly. The accepted compiler's
GAS64 and ordinary C executions return the expected address values, while
Clang's trapping pointer-overflow check stops the same program. The corrected
C path converts the pointer and scaled displacement to native unsigned values,
performs ADD/SUB, and forms the result pointer afterwards. Each operand remains
present once. Pointed-to types and qualifiers remain on the final pointer.

This extends the already-qualified virtual array-origin representation to
ordinary pointer values. It does not change pointer differences, source
bounds checks or the memory operations that use the result. Supported target
contracts use the compiler's native 32- or 64-bit address representation.
Foreign object qualification does not establish foreign runtime execution.

Read-only byte views also follow those native integer address carriers. The
walk is limited to native-width casts and ADD/SUB nodes with a bounded depth;
other scalar calculations and opaque values do not establish a pointer path.
This retains the source address's qualifiers through scalar byte-copy reads.
The first private revision exposed four const-discard diagnostics in two
compiler modules. The revised helper removes that regression without weakening
the C diagnostic profile or changing BASIC parameter types.

## Qualification

The canonical compiler passes all 224 compiler-unit C fixed-point checks with
zero GCC and 2,529 Clang warnings. The preceding startup checkpoint had zero
GCC and 4,755 Clang warnings. The complete private compiler also passes the
existing GCC/Clang backend suite, including native ABI, runtime aliases,
read-only entries, wide calls, virtual array origins and destructor controls.

The permanent `tests/compiler-support/c-pointer-values.py` regression runs
330 cases under both C producers, GCC/Clang, GNU11/GNU23, `-O0`/`-O2` and
checked/unchecked BASIC modes. Its 48 native runs include 16 with the trapping
pointer check. Two GAS64 controls independently verify the reference results;
two historical C controls reproduce the trap. Cases cover one-, three- and
eight-byte elements, pointer slots, const views, native minimum/maximum offsets,
in-place updates and exactly one evaluation of both operand calls. Actual
loads and stores stay within their declared byte and record arrays.

Sixteen Win32 objects and ten other foreign objects qualify emitted types and
address widths. They are compilation evidence, not foreign execution evidence.
The canonical provider also executes all 137 real union-initializer fbcunit
cases through the privately built framework, preserving the installed archive.
Eight semantic tests cover pointer origins, live/alias types, selected C names,
wrapper facts, raw ABIs, reduced modes and module reset.

The initial three-project admission removes 3,779 of 4,343 Clang buffer
observations in Scorched Earth graphics, VNC UI and Stunts data. GCC warning
counts remain unchanged in those profiles. The full selected queue refresh
provides the accepted counts; this sample is not a project runtime claim.

Receipts are under `build/c-emitter-pointer-values-20261103/`:

- `canonical/receipt.json` and `canonical/results.sqlite3`
- `contracts-canonical/receipt.json` and `canonical-gates.json`
- `qualification-r2.json` and its retained backend-suite output
- `audit/receipt.json`, `audit/results.sqlite3` and `summary.json`
- `canonical-source-edits.json` and exact source preimages
- verified archive journals for the closed private compiler revisions

One broad-audit worker uses one reusable C/object pair under a 64 MiB stage cap.
No warning flag is disabled. Original memory accesses can still require source
bounds and lifetime contracts; a smaller warning count is not proof that those
accesses are valid. Installed compiler/runtime binaries remain unchanged, and
PNG remains excluded. The [current status](c-emitter-current-status.md) gives
the accepted queue totals and the [class priorities](c-emitter-class-priorities.md)
retain the outstanding control-flow and aggregate-layout work.

<!-- end of c-emitter-pointer-values.md -->
