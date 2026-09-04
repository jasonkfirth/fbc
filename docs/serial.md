# Portable serial control with fbcom.bi

`OPEN COM` continues to open and own a serial byte stream. The new `fbcom.bi`
header adds portable status and modem-line operations for an already open file
number without exposing a Win32 handle, Unix file descriptor, or another
platform-specific object.

## Basic use

```freebasic
#include once "fbcom.bi"
using FB

dim as integer port = freefile
open com "COM1:115200,N,8,1" for binary as #port
if err <> 0 then end 1

dim as ComStatus status
if ComGetStatus( port, status ) = 0 then
    print "RX queued:"; status.rx_queued
    print "TX queued:"; status.tx_queued
end if

close #port
```

On Unix-like systems the `OPEN COM` device can be a path such as
`/dev/ttyUSB0:115200,N,8,1`. Device naming and the available baud rates still
belong to the target's `OPEN COM` backend.

## Status record

Every `ComStatus` field is an unsigned 32-bit value on every target:

| Field | Meaning |
| --- | --- |
| `capabilities` | Which remaining fields and operations the current device supports |
| `lines` | Current CTS, DSR, DCD, RI, RTS, and DTR state |
| `errors` | Pending break, framing, parity, overrun, and receive-overflow errors |
| `rx_queued` | Bytes waiting in the receive queue |
| `tx_queued` | Bytes waiting in the transmit queue |

Check a capability before interpreting a status field or requesting an
operation. An unsupported field remains zero, which is not the same as a
supported input line currently being low.

The capability flags are `COM_CAP_INPUT_LINES`, `COM_CAP_OUTPUT_LINES`,
`COM_CAP_BREAK`, `COM_CAP_PURGE_RX`, `COM_CAP_PURGE_TX`, `COM_CAP_RX_QUEUE`,
`COM_CAP_TX_QUEUE`, and `COM_CAP_ERRORS`.

## Operations

- `ComGetStatus(file_number, status)` fills the status record.
- `ComSetLines(file_number, mask, values)` changes only the selected RTS and/or
  DTR output lines. A selected bit present in `values` is driven high.
- `ComSetBreak(file_number, enabled)` asserts or clears the transmitter break
  condition.
- `ComPurge(file_number, queues)` discards receive data, queued transmit data,
  or both. Use `COM_PURGE_RX` and `COM_PURGE_TX`.

The functions return zero on success and a nonzero runtime error on failure.
They reject file numbers that are closed, are not `OPEN COM` streams, or request
an operation the current backend cannot perform.

`COM_LINE_CTS`, `COM_LINE_DSR`, `COM_LINE_DCD`, and `COM_LINE_RI` describe
modem input. `COM_LINE_RTS` and `COM_LINE_DTR` describe controllable output.
The error flags are `COM_ERROR_BREAK`, `COM_ERROR_FRAMING`, `COM_ERROR_PARITY`,
`COM_ERROR_OVERRUN`, and `COM_ERROR_RX_OVERFLOW`.

## Portability rule

The same API is implemented by the supported DOS, Windows/Windows CE,
Unix-family, AROS, RISC OS, and NuttX serial backends. Capabilities vary with
the operating system and device driver. Code must branch on the returned
capability mask instead of assuming that every USB adapter or emulated serial
port can drive modem lines or report queue lengths.

The complete runnable example is
[`examples/manual/fileio/fbcom.bas`](../examples/manual/fileio/fbcom.bas).

<!-- end of serial.md -->
