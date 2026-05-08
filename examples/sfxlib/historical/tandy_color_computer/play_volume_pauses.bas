''
'' Tandy TRS-80 Color Computer Extended Color BASIC PLAY pause example.
''
'' The manual changes the volume example by inserting P2 pauses between the
'' notes.  The original line 20 loops forever; this version stops after one
'' pass for the same run-safety reason as play_volume.bas.
''
'' Source:
''   https://colorcomputerarchive.com/repo/Documents/Manuals/Hardware/Getting%20Started%20With%20Extended%20Color%20Basic%20%28Tandy%29.pdf
''

#lang "qb"

5 CLS
10 PLAY "V5;A;P2; V10;A;P2; V15;A;P2; V20;A;P2; V25;A;P2; V30;A;P2"
20 END

'' end of play_volume_pauses.bas
