'' dynamic memory allocation example
'' Ownership: each allocation is released by the pointer that receives it.

const BUFFER_SIZE = 1024 * 8
const BUFFER_FILL_VALUE = 123
const SAMPLE_X = 1234
const SAMPLE_Y = 5678

'' Allocate a chunk of memory of a certain size, measured in bytes
dim as byte ptr buffer = allocate( BUFFER_SIZE )

if( buffer = 0 ) then
	print "Unable to allocate the byte buffer."
	end 1
end if

'' Do something with the buffer
for i as integer = 0 to BUFFER_SIZE - 1
	buffer[i] = BUFFER_FILL_VALUE
next

'' Free up the allocated chunk after we're done using it
deallocate( buffer )
buffer = 0

'' ------------------

type MyType
	x as integer
	y as integer
end type

dim pmytype as MyType ptr

pmytype = allocate( sizeof( MyType ) )

if( pmytype = 0 ) then
	print "Unable to allocate MyType."
	end 1
end if

pmytype->x = SAMPLE_X
pmytype->y = SAMPLE_Y

print pmytype->x, pmytype->y

deallocate( pmytype )
pmytype = 0

sleep
