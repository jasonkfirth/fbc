''
'' Small tool to compress the font and palette data files for gfxlib2 and
'' write the resulting data bytes to inline.h to be compiled into gfxlib2.
'' Run/adjust this to update inline.h as needed.
''
'' Ownership: input buffers are released after combining, the combined buffer
'' is released after compression, and the compressed buffer is released after
'' the generated header has been written.
''
'' best compiled with -g -exx to catch assertions and file I/O errors
''

#define NULL 0

#include once "crt.bi"
#include once "fbc-int/fbcall.bi"

'' INTEGER is native-sized here to match the C ssize_t parameters.
'' fblint: disable-next-line FBL-ABI-004
declare function fb_hEncode FBCALL lib "fbgfx" alias "fb_hEncode" _
	( _
		byval as const ubyte ptr, _
		byval as integer, _
		byval as ubyte ptr, _
		byval as integer ptr _
	) as integer  '' fblint: disable-line FBL320

type Entry
	as zstring * 16 name
	as zstring * 16 file
	as integer expected_size
	as ubyte ptr p
	as integer size
end type

dim shared as Entry entries(0 to ...) = _
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
		'' fblint: disable-next-line FBL-DOC-BIN-003 FBL-IO-009
		if( open( filename, for binary, access read, as #f ) <> 0 ) then
			print "Error: could not open input file '" + filename + "'"
			end 1
		end if

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
		if( get( #f, , *.p, .size, bytes_read ) <> 0 ) or (bytes_read <> .size) then
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
if( fb_hEncode( raw, rawsize, compressed, @compressedsize ) <> 0 ) or _
   (compressedsize <= 0) or (compressedsize > rawsize) then
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

dim as string ccode
ccode += !"/* Automatically created by makedata, to be used by data.c */\n"
ccode += !"/* Compressed internal font/palette data for FB graphics */\n"
ccode += !"\n"

'' Emit all the offset #defines
scope
	dim as integer offset = 0
	for i as integer = 0 to ubound( entries )
		with( entries(i) )
			'' The generated header is bounded to the seven fixed-size input records.
			'' fblint: disable-next-line FBL503
			ccode += "#define DATA_" + .name + " 0x" + hex( offset, 8 ) + !"\n"
			offset += .size
		end with
	next
end scope

'' Emit the compressed data
ccode += !"\nstatic const unsigned char compressed_data[] = {\n"

for i as integer = 0 to compressedsize - 1
	if( (i mod 16) = 0 ) then
		'' The generated header is bounded to the seven fixed-size input records.
		'' fblint: disable-next-line FBL503
		ccode += "    "
	end if

	ccode += "0x" + hex( compressed[i], 2 )

	if( i < (compressedsize - 1) ) then
		ccode += ","
		'' Emit a newline every 16 bytes
		if( ((i + 1) mod 16) = 0 ) then
			ccode += !"\n"
		else
			ccode += " "
		end if
	else
		ccode += !"\n"
	end if
next

ccode += !"};\n\n"

ccode += !"#define DATA_SIZE " & rawsize & !"\n"

'' Write out the emitted C code into the output file
scope
	dim as integer f = freefile( )
	dim as string output_path = exepath( ) + path_separator + ".." + _
	                             path_separator + "gfxdata_inline.h"
	'' This generator intentionally replaces the checked-in generated header.
	'' fblint: disable-next-line FBL103 FBL-IO-005
	if( open( output_path, for output, as #f ) <> 0 ) then
		deallocate( compressed )
		compressed = NULL
		print "Error: could not open output file '" + output_path + "'"
		end 1
	end if
	print #f, ccode;
	close #f
end scope

deallocate( compressed )
compressed = NULL

print rawsize & " bytes in, " & compressedsize & " bytes out " & _
		"(" & (rawsize / compressedsize) & ":1 ratio)"
