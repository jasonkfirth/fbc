# Classic AmigaOS

This port targets AmigaOS 3.x on a 68020 or later CPU. The compiler emits C
for `m68k-amigaos-gcc`, which links native Hunk executables. The runtime,
gfxlib2, and sfxlib use native DOS, Exec, Intuition, graphics, and device APIs.
An FPU and the disk-based IEEE math libraries are not required.

The starting SDK contract comes from FreeBASIC-NG revision
`a60609d219e70ba1c145109b157960d85b87f019`. Runtime file, threading, graphics,
and sound work also draws on this tree's AROS implementation. Classic AmigaOS
has its own platform adapters because its newlib SDK and native DOS handles
are different from AROS POSIXC.

## Building on Linux

The workflow needs a working host FreeBASIC compiler, GNU make, Python 3,
host C build tools, Autoconf, Automake, and Libtool. Docker is needed only
when provisioning the SDK. FS-UAE and Xvfb are needed to run native tests;
`xdotool` and ImageMagick are optional for timeout screenshots.

```sh
build_scripts/debianubuntu-build-freebasic-amiga.sh --jobs 8
```

The SDK is extracted under `out/amiga/toolchain`. Its image is pinned to:

```text
amigadev/m68k-amigaos-gcc@sha256:b18080e6ffca8f793e0f539536a9138e9d2a548ca1a301c7483f43ee15fedfed
```

The SDK contains GCC 6.5.0b and native headers. The build relocates its
container-rooted symbolic links, then applies hash-checked repairs to the
inspected newlib startup objects. Those repairs preserve the exit stack
frame and pass the actual `argv` value to `main`.

The target archives are written to `lib/freebasic/amiga-m68k`. Normal and
multithreaded runtime, graphics, and sound variants use the same
`-m68020 -msoft-float -fno-common` contract. Additional providers are:

| Archive | Purpose |
| --- | --- |
| `libfbsoftfloat.a` | CPU floating arithmetic and conversions without external math libraries |
| `libpthread.a` | AmigaPorts pthread implementation with stable thread and TLS records and repaired cleanup |
| `libffi.a` | libffi 3.4.8 adapted to Hunk symbols, m68k pointer returns, and Exec cache maintenance |

The providers are built from pinned, hash-checked inputs. Their build scripts
record the upstream revisions and preserve the license notices. GCC helpers
retain GPLv3 and the GCC Runtime Library Exception; libffi retains its MIT
license. The runtime install also copies provider licenses and the libffi
headers under the target library directory.

The pinned CPU arithmetic source is verified before bounded build-time repairs
are applied. Those repairs retain product guard/sticky bits, correct an
incomplete single-product mask, classify infinity times NaN, and preserve the
sign of underflow to zero. They are covered by native comparisons with host
hardware results; the original downloaded source and its license remain intact.

For an existing SDK, use `--skip-toolchain --toolchain-root /path/to/sdk`.
`AMIGA_ROOT` selects the support build directory and `--skip-compiler` keeps
an already-built host compiler. Temporary build files stay in that support
directory rather than relying on space in `/tmp`.

## Compiling programs

From the source tree:

```sh
export PATH="$PWD/out/amiga/toolchain/bin:$PATH"
bin/fbc -prefix "$PWD" -i inc -i inc/amiga -target amiga-m68k hello.bas -x hello
```

`amiga`, `amigaos`, and `m68k-amigaos` are target aliases. The driver selects
the GCC backend and native library order automatically. Unsupported CPU
families, other code generators, and `-dll` are rejected before tool execution.
The Hunk linker must select `libfbsoftfloat.a` before the SDK's indirect
floating-point aliases; the compiler and bootstrap link rules do this.

The resulting program can be copied to an Amiga filesystem and started from
its shell. Graphics and sound archives are selected through the ordinary
FreeBASIC commands and include files.

Cross-built Hunk objects and libraries retain FreeBASIC dependency metadata.
The driver reads both ordinary ar containers and the concatenated Hunk units
produced by the native SDK archiver, so dependencies declared by a library
are also selected when linking its callers. External libraries still need
native Amiga builds; host Linux archives cannot satisfy those dependencies.

The native compiler front end can also be built:

```sh
build_scripts/debianubuntu-build-freebasic-amiga.sh --native-compiler
```

Its binary is `out/amiga/native-fbc`; building it preserves the host
`bin/fbc`. A complete native compile and link needs an Amiga-hosted GCC
installation. The Linux SDK's tools are cross tools and cannot run inside
AmigaOS. With the front end alone, `-r -m module module.bas` emits C, which
can be compiled and linked by the cross SDK. The native qualification checks
this complete sequence and runs the resulting Hunk program, including its
command-line argument check.

Native include handling recognizes `volume:directory/file` paths and resolves
assigns through DOS. Shared BASIC headers can keep their `.` and `..`
components: the runtime translates them to native parent components before
opening a file. Empty components in existing native paths retain their
AmigaDOS meaning.

Worker threads inherit their creator's native standard streams. BASIC global
destructors can signal and join workers before the runtime performs its final
task cleanup. That final wait still precedes runtime-state release and Hunk
unloading, because live native tasks share the command's code and data.
Sound and graphics contexts reopened by a destructor are closed before that
wait, while the runtime's locks, streams and thread-local storage still exist.

## Graphics and sound

gfxlib2 uses the `AMIGA` driver. It presents dirty framebuffer rows through
Intuition and graphics.library. It can use a CyberGraphX public screen when
available, or open a private chipset screen. Indexed framebuffers preserve
their palette. Higher-depth BASIC framebuffers are converted to the available
chipset palette, including RGB332 on an eight-plane AGA display.
True-colour conversion selects the closest available component levels using
tables prepared when the screen opens. It does not change the software
framebuffer's pixel values. Indexed modes continue to use the program's
palette, including changes made without drawing another frame.

The requested software canvas can exceed the physical display. The chipset
backend bounds its display-size request to the selected monitor's available
ordinary modes and keeps the original nominal aspect ratio. Larger canvases
retain AmigaOS
[virtual-screen behavior](https://wiki.amigaos.net/wiki/Intuition_Screens#Overscan_and_the_Display_Clip).

The chipset path uses a one-row temporary bitmap for `WritePixelLine8`.
Its width is word-aligned independently of BASIC's framebuffer pitch, so
odd framebuffer widths remain valid. Raw keys use keymap.library; mouse and
window events come from the window's Intuition message port.
`SETMOUSE` hides the native pointer with a transparent chip-memory sprite and
restores the system pointer when requested. Intuition retains responsibility
for presenting it above graphics updates and across focus changes.
The sprite allocation and lifetime follow the native
[Intuition pointer contract](https://wiki.amigaos.net/wiki/Intuition_Pointer).

sfxlib first tries AHI and falls back to Paula's ROM-resident audio.device.
Paula playback uses a left/right channel pair and chip-memory DMA buffers.
The mixer retains its requested rate while a rational phase accumulator
resamples to a supported DMA period. Odd output bytes carry into the next
write because Paula transfers words. Device requests use a reply port owned
by the task performing the write, and both channels finish before their
buffers are reused.

The sound worker and mixer use recursive Exec semaphores. Shutdown requests
stop under the mixer lock, then joins the worker outside that lock before
releasing device state. Output capture writes little-endian PCM WAV even
though this target is big-endian.

## Native qualification

Supply a local Kickstart ROM; ROMs are not distributed with the repository.
The qualification runner creates disposable host-directory boot volumes,
so these tests do not require a Workbench disk image.

```sh
export AMIGA_KICKSTART=/absolute/path/to/kickstart.rom

build_scripts/amiga-run-fbctests.sh --output out/amiga/fbctests --jobs 8
build_scripts/amiga-run-exampleageddon.sh --output out/amiga/exampleageddon --jobs 8
build_scripts/amiga-run-probes.sh --output out/amiga/probes \
    --native-compiler out/amiga/native-fbc
```

Every emulator session runs on its own authenticated Xvfb display with
software rendering. The runner never uses the desktop display. Host audio
is disabled while guest audio interrupts remain active. Xvfb and FS-UAE are
terminated when a batch finishes, times out, or the runner receives SIGTERM.
Install `xvfb` or set `AMIGA_XVFB_BIN` to its executable. An extracted local
installation under `out/amiga/virtual-display` is also recognized.

Each output directory holds a compiler/include/library snapshot, frozen
test sources and resources, compile logs, guest logs, JSON/CSV results, and
a Markdown report. Resuming verifies the recorded input hashes:

```sh
build_scripts/amiga-run-exampleageddon.sh \
    --output out/amiga/exampleageddon --resume
```

Use a new output directory to test changed inputs. `--compile-only` builds
an inventory without launching an emulator. `--dirs file,threads` selects
fbctests directories. `--backend-debug` enables native graphics and sound
diagnostics. The default FS-UAE accuracy setting permits accelerated CPU
execution; `--accuracy 1` can be substantially slower for software floating
point and needs a larger timeout.

Jobs may provide `stdin_text` for a reproducible UTF-8 input transcript. The
runner stages that input separately from program resources and selects its
native DOS handle while the program runs. Five-field manifests remain valid;
the optional sixth field names an input file. The controller restores its
previous input and output handles before starting the next job.

The guest controller uses `LoadSeg` and `RunCommand`, records the actual
return code, and restores process state between jobs. fbctests additionally
requires complete, consistent XML suite and case counts. Printed markers
alone do not qualify a test.

The native probes exercise:

- 200 KB binary command pipes in both directions, backpressure, EOF, early
  close, exit cleanup, metadata, and relative paths;
- inherited native standard streams in child and nested threads, redirected
  worker output, and parent output after joining those threads;
- a worker whose stop request and join belong to a BASIC global destructor;
- sound initialization from a BASIC global destructor after the initial
  sound shutdown, followed by a clean native return;
- direct `LINE INPUT` from LF-terminated redirected standard input, followed
  by `SLEEP`, `GETKEY` and EOF checks on the same buffered stream;
- native bitmap and palette readback at BASIC depths 8, 16, and 32 with an
  odd width, all 256 component values, live palette changes, plus raw-key and
  mouse message translation, including queued key presses/releases consumed
  directly by `MULTIKEY`;
- hidden-page drawing, default `SCREENCOPY`, and visible-page switches at all
  three framebuffer depths;
- native mode selection for a 1280x600 virtual canvas, checking the ROM's
  PAL 1280x512 or NTSC 1280x400 display and pixels at both horizontal edges;
- 1,024 BMP bitfield samples with one- through eight-bit components, including
  narrow alpha expansion and alpha drawing;
- bounded Hunk metadata parsing and native execution of transitive archive
  and direct-object dependency links;
- rejected driver-option exit codes and compile-only SDL2/CRT declarations;
- native sound-driver selection and a quarter-second stereo capture, with
  separate 440 Hz and 880 Hz tones checked for duration, amplitude, and channel
  ordering;
- 20,000 software floating conversion comparisons against a host hardware
  oracle, including boundary values and reproducible random bit patterns;
- 20,000 software multiplication comparisons against a host hardware oracle,
  including signed zeros, subnormals, halfway products, infinities and NaNs,
  plus eight signed-zero division checks;
- 60,000 software addition, subtraction and division comparisons, including
  both operand signs, exact cancellation, overflow and subnormal rounding;
- optional native compiler emission and execution of its generated program.

## Recorded results and limits

The October 2, 2026 runs used FS-UAE 3.2.35, an A1200/020 configuration,
Kickstart 3.1 revision 40.68, 2 MB chip RAM, 8 MB fast RAM, and 256 MB Zorro III
RAM. They booted disposable directory volumes and used private Xvfb displays.

| Qualification | Result |
| --- | --- |
| fbctests | 37 directory batches passed; 1,162,179 assertions; complete XML reports |
| Exampleageddon | All 649 self-contained examples compiled and returned successfully |
| Native probes | Six checks passed, including native compiler output execution |
| Graphics | Native readback and input translation passed at depths 8, 16, and 32 |
| Sound | Paula writes completed; 44,100 Hz capture contained 11,025 stereo frames with the expected tones |

The full Exampleageddon inventory contains 1,683 entries. Interactive demos,
other-platform examples, intentional failures, helper modules, benchmarks,
network examples, and third-party library examples retain their inventory
classifications. They are not counted as unattended runtime passes. All 66
sound example entries compiled; native audio behavior is covered by the
bounded probes above.

Local evidence is in `out/amiga/fbctests-final`,
`out/amiga/exampleageddon-final`, and `out/amiga/probes-workflow`. These
directories contain the exact snapshots used for the runs, rather than a
promise that later workspace edits have been qualified.

The later metadata changes also passed the file batch in
`out/amiga/file-final`. Linux regression checks passed 866 datetime assertions
and the existing raw-output/capture test. A SIGTERM during guest execution
returned runner status 130 and removed its emulator, virtual display, and
authority file; other applications' displays were left alone.

The ROM-only run qualifies the chipset and Paula paths. AHI and CyberGraphX
build successfully but still need runs with those components installed.
The captured WAV records samples accepted by the native driver; it does not
record the emulator's final host audio output. Real hardware timing and
physical serial/printer devices remain separate qualification work.

The October 16 corpus audit also exposed defects in the imported CPU arithmetic
helpers. The provider build repairs the verified assembly copy while retaining
the original pinned source and license notices. These repairs cover guard and
sticky bits, finite/nonfinite classification, overflow and signed-zero returns.
All 14 maintained native probes passed after rebuilding through the normal make
rules, including 100,000 hardware-oracle conversion/arithmetic comparisons and
eight additional signed-zero checks. This focused run does not replace the
earlier complete fbctests and Exampleageddon qualifications.

The corpus vector-field example exposed a further shared gfxlib2 failure:
nonfinite line endpoints could reach integer clipping and raise an Amiga
divide exception. LINE now checks transformed endpoints before conversion.
The graphics suite passed all 99 tests on both targets, with 928,199 host
assertions and 469,447 native assertions. The added regression covers NaNs,
infinities, out-of-range finite values and WINDOW transform overflow for the
line, outline-box and filled-box forms. The original vector-field source
then completed its diagnostic runs without modification.

Corpus asset formats also require explicit byte-order handling. BASIC's binary
`GET` transfers native representations; it does not decode little-endian texture,
font or model files automatically. The audit's necessary BASICVERSE source port
preserves the checked original GCC outputs and decodes all 262,144 texture pixels,
97,280 glyph coverage bytes, 95 advances, 15,204,000 model weight words and
32,000 tokenizer records identically on Amiga. Its 11 entry points now compile
for both targets. A complete model inference step returns successfully and
selects the same token, although floating scores are not bit-identical.

Persistent application worker pools must stop cooperatively and join before
executable segments are released. The runtime allows BASIC global destructors
to perform that cleanup; it cannot safely terminate arbitrary application code
that is still using shared state. BASICVERSE needed its existing stop flags
connected to that lifecycle. Initialization failure and repeated shutdown are
covered by the corpus audit as well as the runtime destructor probe.

Other current platform limits are explicit:

- Files use checked signed 32-bit native positions. Unsupported larger seeks
  fail rather than truncating the requested offset.
- Exec signals are wake-up bits, not POSIX fatal signals. CPU faults remain
  native Amiga alerts; automatic fatal-signal recovery is unavailable.
- Unix-style dynamic libraries and TCP are unavailable with the pinned SDK.
  Native Amiga library vectors require their own bindings.
- OpenGL, shaped/resizable graphics windows, and joystick polling are not
  implemented by this backend.

<!-- end of amiga.md -->
