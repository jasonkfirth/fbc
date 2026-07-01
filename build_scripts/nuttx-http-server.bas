'
' Project: FreeBASIC NuttX remote board workflow
' ------------------------------------------------
'
' File: nuttx-http-server.bas
'
' Purpose:
'
'     Serve staged NuttX modules and data files to a board over a small HTTP
'     server built with FreeBASIC's OPEN TCP support.
'
' Responsibilities:
'
'     - accept simple HTTP/1.0 GET requests
'     - restrict requests to files under one configured directory
'     - stream file contents to NuttX wget
'     - stay small enough to use as build-script infrastructure
'
' This file intentionally does NOT contain:
'
'     - directory listings
'     - CGI or dynamic request handling
'     - upload handling
'     - TLS or authentication
'

#define FB_NO_GFXLIB
#define FB_NO_SFXLIB

#if defined(__FB_DOS__) or defined(__FB_JS__) or defined(__FB_XBOX__)
	print "nuttx-http-server is not supported on this host target"
	end 1
#endif

const DEFAULT_PORT = 8008
const DEFAULT_BIND = "0.0.0.0"
const READ_TIMEOUT_MS = 10000
const CHUNK_SIZE = 8192

type ServerOptions
	bind_host as string
	port as integer
	root_dir as string
end type

declare function Main() as integer
declare function ParseArgs(byref opt as ServerOptions) as integer
declare function ParseInteger(byref text as string, byval minimum as integer, _
	byval maximum as integer, byref value as integer) as integer
declare function NeedValue(byval index as integer, byref option_name as string, _
	byref value as string) as integer
declare sub Usage()
declare function Die(byref message as string) as integer
declare sub LogMessage(byref message as string)
declare function HandleClient(byval client_file as integer, byref opt as ServerOptions) as integer
declare function ReadHttpLine(byval client_file as integer, byref line_text as string, _
	byval timeout_ms as integer) as integer
declare function RequestPathToFile(byref root_dir as string, byref request_path as string, _
	byref local_path as string) as integer
declare function SafeRelativePath(byref path as string) as integer
declare function ServeFile(byval client_file as integer, byref local_path as string) as integer
declare sub SendHttpError(byval client_file as integer, byval status_code as integer, _
	byref reason as string)
declare function ElapsedMs(byval started_at as double) as integer

end Main()

function Main() as integer
	dim opt as ServerOptions
	dim server_file as integer
	dim client_file as integer
	dim spec as string

	opt.bind_host = DEFAULT_BIND
	opt.port = DEFAULT_PORT

	if ParseArgs(opt) <> 0 then
		return 1
	end if

	if len(opt.root_dir) = 0 then
		return Die("--directory is required")
	end if

	server_file = freefile()
	spec = "host=" & opt.bind_host & ",port=" & str(opt.port) & ",backlog=4"

	if( OPEN TCP SERVER( spec AS #server_file ) <> 0 ) then
		return Die("OPEN TCP SERVER failed on " & opt.bind_host & ":" & _
			str(opt.port) & ", ERR=" & str(err))
	end if

	LogMessage("serving " & opt.root_dir & " at http://" & opt.bind_host & _
		":" & str(opt.port) & "/")

	do
		client_file = TCP ACCEPT(#server_file)
		if client_file <> 0 then
			HandleClient(client_file, opt)
			close #client_file
		else
			sleep 20, 1
		end if
	loop

	close #server_file
	return 0
end function

function ParseArgs(byref opt as ServerOptions) as integer
	dim index as integer = 1
	dim arg as string
	dim value as string
	dim parsed as integer

	do
		arg = command(index)
		if len(arg) = 0 then
			exit do
		end if

		select case arg
		case "--bind"
			if NeedValue(index, arg, value) <> 0 then return 1
			opt.bind_host = value
			index += 2

		case "--port"
			if NeedValue(index, arg, value) <> 0 then return 1
			if ParseInteger(value, 1, 65535, parsed) = 0 then
				return Die("--port must be between 1 and 65535")
			end if
			opt.port = parsed
			index += 2

		case "--directory"
			if NeedValue(index, arg, value) <> 0 then return 1
			opt.root_dir = value
			index += 2

		case "--help", "-h"
			Usage()
			end 0

		case else
			return Die("unknown option: " & arg)
		end select
	loop

	return 0
end function

function ParseInteger(byref text as string, byval minimum as integer, _
	byval maximum as integer, byref value as integer) as integer
	dim i as integer
	dim c as integer
	dim result as longint = 0

	if len(text) = 0 then
		return 0
	end if

	for i = 1 to len(text)
		c = asc(mid(text, i, 1))
		if c < asc("0") orelse c > asc("9") then
			return 0
		end if
		result = result * 10 + (c - asc("0"))
		if result > maximum then
			return 0
		end if
	next

	if result < minimum then
		return 0
	end if

	value = cint(result)
	return -1
end function

function NeedValue(byval index as integer, byref option_name as string, _
	byref value as string) as integer
	value = command(index + 1)
	if len(value) = 0 then
		return Die(option_name & " requires a value")
	end if
	return 0
end function

sub Usage()
	print "Usage: nuttx-http-server --directory DIR [options]"
	print
	print "Options:"
	print "  --directory DIR    directory to serve"
	print "  --bind ADDRESS     bind address, default: 0.0.0.0"
	print "  --port PORT        TCP port, default: 8008"
	print "  --help             show this help text"
end sub

function Die(byref message as string) as integer
	print #err, "ERROR: " & message
	return 1
end function

sub LogMessage(byref message as string)
	print #err, message
end sub

function HandleClient(byval client_file as integer, byref opt as ServerOptions) as integer
	dim request_line as string
	dim header_line as string
	dim first_space as integer
	dim second_space as integer
	dim method_name as string
	dim request_path as string
	dim local_path as string

	if ReadHttpLine(client_file, request_line, READ_TIMEOUT_MS) = 0 then
		SendHttpError(client_file, 408, "Request Timeout")
		return 1
	end if

	first_space = instr(request_line, " ")
	if first_space = 0 then
		SendHttpError(client_file, 400, "Bad Request")
		return 1
	end if

	second_space = instr(first_space + 1, request_line, " ")
	if second_space = 0 then
		SendHttpError(client_file, 400, "Bad Request")
		return 1
	end if

	method_name = left(request_line, first_space - 1)
	request_path = mid(request_line, first_space + 1, second_space - first_space - 1)

	do
		if ReadHttpLine(client_file, header_line, READ_TIMEOUT_MS) = 0 then
			exit do
		end if
	loop while len(header_line) <> 0

	if method_name <> "GET" then
		SendHttpError(client_file, 405, "Method Not Allowed")
		return 1
	end if

	if RequestPathToFile(opt.root_dir, request_path, local_path) = 0 then
		SendHttpError(client_file, 403, "Forbidden")
		return 1
	end if

	if ServeFile(client_file, local_path) <> 0 then
		SendHttpError(client_file, 404, "Not Found")
		return 1
	end if

	return 0
end function

function ReadHttpLine(byval client_file as integer, byref line_text as string, _
	byval timeout_ms as integer) as integer
	dim started_at as double = timer

	do while ElapsedMs(started_at) < timeout_ms
		if eof(client_file) = 0 then
			line input #client_file, line_text
			if right(line_text, 1) = chr(13) then
				line_text = left(line_text, len(line_text) - 1)
			end if
			return -1
		end if

		if eoc(client_file) <> 0 then
			return 0
		end if

		sleep 10, 1
	loop

	return 0
end function

function RequestPathToFile(byref root_dir as string, byref request_path as string, _
	byref local_path as string) as integer
	dim relative_path as string
	dim query_pos as integer

	if left(request_path, 1) <> "/" then
		return 0
	end if

	query_pos = instr(request_path, "?")
	if query_pos <> 0 then
		relative_path = mid(request_path, 2, query_pos - 2)
	else
		relative_path = mid(request_path, 2)
	end if

	if len(relative_path) = 0 then
		return 0
	end if

	if SafeRelativePath(relative_path) = 0 then
		return 0
	end if

	if right(root_dir, 1) = "/" orelse right(root_dir, 1) = "\" then
		local_path = root_dir & relative_path
	else
		local_path = root_dir & "/" & relative_path
	end if

	return -1
end function

function SafeRelativePath(byref path as string) as integer
	dim i as integer
	dim c as integer

	if left(path, 1) = "/" orelse left(path, 1) = "\" then
		return 0
	end if

	if instr(path, "..") <> 0 then
		return 0
	end if

	for i = 1 to len(path)
		c = asc(mid(path, i, 1))
		if c <= 32 orelse c >= 127 then
			return 0
		end if

		select case chr(c)
		case "\", ":", "*", "?", """", "<", ">", "|", "%"
			return 0
		end select
	next

	return -1
end function

function ServeFile(byval client_file as integer, byref local_path as string) as integer
	dim input_file as integer
	dim file_size as longint
	dim remaining as longint
	dim wanted as integer
	dim chunk as string

	input_file = freefile()

	on error goto OpenFailed
	open local_path for binary access read as #input_file
	on error goto 0

	file_size = lof(input_file)
	remaining = file_size

	print #client_file, "HTTP/1.0 200 OK" & chr(13) & chr(10);
	print #client_file, "Content-Type: application/octet-stream" & chr(13) & chr(10);
	print #client_file, "Content-Length: " & str(file_size) & chr(13) & chr(10);
	print #client_file, "Connection: close" & chr(13) & chr(10);
	print #client_file, chr(13) & chr(10);

	do while remaining > 0
		wanted = CHUNK_SIZE
		if remaining < wanted then
			wanted = cint(remaining)
		end if

		chunk = input(wanted, #input_file)
		if len(chunk) = 0 then
			exit do
		end if

		print #client_file, chunk;
		remaining -= len(chunk)
	loop

	close #input_file
	return 0

OpenFailed:
	on error goto 0
	return 1
end function

sub SendHttpError(byval client_file as integer, byval status_code as integer, _
	byref reason as string)
	dim body as string

	body = str(status_code) & " " & reason & chr(10)

	print #client_file, "HTTP/1.0 " & str(status_code) & " " & reason & chr(13) & chr(10);
	print #client_file, "Content-Type: text/plain" & chr(13) & chr(10);
	print #client_file, "Content-Length: " & str(len(body)) & chr(13) & chr(10);
	print #client_file, "Connection: close" & chr(13) & chr(10);
	print #client_file, chr(13) & chr(10);
	print #client_file, body;
end sub

function ElapsedMs(byval started_at as double) as integer
	dim now_time as double = timer

	if now_time < started_at then
		now_time += 86400.0
	end if

	return cint((now_time - started_at) * 1000.0)
end function

' end of nuttx-http-server.bas
