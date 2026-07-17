''
'' CHttp curl example
'' ------------------
''
'' File: CHttpStream.bas
''
'' Purpose:
''
''     Move response and request bodies between libcurl and an in-memory
''     stream used by the CHttp example.
''
'' Responsibilities:
''
''     - provide ABI-correct libcurl read and write callbacks
''     - grow and release the receive buffer
''     - configure and perform receive and send transfers
''
'' This file intentionally does NOT contain:
''
''     - libcurl global or easy-handle ownership
''     - form construction
''     - application-level HTTP policy
''

#include once "CHttp.bi"
#include once "curl.bi"
#include once "crt/string.bi"

const CSTREAM_MINALLOC = 1024

type CStream
	as byte ptr				buffer
	as size_t				size
	as size_t				pos
	as integer				owns_buffer
end type

type CHttpStreamCtx_
	as CHttp ptr 			http
	as integer				delcon
	as CStream 				stream
end type

'':::::
constructor CHttpStream _
	( _
		byval http as CHttp ptr _
	)

	ctx = new CHttpStreamCtx_

	if( http = NULL ) then
		http = new CHttp
		ctx->delcon = TRUE
	else
		ctx->delcon = FALSE
	end if

	ctx->http = http
	ctx->stream.buffer = NULL
	ctx->stream.size = 0
	ctx->stream.pos = 0
	ctx->stream.owns_buffer = FALSE

end constructor

'':::::
destructor CHttpStream _
	( _
		_
	)

	if( ctx->stream.buffer <> NULL andalso ctx->stream.owns_buffer ) then
		deallocate( ctx->stream.buffer )
	end if
	ctx->stream.buffer = NULL
	ctx->stream.owns_buffer = FALSE

 	if( ctx->http <> NULL ) then
 		if( ctx->delcon ) then
 			delete ctx->http
 		end if
 		ctx->http = NULL
	end if

	delete ctx

end destructor

'':::::
private function recv_cb cdecl _
	( _
		byval buffer as byte ptr, _
		byval size as size_t, _
		byval nitems as size_t, _
		byval userdata as any ptr _
	) as size_t

	dim as CStream ptr memstream = userdata

	if( memstream = NULL ) then
		return 0
	end if

	dim as size_t maximum_size = cast( size_t, -1 )

	if( size <> 0 andalso nitems > maximum_size \ size ) then
		return 0
	end if

	dim as size_t bytes = size * nitems

	if( bytes <> 0 andalso buffer = NULL ) then
		return 0
	end if

	if( memstream->pos > memstream->size ) then
		return 0
	end if

	if( bytes > maximum_size - memstream->pos ) then
		return 0
	end if

	dim as size_t data_end = memstream->pos + bytes

	if( data_end >= memstream->size ) then
		dim as size_t growth

		if( bytes < CSTREAM_MINALLOC ) then
			growth = CSTREAM_MINALLOC
		else
			growth = bytes
		end if

		'' Reallocate one extra byte because receive() appends a terminator.
		if( memstream->size = maximum_size ) then
			return 0
		end if

		if( growth > maximum_size - memstream->size - 1 ) then
			return 0
		end if

		dim as size_t new_size = memstream->size + growth

		if( new_size < data_end ) then
			return 0
		end if

		dim as byte ptr new_buffer = reallocate( memstream->buffer, new_size + 1 )

		if( new_buffer = NULL ) then
			return 0
		end if

		memstream->buffer = new_buffer
		memstream->size = new_size
		memstream->owns_buffer = TRUE
	end if

	if( bytes <> 0 ) then
		memcpy( memstream->buffer + memstream->pos, buffer, bytes )
	end if
	memstream->pos += bytes

	function = bytes

end function

'':::::
function CHttpStream.receive _
	( _
		byval url as zstring ptr, _
		byval referer as zstring ptr, _
		byval doreset as integer _
	) as integer

	dim as CURL ptr curl

	if( ctx->http = NULL ) then
		return FALSE
	end if

	curl = ctx->http->getHandle( )
	if( curl = NULL ) then
		return FALSE
	end if

 	''
	if( ctx->stream.buffer <> NULL andalso ctx->stream.owns_buffer ) then
		deallocate( ctx->stream.buffer )
	end if
	ctx->stream.buffer = NULL
	ctx->stream.size = 0
	ctx->stream.pos = 0
	ctx->stream.owns_buffer = FALSE

 	''
	if( doreset ) then
		curl_easy_reset( curl )
	end if

	dim as clong follow_location = 1
	dim as clong maximum_redirects = 16

	curl_easy_setopt( curl, CURLOPT_URL, url )

	curl_easy_setopt( curl, CURLOPT_FOLLOWLOCATION, follow_location )
	curl_easy_setopt( curl, CURLOPT_MAXREDIRS, maximum_redirects )

	if( referer <> NULL ) then
		curl_easy_setopt( curl, CURLOPT_REFERER, referer )
	end if

	curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, @recv_cb )
	curl_easy_setopt( curl, CURLOPT_WRITEDATA, @ctx->stream )

 	if( curl_easy_perform( curl ) <> 0 ) then
		if( ctx->stream.buffer <> NULL andalso ctx->stream.owns_buffer ) then
			deallocate( ctx->stream.buffer )
		end if
		ctx->stream.buffer = NULL
		ctx->stream.size = 0
		ctx->stream.pos = 0
		ctx->stream.owns_buffer = FALSE
		return FALSE
 	end if

	if( ctx->stream.buffer <> NULL ) then
		ctx->stream.buffer[ctx->stream.pos] = 0
	end if

	function = TRUE

end function

'':::::
function CHttpStream.read _
	( _
		byval is_binary as integer _
	) as string

	if( ctx->stream.buffer <> NULL ) then
		if( is_binary = FALSE ) then
			function = *cptr(zstring ptr, ctx->stream.buffer)
		else
			dim as string res = space( ctx->stream.pos + 1 )
			memcpy( strptr( res ), ctx->stream.buffer, ctx->stream.pos + 1 )
			function = res
		end if

	else
		function = ""
	end if

end function

'':::::
private function send_cb cdecl _
	( _
		byval buffer as byte ptr, _
		byval size as size_t, _
		byval nitems as size_t, _
		byval userdata as any ptr _
	) as size_t

	dim as CStream ptr memstream = userdata

	if( memstream = NULL ) then
		return 0
	end if

	dim as size_t maximum_size = cast( size_t, -1 )

	if( size <> 0 andalso nitems > maximum_size \ size ) then
		return 0
	end if

	dim as size_t bytes = size * nitems

	if( bytes > memstream->size ) then
		bytes = memstream->size
	end if

	if( bytes = 0 ) then
		return 0
	end if

	if( buffer = NULL or memstream->buffer = NULL ) then
		return 0
	end if

	if( bytes > maximum_size - memstream->pos ) then
		return 0
	end if

	memcpy( buffer, memstream->buffer + memstream->pos, bytes )
	memstream->pos += bytes
	memstream->size -= bytes

	function = bytes

end function

'':::::
function CHttpStream.send _
	( _
		byval url as zstring ptr, _
		byval data_ as any ptr, _
		byval bytes as integer, _
		byval referer as zstring ptr, _
		byval doreset as integer _
	) as integer

	dim as CURL ptr curl

	if( ctx->http = NULL ) then
		return FALSE
	end if

	if( bytes <= 0 ) then
		return TRUE
	end if

	if( data_ = NULL ) then
		return FALSE
	end if

	curl = ctx->http->getHandle( )
	if( curl = NULL ) then
		return FALSE
	end if

 	''
	if( ctx->stream.buffer <> NULL andalso ctx->stream.owns_buffer ) then
		deallocate( ctx->stream.buffer )
	end if
	ctx->stream.buffer = data_
	ctx->stream.size = bytes
	ctx->stream.pos = 0
	ctx->stream.owns_buffer = FALSE

 	''
	if( doreset ) then
		curl_easy_reset( curl )
	end if

	dim as clong follow_location = 1
	dim as clong maximum_redirects = 16

	curl_easy_setopt( curl, CURLOPT_URL, url )

	curl_easy_setopt( curl, CURLOPT_FOLLOWLOCATION, follow_location )
	curl_easy_setopt( curl, CURLOPT_MAXREDIRS, maximum_redirects )

	if( referer <> NULL ) then
		curl_easy_setopt( curl, CURLOPT_REFERER, referer )
	end if

	curl_easy_setopt( curl, CURLOPT_READFUNCTION, @send_cb )
	curl_easy_setopt( curl, CURLOPT_READDATA, @ctx->stream )

	dim as CURLcode transfer_result = curl_easy_perform( curl )

	'' send() borrows the caller's buffer only for the synchronous transfer.
	'' Clear the pointer before any later receive, send, or destructor can see
	'' memory that this object does not own.
	ctx->stream.buffer = NULL
	ctx->stream.size = 0
	ctx->stream.pos = 0
	ctx->stream.owns_buffer = FALSE

	if( transfer_result <> 0 ) then
		return FALSE
	end if

	function = TRUE

end function

'' end of CHttpStream.bas
