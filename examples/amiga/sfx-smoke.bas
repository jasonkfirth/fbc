'' FreeBASIC classic AmigaOS sound smoke test
'' ----------------------------------------
'' File: sfx-smoke.bas
'' Purpose: Exercise native sound initialization, playback, and shutdown.
'' Responsibilities: Play two bounded tones without graphics or input.
'' This file intentionally does NOT contain interactive or capture-device tests.

print "Starting native sound"
sound 440, 6
print "First tone submitted"
sound 880, 6
print "Second tone submitted"
sleep 500, 1
print "AmigaOS sound smoke passed"
end 0

'' end of sfx-smoke.bas
