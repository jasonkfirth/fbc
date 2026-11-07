<!--
Project: FreeBASIC compiler and runtime verification
File: c-emitter-packed-descriptors.md
Purpose: Record the nested packed descriptor layout and native access repair.
Responsibilities: Distinguish current source, ABI checks and remaining acceptance work.
This file intentionally does NOT claim that the complete warning campaign is finished.
-->

# Packed managed descriptors, November 4, 2026

The compiler and native library preserve valid BASIC record declarations.
Existing examples are unchanged. A packed parent can place a managed child at
byte offset one even though that child contains naturally positioned STRING
and array descriptor fields.

The C emitter previously omitted the parent's member-packing attribute after
comparing only the child's declared FIELD setting. Its C representation can
have greater alignment because direct managed fields retain runtime alignment.
The emitter now compares that effective C alignment. Whole-record size and
every emitted named-field offset assertion remain enabled. The independently
reproduced 152-byte child, 154-byte parent and child offset one compile and
execute with both C producers and both C compilers.

Restoring those addresses exposed a native library contract defect. Recoverable
alignment instrumentation reported 95 accesses in the ordinary local string
and array lifecycle. Native implementations now use alignment-one descriptor
pointer views. The public FBSTRING, FBARRAY and FBARRAYDIM storage types retain
their natural alignment. By-value locals, descriptor arrays, temporary-string
pool entries and runtime contexts retain their original layout. Pointer
declarations use the same access contract as definitions and macros.

The library change is mechanical across descriptor pointer declarations and
casts. It preserves comments, string literals, original source newline
conventions and unrelated dirty edits. Exact compressed preimages and source
hashes are retained under `build/c-emitter-packed-managed-20261104`.

## Qualification

The canonical-source compiler is `bf893b7466560ef09c3b2d6be4729671eba6fba564fe092729ffcb9b4ce5af01`.
Its full 224-unit emitted-C fixed point changes one object and retains zero GCC
and 1,652 Clang warnings. It includes no private ERROR ownership providers.
The earlier private compiler `f6b0905a` independently passes the same compiler
warning totals and nested-layout controls.

The permanent `tests/compiler-support/c-packed-descriptors` fixtures check
record size, child offset, prefix/suffix bytes, string values, fixed string
array values and dynamic array storage. Ordinary local destruction runs before
the alignment counter is queried. The focused matrix passes 16 instrumented
Win64 executions, 28 Win32 native objects and eight Win32 emitted-program
objects. Both C producers/tools, O0/O2 and ordinary/MT runtime links are covered.
MT links execute a single thread; Win32 execution is not claimed.

Before/after comparisons pass for all 175 affected Make-selected Windows
runtime modules and 27 graphics consumers under GCC and Clang. Every compared
export set matches and no warning class increases. Forty public layout values
match byte-for-byte at each width under each compiler. Access-type identity
and function-type assertions pass. An old-header caller also links under GCC
LTO with type mismatches treated as errors and executes with the new routines.
Clang LTO is not claimed.

A further 16 Win64 executions instrument all 175 affected Windows runtime
modules in ordinary and MT modes, with zero alignment reports. Both backend
suites, seven procedure-state tests, the compiler structure guard and strict
BASIC lint pass. The stage summary binds their exact inputs. The original focused
driver bytes are retained alongside its historical receipt; later optional
full-cohort support does not rewrite that receipt's source identity.

## Remaining scope

This closes the reproduced nested packed-layout failure and ordinary descriptor
access case. Other packed-layout combinations require their own checks. General
managed/partial construction and private ERROR ownership promotion remain open.
Unselected platform implementations receive the same pointer-type change, but
this receipt does not claim native SDK or runtime qualification for those hosts.
PNG implementations and PNG-specific tests remain excluded.

The later [paired census](c-emitter-packed-queues.md) binds canonical
`bf893b74` and private `f6b0905a` across the whole selected queues. Its private
GCC timing gate remains failed. The subsequent [trailing-layout repair](c-emitter-packed-tail.md)
extends flat packed managed records and nested wrappers in canonical source.
That later compiler has not received another whole-queue census.

The retained core is capped at 32 MiB, with two workers at most and a 512 MiB
free-space floor on C:. The floor stopped a compiler pass. Another 1,773 closed
compressed result/receipt files, totaling 734,103,623 bytes, were moved to E:
with matching hashes and an original-path junction. One physical copy remains.
Compiler output and tool temporaries use E:. Installed binaries and archives
are unchanged.

<!-- end of c-emitter-packed-descriptors.md -->
