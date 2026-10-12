''
'' Project: FreeBASIC curl examples
'' File: CHttp.bas
'' Purpose: Own the easy handle used by the CHttp stream and form examples.
'' Responsibilities: Balance libcurl initialization, handle and header-list cleanup.
'' This file contains no response buffering or asynchronous transfers.
''
'' to build: fbc -lib CHttp.bas CHttpStream.bas CHttpForm.bas
''

#include once "CHttp.bi"
#include once "curl.bi"

type CHttpCtx_
	dim as CURL ptr curl
	dim as curl_slist ptr headerlist
end type

'':::::
constructor CHttp _
	( _
		_
	)

	ctx = NULL
	ctx = new CHttpCtx_
	if( ctx = NULL ) then
		return
	end if

	ctx->curl = NULL
	ctx->headerlist = NULL

	'' Each successful initialization is balanced when this owner is destroyed.
	'' These example objects are used on one thread and must not be copied.
	if( curl_global_init( CURL_GLOBAL_ALL ) <> CURLE_OK ) then
		delete ctx
		ctx = NULL
		return
	end if

	ctx->curl = curl_easy_init()
	if( ctx->curl = NULL ) then
		curl_global_cleanup( )
		delete ctx
		ctx = NULL
		return
	end if

	curl_easy_setopt( ctx->curl, CURLOPT_COOKIEFILE, "" )

	ctx->headerlist = curl_slist_append( NULL, "Expect:" )
	if( ctx->headerlist = NULL ) then
		curl_easy_cleanup( ctx->curl )
		ctx->curl = NULL
		curl_global_cleanup( )
		delete ctx
		ctx = NULL
		return
	end if

end constructor

'':::::
destructor CHttp _
	( _
		_
	)

	if( ctx = NULL ) then
		return
	end if

    if( ctx->headerlist <> NULL ) then
        curl_slist_free_all( ctx->headerlist )
        ctx->headerlist = NULL
    end if

	if( ctx->curl <> NULL ) then
		curl_easy_cleanup( ctx->curl )
		ctx->curl = NULL
	end if

	delete ctx
	ctx = NULL
	curl_global_cleanup( )

end destructor

'':::::
function CHttp.post _
	( _
		byval url as zstring ptr, _
		byval form as CHTtpForm ptr, _
		byval is_binary as integer _
	) as string

	if( ctx = NULL ) then
		return ""
	end if

	if( ctx->curl = NULL ) then
		return ""
	end if

	if( form = NULL ) then
		return ""
	end if
	dim form_handle as any ptr = form->getHandle( )
	if( form_handle = NULL ) then return ""

	dim as CHttpStream ptr http_stream = new CHttpStream( @this )
	if( http_stream = NULL ) then
		return ""
	end if

    curl_easy_reset( ctx->curl )

	do
		if( curl_easy_setopt( ctx->curl, CURLOPT_HTTPHEADER, ctx->headerlist ) <> CURLE_OK ) then exit do
		if( curl_easy_setopt( ctx->curl, CURLOPT_HTTPPOST, form_handle ) <> CURLE_OK ) then exit do
		if( http_stream->receive( url, NULL, FALSE ) ) then
			function = http_stream->read( is_binary )
		end if
	loop while FALSE

    delete http_stream

end function

'':::::
function CHttp.getHandle _
	( _
		_
	) as any ptr

	if( ctx = NULL ) then
		return NULL
	end if

	function = ctx->curl

end function

'' end of CHttp.bas
