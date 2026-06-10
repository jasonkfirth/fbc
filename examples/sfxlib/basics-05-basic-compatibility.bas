''
'' sfxlib basics 05: small BASIC compatibility forms.
''
'' sfxlib keeps a few high-impact old BASIC spellings where they are simple and
'' broadly recognizable.  The main command surface still uses the clearer
'' seconds-based generated sound forms shown in the earlier basics examples.
''

#lang "qb"

print
print "========================================"
print "SFXLIB BASICS 05: CLASSIC BASIC FORMS"
print "========================================"

print "IBM PC BASIC: SOUND frequency, ticks"
print "18 ticks is just under one second at the PC timer rate."
SOUND 262, 18

SLEEP 300, 1

print "Sinclair ZX Spectrum BASIC: BEEP seconds, semitones-from-middle-C"
BEEP 0.35, 0

SLEEP 500, 1

print "Done."

'' end of basics-05-basic-compatibility.bas
