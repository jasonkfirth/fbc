'' Project: FreeBASIC compiler - semantic output
'' File: tooling/semantic-output.bas
'' Purpose: Publish sidecars without overwriting inputs or hiding I/O errors.
'' Responsibilities: Own staging, protected file snapshots, and publication.
'' This file intentionally does NOT contain: parsing or model serialization.

#include once "tooling/semantic-output.bi"
#include once "fbc-int/file-info.bi"
#include once "crt/stdio.bi"
#include once "crt/stdlib.bi"
#include once "crt/errno.bi"

#ifdef __FB_WIN32__
#include once "windows.bi"
#endif

extern "c"
#ifdef __FB_WIN32__
declare function hFullPath alias "_fullpath" _
	( byval result as zstring ptr, byval filename as const zstring ptr, byval bytes as uinteger ) as zstring ptr
declare function hMakeDirectory alias "_mkdir"( byval filename as const zstring ptr ) as long
#elseif defined(__FB_DOS__)
declare sub hFixPath alias "_fixpath"( byval filename as const zstring ptr, byval result as zstring ptr )
#else
declare function hRealPath alias "realpath" _
	( byval filename as const zstring ptr, byval result as zstring ptr ) as zstring ptr
declare function hMakeDirectory alias "mkdir"( byval filename as const zstring ptr, byval mode as ulong ) as long
#endif
end extern

'' -------------------------------------------------------------------------
'' Filesystem identities and writer ownership
'' -------------------------------------------------------------------------

type SEMANTIC_OUTPUT_PROTECTED
	filename as string
	identity as FB_FILE_INFO
	next as SEMANTIC_OUTPUT_PROTECTED ptr
end type

type SEMANTIC_OUTPUT
	stream as FILE ptr
	destination as string
	directory as string
	staging as string
	journal as string
	protected_files as SEMANTIC_OUTPUT_PROTECTED ptr
	failed as integer
end type

private function hPathSeparator( byval ch as ubyte ) as integer
#if defined(__FB_WIN32__) or defined(__FB_DOS__)
	return (ch = asc("/")) or (ch = asc("\"))
#else
	return ch = asc("/")
#endif
end function

private function hPathJoin( byref directory as const string, byref filename as const string ) as string
	if( len(directory) = 0 ) then return filename
	if( hPathSeparator(directory[len(directory) - 1]) ) then return directory + filename
#if defined(__FB_WIN32__) or defined(__FB_DOS__)
	if( directory[len(directory) - 1] = asc(":") ) then return directory + filename
#endif
	return directory + "/" + filename
end function

private function hCanonicalPath( byval filename as const zstring ptr ) as string
#ifdef __FB_WIN32__
	dim as zstring ptr resolved = hFullPath(NULL, filename, 0)
	if( resolved = NULL ) then return ""
	dim as string result = *resolved
	free(resolved)
	return result
#elseif defined(__FB_DOS__)
	'' DJGPP documents FILENAME_MAX bytes for _fixpath's output buffer.
	dim as zstring * FILENAME_MAX resolved
	errno = 0
	hFixPath(filename, @resolved)
	if( errno <> 0 ) then return ""
	return resolved
#else
	dim as zstring ptr resolved = hRealPath(filename, NULL)
	if( resolved <> NULL ) then
		dim as string result = *resolved
		free(resolved)
		return result
	end if
	'' Future emissions need not exist. Resolve their parent, including any
	'' directory symlinks, before adding the final component.
	dim as string path = *filename
	dim as integer slash = instrrev(path, "/")
	dim as string parent = ".", basename = path
	if( slash <> 0 ) then
		parent = left(path, iif(slash = 1, 1, slash - 1))
		basename = mid(path, slash + 1)
	end if
	resolved = hRealPath(strptr(parent), NULL)
	if( resolved = NULL ) then return ""
	parent = *resolved
	free(resolved)
	return hPathJoin(parent, basename)
#endif
end function

private function hSamePath( byref first as const string, byref second as const string ) as integer
#if defined(__FB_WIN32__) or defined(__FB_DOS__)
	return lcase(first) = lcase(second)
#else
	return first = second
#endif
end function

private function hSameIdentity( byref first as const FB_FILE_INFO, byref second as const FB_FILE_INFO ) as integer
	if( (first.flags and FB_FILE_INFO_IDENTITY) = 0 ) then return FALSE
	if( (second.flags and FB_FILE_INFO_IDENTITY) = 0 ) then return FALSE
	for index as integer = 0 to 2
		if( first.identity(index) <> second.identity(index) ) then return FALSE
	next
	return TRUE
end function

private function hOrdinaryDestination( byval filename as const zstring ptr ) as integer
	dim as FB_FILE_INFO info
	if( fb_FileQueryInfo(filename, FALSE, @info) = 0 ) then return FALSE
	'' Query without following links. Never replace a device, directory, FIFO,
	'' or existing symlink; /dev/full must be rejected without opening it.
	return ((info.flags and FB_FILE_INFO_EXISTS) = 0) or ((info.flags and FB_FILE_INFO_REGULAR) <> 0)
end function

private function hReplaceFile( byval source as const zstring ptr, byval destination as const zstring ptr ) as long
#ifdef __FB_WIN32__
	'' The CRT rename does not replace an existing Windows destination. This
	'' host operation replaces it without first deleting the previous model.
	if( MoveFileExA(source, destination, MOVEFILE_REPLACE_EXISTING) ) then return 0
	return -1
#else
	return rename(source, destination)
#endif
end function

'' The standalone BASIC fixture substitutes only these operations. Production
'' builds always use their host implementations, with no runtime fault knobs.
#ifndef SEMANTIC_OUTPUT_WRITE
#define SEMANTIC_OUTPUT_WRITE fwrite
#endif
#ifndef SEMANTIC_OUTPUT_FLUSH
#define SEMANTIC_OUTPUT_FLUSH fflush
#endif
#ifndef SEMANTIC_OUTPUT_CLOSE
#define SEMANTIC_OUTPUT_CLOSE fclose
#endif
#ifndef SEMANTIC_OUTPUT_REPLACE
#define SEMANTIC_OUTPUT_REPLACE hReplaceFile
#endif

private sub hReleaseOutput( byval writer as SEMANTIC_OUTPUT ptr )
	if( writer = NULL ) then exit sub
	if( writer->stream <> NULL ) then SEMANTIC_OUTPUT_CLOSE(writer->stream)
	if( len(writer->staging) <> 0 ) then remove(strptr(writer->staging))
	if( len(writer->journal) <> 0 ) then remove(strptr(writer->journal))
	if( len(writer->directory) <> 0 ) then rmdir(writer->directory)
	dim as SEMANTIC_OUTPUT_PROTECTED ptr item = writer->protected_files
	while( item <> NULL )
		dim as SEMANTIC_OUTPUT_PROTECTED ptr next_item = item->next
		delete item
		item = next_item
	wend
	delete writer
end sub

'' -------------------------------------------------------------------------
'' Checked staging and final publication
'' -------------------------------------------------------------------------

function fbSemanticOutputOpen( byval filename as const zstring ptr ) as any ptr
	if( filename = NULL ) then return NULL
	if( filename[0] = 0 ) then return NULL
	if( hOrdinaryDestination(filename) = FALSE ) then return NULL
	dim as SEMANTIC_OUTPUT ptr writer = new SEMANTIC_OUTPUT
	if( writer = NULL ) then return NULL
	writer->destination = hCanonicalPath(filename)
	if( len(writer->destination) = 0 ) then
		hReleaseOutput(writer)
		return NULL
	end if
	dim as integer length = len(writer->destination)
	while( length > 0 )
		if( hPathSeparator(writer->destination[length - 1]) ) then exit while
		length -= 1
	wend
	dim as string parent = left(writer->destination, length)
	'' mkdir gives exclusive ownership, avoiding a name check followed by a
	'' truncating open. Staging beside the destination keeps rename on one FS.
	dim as ulong nonce = culng(timer * 1000.0)
	for attempt as integer = 0 to 255
#ifdef __FB_DOS__
		dim as string tag = "fs" + hex(nonce and &hffff, 4) + hex(attempt, 2)
#else
		dim as string tag = ".fb-semantic-" + hex(nonce, 8) + "-" + hex(attempt, 2)
#endif
		dim as string candidate = hPathJoin(parent, tag)
#ifdef __FB_WIN32__
		dim as integer created = hMakeDirectory(strptr(candidate)) = 0
#elseif defined(__FB_DOS__)
		dim as integer created = mkdir(candidate) = 0
#else
		'' Only the compiler process may read or replace a staged model.
		dim as integer created = hMakeDirectory(strptr(candidate), &o700) = 0
#endif
		if( created ) then
			writer->directory = candidate
			exit for
		end if
		if( errno <> EEXIST ) then exit for
	next
	if( len(writer->directory) = 0 ) then
		hReleaseOutput(writer)
		return NULL
	end if
	writer->staging = hPathJoin(writer->directory, "model.tmp")
	writer->stream = fopen(strptr(writer->staging), @"wb")
	if( writer->stream = NULL ) then
		hReleaseOutput(writer)
		return NULL
	end if
	return writer
end function

function fbSemanticOutputProtect( byval handle as any ptr, byval filename as const zstring ptr ) as long
	dim as SEMANTIC_OUTPUT ptr writer = handle
	if( writer = NULL ) then return FALSE
	if( writer->failed ) then return FALSE
	if( filename = NULL ) then return TRUE
	if( filename[0] = 0 ) then return TRUE
	dim as string path = hCanonicalPath(filename)
	if( len(path) = 0 ) then
		writer->failed = TRUE
		return FALSE
	end if
	dim as SEMANTIC_OUTPUT_PROTECTED ptr item = writer->protected_files
	while( item <> NULL )
		if( hSamePath(path, item->filename) ) then return TRUE
		item = item->next
	wend
	item = new SEMANTIC_OUTPUT_PROTECTED
	if( item = NULL ) then
		writer->failed = TRUE
		return FALSE
	end if
	item->filename = path
	item->next = writer->protected_files
	writer->protected_files = item
	dim as FB_FILE_INFO destination
	if( (fb_FileQueryInfo(strptr(path), TRUE, @item->identity) = 0) or _
	    (fb_FileQueryInfo(strptr(writer->destination), TRUE, @destination) = 0) or _
	    hSamePath(path, writer->destination) or hSameIdentity(item->identity, destination) ) then
		writer->failed = TRUE
		return FALSE
	end if
	return TRUE
end function

function fbSemanticOutputWrite( byval handle as any ptr, byval buffer as const any ptr, byval bytes as uinteger ) as long
	dim as SEMANTIC_OUTPUT ptr writer = handle
	if( writer = NULL ) then return FALSE
	if( writer->failed ) then return FALSE
	if( bytes <> 0 ) then
		if( buffer = NULL ) then
			writer->failed = TRUE
		elseif( SEMANTIC_OUTPUT_WRITE(buffer, 1, bytes, writer->stream) <> bytes ) then
			writer->failed = TRUE
		end if
	end if
	if( ferror(writer->stream) ) then writer->failed = TRUE
	return writer->failed = FALSE
end function

function fbSemanticOutputJournal _
	( byval handle as any ptr, byval filename as zstring ptr, byval capacity as uinteger ) as any ptr
	dim as SEMANTIC_OUTPUT ptr writer = handle
	if( (writer = NULL) or (filename = NULL) or (capacity = 0) ) then return NULL
	filename[0] = 0
	if( writer->failed or (len(writer->journal) <> 0) ) then return NULL
	'' The exclusively created directory belongs to this writer. This is a
	'' fixed child name, never a caller-supplied path or a public input file.
	dim as string path = hPathJoin(writer->directory, "link.tmp")
	if( len(path) >= capacity ) then
		writer->failed = TRUE
		return NULL
	end if
	dim as FILE ptr stream = fopen(strptr(path), @"w+b")
	if( stream = NULL ) then
		writer->failed = TRUE
		return NULL
	end if
	writer->journal = path
	*filename = path
	return stream
end function

function fbSemanticOutputFinish( byval handle as any ptr, byval publish as long ) as long
	dim as SEMANTIC_OUTPUT ptr writer = handle
	if( writer = NULL ) then return TRUE
	if( writer->stream <> NULL ) then
		if( SEMANTIC_OUTPUT_FLUSH(writer->stream) <> 0 ) then writer->failed = TRUE
		if( SEMANTIC_OUTPUT_CLOSE(writer->stream) <> 0 ) then writer->failed = TRUE
		writer->stream = NULL
	end if
	if( publish and (writer->failed = FALSE) ) then
		dim as FB_FILE_INFO destination, current
		if( hOrdinaryDestination(strptr(writer->destination)) = FALSE ) then writer->failed = TRUE
		if( fb_FileQueryInfo(strptr(writer->destination), TRUE, @destination) = 0 ) then writer->failed = TRUE
		dim as SEMANTIC_OUTPUT_PROTECTED ptr item = writer->protected_files
		while( (writer->failed = FALSE) and (item <> NULL) )
			if( (fb_FileQueryInfo(strptr(item->filename), TRUE, @current) = 0) or _
			    hSamePath(item->filename, writer->destination) or _
			    hSameIdentity(item->identity, destination) or hSameIdentity(current, destination) ) then
				writer->failed = TRUE
			end if
			item = item->next
		wend
		if( writer->failed = FALSE ) then
			if( SEMANTIC_OUTPUT_REPLACE(strptr(writer->staging), strptr(writer->destination)) <> 0 ) then writer->failed = TRUE
		end if
	end if
	dim as long ok = writer->failed = FALSE
	hReleaseOutput(writer)
	return ok
end function

'' end of tooling/semantic-output.bas
