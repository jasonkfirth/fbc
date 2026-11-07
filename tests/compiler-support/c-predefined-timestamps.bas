'' Project: FreeBASIC predefined timestamp regressions
'' File: c-predefined-timestamps.bas
'' Purpose: Check the FORMAT-backed compiler date and time definitions.
'' Responsibilities: Validate spelling, field agreement and controlled-epoch output.
'' This file intentionally does NOT read or change the system clock.

#lang "fb"

const compile_date = __DATE__
const compile_iso = __DATE_ISO__
const compile_time = __TIME__

if len(compile_date) <> 10 then end 1
if len(compile_iso) <> 10 then end 2
if len(compile_time) <> 8 then end 3
if mid(compile_date, 3, 1) <> "-" or mid(compile_date, 6, 1) <> "-" then end 4
if mid(compile_iso, 5, 1) <> "-" or mid(compile_iso, 8, 1) <> "-" then end 5
if mid(compile_time, 3, 1) <> ":" or mid(compile_time, 6, 1) <> ":" then end 6

if mid(compile_date, 7, 4) <> left(compile_iso, 4) then end 7
if left(compile_date, 2) <> mid(compile_iso, 6, 2) then end 8
if mid(compile_date, 4, 2) <> right(compile_iso, 2) then end 9

for position as integer = 1 to len(compile_time)
	if position = 3 or position = 6 then continue for
	if instr("0123456789", mid(compile_time, position, 1)) = 0 then end 10
next
if valint(left(compile_time, 2)) > 23 then end 11
if valint(mid(compile_time, 4, 2)) > 59 then end 12
if valint(right(compile_time, 2)) > 59 then end 13

if command(1) = "values" then
	print compile_date; "|"; compile_iso; "|"; compile_time
else
	print "PREDEFINED_TIMESTAMPS_PASS"
end if

'' end of c-predefined-timestamps.bas
