''
'' GW-BASIC SOUND reference example.
''
'' The GW-BASIC User's Guide gives this short random-sound loop:
''
''   2500 SOUND RND*1000+37, 2
''   2600 GOTO 2500
''
'' The original loops forever.  This version keeps the SOUND line unchanged
'' and changes the unconditional GOTO into a small counted repeat so the demo
'' finishes by itself.
''
'' Source:
''   https://hwiegman.home.xs4all.nl/gw-man/SOUND.html
''

#lang "qb"

2490 C = C + 1
2500 SOUND RND*1000+37, 2
2600 IF C < 20 THEN GOTO 2490

'' end of sound_random.bas
