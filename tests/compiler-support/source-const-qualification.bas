'' Project: FreeBASIC source diagnostic regressions
'' File: source-const-qualification.bas
'' Purpose: Diagnose unsafe nested qualifier additions in selected operations.
'' Responsibilities: Cover aliases, value conversions, arrays and safe boundaries.
'' This file intentionally does NOT execute the unsafe conversion examples.
#lang "fb"

type ConstPayload
	value as long
end type

declare sub BorrowSlot(byref value as const ConstPayload ptr)
declare sub ReadOnlySlot(byref value as const ConstPayload const ptr)
declare sub BorrowNested(byval value as const ConstPayload ptr ptr)
declare sub ReadOnlyNested(byval value as const ConstPayload const ptr ptr)
declare sub BorrowArray(values() as const ConstPayload ptr)
declare sub ReadOnlyArray(values() as const ConstPayload const ptr)
declare sub BorrowDeep(byval value as const ConstPayload ptr const ptr ptr)
declare sub ReadOnlyDeep(byval value as const ConstPayload const ptr const ptr ptr)
declare function FetchAlias() byref as ConstPayload ptr
declare sub SelectSlot overload(byref value as ConstPayload ptr)
declare sub SelectSlot overload(byref value as const ConstPayload ptr)

dim item as ConstPayload = (41)
dim slot as ConstPayload ptr = @item
dim nested as ConstPayload ptr ptr = @slot
dim deep as ConstPayload ptr ptr ptr = @nested
dim slots(0 to 1) as ConstPayload ptr = {@item, @item}

#if QUALIFICATION_CASE = 1
BorrowSlot(slot)
#elseif QUALIFICATION_CASE = 2
dim qualified as const ConstPayload ptr ptr = nested
print cuint(qualified)
#elseif QUALIFICATION_CASE = 3
dim qualified as const ConstPayload ptr ptr
qualified = nested
print cuint(qualified)
#elseif QUALIFICATION_CASE = 4
BorrowNested(nested)
#elseif QUALIFICATION_CASE = 5
BorrowArray(slots())
#elseif QUALIFICATION_CASE = 6
BorrowDeep(deep)
#elseif QUALIFICATION_CASE = 7
ReadOnlySlot(slot)
#elseif QUALIFICATION_CASE = 8
ReadOnlyNested(nested)
#elseif QUALIFICATION_CASE = 9
ReadOnlyArray(slots())
#elseif QUALIFICATION_CASE = 10
ReadOnlyDeep(deep)
#elseif QUALIFICATION_CASE = 11
BorrowSlot(@item)
#elseif QUALIFICATION_CASE = 12
BorrowSlot(iif(true, @item, cptr(ConstPayload ptr, 0)))
#elseif QUALIFICATION_CASE = 13
dim qualified as const ConstPayload ptr = slot
print cuint(qualified)
#elseif QUALIFICATION_CASE = 14
dim byref qualified as const ConstPayload ptr = slot
print cuint(qualified)
#elseif QUALIFICATION_CASE = 15
dim byref qualified as const ConstPayload const ptr = slot
print cuint(qualified)
#elseif QUALIFICATION_CASE = 16
BorrowSlot(FetchAlias())
#elseif QUALIFICATION_CASE = 17
SelectSlot(slot)
#endif

'' end of source-const-qualification.bas
