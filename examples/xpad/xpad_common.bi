''
'' Project: FreeBASIC examples
'' ---------------------------
''
'' File: xpad_common.bi
''
'' Purpose:
''
''     Shared helpers for the GETXPAD examples.
''
'' Responsibilities:
''
''     - poll one controller into a small example-local state type
''     - format GETXPAD status, buttons, d-pad, and axis values
''     - provide a controller-friendly exit check for Xbox-style pads
''
'' This file intentionally does NOT contain:
''
''     - platform-specific controller code
''     - input remapping
''     - game logic
''

#ifndef __EXAMPLES_XPAD_COMMON_BI__
#define __EXAMPLES_XPAD_COMMON_BI__

''
'' GETXPAD itself is a compiler intrinsic.  This include is used here only
'' for the named XPAD_*, d-pad, and keyboard scancode constants.
''
#include "fbgfx.bi"
using fb

const XPAD_EXAMPLE_MAX_PADS = 16

type XPAD_EXAMPLE_STATE
	status as integer
	buttons as integer
	dpad as integer
	lstick_x as single
	lstick_y as single
	rstick_x as single
	rstick_y as single
	ltrigger as single
	rtrigger as single
end type

declare sub xpad_poll( byval pad as integer, byref state as XPAD_EXAMPLE_STATE )
declare sub xpad_add_button( byref text as string, byval buttons as integer, byval mask as integer, byval button_name as string )
declare function xpad_button_list( byval buttons as integer ) as string
declare function xpad_dpad_name( byval dpad as integer ) as string
declare function xpad_status_name( byval status as integer ) as string
declare function xpad_axis_text( byval value as single ) as string
declare function xpad_trigger_text( byval value as single ) as string
declare function xpad_fit( byval value as string, byval columns as integer ) as string
declare function xpad_exit_pressed( byref state as XPAD_EXAMPLE_STATE ) as integer

sub xpad_poll( byval pad as integer, byref state as XPAD_EXAMPLE_STATE )
	state.buttons = 0
	state.dpad = 0
	state.lstick_x = 0.0
	state.lstick_y = 0.0
	state.rstick_x = 0.0
	state.rstick_y = 0.0
	state.ltrigger = 0.0
	state.rtrigger = 0.0

	state.status = getxpad( pad, state.buttons, _
		state.lstick_x, state.lstick_y, _
		state.rstick_x, state.rstick_y, _
		state.ltrigger, state.rtrigger, _
		state.dpad )
end sub

sub xpad_add_button( byref text as string, byval buttons as integer, byval mask as integer, byval button_name as string )
	if (buttons and mask) = 0 then
		exit sub
	end if

	if len( text ) > 0 then
		text = text + " "
	end if

	text = text + button_name
end sub

function xpad_button_list( byval buttons as integer ) as string
	dim as string text = ""

	xpad_add_button text, buttons, XPAD_BUTTON_A, "A"
	xpad_add_button text, buttons, XPAD_BUTTON_B, "B"
	xpad_add_button text, buttons, XPAD_BUTTON_X, "X"
	xpad_add_button text, buttons, XPAD_BUTTON_Y, "Y"
	xpad_add_button text, buttons, XPAD_BUTTON_L1, "L1"
	xpad_add_button text, buttons, XPAD_BUTTON_R1, "R1"
	xpad_add_button text, buttons, XPAD_BUTTON_L2, "L2"
	xpad_add_button text, buttons, XPAD_BUTTON_R2, "R2"
	xpad_add_button text, buttons, XPAD_BUTTON_L3, "L3"
	xpad_add_button text, buttons, XPAD_BUTTON_R3, "R3"
	xpad_add_button text, buttons, XPAD_BUTTON_START, "Start"
	xpad_add_button text, buttons, XPAD_BUTTON_SELECT, "Select"
	xpad_add_button text, buttons, XPAD_BUTTON_GUIDE, "Guide"

	if len( text ) = 0 then
		text = "none"
	end if

	return text
end function

function xpad_dpad_name( byval dpad as integer ) as string
	dim as string text = ""

	if (dpad and XPAD_DPAD_UP) <> 0 then
		text = text + "up"
	end if

	if (dpad and XPAD_DPAD_RIGHT) <> 0 then
		if len( text ) > 0 then text = text + " "
		text = text + "right"
	end if

	if (dpad and XPAD_DPAD_DOWN) <> 0 then
		if len( text ) > 0 then text = text + " "
		text = text + "down"
	end if

	if (dpad and XPAD_DPAD_LEFT) <> 0 then
		if len( text ) > 0 then text = text + " "
		text = text + "left"
	end if

	if len( text ) = 0 then
		text = "center"
	end if

	return text
end function

function xpad_status_name( byval status as integer ) as string
	select case as const status
	case XPAD_STATUS_CONNECTED
		return "connected"
	case XPAD_STATUS_DISCONNECTED
		return "disconnected"
	case else
		return "missing"
	end select
end function

function xpad_axis_text( byval value as single ) as string
	dim as integer scaled = cint( value * 100.0 )
	dim as string sign_text

	if scaled < 0 then
		sign_text = "-"
		scaled = -scaled
	else
		sign_text = " "
	end if

	return sign_text + ltrim( str( scaled \ 100 ) ) + "." + _
		right( "00" + ltrim( str( scaled mod 100 ) ), 2 )
end function

function xpad_trigger_text( byval value as single ) as string
	dim as integer scaled

	if value < 0.0 then value = 0.0
	if value > 1.0 then value = 1.0

	scaled = cint( value * 100.0 )

	return ltrim( str( scaled \ 100 ) ) + "." + _
		right( "00" + ltrim( str( scaled mod 100 ) ), 2 )
end function

function xpad_fit( byval value as string, byval columns as integer ) as string
	if columns <= 0 then
		return ""
	end if

	if len( value ) >= columns then
		return left( value, columns )
	end if

	return value + space( columns - len( value ) )
end function

function xpad_exit_pressed( byref state as XPAD_EXAMPLE_STATE ) as integer
	if state.status <> XPAD_STATUS_CONNECTED then
		return 0
	end if

	if ((state.buttons and XPAD_BUTTON_START) <> 0) and _
		((state.buttons and XPAD_BUTTON_SELECT) <> 0) then
		return -1
	end if

	return 0
end function

#endif

'' end of xpad_common.bi
