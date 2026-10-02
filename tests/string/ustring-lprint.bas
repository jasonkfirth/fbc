'' Project: FreeBASIC text type tests
'' File: ustring-lprint.bas
'' Purpose: Check that Unicode printer formats link with platform libraries.
'' Responsibilities: Retain printer calls without sending output to a device.
'' This file intentionally does NOT open a printer or run the printer probe.

'' TEST_MODE : COMPILE_AND_RUN_OK
#lang "deprecated"

'' Keep the calls in an uncalled procedure so the linker must resolve them
'' while the executable remains safe on hosts without a configured printer.
sub printer_probe()
	dim as ustring value = uchr(&hE9, &h4E2D)
	lprint using ustring("&"); value
	lprint using wstr("&"); wstr(value)
end sub

end 0

'' end of ustring-lprint.bas
