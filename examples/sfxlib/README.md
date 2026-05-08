# sfxlib Examples

Start with the `basics-*` files if you are learning the API:

- `basics-01-generated-sound.bas` shows `SOUND`, `TONE`, `NOTE`, white noise, and pitched noise.
- `basics-02-play-strings.bas` shows foreground, background, and multi-channel `PLAY`.
- `basics-03-channels-mix.bas` shows master volume, channel volume, pan, and balance.
- `basics-04-instruments.bas` shows `WAVE`, `ENVELOPE`, and `INSTRUMENT`.
- `basics-05-basic-compatibility.bas` shows classic BASIC-compatible forms.

The one-command examples demonstrate individual commands in isolation.
`noise-pitch.bas` shows low, medium, and high noise update rates, and
`sfx-pitch.bas` shows one loaded sample played at several pitch ratios.  The
`showcase.bas` example walks through most command groups in one program.

The `historical` directory contains source-anchored examples from older BASIC
systems.  Those examples are stricter: they try to keep old listings unchanged
or explain exactly why a value had to be translated.

For diagnostic runs that should not open the system audio device, set
`SFXLIB_DRIVER=null` before starting the program.  `SFXLIB_MIXER_DUMP` and
`SFXLIB_DRIVER_DUMP` can then record the generated float stream for FFT checks.

/* end of README.md */
