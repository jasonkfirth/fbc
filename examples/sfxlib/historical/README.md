# Historical BASIC Sound Examples

These examples are ports of old BASIC sound listings that are already close to
the sfxlib API.

The rule for this directory is deliberately strict:

- First choice: keep the original listing body essentially unchanged.
- Second choice: improve sfxlib when an old sound API maps cleanly to a modern
  FreeBASIC form without breaking existing programs.
- Last resort: make a small edit and explain the exact difference in the file.

Current source families:

- IBM PC BASICA/GW-BASIC/QBasic 1.1/QuickBASIC/PDS `SOUND frequency,duration`
- IBM PC QuickBASIC 4.5 `PLAY`
- Amiga BASIC `SOUND frequency,duration,volume,voice`
- BBC BASIC `SOUND channel,amplitude,pitch,duration`
- Commodore 128 BASIC 7.0 `VOL` and `SOUND`
- MSX BASIC multi-channel `PLAY`
- Sinclair ZX Spectrum BASIC `BEEP duration,pitch`
- Tandy TRS-80 Color Computer Color BASIC `SOUND`, with the documented
  pitch-code converted to hertz because IBM PC BASIC owns the ambiguous
  `SOUND a,b` spelling.
- Tandy TRS-80 Color Computer Extended Color BASIC `PLAY`
- TI-99/4A Extended BASIC `CALL SOUND(duration,frequency,volume,...)`

For collisions, the IBM PC Microsoft BASIC family is the hard compatibility
rule: BASICA, GW-BASIC, QBasic 1.1, QuickBASIC 4.5, and PDS 7.1 own the
ambiguous forms.  Other machines are added directly when their syntax is
distinct, or with a small note in the example when a value has to be translated.

/* end of README.md */
