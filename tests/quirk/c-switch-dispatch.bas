'' TEST_MODE : COMPILE_AND_RUN_OK
'' Project: FreeBASIC compiler regression tests
'' File: c-switch-dispatch.bas
'' Purpose: Verify constant SELECT dispatch without exposing C label addresses.
'' Responsibilities: Cover signed/unsigned boundaries, holes, initialization,
''     one selector evaluation and managed case-scope retirement.
'' This file intentionally does NOT depend on a C-specific jump-table layout.
#lang "fb"

dim shared as long selector_calls, created_values, destroyed_values

function selector_once( byval value as long ) as long
	selector_calls += 1
	return value
end function

function signed_dispatch( byval value as long ) as long
	dim as long initialized = 37
	select case as const value
	case -80: return initialized + 1
	case -1: return initialized + 2
	case 0: return initialized + 3
	case 7: return initialized + 4
	case 120: return initialized + 5
	case else: return initialized
	end select
end function

'' Keep the key spans small while crossing the sign bit and the 64-bit limit.
function low_dispatch( byval value as longint ) as long
	const low_value as longint = (-9223372036854775807ll - 1ll)
	select case as const value
	case low_value: return 11
	case low_value + 1: return 12
	case low_value + 5: return 15
	case else: return 19
	end select
end function

function high_dispatch( byval value as ulongint ) as long
	select case as const value
	case &hfffffffffffffff0ull: return 21
	case &hfffffffffffffff7ull: return 27
	case &hffffffffffffffffull: return 29
	case else: return 23
	end select
end function

function high32_dispatch( byval value as ulong ) as long
	select case as const value
	case &hfffffff0ul: return 31
	case &hfffffff7ul: return 37
	case &hfffffffful: return 39
	case else: return 33
	end select
end function

function split_scope_dispatch( byval first_mode as long, byval operation as long ) as long
	if( first_mode ) then
		dim as long initialized_left = 123, initialized_right = 17
		select case as const operation
		case 0: return initialized_left + initialized_right
		case 1: return initialized_left - initialized_right
		case else: return initialized_left
		end select
	else
		dim as long initialized_other = 91
		select case as const operation
		case 0: return initialized_other * 2
		case 1: return initialized_other + 1
		case else: return initialized_other
		end select
	end if
end function

type tracked_value
	value as long
	declare constructor()
	declare destructor()
end type
constructor tracked_value()
	created_values += 1
	value = 99
end constructor
destructor tracked_value()
	destroyed_values += 1
end destructor

function managed_dispatch( byval value as long ) as long
	dim as long result = -1
	select case as const value
	case -2 to 2
		dim as tracked_value item
		item.value = value
		select case as const value
		case -2, 0, 2: result = item.value + 10
		case else: result = item.value + 20
		end select
	case else
		result = 77
	end select
	return result
end function

for value as long = -100 to 140
	dim as long expected = 37
	if( value = -80 ) then expected = 38
	if( value = -1 ) then expected = 39
	if( value = 0 ) then expected = 40
	if( value = 7 ) then expected = 41
	if( value = 120 ) then expected = 42
	if( signed_dispatch(value) <> expected ) then end 1
next
const low_value as longint = (-9223372036854775807ll - 1ll)
if( low_dispatch(low_value) <> 11 ) then end 2
if( low_dispatch(low_value + 1) <> 12 ) then end 3
if( low_dispatch(low_value + 5) <> 15 ) then end 4
if( low_dispatch(low_value + 2) <> 19 ) then end 5
if( low_dispatch(low_value + 6) <> 19 ) then end 6
if( high_dispatch(&hfffffffffffffff0ull) <> 21 ) then end 7
if( high_dispatch(&hfffffffffffffff7ull) <> 27 ) then end 8
if( high_dispatch(&hffffffffffffffffull) <> 29 ) then end 9
if( high_dispatch(&hfffffffffffffff8ull) <> 23 ) then end 10
if( high_dispatch(0) <> 23 ) then end 11
if( high32_dispatch(&hfffffff0ul) <> 31 ) then end 12
if( high32_dispatch(&hfffffff7ul) <> 37 ) then end 13
if( high32_dispatch(&hfffffffful) <> 39 ) then end 14
if( high32_dispatch(0) <> 33 ) then end 15
if( split_scope_dispatch(-1, 0) <> 140 ) then end 16
if( split_scope_dispatch(-1, 1) <> 106 ) then end 17
if( split_scope_dispatch(0, 0) <> 182 ) then end 18
if( split_scope_dispatch(0, 1) <> 92 ) then end 19

select case as const selector_once(7)
case 7
case else: end 20
end select
select case as const selector_once(3)
case else
end select
if( selector_calls <> 2 ) then end 21
for value as long = -4 to 4
	dim as long expected = 77
	if( value >= -2 and value <= 2 ) then
		if( (value and 1) = 0 ) then
			expected = value + 10
		else
			expected = value + 20
		end if
	end if
	if( managed_dispatch(value) <> expected ) then end 22
next
if( created_values <> 5 or destroyed_values <> 5 ) then end 23
print "SWITCH_CONTRACT_PASS"
end 0
'' end of c-switch-dispatch.bas
