'' Project: FreeBASIC DOS compiler regressions
'' File: tls-unsupported.bas
'' Purpose: Reject TLS forms without a supported DOS C representation.
'' Responsibilities: Check managed storage and the native C TLS ABI guard.
'' This file does not produce a runnable program.

Thread Local Dim Shared unsupportedString As String

Extern "C"
	Thread Local Extern unsupportedExternal As Long
End Extern

'' end of tls-unsupported.bas
