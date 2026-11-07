/'
Project: FreeBASIC C startup declarations
File: c-startup-declarations.bas
Purpose: Check registration ownership without changing callback identities.
Responsibilities: Module entries, singleton/group priorities, aliases and cleanup.
This file intentionally does NOT depend on same-priority callback order.
'/
#lang "fb"

'' A distinct value makes the module-to-main CDECL call observable.
const STARTUP_MODULE_VALUE = 2468

#ifdef STARTUP_MODULE_PROBE
extern "c"
    declare sub StartupModuleMark alias "startup_module_mark"(byval value as long)
end extern
'' Executable module-level code creates the implicit startup entry.
StartupModuleMark(STARTUP_MODULE_VALUE)
#else
dim shared as long startup_stage, startup_mask, startup_module_seen

sub StartupModuleMark cdecl alias "startup_module_mark"(byval value as long)
    if value <> STARTUP_MODULE_VALUE then end 107
    startup_module_seen += 1
end sub

private sub FirstStartup() constructor 101
    if startup_stage <> 0 then end 91
    startup_stage = 1
end sub

private sub SecondStartup() constructor 102
    if startup_stage <> 1 then end 92
    startup_stage = 2
end sub

private sub GroupFirstStartup() constructor 103
    if startup_stage <> 2 then end 93
    startup_mask or= 1
end sub

private sub GroupSecondStartup() constructor 103
    if startup_stage <> 2 then end 94
    startup_mask or= 2
end sub

private sub BoundStartup cdecl alias "startup.native.init"() constructor 104
    if startup_stage <> 2 or startup_mask <> 3 then end 95
    startup_stage = 3
end sub

sub PublicStartup() constructor 105
    if startup_stage <> 3 then end 96
    startup_stage = 4
end sub

private sub DefaultStartup() constructor
    if startup_stage <> 4 then end 97
    startup_stage = 5
end sub

private sub DefaultCleanup() destructor
    if startup_stage <> 6 then end 98
    startup_stage = 7
end sub

sub PublicCleanup() destructor 105
    if startup_stage <> 7 then end 99
    startup_stage = 8
end sub

private sub BoundCleanup cdecl alias "startup.native.cleanup"() destructor 104
    if startup_stage <> 8 then end 100
    startup_stage = 9
end sub

private sub GroupFirstCleanup() destructor 103
    if startup_stage <> 9 then end 101
    startup_mask or= 4
end sub

private sub GroupSecondCleanup() destructor 103
    if startup_stage <> 9 then end 102
    startup_mask or= 8
end sub

private sub SecondCleanup() destructor 102
    if startup_stage <> 9 or startup_mask <> 15 then end 103
    startup_stage = 10
end sub

private sub FirstCleanup() destructor 101
    if startup_stage <> 10 then end 104
    print "C_STARTUP_DECLARATIONS_CLEANUP_PASS"
end sub

#ifdef STARTUP_NORETURN_PROBE
'' Object-only qualification: a terminating callback prevents group combining.
private sub TerminatingStartup() constructor 106
    end 105
end sub

private sub LaterStartup() constructor 106
    startup_stage = 99
end sub
#endif

if startup_module_seen <> 1 then end 108
if startup_stage <> 5 then end 106
startup_stage = 6
print "C_STARTUP_DECLARATIONS_PASS"
#endif

'' end of c-startup-declarations.bas
