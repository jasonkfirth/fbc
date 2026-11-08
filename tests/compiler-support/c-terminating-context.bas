/'
Project: FreeBASIC checked C procedure context
File: c-terminating-context.bas
Purpose: Keep error-location calls and returning context restoration beside ASM.
Responsibilities: Check ordinary and early returns, then terminate in a callee.
This file intentionally does NOT run platform-specific assembler or fault probes.
'/
#lang "fb"

#include once "crt/string.bi"
#include once "c-opaque-nop.bi"

private sub returning_context(byval early_exit as long)
    if strcmp(erfn, "RETURNING_CONTEXT") <> 0 then end 91
    compiler_test_opaque_nop()
    if early_exit then exit sub
    if strcmp(ermn, "") = 0 then end 92
end sub

private sub terminating_context()
    if strcmp(erfn, "TERMINATING_CONTEXT") <> 0 then end 93
    if strcmp(ermn, "") = 0 then end 94
    compiler_test_opaque_nop()
    print "C_TERMINATING_CONTEXT_PASS"
    end 0
end sub

dim as string caller_function = *erfn
dim as string caller_module = *ermn
returning_context(0)
if *erfn <> caller_function or *ermn <> caller_module then end 95
returning_context(1)
if *erfn <> caller_function or *ermn <> caller_module then end 96
terminating_context()
end 97

'' end of c-terminating-context.bas
