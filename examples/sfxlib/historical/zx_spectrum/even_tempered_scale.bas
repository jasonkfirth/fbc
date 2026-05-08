''
'' Sinclair ZX Spectrum BASIC manual, even-tempered C major scale example.
''
'' The original listing uses BEEP duration,pitch.  sfxlib now accepts that
'' same form, with pitch measured in semitones above middle C.
''
'' Source:
''   https://worldofspectrum.org/ZXBasicManual/zxmanchap19.html
''

#lang "qb"

10 BEEP .5,0: BEEP .5,2: BEEP .5,4: BEEP .5,5: BEEP .5,7: BEEP .5,9: BEEP .5,11: BEEP .5,12: STOP

'' end of even_tempered_scale.bas
