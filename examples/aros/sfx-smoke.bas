''
'' FreeBASIC AROS sound smoke test
'' --------------------------------
''
'' File: sfx-smoke.bas
''
'' Purpose:
''
''     Isolate native AROS sfxlib initialization, playback, and teardown.
''
'' Responsibilities:
''
''     - start the preferred AROS sound backend
''     - submit a short generated tone
''     - terminate without graphics or input dependencies
''
'' This file intentionally does NOT contain:
''
''     - gfxlib2 initialization
''     - emulator orchestration
''     - interactive acceptance criteria
''

sound 880, 6
sleep 500

'' end of sfx-smoke.bas
