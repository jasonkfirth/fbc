' TEST_MODE : MULTI_MODULE_TEST

#lang "fblite"

/'
'    FreeBASIC compiler regression test
'    ----------------------------------
'
'    File: backend-global-mangling-provider.bas
'
'    Purpose:
'        Define suffixed and unsuffixed BASIC globals for the mixed-backend
'        mangling regression test.
'
'    This file intentionally does not contain the test driver.
'/

extern backend_global_unsuffixed as integer
dim backend_global_unsuffixed as integer = 123

extern backend_global_suffixed%
dim backend_global_suffixed% = 456

' end of backend-global-mangling-provider.bas
