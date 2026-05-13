''
'' Project: FreeBASIC examples
'' ---------------------------
''
'' File: stick-view.bas
''
'' Purpose:
''
''     Visualize one GETXPAD controller.
''
'' Responsibilities:
''
''     - draw left and right stick positions
''     - draw trigger pressure bars
''     - highlight the currently pressed buttons
''
'' This file intentionally does NOT contain:
''
''     - dead-zone filtering
''     - controller remapping
''     - gameplay behaviour
''

#include "xpad_common.bi"

declare sub draw_stick( byval cx as integer, byval cy as integer, byval radius as integer, byval x as single, byval y as single, byval label as string )
declare sub draw_trigger( byval x as integer, byval y as integer, byval width_pixels as integer, byval value as single, byval label as string )
declare sub draw_button( byval x as integer, byval y as integer, byval label as string, byval pressed as integer )

dim state as XPAD_EXAMPLE_STATE
dim as integer pad = 0
dim as integer done

if screenres( 800, 480, 32 ) <> 0 then
	print "Unable to open a graphics screen."
	end 1
end if

do
	if multikey( SC_1 ) then pad = 0
	if multikey( SC_2 ) then pad = 1
	if multikey( SC_3 ) then pad = 2
	if multikey( SC_4 ) then pad = 3

	xpad_poll pad, state

	screenlock
	cls

	draw string ( 24, 20 ), "GETXPAD stick-view example", rgb( 255, 255, 255 )
	draw string ( 24, 40 ), "Keys 1-4 select pad. ESC or Start+Select exits.", rgb( 180, 180, 180 )
	draw string ( 24, 68 ), "Pad " + ltrim( str( pad ) ) + ": " + xpad_status_name( state.status ), rgb( 220, 220, 220 )

	if state.status = XPAD_STATUS_CONNECTED then
		draw_stick 180, 220, 90, state.lstick_x, state.lstick_y, "Left stick"
		draw_stick 420, 220, 90, state.rstick_x, state.rstick_y, "Right stick"

		draw_trigger 580, 126, 160, state.ltrigger, "L2"
		draw_trigger 580, 168, 160, state.rtrigger, "R2"

		draw_button 580, 230, "Y", (state.buttons and XPAD_BUTTON_Y) <> 0
		draw_button 620, 270, "B", (state.buttons and XPAD_BUTTON_B) <> 0
		draw_button 540, 270, "X", (state.buttons and XPAD_BUTTON_X) <> 0
		draw_button 580, 310, "A", (state.buttons and XPAD_BUTTON_A) <> 0

		draw_button 60, 355, "L1", (state.buttons and XPAD_BUTTON_L1) <> 0
		draw_button 120, 355, "L3", (state.buttons and XPAD_BUTTON_L3) <> 0
		draw_button 300, 355, "R1", (state.buttons and XPAD_BUTTON_R1) <> 0
		draw_button 360, 355, "R3", (state.buttons and XPAD_BUTTON_R3) <> 0
		draw_button 520, 355, "Select", (state.buttons and XPAD_BUTTON_SELECT) <> 0
		draw_button 610, 355, "Start", (state.buttons and XPAD_BUTTON_START) <> 0
		draw_button 700, 355, "Guide", (state.buttons and XPAD_BUTTON_GUIDE) <> 0

		draw string ( 70, 118 ), "D-pad: " + xpad_dpad_name( state.dpad ), rgb( 220, 220, 220 )
		draw string ( 70, 138 ), "Buttons: " + xpad_button_list( state.buttons ), rgb( 220, 220, 220 )
	else
		draw string ( 24, 112 ), "Connect an Xbox-style controller or select another pad.", rgb( 255, 220, 90 )
		draw string ( 24, 132 ), "GETXPAD keeps missing, connected, and previously disconnected states separate.", rgb( 180, 180, 180 )
	end if

	screenunlock

	done = multikey( SC_ESCAPE ) or xpad_exit_pressed( state )
	sleep 15, 1
loop until done

sub draw_stick( byval cx as integer, byval cy as integer, byval radius as integer, byval x as single, byval y as single, byval label as string )
	dim as integer px = cx + cint( x * radius )
	dim as integer py = cy - cint( y * radius )

	line ( cx - radius, cy - radius )-( cx + radius, cy + radius ), rgb( 70, 70, 70 ), b
	line ( cx - radius, cy )-( cx + radius, cy ), rgb( 70, 70, 70 )
	line ( cx, cy - radius )-( cx, cy + radius ), rgb( 70, 70, 70 )
	circle ( cx, cy ), radius, rgb( 120, 120, 120 )
	circle ( px, py ), 7, rgb( 120, 255, 150 ), , , , f

	draw string ( cx - radius, cy + radius + 18 ), label + " " + _
		xpad_axis_text( x ) + "," + xpad_axis_text( y ), rgb( 220, 220, 220 )
end sub

sub draw_trigger( byval x as integer, byval y as integer, byval width_pixels as integer, byval value as single, byval label as string )
	dim as integer fill_width

	if value < 0.0 then value = 0.0
	if value > 1.0 then value = 1.0

	fill_width = cint( (width_pixels - 2) * value )

	draw string ( x, y - 18 ), label + " " + xpad_trigger_text( value ), rgb( 220, 220, 220 )
	line ( x, y )-( x + width_pixels, y + 18 ), rgb( 120, 120, 120 ), b
	if fill_width > 0 then
		line ( x + 1, y + 1 )-( x + fill_width, y + 17 ), rgb( 120, 255, 150 ), bf
	end if
end sub

sub draw_button( byval x as integer, byval y as integer, byval label as string, byval pressed as integer )
	dim as integer border_color = rgb( 120, 120, 120 )
	dim as integer fill_color = rgb( 30, 30, 30 )
	dim as integer text_color = rgb( 180, 180, 180 )

	if pressed then
		border_color = rgb( 255, 255, 255 )
		fill_color = rgb( 120, 255, 150 )
		text_color = rgb( 0, 0, 0 )
	end if

	line ( x, y )-( x + 58, y + 28 ), border_color, b
	line ( x + 1, y + 1 )-( x + 57, y + 27 ), fill_color, bf
	draw string ( x + 8, y + 10 ), label, text_color
end sub

'' end of stick-view.bas
