/'
 Project: FreeBASIC C backend controls
 File: c-custom-entry.bas
 Purpose: Exercise the explicit alias of an implicit custom main.
 Responsibilities: Check startup arguments and one entry call.
 This file intentionally does NOT use an interactive wait.
'/
#lang "fb"
declare function custom_entry cdecl alias "custom_entry" (byval argc as long, byval argv as zstring ptr ptr) as long
function main cdecl alias "main" (byval argc as long, byval argv as zstring ptr ptr) as long
 if argc < 1 or argv = 0 then return 1
 return custom_entry(argc, argv)
end function
print "CUSTOM_ENTRY_VALUES_PASS"
'' end of c-custom-entry.bas
