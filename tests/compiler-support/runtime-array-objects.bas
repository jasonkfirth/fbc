/'
    Project: FreeBASIC runtime regression tests
    File: runtime-array-objects.bas
    Purpose: Check actual object-array construction and reverse destruction.
    Responsibilities: Exercise PRESERVE, ERASE, automatic cleanup and managed fields.
    This file intentionally does NOT exercise cross-frame ERROR unwinding.
'/
#lang "fb"

dim shared as integer next_id, constructions, destructions, expected_last

private sub require(byval condition as integer, byval marker as integer)
    if condition = 0 then
        print "OBJECT_ARRAY_LANGUAGE_FAIL "; marker
        end 1
    end if
end sub

type Tracked
    id as integer
    payload as string
    declare constructor()
    declare destructor()
end type

constructor Tracked()
    id = next_id
    next_id += 1
    constructions += 1
    payload = "object:" + str(id)
end constructor

destructor Tracked()
    require(id = expected_last, 1)
    require(payload = "object:" + str(id), 2)
    expected_last -= 1
    destructions += 1
end destructor

private sub reset_counts()
    next_id = 1
    constructions = 0
    destructions = 0
    expected_last = 0
end sub

private sub dynamic_lifetimes()
    dim values() as Tracked
    redim values(-3 to 1)
    require(constructions = 5 and destructions = 0, 3)
    expected_last = 5
    redim preserve values(-3 to -1)
    require(constructions = 5 and destructions = 2 and expected_last = 3, 4)
    require(values(-3).id = 1 and values(-1).id = 3, 5)
    erase values
    require(destructions = 5 and expected_last = 0, 6)
    erase values
    require(destructions = 5, 7)
end sub

private sub fixed_lifetimes()
    dim values(4 to 6) as Tracked
    require(constructions = 3 and destructions = 0, 8)
    expected_last = 3
    erase values
    require(constructions = 6 and destructions = 3 and expected_last = 0, 9)
    require(values(4).id = 4 and values(6).id = 6, 10)
    expected_last = 6
    '' Scope cleanup must retire the three reconstructed elements exactly once.
end sub

reset_counts()
dynamic_lifetimes()
require(constructions = 5 and destructions = 5, 11)
reset_counts()
fixed_lifetimes()
require(constructions = 6 and destructions = 6 and expected_last = 3, 12)
print "OBJECT_ARRAY_LANGUAGE_PASS"
end 0

' end of runtime-array-objects.bas
