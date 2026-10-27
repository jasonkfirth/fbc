''
/'
    Project: FreeBASIC gfxlib2
    -------------------------

    File: gfxdata/makedata.bas

    Purpose:

        Compress the fixed font and palette records into the C header included
        by the graphics runtime.

    Responsibilities:

        - validate and load the seven source records
        - combine and compress their bytes
        - emit offsets, compressed data and the total size

    This file intentionally does NOT contain:

        - runtime graphics operations or font decoding
        - platform driver selection

    Targets: host build tool linked to libfbgfx for the same target ABI.
    Module API: standalone executable; its only external ABI is fb_hEncode.

    Ownership: input buffers are released after combining, the combined buffer
    is released after compression, and the compressed buffer is released after
    the generated header has been written.

    Build with -g -exx to catch assertions and file I/O errors.
'/
''

#define NULL 0

#include once "crt.bi"
#include once "fbc-int/fbcall.bi"

'' This reserve covers the generated header text outside the byte payload.
const GENERATED_HEADER_FIXED_RESERVE = 1024

private sub appendCCCode( _
	byref output_buffer as string, _
	byref output_position as integer, _
	byref output_fits as integer, _
	byval append_value as string _
)
	dim as integer text_size = len(append_value)
	if( (output_fits = FALSE) orelse (text_size = 0) ) then exit sub

	if( (output_position < 1) orelse _
	    (text_size > (len(output_buffer) - output_position + 1)) ) then
		output_fits = FALSE
		exit sub
	end if

	mid(output_buffer, output_position, text_size) = append_value
	output_position += text_size
end sub

'' ABI: compile and link this tool for the same target width as libfbgfx.
'' Its C entry point uses ssize_t; native-sized INTEGER preserves that ABI.
declare function fb_hEncode FBCALL lib "fbgfx" alias "fb_hEncode" _
	( _
		byval as const ubyte ptr, _
		byval as integer, _
		byval as ubyte ptr, _
		byval as integer ptr _
	) as integer

type Entry
	as zstring * 16 name
	as zstring * 16 file
	as integer expected_size
	as ubyte ptr p
	as integer size
end type

dim as Entry entries(0 to ...) = _
{ _
	( "FONT_8" , "fnt08x08.fnt", 2048 ), _
	( "FONT_14", "fnt08x14.fnt", 3584 ), _
	( "FONT_16", "fnt08x16.fnt", 4096 ), _
	( "PAL_2"  , "pal002.pal"  ,    6 ), _
	( "PAL_16" , "pal016.pal"  ,   48 ), _
	( "PAL_64" , "pal064.pal"  ,  192 ), _
	( "PAL_256", "pal256.pal"  ,  768 )  _
}

#if defined( __FB_WIN32__ ) or defined( __FB_DOS__ )
	const path_separator = "\"
#else
	const path_separator = "/"
#endif

'' Load data files
for i as integer = 0 to ubound( entries )
	with( entries(i) )
		dim as string filename = exepath( ) + path_separator + .file

		dim as integer f = freefile()
		'' These are headerless fixed-size raw font and RGB palette records.
		'' fblint: disable-next-line FBL-IO-009
		if( open( filename, for binary, access read, as #f ) <> 0 ) then
			print "Error: could not open input file '" + filename + "'"
			end 1
		end if

		'' Require the exact small record size before converting the wide LOF result.
		'' fblint: disable-next-line FBL423 REASON: LongInt is fixed-width; equality bounds the later CInt.
		dim as longint file_size = lof( f )
		if( file_size <> .expected_size ) then
			close #f
			print "Error: unexpected size for '" + filename + "': " & file_size & _
			      " bytes, expected " & .expected_size
			end 1
		end if
		.size = cint( file_size )
		.p = allocate( .size )
		if( .p = NULL ) then
			close #f
			print "Error: could not allocate " & .size & " bytes for '" + filename + "'"
			end 1
		end if

		dim as integer bytes_read = 0
		if( get( #f, , *.p, .size, bytes_read ) <> 0 ) orelse _
		   (bytes_read <> .size) then
			deallocate( .p )
			.p = NULL
			close #f
			print "Error: could not read all data from '" + filename + "'"
			end 1
		end if

		print "loading: '" + filename + "' (" & .size & " bytes)"

		close #f
	end with
next

'' Get size of all entries combined
dim as integer rawsize = 0
for i as integer = 0 to ubound( entries )
	rawsize += entries(i).size
next

'' Combine all the data files into one big buffer
dim as ubyte ptr raw = allocate( rawsize )
if( raw = NULL ) then
	print "Error: could not allocate the " & rawsize & " byte combined data buffer"
	end 1
end if
scope
	dim as integer offset = 0
	for i as integer = 0 to ubound( entries )
		with( entries(i) )
			memcpy( raw + offset, .p, .size )
			offset += .size
			deallocate( .p )
			.p = NULL
		end with
	next
end scope

'' Use this to store the raw data into a file
#if 0
scope
	dim as integer f = freefile( )
	open exepath( ) + "/data.dat" for binary access write as #f
	put #f, , *raw, rawsize
	close #f
end scope
#endif

'' Compress the data
dim as integer compressedsize = rawsize
dim as ubyte ptr compressed = allocate( rawsize )
if( compressed = NULL ) then
	deallocate( raw )
	raw = NULL
	print "Error: could not allocate the " & rawsize & " byte compression buffer"
	end 1
end if
if( fb_hEncode( raw, rawsize, compressed, @compressedsize ) <> 0 ) orelse _
	   (compressedsize <= 0) orelse (compressedsize > rawsize) then
	deallocate( compressed )
	compressed = NULL
	deallocate( raw )
	raw = NULL
	print "Error: font and palette compression failed"
	end 1
end if
deallocate( raw )
raw = NULL

print rawsize, compressedsize

'' Each byte needs six output characters; each 16-byte row also needs indentation.
dim as integer ccode_capacity = GENERATED_HEADER_FIXED_RESERVE + _
		compressedsize * 6 + ((compressedsize + 15) \ 16) * 4
dim as string ccode = space(ccode_capacity)
dim as integer ccode_position = 1
dim as integer ccode_fits = TRUE

if( len(ccode) <> ccode_capacity ) then
	deallocate( compressed )
	compressed = NULL
	print "Error: could not allocate the generated header buffer"
	end 1
end if

appendCCCode( ccode, ccode_position, ccode_fits, !"/*\n" )
appendCCCode( ccode, ccode_position, ccode_fits, _
		!"    Project: FreeBASIC gfxlib2\n" )
appendCCCode( ccode, ccode_position, ccode_fits, _
		!"    File: gfxdata_inline.h\n" )
appendCCCode( ccode, ccode_position, ccode_fits, _
		!"    Purpose: Store compressed font and palette resources.\n" )
appendCCCode( ccode, ccode_position, ccode_fits, _
		!"    Responsibilities: Define offsets, bytes and total size.\n" )
appendCCCode( ccode, ccode_position, ccode_fits, _
		!"    This file intentionally does NOT contain compression or rendering code.\n" )
appendCCCode( ccode, ccode_position, ccode_fits, _
		!"    Generated by gfxdata/makedata.bas; edit the inputs and rebuild.\n" )
appendCCCode( ccode, ccode_position, ccode_fits, !"*/\n\n" )

'' Emit all the offset #defines
scope
	dim as integer offset = 0
	for i as integer = 0 to ubound( entries )
		with( entries(i) )
			appendCCCode( ccode, ccode_position, ccode_fits, _
				"#define DATA_" + .name + " 0x" + hex( offset, 8 ) + !"\n" )
			offset += .size
		end with
	next
end scope

'' Emit the compressed data
appendCCCode( ccode, ccode_position, ccode_fits, _
		!"\nstatic const unsigned char compressed_data[] = {\n" )

for i as integer = 0 to compressedsize - 1
	if( (i mod 16) = 0 ) then
		appendCCCode( ccode, ccode_position, ccode_fits, "    " )
	end if

	appendCCCode( ccode, ccode_position, ccode_fits, _
		"0x" + hex( compressed[i], 2 ) )

	if( i < (compressedsize - 1) ) then
		appendCCCode( ccode, ccode_position, ccode_fits, "," )
		'' Emit a newline every 16 bytes
		if( ((i + 1) mod 16) = 0 ) then
			appendCCCode( ccode, ccode_position, ccode_fits, !"\n" )
		else
			appendCCCode( ccode, ccode_position, ccode_fits, " " )
		end if
	else
		appendCCCode( ccode, ccode_position, ccode_fits, !"\n" )
	end if
next

appendCCCode( ccode, ccode_position, ccode_fits, !"};\n\n" )

appendCCCode( ccode, ccode_position, ccode_fits, _
		!"#define DATA_SIZE " & rawsize & !"\n" )
appendCCCode( ccode, ccode_position, ccode_fits, _
		!"\n/* end of gfxdata_inline.h */\n" )

if( ccode_fits = FALSE ) then
	deallocate( compressed )
	compressed = NULL
	print "Error: generated header exceeded its checked buffer size"
	end 1
end if

'' Write out the emitted C code into the output file
scope
	dim as integer f = freefile( )
	dim as string output_path = exepath( ) + path_separator + ".." + _
	                             path_separator + "gfxdata_inline.h"
	'' This checked-in generated header is rebuilt from fixed source records.
	'' fblint: disable-next-line FBL-IO-005 REASON: generated source is regenerated from checked-in data.
	if( open( output_path, for output, as #f ) <> 0 ) then
		deallocate( compressed )
		compressed = NULL
		print "Error: could not open output file '" + output_path + "'"
		end 1
	end if
	print #f, left( ccode, ccode_position - 1 );
	close #f
end scope

deallocate( compressed )
compressed = NULL

print rawsize & " bytes in, " & compressedsize & " bytes out " & _
		"(" & (rawsize / compressedsize) & ":1 ratio)"

'' end of makedata.bas
