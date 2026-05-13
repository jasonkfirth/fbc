''
'' Project: FreeBASIC examples
'' ---------------------------
''
'' File: first-pad.bas
''
'' Purpose:
''
''     Minimal GETXPAD example for reading controller 0.
''
'' Responsibilities:
''
''     - check whether pad 0 exists
''     - read the full controller state when connected
''     - print the raw values returned by GETXPAD
''
'' This file intentionally does NOT contain:
''
''     - multi-controller scanning
''     - custom dead-zone handling
''     - game-specific input mapping
''

const PAD = 0

declare function status_name( byval status as integer ) as string
declare function axis_text( byval value as single ) as string
declare function trigger_text( byval value as single ) as string

dim as integer buttons, dpad
dim as single lstick_x, lstick_y
dim as single rstick_x, rstick_y
dim as single ltrigger, rtrigger
dim as long status
dim as integer done

if screenres( 640, 360, 32 ) <> 0 then
	print "Unable to open a graphics screen."
	end 1
end if

width 80, 22

do
	buttons = 0
	dpad = 0
	lstick_x = 0.0
	lstick_y = 0.0
	rstick_x = 0.0
	rstick_y = 0.0
	ltrigger = 0.0
	rtrigger = 0.0

	status = getxpad( PAD, buttons, _
		lstick_x, lstick_y, _
		rstick_x, rstick_y, _
		ltrigger, rtrigger, _
		dpad )

	cls
	color 15, 0

	locate 2, 3
	print "GETXPAD first-pad example"
	locate 3, 3
	print "No include file is needed to call GETXPAD. Press ESC to exit."

	locate 6, 3
	print "Pad"; PAD; " status: "; status_name( status ); " ("; status; ")"

	if status = 1 then
		locate 8, 3
		print "Button mask:   &h"; hex( buttons, 8 )
		locate 9, 3
		print "D-pad mask:    &h"; hex( dpad, 8 )
		locate 11, 3
		print "Left stick:    "; axis_text( lstick_x ); ", "; axis_text( lstick_y )
		locate 12, 3
		print "Right stick:   "; axis_text( rstick_x ); ", "; axis_text( rstick_y )
		locate 14, 3
		print "Left trigger:  "; trigger_text( ltrigger )
		locate 15, 3
		print "Right trigger: "; trigger_text( rtrigger )
	else
		locate 8, 3
		print "Connect an Xbox-style controller as pad"; PAD; "."
		locate 10, 3
		print "GETXPAD(id) returns 0 for missing, 1 for connected, and 2 for disconnected."
	end if

	done = (inkey() = chr( 27 ))
	sleep 25, 1
loop until done

function status_name( byval status as integer ) as string
	select case status
	case 1
		return "connected"
	case 2
		return "disconnected"
	case else
		return "missing"
	end select
end function

function axis_text( byval value as single ) as string
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

function trigger_text( byval value as single ) as string
	dim as integer scaled

	if value < 0.0 then value = 0.0
	if value > 1.0 then value = 1.0

	scaled = cint( value * 100.0 )

	return ltrim( str( scaled \ 100 ) ) + "." + _
		right( "00" + ltrim( str( scaled mod 100 ) ), 2 )
end function

'' end of first-pad.bas
