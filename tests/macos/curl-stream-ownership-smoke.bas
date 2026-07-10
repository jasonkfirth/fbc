''
'' FreeBASIC macOS curl tests
'' -------------------------
''
'' File: curl-stream-ownership-smoke.bas
''
'' Purpose:
''
''     Verify that the CHttp example borrows, but never frees, upload memory
''     supplied by its caller.
''
'' Responsibilities:
''
''     - pass static caller-owned storage through CHttpStream.send()
''     - force a synchronous transfer failure without network access
''     - destroy the stream after the failed transfer
''     - reject a null form without dereferencing it
''
'' This file intentionally does NOT contain:
''
''     - an Internet dependency
''     - a successful HTTP upload
''     - ownership of the upload buffer by CHttpStream
''

#include once "CHttp.bi"

const SMOKE_OK = 0
const SMOKE_UNEXPECTED_TRANSFER = 1
const SMOKE_NULL_FORM_ACCEPTED = 2

dim shared payload(0 to 3) as ubyte = _
	{ _
		asc( "d" ), _
		asc( "a" ), _
		asc( "t" ), _
		asc( "a" ) _
	}

scope
	dim stream as CHttpStream

	if( stream.send( _
		"fbc-unsupported://ownership-smoke", _
		@payload(0), _
		ubound(payload) + 1 _
	) ) then
		end SMOKE_UNEXPECTED_TRANSFER
	end if
end scope

scope
	dim http as CHttp

	if( http.post( "fbc-unsupported://null-form-smoke", NULL ) <> "" ) then
		end SMOKE_NULL_FORM_ACCEPTED
	end if
end scope

end SMOKE_OK

'' end of curl-stream-ownership-smoke.bas
