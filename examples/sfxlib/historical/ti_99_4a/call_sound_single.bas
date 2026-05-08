''
'' TI-99/4A Extended BASIC CALL SOUND example.
''
'' The TI Extended BASIC manual gives this example:
''
''   100 CALL SOUND(1000,110,0)
''
'' On the TI, the first value is duration in milliseconds, followed by
'' frequency and volume.  Volume 0 is loudest and 30 is softest.
''
'' Source:
''   https://99er.net/files/TI%20Extended%20Basic%20-%20Linked.pdf
''

#lang "qb"

100 CALL SOUND(1000,110,0)

'' end of call_sound_single.bas
