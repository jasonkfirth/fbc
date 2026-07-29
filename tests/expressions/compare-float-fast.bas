/'
    FreeBASIC compiler regression test
    ----------------------------------

    File: compare-float-fast.bas

    Purpose:

        Exercise runtime floating-point comparisons in fast math mode.

    Responsibilities:

        - cover each relational operation with true and false operands
        - cover branch and value-producing comparison forms
        - cover inverted comparison results

    This file intentionally does NOT contain:

        - NaN expectations, which fast math mode leaves unspecified
        - compile-time constant comparisons
'/

' TEST_MODE : COMPILE_AND_RUN_OK

private function runtimeValue( byval value as double ) as double
	return value
end function

private sub require( byval condition as integer )
	if( condition = FALSE ) then
		end 1
	end if
end sub

private sub checkPair( byval left_value as double, byval right_value as double )
	dim as double lhs = runtimeValue( left_value )
	dim as double rhs = runtimeValue( right_value )
	dim as boolean result

	if( lhs < rhs ) then
		require( left_value < right_value )
	else
		require( left_value >= right_value )
	end if

	if( lhs <= rhs ) then
		require( left_value <= right_value )
	else
		require( left_value > right_value )
	end if

	if( lhs > rhs ) then
		require( left_value > right_value )
	else
		require( left_value <= right_value )
	end if

	if( lhs >= rhs ) then
		require( left_value >= right_value )
	else
		require( left_value < right_value )
	end if

	if( lhs = rhs ) then
		require( left_value = right_value )
	else
		require( left_value <> right_value )
	end if

	if( lhs <> rhs ) then
		require( left_value <> right_value )
	else
		require( left_value = right_value )
	end if

	result = (lhs < rhs)
	require( result = iif( left_value < right_value, TRUE, FALSE ) )
	result = not (lhs < rhs)
	require( result = iif( left_value < right_value, FALSE, TRUE ) )

	result = (lhs <= rhs)
	require( result = iif( left_value <= right_value, TRUE, FALSE ) )
	result = not (lhs <= rhs)
	require( result = iif( left_value <= right_value, FALSE, TRUE ) )
end sub

checkPair( -7.5, 3.25 )
checkPair( 3.25, -7.5 )
checkPair( 4.0, 4.0 )

end 0

/' end of compare-float-fast.bas '/
