<!--
Project: FreeBASIC C backend verification
File: c-emitter-member-address.md
Purpose: Explain named byte-copy slots through native address carriers.
Responsibilities: Preserve storage identity, guarded reads and qualification boundaries.
This file intentionally does NOT reinterpret pointer slots as another pointer type.
-->

# Named slots after native address formation

The native pointer-value repair removed 46,609 Clang observations in the first
complete queue refresh. GCC also exposed five null-dereference observations in
three omaGUI consumers. The widget entry guards were present, and a separate C
layout assertion confirmed that their DATA field and byte-copy displacement
agreed. Application source changes were unnecessary.

The new native unsigned expression had hidden a previously recoverable field
address from the byte-copy emitter. The guard read the named void-pointer slot,
but the later copy used an opaque integer address for that same slot. Restoring
the named copy source keeps the two operations attached to the same C object.
All five observations disappear under the original unsuppressed GCC flags.

Recovery applies only to an address used by a byte copy. It requires a native
unsigned carrier, an original byte-pointer cast, ADD/SUB and a constant
displacement. C promotion can spell the carrier as ULONG or ULONGINT, so the
test uses its representation instead of one BASIC type identity. Constant
casts, target width and signed accumulation are checked before declared member
lookup. Negative, truncating, overflowing or unrecognized paths retain their
native address expression. Member lookup still verifies the complete field
span and representation. Pointer-view value conversions retain their existing
storage restrictions.

The guarded pointer value is still copied with `memcpy`; it is not read through
an incompatible pointer-to-pointer lvalue. Calls and other original operands
remain present once. BASIC/C record layouts and public ABI declarations are
unchanged. The three GUI source trees and their installed applications remain
unchanged.

## Qualification

The complete compiler passes 224 C fixed-point checks with zero GCC and 2,529
Clang warnings. The canonical provider is recorded in
`build/c-emitter-member-address-20261103/canonical/receipt.json`.

The permanent `c-member-pointer-slots.bas` regression checks null entries, null
DATA, valid updates, disabled items and clearing the slot. Both C producers and
both C compilers pass GNU11/GNU23 and O0/O2 native execution; eight Win32 objects
check the corresponding 32-bit representation. A structural assertion requires
the byte-copy source to retain the declared DATA slot.

The 330-case pointer-value matrix still passes 48 native runs, including sixteen
with trapping pointer-overflow checks, plus GAS64/historical controls and 26
foreign objects. Real fbcunit registrations, semantic pointer/name/startup
contracts and the existing backend suite are rechecked against the canonical
provider. Both new regressions are part of the ordinary backend suite, and its
Clang pointer-value case retains the trapping check.

The latest whole-queue receipt and counts are linked from the
[current status](c-emitter-current-status.md). The first pointer refresh,
including its five intervening observations, remains intact. Exact preimages
and separate revision receipts preserve the unrelated dirty compiler and test
driver work.

Each broad worker uses one C/object scratch pair. The pointer and member-address
packets retain separate 64 MiB guards; closed revisions are compressed with
verified reconstruction hashes. An exploratory GUI scratch check stopped before
C compilation at about 70 MiB and its duplicate raw output was retired. No
installed compiler/runtime binary is replaced, and PNG remains excluded.

<!-- end of c-emitter-member-address.md -->
