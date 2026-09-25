# libopenmpt examples for FreeBASIC

These examples port the in-memory loading and raw-PCM output patterns from
libopenmpt's canonical C examples. They use the C API binding in `inc/libopenmpt.bi`
and do not require PortAudio.

Build from the FreeBASIC source tree:

```sh
fbc -i inc examples/libopenmpt/libopenmpt_example_c_mem.bas
fbc -i inc examples/libopenmpt/libopenmpt_example_c_stdout.bas
```

When cross-compiling for Windows from this source tree, add the matching import
library directory and configure a Windows cross-toolchain:

```sh
fbc -target win32 -i inc -p contrib/winlibs-legacy/lib/win32 examples/libopenmpt/libopenmpt_example_c_mem.bas
fbc -target win64 -i inc -p contrib/winlibs-legacy/lib/win64 examples/libopenmpt/libopenmpt_example_c_mem.bas
```

`libopenmpt_example_c_mem` accepts a module filename and an optional WAVE output
filename. It renders 48 kHz, stereo, signed 16-bit PCM and defaults to
`module.wav`.

`libopenmpt_example_c_stdout` accepts a module filename and writes raw,
48 kHz, stereo, signed 16-bit native-endian PCM to standard output. On a
little-endian Linux system it can be played with:

```sh
./libopenmpt_example_c_stdout song.mod | aplay --file-type raw --format=S16_LE --channels=2 --rate=48000
```

Both examples read the complete input module into memory before creating the
libopenmpt module. This keeps the samples independent of platform-specific
`FILE` callbacks and makes their memory ownership explicit. The input buffer is
released as soon as libopenmpt has loaded the module. Playback is limited to
one pass; repeat count zero means one play, not infinite repeat.
The example rejects input files of 2 GiB or larger, and the WAVE writer stops
before the classic RIFF 4 GiB size limit.

The examples use libopenmpt's default 16-bit dither. Rendered samples may differ
slightly between runs, so compare the output format and length rather than the
raw file hash.

The imported Windows DLLs are not bundled with the examples. See
[`contrib/winlibs-legacy/lib/libopenmpt.txt`](../../contrib/winlibs-legacy/lib/libopenmpt.txt)
for the required runtime files. The upstream BSD-3-Clause license is in
`contrib/winlibs-legacy/lib/libopenmpt.LICENSE.txt`.

The official examples are documented in the [libopenmpt C API overview](https://lib.openmpt.org/doc/libopenmpt_c_overview.html).
