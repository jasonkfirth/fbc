'/*
'    Project: FreeBASIC NuttX GPIO pin support
'    -----------------------------------------
'
'    File: nuttx/gpiopins.bi
'
'    Purpose:
'
'        Provide the NuttX-facing include path for the common FreeBASIC
'        GPIO pin API.
'
'    Responsibilities:
'
'        - include the shared gpiopins.bi API
'        - document that NuttX maps GpioPinOpen(n) to /dev/gpioN
'        - keep board-specific pin numbering outside the shared header
'
'    This file intentionally does NOT contain:
'
'        - duplicated GPIO declarations
'        - board pin maps
'        - NuttX ioctl structure bindings
'*/

#ifndef __nuttx_gpiopins_bi__
#define __nuttx_gpiopins_bi__

#include once "../gpiopins.bi"

#endif

' end of nuttx/gpiopins.bi
