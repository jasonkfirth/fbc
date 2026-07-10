''
'' FreeBASIC macOS smoke tests
'' ---------------------------
''
'' File: curl-smoke.bas
''
'' Purpose:
''
''     Exercise the FreeBASIC curl binding against the libcurl supplied
''     by macOS.
''
'' Responsibilities:
''
''     - verify the Darwin LP64 layout of public libcurl types
''     - query the linked libcurl version through the binding
''     - transfer a local system file through the easy interface
''     - exercise size_t callbacks and curl_off_t varargs values
''
'' This file intentionally does NOT contain:
''
''     - an Internet dependency
''     - assumptions about a Homebrew or MacPorts installation
''     - coverage of protocols that Apple may omit from system libcurl
''

#include once "curl.bi"

'' -------------------------------------------------------------------------
'' Callback and diagnostic helpers
'' -------------------------------------------------------------------------

private function write_callback cdecl _
	( _
		byval buffer as zstring ptr, _
		byval element_size as size_t, _
		byval element_count as size_t, _
		byval userdata as any ptr _
	) as size_t

	if( userdata = 0 ) then
		return 0
	end if

	dim as size_t maximum_size = cast( size_t, -1 )

	if( element_size <> 0 ) then
		if( element_count > maximum_size \ element_size ) then
			return 0
		end if
	end if

	dim as size_t bytes = element_size * element_count

	if( bytes <> 0 and buffer = 0 ) then
		return 0
	end if

	dim as size_t ptr received = userdata

	if( bytes > maximum_size - *received ) then
		return 0
	end if

	*received += bytes
	return bytes
end function

private sub fail_after_init _
	( _
		byref message as string, _
		byval curl as CURL ptr = 0 _
	)

	print "curl-smoke: failed: "; message

	if( curl <> 0 ) then
		curl_easy_cleanup curl
	end if

	curl_global_cleanup
	end 1
end sub

private function byte_offset _
	( _
		byval base_address as any ptr, _
		byval member_address as any ptr _
	) as size_t

	return cast( size_t, member_address ) - cast( size_t, base_address )
end function

'' -------------------------------------------------------------------------
'' Darwin ABI validation
'' -------------------------------------------------------------------------

''
'' Apple uses the LP64 data model for both x86_64 and arm64.  These checks
'' catch width, alignment, and embedded Darwin sockaddr mistakes before a
'' transfer passes one of the affected types into libcurl.
''
if( len( size_t ) <> 8 ) then
	print "curl-smoke: failed: size_t is not 8 bytes"
	end 1
end if

if( len( curl_off_t ) <> 8 ) then
	print "curl-smoke: failed: curl_off_t is not 8 bytes"
	end 1
end if

if( len( curl_sockaddr ) <> 32 ) then
	print "curl-smoke: failed: curl_sockaddr has an unexpected layout"
	end 1
end if

if( len( curl_fileinfo ) <> 128 ) then
	print "curl-smoke: failed: curl_fileinfo has an unexpected layout"
	end 1
end if

if( len( curl_version_info_data ) <> 200 ) then
	print "curl-smoke: failed: curl_version_info_data has an unexpected layout"
	end 1
end if

if( len( curl_header ) <> 48 ) then
	print "curl-smoke: failed: curl_header has an unexpected layout"
	end 1
end if

dim as curl_sockaddr curl_address_layout

if( byte_offset( @curl_address_layout, @curl_address_layout.addr ) <> 16 ) then
	print "curl-smoke: failed: curl_sockaddr.addr has an unexpected offset"
	end 1
end if

dim as curl_fileinfo file_info_layout

if( byte_offset( @file_info_layout, @file_info_layout.hardlinks ) <> 48 ) then
	print "curl-smoke: failed: curl_fileinfo.hardlinks has an unexpected offset"
	end 1
end if

if( byte_offset( @file_info_layout, @file_info_layout.b_size ) <> 112 ) then
	print "curl-smoke: failed: curl_fileinfo.b_size has an unexpected offset"
	end 1
end if

dim as curl_version_info_data version_info_layout

if( byte_offset( @version_info_layout, @version_info_layout.ssl_version_num ) <> 48 ) then
	print "curl-smoke: failed: curl_version_info_data.ssl_version_num has an unexpected offset"
	end 1
end if

if( byte_offset( @version_info_layout, @version_info_layout.gsasl_version ) <> 192 ) then
	print "curl-smoke: failed: curl_version_info_data.gsasl_version has an unexpected offset"
	end 1
end if

dim as curl_header header_layout

if( byte_offset( @header_layout, @header_layout.origin ) <> 32 ) then
	print "curl-smoke: failed: curl_header.origin has an unexpected offset"
	end 1
end if

if( byte_offset( @header_layout, @header_layout.anchor ) <> 40 ) then
	print "curl-smoke: failed: curl_header.anchor has an unexpected offset"
	end 1
end if

'' -------------------------------------------------------------------------
'' System libcurl transfer
'' -------------------------------------------------------------------------

dim as CURLcode result = curl_global_init( CURL_GLOBAL_DEFAULT )

if( result <> CURLE_OK ) then
	print "curl-smoke: failed: curl_global_init: "; *curl_easy_strerror( result )
	end 1
end if

dim as curl_version_info_data ptr version_info = curl_version_info( CURLVERSION_NOW )

if( version_info = 0 or version_info->version = 0 ) then
	fail_after_init "curl_version_info returned no version"
end if

dim as string version_string = *version_info->version

dim as CURL ptr curl = curl_easy_init()

if( curl = 0 ) then
	fail_after_init "curl_easy_init returned no handle"
end if

dim as zstring * CURL_ERROR_SIZE error_text
dim as zstring * 24 test_url = "file:///etc/hosts"
dim as size_t received_bytes = 0
dim as clong no_signal = 1
dim as curl_off_t maximum_file_size = 1024 * 1024

error_text = ""

result = curl_easy_setopt( curl, CURLOPT_ERRORBUFFER, strptr( error_text ) )
if( result <> CURLE_OK ) then
	fail_after_init "could not install the error buffer", curl
end if

result = curl_easy_setopt( curl, CURLOPT_URL, strptr( test_url ) )
if( result <> CURLE_OK ) then
	fail_after_init "could not set the local test URL", curl
end if

result = curl_easy_setopt( curl, CURLOPT_NOSIGNAL, no_signal )
if( result <> CURLE_OK ) then
	fail_after_init "could not set CURLOPT_NOSIGNAL", curl
end if

result = curl_easy_setopt _
	( _
		curl, _
		CURLOPT_MAXFILESIZE_LARGE, _
		maximum_file_size _
	)
if( result <> CURLE_OK ) then
	fail_after_init "could not set CURLOPT_MAXFILESIZE_LARGE", curl
end if

result = curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, @write_callback )
if( result <> CURLE_OK ) then
	fail_after_init "could not install the write callback", curl
end if

result = curl_easy_setopt( curl, CURLOPT_WRITEDATA, @received_bytes )
if( result <> CURLE_OK ) then
	fail_after_init "could not install the write callback data", curl
end if

result = curl_easy_perform( curl )

if( result <> CURLE_OK ) then
	dim as string message = "local transfer: " + *curl_easy_strerror( result )

	if( error_text[0] <> 0 ) then
		message += ": " + error_text
	end if

	fail_after_init message, curl
end if

dim as curl_off_t downloaded_bytes = 0

result = curl_easy_getinfo( curl, CURLINFO_SIZE_DOWNLOAD_T, @downloaded_bytes )
if( result <> CURLE_OK ) then
	fail_after_init "could not read CURLINFO_SIZE_DOWNLOAD_T", curl
end if

if( received_bytes = 0 ) then
	fail_after_init "the local transfer returned no data", curl
end if

if( downloaded_bytes <> received_bytes ) then
	fail_after_init "callback and libcurl byte counts differ", curl
end if

curl_easy_cleanup curl
curl_global_cleanup

print "curl-smoke: libcurl "; version_string; ", received "; received_bytes; " bytes"
end 0

'' end of curl-smoke.bas
