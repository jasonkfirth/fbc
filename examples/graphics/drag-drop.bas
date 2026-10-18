'' FreeBASIC graphics examples: drag-drop.bas
'' Purpose: Receive local files and supported UTF-8 text through SCREENEVENT.
'' Responsibilities: Capability checking, copied payloads and responsive input.
'' This example displays data; it never opens a dropped path or executes text.
#lang "fb"
#include once "fbdrop.bi"

'' Build normally for gfxlib2, or add -gfx3 for gfxlib3. The source API is the
'' same. Windows supports file drops; X11 also accepts UTF-8 text selections.
if screenres(640, 360, 32) <> 0 then end 1
dim status as long = FBC.screenDropEnable()
if status <> FB.DROP_OK then
	print "This graphics driver has no drop receiver, status "; status
	sleep 2000, 1
	screen 0
	end 1
end if
print "Drop files or supported text here. Escape or Close exits."
dim event_data as FB.EVENT
dim running as long = 1
while running
	while screenevent(@event_data)
		select case event_data.type
		case FB.EVENT_DROP
			print "Drop "; event_data.drop_id; ", items "; event_data.drop_count
			for index as long = 0 to event_data.drop_count - 1
				dim text as string
				status = FBC.screenDropGet(event_data.drop_id, index, text)
				if status = FB.DROP_OK then
					print left(text, 72)
				else
					print "Payload unavailable, status "; status
				end if
			 next
		case FB.EVENT_WINDOW_CLOSE
			running = 0
		case FB.EVENT_KEY_PRESS
			if event_data.scancode = FB.SC_ESCAPE then running = 0
		end select
	wend
	sleep 10, 1
wend
FBC.screenDropEnable(0)
screen 0
'' end of drag-drop.bas
