function nearEqual( byval lhs as single, byval rhs as single ) as integer
	function = (abs( lhs - rhs ) < 0.0001)
end function

setenviron "SFXLIB_DRIVER=null"

ASSERT( tempo() = 120 )

tempo 96
ASSERT( tempo() = 96 )

tempo 1
ASSERT( tempo() = 20 )

tempo 999
ASSERT( tempo() = 400 )

tempo 120
ASSERT( tempo() = 120 )

channel 0
ASSERT( channel() = 0 )

channel 3
ASSERT( channel() = 3 )

channel -1
ASSERT( channel() = 0 )

channel 999
ASSERT( channel() = 15 )

channel 0
ASSERT( channel() = 0 )

volume 0.25
ASSERT( nearEqual( volume(), 0.25 ) )

volume -1.0
ASSERT( nearEqual( volume(), 0.0 ) )

volume 2.0
ASSERT( nearEqual( volume(), 1.0 ) )

volume 2, 0.75
ASSERT( nearEqual( volume( 2 ), 0.75 ) )

volume 2, -1.0
ASSERT( nearEqual( volume( 2 ), 0.0 ) )

volume 2, 2.0
ASSERT( nearEqual( volume( 2 ), 1.0 ) )

pan 4, 0.5
ASSERT( nearEqual( pan( 4 ), 0.5 ) )

pan 4, -2.0
ASSERT( nearEqual( pan( 4 ), -1.0 ) )

pan 4, 2.0
ASSERT( nearEqual( pan( 4 ), 1.0 ) )

balance -0.5
ASSERT( nearEqual( balance(), -0.5 ) )

balance -2.0
ASSERT( nearEqual( balance(), -1.0 ) )

balance 2.0
ASSERT( nearEqual( balance(), 1.0 ) )

octave 5
ASSERT( octave() = 5 )

octave -1
ASSERT( octave() = 0 )

octave 99
ASSERT( octave() = 8 )

channel 1
wave 1, 0
envelope 1, 0.01, 0.10, 0.50, 0.20
instrument 1, 1, 1
voice 1
ASSERT( voice() = 1 )

voice -1
ASSERT( voice() = -1 )
