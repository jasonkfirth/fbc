''
'' sfxlib basics 05: classic BASIC compatibility forms.
''
'' sfxlib also accepts several old BASIC sound spellings.  This example keeps
'' them together so new users can see which forms are compatibility syntax and
'' how they differ from the seconds-based modern commands.
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

print "Commodore 128 BASIC 7.0: VOL and SOUND voice, sid-frequency, ticks"
VOL 8
SOUND 1, 4096, 60

SLEEP 300, 1

print "BBC BASIC: SOUND channel, amplitude, pitch, duration"
SOUND 1, -15, 53, 20

SLEEP 300, 1

print "TI-99/4A Extended BASIC: CALL SOUND(duration-ms, frequency, volume)"
CALL SOUND(1000, 110, 0)

SLEEP 300, 1

print "TI-99/4A Extended BASIC can include three simultaneous tones."
CALL SOUND(500, 110, 0, 131, 0, 196, 3)

SLEEP 700, 1

print "Done."

'' end of basics-05-basic-compatibility.bas
