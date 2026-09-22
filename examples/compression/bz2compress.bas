''
'' Project: FreeBASIC examples
'' --------------------------
''
'' File: bz2compress.bas
''
'' Purpose:
''
''     Compress one file through the bzip2 streaming API.
''
'' Responsibilities:
''
''     - load the source file into caller-owned storage
''     - grow the compressed output buffer safely
''     - release owned storage after each success or failure path
''
'' Ownership:
''
''     hLoadFile and hCompressWithBz2 transfer one allocated buffer to the
''     caller on success.  The module entry point releases those buffers on
''     every path after it acquires them.
''
'' This file intentionally does NOT contain:
''
''     - bzip2 decompression support
''     - command-line option parsing
''

#include once "bzlib.bi"

'' Reads in a file's content into a buffer owned by the caller.
private function hLoadFile _
	( _
		byref filename as string, _
		byref buffer as byte ptr, _
		byref size as integer _
	) as integer

	dim f as integer

	buffer = 0
	size = 0

	print "loading file '" + filename + "'... ";

	f = freefile( )
	if( open( filename, for binary, access read, as #f ) <> 0 ) then
		print "error: could not open"
		return -1
	end if

	size = lof( f )
	if( size < 0 ) then
		close #f
		print "error: invalid file size"
		return -1
	end if

	if( size > 0 ) then
		buffer = callocate( size )
		if( buffer = 0 ) then
			close #f
			print "error: could not allocate input buffer"
			return -1
		end if

		get #f, , *buffer, size
	end if

	close #f

	print size & " bytes"

	return 0
end function

'' Creates a new file and writes a caller-owned buffer into it.
private function hWriteFile _
	( _
		byref filename as string, _
		byval p as byte ptr, _
		byval size as integer _
	) as integer

	dim f as integer

	if( size < 0 ) then
		print "error: invalid output size"
		return -1
	end if

	if( p = 0 ) then
		print "error: missing output buffer"
		return -1
	end if

	print "writing file '" + filename + "'... ";

	f = freefile( )
	if( open( filename, for binary, access write, as #f ) <> 0 ) then
		print "error: could not create/overwrite file"
		return -1
	end if

	if( size > 0 ) then
		put #f, , *p, size
	end if

	close #f

	print size & " bytes"

	return 0
end function

private function hCompressWithBz2 _
	( _
		byval buffer_in as byte ptr, _
		byval size_in as integer, _
		byref buffer_out as byte ptr, _
		byref size_out as integer _
	) as integer

	dim bz as bz_stream
	dim action as integer
	dim result as integer
	dim resized_buffer as byte ptr
	const output_growth as integer = 512

	buffer_out = 0
	size_out = 0

	if( size_in < 0 ) then
		print "error: invalid input size"
		return -1
	end if

	if( size_in > 0 andalso buffer_in = 0 ) then
		print "error: missing input buffer"
		return -1
	end if

	print "compressing " & size_in & " bytes... ";

	if( BZ2_bzCompressInit( @bz, 9, 0, 0 ) <> BZ_OK ) then
		print "error: could not initialize bzip2"
		return -1
	end if

	'print "---"
	bz.next_in = buffer_in
	bz.avail_in = size_in
	do
		if( bz.avail_in > 0 ) then
			action = BZ_RUN
		else
			action = BZ_FINISH
		end if

		if( bz.total_out_lo32 >= size_out ) then
			' Keep the fixed growth amount from the original example, but
			' reject arithmetic overflow before passing a size to Reallocate.
			if( size_out > &h7fffffff - output_growth ) then
				BZ2_bzCompressEnd( @bz )
				print "error: compressed output is too large"
				return -1
			end if

			resized_buffer = reallocate( buffer_out, size_out + output_growth )
			if( resized_buffer = 0 ) then
				BZ2_bzCompressEnd( @bz )
				print "error: could not allocate output buffer"
				return -1
			end if

			buffer_out = resized_buffer
			size_out += output_growth
		end if

		bz.next_out = buffer_out + bz.total_out_lo32
		bz.avail_out = size_out - bz.total_out_lo32

		'print bz.avail_in & " avail_in, " & bz.avail_out & " avail_out, " & bz.total_out_lo32 & " total_out"

		result = BZ2_bzCompress( @bz, action )
		if( result = BZ_STREAM_END ) then
			exit do
		end if

		if( result <> BZ_RUN_OK andalso result <> BZ_FINISH_OK ) then
			BZ2_bzCompressEnd( @bz )
			deallocate( buffer_out )
			buffer_out = 0
			size_out = 0
			print "error: bzip2 compression failed"
			return -1
		end if
	loop
	'print "---"

	BZ2_bzCompressEnd( @bz )

	size_out = bz.total_out_lo32
	print size_out & " bytes"

	return 0
end function

	dim as string filename

	'' File to compress
	'filename = __FILE__
	filename = "zlib.bas"

	dim as byte ptr buffer_in, buffer_out
	dim as integer size_in, size_out

	dim as integer status

	status = hLoadFile( filename, buffer_in, size_in )
	if( status = 0 ) then
		status = hCompressWithBz2( buffer_in, size_in, buffer_out, size_out )
	end if
	if( status = 0 ) then
		status = hWriteFile( filename + ".bz2", buffer_out, size_out )
	end if

	if( buffer_in <> 0 ) then
		deallocate( buffer_in )
		buffer_in = 0
	end if
	if( buffer_out <> 0 ) then
		deallocate( buffer_out )
		buffer_out = 0
	end if

	if( status <> 0 ) then
		end 1
	end if

'' end of bz2compress.bas
