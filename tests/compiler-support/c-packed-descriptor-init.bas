/'
    Project: FreeBASIC C backend regression tests
    File: c-packed-descriptor-init.bas
    Purpose: Keep packed descriptor gaps out of ordinary initialized storage.
    Responsibilities: Check shared, static and local member defaults and strides.
    This file intentionally does NOT inspect padding bytes or use ERROR handlers.
'/
#lang "fb"

type InitializedTail field = 1
    prefix as ubyte = 17
    text as string = ""
    suffix as ubyte = 23
end type

dim shared global_tail as InitializedTail
dim shared global_array(0 to 1) as InitializedTail

private sub exercise()
    static saved as InitializedTail
    dim local_tail as InitializedTail

    if global_tail.prefix <> 17 or global_tail.suffix <> 23 then end 81
    if global_array(0).prefix <> 17 or global_array(0).suffix <> 23 then end 82
    if global_array(1).prefix <> 17 or global_array(1).suffix <> 23 then end 83
    if saved.prefix <> 17 or saved.suffix <> 23 then end 84
    if local_tail.prefix <> 17 or local_tail.suffix <> 23 then end 85
    if global_tail.text <> "" or global_array(1).text <> "" then end 86
    if saved.text <> "" or local_tail.text <> "" then end 87

    global_tail.text = "shared"
    saved.text = "static"
    local_tail.text = "local"
    if global_tail.text <> "shared" or saved.text <> "static" then end 88
    if local_tail.text <> "local" then end 89
end sub

exercise()
print "PACKED_DESCRIPTOR_INIT_PASS"
end 0

' end of c-packed-descriptor-init.bas
