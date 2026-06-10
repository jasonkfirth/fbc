# sfxlib Examples

Start with the `basics-*` files if you are learning the API:

- `basics-01-generated-sound.bas` shows `SOUND`, `TONE`, `NOTE`, white noise, and pitched noise.
- `basics-02-play-strings.bas` shows foreground, background, and multi-channel `PLAY`.
- `basics-03-channels-mix.bas` shows master volume, channel volume, pan, and balance.
- `basics-04-instruments.bas` shows `WAVE`, `ENVELOPE`, and `INSTRUMENT`.
- `basics-05-basic-compatibility.bas` shows the small retained compatibility forms.

`composer-grid.bas` is a mouse-driven gfxlib sketchpad for placing sine,
square, triangle, saw, pitched-noise, and loaded wave-file sounds on a
scrollable timeline. It can load and save its plain text song layout and
export the current song as a WAV file through the raw output-capture helper.

The one-command examples demonstrate individual commands in isolation.
`noise-pitch.bas` shows low, medium, and high noise update rates, and
`sfx-pitch.bas` shows one loaded sample played at several pitch ratios.  The
`raw-write.bas` example shows direct sample writes through `sfxlib_raw.bi`;
the same opt-in include also exposes output-side WAV recording helpers.
The `showcase.bas` example walks through most command groups in one program.

For diagnostic runs that should not open the system audio device, set
`SFXLIB_DRIVER=null` before starting the program.  `SFXLIB_MIXER_DUMP` and
`SFXLIB_DRIVER_DUMP` can then record the generated float stream for FFT checks.

/* end of README.md */
