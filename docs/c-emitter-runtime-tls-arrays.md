<!--
Project: FreeBASIC C backend verification
File: c-emitter-runtime-tls-arrays.md
Purpose: Record runtime-backed array indexing and terminating C epilogues.
Responsibilities: Explain storage ownership, regression coverage and remaining limits.
This file intentionally does NOT claim every TLS or foreign-provider combination.
-->

# Runtime-backed fixed arrays and C epilogues

A fixed TLS array can have a computed storage address. The variable parser
previously passed that expression to an IDX node, whose loader requires a
plain variable and its symbol. A valid DOS C-backend consumer consequently
crashed during compilation. GAS64 exposed the same index-node assumption.

The parser now applies the fixed array's existing byte bias to the computed
storage and constructs a dereference. Ordinary variables keep their IDX path.
Index expressions and bounds checks are retained, and the storage expression
is evaluated once. No example is rewritten or excluded to avoid the crash.

The accompanying DOS work uses runtime storage instead of unsupported native
C TLS declarations. Public externs retain their name-based lookup and the
provider's initializer template. Private DOS objects use their static template
addresses as keys, so identical private names in different modules remain
separate. Those keys have static lifetime and the runtime does not free them.
The new runtime entry is additive; rebuilding the DOS runtime is required for
clients which use it. Installed release archives remain unchanged.

## Terminating bodies

The C backend omits a synthetic result return after a procedure whose settled
flow cannot return. Ordinary procedures retain their noreturn attribute.
C's entry point uses its ordinary signature: older DJGPP GCC inserts an
implicit return zero in main and diagnoses that return even when its body
ends in a terminating call. A standalone native C control reproduces this
behavior without FreeBASIC. Diagnostic flags remain enabled.

The small terminating-result regression retains the existing FBC warning 13
for a function whose result was not explicitly assigned. That source policy
is separate from the compiler-generated C epilogue.

## Qualification

The selected development compiler is
`9e800d585c799c061f42beafa0fc35f2134762c15c5c15f39cf24cceeda610b7`.
It reaches an exact 224-unit GCC-clean fixed point. The array matrix checks
eight native C configurations, GAS64 at O0/O2, and two actual DOS executables.
All host cases also reject the invalid index with runtime error 6. The DOS
cases cover nonzero multidimensional bounds, worker isolation, source index
evaluation, and first-touch initialization across separately compiled modules.
Their generated C compiles with zero warnings under the original aggressive
GCC flags. The older DJGPP compiler uses its supported GNU2x spelling.

The native Clang controls at both pointer widths retain their existing GNU
identifier warnings.
These remain visible and separate from the repaired array path. No whole-queue
warning reduction is inferred from this fixture.

`tls-arrays-values-v6-20261113.json.zlib` records all twelve configurations,
twenty-two positive/rejection executions and both preceding-compiler crash
controls. It also records the copied DOS library hashes, actual guest executable
hashes and receipts. The single disposable directory stays below 24 MiB;
release executables and archives are preserved.

The complete GCC/Clang backend gate passes. All 703 retained fbctests profiles
are re-emitted: 702 retain exact C identities and one requires fresh native
compilation. Their original unsuppressed GCC results remain zero, with source
warning lines unchanged. The replay is recorded in
`noreturn-fbctests-v2-20261113.sqlite3`; it does not execute all test programs.
See [current status](c-emitter-current-status.md) for the selected compiler and
last completed census. Full foreign-provider and descriptor qualification
remain outside this focused regression.

<!-- end of c-emitter-runtime-tls-arrays.md -->
