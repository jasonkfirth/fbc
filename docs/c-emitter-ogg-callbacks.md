<!--
Project: FreeBASIC compiler verification
File: c-emitter-ogg-callbacks.md
Purpose: Record the FBSound memory-callback contract repair and its qualification.
Responsibilities: Explain SDK types, cursor arithmetic, ownership and retained warnings.
This file intentionally does NOT claim full audio-loader or campaign qualification.
-->

# FBSound Ogg memory callbacks

Two selected FBSound profiles cast the signed-integer read callback to
libvorbis's unsigned `size_t` callback type. Inspection also found that the
callback ignored the element size, seek returned a position instead of a status,
end-relative seeks ignored their offset, close reported failure and Win64 tell
could narrow a native index into C's 32-bit long.

The private callbacks in `C:/fb_corpus/fbsound/src/fbsound.bas` now follow the
`ov_callbacks` contract:

- Read uses native unsigned element sizes, counts and return values. It bounds
  the byte count by the remaining input before multiplying, copies any final
  partial element and returns the number of complete elements.
- Read uses CRT `memcpy`, including for one byte. FBSound's CPU Copy wrapper
  requires a count greater than one and otherwise leaves the byte uncopied.
- Seek uses start, current or one-past-end origins, checks the offset before
  adding it, returns zero on success and preserves the cursor on rejection.
- Close succeeds without freeing caller-owned input storage.
- Tell returns minus one when the cursor cannot fit in the target C long.
  The loader rejects file sizes outside the native signed index range before
  allocating the memory view.

Each decoder has its own cursor. Calls operating on the same decoder require
caller serialization. The fixture uses synchronous SDK calls and never opens
an audio device.

## Native and SDK evidence

`build/c-emitter-callback-contracts-20261102` retains the exact production
callback bodies, before and after, and the original CPU module used by the
historical copy control. The SDK program includes the actual libvorbis header
and links pinned static libvorbisfile, libvorbis and libogg archives. Its 46
SDK headers are bound through both compilers' dependency closures.

Both C emitters, GCC and Clang, GNU11/GNU23 and O0/O2 pass 16 native cases.
They check complete and partial elements, zero-sized reads, EOF, cursor bounds,
start/current/end seeks, return values, unchanged positions after rejected seeks,
C-long narrowing and caller ownership. A real 2.4 MiB corpus Ogg file is opened,
decoded, reset with `ov_pcm_seek` and decoded again. The replayed PCM matches.
The fixture frees its input only after clearing the decoder and copies no asset
into the audit stage.

Five separate historical controls fail at their expected checks under each
emitter/compiler pair, for 20 observed failures: element-sized read, one-byte
copy, end seek, close status and Win64 tell narrowing. The old read controls use
ordinary small requests and stop at the first failed assertion.

Forty callback objects, twenty CPU objects and twenty SDK objects pass, including
their Win32 checks. Eight additional whole-source Win32 objects pass for the
static and dynamic FBSound profiles with explicit `NO_ASM`. Eight foreign callback
objects pass for Linux x86_64, ARMv7, AArch64 and Darwin AArch64. These object
checks establish neither Win32 execution nor foreign runtime behavior.

The first ARM replay retained an x86-only `-asm intel` option and was rejected
before emission. The corrected foreign recipes omit that option. Completed
native commands, fragments, source, SDK and runtime inputs were preserved and
verified before reusing their receipts; all eight foreign objects then passed.
The original rejected command remains in the database's resolved harness ledger.

This is callback and SDK-decoding qualification. The public loader's complete
temporary-WAV conversion, plugin/device lifecycle and audio playback remain
outside the native fixture.

## Current selected diagnostics

| Queue | Profiles | GCC warnings | Clang warnings |
| --- | ---: | ---: | ---: |
| Current compiler | 224 | 0 | 4,802 |
| Unit tests / fbcunit | 703 | 16 | 14,347 |
| Production corpus/projects | 1,117 | 485 | 11,087 |
| Additional projects | 239 | 1,805 | 41,133 |
| Total | 2,283 | 2,306 | 71,369 |

The two affected production C outputs are freshly checked under the original
aggressive, unsuppressed flags. GCC loses two unused-parameter observations.
Clang loses those two observations and two incompatible callback casts, and
adds two raw-buffer pointer-arithmetic observations at the bounded `memcpy`
calls. These additions remain counted; exact emitted expressions are retained
in `retained-buffer-observations.json`. No other diagnostic group changes.

All 2,057 other project C outputs are re-emitted and byte-identical. The compiler
source and executable remain unchanged from their 224-unit fixed-point check.
Twelve PNG profiles stay excluded.

## Preservation and remaining work

Only `fbsound/src/fbsound.bas` changes in the corpus. Exact inverse edits restore
the preimage, including unrelated whitespace and original LF endings. Source
lint retains its 307 whitespace findings and one existing large-file finding,
with zero errors or new findings. The generated repaired callback fragment has
zero lint findings. Installed compiler/runtime hashes and unrelated dirty work
remain unchanged. The retained stage has a 32 MiB cap.

Five selected Clang function-cast occurrences remain: the JPEG C-long callback
and four Win32 generic GetProcAddress conversions. They remain enabled and
counted. Broader pointer and descriptor contracts, nonlocal ERROR ownership,
public CRT compatibility, unselected contexts and semantic lint qualification
remain tracked in [current status](c-emitter-current-status.md).

<!-- end of c-emitter-ogg-callbacks.md -->
