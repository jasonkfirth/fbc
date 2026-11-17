# Wii checked system randomness

File: `docs/wii-secure-random.md`

The Wii implementation of `FBC.secureRandom` can obtain system bytes from a
qualified Dolphin IOS extension. Stock Dolphin and physical Wii systems without
that extension return `FB.SECURE_RANDOM_UNAVAILABLE`. This provider does not
claim that a console clock, serial number, network packet or saved seed is an
entropy source. Physical-console entropy remains a separate platform task.

The public runtime entry validates the destination and the one MiB request
limit. It clears the entire valid destination after any provider failure.
The Wii provider opens `/dev/dolphin`, checks the capability reply, and reads
at most 4096 bytes per IOS transfer. Each call owns its descriptor and aligned
scratch allocation, which is wiped before release. Concurrent calls share no
generator state or scratch memory.

The extension uses private ioctlv identifiers rather than consuming the next
upstream Dolphin operation number:

| Operation | Identifier | Input vectors | Output vectors |
| --- | --- | --- | --- |
| Capabilities | `0x584c5201` | none | one, exactly 16 bytes |
| Read bytes | `0x584c5202` | none | one, 1 through 4096 bytes |

The capability reply is a byte layout, independent of host endianness:

| Bytes | Contents |
| --- | --- |
| 0 through 7 | ASCII `XLRNG001` |
| 8 through 9 | Big-endian maximum transfer, 4096 |
| 10 through 11 | Big-endian flags, 1 means checked host system CSPRNG |
| 12 through 15 | Reserved, zero |

The runtime requires this exact version and profile. An unknown operation,
missing device or mismatched reply is unavailable support, never permission to
fall back to another generator. A read failure is an I/O error.

The Windows qualification extension calls `BCryptGenRandom` with the system
preferred RNG and checks its status before publishing bytes to guest memory.
It has no emulated generator state to restore from a save state. Dolphin's
normal nondeterministic-operation guard rejects these operations in deterministic
sessions. Other host platforms need their own checked implementation before
advertising the capability reply.

Qualification must cover successful calls, transfers crossing the IPC chunk
boundary, invalid arguments, stock-emulator refusal and actual TLS consumption.
Variation between two samples is a useful wiring check; it does not establish
the quality of an entropy source by itself.

<!-- end of wii-secure-random.md -->
