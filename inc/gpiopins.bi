'/*
'    Project: FreeBASIC GPIO pin support
'    -----------------------------------
'
'    File: gpiopins.bi
'
'    Purpose:
'
'        Provide a small BASIC-facing API for digital GPIO pins.
'
'    Responsibilities:
'
'        - expose common GPIO pin mode constants
'        - expose digital low/high value constants
'        - declare the runtime functions used to open, read, write, and
'          configure GPIO pins
'        - keep board-specific pin tables and device discovery out of user
'          programs
'
'    This file intentionally does NOT contain:
'
'        - Raspberry Pi, RP2350, ESP32, or PC-specific pin numbering tables
'        - pin mux setup for individual boards
'        - interrupt callback helpers
'        - direct operating-system header bindings
'*/

#ifndef __gpiopins_bi__
#define __gpiopins_bi__

' These values are the common GPIO modes used by the first NuttX backend.
' Other backends should map unsupported modes to a clean runtime failure
' instead of silently changing the requested electrical behavior.
const GPIOPIN_INPUT                    = 0
const GPIOPIN_INPUT_PULLUP             = 1
const GPIOPIN_INPUT_PULLDOWN           = 2
const GPIOPIN_OUTPUT                   = 3
const GPIOPIN_OUTPUT_OPENDRAIN         = 4
const GPIOPIN_INTERRUPT                = 5
const GPIOPIN_INTERRUPT_HIGH           = 6
const GPIOPIN_INTERRUPT_LOW            = 7
const GPIOPIN_INTERRUPT_RISING         = 8
const GPIOPIN_INTERRUPT_FALLING        = 9
const GPIOPIN_INTERRUPT_BOTH           = 10
const GPIOPIN_INTERRUPT_WAKEUP         = 11
const GPIOPIN_INTERRUPT_HIGH_WAKEUP    = 12
const GPIOPIN_INTERRUPT_LOW_WAKEUP     = 13
const GPIOPIN_INTERRUPT_RISING_WAKEUP  = 14
const GPIOPIN_INTERRUPT_FALLING_WAKEUP = 15
const GPIOPIN_INTERRUPT_BOTH_WAKEUP    = 16

const GPIOPIN_LOW  = 0
const GPIOPIN_HIGH = 1

extern "C"
    declare function GpioPinOpen alias "fb_GpioPinOpen" _
        (byval pin as integer) as integer

    declare function GpioPinOpenDevice alias "fb_GpioPinOpenDevice" _
        (byval device as zstring ptr) as integer

    declare function GpioPinClose alias "fb_GpioPinClose" _
        (byval handle as integer) as integer

    declare function GpioPinRead alias "fb_GpioPinRead" _
        (byval handle as integer) as integer

    declare function GpioPinWrite alias "fb_GpioPinWrite" _
        (byval handle as integer, byval value as integer) as integer

    declare function GpioPinGetType alias "fb_GpioPinGetType" _
        (byval handle as integer) as integer

    declare function GpioPinSetType alias "fb_GpioPinSetType" _
        (byval handle as integer, byval pin_type as integer) as integer
end extern

#endif

' end of gpiopins.bi
