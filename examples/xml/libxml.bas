'' FreeBASIC XML examples
'' File: libxml.bas
'' Purpose: Display XML reader nodes and report parse failure.
'' Responsibilities: Check nullable borrowed strings and release the reader.
'' This file contains no DOM ownership, entity substitution or concurrent readers.

#include once "libxml/xmlreader.bi"

Function main() As Integer
	Dim filename As String = Command(1)
	If Len(filename) = 0 Then
		Print "Usage: libxml filename"
		Return 1
	End If
	Dim reader As xmlTextReaderPtr = xmlReaderForFile(filename, NULL, XML_PARSE_NONET)
	If reader = NULL Then
		xmlCleanupParser()
		Print "Unable to open "; filename
		Return 1
	End If
	Dim result As Long = xmlTextReaderRead(reader)
	Do While result = 1
		'' ConstName and ConstValue borrow reader storage. Some nodes have no
		'' value or name, so do not dereference a missing pointer.
		Dim constname As Const ZString Ptr = xmlTextReaderConstName(reader)
		Dim value As Const ZString Ptr = xmlTextReaderConstValue(reader)
		Dim name_text As String, value_text As String
		If constname <> NULL Then name_text = *constname
		If value <> NULL Then value_text = *value
		Print xmlTextReaderDepth(reader); xmlTextReaderNodeType(reader); " "; name_text; _
		      xmlTextReaderIsEmptyElement(reader); xmlTextReaderHasValue(reader); value_text
		result = xmlTextReaderRead(reader)
	Loop
	xmlFreeTextReader(reader)
	'' Process-wide cleanup belongs after the last reader, not in a callback.
	xmlCleanupParser()
	If result <> 0 Then
		Print "failed to parse: "; filename
		Return 1
	End If
	Return 0
End Function

End main()

'' end of libxml.bas
