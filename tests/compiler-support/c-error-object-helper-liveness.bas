/'
    Project: FreeBASIC C continuation ownership qualification
    File: c-error-object-helper-liveness.bas
    Purpose: Keep an unused retirement helper out of a terminating procedure.
    Responsibilities: Cover qualified scalar storage beside a guarded resource.
    This file intentionally does NOT unwind after END or qualify union ownership.
'/
#lang "fb"

type PlainState
    ready as integer
    declare constructor()
    declare destructor()
end type

constructor PlainState()
    ready = -1
end constructor

destructor PlainState()
    ready = 0
end destructor

type GuardedState
    union
        small as long
        large as longint
    end union
    text as string
end type

private sub worker()
    dim state as PlainState
    dim guarded as GuardedState
    if state.ready <> -1 then end 91
    print "OBJECT_HELPER_LIVENESS_PASS"
    end 0
end sub

worker()

' end of c-error-object-helper-liveness.bas
