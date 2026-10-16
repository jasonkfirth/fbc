'' FreeBASIC graphics regressions
'' File: line.bas
'' Purpose: Check styled LINE pixels and invalid endpoint rejection.
'' Responsibilities: Compare complete null-driver framebuffers after drawing.
'' This file intentionally does NOT contain rasterization implementation.

# include "fbcunit.bi"
#include once "fbgfx.bi"

SUITE( fbc_tests.gfx.line_ )

	const SCREEN_W = 64
	const SCREEN_H = 32

	TEST( linestyle )
	
		CU_ASSERT( screenres( SCREEN_W, SCREEN_H, 32, , fb.GFX_NULL ) = 0 )

		dim style as ulong = &b1000000011110000
		dim c0 as ulong = rgb(0,0,0)
		dim c1 as ulong = rgb(0,255,0)
		dim c2 as ulong 

		for i as integer = 0 to 31		
			line (i, i) - (SCREEN_W-1, i), c1, , style
		next

		'' 1000000011110000100000001111000010000000111100001000000011110000
		'' 0100000001111000010000000111100001000000011110000100000001111000
		'' 0010000000111100001000000011110000100000001111000010000000111100
		'' 0001000000011110000100000001111000010000000111100001000000011110
		'' 0000100000001111000010000000111100001000000011110000100000001111
		'' ...

		for y as integer = 0 to SCREEN_H-1
			for x as integer = y to SCREEN_W-1
				c2 = c0
				if( x >= y ) then
					dim index as integer = (x - y) mod 16
					if( (style and (&h8000ul shr index)) <> 0 ) then
						c2 = c1
					end if 
				end if
				CU_ASSERT( point( x, y ) = c2 )
			next
		next
	END_TEST

	TEST( invalid_coordinates )
		CU_ASSERT( screenres(8, 8, 32, , fb.GFX_NULL) = 0 )
		'' Both NaN signs, infinities and finite values outside the int range.
		dim patterns(0 to 5) as ulong => { &h7fc00000ul, &hffc00000ul, _
			&h7f800000ul, &hff800000ul, &h7f7ffffful, &hff7ffffful }
		for sample as integer = 0 to ubound(patterns)
			for endpoint as integer = 0 to 3
				for shape as integer = 0 to 2
					dim coords(0 to 3) as single => { 0, 0, 7, 7 }
					coords(endpoint) = cvs(mkl(patterns(sample)))
					cls
					select case shape
					case 0
						line (coords(0), coords(1))-(coords(2), coords(3)), rgb(255,255,255)
					case 1
						line (coords(0), coords(1))-(coords(2), coords(3)), rgb(255,255,255), b
					case 2
						line (coords(0), coords(1))-(coords(2), coords(3)), rgb(255,255,255), bf
					end select
					for y as integer = 0 to 7
						for x as integer = 0 to 7
							CU_ASSERT( point(x,y) = rgb(0,0,0) )
						next
					next
				next
			next
		next

		'' Finite endpoints can overflow only after WINDOW translates them.
		window (0,0)-(1e-30,1e-30)
		line (0,0)-(1,1), rgb(255,255,255)
		window
		for y as integer = 0 to 7
			for x as integer = 0 to 7
				CU_ASSERT( point(x,y) = rgb(0,0,0) )
			next
		next
		screen 0
	END_TEST

END_SUITE

'' end of line.bas
