<!--
Project: FreeBASIC graphics libraries
File: gfx-maintenance.md
Purpose: Give maintainers a practical route through graphics changes.
Responsibilities: Explain boundaries, invariants, failure handling and validation.
This file intentionally does NOT contain a graphics language reference.
-->

# Maintaining the graphics runtimes

Start with the source maps for [gfxlib2](../src/gfxlib2/README.md) and
[gfxlib3](../src/gfxlib3/README.md). gfxlib2 owns the default software graphics
runtime. gfxlib3 is selected explicitly and runs queued commands on a renderer
thread. They implement related BASIC behavior through different memory and
synchronization models.

## Choose the owner before editing

| Change | First owner | Other contracts to inspect |
| --- | --- | --- |
| BASIC graphics argument or overload | Compiler `parser/intrinsics`, `runtime/rtl-gfx.bas` | Both runtimes' C entry points and `inc/fbgfx*.bi` |
| Coordinates, VIEW, WINDOW, relative positions | gfxlib2 `draw`, gfxlib3 `compat` | Target clipping and invalid float conversion tests |
| CPU image layout, GET/PUT, direct access | gfxlib2 `image`/`pixels`, gfxlib3 `compat`/`api` | Packed image headers, pitch and callback barriers |
| A pixel algorithm | gfxlib2 `draw`/`pixels`, gfxlib3 backend modules | Null reference behavior, GPU shaders and all supported depths |
| Mode opening, pages or resize | gfxlib2 `core`, gfxlib3 `api`/`compat` | Native adapter, failure cleanup and borrowed framebuffer pointers |
| Native events or windows | `drivers/<os>` in the relevant library | Shared event layout, queue locks, drop payload ownership |
| Queueing or GPU resource lifetime | gfxlib3 `core` | Producer mutex, command copies, completion and GPU fences |
| OpenGL or Vulkan driver work | gfxlib3 `backend` | Private backend header and platform context ownership |
| File format | gfxlib2 `image`, gfxlib3 `codec`/`api` | Stream limits, malformed fixtures and allocation failures |

Native adapters report input and present pixels. They do not interpret BASIC
coordinate rules. Public API code validates and translates operations; it does
not manipulate driver objects. Backends consume validated command records and
do not read producer drawing state after enqueueing.

## Memory and binary contracts

An image pointer can describe the current packed `PUT_HEADER` or the older
four-byte header. Check its discriminator before reading current-header fields.
Use the stored pitch and the correct pixel-data offset. The 15/16-bit and
24/32-bit language depths do not imply identical byte counts at every boundary.
Sub-byte software modes also use unpacked logical pixel storage.

Headers, event records and shader payloads are binary contracts. Preserve
packing, native byte order, bitfield positions and exact field widths. A BASIC
array descriptor is separate from the pixel buffer it describes. A native drop
event contains metadata for copied payload retrieval, not a transferred native
pointer. The public event record remains 20 bytes.

Before allocating, validate signed dimensions and check multiplication,
addition, alignment and narrowing independently. Reject an impossible size
before allocating or writing. Keep the previous live state until all resources
needed for its replacement have been acquired. A failure path must release
each partially acquired resource once and wake every waiter it owns.

gfxlib2 exposes borrowed CPU framebuffer pointers. gfxlib3 has CPU-compatible
images, cached uploads and explicit GPU surfaces. A producer's image pointer
does not become renderer-owned storage by being placed in a queue. Copy bytes
needed later, retain handles for queued use and use a completion barrier before
returning CPU readback data or invoking a CPU-only compatibility callback.

## Locks and lifetime

For gfxlib2, inspect `core/gfx_access.c`, `core/gfx_screen.c` and `fb_gfx.h`
together. The graphics lock protects shared mode and drawing state; the driver
lock coordinates direct framebuffer access with presentation; the event mutex
protects the event ring. Polling a native driver while holding the event mutex
can deadlock when that driver posts an event. Resize callbacks publish requests
and leave allocation and state replacement to the graphics API boundary.

For gfxlib3, submission order and GPU completion are separate. The producer
mutex orders packets. Queue acceptance transfers command ownership. A renderer
sequence identifies ordered work; a completed GPU fence makes deferred
resource destruction safe. Device objects and GL entry points belong to the
renderer thread. An interop callback runs there under the documented API
contract, not on whichever producer asked for the callback.

The backend-private OpenGL and Vulkan headers expose concrete state only to
their own implementation units. New cross-unit helpers need one definition,
a backend-specific name and a declaration in the corresponding private header.
Keep helpers used by one file `static`. Do not pass those private types into
the queue, common renderer or public headers.

## Build rules that prevent silent omissions

`mk/gfx-sources.mk` lists shared subsystem directories explicitly.
Each library's `drivers` directory contains its OS adapters and CPU kernels.
`mk/source-graph.mk` selects target replacements and derives normal, MT and PIC
object lists from that selection. A shared source is suppressed by a more
specific implementation with the same basename. Objects keep directory paths;
archive members keep basenames. A duplicate selected basename is a build error.

When moving or adding a file:

1. Put shared code in its owning subsystem and native code below `drivers`.
   Keep an existing basename when possible.
2. Give a new subsystem a Make group and a row in the source map.
3. Keep quoted includes relative to the including file. Check source-inclusion
   wrappers, not only header includes.
4. Include new private headers and textual fragments in clean-build dependency
   lists. Compiler-generated `.d` files cannot protect the first build.
5. Update scripts with reduced source lists, including the NuttX smoke builder,
   and fixtures that compile an instrumented C implementation directly.
6. Run the structure check before rebuilding target libraries.

```sh
make gfx-structure-test
```

The check queries 31 target/architecture/feature combinations through Make.
It checks shared ownership, unique archive names, local include resolution,
private-header boundaries and declared header dependencies. RISC OS must keep
both its Wimp window adapter and the BASIC WINDOW coordinate module. The
checker runs as part of `mk-structure-test`, which quick/full checks already use.
It also rejects native source directories beside the shared subsystems, keeping
platform details below `drivers` as the source tree grows.
It is graph validation, not cross-platform compilation evidence.

## Validate behavior in increasing scope

Build both graphics runtimes and the matching compiler/runtime first. Tests
linked before an archive change do not validate the new archive. Use one named
source snapshot when another session is editing the same tree. Preserve dirty
working files; do not reset the checkout or copy old object trees into a new
qualification snapshot.

```sh
make -j2 compiler rtlib fbrt gfxlib2 gfxlib3 sfxlib BUILD_FBC=/path/to/seed/fbc
make -j2 tests-test
FB_GFX_DRIVER=none SFXLIB_DRIVER=null python3 build_scripts/exampleageddon-freebasic.py \
  --fbc "$PWD/bin/fbc" --prefix "$PWD" --include-dir "$PWD/inc" \
  --outdir "$PWD/out/gfx-examples" --jobs 2 --fail-on-self-contained
```

Use `gmake` on FreeBSD and `bin/fbc.exe` on Windows. Set Exampleageddon's
`--target-os windows` on Windows so platform classification is explicit.
`tests-test` includes unit, language/log and warning suites; check each result.
The warning runner generates diagnostic references for five target profiles;
completion means those compiler invocations succeeded, including expected
ordinary compiler errors in negative fixtures.
Exampleageddon compiles the corpus and gates the self-contained runnable
subset. Its zero exit does not prove that every optional third-party library
or interactive example works. Keep classified failures in the report.

On Windows, use one coherent MinGW toolchain from an MSYS Bash session. The
qualification here used `/mingw64/bin:/ucrt64/bin:/usr/bin:/bin` as PATH and
selected GCC, AS, AR, LD and WINDRES from `/mingw64/bin`. A Windows PATH with
space-containing entries can break nested test Make invocations. Check the
actual compiler and linker paths before interpreting a harness failure as a
graphics regression.

For gfxlib3 on Windows and Linux, rebuild the existing graphics suites through
their wrappers after the ordinary tests:

```sh
make -C tests -j2 -B -f gfx3/gfx3-unit-tests.mk all
make -C tests -j2 -B -f gfx3/gfx3-log-tests.mk all FB_LANG=fb
```

These wrappers force selection and relinking against gfxlib3. Null-driver
pixel checks are deterministic and cover BASIC compatibility; they do not
exercise a real GL or Vulkan implementation. Run the existing infrastructure,
Vulkan bootstrap and GPU-surface fixtures for backend changes. Use an isolated
X display for unattended Linux windowed tests and force each intended backend
so fallback cannot turn a failed backend into an apparent pass.

For image or raster changes, compare complete output buffers at supported
depths and pitches, including clipping, overlap and alpha cases. For lifecycle
changes, cover partial initialization, repeated open/close, queued failure,
blocked waiters and resource release after the last GPU use. A successful
compile or a running process alone does not prove rendering or input behavior.

Run unattended tests without the privilege to change the host clock. Historical
DATE/TIME examples and tests are especially relevant when SSH access uses root.
Keep input timestamps consistent inside the task workspace if the host clock
is stale; a future-dated header can otherwise force repeated library rebuilds.

The [current verification record](gfx-restructure-verification.md) distinguishes
passing public-API GPU fixtures from older infrastructure and custom-font
checks with reproduced baseline failures. Keep those results visible when
choosing a gate for a later change.

## Keep qualification evidence small and useful

Retain source hashes, selected target/compiler, commands, exit statuses, suite
counts and classified limitations. Check that the source hashes still match
after tests. Use at most two build jobs on the shared two-core guests and
coordinate concurrent work there. Reuse one workspace and transfer archive;
remove only task-owned superseded products after recording the evidence.

The [restructuring verification record](gfx-restructure-verification.md) records
the current refactor's results and scope. Native graphics/input behavior on
other platforms still needs those platforms' tests when their code changes.

<!-- end of gfx-maintenance.md -->
