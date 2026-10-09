'' Project: FreeBASIC compiler - semantic source contexts
'' File: tooling/semantic-source.bas
'' Purpose: Preserve actual file revisions and source context occurrences.
'' Responsibilities: Observe open/close, include outcomes, remaps, and origins.
'' This file intentionally does NOT contain: path search, decoding, or parsing.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-source.bi"
#include once "tooling/semantic-source-file.bi"
#include once "tooling/semantic-link.bi"
#include once "tooling/semantic-macros.bi"
#include once "tooling/semantic-coordinates.bi"
#include once "tooling/semantic-constructs.bi"
#include once "tooling/semantic-preprocessor.bi"
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
'' Policy counts belong to one source occurrence, not a pathname. Reopening a
'' file gets a fresh receipt even when the preprocessor skips its guarded body.
dim shared as integer semantic_source_once(0 to FB_MAXINCRECLEVEL)
dim shared as longint semantic_source_libraries(0 to FB_MAXINCRECLEVEL)
dim shared as longint semantic_source_imports(0 to FB_MAXINCRECLEVEL)

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

private function hDirective(byval location as LEX_LOCATION ptr, byval require_physical as integer) as string
	if( (location = NULL) orelse ((require_physical) and (location->is_physical = FALSE)) ) then
		'' Include context ranges require a physical directive origin.
		return "0" + TABCHAR + "" + TABCHAR + "0" + TABCHAR + "0" + TABCHAR + "0" + TABCHAR + "0"
	end if
	return fbSemanticModelNumber(abs(location->is_physical <> FALSE)) + TABCHAR + _
		fbSemanticModelEscape(location->source_file) + TABCHAR + fbSemanticModelNumber(location->start_line) + TABCHAR + _
		fbSemanticModelNumber(location->start_column) + TABCHAR + fbSemanticModelNumber(location->end_line) + TABCHAR + _
		fbSemanticModelNumber(location->end_column)
end function

sub fbSemanticModelResetSources( )
	'' Every live stream must be observed before the compiler closes it. The
	'' reset does not dereference an old FILE* after a failed parse or restart.
	for depth as integer = 0 to FB_MAXINCRECLEVEL
		if( semantic_source_revisions(depth) <> NULL ) then fbSemanticModelFailAt("semantic-source.bas:49")
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
	fbSemanticLinkSourceOpen(filename, depth)
	if( fbSemanticModelEnabled( ) = FALSE ) then exit sub
	if( (depth < 0) or (depth > FB_MAXINCRECLEVEL) or (semantic_source_revisions(depth) <> NULL) ) then
		fbSemanticModelFailAt("semantic-source.bas:74")
		exit sub
	end if
	dim as zstring * 65 digest
	dim as ulongint bytes = 0
	dim as long status = 0
	dim as any ptr stream = cptr(any ptr, fileattr(env.inf.num, 2))
	semantic_source_revisions(depth) = fbSemanticSourceOpen(stream, @digest, @bytes, @status)
	if( semantic_source_revisions(depth) = NULL ) then
		fbSemanticModelFailAt("semantic-source.bas:83")
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
	semantic_source_once(depth) = FALSE
	semantic_source_libraries(depth) = 0
	semantic_source_imports(depth) = 0
	fbSemanticModelAppendProvenance("SRC" + TABCHAR + fbSemanticModelNumber(semantic_source_ids(depth)) + _
		TABCHAR + fbSemanticModelNumber(parentid) + TABCHAR + fbSemanticModelNumber(fileid) + _
		TABCHAR + fbSemanticModelNumber(fbSemanticModelModuleIdentity( )) + TABCHAR + kind + _
		TABCHAR + fbSemanticModelNumber(depth) + TABCHAR + fbSemanticModelEscape(requested) + _
		TABCHAR + hDirective(directive, TRUE))
	if( directive <> NULL ) then
		fbSemanticModelExportCoordinates("source-context", semantic_source_ids(depth), "include", *directive, *directive)
	end if
end sub

sub fbSemanticModelCloseSource(byval depth as integer)
	fbSemanticLinkSourceClose(depth)
	if( (depth < 0) or (depth > FB_MAXINCRECLEVEL) ) then exit sub
	if( semantic_source_revisions(depth) = NULL ) then exit sub
	dim as long status = fbSemanticSourceClose(semantic_source_revisions(depth))
	semantic_source_revisions(depth) = NULL
	if( fbSemanticModelFullEnabled( ) ) then
		fbSemanticModelAppendDetail("K" + TABCHAR + "symbol" + TABCHAR + _
			fbSemanticModelNumber(fbSemanticModelSymbolId(@symbGetGlobalNamespc( ))) + TABCHAR + _
			"header-policy:" + fbSemanticModelNumber(semantic_source_ids(depth)) + TABCHAR + _
			fbSemanticModelEscape(fbSemanticModelNumber(abs(semantic_source_once(depth) <> FALSE)) + TABCHAR + _
				fbSemanticModelNumber(semantic_source_libraries(depth)) + TABCHAR + _
				fbSemanticModelNumber(semantic_source_imports(depth))))
	end if
	fbSemanticModelAppendProvenance("SRE" + TABCHAR + fbSemanticModelNumber(semantic_source_ids(depth)) + _
		TABCHAR + iif(status = 1, "verified", iif(status = 2, "unverified-stream", "changed-or-unreadable")))
	semantic_source_ids(depth) = 0
	semantic_source_handles(depth) = 0
	if( status = 0 ) then fbSemanticModelFailAt("semantic-source.bas:115")
end sub

'' -------------------------------------------------------------------------
'' Accepted header policies and preprocessor library inputs
'' -------------------------------------------------------------------------

sub fbSemanticModelSourcePolicyInput _
	( byref kind as const string, byref value as const string, byval source as LEX_LOCATION ptr )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	dim as integer depth = env.includerec
	dim as longint sourceid = fbSemanticModelCurrentSource( )
	if( (sourceid = 0) or (depth < 0) or (depth > FB_MAXINCRECLEVEL) ) then
		fbSemanticModelFailAt("semantic-source.bas:header-policy")
		exit sub
	end if
	select case kind
	case "once"
		semantic_source_once(depth) = TRUE
	case "namespace-import"
		dim as longint statementid = fbSemanticModelCurrentStatement( )
		dim as longint recipient = fbSemanticModelSymbolId(symbGetCurrentNamespc( ))
		if( (statementid = 0) or (recipient = 0) ) then
			fbSemanticModelFailAt("semantic-source.bas:import-recipient")
			exit sub
		end if
		semantic_source_imports(depth) += 1
		'' USING changes the current namespace even inside a procedure. Keep
		'' its actual recipient rather than guessing scope from its statement.
		dim as string payload = fbSemanticModelNumber(sourceid) + TABCHAR + fbSemanticModelNumber(recipient)
		fbSemanticModelAppendDetail("K" + TABCHAR + "symbol" + TABCHAR + _
			fbSemanticModelNumber(fbSemanticModelSymbolId(@symbGetGlobalNamespc( ))) + TABCHAR + _
			"header-import:" + fbSemanticModelNumber(statementid) + TABCHAR + fbSemanticModelEscape(payload))
	case "inclib", "libpath"
		if( source = NULL ) then
			fbSemanticModelFailAt("semantic-source.bas:library-location")
			exit sub
		end if
		semantic_source_libraries(depth) += 1
		dim as string role = "header-library:" + fbSemanticModelNumber(fbSemanticModelNextDetailIdentity( ))
		dim as longint owner = fbSemanticModelSymbolId(@symbGetGlobalNamespc( ))
		'' Escape the nested string separately: an accepted library path can
		'' itself contain a tab, newline or percent character.
		dim as string payload = fbSemanticModelNumber(sourceid) + TABCHAR + kind + TABCHAR + _
			fbSemanticModelEscape(value) + TABCHAR + fbSemanticModelNumber(fbSemanticModelPPCurrentBranch( ))
		fbSemanticModelAppendDetail("K" + TABCHAR + "symbol" + TABCHAR + fbSemanticModelNumber(owner) + _
			TABCHAR + role + TABCHAR + fbSemanticModelEscape(payload))
		'' Physical coordinates belong to the include occurrence; macro
		'' origins attach to the property owner through the symbol contract.
		fbSemanticModelExportCoordinates("source-context", sourceid, role, *source, *source)
		fbSemanticModelMacroOrigin("symbol", owner, source->macro_identity, role)
	case else
		fbSemanticModelFailAt("semantic-source.bas:unknown-policy")
	end select
end sub

sub fbSemanticModelIncludeOutcome(byref requested as const string, byref resolved as const string, _
	byref outcome as const string, byval directive as LEX_LOCATION ptr)
	if( fbSemanticModelEnabled( ) = FALSE ) then exit sub
	dim as longint parentid = fbSemanticModelCurrentSource( )
	if( (parentid = 0) and (env.includerec > 0) and (env.includerec <= FB_MAXINCRECLEVEL) ) then
		parentid = semantic_source_ids(env.includerec - 1)
	end if
	dim as longint identity = fbSemanticModelNextDetailIdentity( )
	dim as LEX_LOCATION invocation
	dim as LEX_LOCATION ptr observation = directive
	dim as integer require_physical = TRUE
	if( directive <> NULL ) then
		if( (directive->is_physical = FALSE) and (directive->macro_identity <> 0) ) then
			'' INC can retain a noneditable invocation anchor without claiming
			'' that replacement text is a written include filename range.
			if( fbSemanticModelMacroExpressionLocation(*directive, *directive, 0, 0, invocation) ) then
				observation = @invocation
				require_physical = FALSE
			end if
		end if
	end if
	fbSemanticModelAppendProvenance("INC" + TABCHAR + fbSemanticModelNumber(identity) + _
		TABCHAR + fbSemanticModelNumber(parentid) + TABCHAR + _
		fbSemanticModelEscape(requested) + TABCHAR + fbSemanticModelEscape(resolved) + TABCHAR + outcome + _
		TABCHAR + hDirective(observation, require_physical))
	if( directive <> NULL ) then fbSemanticModelExportCoordinates("include", identity, "directive", *directive, *directive)
end sub

sub fbSemanticModelSourceRemap(byval logical_line as longint, byref logical_file as const string, _
	byref directive as LEX_LOCATION)
	fbSemanticLinkSourceRemapped(env.includerec)
	if( fbSemanticModelEnabled( ) = FALSE ) then exit sub
	'' A line-only remap changes the coordinate domain just as a filename
	'' override does. Matching the original filename does not restore physical
	'' editing eligibility; that requires an independently proved mapping.
	fbSemanticModelMarkSourceRemapped(env.includerec)
	dim as longint identity = fbSemanticModelNextDetailIdentity( )
	'' Keep the written #line range in the receipt even when its physical flag is false.
	fbSemanticModelAppendProvenance("MAP" + TABCHAR + fbSemanticModelNumber(fbSemanticModelCurrentSource( )) + _
		TABCHAR + fbSemanticModelNumber(logical_line) + TABCHAR + fbSemanticModelEscape(logical_file) + _
		TABCHAR + hDirective(@directive, FALSE) + TABCHAR + fbSemanticModelNumber(identity))
	'' The remap identity survives repeated logical line/file choices.
	fbSemanticModelExportCoordinates("remap", identity, "directive", directive, directive)
end sub

'' end of tooling/semantic-source.bas
