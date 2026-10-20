<!--
Project: FreeBASIC runtime library
File: drivers/README.md
Purpose: Map native runtime layers and explain their replacement contract.
Responsibilities: Describe platform ownership, shared layers and porting rules.
This file intentionally does NOT contain portable BASIC runtime policy.
-->

# Runtime drivers

This directory contains the runtime's platform and architecture dependencies.
The [portable subsystems](../README.md) own BASIC semantics, descriptors and
shared lifetime rules. Drivers adapt those contracts to native APIs. These are
not dynamically loaded plugins: Make selects implementations before compiling.

| Directory | Owner |
| --- | --- |
| `unix` | Shared Unix console, process, file, signal and pthread implementations |
| `linux`, `android` | Linux and Android replacements; Android selects its own layer |
| `freebsd`, `netbsd`, `openbsd`, `dragonfly` | BSD-specific native APIs |
| `darwin`, `haiku`, `solaris` | Darwin, Haiku and Solaris replacements; illumos uses Solaris |
| `amiga`, `aros`, `riscos` | Amiga, AROS and RISC OS adapters using the shared Unix layer where selected |
| `win32` | Windows implementations, also selected for Cygwin |
| `wince` | Windows CE implementations and architecture-specific CRT support |
| `dos` | DOS services, interrupts, threading and native startup overrides |
| `js` | Emscripten/browser adapters |
| `nuttx` | NuttX adapters and the reduced embedded runtime used by smoke builds |
| `wii`, `xbox` | Console-specific runtime services |
| `x86` | Shared 32-bit x86 assembly helpers |

For each C filename, source selection prefers `OS/architecture`, then `OS`,
then `unix` where enabled, then a portable subsystem. A same-name file replaces
the complete lower-layer translation unit. Preserve that filename when moving
or splitting code, or explicitly revise the selection contract. Assembly is
selected separately and may not share an archive member name with selected C.
[`mk/rtlib-sources.mk`](../../../mk/rtlib-sources.mk) owns these rules.

An OS's `static` directory can replace startup objects from `../static`.
Startup objects remain separate link inputs. Common startup code stays outside
`drivers` because it is shared by targets without a native override.

Include shared runtime contracts through the root `fb.h` and `fb_*.h` headers.
Keep native helper headers beside their driver. Cross-platform helpers should
live in a portable subsystem when their dependencies permit it. Document native
API quirks, callback lifetimes, signal safety and lock requirements at the code
that depends on them; a different OS may impose different guarantees.

When adding a platform, update source selection, configuration headers, startup
selection and the structure check together. The structure check exercises source
selection for every declared platform; it does not validate that platform's ABI
or OS calls. Run its native tests before reporting the port as working.

<!-- end of drivers/README.md -->
