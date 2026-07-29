' TEST_MODE : MULTI_MODULE_TEST

#lang "fblite"

/'
'    FreeBASIC compiler regression test
'    ----------------------------------
'
'    File: backend-global-mangling-main.bas
'
'    Purpose:
'        Verify that BASIC global names emitted by one backend can be
'        referenced by a module emitted by another backend.
'
'    This file intentionally does not define the shared variables.
'/

extern backend_global_unsuffixed as integer
extern backend_global_suffixed%

if( backend_global_unsuffixed <> 123 ) then
	end 1
end if

if( backend_global_suffixed% <> 456 ) then
	end 1
end if

end 0

' end of backend-global-mangling-main.bas
