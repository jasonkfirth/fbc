'' Project: FreeBASIC semantic sidecar tests
'' File: effects.bas
'' Purpose: Select compiler-owned runtime families without executing their I/O.
'' Responsibilities: Allocation, strings, arrays, math, files, graphics and sound.
'' This file intentionally does NOT use an alias spelling as effect evidence.
#lang "fb"

declare function UnregisteredAllocator alias "malloc"(byval bytes as uinteger) as any ptr
declare function UnregisteredMath alias "fb_SGNd"(byval value as double) as long

sub EffectSelections(byval value as double)
    dim memory as any ptr = allocate(16)
    memory = reallocate(memory, 32)
    deallocate(memory)
    memory = UnregisteredAllocator(16)
    dim unknown_result as long = UnregisteredMath(value)
    dim indirect as function(byval as uinteger) as any ptr = @UnregisteredAllocator
    memory = indirect(16)
    dim text as string = "abc"
    text += "def"
    mid(text, 1, 1) = "Z"
    dim unicode_text as ustring = "hello"
    unicode_text[0] = 65
    dim values() as integer
    redim values(0 to 2)
    values(1) = 7
    dim file_number as long = freefile
    open "unexecuted-effect-fixture.tmp" for output as #file_number
    print #file_number, text
    close #file_number
    dim mutex_handle as any ptr = mutexcreate()
    mutexlock(mutex_handle)
    mutexunlock(mutex_handle)
    mutexdestroy(mutex_handle)
    randomize value
    dim computed as double = atn(value) + rnd(value)
    screenres 16, 16
    pset (0, 0), rgb(1, 2, 3)
    beep 0.01, 440
    print computed, unknown_result, lbound(values)
end sub

sub EffectOpaqueAssembly()
    asm
        nop
    end asm
end sub

print "effects-ok"

'' end of effects.bas
