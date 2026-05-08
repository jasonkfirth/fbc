''
'' TRS-80 Color Computer Color BASIC SOUND example.
''
'' The Color Computer Operation Manual gives this immediate-mode example:
''
''   SOUND 39, 10
''
'' On the CoCo, the first value is a pitch-code, not hertz, and the second
'' value is counted in .06 second units.  sfxlib gives the ambiguous
'' two-argument SOUND form to IBM PC BASIC compatibility, where the first value
'' is hertz and the second is 18.2 Hz timer ticks.
''
'' Therefore this demo keeps the same audible result by changing the CoCo pitch
'' code 39 to middle C in hertz, and changing 10 CoCo duration ticks to about
'' 11 IBM/GW-BASIC timer ticks.
''
'' Source:
''   https://www.colorcomputers.com/trs-80_color_computer_operations_manual
''

#lang "qb"

10 SOUND 262, 11

'' end of sound_middle_c.bas
