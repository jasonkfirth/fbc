'' Project: FreeBASIC compiler driver
'' -----------------------------------------
''
'' File: driver/fbc-platform.bas
''
'' Purpose:
''
''     Provide the driver target hook implementations in one translation unit.
''
'' Responsibilities:
''
''     - own the target hook table and its dispatch functions
''     - include the per-target toolchain and library policies
''
'' This file intentionally does NOT contain:
''
''     - host arithmetic policy or BASIC grammar
''

#include once "driver/fbc-private.bi"

#include once "driver/fbc-platform.bi"

'' end of driver/fbc-platform.bas
