'' Project: FreeBASIC native curl qualification
'' File: curl-native.bas
'' Purpose: Check the real curl ABI and synchronous CHttp transfers.
'' Responsibilities: Exact binary bodies, borrowed upload lifetime, redirects,
'' multipart fields, repeated owners and transfer error recovery.
'' This file contains no substitute SDK or public network endpoint.
#lang "fb"
#include once "curl.bi"
#include once "CHttp.bi"

sub require( byval condition as boolean, byref description as const string )
	if( not condition ) then
		print "CURL_NATIVE=FAIL "; description
		end 1
	end if
end sub

if( command(1) = "abi" ) then
	print "ABI"; sizeof(size_t); sizeof(clong); sizeof(curl_off_t); _
	      sizeof(curl_socket_t); sizeof(curl_socklen_t); sizeof(CURLcode); _
	      sizeof(curl_slist); offsetof(curl_slist, data); offsetof(curl_slist, next); _
	      sizeof(curl_httppost); offsetof(curl_httppost, contents); offsetof(curl_httppost, bufferlength)
	end 0
end if

dim base_url as string = command(1)
require( left(base_url, 17) = "http://127.0.0.1:", "a private loopback fixture is required" )
dim expected as string = space( 256 * 1025 + 4 )
for index as integer = 0 to len(expected) - 5
	expected[index] = index and 255
next
mid( expected, len(expected)-3, 4 ) = "tail"

for cycle as integer = 1 to 3
	scope
		dim connection as CHttp
		dim stream as CHttpStream = CHttpStream( @connection )
		dim handle as CURL ptr = connection.getHandle( )
		require( handle <> NULL, "create connection" )
		dim as clong timeout_ms = 3000
		require( curl_easy_setopt( handle, CURLOPT_TIMEOUT_MS, timeout_ms ) = CURLE_OK, "set timeout" )
		for path as integer = 0 to 1
			dim url as string = base_url & iif( path = 0, "/body", "/chunked" )
			require( stream.receive( url, NULL, FALSE ), "receive binary body" )
			require( stream.read( TRUE ) = expected, "binary body length and bytes" )
			require( len(stream.read()) = 0, "text stops at the initial zero" )
		next
		require( stream.receive( base_url & "/empty", NULL, FALSE ), "receive empty body" )
		require( stream.read(TRUE) = "", "empty body has no terminator byte" )

		'' The caller retains the buffer before, during and after synchronous PUT.
		for path as integer = 0 to 1
			dim url as string = base_url & iif( path = 0, "/upload", "/redirect-upload" )
			require( stream.send( url, strptr(expected), len(expected), NULL, FALSE ), "upload and redirect rewind" )
			require( stream.read(TRUE) = expected, "upload response is separate from caller storage" )
			require( expected[0] = 0 and expected[255] = 255, "caller still owns intact bytes" )
		next
		require( stream.send( base_url & "/upload-text", strptr(expected), len(expected), NULL, FALSE ), "upload with text response" )
		require( stream.read() = "UPLOAD_OK", "text upload response is terminated" )
		require( stream.read(TRUE) = "UPLOAD_OK", "binary upload response omits terminator" )
		require( stream.send( base_url & "/upload", NULL, 0, NULL, FALSE ), "empty PUT must execute" )
		require( stream.read(TRUE) = "", "empty PUT response" )
		require( not stream.send( base_url & "/upload", NULL, 1 ), "reject missing upload storage" )
		require( not stream.send( base_url & "/upload", strptr(expected), -1 ), "reject negative length" )
		require( not stream.send( NULL, strptr(expected), len(expected) ), "reject missing URL" )
		require( stream.receive( base_url & "/body", NULL, FALSE ), "GET after upload without reset" )
		require( stream.read(TRUE) = expected, "GET after upload bytes" )
		require( not stream.receive( base_url & "/truncated", NULL, FALSE ), "reject truncated response" )
		require( stream.read(TRUE) = "", "failed transfer clears partial bytes" )

		timeout_ms = 100
		require( curl_easy_setopt( handle, CURLOPT_TIMEOUT_MS, timeout_ms ) = CURLE_OK, "set short deadline" )
		require( not stream.receive( base_url & "/slow", NULL, FALSE ), "real timeout" )
		require( stream.read(TRUE) = "", "timeout clears response" )
		timeout_ms = 3000
		require( curl_easy_setopt( handle, CURLOPT_TIMEOUT_MS, timeout_ms ) = CURLE_OK, "restore deadline" )
		require( stream.receive( base_url & "/body", NULL, FALSE ), "recover after error" )
		require( stream.read(TRUE) = expected, "recovered response" )

		dim form as CHttpForm
		require( form.add( "field", "alpha" ), "add copied text field" )
		require( form.add( "number", 42 ), "add numeric field" )
		require( form.add( "typed", "beta", "text/plain" ), "add typed field" )
		require( not form.add( NULL, "invalid" ), "reject missing form field name" )
		require( connection.post( base_url & "/form", @form, TRUE ) = "FORM_OK", "multipart form POST" )
	end scope
next
print "CURL_NATIVE=PASS cycles=3 binary=262404 upload=12 redirects=3 errors=6 form=3"

'' end of curl-native.bas
