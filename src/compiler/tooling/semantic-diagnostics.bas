'' Project: FreeBASIC compiler - structured semantic diagnostics
'' File: tooling/semantic-diagnostics.bas
'' Purpose: Retain diagnostics actually reported by the compiler.
'' Responsibilities: Preserve severity, code, text, source, and parser context.
'' This file intentionally does NOT contain: diagnostic policy or console parsing.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-source.bi"
#include once "tooling/semantic-constructs.bi"
#include once "tooling/semantic-diagnostics.bi"
#include once "tooling/semantic-output.bi"
#include once "parser/parser.bi"
#include once "diagnostics/error.bi"

'' -------------------------------------------------------------------------
'' Invocation-owned diagnostic output
'' -------------------------------------------------------------------------

'' A rejected module cannot publish authoritative AST facts. Diagnostics are
'' observations of the compiler's actual checks and can still be complete.
'' This independent writer is bounded and uses the semantic writer's checked
'' staging, source identity protection and final publication.
private const DIAGNOSTIC_MAX_SOURCES = 5000
private const DIAGNOSTIC_MAX_RECORDS = 100000
private const DIAGNOSTIC_MAX_BYTES = 67108864
private dim shared as any ptr diagnostic_output
private dim shared as integer diagnostic_failed, diagnostic_modules
private dim shared as integer diagnostic_count, diagnostic_errors, diagnostic_bytes
private dim shared as integer diagnostic_source_count
private dim shared as string diagnostic_sources(0 to DIAGNOSTIC_MAX_SOURCES - 1)
private dim shared as string diagnostic_context

private sub hDiagnosticWrite( byref record_text as const string )
	if( (diagnostic_output = NULL) or diagnostic_failed ) then exit sub
	if( len(record_text) >= DIAGNOSTIC_MAX_BYTES - diagnostic_bytes ) then
		diagnostic_failed = TRUE
		exit sub
	end if
	dim as string line_text = record_text + NEWLINE
	if( fbSemanticOutputWrite(diagnostic_output, strptr(line_text), len(line_text)) = 0 ) then
		diagnostic_failed = TRUE
	else
		diagnostic_bytes += len(line_text)
	end if
end sub

sub fbSemanticDiagnosticsProtectFile( byref filename as const string )
	if( (diagnostic_output = NULL) or (len(filename) = 0) ) then exit sub
	if( fbSemanticOutputProtect(diagnostic_output, strptr(filename)) = 0 ) then diagnostic_failed = TRUE
end sub

sub fbSemanticDiagnosticsSource( byref filename as const string )
	if( (diagnostic_output = NULL) or diagnostic_failed or (len(filename) = 0) ) then exit sub
	fbSemanticDiagnosticsProtectFile(filename)
	for index as integer = 0 to diagnostic_source_count - 1
		if( diagnostic_sources(index) = filename ) then exit sub
	next
	if( diagnostic_source_count >= DIAGNOSTIC_MAX_SOURCES ) then
		diagnostic_failed = TRUE
		exit sub
	end if
	diagnostic_sources(diagnostic_source_count) = filename
	diagnostic_source_count += 1
	hDiagnosticWrite("SRC" + TABCHAR + fbSemanticModelNumber(diagnostic_source_count) + TABCHAR + fbSemanticModelEscape(filename))
end sub

sub fbSemanticDiagnosticsModule( byref filename as const string )
	if( (diagnostic_output = NULL) or diagnostic_failed ) then exit sub
	if( diagnostic_modules >= DIAGNOSTIC_MAX_RECORDS ) then
		diagnostic_failed = TRUE
		exit sub
	end if
	diagnostic_modules += 1
	diagnostic_context = ""
	fbSemanticDiagnosticsSource(filename)
	hDiagnosticWrite("M" + TABCHAR + fbSemanticModelNumber(diagnostic_modules) + TABCHAR + _
		fbSemanticModelEscape(filename) + TABCHAR + fbSemanticModelEscape(fbGetTargetId()))
end sub

sub fbSemanticDiagnosticsContext( byref context_text as const string )
	if( diagnostic_output <> NULL ) then diagnostic_context = context_text
end sub

function fbSemanticDiagnosticsBegin( byref filename as const string ) as integer
	if( diagnostic_output <> NULL ) then
		if( fbSemanticOutputFinish(diagnostic_output, FALSE) = 0 ) then
			diagnostic_output = NULL
			return FALSE
		end if
	end if
	diagnostic_output = NULL
	for index as integer = 0 to diagnostic_source_count - 1
		diagnostic_sources(index) = ""
	next
	diagnostic_source_count = 0
	diagnostic_modules = 0
	diagnostic_count = 0
	diagnostic_errors = 0
	diagnostic_bytes = 0
	diagnostic_context = ""
	diagnostic_failed = FALSE
	diagnostic_output = fbSemanticOutputOpen(strptr(filename))
	if( diagnostic_output = NULL ) then return FALSE
	hDiagnosticWrite("FBCDIA" + TABCHAR + "1" + TABCHAR + FB_VERSION)
	return diagnostic_failed = FALSE
end function

function fbSemanticDiagnosticsEnd( byval succeeded as integer ) as integer
	if( diagnostic_output = NULL ) then return TRUE
	hDiagnosticWrite("END" + TABCHAR + "1" + TABCHAR + fbSemanticModelNumber(diagnostic_modules) + _
		TABCHAR + fbSemanticModelNumber(diagnostic_source_count) + TABCHAR + fbSemanticModelNumber(diagnostic_count) + _
		TABCHAR + fbSemanticModelNumber(diagnostic_errors) + TABCHAR + fbSemanticModelNumber(abs(succeeded <> FALSE)))
	dim as integer published = fbSemanticOutputFinish(diagnostic_output, diagnostic_failed = FALSE)
	diagnostic_output = NULL
	for index as integer = 0 to diagnostic_source_count - 1
		diagnostic_sources(index) = ""
	next
	diagnostic_source_count = 0
	diagnostic_context = ""
	return (published <> 0) and (diagnostic_failed = FALSE)
end function

private sub hDiagnosticObserve _
	( byref severity as const string, byval code as integer, byval line_number as integer, _
	  byref message_text as const string, byref detail_text as const string, byref custom_text as const string )
	if( (diagnostic_output = NULL) or diagnostic_failed ) then exit sub
	if( diagnostic_count >= DIAGNOSTIC_MAX_RECORDS ) then
		diagnostic_failed = TRUE
		exit sub
	end if
	diagnostic_count += 1
	dim as string kind_text
	if( severity = "error" ) then
		diagnostic_errors += 1
		kind_text = diagnostic_context
		if( code = FB_ERRMSG_FORNEXTVARIABLEMISMATCH ) then kind_text = "for-next-variable-mismatch"
	end if
	fbSemanticDiagnosticsSource(env.inf.name)
	dim as LEX_LOCATION token_point = lexGetLastLocation( )
	dim as integer physical = FALSE, raw_line = 0, raw_column = 0
	'' The displayed line can be changed by #line. The last consumed token
	'' supplies a physical point only when its source and displayed line agree
	'' with the actual diagnostic. No editable expression range is invented.
	if( token_point.is_physical and token_point.raw_valid and (token_point.source_file = env.inf.name) and _
		(token_point.start_line = line_number) and (token_point.raw_start_line > 0) and (token_point.raw_start_column >= 0) ) then
		physical = TRUE
		raw_line = token_point.raw_start_line
		raw_column = token_point.raw_start_column
	end if
	hDiagnosticWrite("DI" + TABCHAR + fbSemanticModelNumber(diagnostic_count) + TABCHAR + _
		fbSemanticModelNumber(diagnostic_modules) + TABCHAR + severity + TABCHAR + fbSemanticModelNumber(code) + _
		TABCHAR + fbSemanticModelEscape(kind_text) + TABCHAR + fbSemanticModelEscape(env.inf.name) + _
		TABCHAR + fbSemanticModelNumber(line_number) + TABCHAR + fbSemanticModelEscape(message_text) + _
		TABCHAR + fbSemanticModelEscape(detail_text) + TABCHAR + fbSemanticModelEscape(custom_text) + _
		TABCHAR + fbSemanticModelNumber(abs(physical)) + TABCHAR + fbSemanticModelNumber(raw_line) + _
		TABCHAR + fbSemanticModelNumber(raw_column))
end sub

'' -------------------------------------------------------------------------
'' Shared compiler diagnostic observation
'' -------------------------------------------------------------------------

sub fbSemanticModelDiagnostic _
	( byref severity as const string, byval code as integer, byval line_number as integer, _
	  byval message as const zstring ptr, byval detail as const zstring ptr, byval custom_text as const zstring ptr )
	if( (fbSemanticModelEnabled( ) = FALSE) and (diagnostic_output = NULL) ) then exit sub
	dim as string text, extra, custom
	if( message <> NULL ) then text = *message
	if( detail <> NULL ) then extra = *detail
	if( custom_text <> NULL ) then custom = *custom_text
	hDiagnosticObserve(severity, code, line_number, text, extra, custom)
	if( fbSemanticModelEnabled( ) = FALSE ) then exit sub
	dim as longint identity = fbSemanticModelNextDetailIdentity( )
	'' Reporting must not ask the lexer to scan another token. A diagnostic
	'' point is informational, even when its line resembles a source range.
	fbSemanticModelAppendProvenance("DI" + TABCHAR + fbSemanticModelNumber(identity) + TABCHAR + severity + _
		TABCHAR + fbSemanticModelNumber(code) + TABCHAR + fbSemanticModelEscape(text) + TABCHAR + _
		fbSemanticModelEscape(extra) + TABCHAR + fbSemanticModelEscape(custom) + TABCHAR + _
		fbSemanticModelEscape(env.inf.name) + TABCHAR + fbSemanticModelNumber(line_number) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelCurrentSource( )) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelCurrentStatement( )) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelSymbolId(parser.currproc)))
end sub

'' end of tooling/semantic-diagnostics.bas
