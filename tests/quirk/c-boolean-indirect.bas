'' TEST_MODE : COMPILE_AND_RUN_OK
'' Project: FreeBASIC compiler regression tests
'' File: c-boolean-indirect.bas
'' Purpose: Check Boolean conversions through qualified pointer reads.
'' Responsibilities: Exercise true and false values and integer/float results.
'' This file intentionally does NOT test invalid Boolean storage representations.

dim as boolean value = true
dim as boolean ptr writable = @value
dim as const boolean ptr readonly_value = @value

if *writable <> -1 then end 1
if cint(*writable) <> -1 then end 2
if cdbl(*readonly_value) <> -1.0 then end 3

value = false
if *writable <> 0 then end 4
if cint(*readonly_value) <> 0 then end 5
if cdbl(*writable) <> 0.0 then end 6

'' end of c-boolean-indirect.bas
