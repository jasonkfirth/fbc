/'
    Project: FreeBASIC C continuation qualification
    File: c-error-local-regions.bas
    Purpose: Keep region-local values automatic without losing observable captures.
    Responsibilities: Check handler reads, escaped slots, region boundaries and balance.
    This file intentionally does NOT qualify opaque code or concurrent execution.
'/
#lang "fb"

declare sub allocation_begin cdecl alias "allocation_begin"()
declare function allocation_end cdecl alias "allocation_end"() as long
declare sub finish_tls cdecl alias "finish_tls"()
declare sub region_save_slot cdecl alias "region_save_slot"(byval slot as integer ptr)
declare function region_read_slot cdecl alias "region_read_slot"() as integer
declare sub region_clear_slot cdecl alias "region_clear_slot"()
declare function region_identity cdecl alias "region_identity"(byval number as integer) as integer

private function local_values() as integer
    on local error goto caught
#ifdef TEST_OPAQUE
    asm
        nop
    end asm
#endif
    dim first as integer
    dim second as integer
    dim combined as integer
    first = 17
    second = 19
    combined = first * second + 29
    if combined <> 352 then error 24
    return 352
caught:
    return -1
end function

private function across_region() as integer
    on local error goto caught
    dim retained as integer
    retained = 71
    goto following
following:
    return retained
caught:
    return -1
end function

private function across_call() as integer
    on local error goto caught
    dim retained as integer
    retained = 51
    if region_identity(3) <> 3 then error 24
    return retained
caught:
    return -1
end function

private function handler_value() as integer
    on local error goto caught
    dim retained as integer
    retained = 47
    error 24
    return -1
caught:
    return retained
end function

private function escaped_value() as integer
    on local error goto caught
    dim retained as integer
    dim saved_address as integer ptr
    retained = 83
    saved_address = @retained
    region_save_slot(saved_address)
    error 24
    return -1
caught:
    dim result as integer = region_read_slot()
    region_clear_slot()
    return result
end function

private function default_value() as integer
    on local error goto caught
    dim retained as integer
    retained += 3
    return retained
caught:
    return -1
end function

private sub check_values()
    if local_values() <> 352 then end 81
    if across_region() <> 71 then end 82
    if across_call() <> 51 then end 83
    if handler_value() <> 47 then end 84
    if escaped_value() <> 83 then end 85
    if default_value() <> 3 then end 86
end sub

check_values()
finish_tls()
allocation_begin()
for repetition as integer = 0 to 63
    check_values()
next
finish_tls()
if allocation_end() <> 0 then end 87
print "LOCAL_REGION_VALUES_PASS"
end 0

' end of c-error-local-regions.bas
