'' TEST_MODE : COMPILE_AND_RUN_OK
'' Project: FreeBASIC C identifier qualification
'' File: c-identifiers.bas
'' Purpose: Separate encoded aliases from literal escape text and backend names.
'' Responsibilities: Check keyword/type aliases and generated temporary scopes.
'' This file intentionally does NOT use platform-specific APIs.
#lang "fb"
function dollar_name cdecl alias "fb_c_A$X"( byval value as long ) as long
	return value + 1
end function
function underscore_name cdecl alias "fb_c_A_24X"( byval value as long ) as long
	return value + 2
end function
function keyword_name cdecl alias "struct"( byval value as long ) as long
	return value + 3
end function
function scalar_name cdecl alias "int32"( byval value as long ) as long
	return value + 4
end function
function macro_name cdecl alias "__FB_STATIC_ASSERT"( byval value as long ) as long
	return value + 5
end function
function register_name cdecl alias "fb_c_vreg_0"( byval value as long ) as long
	return value + 6
end function
type fixed_pair
	first_value as long
	second_value as long
end type
dim as fixed_pair pair_value = (10, 20)
if dollar_name(pair_value.first_value) <> 11 then end 1
if underscore_name(pair_value.first_value) <> 12 then end 2
if keyword_name(pair_value.first_value) <> 13 then end 3
if scalar_name(pair_value.first_value) <> 14 then end 4
if macro_name(pair_value.first_value) <> 15 then end 5
if register_name(pair_value.first_value) <> 16 then end 6
dim as long selected = 2, selected_value = 0
select case as const selected
case 0: selected_value = 10
case 1: selected_value = 20
case 2: selected_value = 30
case else: selected_value = -1
end select
if selected_value <> 30 then end 7
end 0
'' end of collisions.bas
