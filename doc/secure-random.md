<!-- FreeBASIC runtime; File: secure-random.md.
Purpose: Describe the checked randomness API, providers and failure contract.
Responsibilities: Public ABI, platform prerequisites and verification boundaries.
This document does not qualify applications or estimate entropy statistically. -->

# Checked secure randomness

Include `fbrandom.bi` in the `fb` dialect and call
`FBC.secureRandom(destination, byte_count)`. This is a runtime-library API;
the declaration works with the existing compiler and needs no parser change.
The selected runtime must contain the new `fb_SecureRandom` entry point.
Including the header alone does not add that symbol to an installed library.

```freebasic
#lang "fb"
#include once "fbrandom.bi"

dim token(0 to 31) as ubyte
dim status as long = FBC.secureRandom(@token(0), sizeof(token))
if status <> FB.SECURE_RANDOM_OK then
    '' Stop the operation that required a secret.
    end 1
end if
```

The caller owns a writable span of `byte_count` bytes. A non-null pointer is
not a proof that an arbitrary address is writable. Requests are limited to
`FB.SECURE_RANDOM_MAX_BYTES`, currently 1,048,576 bytes. `UInteger` matches the
target's pointer-sized `size_t`; `Long` is the 32-bit status type.

| Status | Meaning and output |
| --- | --- |
| `FB.SECURE_RANDOM_OK` (0) | Every requested byte was supplied. |
| `FB.SECURE_RANDOM_UNAVAILABLE` (1) | The target/provider is absent or not ready. The complete valid span is zeroed. |
| `FB.SECURE_RANDOM_IO_ERROR` (2) | The provider, transfer or cleanup failed. The complete valid span is zeroed, including any partial result. |
| `FB.SECURE_RANDOM_INVALID_ARGUMENT` (3) | The count exceeds the limit, or a nonempty request has a null destination. No memory is changed. |

A zero-length request succeeds, including with a null destination, and does
not establish provider availability. There is no partial-success return.
Provider errors other than the defined unavailable status become I/O errors.
The common entry point clears failures with volatile stores, outside any
platform-specific cleanup path. It does not allocate a result buffer or change
the BASIC `ERR` value.

The API neither seeds nor uses `RND`. Separate caller-owned buffers may be used
concurrently. The DOS provider holds the existing runtime scheduler exclusion
lock while using DJGPP's shared real-mode transfer area. A caller must not
simultaneously modify or read the same output span from another thread.

## Providers

| Target | Source and refusal behavior |
| --- | --- |
| Windows | CNG `BCryptGenRandom` with the system-preferred generator. Load `bcrypt.dll` from the system directory with restricted DLL search. Missing provider support is unavailable; an unsuccessful CNG call or cleanup fails the whole request. |
| Linux / Android | The `getrandom` system call with `GRND_NONBLOCK`, in at most 256-byte chunks. An unready pool or missing syscall is unavailable. Interrupted calls have a bounded retry count. |
| FreeBSD 12 or later | `getrandom` with `GRND_NONBLOCK`, with the same complete-read and retry rules. Older versions do not acquire a weaker fallback. |
| Haiku x86-64 | Check CPUID for RDRAND, then check carry on every instruction with at most ten attempts. Compare consecutive full 64-bit words, including a discarded first word for short requests. Missing CPU support is unavailable; a failed or repeated result fails the request. |
| Extended DOS | Open the `XLRNG$` character device, verify its control protocol, then perform exact reads of at most 4096 bytes. An ordinary file with that name is rejected. The qualified emulator/host is a prerequisite. |
| Other native targets | The generic provider reports unavailable. A platform build or security qualification is required before claiming additional target support. |

The Haiku provider relies on the CPU's DRNG. Its repeat check detects a stuck
source; it does not attest hardware or prove cryptographic strength. No
provider falls back to timer seeds, a software PRNG, a user seed file or `RND`.

Microsoft documents the CNG call and its system-preferred mode in
[BCryptGenRandom](https://learn.microsoft.com/en-us/windows/win32/api/bcrypt/nf-bcrypt-bcryptgenrandom).
Intel documents CPUID, carry handling and bounded retries in the
[DRNG software implementation guide](https://www.intel.com/content/www/us/en/developer/articles/guide/intel-digital-random-number-generator-drng-software-implementation-guide.html).
The implementation uses these platform facilities directly, without defining
a new cryptographic generator.

## DOS control protocol

`INT 21h`, `AX=4402h`, reads the device control channel into `DS:DX`. The caller
requests exactly 16 bytes in `CX` and requires carry clear with `AX=16`.
Device-information bits 7 and 14 must identify a character device with IOCTL
support before this request is made.

| Bytes | Encoding |
| --- | --- |
| 0..7 | ASCII `XLRNG001`, protocol version one |
| 8..9 | Little-endian maximum read, 4096 |
| 10..11 | Little-endian flags, 1 = checked host system CSPRNG |
| 12..15 | Reserved, all zero |

The capability reply describes the provider contract. A later read can still
fail. The runtime validates the entire reply, rejects short reads and closes
the handle on every opened path. The emulator device is read-only and accepts
neither guest seed material nor control writes. Its Windows host provider
uses CNG; it retains no seed, generator state or open handle between calls.
Stock DOS or an unextended emulator continues to report unavailable.

DOS reads use `INT 21h`, `AH=3Fh`, directly into the checked real-mode
workspace. The runtime copies the complete result to the caller and clears the
largest workspace span used before releasing its scheduler lock. This avoids
leaving a secret in a CRT read buffer. A shorter final block must not leave
bytes from an earlier block behind. `dos-scratch.bas` observes this cleanup,
including a 4097-byte request split into a full block and one final byte.

## Building and checking

The source graph selects `sys_secure_random.c` and exactly one
`sys_hsecure_random.c` provider using the existing generic/Unix/target override
rules. The controlled-failure provider under `tests/secure-random` is test-only
and is outside the runtime source graph.

`build_scripts/test-secure-random.py` compiles private runtime objects and two
small programs using an explicitly selected existing SDK. For example:

```text
python3 -B build_scripts/test-secure-random.py --compiler /path/to/fbc/bin/fbc --cc cc --output /fresh/owned/results
```

The live case checks argument limits, output guards, complete fills and four
concurrent callers. The separate controlled-provider case checks clearing
after a partial failure, unavailable results and invalid provider statuses.
`live.bas --unavailable` checks refusal and complete clearing on a target
without a provider. `dos-device.c` checks the real DOS device's capabilities,
malformed control lengths, maximum read, rejected writes/seek and guard bytes.
Sample comparisons are sanity checks, not statistical entropy certification.

Qualification receipts identify source/tool hashes and actual test exits.
Explicit test objects do not install or replace `libfb`/`libfbmt`; applications
need a deliberately selected runtime containing the new objects. The runtime
keeps the project's LGPL license and static-linking exception described in
`readme.txt`. The separate DOSBox-X extension keeps DOSBox-X's GPL license.

`build_scripts/stage-secure-random.py` can prepare a private MT archive from a
successful API receipt and an existing SDK archive. It checks every original
member's name, order and contents, adds only the two API objects, and links a
test without explicit API objects. The installed SDK stays in place. Select
the archive the compiler actually links: Haiku's executable path uses
`libfbmtpic.a`, so that overlay builds PIC objects with `-fPIC`. Other tested
executable profiles use `libfbmt.a`. These candidate overlays do not qualify
every runtime build variant or install a compiler update.

<!-- end of secure-random.md -->
