<!--
Project: FreeBASIC native callback qualification
File: c-emitter-thread-callbacks.md
Purpose: Explain static typed thread adapters and address-narrowing controls.
Responsibilities: Record the callback ABI, retained source conversions and evidence.
This file does not claim complete corpus qualification or installed promotion.
-->

# Static typed thread workers

`ThreadCreate` calls a worker through the runtime's `void (*)(void *)` contract.
BASIC examples also use static workers whose physical parameter is a typed
object pointer. Casting the procedure address to the runtime signature leaves
the generated C calling a function through an incompatible function type.

The C backend now emits a stateless adapter for an eligible direct procedure
address. The adapter has the runtime signature, casts the original user data
to the worker's actual parameter type, and calls its ordinary typed declaration.
The runtime retains the adapter's code address and the original user pointer.
There is no extra allocation, borrowed stack context or shared adapter state.
Other source uses of the original function address keep their identity.

Eligibility requires one physical object-pointer parameter and a void result.
Imported, runtime and abstract targets are excluded. On x86 the worker's calling
convention must match the runtime convention; the wrapper carries that
convention too. Dynamic callback expressions retain their existing evaluation
and conversion path. This repair does not resolve every stored function-pointer
variance warning.

Only function-pointer and `Any Ptr` casts can be traversed when recognizing a
direct address. An intervening integer narrowing or unrelated pointer cast
belongs to the source expression. The compile-only `uninvokedNarrowFactory`
control deliberately narrows a code address and never calls it. Its emitted C
must retain the byte conversion and must not substitute the original worker.

## GAS64 address values

The same control exposed invalid assembly: GAS64 selected a byte register as
the destination of `lea`. The instruction has no byte-register form. Byte
address conversions now materialize the address in the same full physical
register; later byte uses retain the requested low bits. Boolean conversion
tests the complete address and keeps the register live until its result is
consumed. Word and dword value controls retain their existing narrowing.

## Current local evidence

The unselected candidate is
`7aa49433fcc890e7c1d779c1e6a405c08ed5d514d5d9b256e916f2b50ea06799`.
It reproduces all 224 compiler C units at a fixed point with zero GCC warnings.
The final rebuild reuses 223 unchanged native objects, then all 224 in its
fixed-point round. Build scratch is reused and removed after qualification.
Installed binaries and normal runtime archives are unchanged.
The live JRPG audit continues using its original selected executable.

The regression passes 20 native value runs: both C producers through GCC and
Clang, GNU11/GNU23 and O0/O2, plus GAS64 and LLVM at O0/O2. C checks retain the
original aggressive unsuppressed flags and make incompatible callback casts
fatal. Four Win32 C object checks and two Linux ELF assembly checks pass;
these objects are not native execution evidence for those targets.

Values cover typed workers, a forward static method, function identity,
callback/context evaluation counts, packed payloads and signed/unsigned address
narrowing. Every worker is joined before its record is read, and workers run
one at a time. The two complete C-backend suites pass in 248.084 seconds,
including 462 source-warning checks. Compiler structure passes 409 documented
files and 21 host graphs. Strict lint is clean for the four owned BASIC files.

Eight semantic models pass the independent reader across both C producers,
32/64-bit targets and text/binary transport. Enabling export preserves eight
exact native C identities. The adapter remains an ordinary typed pointer value
in the existing call-ABI schema; no new argument kind or schema revision is needed.
Both permanent C-backend regression lanes also validate the callback model.

The four unchanged critical-section and timer-library profiles lose four Clang
strict callback-cast observations, with every other warning class unchanged.
The full retained corpus refresh is complete: 6,419 profiles, 6,313 accepted C
units, GCC 198 / Clang 16,007. The four removed callback observations are the
only changes against the preceding equal-coverage candidate. All other warning
classes retain their counts; no project applications were executed.

The core worker finished all 5,845 profiles, then rejected four concurrently
edited native library files at its final input guard. Its raw database remains
unchanged. A separate dependency reconciliation re-emits 412 fresh profiles,
checks their exact C hashes and runs both native preprocessors' dependency
queries. None reads the changed native files or any additional header. This
resolves the generated-C census guard without qualifying those library edits.
The retained label-alignment runtime proof still applies by exact C, tool,
source and runtime-artifact identity.

An intermediate adapter incorrectly discarded an integer cast and was rejected.
The first GAS64 fix then failed the Boolean register-lifetime control and was
also rejected. A later ELF harness check read stale C because GAS64 replaced
the requested `.c` suffix with `.asm`; the corrected check uses an explicit,
fresh assembly slot. Original failed receipts remain intact. The final candidate
reruns the complete native value and object matrix.
The next candidate failed a direct semantic export because it introduced an
unsupported argument kind. Its recount was stopped through its verified process
tree after 238 records. Those partial records remain intact and supply native
diagnostics only after exact C/tool/flag checks in the corrected candidate's
refresh. They are not a completed census. Four rejected intermediate executables
were retired, freeing 16,653,312 bytes; their recipes and object ancestry remain.

Receipts use the existing `build/c-emitter-forward-views-20261105/` directory:
`thread-callback-selfhost-v5-20261112`,
`thread-callback-qualification-v5-20261112`,
`thread-callback-semantic-v5-20261112`,
`thread-callback-gates-v5-20261112`, and
`thread-callback-original-examples-v5-20261112` compressed packets. The retained
core database is `thread-callback-v5-queue-refresh-20261112.sqlite3`; the completed
serial tail covers the retained extracted examples, TurboTrek contexts and Tiko
entry. `thread-callback-v5-terminal-reconciliation-20261112.json.zlib` resolves
the raw final guard, and `thread-callback-v5-cohort-close-20261112.json.zlib`
binds all completed queues and their warning classes.
No corpus applications are executed. PNG, moving Optical inputs and existing
closed assembler/provider boundaries remain excluded.

<!-- end of c-emitter-thread-callbacks.md -->
