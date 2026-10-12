'' Project: FreeBASIC native compression and XML qualification
'' File: compression-xml-abi.bas
'' Purpose: Describe the corresponding FreeBASIC layouts before SDK calls.
'' Responsibilities: Report sizes and offsets for independent C comparison.
'' This file contains no codec implementation or guessed SDK metadata.
#lang "fb"
#include once "zip.bi"
#include once "expat.bi"
#include once "libxml/xmlreader.bi"
#include once "zlib.bi"
#include once "bzlib.bi"
#include once "lzo/lzo1x.bi"

Print "ZIP"; SizeOf(zip_uint64_t); SizeOf(time_t); SizeOf(zip_stat_t); _
      OffsetOf(zip_stat_t, name); OffsetOf(zip_stat_t, index); OffsetOf(zip_stat_t, mtime); _
      OffsetOf(zip_stat_t, crc); OffsetOf(zip_stat_t, comp_method); OffsetOf(zip_stat_t, flags)
Print "EXPAT"; SizeOf(XML_Char); SizeOf(XML_LChar); SizeOf(XML_Index); SizeOf(XML_Size); _
      SizeOf(XML_Content); OffsetOf(XML_Content, children); SizeOf(XML_ParsingStatus); _
      SizeOf(XML_Memory_Handling_Suite)
Print "XML"; SizeOf(xmlChar); SizeOf(xmlTextReaderPtr); SizeOf(xmlReaderTypes); SizeOf(xmlParserOption)
Print "ZLIB"; SizeOf(uLong_); SizeOf(z_off_t); SizeOf(z_stream); _
      OffsetOf(z_stream, total_in); OffsetOf(z_stream, state); OffsetOf(z_stream, zalloc); _
      OffsetOf(z_stream, adler); OffsetOf(z_stream, reserved)
Print "BZ2"; SizeOf(bz_stream); OffsetOf(bz_stream, next_out); OffsetOf(bz_stream, state); _
      OffsetOf(bz_stream, bzalloc); OffsetOf(bz_stream, opaque)
Print "LZO"; SizeOf(lzo_uint); SizeOf(lzo_int); SizeOf(lzo_align_t); " "; LZO1X_1_15_MEM_COMPRESS

'' end of compression-xml-abi.bas
