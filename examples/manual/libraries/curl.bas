'' examples/manual/libraries/curl.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'curl'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=ExtLibcurl
'' --------

'' Purpose:
''
''     Demonstrate a simple HTTP GET through the libcurl easy interface.
''
'' Responsibilities:
''
''     - create and clean up an easy handle
''     - install a correctly typed write callback
''     - print response data as it arrives
''
'' This file intentionally does NOT contain:
''
''     - application-level retry or redirect policy
''     - response aggregation
''     - an event-driven multi interface example
''
'' Curl HTTP Get example

#include Once "curl.bi"
#include Once "crt/string.bi"

'' this callback will be called when any data is received
Private Function write_callback cdecl _
	( _
		ByVal buffer As Byte Ptr, _
		ByVal size As size_t, _
		ByVal nitems As size_t, _
		ByVal outstream As Any Ptr _
	) As size_t

	Static As ZString Ptr zstr = 0
	Static As size_t maxbytes = 0

	Dim As size_t maximum_size = Cast( size_t, -1 )

	If( size <> 0 AndAlso nitems > maximum_size \ size ) Then
		Return 0
	End If

	Dim As size_t bytes = size * nitems

	If( bytes <> 0 AndAlso buffer = 0 ) Then
		Return 0
	End If

	'' Leave room for the terminator added below.
	If( bytes = maximum_size ) Then
		Return 0
	End If

	'' current zstring buffer too small?
	If( maxbytes < bytes OrElse zstr = 0 ) Then
		Dim As ZString Ptr new_zstr = Reallocate( zstr, bytes + 1 )

		If( new_zstr = 0 ) Then
			Return 0
		End If

		zstr = new_zstr
		maxbytes = bytes
	End If

	'' "buffer" is not null-terminated, so we must dup it and add the null-term
	If( bytes <> 0 ) Then
		memcpy( zstr, buffer, bytes )
	End If
	zstr[bytes] = 0

	'' just print it..
	Print *zstr

	Return bytes
End Function

	'' init
	Dim As CURL Ptr curl = curl_easy_init( )
	If( curl = 0 ) Then
		End 1
	End If

	'' set url and callback
	curl_easy_setopt( curl, CURLOPT_URL, "freebasic.net" )
	curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, @write_callback )

	'' execute..
	curl_easy_perform( curl )

	'' shutdown
	curl_easy_cleanup( curl )

'' end of curl.bas
