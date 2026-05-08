''
'' Amiga BASIC SOUND reference example.
''
'' The Amiga BASIC manual documents:
''
''   SOUND frequency, duration [, [volume] [, voice]]
''
'' and gives this example for A440 at volume 100 on audio channel 0.  sfxlib
'' accepts the same four-argument form; duration is interpreted as Amiga BASIC
'' timer ticks, where 18.2 ticks is about one second.
''
'' Source:
''   https://archive.org/details/AmigaBasic/
''

#lang "qb"

SOUND 440,20,100,0

'' end of sound_a440.bas
