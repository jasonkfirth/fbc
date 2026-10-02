'' Project: FreeBASIC compiler - semantic source contexts
'' File: tooling/semantic-source.bas
'' Purpose: Preserve actual file revisions and source context occurrences.
'' Responsibilities: Observe open/close, include outcomes, remaps, and origins.
'' This file intentionally does NOT contain: path search, decoding, or parsing.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-source.bi"
#include once "tooling/semantic-source-file.bi"
#include once "tooling/semantic-coordinates.bi"
#include once "parser/parser.bi"
#include once "file.bi"

'' FileAttr(number, 2) returns the runtime's CRT FILE* for ordinary binary
'' source files on every host. It is not the Unix descriptor or Win32 handle
'' returned for serial/printer devices. The revision observer restores the stream
'' position and never owns or closes the compiler's stream.

dim shared as longint semantic_source_ids(0 to FB_MAXINCRECLEVEL)
dim shared as any ptr semantic_source_revisions(0 to FB_MAXINCRECLEVEL)
dim shared as integer semantic_source_handles(0 to FB_MAXINCRECLEVEL)
dim shared as integer semantic_source_formats(0 to FB_MAXINCRECLEVEL)
dim shared as integer semantic_source_regular(0 to FB_MAXINCRECLEVEL)

private function hEncoding(byval format as integer) as string
	select case format
	case FBFILE_FORMAT_ASCII: return "unmarked-bytes"
	case FBFILE_FORMAT_UTF8: return "utf-8-bom"
	case FBFILE_FORMAT_UTF16LE: return "utf-16le"
	case FBFILE_FORMAT_UTF16BE: return "utf-16be"
	case FBFILE_FORMAT_UTF32LE: return "utf-32le"
	case FBFILE_FORMAT_UTF32BE: return "utf-32be"
	case else: return "unknown"
	end select
end function

private function hDirective(byval location as LEX_LOCATION ptr) as string
	if( location = NULL ) then return "0" + TABCHAR + "" + TABCHAR + "0" + TABCHAR + "0" + TABCHAR + "0" + TABCHAR + "0"
	return fbSemanticModelNumber(abs(location->is_physical <> FALSE)) + TABCHAR + _
		fbSemanticModelEscape(location->source_file) + TABCHAR + fbSemanticModelNumber(location->start_line) + TABCHAR + _
		fbSemanticModelNumber(location->start_column) + TABCHAR + fbSemanticModelNumber(location->end_line) + TABCHAR + _
		fbSemanticModelNumber(location->end_column)
end function

sub fbSemanticModelResetSources( )
	'' Every live stream must be observed before the compiler closes it. The
	'' reset does not dereference an old FILE* after a failed parse or restart.
	for depth as integer = 0 to FB_MAXINCRECLEVEL
		if( semantic_source_revisions(depth) <> NULL ) then fbSemanticModelFail( )
		semantic_source_ids(depth) = 0
	next
end sub

function fbSemanticModelCurrentSource( ) as longint
	if( (env.includerec < 0) or (env.includerec > FB_MAXINCRECLEVEL) ) then return 0
	return semantic_source_ids(env.includerec)
end function

function fbSemanticModelSourceHandle( byval identity as longint, byref format as integer ) as integer
	if( identity = 0 ) then return 0
	for depth as integer = 0 to FB_MAXINCRECLEVEL
		if( (semantic_source_ids(depth) = identity) and (semantic_source_revisions(depth) <> NULL) and semantic_source_regular(depth) ) then
			format = semantic_source_formats(depth)
			return semantic_source_handles(depth)
		end if
	next
	return 0
end function

sub fbSemanticModelOpenSource(byref filename as const string, byval depth as integer, _
	byref kind as const string, byref requested as const string, byval directive as LEX_LOCATION ptr)
	if( fbSemanticModelEnabled( ) = FALSE ) then exit sub
	if( (depth < 0) or (depth > FB_MAXINCRECLEVEL) or (semantic_source_revisions(depth) <> NULL) ) then
		fbSemanticModelFail( )
		exit sub
	end if
	dim as zstring * 65 digest
	dim as ulongint bytes = 0
	dim as long status = 0
	dim as any ptr stream = cptr(any ptr, fileattr(env.inf.num, 2))
	semantic_source_revisions(depth) = fbSemanticSourceOpen(stream, @digest, @bytes, @status)
	if( semantic_source_revisions(depth) = NULL ) then
		fbSemanticModelFail( )
		exit sub
	end if
	dim as longint fileid = fbSemanticModelNextDetailIdentity( )
	fbSemanticModelAppendProvenance("FILE" + TABCHAR + fbSemanticModelNumber(fileid) + TABCHAR + _
		fbSemanticModelEscape(filename) + TABCHAR + ltrim(str(bytes)) + TABCHAR + digest + TABCHAR + _
		hEncoding(env.inf.format) + TABCHAR + iif(status = 1, "regular", "stream"))
	dim as longint parentid = 0
	if( depth > 0 ) then parentid = semantic_source_ids(depth - 1)
	semantic_source_ids(depth) = fbSemanticModelNextDetailIdentity( )
	semantic_source_handles(depth) = env.inf.num
	semantic_source_formats(depth) = env.inf.format
	semantic_source_regular(depth) = status = 1
	fbSemanticModelAppendProvenance("SRC" + TABCHAR + fbSemanticModelNumber(semantic_source_ids(depth)) + _
		TABCHAR + fbSemanticModelNumber(parentid) + TABCHAR + fbSemanticModelNumber(fileid) + _
		TABCHAR + fbSemanticModelNumber(fbSemanticModelModuleIdentity( )) + TABCHAR + kind + _
		TABCHAR + fbSemanticModelNumber(depth) + TABCHAR + fbSemanticModelEscape(requested) + _
		TABCHAR + hDirective(directive))
	if( directive <> NULL ) then
		fbSemanticModelExportCoordinates("source-context", semantic_source_ids(depth), "include", *directive, *directive)
	end if
end sub

sub fbSemanticModelCloseSource(byval depth as integer)
	if( (depth < 0) or (depth > FB_MAXINCRECLEVEL) ) then exit sub
	if( semantic_source_revisions(depth) = NULL ) then exit sub
	dim as long status = fbSemanticSourceClose(semantic_source_revisions(depth))
	semantic_source_revisions(depth) = NULL
	fbSemanticModelAppendProvenance("SRE" + TABCHAR + fbSemanticModelNumber(semantic_source_ids(depth)) + _
		TABCHAR + iif(status = 1, "verified", iif(status = 2, "unverified-stream", "changed-or-unreadable")))
	semantic_source_ids(depth) = 0
	semantic_source_handles(depth) = 0
	if( status = 0 ) then fbSemanticModelFail( )
end sub

sub fbSemanticModelIncludeOutcome(byref requested as const string, byref resolved as const string, _
	byref outcome as const string, byval directive as LEX_LOCATION ptr)
	if( fbSemanticModelEnabled( ) = FALSE ) then exit sub
	dim as longint parentid = fbSemanticModelCurrentSource( )
	if( (parentid = 0) and (env.includerec > 0) and (env.includerec <= FB_MAXINCRECLEVEL) ) then
		parentid = semantic_source_ids(env.includerec - 1)
	end if
	dim as longint identity = fbSemanticModelNextDetailIdentity( )
	fbSemanticModelAppendProvenance("INC" + TABCHAR + fbSemanticModelNumber(identity) + _
		TABCHAR + fbSemanticModelNumber(parentid) + TABCHAR + _
		fbSemanticModelEscape(requested) + TABCHAR + fbSemanticModelEscape(resolved) + TABCHAR + outcome + _
		TABCHAR + hDirective(directive))
	if( directive <> NULL ) then fbSemanticModelExportCoordinates("include", identity, "directive", *directive, *directive)
end sub

sub fbSemanticModelSourceRemap(byval logical_line as longint, byref logical_file as const string, _
	byref directive as LEX_LOCATION)
	if( fbSemanticModelEnabled( ) = FALSE ) then exit sub
	'' A line-only remap changes the coordinate domain just as a filename
	'' override does. Matching the original filename does not restore physical
	'' editing eligibility; that requires an independently proved mapping.
	fbSemanticModelMarkSourceRemapped(env.includerec)
	dim as longint identity = fbSemanticModelNextDetailIdentity( )
	fbSemanticModelAppendProvenance("MAP" + TABCHAR + fbSemanticModelNumber(fbSemanticModelCurrentSource( )) + _
		TABCHAR + fbSemanticModelNumber(logical_line) + TABCHAR + fbSemanticModelEscape(logical_file) + _
		TABCHAR + hDirective(@directive) + TABCHAR + fbSemanticModelNumber(identity))
	'' The remap identity survives repeated logical line/file choices.
	fbSemanticModelExportCoordinates("remap", identity, "directive", directive, directive)
end sub

'' end of tooling/semantic-source.bas
