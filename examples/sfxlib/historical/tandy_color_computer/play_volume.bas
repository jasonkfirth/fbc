''
'' Tandy TRS-80 Color Computer Extended Color BASIC PLAY volume example.
''
'' The manual's program repeats with line 20 GOTO 10 until BREAK is pressed.
'' This example stops after one pass so automated runs finish cleanly and do
'' not leave sound playing after the process exits.
''
'' Source:
''   https://colorcomputerarchive.com/repo/Documents/Manuals/Hardware/Getting%20Started%20With%20Extended%20Color%20Basic%20%28Tandy%29.pdf
''

#lang "qb"

5 CLS
10 PLAY "V5;A; V10;A; V15;A; V20;A; V25;A; V30;A"
20 END

'' end of play_volume.bas
