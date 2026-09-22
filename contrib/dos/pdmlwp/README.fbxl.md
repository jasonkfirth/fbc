# PDMLWP C core for the optional FreeBASIC DOS runtime

Imported from PDMLWP 0.3 by Paolo De Marino, based on Sengan Short and Josh
Turpen's work. Source package:
https://ftp5.gwdg.de/pub/msdos/gcc/djgpp/v2tk/pdmlwp03.zip

Archive SHA-256:
`3f3f5f420b794722d33b9dc7b3e35d31e7a6ca697ef3db536de6c3d03e4a4931`

Imported files: src/lwp.c, src/lwpasm.s, include/lwp.h, and original license,
history, readme and credit files. Only the C/assembly scheduler is built.
The C++ wrapper and old dpmiint.s replacement are excluded; modern DJGPP
supplies the interrupt bridge. LWP_TCP means cleanup, not TCP/IP.

## Local changes, September 2026

The integration repairs allocation-lock addresses and failure cleanup,
stack sizes/alignment, initial x87 preservation, errno/DPMI error isolation,
missing-id kill handling, and assembly code-lock boundaries. RTC-only
initialization checks ownership, saves/restores RTC/PIC state, and checks
interrupt setup failures. C dispatch establishes ES=DS and clears DF after
saving interrupted state, which is necessary when preempting DMA copies.
Build flags prevent source-marker reordering and LTO. XMM/SSE is unsupported.

The optional fast RTC consumer has a private locked stack and an integer-only
callback contract. A rate divider preserves scheduler time slices while queued
PC speaker samples use the extra ticks. Registration checks RTC ownership and
removal restores the base rate. Deferred synthetic signals recheck scheduling
exclusion at dispatch; rtlib services deferred ticks after leaving a critical
section. The provider itself contains no mixer or speaker-port code.

Sleep deadlines now use unsigned RTC scheduling ticks, with upward rounding
and bounded chunks for counter wrap. The speaker divider preserves this clock
at the base scheduling rate. Dispatch no longer calls ftime or enters DOS
calendar conversion. The manual signal return also restores DJGPP's previous
exception-frame pointer before leaving the current frame.

Runtime wrappers, TLS and synchronization live in src/rtlib/dos. Sound is in
src/sfxlib/dos. See [the provider notes](../../../docs/freedos-providers.md).

## License and credits

The original source specifies GNU Library General Public License version 2
or later, with an additional requirement to credit everyone in THANKS.
Preserve [copying.lib](copying.lib), [copying](copying), original notices and
[thanks](thanks). This license is separate from FreeBASIC's runtime license;
do not apply the FreeBASIC runtime linking exception to these imported files.

For statically linked distributions, follow the license requirements for
library source, modification notices and materials needed to relink with a
modified library. The standalone builder retains library objects; application
distributions must retain their relinkable objects and complete link command.
Runtime installation carries this source tree, notices and standalone builder
beside libfbpdmlwp.a in pdmlwp-source, preserving the builder's relative paths.

Credit to Paolo De Marino and everyone listed in THANKS: Josh Turpen,
Sengan Short, Malcolm Taylor, Charles Sandmann, DJ Delorie, Chih-Hao Tsai,
Eli Zaretskii, Douglas Eleveld, and Paul Cunningham.

<!-- end of README.fbxl.md -->
