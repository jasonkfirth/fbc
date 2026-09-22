'' examples/manual/meta/header.bi
''
'' Example extracted from the FreeBASIC Manual
'' from topic '$INCLUDE'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgMetaInclude
'' --------

#ifndef FB_EXAMPLES_MANUAL_META_HEADER_BI
#define FB_EXAMPLES_MANUAL_META_HEADER_BI

' header.bi file
Type FooType
	Bar As Byte
	Barbeque As Byte
End Type
Dim Foo As FooType

#endif
