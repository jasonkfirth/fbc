''
'' Commodore 128 BASIC 7.0 SOUND example.
''
'' The C128 System Guide introduces SOUND with this two-line program.  The
'' listing body is unchanged: VOL sets the chip-wide volume, then SOUND plays
'' voice 1 at frequency-control value 4096 for 60 sixtieths of a second.
''
'' Source:
''   https://www.commodore.ca/manuals/128_system_guide/sect-07a.htm
''

#lang "qb"

10 VOL 5
20 SOUND 1,4096,60

'' end of sound_beep.bas
