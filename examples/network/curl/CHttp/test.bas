''
'' CHttp example
''
'' to build: fbc test.bas
''			 (build the CHttp static library first)
''

#include once "CHttp.bi"

	scope
		'' create object
		dim as CHttpStream http_stream
	
		'' get the page
		if( http_stream.receive( "http://freebasic.net/" ) ) then
			print http_stream.read( )
		end if
	
		'' the stream will be closed when the object is destroyed
	end scope
	
