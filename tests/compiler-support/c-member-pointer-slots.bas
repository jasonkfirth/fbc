'' Project: FreeBASIC C backend tests
'' File: c-member-pointer-slots.bas
'' Purpose: Preserve a guarded pointer slot's identity through a record-pointer cast.
'' Responsibilities: Null entry paths, actual slot copies and valid state updates.
'' This file intentionally does NOT create a window or reinterpret pointer storage.

type SlotState
	item_enabled(0 to 31) as integer
	hot_item as integer
	updates as integer
end type

type SlotFrame
	'' Match the native-word displacement used by the GUI regression while
	'' retaining the same source contract on 32-bit object checks.
	header(0 to 37) as integer
	data as any ptr
end type

dim shared as integer input_calls

function CurrentItem(byval state as SlotState ptr) as integer
	input_calls += 1
	if state->item_enabled(0) then return 0
	return -1
end function

sub UpdateFrame(byval frame as SlotFrame ptr)
	if frame = 0 orelse frame->data = 0 then exit sub
	var state = cast(SlotState ptr, frame->data)
	var selected = CurrentItem(state)
	if selected >= 0 andalso state->item_enabled(selected) = 0 then selected = -1
	state->hot_item = selected
	state->updates += 1
end sub

function ReadFrame(byval frame as SlotFrame ptr) as integer
	if frame = 0 orelse frame->data = 0 then return -7
	var state = cast(const SlotState ptr, frame->data)
	return state->hot_item
end function

dim frame as SlotFrame
dim state as SlotState
UpdateFrame(0)
UpdateFrame(@frame)
if input_calls <> 0 or ReadFrame(0) <> -7 or ReadFrame(@frame) <> -7 then end 1
state.item_enabled(0) = -1
frame.data = @state
UpdateFrame(@frame)
if input_calls <> 1 or state.updates <> 1 or ReadFrame(@frame) <> 0 then end 2
state.item_enabled(0) = 0
UpdateFrame(@frame)
if input_calls <> 2 or state.updates <> 2 or ReadFrame(@frame) <> -1 then end 3
frame.data = 0
UpdateFrame(@frame)
if input_calls <> 2 or state.updates <> 2 or ReadFrame(@frame) <> -7 then end 4
print "MEMBER_POINTER_SLOTS_PASS"

'' end of c-member-pointer-slots.bas
