'
' Project: FreeBASIC NuttX remote board workflow
' ------------------------------------------------
'
' File: nuttx-remote-console.bas
'
' Purpose:
'
'     Send commands to a NuttX NSH console over either a telnet session or a
'     serial port.
'
' Responsibilities:
'
'     - handle enough telnet option negotiation for NSH command sessions
'     - pace serial command input for small NuttX targets
'     - collect command output until the NSH prompt returns
'     - provide a FreeBASIC-built command transport for upload/build scripts
'
' This file intentionally does NOT contain:
'
'     - firmware flashing logic
'     - FreeBASIC program compilation
'     - NuttX board configuration policy
'

#define FB_NO_GFXLIB
#define FB_NO_SFXLIB

#if defined(__FB_DOS__) or defined(__FB_JS__) or defined(__FB_XBOX__)
	print "nuttx-remote-console is not supported on this host target"
	end 1
#endif

const MAX_COMMANDS = 256
const DEFAULT_TIMEOUT = 30.0

const TELNET_IAC = 255
const TELNET_DONT = 254
const TELNET_DO = 253
const TELNET_WONT = 252
const TELNET_WILL = 251
const TELNET_SB = 250
const TELNET_SE = 240

type ConsoleOptions
	telnet_host as string
	telnet_port as integer
	serial_port as string
	baud as integer
	timeout_seconds as double
	startup_wait_ms as integer
	prompt as string
	char_delay_ms as integer
	line_delay_ms as integer
	no_sync as integer
	command_count as integer
	commands(0 to MAX_COMMANDS - 1) as string
end type

declare function Main() as integer
declare function ParseArgs(byref opt as ConsoleOptions) as integer
declare function ParseInteger(byref text as string, byval minimum as integer, _
	byval maximum as integer, byref value as integer) as integer
declare function ParseSeconds(byref text as string, byref milliseconds as integer) as integer
declare function NeedValue(byval index as integer, byref option_name as string, _
	byref value as string) as integer
declare sub Usage()
declare function Die(byref message as string) as integer
declare function OpenConsole(byref opt as ConsoleOptions, byref file_num as integer, _
	byref is_telnet as integer) as integer
declare function SynchronizeConsole(byval file_num as integer, byval is_telnet as integer, _
	byref opt as ConsoleOptions) as integer
declare function RunConsoleCommand(byval file_num as integer, byval is_telnet as integer, _
	byref opt as ConsoleOptions, byref command_text as string, byval index as integer) as integer
declare sub WriteLineToConsole(byval file_num as integer, byval is_telnet as integer, _
	byref text as string, byval char_delay_ms as integer)
declare function ReadUntil(byval file_num as integer, byval is_telnet as integer, _
	byref needle as string, byval timeout_seconds as double, byref out_text as string) as integer
declare function ReadSome(byval file_num as integer, byval is_telnet as integer) as string
declare function ReadByteFromConsole(byval file_num as integer, byref value as integer, _
	byval wait_ms as integer) as integer
declare sub SendTelnetReply(byval file_num as integer, byval command_byte as integer, _
	byval option_byte as integer)
declare function ElapsedSeconds(byval started_at as double) as double
declare function BuildSerialSpec(byref opt as ConsoleOptions) as string
declare function SleepForMs(byval milliseconds as integer) as integer

end Main()

function Main() as integer
	dim opt as ConsoleOptions
	dim file_num as integer
	dim is_telnet as integer
	dim i as integer

	opt.telnet_port = 23
	opt.baud = 115200
	opt.timeout_seconds = DEFAULT_TIMEOUT
	opt.prompt = "nsh>"
	opt.char_delay_ms = 15
	opt.line_delay_ms = 250

	if ParseArgs(opt) <> 0 then
		return 1
	end if

	if OpenConsole(opt, file_num, is_telnet) <> 0 then
		return 1
	end if

	if opt.startup_wait_ms > 0 then
		SleepForMs(opt.startup_wait_ms)
	end if

	if opt.no_sync = 0 then
		if SynchronizeConsole(file_num, is_telnet, opt) <> 0 then
			close #file_num
			return 1
		end if
	end if

	for i = 0 to opt.command_count - 1
		if RunConsoleCommand(file_num, is_telnet, opt, opt.commands(i), i) <> 0 then
			close #file_num
			return 1
		end if
	next

	close #file_num
	return 0
end function

function ParseArgs(byref opt as ConsoleOptions) as integer
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
		case "--telnet"
			if NeedValue(index, arg, value) <> 0 then return 1
			opt.telnet_host = value
			index += 2

		case "--serial"
			if NeedValue(index, arg, value) <> 0 then return 1
			opt.serial_port = value
			index += 2

		case "--telnet-port"
			if NeedValue(index, arg, value) <> 0 then return 1
			if ParseInteger(value, 1, 65535, parsed) = 0 then
				return Die("--telnet-port must be between 1 and 65535")
			end if
			opt.telnet_port = parsed
			index += 2

		case "--baud"
			if NeedValue(index, arg, value) <> 0 then return 1
			if ParseInteger(value, 1, 4000000, parsed) = 0 then
				return Die("--baud must be a positive serial speed")
			end if
			opt.baud = parsed
			index += 2

		case "--timeout"
			if NeedValue(index, arg, value) <> 0 then return 1
			opt.timeout_seconds = val(value)
			if opt.timeout_seconds <= 0.0 then
				return Die("--timeout must be a positive number of seconds")
			end if
			index += 2

		case "--startup-wait"
			if NeedValue(index, arg, value) <> 0 then return 1
			if ParseSeconds(value, parsed) = 0 then
				return Die("--startup-wait must be a non-negative number of seconds")
			end if
			opt.startup_wait_ms = parsed
			index += 2

		case "--prompt"
			if NeedValue(index, arg, value) <> 0 then return 1
			opt.prompt = value
			index += 2

		case "--char-delay"
			if NeedValue(index, arg, value) <> 0 then return 1
			if ParseSeconds(value, parsed) = 0 then
				return Die("--char-delay must be a non-negative number of seconds")
			end if
			opt.char_delay_ms = parsed
			index += 2

		case "--line-delay"
			if NeedValue(index, arg, value) <> 0 then return 1
			if ParseSeconds(value, parsed) = 0 then
				return Die("--line-delay must be a non-negative number of seconds")
			end if
			opt.line_delay_ms = parsed
			index += 2

		case "--cmd"
			if NeedValue(index, arg, value) <> 0 then return 1
			if opt.command_count >= MAX_COMMANDS then
				return Die("too many --cmd options")
			end if
			opt.commands(opt.command_count) = value
			opt.command_count += 1
			index += 2

		case "--no-sync"
			opt.no_sync = -1
			index += 1

		case "--help", "-h"
			Usage()
			end 0

		case else
			return Die("unknown option: " & arg)
		end select
	loop

	if len(opt.telnet_host) <> 0 andalso len(opt.serial_port) <> 0 then
		return Die("--telnet and --serial are mutually exclusive")
	end if

	if len(opt.telnet_host) = 0 andalso len(opt.serial_port) = 0 then
		return Die("one of --telnet or --serial is required")
	end if

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

function ParseSeconds(byref text as string, byref milliseconds as integer) as integer
	dim seconds as double

	if len(text) = 0 then
		return 0
	end if

	seconds = val(text)
	if seconds < 0.0 then
		return 0
	end if

	if seconds > 2147483.0 then
		return 0
	end if

	milliseconds = cint(seconds * 1000.0)
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
	print "Usage: nuttx-remote-console [options]"
	print
	print "Options:"
	print "  --telnet HOST          connect to NSH telnetd"
	print "  --serial DEVICE        connect to a serial console"
	print "  --telnet-port PORT     telnet port, default: 23"
	print "  --baud RATE            serial baud rate, default: 115200"
	print "  --timeout SECONDS      command timeout, default: 30"
	print "  --startup-wait SECONDS wait after opening the transport"
	print "  --prompt TEXT          NSH prompt text, default: nsh>"
	print "  --char-delay SECONDS   delay between serial bytes, default: 0.015"
	print "  --line-delay SECONDS   delay before marker echo, default: 0.25"
	print "  --cmd TEXT             command to run, may be repeated"
	print "  --no-sync              skip initial prompt synchronization"
	print "  --help                 show this help text"
end sub

function Die(byref message as string) as integer
	print #err, "ERROR: " & message
	return 1
end function

function OpenConsole(byref opt as ConsoleOptions, byref file_num as integer, _
	byref is_telnet as integer) as integer
	dim spec as string

	file_num = freefile()

	if len(opt.telnet_host) <> 0 then
		is_telnet = -1
		spec = "host=" & opt.telnet_host & ",port=" & str(opt.telnet_port)

		if( OPEN TCP( spec AS #file_num ) <> 0 ) then
			return Die("OPEN TCP failed for " & opt.telnet_host & ":" & _
				str(opt.telnet_port) & ", ERR=" & str(err))
		end if

		return 0
	end if

	is_telnet = 0
	spec = BuildSerialSpec(opt)

	on error goto OpenSerialFailed
	open com spec as #file_num
	on error goto 0

	return 0

OpenSerialFailed:
	on error goto 0
	return Die("OPEN COM failed for " & spec & ", ERR=" & str(err))
end function

function SynchronizeConsole(byval file_num as integer, byval is_telnet as integer, _
	byref opt as ConsoleOptions) as integer
	dim text as string
	dim timeout_seconds as double = opt.timeout_seconds

	if timeout_seconds > 5.0 then
		timeout_seconds = 5.0
	end if

	WriteLineToConsole(file_num, is_telnet, "", opt.char_delay_ms)

	if ReadUntil(file_num, is_telnet, opt.prompt, timeout_seconds, text) = 0 then
		print text;
		return Die("console prompt was not seen during synchronization")
	end if

	print text;
	return 0
end function

function RunConsoleCommand(byval file_num as integer, byval is_telnet as integer, _
	byref opt as ConsoleOptions, byref command_text as string, byval index as integer) as integer
	dim marker as string
	dim text as string
	dim tail as string
	dim tail_timeout as double

	marker = "__fbxl_nuttx_done_" & right("0000" & str(index), 4) & "__"

	WriteLineToConsole(file_num, is_telnet, command_text, opt.char_delay_ms)
	SleepForMs(opt.line_delay_ms)
	WriteLineToConsole(file_num, is_telnet, "echo " & marker, opt.char_delay_ms)

	if ReadUntil(file_num, is_telnet, marker, opt.timeout_seconds, text) = 0 then
		print text;
		return Die("command did not finish before timeout: " & command_text)
	end if

	tail_timeout = opt.timeout_seconds
	if tail_timeout > 5.0 then
		tail_timeout = 5.0
	end if

	ReadUntil(file_num, is_telnet, opt.prompt, tail_timeout, tail)
	print text; tail;

	return 0
end function

sub WriteLineToConsole(byval file_num as integer, byval is_telnet as integer, _
	byref text as string, byval char_delay_ms as integer)
	dim i as integer

	if is_telnet <> 0 then
		print #file_num, text & chr(13) & chr(10);
		exit sub
	end if

	for i = 1 to len(text)
		print #file_num, mid(text, i, 1);
		SleepForMs(char_delay_ms)
	next

	print #file_num, chr(10);
end sub

function ReadUntil(byval file_num as integer, byval is_telnet as integer, _
	byref needle as string, byval timeout_seconds as double, byref out_text as string) as integer
	dim started_at as double = timer
	dim chunk as string

	out_text = ""

	do while ElapsedSeconds(started_at) < timeout_seconds
		chunk = ReadSome(file_num, is_telnet)
		if len(chunk) <> 0 then
			out_text &= chunk
			if len(needle) = 0 orelse instr(out_text, needle) <> 0 then
				return -1
			end if
		else
			SleepForMs(20)
		end if

		if eoc(file_num) <> 0 then
			exit do
		end if
	loop

	return 0
end function

function ReadSome(byval file_num as integer, byval is_telnet as integer) as string
	dim value as integer
	dim command_byte as integer
	dim option_byte as integer
	dim previous as integer
	dim buffer as string

	do while eof(file_num) = 0
		if ReadByteFromConsole(file_num, value, 0) = 0 then
			exit do
		end if

		if is_telnet = 0 orelse value <> TELNET_IAC then
			buffer &= chr(value)
			continue do
		end if

		if ReadByteFromConsole(file_num, command_byte, 1000) = 0 then
			exit do
		end if

		if command_byte = TELNET_IAC then
			buffer &= chr(TELNET_IAC)
		elseif command_byte = TELNET_WILL orelse command_byte = TELNET_WONT orelse _
			command_byte = TELNET_DO orelse command_byte = TELNET_DONT then
			if ReadByteFromConsole(file_num, option_byte, 1000) = 0 then
				exit do
			end if
			SendTelnetReply(file_num, command_byte, option_byte)
		elseif command_byte = TELNET_SB then
			previous = -1
			do
				if ReadByteFromConsole(file_num, value, 1000) = 0 then
					exit do
				end if
				if previous = TELNET_IAC andalso value = TELNET_SE then
					exit do
				end if
				previous = value
			loop
		end if
	loop

	return buffer
end function

function ReadByteFromConsole(byval file_num as integer, byref value as integer, _
	byval wait_ms as integer) as integer
	dim started_at as double = timer
	dim text as string
	dim wait_seconds as double = cdbl(wait_ms) / 1000.0

	do
		if eof(file_num) = 0 then
			text = input(1, #file_num)
			if len(text) <> 0 then
				value = asc(text, 1) and 255
				return -1
			end if
		end if

		if eoc(file_num) <> 0 then
			return 0
		end if

		if wait_ms = 0 orelse ElapsedSeconds(started_at) >= wait_seconds then
			return 0
		end if

		SleepForMs(10)
	loop
end function

sub SendTelnetReply(byval file_num as integer, byval command_byte as integer, _
	byval option_byte as integer)
	if command_byte = TELNET_WILL orelse command_byte = TELNET_WONT then
		print #file_num, chr(TELNET_IAC) & chr(TELNET_DONT) & chr(option_byte);
	else
		print #file_num, chr(TELNET_IAC) & chr(TELNET_WONT) & chr(option_byte);
	end if
end sub

function ElapsedSeconds(byval started_at as double) as double
	dim now_time as double = timer

	if now_time < started_at then
		now_time += 86400.0
	end if

	return now_time - started_at
end function

function BuildSerialSpec(byref opt as ConsoleOptions) as string
	dim serial_device as string = opt.serial_port

	if right(serial_device, 1) = ":" then
		return serial_device & str(opt.baud) & ",N,8,1,CS0,DS0,CD0,RS,BIN"
	end if

	return serial_device & ":" & str(opt.baud) & ",N,8,1,CS0,DS0,CD0,RS,BIN"
end function

function SleepForMs(byval milliseconds as integer) as integer
	if milliseconds > 0 then
		sleep milliseconds, 1
	end if
	return 0
end function

' end of nuttx-remote-console.bas
