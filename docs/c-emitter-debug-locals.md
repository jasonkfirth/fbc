<!--
Project: FreeBASIC C backend qualification
File: c-emitter-debug-locals.md
Purpose: Explain unread source storage and discarded TYPEOF scratch in debug C.
Responsibilities: Record ownership, source diagnostics and native qualification.
This file does not claim complete corpus or foreign runtime qualification.
-->

# Unread storage in debug C

The fb-ext-lib expansion exposed 62 GCC unread-local observations in 20 debug
profiles. Ten concern compiler-generated record temporaries left after `TYPEOF`
discarded a record-returning call. The other 52 concern named source locals:
30 records, 20 pointers and two scalars. Removing source declarations or stores
would change the debugger's view and could discard initializer or lifetime
effects.

`backend/c/c-debug-locals.bi` indexes local reads in reachable optimized AST
statements. Direct stores are distinguished from destination reads; their
right-hand sides still receive the complete scan. Addresses, constructors,
indirect stores, hidden call result slots, string copyback and profiling
children are conservative observations. The procedure owns its hash keys and
borrows symbols until ordinary cleanup clears the index. Opaque assembly
disables this policy.

For an unread named scalar or record, debug C appends an enum containing
`sizeof(&local)` to the declaration line. This constant expression preserves
declarations, initializers, stores and lifetime code without evaluating a value
or adding runtime storage. Arrays, references, descriptors, generated variables,
static and shared storage follow their existing policies. Non-debug C stays
unchanged.

The `TYPEOF` temporary has a different disposition. The parser deletes the
queried expression and its destructor list. Constructor/destructor flags on
the type do not instantiate an object. The backend can omit an unreferenced
compiler-owned declaration after the complete statement scan, while retaining
its type. Real construction, cleanup, hidden return slots, initialization trees
and opaque assembly remain reasons to retain storage.

FBC warnings 54 and 55 keep their existing behavior. Scalar and pointer source
warnings remain visible. Constructed records retain the existing lifetime-aware
policy: construction, destruction or an escaped address can make an object
useful without an explicit source read. This change does not introduce a broad
unused-object warning. Native data-flow and cast diagnostics remain enabled.

## Qualification

Selected development compiler hash:
`fc092d79d744a7491bf980fade21705900fbd39dd430dcde0898327f089f85f9`.
All 224 compiler C units reproduce at a fixed point with zero GCC warnings.
Both complete backend suites pass, including 462 source-warning controls.

The focused regression passes 16 native value runs across both C producers and
native tools, GNU11/C23 and O0/O2. Four GDB runs inspect four named unread values
in unoptimized debug programs. Four opaque controls retain native warnings,
two ordinary C pairs are byte-identical to the preceding compiler, and four
Win32 objects pass. Eight semantic models pass the independent reader; export
preserves eight exact C identities. Win32 objects are compilation evidence only.

The record fixture checks two real constructions and destructions while its
queried factory is never called. The old compiler emits an additional unused
record declaration. Enum declarations can change private stack-slot placement;
checks compare runtime/debugger values rather than identical instructions.

All 194 fb-ext-lib contexts are checked: 186 accepted C units and eight unchanged
runner-context refusals. GCC falls from 77 to 15 warnings and Clang from 227 to
165. All 62 unread-local observations disappear per tool; every other native
warning line is preserved in each profile. All 20 changed C files have identical program text
after removing only acknowledgements, line directives and proven unreferenced
temporary declarations. The other 166 accepted C files are byte-identical.
Accepted units retain 658 FBC parameter and 250 variable warnings; refused
contexts retain two parameter warnings. Nine GCC data-flow and six const-cast
observations remain. Native flags stay unsuppressed.

The separate Tiko/toolkit recount accepts all 125 contexts: GCC 40 to 29 and
Clang 4,593 to 4,582. Its 11 changed C files retain identical program text after
removing only constant debug declarations; the other 114 are byte-identical.
Every other native warning line and all 2,078 source parameter / 801 source
variable warnings remain. Its 28 null and one bounds observations still require
source investigation; this check does not execute the applications.

The backend gate also exposed a console fixture still naming the old private C
context after an unrelated local runtime rename. Its mock accepts both names
and removes the runtime ABI alias only while including the mocked implementation.
Committed and locally renamed implementations each pass eight GCC/Clang
narrow/wide and threaded/non-threaded checks. Production runtime sources and
archives are unchanged by the fixture repair.

Receipts reuse `build/c-emitter-forward-views-20261105/`, with
`debug-local-selfhost-v1-`, `debug-local-values-v1-`, `debug-local-semantic-v1-`,
`debug-local-gates-v1r2-`, `debug-local-class-close-v1-` and
`current-fbext-debug-local-v1-` prefixes. The
[current census](c-emitter-current-status.md) separates compiler cuts and project
contexts from pending native and runtime qualification.

<!-- end of c-emitter-debug-locals.md -->
