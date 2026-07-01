/'
    Project: FreeBASIC test suite
    -----------------------------

    File: functions/gpiopins-header.bas

    Purpose:

        Verify that the shared GPIO pin include can be parsed and used by
        ordinary BASIC source.

    Responsibilities:

        - include the platform-neutral gpiopins.bi header
        - include the NuttX forwarding header to catch duplicate-declaration
          mistakes
        - compile references to each public GPIO pin function

    This file intentionally does NOT contain:

        - any runtime GPIO hardware access
        - board-specific pin numbers
        - NuttX device assumptions
'/

' TEST_MODE : COMPILE_ONLY_OK

#include once "nuttx/gpiopins.bi"
#include once "gpiopins.bi"

#assert GPIOPIN_INPUT = 0
#assert GPIOPIN_OUTPUT = 3
#assert GPIOPIN_HIGH = 1

private sub GpioPinsHeaderSmoke()
    dim as integer handle
    dim as integer value

    handle = GpioPinOpen(0)
    handle = GpioPinOpenDevice("/dev/gpio0")
    value = GpioPinRead(handle)
    value = GpioPinWrite(handle, GPIOPIN_HIGH)
    value = GpioPinGetType(handle)
    value = GpioPinSetType(handle, GPIOPIN_OUTPUT)
    value = GpioPinClose(handle)

    if value <> 0 then
        handle = value
    end if
end sub

' end of functions/gpiopins-header.bas
