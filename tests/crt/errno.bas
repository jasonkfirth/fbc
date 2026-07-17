' TEST_MODE : COMPILE_AND_RUN_OK

''
'' FreeBASIC CRT errno binding test
'' --------------------------------
''
'' File: tests/crt/errno.bas
''
'' Purpose:
''
''     Verify Haiku's thread-local errno accessor and status_t constants.
''
'' Responsibilities:
''
''     - include crt/errno.bi exactly as user programs do
''     - make a failing CRT call set errno
''     - compare errno with the platform EBADF value
''
'' This file intentionally does NOT contain:
''
''     - socket data-transfer coverage
''     - errno tables for unrelated platforms
''

#ifdef __FB_HAIKU__

#include once "crt/errno.bi"
#include once "crt/sys/socket.bi"

errno = 0
if closesocket(-1) = 0 then end 1
if errno <> EBADF then end 2

#endif

'' end of tests/crt/errno.bas
