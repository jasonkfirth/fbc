''
'' ordinary TinyPTC test, based on the original
''

#include once "tinyptc.bi"

const SCR_WIDTH = 320
const SCR_HEIGHT = 200
const SCR_SIZE = SCR_WIDTH*SCR_HEIGHT

dim shared buffer( 0 to SCR_SIZE-1 ) as integer

sub main()
	dim n as integer, carry as integer, index as integer, seed as integer
	
	if( ptc_open( "freeBASIC v0.01 - tinyPTC test", SCR_WIDTH, SCR_HEIGHT ) = 0 ) then
		end -1
	end if
	
    seed = &h12345

    do
    
    	for index = 0 to SCR_SIZE-1
			n = (seed shr 3) xor seed
        	carry = n and 1
        	n = n shr 1
        	seed = seed shr 1
        	seed = seed or (carry shl 30)
        	n = n and &hFF
        	buffer(index) = rgb( n, n, n )
    	next index
    
    	ptc_update @buffer(0)
    
    loop until( inkey = chr( 27 ) )

	ptc_close
end sub

main()
