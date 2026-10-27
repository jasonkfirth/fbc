'' Project: FreeBASIC error-handler regression tests
'' File: local-error-early-exit.bas
'' Purpose: Preserve the caller's handler when local registration is conditional.
'' Responsibilities: Exercise early exit, alternate registrations and repeated
''     registration without relying on RESUME or a particular stack layout.
'' This file intentionally does NOT test error-code reset behavior.
' TEST_MODE : COMPILE_AND_RUN_OK

#ifdef LOCAL_ERROR_TEST_FB
	#lang "fb"
#else
	#lang "fblite"
#endif

private sub guarded_handler( byval mode as integer )

	if mode = 0 then exit sub
	if mode = 1 then
		on local error goto first_handler
	else
		on local error goto second_handler
	end if
	exit sub

first_handler:
	end 101
second_handler:
	end 102
end sub

private sub repeated_handler()

	for count as integer = 1 to 3
		on local error goto local_handler
	next
	exit sub

local_handler:
	end 103
end sub

private sub cleared_handler()

	for count as integer = 1 to 3
		on local error goto 0
	next

end sub

private sub nested_handler( byval depth as integer )

	on local error goto local_handler
	if depth > 0 then nested_handler( depth - 1 )
	exit sub

local_handler:
	end 104
end sub

on error goto after_early_exit
guarded_handler( 0 )
error 24
end 1

after_early_exit:
if err <> 24 then end 2
on error goto after_first_branch
guarded_handler( 1 )
error 25
end 3

after_first_branch:
if err <> 25 then end 4
on error goto after_second_branch
guarded_handler( 2 )
error 26
end 5

after_second_branch:
if err <> 26 then end 6
on error goto after_repeated_handler
repeated_handler()
error 27
end 7

after_repeated_handler:
if err <> 27 then end 8
on error goto after_cleared_handler
cleared_handler()
error 28
end 9

after_cleared_handler:
if err <> 28 then end 10
on error goto after_nested_handler
nested_handler( 3 )
error 29
end 11

after_nested_handler:
if err <> 29 then end 12
end 0

'' end of local-error-early-exit.bas
