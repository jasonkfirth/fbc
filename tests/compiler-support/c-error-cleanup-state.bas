/'
    Project: FreeBASIC C ERROR regression tests
    File: c-error-cleanup-state.bas
    Purpose: Keep handled ERR visible while abandoned worker resources retire.
    Responsibilities: Check arrays, strings, destructors and nested handlers.
    This file intentionally does NOT qualify partial construction or native jumps.
'/
#lang "fb"

dim shared constructions as integer
dim shared destructions as integer

type Tracked
    value as integer
    declare constructor()
    declare destructor()
end type

constructor Tracked()
    value = 41
    constructions += 1
end constructor

destructor Tracked()
    if value <> 41 then end 81
    destructions += 1
    ' User cleanup may use APIs which change ERR, just as array erasure does.
    err = 99
end destructor

private sub worker(byval kind as integer, byval number as integer)
    dim instance as Tracked
    dim text as string = "retired worker string"
    dim numbers() as integer
    dim words() as string
    dim fixed_words(0 to 1) as string
    fixed_words(0) = text
    select case kind
    case 1
        redim numbers(-2 to 2)
        numbers(0) = instance.value
    case 2
        redim words(3 to 5)
        words(4) = text
    case 3
        redim numbers(0 to 2)
        redim words(0 to 2)
        words(1) = text
        erase numbers
        erase words
    end select
    error number
end sub

private function inner_owner(byval kind as integer) as integer
    on local error goto caught
    worker(kind, 47)
    return -1
caught:
    return err
end function

private function owner(byval kind as integer, byval nested as integer, _
                       byval change_number as integer) as integer
    on local error goto caught
    worker(kind, 24)
    return -1
caught:
    if err <> 24 then end 82
    if nested then
        if inner_owner(kind) <> 47 then end 83
        ' Successful REDIM/ERASE sets ERR before the nested throw saves it.
        ' Cleanup must preserve that state as well as the original error.
        if err <> iif(kind = 0, 24, 0) then end 84
    end if
    if change_number then err = 55
    return err
end function

for iteration as integer = 0 to 7
    for kind as integer = 0 to 3
        for mode as integer = 0 to 2
            dim expected as integer = iif(mode = 2, 55, 24)
            if mode = 1 and kind <> 0 then expected = 0
            dim result as integer = owner(kind, mode = 1, mode = 2)
            dim retained_error as integer = err
            if result <> expected or retained_error <> expected then end 85
        next
    next
next
if constructions <> 128 or destructions <> constructions then end 86
print "CLEANUP_STATE_LANGUAGE_PASS"
end 0

' end of c-error-cleanup-state.bas
