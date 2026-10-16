'' FreeBASIC classic AmigaOS shutdown qualification
'' ----------------------------------------------
'' File: sound-exit.bas
'' Purpose: Check sound services used from BASIC global destructors.
'' Responsibilities: Reopen sound after END's initial shutdown and return cleanly.
'' This file intentionally does NOT contain: mixer or task cleanup implementation.

private sub Cleanup() destructor
	'' STOP may initialize a fresh context after END closed the original one.
	'' Final runtime cleanup must stop that context before waiting for tasks.
	sfx stop
	print "NATIVE_SOUND_DESTRUCTOR_OK"
end sub

sfx stop
print "NATIVE_SOUND_EXIT_READY"
end 0

'' end of sound-exit.bas
