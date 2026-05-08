''
'' TI-99/4A Extended BASIC three-voice CALL SOUND example.
''
'' The TI Extended BASIC manual gives this example:
''
''   100 CALL SOUND(500,110,0,131,0,196,3)
''
'' It plays three tones together for half a second.  The listing body is kept
'' unchanged so the example exercises the same duration/frequency/volume pairs
'' used on the original machine.
''
'' Source:
''   https://99er.net/files/TI%20Extended%20Basic%20-%20Linked.pdf
''

#lang "qb"

100 CALL SOUND(500,110,0,131,0,196,3)

'' end of call_sound_three_voices.bas
