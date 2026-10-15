''
'' Project: FreeBASIC compiler examples
'' File: dylib.bas
'' Purpose: Load mydll at runtime, call AddNumbers, and print the result.
'' Responsibilities: resolve the DXE symbol and release the loaded library.
'' This file intentionally does NOT contain mydll's implementation.
''
'' Compile as: fbc dylib.bas
'' (mydll must be compiled first)
''

dim library as any ptr

'' Function pointer through which the function is going to be called
dim AddNumbers as function( byval a as integer, byval b as integer ) as integer

'' Windows and Linux discover their platform library suffixes. DOS requires
'' the complete DXE filename so the loader can find the module.
#if defined( __FB_DOS__ )
library = dylibload( "mydll.dxe" )
#else
library = dylibload( "mydll" )
#endif
if( library = 0 ) then
	print "Cannot load the mydll dynamic library"
	sleep
	end 1
end if

'' Here the exact function name exported by the DLL must be given:
'' If ALIAS "..." was used then it is the exact specified alias name,
'' otherwise it's the normal name but all UPPERCASED (BASIC name mangling).
'' (EXTERN ... END EXTERN blocks also change name mangling modes if used)
'' DOS DXE symbols also require a leading underscore.
#if defined( __FB_DOS__ )
addnumbers = dylibsymbol( library, "_AddNumbers" )
#else
addnumbers = dylibsymbol( library, "AddNumbers" )
#endif
if( addnumbers = 0 ) then
	print "Could not get AddNumbers()'s address from mydll library"
	end 1
end if

randomize( timer( ) )

dim as integer x = rnd * 10
dim as integer y = rnd * 10

print x; " +"; y; " ="; AddNumbers( x, y )

'' Done with the library. The OS will automatically clean up, but we can also
'' unload earlier (before process exit) if it's no longer needed.
'' Once it's unloaded, all function pointers to it become invalid!
dylibfree library

'' end of dylib.bas
