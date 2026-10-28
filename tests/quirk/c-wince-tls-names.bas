'' TEST_MODE : COMPILE_AND_RUN_OK
'' Project: FreeBASIC compiler regression tests
'' File: c-wince-tls-names.bas
'' Purpose: Preserve ordinary and exported TLS names containing dollar signs.
'' Responsibilities: Compile and access initialized private and public TLS.
'' This file intentionally does NOT test platform TLS descriptor internals.

#if not defined( __FB_JS__ ) and ((not defined( __FB_DOS__ )) or ( __FB_MT__ <> 0 ))
thread local dim shared private_value as long = 7
public thread local dim shared exported_value alias "fbcTlsValue$" as long = 11

if private_value <> 7 then end 1
if exported_value <> 11 then end 2
private_value = 13
exported_value = 17
if private_value <> 13 then end 3
if exported_value <> 17 then end 4
#endif

'' end of c-wince-tls-names.bas
