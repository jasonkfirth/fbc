'' Project: FreeBASIC compiler - compiler invocation and external tool pipeline
'' -----------------------------------------
''
'' File: driver/objinfo.bi
''
'' Purpose:
''
''     Define the streaming interface to object and archive metadata.
''
'' Responsibilities:
''
''     - own invocation state, input files, and output naming
''     - coordinate compilation, metadata, tool execution, and linking
''
'' This file intentionally does NOT contain:
''
''     - BASIC grammar or AST node implementation
''

#include once "support/containers/list.bi"

enum
	OBJINFO_LIB = 0
	OBJINFO_LIBPATH
	OBJINFO_MT
	OBJINFO_GFX
	OBJINFO_SFX
	OBJINFO_LANG
	OBJINFO_GFX3
	OBJINFO__COUNT
end enum

declare sub objinfoReadObj( byref objfile as string )
declare sub objinfoReadLibfile( byref libfile as string )
declare sub objinfoReadLib( byref libname as string, byval libpaths as TLIST ptr )
declare function objinfoReadNext( byref dat as string ) as integer
declare function objinfoGetFilename( ) as zstring ptr
declare sub objinfoReadEnd( )
declare function objinfoEncode( byval entry as integer ) as zstring ptr

'' end of driver/objinfo.bi
