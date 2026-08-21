# FreeBASIC MIPS Linux port

File: `docs/mips.md`

Purpose: document the supported MIPS Linux targets, reproducible build
environment, qualification workflow, and native package contract.

Responsibilities:

- identify the four supported GNU ABIs
- explain the Docker-contained toolchain
- provide build, resume, and focused-test commands
- describe the contents and requirements of native compiler packages

This file intentionally does not document MIPS operating systems other than
Linux, Windows CE targets, or generic compiler internals.

## Supported targets

| GNU target triplet | FreeBASIC target | Width | Byte order | QEMU |
| --- | --- | ---: | --- | --- |
| `mips-linux-gnu` | `linux-mips32` | 32 | big-endian | `qemu-mips` |
| `mipsel-linux-gnu` | `linux-mips32el` | 32 | little-endian | `qemu-mipsel` |
| `mips64-linux-gnuabi64` | `linux-mips64` | 64 | big-endian | `qemu-mips64` |
| `mips64el-linux-gnuabi64` | `linux-mips64el` | 64 | little-endian | `qemu-mips64el` |

The reusable compiler architecture model owns MIPS32, MIPS32 little-endian,
MIPS64, and MIPS64 little-endian. Linux-specific link policy and runtime
selection remain in Linux platform files.

## Host requirements

The MIPS cross packages are not required on the host. Some Ubuntu repository
configurations do not publish names such as `gcc-mips-linux-gnu`, which is why
the build uses a pinned Ubuntu 24.04 container containing all four GNU cross
toolchains, target libc sysroots, QEMU user mode, libffi, and ncurses.

Install the two host tools only if needed:

```console
sudo apt-get update
sudo apt-get install -y docker.io rsync
```

No host APT source changes or foreign architectures are required.

## Complete build and qualification

From the FreeBASIC source root, run:

```console
./build_scripts/debianubuntu-build-freebasic-mips.sh
```

The script copies the current source, including uncommitted port work, into
`out/mips/work`. Generated objects and earlier output are excluded. The build
then performs these checks inside the container:

1. Builds a current x86_64 host compiler from bootstrap sources.
2. Builds rtlib, gfxlib2, and sfxlib for all four MIPS ABIs.
3. Builds one native MIPS compiler per ABI.
4. Runs each native compiler under QEMU and has it drive the matching GNU
   supply chain to produce a runnable MIPS program.
5. Runs every `DIRLIST_FB` directory as a bounded fbcunit executable under
   the matching QEMU interpreter.
6. Creates and validates native `tar.xz` and ZIP packages.

The default parallelism is capped at four jobs to avoid unnecessary compiler
memory pressure. Override it when the host has a different useful limit:

```console
./build_scripts/debianubuntu-build-freebasic-mips.sh --jobs 2
```

To continue after an interrupted full test run, retain the existing output and
use the recorded successful test logs:

```console
./build_scripts/debianubuntu-build-freebasic-mips.sh \
    --skip-image --reuse-worktree --resume
```

For a focused failure investigation inside the toolchain container, invoke
`build_scripts/mips-run-fbctests.sh` with `--targets` and `--dirs`. The outer
build script is the normal entrypoint because it supplies the required tools.

## Headless media policy

The MIPS qualification environment is QEMU user mode rather than a desktop
system emulator. gfxlib2 is still built and its memory-oriented unit tests run,
but X11 and OpenGL backends are disabled. sfxlib is built with its platform
core and format support while ALSA and PulseAudio device backends are disabled.
This prevents missing desktop services from being mistaken for missing media
libraries.

## Native packages

Individual packages are written below `out/mips/packages` for all four ABIs.
Each archive contains:

- `bin/fbc`
- `include/freebasic`
- the ABI-matched normal and multithreaded rtlib
- gfxlib2 and sfxlib, including multithreaded variants
- static libffi and libtinfo dependencies
- project and dependency license documentation

The archive does not bundle GCC or binutils. A target machine needs its native
GNU C supply chain installed so `fbc` can compile generated C and link programs.
The compiler discovers the rest of the package relative to its own executable,
so the extracted directory can be relocated as a unit.

<!-- end of docs/mips.md -->
