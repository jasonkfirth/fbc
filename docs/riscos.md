# FreeBASIC on RISC OS

This document describes the compiler target, build workflow, emulator setup,
and current native-platform boundary for FreeBASIC on RISC OS.

The port targets 32-bit ARM RISC OS through GCCSDK and UnixLib.  It uses the
historical APCS-32 ABI required by GCCSDK, soft floating point, little-endian
ARM code, and an ARMv4 instruction baseline suitable for StrongARM-equipped
RiscPC systems as well as newer RISC OS machines.

The initial platform builds console programs, file and string code, maths,
sockets, and pthread-backed FreeBASIC threads. UnixLib provides pthread entry
points in libc, so GCCSDK does not need a separate thread library or a
`-pthread` driver option. The build also compiles GCC's bundled ARM libffi with
the port's ARMv4 soft-float ABI, enabling FreeBASIC's argument-marshalling
`THREADCALL` form.

PC port I/O, serial ports, console mouse polling, and `MULTIKEY` return the
normal unsupported-operation error. gfxlib2 and sfxlib build with deterministic
null backends. Native Wimp screen/input and SoundDMA drivers remain separate
follow-up work; no partial desktop driver is registered in the meantime.

## Target replacement layout

RISC OS implementation files live in target directories instead of adding
RISC OS branches throughout shared sources. Runtime replacements are under
`src/rtlib/riscos/`; compiler platform policy is under
`src/compiler/riscos/`; complete CRT and library-header replacements are under
`inc/riscos/`; and target-specific fbctests sources are under `tests/riscos/`
and `tests/fbcunit/src/riscos/`. The build selects a same-named target file
before its shared counterpart and searches the RISC OS include tree before the
shared include tree.

The remaining RISC OS preprocessor entries in shared code are target-detection
tables. They establish the host and compile target but do not contain platform
implementations.

## Quick build, package, and test workflow

On Debian or Ubuntu, the main workflow installs its host dependencies, builds
GCCSDK and the native compiler, creates a RiscPkg archive, and prepares RPCEmu:

```sh
./build_scripts/debianubuntu-build-freebasic-riscos.sh
```

The GCCSDK build can take several hours on a clean machine. Subsequent runs
reuse its checkout and build products. Useful development options include:

```sh
# Dependencies were already installed.
./build_scripts/debianubuntu-build-freebasic-riscos.sh --skip-deps

# Repackage an existing out/riscos/hostfs/FreeBASIC tree.
./build_scripts/debianubuntu-build-freebasic-riscos.sh \
  --skip-deps --no-build --no-emulator
```

The installable result is
`out/riscos/packages/FreeBASIC_<version>-<revision>.zip`. The archive uses
Acorn-origin ZIP entries and SparkFS metadata, so its RISC OS file types survive
installation. Its RiscPkg control record declares `GCC4` as a dependency and
installs a movable `Apps.Development.!FreeBASIC` component.

Install the archive with a RiscPkg-compatible package manager such as PackMan.
Install and boot its GCC4 dependency first, then double-click `!FreeBASIC` once
to add `fbc` to `Run$Path`. The compiler discovers its movable installation
directory, so a normal command does not need an explicit prefix:

```text
fbc hello.bas -x hello
```

After the one-time RPCEmu boot setup described below, run the RISC OS unit-test
suite with:

```sh
./build_scripts/riscos-run-fbctests.sh
```

## Emulator and ROM choices

RISC OS 5 is open source, primarily under the Apache 2.0 licence.  RISC OS Open
publishes a stable 5.30 IOMD soft-load ROM which is suitable for RPCEmu, so the
emulator workflow does not require a purchased or otherwise proprietary ROM.
RPCEmu itself is distributed under the GNU General Public License.

Current QEMU releases provide a `raspi0` machine for Raspberry Pi Zero hardware.
That does not yet make stock QEMU the dependable full-system path for this port.
In testing with QEMU 10.2.1, the official RISC OS 5.30 Raspberry Pi image enters
the kernel but does not reach a usable desktop because several legacy mailbox
and property-interface behaviours are not modeled.  Directly loading
`RISCOS.IMG` avoids the Raspberry Pi boot firmware files, but it does not avoid
those emulation gaps.

RPCEmu remains the supported full-system test path because it emulates the
RiscPC/A7000 hardware and the matching open ROM is available directly from
RISC OS Open.  GCCSDK has also used a RISC OS-aware QEMU user-mode build for
UnixLib tests.  That tests individual ELF programs rather than booting RISC OS.

The compiler target and ABI do not depend on either emulator.  The same output
can be copied to physical RISC OS hardware, including a Raspberry Pi Zero.

## 1. Build GCCSDK

The supported cross driver is `arm-unknown-riscos-gcc`.  Do not substitute an
`arm-none-eabi` or Linux ARM compiler. Those drivers do not provide GCCSDK's
RISC OS startup objects, UnixLib ABI, linker emulation, or file conventions.

On Debian/Ubuntu, install the GCCSDK and RPCEmu build prerequisites first:

```sh
sudo apt update
sudo apt install -y \
  autogen bison build-essential bzip2 ca-certificates cmake curl file flex \
  git gperf help2man libasound2-dev libgmp-dev libmpc-dev libmpfr-dev \
  libncurses-dev libqt5multimedia5-plugins m4 patch pkg-config procps python3 \
  qt5-qmake qtbase5-dev qtmultimedia5-dev rsync subversion texinfo unzip wget \
  xsltproc zip
```

Then run:

```sh
./build_scripts/riscos-gccsdk.sh
source out/riscos/gccsdk/env.sh
```

`--revision REV` makes the Subversion checkout reproducible.  Without it, the
script builds the current GCCSDK trunk.  GCCSDK can take a few hours to build.

If GCCSDK is already installed, export its standard variables instead:

```sh
export GCCSDK_INSTALL_CROSSBIN=/path/to/gccsdk/cross/bin
export GCCSDK_INSTALL_ENV=/path/to/gccsdk/env
export GCCSDK_TARGET_ENV=/path/to/gccsdk/cross/arm-unknown-riscos
export PATH="$GCCSDK_INSTALL_CROSSBIN:$PATH"
```

`GCCSDK_INSTALL_ENV` is GCCSDK's Autobuilder wrapper directory.
`GCCSDK_TARGET_ENV` names the target sysroot containing `include` and `lib`.

## 2. Build the runtime and a program

The end-to-end smoke helper builds the target runtime, compiles a static
FreeBASIC console program, verifies that the result is ARM ELF, and stages it
with a RISC OS filetype suffix for RPCEmu HostFS:

```sh
./build_scripts/riscos-build-smoke.sh
```

Use `--with-libs` to compile the current null-backend gfxlib2 and sfxlib too.
To build another program:

```sh
bin/fbc -target arm-unknown-riscos -static program.bas -x program
```

The compiler also accepts `-target riscos`; the full triplet is preferred in
scripts because it identifies the required GCCSDK driver unambiguously.

The linker first produces static UnixLib ELF. GCCSDK's host-side `elf2aif`
tool then creates the supported native AIF file (`&FF8`). If that tool is not
available, the smoke helper stages an ELF file (`&E1F`), which requires
ELFLoader on RISC OS. UnixLib programs also require the open SharedUnixLibrary
module. The helper finds the GCCSDK `sul` build product in the cross-bin
directory and stages it as `!System.310.Modules.SharedULib`. Use
`--shared-unix-library FILE` if the module is stored elsewhere.

Dynamic ELF programs additionally require SOManager and their shared objects.
The shared-library linker path has not yet completed guest acceptance testing,
so static AIF executables remain the supported format.

## 3. Build and run the native compiler

The native lane builds GCC, binutils, FreeBASIC, the runtime libraries, and an
open static `elf2aif` for execution inside RISC OS:

```sh
./build_scripts/riscos-gccsdk.sh --with-native
source out/riscos/gccsdk/env.sh
./build_scripts/riscos-build-native.sh --with-libs
./build_scripts/riscos-rpcemu.sh --run
```

The native package is staged below `out/riscos/hostfs`. In the RISC OS
desktop, run `!GCC`, obey `FreeBASIC.SetPaths`, and then obey
`FreeBASIC.Compile`. The compile smoke grants GCC a 32 MiB Wimp slot, compiles
`hello.bas` entirely inside the guest, converts the linked ELF to AIF, and runs
the result. A successful transcript ends with:

```text
making AIF:   elf2aif "/HostFS:$/FreeBASIC/examples/hello"
Native compile return code: 0
FreeBASIC for RISC OS
FB_RISCOS_SMOKE_OK total= 10
Native run return code: 0
```

Native `fbc` invokes GCCSDK through the RISC OS `Run$Path` and emits AIF
executables by default. The compiler itself is currently built at `-O0`
because GCCSDK GCC 4.7 has an optimizer failure in one of the generated
compiler translation units at `-O1` and above. Programs compiled by that
compiler retain their requested optimization level.

## 4. Prepare RPCEmu

RPCEmu 0.9.5 requires Qt 5 development tools, including Qt Multimedia. It also
requires `curl`, `sha256sum`, `tar`, and `unzip`. The helper downloads and
verifies RPCEmu, the official RISC OS Open 5.30 IOMD ROM, and the matching
HardDisc4 boot tree. It builds the emulator, converts the RISC OS filetype
metadata for HostFS, and overlays the staged FreeBASIC files. The default
RPCEmu model is a StrongARM RiscPC, matching the port's ARMv4 baseline:

```sh
./build_scripts/riscos-rpcemu.sh --run
```

The ROM archive and emulator source are pinned by SHA-256.  Pass
`--rom /path/to/rom` only to override the official open ROM with another
combined 2, 4, 6, or 8 MiB image.

The default CMOS state names ADFS as the boot filesystem. On the first boot,
RPCEmu therefore stops at a command line. Configure the open HostFS boot tree
once and restart the emulator:

```text
*configure filesystem hostfs
*configure boot
```

After RISC OS reaches the desktop, open HostFS, then FreeBASIC, and run
`fbhello`. A successful console run prints:

```text
FreeBASIC for RISC OS
FB_RISCOS_SMOKE_OK total= 10
```

RPCEmu 0.9.5 supports at most 256 MiB of emulated RAM, and the helper now uses
that maximum by default. The accepted values are 4, 8, 16, 32, 64, 128, and
256 MiB. A 2 GiB setting is not available on this RiscPC emulator model.

The complete `DIRLIST_FB` unit-test selection passes at 256 MiB when split into
the runner's default four-directory batches. Each test process receives a
192 MiB TaskWindow slot. The default run uses ten batches; the current accepted
run passed 1,142,077 assertions across 3,092 tests. An original Raspberry Pi
Zero has 512 MiB, so it can provide more headroom than RPCEmu, but the validated
RPCEmu lane does not require it.

## 5. Run fbctests under RISC OS

The runner refreshes the RISC OS runtime libraries, cross-builds static AIF
test programs, stages tracked resource files using UnixLib suffix directories,
boots RPCEmu for each batch, and rejects test failures, crashes, and timeouts:

```sh
./build_scripts/riscos-run-fbctests.sh
```

Logs are written to `out/riscos/fbctests/logs`. A successful default run ends
with:

```text
==> RISC OS fbctests passed: 10/10 batches
```

The runner covers every top-level directory in `DIRLIST_FB`. It does not run
the separate compiler diagnostic pass/fail log harness. To resume an interrupted
run, or to reduce the memory used by each linked test image, use:

```sh
./build_scripts/riscos-run-fbctests.sh --resume
./build_scripts/riscos-run-fbctests.sh --batch-size 1 --resume
```

## 6. Run Exampleageddon under RISC OS

The RISC OS Exampleageddon runner cross-compiles and classifies every `.bas`
file in `examples/`. It then converts each successfully built self-contained
program to AIF, stages its resources through UnixLib suffix directories, and
runs those programs sequentially inside RISC OS Open:

```sh
./build_scripts/riscos-run-exampleageddon.sh
```

The compile inventory includes interactive, platform-specific, and external
library examples so gaps remain visible. The unattended acceptance policy only
requires the self-contained group to compile and return zero in RISC OS. Guest
programs run in batches of 25 by default, but each program is a separate AIF
process. A batch timeout can be isolated without discarding completed logs:

```sh
./build_scripts/riscos-run-exampleageddon.sh --batch-size 1 --resume
```

The combined report is written to
`out/riscos/exampleageddon/riscos-report.md`; per-example data is in
`riscos-results.csv`, and raw RISC OS transcripts are retained under `logs/`.
The current accepted run cross-compiled 660 self-contained programs and ran
all 660 successfully under RISC OS, with no failure, timeout, or missing result.

Named directories are useful while repairing a focused area:

```sh
./build_scripts/riscos-run-fbctests.sh --dirs string,wstring
```

Resume records include the exact directory list, so a successful log from a
different selection cannot accidentally skip a batch.

## 7. Run on a Raspberry Pi Zero

The Raspberry Pi Imager catalogue includes an official RISC OS 5.30 SD-card
image for the original Raspberry Pi and Pi Zero family.  Write that image with
Raspberry Pi Imager, boot the Pi, and transfer the staged AIF or ELF program from
`out/riscos/hostfs` using the filesystem or network method available on the
machine.

There is one licensing distinction worth keeping explicit.  The RISC OS kernel
and operating-system sources are open, but the physical Pi image includes
Broadcom boot firmware such as `bootcode.bin` and `start.elf`.  Those files are
redistributable for use with Raspberry Pi devices under their own licence.  The
FreeBASIC port does not link against them or depend on them when targeting other
RISC OS machines.

## Verification without an emulator

The compiler target wiring can be checked on any development host, even before
GCCSDK is installed:

```sh
make compiler-riscos-smoke
bin/fbc -target arm-unknown-riscos -print target
```

The first command verifies the RISC OS predefined macros, generated C, GCCSDK
driver selection, and ARMv4 flag.  If the cross compiler is on `PATH`, it also
compiles and inspects a real APCS-32 ARM object.

For a complete acceptance run, build the runtime and native compiler with
GCCSDK, compile and run the smoke AIF under RPCEmu, and repeat it on at least
one physical RISC OS machine. QEMU user-mode UnixLib tests are useful
additional coverage but are not a substitute for RiscPC SWI, filesystem,
Wimp, and module behavior. Raspberry Pi full-system QEMU should remain an
experimental lane until the official image boots without local emulator
patches.

## Remaining native work

The port boundary is intentionally explicit.  The next platform-specific
increments are:

1. A Wimp or direct-screen gfxlib2 driver, with keyboard and mouse state.
2. A SoundDMA sfxlib driver.
3. Native serial-device handling.
4. Cross-target execution of the separate compiler diagnostic log harness.
5. The missing Raspberry Pi mailbox and peripheral behaviour needed for a
   reproducible full-system QEMU lane.

Relevant upstream projects and setup references:

- [GCCSDK](http://www.riscos.info/index.php/GCCSDK)
- [GCCSDK cross-compilation](http://www.riscos.info/index.php/Cross-compiling_software_with_GCCSDK)
- [ELFLoader](http://www.riscos.info/index.php/ELFLoader)
- [RPCEmu](https://www.marutan.net/rpcemu/)
- [RISC OS Open packaged software and PackMan](https://packages.riscosopen.org/packages/)
- [RISC OS Open RiscPC downloads](https://www.riscosopen.org/content/downloads/riscpc)
- [QEMU Raspberry Pi machine documentation](https://www.qemu.org/docs/master/system/arm/raspi.html)
- [Raspberry Pi Imager operating-system catalogue](https://downloads.raspberrypi.com/os_list_imagingutility_v4.json)
- [Raspberry Pi firmware licence](https://github.com/raspberrypi/firmware)

<!-- end of riscos.md -->
