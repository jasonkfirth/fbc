'' FreeBASIC curl examples
'' File: test.bas
'' Purpose: Read a URL using the CHttp wrapper.
'' Responsibilities: Report transfer failure and let scoped owners clean up.
'' This file contains no form construction or upload policy.
'' Build the CHttp static library first, then compile test.bas.

#include once "CHttp.bi"

dim exit_status as integer = 1
scope
	dim url as string = command(1)
	if( len( url ) = 0 ) then url = "https://freebasic.net/"
	dim http_stream as CHttpStream
	if( http_stream.receive( url ) ) then
		print http_stream.read( TRUE );
		exit_status = 0
	else
		print "HTTP transfer failed"
	end if
	'' Destruction releases the stream and its automatically created connection.
end scope
end exit_status

'' end of test.bas
