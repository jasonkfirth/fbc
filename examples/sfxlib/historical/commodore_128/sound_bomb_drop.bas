''
'' Commodore 128 BASIC 7.0 SOUND sweep example.
''
'' The C128 System Guide describes this SOUND line as a downward sawtooth
'' sweep, like a falling bomb sound effect.  A short VOL line is included
'' because the guide notes that SOUND is silent until the chip volume is set.
''
'' Source:
''   https://www.commodore.ca/manuals/128_system_guide/sect-07a.htm
''

#lang "qb"

10 VOL 5
100 SOUND 1,49152,240,1,0,100,1,0

'' end of sound_bomb_drop.bas
