<!--
Project: FreeBASIC C backend
File: c-emitter-blocks.md
Purpose: Describe C block declarations and named addresses for packed byte copies.
Responsibilities: Record ordering, scope, representation and qualification contracts.
This file intentionally does NOT claim complete compiler or platform acceptance.
-->

# C block declarations and packed copies

`src/compiler/backend/c/ir-hlc.bas` gives each procedure and lexical scope a
declaration section followed by a body section. Uninitialized C declarations and
constant-initialized C statics belong to the first section. Assignments, calls
and automatic initialized declarations keep their body positions. Closing a
scope joins its sections without changing its C
braces or the BASIC scope that owns its variables.

Local type definitions are emitted into the declaration section of their
owning scope before variables that use them. BASIC scope numbers predate the
header/body section pairs, so the emitter maps them to the corresponding
declaration section and keeps sibling scope types separate.

This distinction also applies to call-result temporaries. Storage can precede
statements, but evaluating the call must remain at its original position.
Builtin `va_list` arrays retain their initializer because C cannot assign an
array afterward. Procedure-entry label removal still operates on the body
before it is joined to the declaration section.

The existing section stack owns both buffers. Its bound accounts for two
sections per scope; checked indexing, retained capacity and backend shutdown
release remain in place.

## Static local declarations

C static initializers describe constant data, including addresses in local array
descriptors. Moving their declarations within the same C block does not execute
an initializer earlier or recreate storage on another call. BASIC runtime
initialization remains in the body. The emitter records whether it actually
wrote C's `static` storage class; a general BASIC symbol attribute alone does
not select this treatment.

`tests/scopes/static-local-order.bas` calls the same procedure through both
branches repeatedly. It checks persistent scalar and array updates, descriptor
bounds, a fixed string, distinct sibling static locals and calls before and
after the declarations. Both Win64 native controls and both Win32 object
profiles retain their original assertions. The control's declaration warning
is present with the previous emitter and absent with the repaired emitter.

## Packed field addresses

`c-member-access.bi` can recover a named field address for a byte copy without
turning that copy into a typed load or store. Packed fields continue through
`memcpy`. Their declared address supplies layout information that raw byte
pointer arithmetic loses. Ordinary value access retains its stricter alignment
rules, and records represented as bitfield byte bodies cannot use this path.

An address-only pointer match permits a pointer-word temporary with another
object-pointer type. The copy preserves the stored representation; it does not
perform a pointer conversion. Field bounds, offsets and actual declared types
still come from the compiler's symbol metadata.

## Qualification

The unchanged helper snapshot reports 368 declaration-order and 186 buffer
warnings with the previous emitter, then zero and 173 with the repaired emitter.
The introduced fifteen declaration and three buffer warnings are removed.
`tests/structs/order-and-packed.bas` checks call order, sibling locals, packed
pointer storage and surrounding guard bytes. Its four generated C profiles are
byte-identical to the qualified reduction, retaining both Win64 native runs and
the Win32 object checks. Strict non-semantic lint reports no diagnostics.

The same 189-module Win64 cohort passes 378 unsuppressed GCC/Clang profiles.
GCC remains at 3,919 warnings. Clang falls from 17,485 to 10,324: declaration-order
warnings fall from 6,987 to 37 and buffer warnings from 4,184 to 3,973. All complete
global-symbol sets match. The initializer warning families remain zero.

This measurement uses 374 fresh C compilations and four exact-C retained proofs;
325 fresh compilations reproduce their previous object bytes. Complete GCC and
Clang compiler producers are linked from the 189 qualified objects for each
profile. Each producer re-emits all 378 C outputs byte for byte, retaining their
compile proofs without another C compilation. The remaining warnings and broader
corpus/platform checks remain separate gates. The remaining 37 declaration-order
observations in that measurement identify initialized static locals.

The following static-local repair passes the same 378-profile cohort with zero
declaration-order warnings. GCC remains at 3,919 warnings and Clang falls to
10,287. It recompiles 58 changed C profiles and retains 320 exact-C proofs;
53 fresh compilations reproduce their previous object bytes. All complete
global-symbol sets match. The initializer families remain zero, and all other
warning-family counts remain unchanged.
Both complete producers link the newly qualified objects and each re-emit all
378 C outputs byte for byte. The final Clang producer repeats the two native
controls from four exact-C focused profiles; the GCC producer retains the same
focused compile and native proofs through its four matching C outputs.

<!-- end of c-emitter-blocks.md -->
