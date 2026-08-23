/'
    Project: FreeBASIC portable serial-port control API
    ---------------------------------------------------

    File: fbcom.bi

    Purpose:

        Expose modem-control lines and low-level state for ports opened with
        OPEN COM without requiring platform-specific headers.

    Responsibilities:

        - define portable modem-line, capability, error, and purge flags
        - describe the fixed-width status returned by ComGetStatus
        - declare control functions implemented by the FreeBASIC runtime

    This file intentionally does NOT contain:

        - serial-port discovery
        - baud rate, parity, or framing configuration handled by OPEN COM
        - direct access to platform-native handles

    Dialect and profile assumptions:

        The public namespace is used in the default FB dialect. The declarations
        use fixed-width Long values and are portable to every runtime target.

    Targets:

        All FreeBASIC runtime targets. Capability bits distinguish native serial
        controls from targets whose runtime cannot expose them.

    Module API boundary:

        ComGetStatus, ComSetLines, ComSetBreak, and ComPurge operate only on a
        file number already opened by OPEN COM.
'/

#ifndef __fbcom_bi__
#define __fbcom_bi__

#if defined( __FB_XBOX__ ) or _
    (defined( __FB_WIN32__ ) and defined( __FB_X86__ ) and _
     not defined( __FB_64BIT__ ))
	#define __FB_COM_FBCALL stdcall
#else
	#define __FB_COM_FBCALL cdecl
#endif

#if __FB_LANG__ = "qb"
	#define __FB_COM_ULONG __ulong
#else
	#define __FB_COM_ULONG ulong
#endif

#if __FB_LANG__ = "fb"
namespace FB
#endif

	'' Modem input and output lines reported in ComStatus.lines.
	const as __FB_COM_ULONG _
		COM_LINE_CTS = &h00000001, _
		COM_LINE_DSR = &h00000002, _
		COM_LINE_DCD = &h00000004, _
		COM_LINE_RI  = &h00000008, _
		COM_LINE_RTS = &h00000010, _
		COM_LINE_DTR = &h00000020

	'' Operations and status fields supported by the current device.
	const as __FB_COM_ULONG _
		COM_CAP_INPUT_LINES  = &h00000001, _
		COM_CAP_OUTPUT_LINES = &h00000002, _
		COM_CAP_BREAK        = &h00000004, _
		COM_CAP_PURGE_RX     = &h00000008, _
		COM_CAP_PURGE_TX     = &h00000010, _
		COM_CAP_RX_QUEUE     = &h00000020, _
		COM_CAP_TX_QUEUE     = &h00000040, _
		COM_CAP_ERRORS       = &h00000080

	'' Pending line errors reported in ComStatus.errors.
	const as __FB_COM_ULONG _
		COM_ERROR_BREAK       = &h00000001, _
		COM_ERROR_FRAMING     = &h00000002, _
		COM_ERROR_PARITY      = &h00000004, _
		COM_ERROR_OVERRUN     = &h00000008, _
		COM_ERROR_RX_OVERFLOW = &h00000010

	'' Receive and transmit queues accepted by ComPurge.
	const as __FB_COM_ULONG _
		COM_PURGE_RX = &h00000001, _
		COM_PURGE_TX = &h00000002

	''
	'' Every field has a fixed 32-bit representation on all FreeBASIC targets.
	'' A capability bit is set only when the associated value or operation is
	'' available through the current platform and serial-device driver.
	''
	type ComStatus
		capabilities as __FB_COM_ULONG
		lines as __FB_COM_ULONG
		errors as __FB_COM_ULONG
		rx_queued as __FB_COM_ULONG
		tx_queued as __FB_COM_ULONG
	end type

	#if __FB_LANG__ <> "qb"
	extern "C"
	#endif
		'' Query an already-open OPEN COM file number. Unsupported status fields
		'' remain zero and have no corresponding capability bit.
		declare function ComGetStatus __FB_COM_FBCALL alias "fb_ComGetStatus" _
			( byval file_number as integer, byref status as ComStatus ) as integer

		'' Set only the output lines selected by mask. The mask may contain RTS,
		'' DTR, or both; values contains the desired high lines.
		declare function ComSetLines __FB_COM_FBCALL alias "fb_ComSetLines" _
			( byval file_number as integer, byval mask as __FB_COM_ULONG, _
			  byval values as __FB_COM_ULONG ) as integer

		'' Assert or clear the transmitter break condition.
		declare function ComSetBreak __FB_COM_FBCALL alias "fb_ComSetBreak" _
			( byval file_number as integer, byval enabled as integer ) as integer

		'' Discard queued input, queued output, or both.
		declare function ComPurge __FB_COM_FBCALL alias "fb_ComPurge" _
			( byval file_number as integer, _
			  byval queues as __FB_COM_ULONG ) as integer
	#if __FB_LANG__ <> "qb"
	end extern
	#endif

#if __FB_LANG__ = "fb"
end namespace
#endif

#endif

#undef __FB_COM_FBCALL
#undef __FB_COM_ULONG

/' end of fbcom.bi '/
