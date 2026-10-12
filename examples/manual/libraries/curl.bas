'' FreeBASIC manual examples
'' File: curl.bas
'' Purpose: Fetch a URL with libcurl's easy interface.
'' Responsibilities: Preserve callback bytes, report errors, and release curl state.
'' This file contains no redirects, retries or asynchronous transfers.
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=ExtLibcurl

#include once "curl.bi"
#include once "crt/string.bi"

'' libcurl supplies a byte count, not a terminated string. A FreeBASIC STRING
'' preserves embedded zeros; the semicolon avoids adding callback-dependent
'' newlines. The temporary chunk belongs to this synchronous callback only.
private function write_callback cdecl ( byval buffer as byte ptr, _
                                        byval size as size_t, _
                                        byval nitems as size_t, _
                                        byval userdata as any ptr ) as size_t
	dim maximum as size_t = cast( size_t, -1 )
	if( size <> 0 andalso nitems > maximum \ size ) then return 0
	dim bytes as size_t = size * nitems
	if( bytes > (maximum shr 1) ) then return 0
	if( bytes <> 0 andalso buffer = NULL ) then return 0
	dim chunk as string = space( bytes )
	if( len( chunk ) <> bytes ) then return 0
	if( bytes <> 0 ) then memcpy( strptr( chunk ), buffer, bytes )
	print chunk;
	return bytes
end function

function main() as integer
	dim url as string = command(1)
	if( len( url ) = 0 ) then url = "https://freebasic.net/"
	if( curl_global_init( CURL_GLOBAL_ALL ) <> CURLE_OK ) then return 1
	dim handle as CURL ptr = curl_easy_init( )
	if( handle = NULL ) then
		curl_global_cleanup( )
		return 1
	end if

	'' Long options use C long in varargs, including on 64-bit Windows.
	'' Bound the wait to five seconds for connection and fifteen in total.
	dim as clong connect_timeout = 5000, total_timeout = 15000, fail_http = 1
	dim result as CURLcode = CURLE_FAILED_INIT
	do
		if( curl_easy_setopt( handle, CURLOPT_URL, strptr( url ) ) <> CURLE_OK ) then exit do
		if( curl_easy_setopt( handle, CURLOPT_WRITEFUNCTION, @write_callback ) <> CURLE_OK ) then exit do
		if( curl_easy_setopt( handle, CURLOPT_CONNECTTIMEOUT_MS, connect_timeout ) <> CURLE_OK ) then exit do
		if( curl_easy_setopt( handle, CURLOPT_TIMEOUT_MS, total_timeout ) <> CURLE_OK ) then exit do
		if( curl_easy_setopt( handle, CURLOPT_FAILONERROR, fail_http ) <> CURLE_OK ) then exit do
		result = curl_easy_perform( handle )
	loop while FALSE

	if( result <> CURLE_OK ) then print "curl transfer failed: "; *curl_easy_strerror( result )
	curl_easy_cleanup( handle )
	curl_global_cleanup( )
	if( result <> CURLE_OK ) then return 1
	return 0
end function

end main()

'' end of curl.bas
