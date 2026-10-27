'' Project: FreeBASIC compiler tests
'' File: c-temp-varargs.bas
'' Purpose: Check aggregate temporary storage beside builtin argument lists.
'' Responsibilities: Verify vararg scalar/aggregate values and independent copies.
'' This file intentionally does NOT use platform I/O or external test libraries.
'' TEST_MODE : COMPILE_AND_RUN_OK
#lang "fb"

type ArgumentPair
	left_value as long
	right_value as long
end type

private sub inspect_arguments cdecl( byval sentinel as long, ... )

	dim as cva_list original_args = any, copied_args = any
	cva_start( original_args, sentinel )
	cva_copy( copied_args, original_args )
	if( cva_arg( copied_args, long ) <> 12 ) then end 1
	dim as zstring ptr text = cva_arg( copied_args, zstring ptr )
	if( *text <> "cat" ) then end 2
	if( cva_arg( copied_args, double ) <> 1.5 ) then end 3
	dim as integer consumed_extra = ((sentinel <> 0) andalso (cva_arg( copied_args, long ) = 99))
	if( consumed_extra <> (sentinel <> 0) ) then end 4
	dim as ArgumentPair ptr pair = cva_arg( copied_args, ArgumentPair ptr )
	if( (pair->left_value <> 3) or (pair->right_value <> 7) ) then end 5
	if( cva_arg( original_args, long ) <> 12 ) then end 6
	cva_end( copied_args )
	cva_end( original_args )

end sub

dim as zstring * 8 text = "cat"
dim as ArgumentPair pair = type( 3, 7 )
inspect_arguments( 0, clng( 12 ), @text, cdbl( 1.5 ), @pair )
inspect_arguments( 1, clng( 12 ), @text, cdbl( 1.5 ), clng( 99 ), @pair )
print "c-temp-varargs-ok"
end 0

'' end of c-temp-varargs.bas
