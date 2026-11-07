/'
Project: FreeBASIC C identifier qualification
File: c-reserved-names.bas
Purpose: Preserve native bindings while using ordinary generated C identifiers.
Responsibilities: Check initialization hooks and expose object-only export probes.
This file intentionally does NOT invoke the foreign exports or open a window.
'/
#lang "fb"

dim shared as long initialization_stage

private sub __construc() constructor
    initialization_stage = 1
end sub

private sub __destruc() destructor
    if initialization_stage <> 2 then end 91
    print "C_RESERVED_DESTRUCTOR_PASS"
end sub

#ifdef C_RESERVED_EXPORT_PROBE
'' This branch is compiled as an object only. Its undefined symbols are compared
'' with the predecessor, including Win32 calling-convention decoration.
extern "c"
    declare function _fileno(byval stream as any ptr) as long
    declare function _get_osfhandle(byval descriptor as long) as integer
    declare function _setmode(byval descriptor as long, byval mode as long) as long
    declare function __caca0_get_event(byval mask as ulong, byval timeout as long) as ulong
    declare function _initwin_1(byval width as long, byval height as long) as long
    declare function _initwin_2(byval width as long, byval height as long, byval title as zstring ptr) as long
    declare function __lzo_init_v2(byval version as ulong, byval short_size as long, _
        byval int_size as long, byval long_size as long, byval uint32_size as long, _
        byval uint_size as long, byval dictionary_size as long, byval string_size as long, _
        byval pointer_size as long, byval callback_size as long) as long
end extern
extern "windows"
    declare function __WSAFDIsSet(byval descriptor as uinteger, byval descriptors as any ptr) as long
end extern

function reserved_export_probe cdecl alias "reserved_export_probe"(byval stream as any ptr) as integer
    dim descriptor as long = _fileno(stream)
    dim result as integer = _get_osfhandle(descriptor)
    result += _setmode(descriptor, 0)
    result += __caca0_get_event(1, -1)
    result += _initwin_1(320, 200)
    result += _initwin_2(320, 200, "probe")
    result += __lzo_init_v2(0, 2, 4, 4, 4, 4, 4, 4, 4, 4)
    result += __WSAFDIsSet(0, stream)
    return result
end function
#endif

if initialization_stage <> 1 then end 92
initialization_stage = 2
print "C_RESERVED_NAMES_PASS"

'' end of c-reserved-names.bas
