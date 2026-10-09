'' Project: FreeBASIC compiler - native link observations
'' File: tooling/semantic-link-transport.bas
'' Purpose: Publish linker decisions independently of an accepted AST model.
'' Responsibilities: Checked output, private callback journal and tool outcomes.
'' This file intentionally does NOT contain: linker error parsing or name resolution.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-link.bi"
#include once "tooling/semantic-output.bi"
#include once "fbc-int/file-info.bi"
#include once "crt/stdio.bi"
#include once "crt/stdlib.bi"

private sub hLinkTrace( byref phase as const string, byref detail as const string )
#ifdef SEMANTIC_LINK_TRACE
	print "semantic-link trace: "; phase; " "; detail
#endif
end sub

extern "c"
declare function fb_hGetExeName( byval buffer as zstring ptr, byval bytes as integer ) as zstring ptr
#ifndef __FB_WIN32__
declare function unsetenv( byval key_text as const zstring ptr ) as long
#endif
end extern

'' -------------------------------------------------------------------------
'' Invocation-owned output and callback state
'' -------------------------------------------------------------------------

private const LINK_MAX_BYTES = 67108864
private const LINK_MAX_CALLBACKS = 100000
private const LINK_MAX_SYMBOL_BYTES = 4096
private const LINK_JOURNAL_MAX_BYTES = 16777216
private const LINK_CALLBACK_ENV = "FBC_SEMANTIC_LINK_CALLBACK_V1"
private dim shared as string link_filename, link_journal, link_header
private dim shared as string link_previous_environment
private dim shared as integer link_environment_present, link_environment_changed
private dim shared as any ptr link_output
private dim shared as integer link_failed, link_bytes, link_attempted, link_completed, link_callbacks
private dim shared as integer link_native_exit
private dim shared as string link_tool, link_coverage

sub fbSemanticLinkOutput( byref filename as const string )
	hLinkTrace("output", filename)
	link_filename = filename
end sub

function fbSemanticLinkFilename( ) as string
	return link_filename
end function

function fbSemanticLinkEnabled( ) as integer
	return (link_output <> NULL) and (link_failed = FALSE)
end function

sub fbSemanticLinkFail( )
	link_failed = TRUE
end sub

sub fbSemanticLinkWrite( byref record_text as const string )
	if( (link_output = NULL) or link_failed ) then exit sub
	dim as string line_text = record_text + NEWLINE
	if( len(line_text) > LINK_MAX_BYTES - link_bytes ) then
		link_failed = TRUE
		exit sub
	end if
	if( fbSemanticOutputWrite(link_output, strptr(line_text), len(line_text)) = 0 ) then
		link_failed = TRUE
	else
		link_bytes += len(line_text)
	end if
end sub

sub fbSemanticLinkProtect( byref filename as const string )
	if( (link_output = NULL) or (len(filename) = 0) ) then exit sub
	if( fbSemanticOutputProtect(link_output, strptr(filename)) = 0 ) then link_failed = TRUE
end sub

private sub hRestoreCallbackEnvironment( )
	if( link_environment_changed = FALSE ) then exit sub
#ifndef __FB_WIN32__
	if( link_environment_present = FALSE ) then
		if( unsetenv(strptr(LINK_CALLBACK_ENV)) <> 0 ) then link_failed = TRUE
	else
#endif
		if( setenviron(LINK_CALLBACK_ENV + "=" + link_previous_environment) <> 0 ) then link_failed = TRUE
#ifndef __FB_WIN32__
	end if
#endif
	link_environment_changed = FALSE
	link_previous_environment = ""
end sub

function fbSemanticLinkBegin( ) as integer
	hLinkTrace("begin", link_filename)
	hRestoreCallbackEnvironment( )
	if( link_output <> NULL ) then fbSemanticOutputFinish(link_output, FALSE)
	link_output = NULL
	link_failed = FALSE
	link_bytes = 0
	link_attempted = FALSE
	link_completed = FALSE
	link_callbacks = 0
	link_native_exit = 0
	link_tool = ""
	link_coverage = "unavailable"
	link_journal = ""
	link_header = ""
	fbSemanticLinkBindingsReset( )
	if( len(link_filename) = 0 ) then return TRUE
	link_output = fbSemanticOutputOpen(strptr(link_filename))
	if( link_output = NULL ) then return FALSE
	fbSemanticLinkWrite("FBCLNK" + TABCHAR + "2" + TABCHAR + FB_VERSION)
	return link_failed = FALSE
end function

'' -------------------------------------------------------------------------
'' Native callback protocol
'' -------------------------------------------------------------------------

private function hJournalIdentity( byref info as const FB_FILE_INFO ) as string
	return hex(info.identity(0), 16) + hex(info.identity(1), 16) + hex(info.identity(2), 16)
end function

private function hJournalOrdinary( byref filename as const string, byref info as FB_FILE_INFO ) as integer
	if( fb_FileQueryInfo(strptr(filename), FALSE, @info) = 0 ) then return FALSE
	return (info.flags and (FB_FILE_INFO_REGULAR or FB_FILE_INFO_IDENTITY)) = _
		(FB_FILE_INFO_REGULAR or FB_FILE_INFO_IDENTITY)
end function

'' GNU ld calls its helper with exactly a failure kind and an object symbol.
'' This entry point runs before compiler initialization, only while the parent
'' linker invocation supplies a matching private journal. It opens no new file
'' and compares the open stream's native identity before appending one record.
function fbSemanticLinkCallback( byval argument_count as integer ) as integer
	dim as string capsule = environ(LINK_CALLBACK_ENV)
	if( (len(capsule) = 0) or (argument_count <> 3) ) then return -1
	dim as string kind_text = command(1)
	if( (kind_text <> "undefined-symbol") and (kind_text <> "missing-lib") ) then return -1
	dim as string symbol_text = command(2)
	if( (len(symbol_text) = 0) or (len(symbol_text) > LINK_MAX_SYMBOL_BYTES) ) then return 2
	dim as integer separator = instr(capsule, TABCHAR)
	if( (separator <= 1) or (separator > FB_MAXPATHLEN) ) then return 2
	dim as string filename = left(capsule, separator - 1), expected = mid(capsule, separator + 1)
	if( (len(expected) <> 48) or (instr(filename, chr(10)) > 0) or (instr(filename, chr(13)) > 0) ) then return 2
	dim as FB_FILE_INFO before, current
	if( hJournalOrdinary(filename, before) = FALSE ) then return 2
	if( (hJournalIdentity(before) <> expected) or (before.bytes > LINK_JOURNAL_MAX_BYTES) ) then return 2
	dim as FILE ptr stream = fopen(strptr(filename), @"r+b")
	if( stream = NULL ) then return 2
	dim as integer ok = fb_FileQueryStreamInfo(stream, @current) <> 0
	if( ok ) then ok = hJournalIdentity(current) = expected
	dim as zstring * 128 header_buffer
	if( ok ) then ok = fgets(@header_buffer, sizeof(header_buffer), stream) <> NULL
	if( ok ) then ok = (header_buffer = "FBCLNK-CALLBACK" + TABCHAR + expected + NEWLINE)
	dim as string record_text = kind_text + TABCHAR + fbSemanticModelEscape(symbol_text) + NEWLINE
	if( ok ) then ok = len(record_text) <= LINK_JOURNAL_MAX_BYTES - current.bytes
	if( ok ) then ok = fseek(stream, 0, SEEK_END) = 0
	if( ok ) then ok = fwrite(strptr(record_text), 1, len(record_text), stream) = len(record_text)
	if( fflush(stream) <> 0 ) then ok = FALSE
	if( fclose(stream) <> 0 ) then ok = FALSE
	return iif(ok, 0, 2)
end function

private function hShellQuote( byref value as const string ) as string
#ifdef __FB_WIN32__
	'' cmd.exe expands percent and delayed-expansion markers inside quotes.
	'' Refuse those uncommon tool paths rather than executing a different path.
	if( (instr(value, """") > 0) or (instr(value, "%") > 0) or (instr(value, "!") > 0) ) then return ""
	return """" + value + """"
#else
	dim as string result = "'"
	for index as integer = 0 to len(value) - 1
		if( value[index] = asc("'") ) then
			result += "'" + """'""" + "'"
		else
			result += chr(value[index])
		end if
	next
	return result + "'"
#endif
end function

private function hHasCallbackOption( byref toolpath as const string ) as integer
	if( (instr(toolpath, chr(10)) > 0) or (instr(toolpath, chr(13)) > 0) ) then return FALSE
	dim as string quoted = hShellQuote(toolpath)
	if( len(quoted) = 0 ) then return FALSE
	dim as integer number = freefile( )
	if( open pipe(quoted + " --help", for input, as #number) <> 0 ) then return FALSE
	'' This is interface discovery from tool help, never parsing an error
	'' message to decide whether an input program has an unresolved symbol.
	dim as integer found = FALSE, bytes = 0
	dim as string tail_text
	do while( eof(number) = FALSE )
		dim as string chunk = input(1024, number)
		bytes += len(chunk)
		if( bytes > 1048576 ) then
			found = FALSE
			exit do
		end if
		tail_text += chunk
		if( instr(tail_text, "--error-handling-script") > 0 ) then found = TRUE
		tail_text = right(tail_text, 32)
	loop
	if( close(number) <> 0 ) then found = FALSE
	return found
end function

function fbSemanticLinkPrepare( byref toolpath as const string, byref arguments as string ) as integer
	hLinkTrace("prepare", link_filename)
	if( fbSemanticLinkEnabled( ) = FALSE ) then return link_failed = FALSE
	if( link_attempted ) then
		link_failed = TRUE
		return FALSE
	end if
	link_attempted = TRUE
	link_tool = toolpath
	'' Retain the original native closure arguments, before adding our helper.
	fbSemanticLinkWrite("ARGS" + TABCHAR + fbSemanticModelEscape(arguments))
	'' A caller's existing helper is part of the native link configuration.
	'' Do not replace it or claim that our callback coverage is available.
	if( instr(arguments, "--error-handling-script") > 0 ) then return TRUE
	if( hHasCallbackOption(toolpath) = FALSE ) then return TRUE
	dim as zstring * FB_MAXPATHLEN+1 executable_buffer, journal_buffer
	dim as zstring ptr executable_name = fb_hGetExeName(@executable_buffer, sizeof(executable_buffer) - 1)
	if( executable_name = NULL ) then return TRUE
	dim as string executable = exepath( ) + FB_HOST_PATHDIV + *executable_name
	if( (instr(executable, """") > 0) or (instr(executable, chr(10)) > 0) or (instr(executable, chr(13)) > 0) ) then return TRUE
	dim as FILE ptr stream = fbSemanticOutputJournal(link_output, @journal_buffer, sizeof(journal_buffer))
	if( stream = NULL ) then
		link_failed = TRUE
		return FALSE
	end if
	link_journal = journal_buffer
	dim as FB_FILE_INFO info
	dim as integer ok = fb_FileQueryStreamInfo(stream, @info) <> 0
	if( ok ) then ok = (info.flags and FB_FILE_INFO_IDENTITY) <> 0
	link_header = "FBCLNK-CALLBACK" + TABCHAR + hJournalIdentity(info) + NEWLINE
	if( ok ) then ok = fwrite(strptr(link_header), 1, len(link_header), stream) = len(link_header)
	if( fflush(stream) <> 0 ) then ok = FALSE
	if( fclose(stream) <> 0 ) then ok = FALSE
	if( ok = FALSE ) then
		link_failed = TRUE
		return FALSE
	end if
	link_previous_environment = environ(LINK_CALLBACK_ENV)
	link_environment_present = getenv(strptr(LINK_CALLBACK_ENV)) <> NULL
	if( setenviron(LINK_CALLBACK_ENV + "=" + link_journal + TABCHAR + hJournalIdentity(info)) <> 0 ) then
		link_failed = TRUE
		return FALSE
	end if
	link_environment_changed = TRUE
	link_coverage = "available"
	arguments += " --error-handling-script=""" + executable + """"
	return TRUE
end function

'' The callback writer emits byte escapes, not decoded text. Validate one
'' field before forwarding it so a damaged journal cannot manufacture another
'' record or exceed the same symbol bound used by the native callback.
private function hJournalSymbol( byref escaped as const string ) as integer
	dim as integer position = 1, bytes = 0
	do while( position <= len(escaped) )
		dim as integer code = asc(escaped, position)
		if( (code < 32) or (code >= 127) ) then return FALSE
		if( code = asc("%") ) then
			if( position + 2 > len(escaped) ) then return FALSE
			dim as integer decoded = 0
			for digit as integer = position + 1 to position + 2
				code = asc(escaped, digit)
				select case code
				case asc("0") to asc("9"): code -= asc("0")
				case asc("A") to asc("F"): code = code - asc("A") + 10
				case else: return FALSE
				end select
				decoded = decoded * 16 + code
			next
			if( decoded = 0 ) then return FALSE
			position += 3
		else
			position += 1
		end if
		bytes += 1
		if( bytes > LINK_MAX_SYMBOL_BYTES ) then return FALSE
	loop
	return bytes > 0
end function

sub fbSemanticLinkCompleted( byval native_exit as integer )
	hLinkTrace("completed", str(native_exit) + " " + link_coverage)
	if( link_output = NULL ) then exit sub
	link_native_exit = native_exit
	link_completed = TRUE
	hRestoreCallbackEnvironment( )
	if( link_coverage <> "available" ) then exit sub
	dim as FB_FILE_INFO info
	if( hJournalOrdinary(link_journal, info) = FALSE ) then
		link_failed = TRUE
		exit sub
	end if
	if( info.bytes > LINK_JOURNAL_MAX_BYTES ) then
		link_failed = TRUE
		exit sub
	end if
	dim as FILE ptr stream = fopen(strptr(link_journal), @"rb")
	if( stream = NULL ) then
		link_failed = TRUE
		exit sub
	end if
	'' A pathname can be replaced between the no-follow query and fopen.
	'' The opened object must still be the invocation-owned journal, not just
	'' another file containing a copied header.
	dim as FB_FILE_INFO current
	if( fb_FileQueryStreamInfo(stream, @current) = 0 ) then
		link_failed = TRUE
	elseif( (hJournalIdentity(current) <> hJournalIdentity(info)) or _
		(link_header <> "FBCLNK-CALLBACK" + TABCHAR + hJournalIdentity(current) + NEWLINE) or _
		(current.bytes > LINK_JOURNAL_MAX_BYTES) ) then
		link_failed = TRUE
	end if
	dim as zstring * 32770 buffer
	if( fgets(@buffer, sizeof(buffer), stream) = NULL ) then
		link_failed = TRUE
	elseif( buffer <> link_header ) then
		hLinkTrace("wrong journal header", buffer + " expected " + link_header)
		link_failed = TRUE
	end if
	do while( link_failed = FALSE )
		if( fgets(@buffer, sizeof(buffer), stream) = NULL ) then exit do
		dim as string record_text = buffer
		if( (len(record_text) < len(NEWLINE)) or (right(record_text, len(NEWLINE)) <> NEWLINE) or (link_callbacks >= LINK_MAX_CALLBACKS) ) then
			hLinkTrace("invalid callback record", record_text)
			link_failed = TRUE
			exit do
		end if
		record_text = left(record_text, len(record_text) - len(NEWLINE))
		dim as integer separator = instr(record_text, TABCHAR)
		if( separator <= 1 ) then
			link_failed = TRUE
			exit do
		end if
		dim as string kind_text = left(record_text, separator - 1)
		if( (separator <= 1) or ((kind_text <> "undefined-symbol") and (kind_text <> "missing-lib")) or _
			(instr(separator + 1, record_text, TABCHAR) > 0) or (separator = len(record_text)) ) then
			link_failed = TRUE
			exit do
		end if
		if( hJournalSymbol(mid(record_text, separator + 1)) = FALSE ) then
			link_failed = TRUE
			exit do
		end if
		link_callbacks += 1
		fbSemanticLinkWrite("U" + TABCHAR + fbSemanticModelNumber(link_callbacks) + TABCHAR + record_text)
	loop
	if( ferror(stream) <> 0 ) then link_failed = TRUE
	if( fclose(stream) <> 0 ) then link_failed = TRUE
	if( (native_exit = 0) and (link_callbacks > 0) ) then link_failed = TRUE
end sub

function fbSemanticLinkEnd( byval succeeded as integer ) as integer
	hLinkTrace("end", link_filename)
	hRestoreCallbackEnvironment( )
	if( link_output = NULL ) then return TRUE
	dim as integer bindings_ok = fbSemanticLinkBindingsEnd( )
	hLinkTrace("bindings end", str(bindings_ok) + " failed=" + str(link_failed))
	if( bindings_ok = FALSE ) then link_failed = TRUE
	fbSemanticLinkWrite("LINK" + TABCHAR + fbSemanticModelNumber(abs(link_attempted)) + TABCHAR + _
		fbSemanticModelNumber(abs(link_completed)) + TABCHAR + fbSemanticModelNumber(link_native_exit) + _
		TABCHAR + link_coverage + TABCHAR + fbSemanticModelEscape(link_tool))
	fbSemanticLinkWrite("END" + TABCHAR + "2" + TABCHAR + fbSemanticModelNumber(link_callbacks) + _
		TABCHAR + fbSemanticModelNumber(abs(succeeded)))
	dim as integer published = fbSemanticOutputFinish(link_output, link_failed = FALSE)
	link_output = NULL
	link_journal = ""
	link_header = ""
	return (published <> 0) and (link_failed = FALSE)
end function

'' end of tooling/semantic-link-transport.bas
