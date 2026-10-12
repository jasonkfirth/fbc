'' Project: FreeBASIC curl examples
'' File: CHttpStream.bi
'' Purpose: Declare the synchronous CHttp body stream.
'' Responsibilities: Describe receive, exact binary reads and borrowed-buffer uploads.
'' This file contains no callback implementation or libcurl declarations.
''
'' One thread owns each stream. If a connection is supplied, it must outlive
'' the stream. These owning objects must not be copied.

#ifndef __CHTTPSTREAM_BI__
#define __CHTTPSTREAM_BI__

type CHttpStreamCtx as CHttpStreamCtx_

type CHttpStream
	declare constructor	_
		( _
			byval http as CHttp ptr = NULL _
		)

	declare destructor _
		( _
			_
		)

	declare function receive _
		( _
			byval url as zstring ptr, _
			byval referer as zstring ptr = NULL, _
			byval doreset as integer = TRUE _
		) as integer


	'' Text reads stop at the first zero; binary reads return exactly the body.
	declare function read _
		( _
			byval is_binary as integer = FALSE _
		) as string

	'' send() uploads bytes synchronously (PUT for HTTP), including an empty
	'' body. It borrows data_ until return, owns the response, and supports
	'' rewind after a redirect. read() can retrieve the upload response.
	declare function send _
		( _
			byval url as zstring ptr, _
			byval data_ as any ptr, _
			byval bytes as integer, _
			byval referer as zstring ptr = NULL, _
			byval doreset as integer = TRUE _
		) as integer

private:
	dim as CHttpStreamCtx ptr ctx = any
end type

#endif

'' end of CHttpStream.bi
