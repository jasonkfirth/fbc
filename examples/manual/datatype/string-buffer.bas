'' examples/manual/datatype/string-buffer.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'STRING'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgString
'' --------

'' Variable-length strings as buffers

'' Reserving space for a string,
'' using Space() to produce lots of space characters (ASCII 32)
Dim As String mybigstring = Space(1024)
Dim As UInteger buffer_address = Cast(UInteger, StrPtr(mybigstring))
Print "buffer address: &h" & Hex(buffer_address, SizeOf(buffer_address) * 2) & ", length: " & Len(mybigstring)

'' Explicitly destroying a string
mybigstring = ""
buffer_address = Cast(UInteger, StrPtr(mybigstring))
Print "buffer address: &h" & Hex(buffer_address, SizeOf(buffer_address) * 2) & ", length: " & Len(mybigstring)
