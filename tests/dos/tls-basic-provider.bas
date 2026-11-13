'' Project: FreeBASIC DOS compiler regressions
'' File: tls-basic-provider.bas
'' Purpose: Supply initialized BASIC TLS templates for another module.
'' Responsibilities: Export initialized scalar storage and a TLS accessor.
'' This file does not create threads or expose native C TLS storage.

Public Thread Local Dim Shared dosTlsValue As Long = 17
Public Thread Local Dim Shared dosTlsWide As ULongInt = &h8877665544332211ULL
Private Thread Local Dim Shared moduleTlsValue As Long = 41

Function dosTlsRead() As Long
	Return dosTlsValue
End Function

Function dosTlsPrivateRead() As Long
	Return moduleTlsValue
End Function

'' end of tls-basic-provider.bas
