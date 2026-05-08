''
'' BBC BASIC SOUND reference example.
''
'' BBC BASIC uses:
''
''   SOUND channel, amplitude, pitch, duration
''
'' The manual identifies pitch 53 as middle C and duration 20 as one second.
'' Amplitude -15 is the loudest normal volume.
''
'' Source:
''   https://www.riscos.com/support/developers/bbcbasic/part2/sound.html
''

#lang "qb"

10 SOUND 1,-15,53,20

'' end of sound_middle_c.bas
