<!--
Project: FreeBASIC C backend verification
File: c-emitter-census-reproducibility.md
Purpose: Explain retained corpus contexts and removal of an obsolete include path.
Responsibilities: Record source seals, exact native reuse and outstanding qualification.
This file intentionally does NOT publish partial counts as a completed census.
-->

# Canonical retained-corpus contexts

Development main now selects `6634540c` through the
[byte-memory qualification](c-emitter-byte-memory.md). The 6,419-profile refresh
uses streamed parent rows, exact-C native reuse and fresh native checks for
changed C under the original diagnostics. It completes all profiles with GCC 12 /
Clang 15,862, no stale inputs, no native failures and no new frontend regressions.
Its 132,636,482-byte owned peak remains within 256 MiB. The following
completed counts retain their earlier compiler and input identities.

The selected compiler is `987d6d9d`. Its completed 6,419-profile retained recount
is `c-abi-retained-v1-20261113.sqlite3` in the existing work area, reporting
12 GCC and 15,785 Clang observations. All 224 compiler C identities match its
self-host fixed point. The separate 703-profile fbctests check has zero GCC
warnings. Current counts and expanded queues are recorded in
[current status](c-emitter-current-status.md).

Thirty-two Optical profiles now have actual current build contexts and 64 fresh
native object checks. They add zero GCC and 76 Clang observations. The completed
retained view covers the same 6,419 configured profiles, now with 6,345 accepted
C profiles at 12 GCC and 15,861 Clang observations. Its composed receipt is
`c-abi-retained-optical-view-v1-20261114.json.zlib`; both parent databases retain
their original bytes.

The subsequent external sorting changes affect two SDK header inventory entries
in the original selection receipt. A fresh reconciliation verifies all 11,744
selection inputs and the current retained source seal. The actual include
closures of all 224 compiler units and 703 fbctests profiles contain neither
changed header. The compiler image and original selection receipt preserve their
identities. Installed release executables and recorded runtime archives also
match. This proof is `c-abi-selected-library-reconciliation-v1-20261114.json.zlib`.
The affected library consumers have their own fresh native checks.

The later provider repair changes one further inventory-only implementation
entry. `provider-inventory-reconciliation-v1-20261114.json.zlib` verifies the
current selection inputs, zero affected readers in 6,531 accepted parent
profiles, fresh provider objects and preserved products. Full accounting of all
353 library BASIC files is recorded in
[provider qualification](c-emitter-library-providers.md).

The final 194-profile library expansion completes at zero GCC and 156 Clang
observations. The eight runner-context fragments described below also compile
with zero GCC and nine Clang padding observations, recorded in
`fbext-fragment-contexts-v1-20261114.json.zlib`. These supplemental checks stop at
native objects. Original standalone refusals retain their diagnostic records.

## Historical recount context

The running retained census uses the immutable compiler
`9e800d585c799c061f42beafa0fc35f2134762c15c5c15f39cf24cceeda610b7`.
Its 6,419-profile retained recount is recorded in
`build/c-emitter-forward-views-20261105/noreturn-retained-v3-20261113.sqlite3`.
The recount remains incomplete until the database contains its successful
`summary` record. Complete preceding-compiler counts remain in
[current status](c-emitter-current-status.md).
The selected ASM/library-name compiler is newer; the two scopes must not be
reported as the same source or compiler cut.

All 224 compiler profiles completed before the next compiler repair began.
Their exact input bytes and completed-reader inventory are retained in
`private-asm-baseline-source-cut-v1-20261113.json.zlib`. Later compiler edits
belong to the next candidate. The older census continues through its immutable
executable, and include tracing rejects a new read of a changed compiler input.
Its final seal must verify this source-cut provenance and requalify any
independently changed test input; a partial count is not a successful seal.

Include tracing found that 699 retained fbctests commands still selected
`fbcunit/inc` through an old private build directory. The header has the same
SHA256 as the checkout's `tests/fbcunit/inc/fbcunit.bi`:
`f6f9d73faef89f27c51dcd4a90be6ed1884393401748c77d3aa30368707db9b5`.
The recount replaces that argument with the canonical checkout directory.
It retains each test's source, options and working directory. The old directory
is no longer needed to run these rebased commands. The recorded include
comparison is `noreturn-canonical-fbcunit-includes-v1-20261113.json.zlib`.

The include trace also seals a transitive interface declaration omitted from
the old inventory. Loader implementations and excluded image programs remain
outside the qualification. The recount only emits and compiles source; it does
not execute corpus applications.

Each profile keeps the original C output basename because module startup
identifiers depend on it. Native results are reused only when the emitted C
is byte-identical, compiler identities and diagnostic options match, and no
native include or path/date-sensitive macro invalidates that comparison.
Changed C receives fresh native compilation with the original unsuppressed
GCC and Clang options. BASIC diagnostics and previous refusal results are also
checked. The two pre-existing assembler boundaries retain their recorded
compile modes and remain separately identified.

The controller checkpoints completed profiles and uses one disposable source
directory. Its database, emitted source, object and logs share a 64 MiB bound;
completed intermediates are retired. A stopped controller is resumed only
after its process is confirmed terminal and its source/tool identity agrees.
The five-profile and 543-profile terminal checkpoints are retained as explicit
inputs to the resumed recount.

## Reproducing the compiler checks

The source changes and fixtures are in the repository. With a compiler built
from that source and a matching runtime prefix, the public backend check is:

```text
python3 -B build_scripts/test-compiler-backends.py --fbc <current-fbc> --toolchain-prefix <runtime-prefix> --backend gcc --backend clang
```

The complete external-corpus count additionally requires the external projects,
their source revisions and recorded build contexts. The local retained plan
does not make those projects part of a clean fbc checkout. The public fixture
results and the machine-specific corpus totals therefore have distinct
reproduction requirements.

The eight retained fb-ext-lib regression refusals also require their supplied
test runners. The JSON runner includes the production implementation before
its contract test. Shared-ownership and socket tests expose private fields in
isolated test headers, leaving the production API unchanged. UDP tests require
the production module include directory. Compiling those test fragments alone
does not reproduce their intended context.

## Startup-registration lifecycle checks

Replacing MinGW constructor attributes with explicit `.ctors` and `.dtors`
pointer tables is still experimental. The ordinary native pilot completed
32 value checks across GCC/Clang, Windows widths, both object link orders and
returning/terminating constructors. Its proposed tables matched the original
callback traces and removed the Clang startup warnings in that pilot.

The LTO follow-up rejects that implementation. It completed another 32 value
checks with function/data sections and linker section collection. Four GCC
comparisons changed module order. Eight Clang comparisons omitted priority
callbacks, including the terminating constructor, and produced no final-report
callback output. Default-priority ordering also changed. The native tools
accepted the tables, but warning-free compilation did not preserve lifecycle
behavior. No compiler registration change
has been applied or selected on the strength of these pilots. The remaining
startup-warning class is open until a replacement passes these lifecycle checks.

The local evidence is `startup-record-pilot-v1-20261113.json.zlib`, its output
seal, and `startup-record-lto-v3-20261113.json.zlib`. The LTO receipt carries
the sixteen completed GCC checks from the preceding checkpoint instead of
repeating them. Clang LTO uses LLD explicitly; passing only an absolute linker
path did not identify an LTO-capable linker to this Clang driver.

## Unnamed bit-field padding pilot

A separate C layout pilot tests ordinary padding, zero-width integer bit-fields
and unnamed byte-sized bit-fields at four internal/trailing gaps. Both Windows
widths and both native tools preserve the checked sizes, alignments, offsets
and positional initializers. Zero-width fields retain the padding warnings.
Byte-sized fields remove them in these four layouts.

The follow-up completes 64 actual Windows executions across both native tools,
both widths, ordinary/LTO links, both link orders and all four combinations of
ordinary/explicit caller and callee declarations. Values agree, but eight mixed
GCC LTO links introduce `-Wlto-type-mismatch`. All four record interfaces are
affected. Separate Clang x86_64 Linux assembly probes have identical complete
output for these four functions. Those foreign probes cover generated calling
sequences only.

The byte-field representation therefore remains an experiment. Compatibility
with existing C declarations still requires a solution to those GCC LTO type
diagnostics. The native diagnostic options remain unsuppressed. The C fixture's
four exact floating comparisons retain their own Clang `-Wfloat-equal` warnings.

Receipts are `padding-bitfields-pilot-v1-20261113.json.zlib` and
`padding-bitfields-abi-v2-20261113.json.zlib`. The latter carries 39 completed
executions from the preceding checkpoint and adds 25. Its disposable products
peak at 98,128 bytes. Compiler sources retain their selected representation.

<!-- end of c-emitter-census-reproducibility.md -->
