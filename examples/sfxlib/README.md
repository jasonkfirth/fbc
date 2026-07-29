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

`voice-effects-stress.bas` plays exact 4, 16, and 32 simultaneous-voice
stages. It combines four waveforms and envelope definitions, channel panning,
and the optional ping-pong echo from `sfxlib_effects.bi`. The program saves the
driver output as a WAV file and fails if the active driver reports an underrun.

`midi-synth-player.bas` reads Standard MIDI Files and synthesizes them entirely
through sfxlib. It supports merged format 0 and format 1 tracks, tempo changes,
running status, sustain, General MIDI program families, percussion, channel
volume and pan, and pitch bend at note onset. Unlike `midi-play.bas`, it does
not send events to an operating-system MIDI synthesizer. The optional second
argument selects an output WAV (`-` disables capture), the third limits the
render duration in seconds, and `noecho` as the fourth argument disables the
default ping-pong echo. The program reports score and mixer peak polyphony,
dropped notes, and driver underruns, which makes dense MIDI files useful as
repeatable mixer stress tests.

The built-in `MIDI OPEN`, `MIDI SEND`, and `MIDI PLAY` commands still prefer
the platform MIDI backend. If it cannot be opened, sfxlib now falls back to a
small 32-voice C FM synthesizer mixed through the normal audio output. The
fallback has an individual compact preset for every General MIDI program
number, common channel controls, and the standard channel 10 percussion range.
`SFXLIB_MIDI_DRIVER=fm` selects it explicitly.

For diagnostic runs that should not open the system audio device, set
`SFXLIB_DRIVER=null` before starting the program.  `SFXLIB_MIXER_DUMP` and
`SFXLIB_DRIVER_DUMP` can then record the generated float stream for FFT checks.

/* end of README.md */
