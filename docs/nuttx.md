# FreeBASIC for NuttX and RISC-V boards

FreeBASIC 1.20.4 includes the experimental `nuttx-riscv32` compiler target, a
NuttX runtime, low-memory gfxlib2 support, sfxlib integration, examples, and
host tools for QEMU and physical RISC-V boards. The supported workflow emits C
with FreeBASIC and compiles it inside the matching NuttX application or
loadable-module build.

The source tree does not vendor Apache NuttX. The helpers clone or reuse an
external `nuttx/` and `apps/` workspace so the NuttX version and board
configuration remain explicit.

## Installing the SDK

The `freebasic-nuttx` Debian package is published for Ubuntu Resolute amd64 and
is also recommended by `freebasic-full` in that repository. On an Ubuntu
Resolute amd64 host:

```sh
curl -fsSLO https://deb.fbxl.net/install.sh
sh install.sh --release resolute --arch amd64
sudo apt-get install freebasic-nuttx
```

The package installs the runtime source SDK, examples, compatibility patches,
and these user commands:

- `nuttx-riscv32-qemu-smoke`
- `fbc-nuttx-esp32p4`
- `fbc-nuttx-esp32p4-firmware`
- `nuttx-remote-console`
- `nuttx-http-server`

Use each command's `--help` output and installed manual page for its complete
option list.

## QEMU workflow

The quickest end-to-end check prepares a RISC-V NuttX workspace, compiles the
default FreeBASIC example into the firmware image, starts
`qemu-system-riscv32`, and verifies the console marker:

```sh
nuttx-riscv32-qemu-smoke
```

To select another program or stop after preparing the source trees:

```sh
nuttx-riscv32-qemu-smoke --bas program.bas --app-name program
nuttx-riscv32-qemu-smoke --prepare-only
```

`--with-gfxlib` links the NuttX graphics support. The storage and USB options
can add temporary filesystems, block devices, and supported xHCI test devices.
Those are qualification features; an ordinary console program does not need
them.

## ESP32-P4 workflow

Firmware preparation is deliberately separate from application deployment:

```sh
fbc-nuttx-esp32p4-firmware
fbc-nuttx-esp32p4 --bas program.bas --serial-port /dev/ttyACM0 --run
```

The firmware command fetches or reuses NuttX, applies the checked-in
compatibility patches, selects the supported Ethernet board configuration, and
builds the image. Add `--flash --port DEVICE` only when the named board is
attached and ready to be changed.

The application command builds a loadable module, serves it from a temporary
HTTP endpoint, transfers it to `/data/fb` or `/mnt/sd0/fb`, and can start it
through telnet or serial NSH control. `--no-upload` performs only the local
build and staging step.

## RP2350-PiZero workflow

The source SDK also contains the Waveshare RP2350-PiZero firmware and loadable
module helpers. A typical module build uses an already prepared NuttX tree:

```sh
nuttx-rp2350-build-fbmodule.sh \
  --nuttx-workdir /path/to/workspace \
  --bas program.bas \
  --app-name program
```

The resulting ELF is intended for `/mnt/sd0/bin` on the board. The repository
also contains a PowerShell YMODEM uploader for a board whose NSH console and SD
card have already been configured. Firmware image construction and module
transfer are separate operations so rebuilding one program does not require a
firmware flash.

## Runtime scope

The NuttX runtime covers console, files and directories, environment access,
DATA/READ, math, time, strings and WSTRINGs, runtime errors, memory reporting,
threads, TCP, GPIO entry points, gfxlib2, and sfxlib. Small boards still impose
real limits on framebuffer pages, heap, thread stacks, media decoders, and
network buffers. The helpers expose stack and feature switches instead of
assuming desktop resources.

Hardware-dependent DVI, USB host, SD, Ethernet, serial, GPIO, and audio paths
must be tested on the relevant board. QEMU is useful for compiler, runtime,
filesystem, and network coverage, but it cannot prove those physical paths.

## Development and verification

The source tree includes scripts for generated-C audits, fbctests,
Exampleageddon, QEMU suites, runtime-symbol coverage, RP2350 memory budgets,
firmware images, and YMODEM upload. Start with the installed package commands
above unless you are modifying the compiler or NuttX runtime itself.

<!-- end of nuttx.md -->
