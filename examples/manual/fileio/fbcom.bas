/'
    Project: FreeBASIC manual examples
    ----------------------------------

    File: fileio/fbcom.bas

    Purpose:

        Demonstrate portable modem-status and RTS control on an OPEN COM file.

    Responsibilities:

        - open a caller-selected serial device
        - print the status fields supported by its driver
        - optionally drive RTS on or off

    This file intentionally does NOT contain:

        - a platform-specific serial device name
        - automatic serial-port discovery
        - changes to unrequested modem-control lines

    Dialect and profile assumptions:

        This example uses the default FB dialect and the portable fbcom.bi API.
        The caller supplies an OPEN COM device string suitable for the target.

    Targets:

        Any runtime target with OPEN COM support. The reported capabilities
        identify which raw controls the selected backend implements.

    Resource ownership:

        The example owns the OPEN COM file number and closes it on every path
        after a successful open. The operating system owns the serial device.

    Serial stream:

        FOR BINARY selects unmodified byte I/O. This is a live serial stream,
        not a persistent binary file format with a magic value or version.

    Module API boundary:

        This is a command-line example and exports no reusable procedures.

    Usage:

        fbcom "COM1:115200,N,8,1" [rts=on|rts=off]
        fbcom "/dev/ttyUSB0:115200,N,8,1" [rts=on|rts=off]
'/

#include once "fbcom.bi"

using FB

dim as string port_spec = command( 1 )
dim as string operation = lcase( command( 2 ) )

if( port_spec = "" ) then
	print "Usage: fbcom <OPEN COM device> [rts=on|rts=off]"
	end 0
end if

dim as integer file_number = freefile
'' OPEN COM is a live byte stream, not a versioned binary disk format.
'' fblint: disable-next-line FBL-DOC-BIN-003 FBL-IO-009
open com port_spec for binary access read write as #file_number
if( err <> 0 ) then
	print "Could not open "; port_spec; ". Runtime error "; err; "."
	end 1
end if

dim as ComStatus status
dim as integer result = ComGetStatus( file_number, status )
if( result <> 0 ) then
	print "This serial backend does not expose raw status. Runtime error "; _
		result; "."
	close #file_number
	end 1
end if

print "Capabilities: &h"; hex( status.capabilities, 8 )
print "Lines:        &h"; hex( status.lines, 8 )
print "Errors:       &h"; hex( status.errors, 8 )
print "RX queued:    "; status.rx_queued
print "TX queued:    "; status.tx_queued

if( operation = "rts=on" orelse operation = "rts=off" ) then
	if( (status.capabilities and COM_CAP_OUTPUT_LINES) = 0 ) then
		print "This serial driver cannot drive RTS directly."
	else
		'' ULONG is the deliberate fixed 32-bit mask type used by fbcom.bi.
		'' fblint: disable-next-line FBL423
		dim as ulong values
		if( operation = "rts=on" ) then
			values = COM_LINE_RTS
		end if
		result = ComSetLines( file_number, COM_LINE_RTS, values )
		if( result <> 0 ) then
			print "RTS change failed with runtime error "; result; "."
		else
			print ucase( operation )
		end if
	end if
elseif( operation <> "" ) then
	print "Expected rts=on or rts=off."
end if

close #file_number

/' end of fbcom.bas '/
