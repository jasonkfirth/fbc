<!--
Project FreeBASIC C backend qualification
File c-emitter-noinline.md
Record native procedure-policy preservation and allocator regression evidence.
Application publication and the broader warning campaign retain their own scope.
-->

# Native procedure policy and allocator returns

Gnulib's original `readlink_stk` explicitly carries `__noinline__` to avoid
GCC PR93644. C2FB discarded that policy while translating the correct
stack-buffer copy and allocation paths. The original Coreutils v9.12
submodule commit is `106e9b2384d08a1696fcbd40cbab52237943f208`.
The source explains the GCC 10/11 and later LTO behavior. [Original helper](https://github.com/coreutils/gnulib/blob/106e9b2384d08a1696fcbd40cbab52237943f208/lib/careadlinkat.c)

FreeBASIC accepts contextual `NoInline` after a complete procedure signature.
Declarations and definitions merge this policy. Ordinary identifiers remain
available, and procedure-pointer types retain their existing ABI identity.
The C emitters place the native attribute on actual declarations and bodies.
Declaration probes include it when comparing existing prototypes, avoiding a
duplicate declaration exposed by the member-procedure regression.
The manual documents the syntax in `KeyPgNoInline.wakka` and the procedure
declaration, function, subroutine and category pages.

C2FB recognizes GNU and Microsoft declaration attributes at their own list
depth. It carries the policy through scanned declarations, merged symbols,
body bindings and generated headers. Microsoft attribute lists accept
whitespace-separated modifiers. [Microsoft declaration grammar](https://learn.microsoft.com/en-us/cpp/cpp/declspec?view=msvc-170)

Parameter attributes, strings, nested
attribute arguments and ordinary identifiers remain distinct inputs. Function
typedef declarations retain the procedure policy while their pointer types
keep their existing ABI. Fourteen normal-entry converter functions and four
generated-header declarations pass. Both C backends also compile the generated
function and function-typedef fixtures.
The complete converter remains GCC-clean. Its Windows Clang build records
159 padding observations and two startup observations, including one padding
observation from the extended model.

The generated `careadlinkat` owner changes only two procedure headers.
Fresh GCC and Clang producer emissions agree byte for byte. Linux GCC 14.2
and Clang 19.1.7 execute the complete original and translated modules at O0
and O2. Their 38,408 comparison pairs include 28,800 allocator-contract cases,
9,600 deliberately returning `die` callback controls and eight synthetic
size-limit controls. Checks cover
caller buffers, stack-buffer copying, growth, optional shrinking, allocation
failures, read failures, errno restoration, allocation accounting, guard
bytes, returned data and the size-limit branch.

The original GCC O2 module reports one `-Wreturn-local-addr` observation.
The repaired module reports zero GCC warnings at both optimization levels.
Clang retains one reserved-identifier observation at each level. The native
allocator worker peaks below 100 KB of owned scratch storage and removes
its temporary directory.

Templari's `chat_shell.bas` and `chat_shell.bi` changed after the broad replay.
The follow-up refresh covers 51 affected contexts through both C producers,
reusing native diagnostics for byte-identical C. Its complete 677-profile
summary retains GCC 2 / Clang 54,317 and records the new source snapshots.

Native Windows procedure tests pass 2,275 assertions across GCC/Clang O0/O2
and GAS64. All four C compilations are warning-free under unsuppressed
diagnostics. The compiler reaches a 224-unit emitted-C self-host fixed point
with zero GCC warnings. Strict source-only lint reports zero errors with the
existing advisory count preserved. The new BASIC fixture is
lint-clean.

The allocator execution harness initially used the Windows ABI for a Linux
profile whose C `long` is 64 bits. Its completed execution evidence comes
from the Linux worker. The Windows evidence covers native Win64 procedure
tests. The module's full original C body and allocator header are pinned in
the receipt, with small qualification declarations supplying platform macros.

## Reproduction

`build_scripts/test-careadlinkat-allocator.py` compares a supplied emitted C
module with its reference C using the canonical allocator header, explicit
native flags and one optimization level. The normal compiler backend test
runner includes `tests/compiler-support/c-procedure-noinline.bas`.
C2FB's Make target includes `tests/test_noinline.py`.
Regenerated `NoInline` headers use the source-pinned development compiler
that implements the modifier.

Receipts and the checked port patch live under
`build/c-emitter-forward-views-20261105`.

- `noinline-selfhost-v2-20261117.json.zlib`
- `noinline-build-v2-20261117.json.zlib`
- `noinline-owners-v3-20261117.json.zlib`
- `noinline-native-v1-20261117.json.zlib`
- `noinline-linux-v1-20261117.json.zlib`
- `noinline-header-v1-20261117.json.zlib`
- `noinline-lint-v2-20261117.json.zlib`
- `noinline-suite-v1-20261117.json.zlib`
- `noinline-queue-replay-v1-20261117.json.zlib`
- `noinline-templari-refresh-v1-20261117.json.zlib`
- `noinline-port-v2-20261117.patch`
- `noinline-qualification-v1-20261117.json.zlib`

The checked port patch preserves its preceding repair cut. Whole-port
publication, broader converter integration, remaining GCC owners, Clang
cleanup and outstanding platform checks continue under the existing goal.

<!-- end of c-emitter-noinline.md -->
