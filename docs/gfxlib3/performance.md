# gfxlib3 performance programme

## Purpose

gfxlib3 is intended to be a compatible graphics runtime that executes drawing
on the selected GPU. It must not claim a performance win merely because the
Basic thread queued commands before the renderer completed them. Every
throughput timing therefore ends with a compatible ordered readback or
presentation boundary. A producer timing is valid only when the same fixture
also reports the later completion boundary.

The exhaustive language and ABI inventory remains
[`gfxlib2-api-inventory.md`](gfxlib2-api-inventory.md). This document classifies
every inventory family according to whether a GPU performance test is useful,
and records the renderer path expected for each drawing family.

## Running the desktop matrix

From the source root on Windows:

```powershell
pwsh -File tests/gfx3/run-performance-matrix.ps1
```

The runner builds every registered fixture three times: gfxlib2, forced
gfxlib3 OpenGL, and forced gfxlib3 Vulkan. Each compiled fixture runs three
times by default and prints machine-readable `BENCHMARK=`,
`BENCHMARK_SAMPLE=`, `*_seconds=`, and `BENCHMARK_MEDIAN_*` records. Use
`-Runs 1` only for a quick smoke pass. Results are hardware observations, not
universal pass/fail thresholds.

## Command-family benchmark manifest

| gfxlib2 inventory family | Performance fixture | Expected gfxlib3 path | Status |
| --- | --- | --- | --- |
| `SCREEN`, `SCREENRES`, renderer selection | `mode-open-benchmark.bas` | backend selector then native WSI initialization | measured; cold-start gap recorded |
| `SCREENSET`, `FLIP`, `SCREENCOPY`, `SCREENSYNC` | `screen-state-benchmark.bas`, `oma-sprite-benchmark.bas`, `page-flip-presentation-smoke.bas` | ordered GPU blit plus explicit present | measured and regression-tested |
| `SCREENLOCK`, `SCREENUNLOCK`, `SCREENPTR` | `screen-state-benchmark.bas` | synchronized CPU shadow, upload only at the ordering boundary | measured; performance gap recorded |
| `PSET`, `PRESET`, `POINT` | `primitive-benchmark.bas` | batched `POINTS` GPU command; `POINT` is an ordered GPU readback | measured |
| `LINE`, box, filled box | `primitive-benchmark.bas` | line and rectangle GPU shaders | measured |
| `CIRCLE`, ellipse, arc, fill | `primitive-benchmark.bas`, `arc-benchmark.bas` | ellipse GPU shader or ordered arc-point command | ellipse and arc measured |
| `PAINT` | `paint-benchmark.bas`, `primitive-benchmark.bas`, `paint-large-gpu-surface-smoke.bas` | exact compute for desktop pages and GPU surfaces; GLES shader frontier for GPU surfaces; cached CPU shadow for normal GLES pages | producer and completed-work timing added; repeated screen workload now beats gfxlib2 on every tested backend |
| `DRAW` | `draw-benchmark.bas` | parser on CPU, constituent GPU primitive commands | measured |
| `DRAW STRING`, graphical console | `primitive-benchmark.bas`, `console-benchmark.bas`, and console smokes | GPU glyph/rectangle path after CPU text-state update | built-in text and console output measured |
| `VIEW`, `WINDOW`, `PMAP`, `POINTCOORD` | included as state in every drawing fixture | CPU coordinate/state preparation, then GPU command | correctness paths, no independent GPU benchmark |
| `PALETTE` family | `screen-state-benchmark.bas` | GPU palette upload/presentation lookup | measured; performance gap recorded |
| `IMAGECREATE`, `IMAGEDESTROY`, `IMAGEINFO` | `image-allocation-benchmark.bas`, `image-cache-benchmark.bas`, `large-image-cache-smoke.bas` | CPU compatibility-image allocation plus exact mutable-image residency validation | measured as a CPU compatibility family; large mutable reuse and direct pointer edits verified |
| `GET`, `PUT` all standard modes | `transfer-benchmark.bas`; `primitive-benchmark.bas` retains the common transparent-sprite workload | upload/temporary GPU surface then GPU blit/compute blend; `GET` downloads | full built-in mode and GET matrix implemented |
| `BLOAD`, `BSAVE`, `IMAGECONVERTROW` | `file-row-benchmark.bas` | CPU parsing and conversion, GPU upload only for a screen target | measured as a compatibility-only family |
| input, events, joystick, touch, `SCREENCONTROL`, `SCREENGLPROC` | latency/correctness smokes | platform/input state and renderer control, not drawing | no GPU throughput metric applies |
| graphical keyboard, `PRINT`, `LOCATE`, `WIDTH`, `LINE INPUT` | `console-benchmark.bas`; console/input smokes | CPU console state with ordered GPU glyph updates | PRINT, LOCATE, and WIDTH measured; interactive LINE INPUT remains latency-only |

The public gfxlib3 surface extension is benchmarked independently because it
allows applications to keep source, target, and transfers in device memory.
Its required workloads are device-to-device blit, primitive batch, map/upload,
map/download, and visible present. They are not substitutes for the compatible
FB.IMAGE paths above.

## GPU-path audit

The rendering families above must remain on these paths:

| Public operation | Common command | Desktop OpenGL | Vulkan | Android AGM A8 |
| --- | --- | --- | --- | --- |
| clear, points, lines, rectangles, ellipses | `CLEAR`, `POINTS`, `LINE`, `RECTANGLE`, `ELLIPSE` | compute shader over integer textures | Vulkan compute pipeline | GLES 3 fragment/raster path |
| standard `PUT` and `SCREENCOPY` | `BLIT` | compute blit; direct instanced raster batch for adjacent PSET/TRANS | compute blit | GLES shader blit; direct instanced batch for adjacent PSET/TRANS |
| solid/patterned `PAINT`, GPU-only surface | `PAINT` | compute frontier | compute frontier | GPU ping-pong fragment frontier |
| solid/patterned `PAINT`, normal screen page | `PAINT` on desktop; bounded flood plus deferred `SURFACE_UPLOAD` on GLES | exact compute with rectangular specialization and scanline fallback | exact compute with rectangular specialization and scanline fallback | retained CPU shadow and one deferred dirty upload |
| `POINT`, `GET`, surface download | `READ_PIXEL`, download command | framebuffer readback after ordered fence | staged device-to-host copy | GLES readback |
| visible display | `PRESENT` | presentation shader and WGL/GLX swap | swapchain image transfer and queue present | GLES presentation shader and EGL swap |

CPU fallback is permitted only where it preserves a materially faster compatible
operation. Besides compatibility-only data such as ordinary FB.IMAGE allocation,
file decoding, custom CPU blend callbacks, or a required SCREENLOCK shadow, this
currently includes PAINT on a normal GLES screen page. The ES 3.0 frontier
advances only one Manhattan pixel per raster pass, making a conventional
full-page fill pathological. gfxlib3 retains a CPU shadow across compatible
fills and performs one tightly scoped deferred upload instead. Desktop screen
pages and all GPU-only surfaces retain their GPU implementations. A performance
report must identify the GLES hybrid path rather than call it fully GPU
accelerated.

## Current measured finding, 2026-07-18

The first direct matrix showed that correctness has outrun throughput. On the
current desktop, the pre-optimization primitive run was substantially slower
than gfxlib2 for immediate PSET, LINE, boxes, ellipses, and PAINT. The OMA
sprite frame also spent most of its time in presentation and repeated ordered
batch work. Those are failing performance results, not an acceptable final
state.

The first repairs now in the tree are:

- page-copy and visible-page changes enqueue presentation without blocking the
  Basic thread; `SCREENSYNC` remains the explicit completion operation;
- desktop WGL and Android EGL request a zero swap interval, and Vulkan chooses
  IMMEDIATE presentation when supported;
- the renderer and OpenGL transparent-PUT winner batch accept 1,024 ordered
  commands rather than breaking a normal sprite frame into sixteen 64-command
  framebuffer resolves;
- normal PSET staging accepts 65,536 points per bounded GPU command.

The focused public PSET path also removes a redundant mode-mutex acquisition
while the runtime-wide graphics lock is already held. On the completed-work
fixture, PSET improved from 0.29906 to 0.02792 seconds on OpenGL and from
0.38232 to 0.02265 seconds on Vulkan. This is progress, not parity with the
gfxlib2 memory-buffer loop.

The filled-ellipse path has now been changed from one shader invocation that
both advances the midpoint state and writes every pixel, to a 64-lane compute
workgroup. Lane zero keeps the exact Float64 midpoint state machine and the
other lanes cooperatively write each opaque filled span. This does not reorder
any Basic command. Outlines and alpha-filled ellipses deliberately retain the
serial path, because their overlapping destination reads are observable.
`circle-compat-smoke.bas` produced its established `6BDC39D7` image hash on
both forced desktop OpenGL and forced Vulkan after the change. On the RTX
2060, the 256 filled circles in `primitive-benchmark.bas` fell from about
0.61 seconds to 0.046 seconds on OpenGL and from about 0.45 seconds to 0.083
seconds on Vulkan. This is a material example of primitive math and pixel
coverage being performed by shader lanes rather than the Basic thread.

The PAINT selector now chooses the bounded transfer path for transfer-capable
screen pages on all three renderers. A full-page compatibility shadow is retained
across consecutive PAINT commands and invalidated by the next GPU draw, avoiding
redundant downloads while preserving command ordering. The GPU-only surface path
continues to use compute/frontier rendering and passes
`paint-large-gpu-surface-smoke.bas` on forced OpenGL. Pattern, depth, and alpha
PAINT regressions pass on both forced desktop GPU backends.

On the current desktop matrix, the original GPU-frontier PAINT section measured
8.95 seconds on OpenGL and 15.84 seconds on Vulkan, versus 0.097 seconds in
gfxlib2. The first hybrid measurements reduce the corresponding sections to the
roughly 1 to 2 second range. This is a substantial correction to a pathological
algorithm, but it is still not performance parity and remains an open optimization
item.

These changes are regression-tested by the explicit OpenGL and Vulkan page-flip
smoke. They do not yet establish the required overall performance lead. The
next implementation work is multi-command batching for rectangle and text,
more Vulkan blit coalescing, and further PAINT parallelization beyond the
current GPU scanline algorithm.

## GPU scanline PAINT, 2026-07-18

Desktop OpenGL and Vulkan PAINT now keep their visited map and queue in
GPU-resident transient storage but enqueue horizontal spans rather than every
pixel. One scanline seed expands left and right until the submitted border
colour, then discovers non-border spans above and below. This is still the
same four-neighbour connected-region rule: a diagonal cannot cross a wall,
and every discovered pixel is marked before it is queued. It does not stage a
screen or `Gfx3Surface` through the Basic thread.

The RTX 2060 primitive fixture measured PAINT at 0.75213 seconds on OpenGL
and 2.25265 seconds on Vulkan, compared with the preceding roughly 9.04 and
13.70 second pixel-FIFO paths. Forced Vulkan on the Intel UHD 630 completed
the same PAINT section in 2.11353 seconds. Alpha, patterned, 8/16/32-bit
pattern-layout, and large GPU-only surface PAINT smokes passed on both desktop
backends. This is a material reduction, but it is not gfxlib2 parity yet:
the reference memory-buffer fixture remains about 0.06687 seconds.

## Ordered Vulkan LINE batches, 2026-07-18

The Vulkan renderer now recognizes consecutive `LINE` commands targeting the
same GPU surface and records up to 256 of them in one Vulkan command submission.
Each workgroup still runs in command order and has a compute memory barrier
before the following line. This keeps overlapping lines and destination-read
alpha blending compatible while removing repeated command-buffer setup,
submission, and fence costs from the Basic thread.

On the RTX 2060, `primitive-benchmark.bas` reduced its LINE section from
approximately 3.07 seconds to 0.16766 seconds. Forced Vulkan line-pattern,
command-order, and alpha-primitive smokes passed on both the RTX 2060 and
Intel UHD 630. The complete Intel primitive fixture remains bounded by the
serial PAINT workload and timed out at 124 seconds, so no Intel full-fixture
throughput claim is made from that run.

## Android scope

The attached AGM A8 is Android API 24 with an Adreno 306 and OpenGL ES 3.0. It
does not advertise a usable Vulkan feature, so it is a physical GLES benchmark
target only. It cannot honestly validate the Android Vulkan renderer. A
Vulkan-capable Android device is required for that matrix column.

Android presentation is also ultimately paced by the NativeActivity compositor.
Measure device rendering and completed readbacks separately from refresh
cadence, and retain a visible-frame regression test for every presentation
change.

On 2026-07-18, freshly packaged `android-renderer-smoke.bas` and
`android-primitive-benchmark.bas` both completed on the attached device with
`FREEBASIC_ANDROID_EXIT:0`. The completed-readback primitive timings were:

| Family | Workload | Seconds |
| --- | ---: | ---: |
| PSET | 20,000 | 0.0997 |
| LINE | 500 | 0.2377 |
| filled box | 500 | 0.0801 |
| filled circle | 32 | 0.2268 |
| PAINT | one bounded region | 0.0859 |
| DRAW STRING | 128 | 0.0780 |
| PUT TRANS | 1,024 16 by 16 sprites | 0.0547 |

The final ordered pixel readback returned `4278190080`. This validates the
actual Adreno GLES renderer rather than merely a title screen.

The GLES backend now combines adjacent `PUT PSET` and `PUT TRANS` commands
that share source, destination and clipping state into one instanced GPU raster
draw. Its fragment shader writes only the source texture and relies on normal
raster primitive order, which retains legacy last-writer semantics for these
non-destination-reading modes. It neither snapshots nor uploads each sprite.
On the same Adreno 306 workload, 1,024 transparent 16 by 16 PUTs fell from
0.9188 to 0.0547 seconds, a 16.8-fold improvement. The Android OMA-style
RGB565 sprite fixture also completed correctly: 30 frames of 1,024 sprites
took 1.676 seconds with its ordered page copies and readbacks included.

## Android revalidation correction, 2026-07-18

The current physical-device run used the checkout's Android packager with the
checkout's ARM64 gfxlib3 PIC archive. The installed package root did not carry
that archive and produced an unresolved `fb_GfxScreenRes` symbol before native
startup, so it is not valid for gfxlib3 testing.

After rebuilding the ARM64 archive with NDK 27.2.12479018, both fresh APKs
ran on the AGM A8 (API 24, Adreno 306, OpenGL ES 3.0). The renderer smoke
reached and remained in its success-display interval; the primitive workload
then exited with `FREEBASIC_ANDROID_EXIT:0`. It covered batched PSET, LINE,
filled box, filled circle, PAINT, DRAW STRING, transparent PUT, and ordered
POINT readback on actual GLES hardware.

This retest also repaired GLES PSET: the point batch now explicitly selects
the POINT operation in the shared primitive shader. Previously it inherited
the preceding primitive type, allowing a PSET to test stale LINE, BOX, or
CIRCLE coverage and leave the destination untouched. The benchmark does not
write its printed timing values to Android logcat, so this revalidation proves
completion and correctness, not a new numeric Android timing claim.

## Complete built-in transfer matrix, 2026-07-18

`tests/gfx3/transfer-benchmark.bas` now measures each built-in compatible PUT
mode separately: `PSET`, `PRESET`, `AND`, `OR`, `XOR`, `TRANS`, `ALPHA`,
explicit-alpha `BLEND`, and `ADD`. It also measures `GET` separately because
GET is an intentional ordered GPU-to-CPU transfer rather than a primitive
whose completion can be hidden behind queueing. The fixture ends every section
with a completed readback and modifies the source FB.IMAGE after the timed
work. The final PSET proves that cache reuse observes a later source edit.
`PUT CUSTOM` remains outside this throughput fixture because its caller-owned
Basic callback is necessarily a synchronized CPU operation.

The image cache now keeps an exact tightly packed snapshot for source images
up to one MiB. This preserves direct FB.IMAGE-write compatibility using a
row-wise `memcmp` instead of relying solely on a probabilistic content hash.
Larger cached images retain the bounded hash fallback so a cache of transient
background images cannot silently duplicate an unbounded amount of application
memory.

On the current desktop machine, the fixture completes on forced OpenGL and
Vulkan with the same final source-edit pixel. The measured result is not yet
competitive with gfxlib2 for repeatedly drawing a small CPU image. For 4,096
32 by 32 transfers, gfxlib2 measured roughly 0.006 to 0.024 seconds per
built-in PUT mode, while current gfxlib3 samples measured approximately 0.046
to 0.462 seconds on OpenGL and 0.058 to 0.111 seconds on Vulkan. Completed GET
readbacks varied from 0.022 to 2.10 seconds on OpenGL and 0.088 to 0.68 seconds
on Vulkan across warm and cold runs, versus 0.00018 seconds for the gfxlib2
memory copy. Future comparisons must retain repeated samples and report a
median; these are open performance deficits, not parity claims.

The current physical Android GLES result is also recorded rather than inferred.
On the AGM A8, 512 32 by 32 PUTs completed in 0.078 seconds for `TRANS` and
0.100 seconds for `PSET`; destination-reading modes ranged from 0.413 to
0.463 seconds. The 64 GET operations completed in 0.032 seconds. The final
source-edit/readback pixel was `4285267712`, and the process returned
`FREEBASIC_ANDROID_EXIT:0`.

The rebuilt Android OMA fixture then completed 30 frames of 1,024 RGB565
transparent sprites in 1.698 seconds with `oma_sprite_benchmark_pixel=0`.
This validates the exact current GLES archive after the transfer-cache and
queue changes. Android Vulkan remains untestable on this handset because the
system reports no Vulkan HAL.

## Shared screen GET snapshot and PRESET batching, 2026-07-18

Normal 32-bit screen `GET` now reuses the compatible page shadow as a
read-only snapshot. The first GET following a GPU screen write downloads the
full current page; following GET rectangles copy from that snapshot until the
next GPU write invalidates it. This preserves the immediate CPU FB.IMAGE
result while avoiding one device fence per small rectangle.

In the 512 GET fixture, the current OpenGL run measured 0.03785 seconds and
the Vulkan run measured 0.09753 seconds, versus preceding 0.87335 and
0.58828-second measurements. Full-page readback is still slower than the
gfxlib2 memory framebuffer and remains a compatibility cost, but repeated
GET no longer serializes the driver hundreds of times.

`PUT PRESET` now joins PSET and TRANS in the distinct-source raster batch on
OpenGL and GLES. PRESET inverts its source before writing and has no
destination read, so raster order retains the exact last-writer rule. The
OpenGL transfer fixture reduced PRESET from 0.17624 to 0.05803 seconds. AND,
OR, XOR, ALPHA, BLEND, and ADD still read their destination and retain ordered
GPU compute work pending a compatible tile-batched path.

## DRAW command-language fixture, 2026-07-18

`tests/gfx3/draw-benchmark.bas` repeats a fixed QB DRAW program that moves to
an absolute coordinate, changes colour, and emits a rectangle as four line
commands. Its fixed string keeps Basic-side string construction out of the
measurement; a final POINT readback verifies the command stream completed.
The first fixture run completed 10,000 DRAW programs with the expected pixel
on gfxlib2 (0.208 seconds), forced OpenGL gfxlib3 (2.331 seconds), and forced
Vulkan gfxlib3 (1.243 seconds). OpenGL now recognizes adjacent opaque lines
with the same target, clip, and style. A GPU selection pass records the last
line reaching each pixel and a resolve pass writes the winning per-line colour.
This preserves FIFO overlap semantics without a dispatch per line; alpha lines
remain on the exact destination-reading shader. The updated OpenGL DRAW sample
completed in 1.116 seconds, and the colour-varying primitive LINE section fell
from the earlier 0.296 seconds to 0.105 seconds. Both remain behind gfxlib2,
but now use bounded GPU batches rather than one CPU-issued compute dispatch for
every compatible line.

## Frame command queue capacity, 2026-07-18

The public mode configuration previously admitted only 1,024 queued commands.
That was a functional bound, not a renderer requirement: a normal frame with
4,096 sprites had to wake and block the producer four times, splitting its
otherwise compatible backend batches. The queue now holds 8,192 commands. It
is still bounded, and its storage is only a command-pointer ring, but it
allows a complete busy BASIC frame to reach the render thread as one ordered
run.

The complete desktop matrix preserved every final validation pixel. On the
current RTX 2060, the 10,000-command DRAW fixture fell from 2.13 to 1.07
seconds under OpenGL and from 1.03 to 0.99 seconds under Vulkan. The OpenGL
filled-box section improved from 0.117 to 0.093 seconds and the Vulkan line
section from 0.160 to 0.126 seconds. Transfer timing did not materially
change, which confirms that those paths are backend-execution limited rather
than queue-backpressure limited.

## Ordered OpenGL logic-operation PUT batch, 2026-07-18

AND, OR, and XOR read their destination, but their required operation is also
an exact OpenGL colour logical operation. gfxlib3 now sends compatible runs of
one source surface, one destination surface, one clip, and one logical mode to
the existing instanced raster batch. It enables `GL_COLOR_LOGIC_OP` only for
that draw and restores normal state immediately afterwards. Fragment order is
the submitted Basic order, so overlapping sprites retain the same per-pixel
result as individual compute dispatches while the GPU performs the source and
destination bit operation.

The forced-OpenGL `image-smoke.bas` passed every built-in PUT mode after the
change. In the 4,096-sprite transfer fixture, current AND, OR, and XOR times
were 0.064, 0.067, and 0.062 seconds respectively, down from the preceding
roughly 0.18, 0.16, and 0.16 seconds. ALPHA, explicit BLEND, and ADD continue
to use the ordered compute path because their packed-channel arithmetic is not
one of OpenGL's fixed logical operations.

## Ordered OpenGL text-point batch, 2026-07-18

`DRAW STRING` and graphical-console glyphs are emitted as short `POINTS`
commands after the CPU has performed the legacy font and cursor-state work.
They already execute on GPU storage, but each command formerly caused its own
compute dispatch. OpenGL now combines an adjacent run when every point is
opaque and every source command has one colour. A selection pass retains the
last Basic command touching each pixel and a resolve pass writes the winning
colour. Mixed-colour and alpha POINTS commands remain on the exact original
shader, so this optimization cannot alter their ordering or blending rules.

The current OpenGL primitive fixture completed its 1,000 built-in text writes
in 0.02050 seconds, down from the preceding 0.22368 seconds. The separate
console and custom-font smoke fixtures passed, including the custom-font
hashes `B1C32E2D` and `5EDE6DDD`. The 10,000-program DRAW fixture also
completed with its expected final pixel in 1.023 seconds. These results are
still behind the same-run gfxlib2 text result of 0.00439 seconds and DRAW
result of 0.314 seconds. Vulkan text remained a documented optimization gap at
this point; its first forced run measured 0.726 seconds for text and 1.208
seconds for DRAW.

Vulkan now also combines a conservative subset of POINTS commands. It joins
only adjacent opaque commands with identical clips whose visible point bounding
rectangles do not overlap. Since no pixel is written by two merged commands,
the GPU is free to execute the unified point list in any invocation order
without changing Basic semantics. The current forced Vulkan primitive run
measured text at 0.02982 seconds, with the expected final pixel and the
custom-font hashes still passing. DRAW measured 1.107 seconds in that run.
Both values remain behind gfxlib2, but the former per-glyph submission cost is
no longer the dominant Vulkan text path.

## Android GLES remeasurement, 2026-07-18

The current ARM64 archive was rebuilt with NDK 27 and installed as fresh APKs
on the attached AGM A8. The primitive fixture exited successfully with final
pixel `4278190080`. Its current timings were PSET 0.10912 seconds, LINE
0.23903, filled BOX 0.06348, CIRCLE 0.20810, PAINT 0.10466, built-in text
0.07537, and transparent PUT 0.07368. The transfer fixture also exited with
`FREEBASIC_ANDROID_EXIT:0` and final source-edit pixel `4285267712`: PSET
0.08259, PRESET 0.03897, TRANS 0.05108, destination-reading PUT modes
0.44969 to 0.54044, and 64 GET operations 0.01846 seconds.

These are current physical GLES measurements, not estimates. They preserve the
existing conclusion: source-only sprite transfers are practical on this API-24
Adreno 306, while destination-reading transfer modes and text still need more
batching work. The device exposes OpenGL ES 3.0 but no Vulkan HAL, so Vulkan
cannot be validated on this particular phone.

## Current full desktop matrix, 2026-07-18

The final post-logical-operation matrix completed with matching final pixels
for every primitive and transfer backend run. It still does not establish the
requested broad throughput parity with gfxlib2. The clearest remaining gaps
are repeated `CLS` (OpenGL 0.465 seconds versus gfxlib2 0.082), opaque
filled boxes (OpenGL 0.094 versus 0.005), PAINT (OpenGL 0.633 and Vulkan
0.658 versus 0.055), text (OpenGL 0.029 and Vulkan 0.394 versus 0.002), and
the OMA sprite fixture (OpenGL 0.464 and Vulkan 0.631 versus 0.218). The
destination-reading alpha modes also remain above gfxlib2: current OpenGL
ALPHA/BLEND/ADD were 0.149/0.103/0.080 seconds, while Vulkan was
0.067/0.058/0.063 and gfxlib2 was 0.026/0.026/0.029.

This matrix is intentionally recorded as an open optimization ledger rather
than a performance claim. The next backend design work is an ordered
tile-binned shader path for alpha, blend, and add, followed by command-family
specialization for repeated clears, opaque boxes, glyph runs, and PAINT.

## OpenGL filled-box raster path, 2026-07-18

Adjacent opaque filled `LINE ... , BF` commands now use instanced integer
framebuffer quads instead of compute winner-texture selection. OpenGL raster
primitive order supplies the same last-writer result for overlapping boxes;
alpha boxes remain on the destination-reading compute path. The primitive,
alpha, and page-flip smoke fixtures all passed after the change. The current
OpenGL filled-box section measured 0.07975 seconds, compared with the prior
roughly 0.099 seconds. This is a real reduction in compute and dispatch work,
but it remains well behind the same-run gfxlib2 result of 0.01093 seconds and
is therefore an open optimization item rather than a parity claim.

## Ordered OpenGL destination-reading tile batch, 2026-07-18

The tile-binned destination-reading PUT path now handles 32-bit ALPHA, BLEND,
and ADD from a distinct source surface. The CPU creates a compact ordered list
of the sprite commands touching each 16 by 16 destination tile. One compute
workgroup owns that tile, loads its destination pixel once, and replays only
its tile's commands in Basic order before writing the final pixel. Tiles own
disjoint pixels, so no atomics or cross-workgroup ordering are required.
Unusual shapes (self-blits, non-32-bit surfaces, or an excessively large tile
grid) retain the existing exact one-dispatch path.

The forced-OpenGL image smoke passed after the completed three-mode change.
In the 4,096-sprite transfer fixture on the RTX 2060, ALPHA, BLEND, and ADD
completed in 0.06433, 0.06328, and 0.07003 seconds respectively, with the
expected final pixel. A separately forced Intel UHD 630 OpenGL process
reported its actual adapter and produced 0.05782, 0.05270, and 0.05508
seconds with the same final pixel. This remains above the current gfxlib2
results, but the work is now executed as ordered GPU tile programs rather
than 4,096 CPU-driven destination-read dispatches.

## Screen-state command benchmark, 2026-07-18

`screen-state-benchmark.bas` measures indexed `PALETTE` updates, ordered
`SCREENSET`/`SCREENCOPY`/`SCREENSYNC` traffic, and 32-bit
`SCREENLOCK`/`SCREENPTR`/`SCREENUNLOCK` compatibility access. The fixture is
included in `run-performance-matrix.ps1` for gfxlib2 and both forced desktop
gfxlib3 renderers. Each section ends at an ordered visibility or readback
boundary, so it cannot report merely queued CPU work.

For 256 operations on the current desktop, gfxlib2 measured palette 0.01026
seconds, page traffic 0.02314 seconds, and lock access 0.000015 seconds.
OpenGL measured 0.60661, 1.52352, and 0.90123 seconds; Vulkan measured
0.04576, 0.51866, and 0.83249 seconds. The matching final lock pixel was
`4279511295` in all runs. These results confirm the existing GPU paths and
also identify palette submission, per-page synchronization, and full-shadow
uploads as open compatibility performance deficits.

Adjacent palette changes now coalesce in both desktop GPU backends. CPU palette
queries still read the common mode state immediately, and a PRESENT remains an
ordering boundary; only superseded renderer palette snapshots are omitted.
The same 256-update fixture now measures 0.00879 seconds on OpenGL and
0.00391 on Vulkan, with the same final pixel. This removes the palette command
submission bottleneck; page and lock synchronization remain open work.

The connected AGM A8 Android device (Adreno 306, OpenGL ES 3.0) completed the
same fixture with exit status zero. Its latest run measured 0.04178 seconds
for palette traffic, 0.09331 seconds for page traffic, and 1.64793 seconds
for the SCREENLOCK/SCREENUNLOCK path. The page measurement was 1.59950 seconds
before the GLES direct-copy and asynchronous-present coalescing paths. Its
final pixel was correct for the reduced Android iteration count. The device
has no Vulkan HAL, so this confirms the GLES path only. Lock traffic remains a
priority optimization target; the fixture prevents that cost and its semantics
from being hidden by a title-screen-only smoke test.

For desktop OpenGL, a fully visible, non-overlapping 32-bit PSET transfer now
uses `glCopyImageSubData` rather than the general BLIT compute shader. This is
the normal full-page `SCREENCOPY` case and keeps the data in video memory. The
same 256-copy section then measured 0.03656 seconds, down from 0.77160 seconds
before the change, with final pixel `4279511295`. The OpenGL and Vulkan
page-flip presentation smoke fixtures both passed, and the same fixture exits
zero on the attached Android device. A Vulkan transfer-engine experiment was
intentionally rejected because, on this driver, one ordered submission per
compatibility copy was slower than the existing compute path.
After reverting it, Vulkan now defers only intermediate asynchronous PRESENT
commands within one renderer drain, just as the OpenGL backend does. A later
command in that drain cannot observe an intermediate front buffer, while a
synchronous PRESENT remains a one-command completion boundary. The current
Vulkan run measured 0.14807 seconds for this section with the exact final
pixel, down from 0.44143 seconds before present coalescing. Copy submission
batching remains open, but this removes repeated swap-chain work without
carrying a transfer-path regression.

GLES uses the equivalent framebuffer copy on the same fully visible 32-bit
PSET case. It also now coalesces intermediate asynchronous PRESENT commands
within one renderer drain. The attached Android run above verifies both paths
on real hardware and reduces its page section from 1.59950 seconds to 0.09331
seconds. The Adreno result remains above gfxlib2, but presentation churn is no
longer the dominant page-copy cost.

## SCREENRES presentation warm-up, 2026-07-18

`tests/gfx3/image-cache-benchmark.bas` now separates the first compatible
CPU-image PUT from a second fresh image and from a 4,096-sprite steady-state
run. It showed that the apparent OpenGL PSET regression was not per-sprite
math: the first PUT cost about 0.40 seconds, a second fresh image cost about
0.008 seconds, and the same-image sprite run cost about 0.053 seconds. The
one-time delay was the window driver's deferred first presentation.

Mode initialization now performs one ordered present before `SCREENRES`
returns. This moves native swap-chain and presentation-pipeline preparation to
the API boundary where a caller expects the screen to be ready. After a forced
rebuild, the OpenGL cold PSET measurement was 0.01264 seconds, the second
fresh image was 0.00971 seconds, and the 4,096-sprite run was 0.06571 seconds.
The normal transfer fixture's PSET section was 0.06479 seconds, rather than
the earlier roughly 0.7-second first-use result. Vulkan's cold PSET measured
0.00982 seconds with a 0.07586-second steady-state run. These measurements
still require further work to beat gfxlib2 consistently, but they now measure
the sprite command path instead of deferred screen initialization.

## OpenGL asynchronous page-present coalescing, 2026-07-18

Page-selection and page-copy APIs enqueue non-blocking PRESENT commands. The
OpenGL executor now retains only the final visible asynchronous present in one
renderer drain, then performs one swap at the normal ordered batch boundary.
An explicit synchronous `SCREENSYNC` remains a one-command completion boundary
and still presents immediately. This keeps page-flip semantics while avoiding
hundreds of unobservable front-buffer swaps in a CPU loop.

Forced OpenGL `screen-state-benchmark.bas` retained the final lock pixel
`4279511295` and reduced the 256-operation page-traffic section from the
previous 1.52352 seconds to 0.04278 seconds. `page-flip-presentation-smoke`
also exited zero. This is close to, but still slower than, the same-run
gfxlib2 page result of 0.02314 seconds; it is a measured page-flip repair,
not a claim of broad parity.

## Historical OpenGL PAINT scratch-buffer reuse, 2026-07-18

The exact OpenGL PAINT path previously created and deleted its full GPU
scanline-queue buffer for every fill. The shader already clears that queue
before using it, so the backend now retains one grow-only scratch buffer for
the lifetime of its render context. This removes repeated large driver
allocations and their possible retirement synchronization without changing the
border, pattern, or alpha rules.

The forced OpenGL primitive fixture retained its final pixel and measured
PAINT at 0.48050 seconds, down from the preceding roughly 0.63 seconds.
Pattern and large GPU-surface PAINT smokes both passed. A 256-lane and then a
1024-lane workgroup flood-fill prototype were also measured. They were correct
but took about 0.52 and 0.64 seconds respectively because their global queue
atomics and image synchronization cost more than they saved. They were not
retained. The persistent-buffer serial scanline shader remains the current
implementation, with a repeat measurement of 0.50935 seconds after restoring
it. At this checkpoint parallel PAINT remained an explicit open item, pending
a work-efficient algorithm rather than merely more shader invocations. The
2026-07-22 multi-dispatch section records the later exact rectangular solution.

## Opaque arc command fixture, 2026-07-18

`arc-benchmark.bas` draws 4,096 varied, opaque non-radial `CIRCLE` arcs on
the desktop and 256 on Android. The fixture ends with `POINT(0, 0)`, so the
reported time includes completion of the queued drawing commands. It is part
of the standard desktop matrix and prints the measured duration, arc count,
and final pixel.

On the current desktop, gfxlib2 measured 0.24481 seconds, forced OpenGL
measured 0.12411 seconds, and forced Vulkan measured 0.11161 seconds. All
three reported final pixel `4278190080`. The established full circle and arc
compatibility image hash, `6BDC39D7`, also passed on both forced desktop GPU
backends after the benchmark was added.

The attached AGM A8 completed fresh gfxlib2 and gfxlib3 GLES APKs with exit
status zero and final pixel `4278190080`. It measured 0.09731 seconds for
gfxlib2 and 0.14306 seconds for gfxlib3. This is a GLES compatibility pass but
not an Android performance win.

OpenGL already executes the arc pixels through its GPU point path and beats
the current gfxlib2 result for this fixture. Vulkan records adjacent opaque
`POINTS` commands in one submission, retaining one descriptor range, compute
dispatch, and write-to-read barrier per command. The first retained ordered
batch used a 64-command backend limit. It replaced the preceding
0.91890-second result with a 0.11161-second result without changing the image
hash.

The Vulkan runtime has capacity for 256 ordered dispatch descriptors per
submission. The backend now uses that whole capacity and no longer performs a
second, obsolete point-bound overlap scan. This is safe because the runtime,
unlike a merged point shader dispatch, barriers every command before recording
the following one. A fresh RTX 2060 run measured 0.07465 seconds for the
4,096 arcs, down from 0.90890 seconds in the immediately preceding run; a
forced Intel UHD 630 run measured 0.11635 seconds. Both reported final pixel
`4278190080`, and both adapters retained the `6BDC39D7` circle fixture hash.
A trial that fed arcs through the general PSET batch was rejected: an arc
revisits pixels, while unordered compute invocations cannot preserve
repeated-write semantics. GLES still submits each compatible arc as an ordered
point command and remains an Android performance deficit.

## Deferred SCREENLOCK shadow commit, 2026-07-18

`SCREENPTR` exposes CPU-writable storage, but a sequence of
`SCREENLOCK`/`SCREENUNLOCK` pairs does not require a device transfer until a
following GPU operation, ordered `POINT`, page operation, or `SCREENSYNC`
needs the page. The former compatibility path synchronously submitted a
full-page upload and renderer barrier on every outer unlock. That made 256
one-byte edits to a 320 by 240 page cost roughly 0.8 seconds on OpenGL and
0.7 seconds on Vulkan.

The shadow now remains authoritative across outer unlocks. The first GPU
consumer commits it in command order; `SCREENSYNC` explicitly performs that
commit before presenting, and `POINT` commits before its GPU readback.
Nested locks retain eager dirty-range handling so a pointer retained by an
outer lock remains compatible with a GPU draw between unlocks. The benchmark
now includes `SCREENSYNC` before timing is reported, preventing an empty CPU
queue from being mistaken for a completed transfer.

On the current RTX desktop, the completed 256-edit section measured 0.04834
seconds on OpenGL and 0.05616 seconds on Vulkan, with final pixel
`4279511295`; the threaded OpenGL archive measured 0.03181 seconds. gfxlib2
still completes the same memory-buffer loop in 0.01356 seconds, so this is a
large compatibility-path reduction, not yet an across-the-board win.

## Historical isolated screen PAINT timing, 2026-07-18

`paint-benchmark.bas` separates the conventional border-enclosed screen PAINT
workload from the other primitive families. It reports the first completed
fill separately from the remaining completed fills, and checks the interior
pixel, final pixel, and border pixel. It is included in the desktop matrix for
gfxlib2, forced OpenGL, and forced Vulkan.

The initial desktop run recorded gfxlib2 at 0.00877 seconds for the first
fill and 0.09549 seconds for the remaining fifteen. Before deferred shadow
commit, forced Vulkan recorded 0.20593 and 0.82249 seconds on the RTX 2060.
The CPU page shadow now remains authoritative across consecutive compatible
PAINT calls and uploads once when the final ordered `POINT` consumes it. Its
reusable visited map and FIFO queue also avoid heap allocation per fill. A
warm RTX run measured 0.15821 seconds for the first fill and 0.37293 seconds
for the remaining fifteen; forced OpenGL measured 0.18076 and 0.38511 seconds.
All runs retained the expected first pixel `4280226057`, final pixel
`4291850288`, and border pixel `4278190335`.

This confirms that normal, transfer-capable screen pages take the intended
bounded CPU flood plus deferred dirty GPU upload path, not the GPU-only PAINT
shader. The result remains materially slower than gfxlib2, so it is an
explicit open optimization item. GPU-only surfaces continue to use the exact
renderer path; any parallel replacement must first preserve the serpentine
large-surface fixture rather than optimizing only rectangular fills.

This paragraph records the implementation at that checkpoint. The 2026-07-22
section supersedes it for desktop pages: bounded OpenGL and Vulkan pages now use
the exact multi-dispatch compute route. Normal GLES pages retain the hybrid
policy described here.

## Same-depth page-copy path, 2026-07-18

gfxlib3 stores logical 8-, 16-, and 32-bit surfaces in matching `R32UI`
textures on desktop OpenGL and GLES. A full, non-overlapping `PSET`
`SCREENCOPY` between surfaces of the same logical depth therefore needs no
shader and no colour conversion. The OpenGL backend now uses
`glCopyImageSubData`; GLES uses its matching framebuffer copy path. Clipped,
self-copying, depth-converting, and non-PSET operations retain their existing
compatible GPU commands.

The exact ARM64 archive was rebuilt and run on the connected Android device.
Its OMA fixture completed 30 frames of 1,024 RGB565 transparent sprites in
1.29995 seconds with final pixel zero and `FREEBASIC_ANDROID_EXIT:0`, improving
on the preceding 1.698-second physical-device measurement. The fixture is
still bounded by its required per-frame ordered pixel readback, and remains
well behind gfxlib2; this is a verified path correction, not a parity claim.

The OMA fixture also accepts `OMA_BENCHMARK_SKIP_COPY` and
`OMA_BENCHMARK_FINAL_READBACK` for diagnosis. On the desktop OpenGL backend,
the corresponding 30-frame measurements were 0.42573 and 0.38780 seconds.
The normal fixture measured 0.52869 seconds. This shows that page copy and the
per-frame completion boundary account for part of the cost, but ordered
transparent sprite rendering is the dominant remaining path.

## Mode-opening fixture, 2026-07-18

`mode-open-benchmark.bas` measures one public `SCREENRES` call, including the
selected renderer's native window, GPU context or device, logical pages, and
initial presentation preparation. It draws and reads one validation pixel only
after the mode-open timer stops, so drawing time is not attributed to startup.

The current desktop run reported 0.38180 seconds for gfxlib2, 1.87765 seconds
for forced OpenGL, and 2.81737 seconds for forced Vulkan. Every run returned
pixel `4278985272`. This is an explicit cold-start performance deficit; the
fixture separates it from the steady-state graphics measurements rather than
allowing deferred first-use initialization to distort them.

The attached Android GLES device completed the same fixture with exit status
zero, pixel `4278985272`, and a 2.25115-second `SCREENRES` measurement. This
is the GLES mode-open result only: the handset provides no Vulkan HAL.

## OpenGL opaque rectangle batch, 2026-07-18

Filled opaque boxes use framebuffer raster order and do not need the 10-bit
winner tags used by ordered compute batches. Their OpenGL cap is now 8,192
commands, matching the renderer queue and allowing the 6,000-box fixture to
reach one instanced draw. The batch also retains the shared destination once
at its tail sequence rather than once per rectangle.

The forced OpenGL primitive fixture retained final pixel `4278190080` and
reduced its filled-box section from 0.10675 seconds to 0.07962 seconds. This
remains above the same-run gfxlib2 result of 0.00712 seconds, so it is a
measured batching improvement rather than a parity claim.

## Graphical-console benchmark and batching, 2026-07-18

`tests/gfx3/console-benchmark.bas` independently measures `WIDTH`, `LOCATE`,
`COLOR`, and graphical `PRINT`. It uses a fixed 640 by 480 mode, relocates
each line, changes the foreground colour, and ends with an ordered `POINT`.
The final pixel establishes that the glyph workload completed, while the
fixture intentionally excludes interactive `LINE INPUT` latency.

The desktop compute backends now encode each console fragment's background
pixels followed by its glyph pixels in one opaque `POINTS` stream. This is the
same observable write order as the legacy rectangle then glyph sequence, but
it lets adjacent PRINT fragments enter the existing GPU point batches. The
new stream reduced 4,000 prints from 1.37754 to 0.57034 seconds on forced
OpenGL and from 5.43745 to 0.63229 seconds on forced Vulkan in the complete
post-change matrix. Both returned pixel `4278190120`. The same matrix's
gfxlib2 run measured 0.26149 seconds, so the path is substantially improved
but is not yet a reason to claim parity.

The connected AGM A8 (Adreno 306, GLES 3.0) uses a backend-specific form: a
raster rectangle clears the background and GPU points draw the foreground.
On this device that avoids expanding every background pixel into a vertex.
The rebuilt ARM64 archive completed 512 prints in 0.517694 seconds with pixel
`4278190080` and `FREEBASIC_ANDROID_EXIT:0`. The device has no Vulkan HAL;
there is no Android Vulkan result to report.

## Desktop adapter verification, 2026-07-18

`vulkaninfo --summary` reported two usable desktop adapters in loader order:
index 0 is the NVIDIA GeForce RTX 2060 and index 1 is the Intel UHD Graphics
630. gfxlib3's documented `FBGFX3_VULKAN_DEVICE_INDEX` diagnostic override
was used to force each device. `command-compat-smoke.bas` exited zero on both,
and the forced Vulkan `draw-benchmark.bas` retained pixel `4280202480` at
0.80530 seconds on the RTX and 1.21723 seconds on Intel. These are completed
GPU results, not loader enumeration only.

The current forced-OpenGL `gl-interop-smoke.bas` reports NVIDIA Corporation
and `NVIDIA GeForce RTX 2060/PCIe/SSE2`, then exits zero. Windows WGL adapter
selection is owned by the operating system's per-executable graphics
preference rather than by a portable OpenGL adapter index. The prior
verification entry records the corresponding Intel UHD 630 OpenGL smoke after
that preference was assigned, so both desktop APIs have exercised both
available adapters within their supported selection mechanisms.

## Deferred SCREENLOCK snapshot, 2026-07-18

`SCREENPTR` exposes raw writable memory, so gfxlib3 maintains a CPU shadow and
uploads it at an ordering boundary. The former implementation also copied the
whole shadow at every outer `SCREENLOCK` in case a later nested lock needed a
changed-row comparison. That made a one-byte edit copy the complete page on
every iteration before any GPU work occurred.

Snapshots now begin only when a lock actually nests. An ordinary outer lock
keeps its valid shadow and defers the one upload until a GPU boundary. A nested
unlock without a snapshot uses its public start/end line range, matching the
legacy dirty-line contract. `screenptr-nested-lock-smoke.bas` passed on forced
OpenGL and Vulkan after the change, and the rebuilt Android application exited
zero on the connected handset.

The isolated desktop lock section measured 0.03196 seconds on OpenGL and
0.01802 seconds on Vulkan for 256 one-byte edits, both with final pixel
`4279511295`; the same gfxlib2 run was 0.01880 seconds. Vulkan has therefore
reached this fixture's gfxlib2 result on this machine, while OpenGL is much
closer. On the Adreno 306, 64 edits fell from the preceding 1.64793 seconds to
0.06195 seconds, with final pixel `4279511103` and
`FREEBASIC_ANDROID_EXIT:0`. This remains a compatibility CPU-memory path, but
the upload and presentation remain GPU-resident.

## Ordered compatibility command submission batching, 2026-07-18

The common context now retains up to 1,024 asynchronous commands behind one
submission mutex before transferring them to the renderer FIFO with one queue
lock and one wakeup. This does not combine or reorder renderer commands: each
command retains its own sequence number, and every synchronous command first
flushes the retained FIFO before submitting its request/response boundary.
The backend therefore continues to execute the existing GPU primitive, blit,
and presentation paths, but a BASIC sprite loop no longer pays a queue lock
and render-thread notification for every PUT.

The isolated OMA workload with page copy removed and one final ordered POINT
fell from 0.39263 to 0.19244 seconds on forced OpenGL and from 0.43155 to
0.23334 seconds on forced Vulkan. The normal OMA workload retained pixel zero
at 0.35839 seconds on OpenGL and 0.51055 seconds on Vulkan. DRAW also retained
pixel `4280202480` while falling to 0.18481 seconds on OpenGL and 0.36634
seconds on Vulkan. This is a substantial command-submission reduction, though
the full OMA path remains slower than gfxlib2's memory-buffer result.

## Historical PAINT span flood and compatibility upload, 2026-07-18

`tests/gfx3/paint-benchmark.bas` measures a large border-based PAINT followed
by repeated fills, with first, final, and border pixels checked. The normal
screen-page form remains a CPU compatibility path because PAINT must preserve
read-after-write behavior for legacy CPU-accessible pages. GPU-only surfaces
continue to use the renderer path; this change does not replace that path with
CPU drawing.

The compatibility flood now queues one seed for each contiguous neighbouring
span instead of one entry per pixel. It fills complete opaque 8, 16, and
32-bit spans directly, while patterned and alpha-blended cases retain the
existing exact per-pixel primitive behavior. The affected viewport is still
uploaded once through the GPU backend after discovery. On the direct desktop
fixture, repeated PAINT fell to 0.17567 seconds on forced OpenGL and 0.14085
seconds on forced Vulkan; the checked pixels were `4280226057`, `4291850288`,
and `4278190335` on both. Pattern and depth smoke tests passed on both
backends after the final span-write change.

The rebuilt ARM64 archive completed the same connected AGM A8 GLES run with a
first fill of 0.07907 seconds, repeated fills in 0.05259 seconds, and checked
pixels `4280226057`, `4281605132`, and `4278190335`; it exited with
`FREEBASIC_ANDROID_EXIT:0`. The gfxlib2 comparison remains faster on this
particular CPU-memory fixture, so this is recorded as a measured reduction,
not a parity or GPU-path claim.

The GPU-only PAINT smoke explicitly forces `fb.GFX_OPENGL` or `fb.GFX_VULKAN`
for the two desktop builds instead of accidentally substituting the null
backend for the latter. It passes on both desktop renderers. After the current
GLES archive rebuild, the expanded physical Android GPU-surface smoke also
exited zero, including its GPU-only PAINT check; that current result supersedes
the earlier Adreno failure recorded during development.

## Compatibility-image allocation benchmark, 2026-07-18

`tests/gfx3/image-allocation-benchmark.bas` now supplies the matrix fixture
for `IMAGECREATE`, `IMAGEINFO`, and `IMAGEDESTROY`. It initializes a compatible
32-bit screen outside the timed region, then creates and validates 10,000
64 by 64 images before destroying each one. The first direct run completed in
0.02321 seconds with gfxlib2 and 0.04129 seconds with gfxlib3, both producing
checksum `168040000`. This family is intentionally reported separately from
GPU command performance: it is a required CPU-compatible allocation path and
does not have a useful shader implementation. The same gfxlib3 fixture ran on
the attached Android handset in 0.48372 seconds with checksum `168040000` and
`FREEBASIC_ANDROID_EXIT:0`.

## Public PUT and GET lock-path reduction, 2026-07-18

`fb_GfxPut` and `fb_GfxGet` already enter with the runtime-wide
`FB_GRAPHICS_LOCK` held. That is the public serialization boundary for active
mode replacement, draw state, and graphics calls. Their former implementation
first took `mode->mutex` to flush pending PSET commands, then took the same
mutex again for the actual transfer. This imposed two unnecessary uncontended
mutex pairs for every sprite while the graphics lock already excluded any
independent public mode or page writer.

gfxlib3 now uses a graphics-lock-aware pending-PSET flush and does the GET or
PUT operation under that existing boundary. Renderer queue ownership,
resource lifetimes, and all synchronous readback boundaries are unchanged.
The change does not turn CPU FB.IMAGE objects into opaque surfaces: the exact
source snapshot check and the normal GPU cache/upload logic still execute.

On the isolated completed-work OMA transparent-PUT workload, 30,720 RGB565
13 by 16 sprites with one final POINT fell from 0.11887 to 0.07731 seconds
on forced OpenGL and from 0.18787 to 0.16725 seconds on forced RTX Vulkan.
The normal page-copy workload retained pixel zero and measured 0.11167 seconds
on OpenGL, 0.26126 seconds on RTX Vulkan, and 0.45257 seconds on Intel UHD
630 Vulkan; gfxlib2 measured 0.01344 seconds on the same machine. This is a
measured CPU front-end reduction, not a claim that the compatible micro-sprite
case now beats the legacy memory loop.

The current Android ARM64 threaded-PIC archive was SHA-256 matched into an
isolated package runtime. The physical AGM A8 GLES OMA workload completed
with pixel zero and `FREEBASIC_ANDROID_EXIT:0` in 1.17121 seconds for 30
frames of 1,024 RGB565 transparent sprites with page copies and ordered
readbacks. The preceding recorded run was 1.676 seconds. Repeated runs of the
smaller 1,024-sprite primitive fixture remained in the 0.060 to 0.064 second
range, so no separate Android microbenchmark speed claim is made from that
variable measurement.

The all-mode desktop transfer benchmark also retained pixel `4290510847`.
On forced OpenGL, 4,096 alpha and explicit blend PUTs completed in 0.01800 and
0.01788 seconds, respectively, compared with gfxlib2's 0.02389 and 0.02277
seconds. The other standard GPU transfer modes remain slower on this small
CPU-image benchmark, and GET necessarily waits for device-to-CPU visibility.
Forced Vulkan completed every mode with the same pixel on both the RTX 2060
and Intel UHD 630.

The current physical Android all-mode transfer run also exited
`FREEBASIC_ANDROID_EXIT:0` with final pixel `4285267712`. Its 512-operation
PSET, PRESET, and TRANS sections completed in 0.07298, 0.04121, and 0.05202
seconds; GET completed 64 small reads in 0.01966 seconds. AND, OR, XOR,
ALPHA, BLEND, and ADD each took roughly 0.40 to 0.46 seconds on this Android
7 Adreno 306 driver. Those destination-reading GLES compatibility paths are
correctly GPU-rendered but remain an explicit mobile performance deficit.

## GLES direct-source destination-reading PUT, 2026-07-18

The GLES destination-reading PUT shader formerly copied both the source
rectangle and the destination rectangle into temporary textures for every
AND, OR, XOR, ALPHA, BLEND, and ADD command. The destination copy is required:
the output framebuffer cannot safely also be sampled. The source copy is not
required when source and destination are distinct textures, which is the
normal `PUT screen, image` case. The shader now receives the source origin and
samples the image texture directly; a self PUT still takes the original source
snapshot before rendering, preserving overlap and feedback semantics.

Uniform locations are also resolved once when the program links. The archive
was SHA-256 matched into the Android package runtime before measurement. On
the connected AGM A8, `transfer-benchmark.bas` retained final pixel
`4285267712` and exited zero. Its 512-operation AND, OR, XOR, ALPHA, BLEND,
and ADD sections measured 0.36988, 0.27860, 0.26862, 0.27152, 0.27080, and
0.28132 seconds, compared with the preceding approximately 0.41 to 0.45
seconds. Transparent PUT fell from 0.05202 to 0.03911 seconds. This removes a
per-command GPU copy, while keeping all blend arithmetic in the fragment
shader and every checked legacy result exact.

## Vulkan reusable ordered-PUT command storage, 2026-07-18

Vulkan batches already shared one queue submission for up to 256 adjacent PUT
commands, but each batch created and deferred-destroyed a host-visible command
buffer. The command records are now a persistently mapped allocation owned by
the selected submission slot. A slot is only rewritten after its fence has
signalled, so descriptor and command-record reuse follows the same safe
asynchronous lifetime as its command buffer.

The normal and `-mt` Win64 archives rebuilt with warnings treated as errors.
On the RTX 2060, the isolated OMA transparent-PUT workload, with page copies
disabled and one final ordered POINT, completed in 0.15795 seconds for 30
frames and retained pixel zero. The immediately preceding measured result for
that fixture was 0.16725 seconds. The full transfer matrix retained pixel
`4290510847`; mode timings vary by driver clock state, so this change is
recorded as a measured allocation reduction with a focused throughput gain,
not as a broad Vulkan performance claim.

## Final comparative matrix snapshot, 2026-07-18

The current `run-performance-matrix.ps1` completed all registered fixtures
with matching result pixels on gfxlib2, forced OpenGL, and forced Vulkan. On
this desktop, the clear path remains the clearest GPU win: 200 clears took
`0.13320` seconds in gfxlib2, `0.01422` in OpenGL, and `0.00764` in Vulkan.
The 4,096-arc fixture also remained ahead at `0.38335` seconds in gfxlib2,
`0.12739` in OpenGL, and `0.12298` in Vulkan. The CPU `DRAW` parser plus GPU
primitive submission completed in `0.22981` seconds on OpenGL versus
`0.34897` in gfxlib2.

The same completed-work run remains candid about the unfinished work. Small
primitive, text, CPU-image PUT, GET, ordinary page PAINT, and OMA sprite
workloads do not yet consistently beat gfxlib2. `IMAGECREATE` now fills owned
images by pixel width once per row, while retaining unaligned-safe conversion
writes. The 10,000-image fixture measured `0.02085` seconds for gfxlib2 and
`0.02252` for gfxlib3 with the same `168040000` checksum, a much smaller gap
than the earlier direct observation but not a claimed win. These results keep
the command-family benchmarks useful as an optimization backlog rather than
allowing renderer selection to be justified by unverified throughput claims.

The rebuilt ARM64 archive ran that same 10,000-image workload on the attached
Adreno 306 handset with checksum `168040000` and
`FREEBASIC_ANDROID_EXIT:0` in `0.33896` seconds. This improves the earlier
`0.48372` device observation while remaining a CPU compatibility measurement,
not a graphics-processor throughput claim.

## Stable solid CPU-image PUT specialization, 2026-07-18

The regular CPU-image cache already makes an exact snapshot comparison before
using a cached GPU texture. It now records the full-image native color during
that snapshot. On compute-capable OpenGL and Vulkan backends, an unchanged
full-image `PSET`, `PRESET`, or non-key `TRANS` PUT becomes a batched opaque
GPU rectangle. The cache texture is still initialized before the shortcut
returns, preserving a later source-reading PUT mode. GLES retains its
instanced BLIT route because individual GLES rectangles made the physical OMA
workload substantially slower.

The expanded image smoke verifies a solid PSET followed by an AND using the
same cached source. It exits zero through forced OpenGL and RTX Vulkan. OMA
samples on the desktop remain display-clock sensitive, so this change is not
claimed as the requested broad gfxlib2 replacement win. On the connected
Adreno 306, the capability gate retained the established shader-batch path:
30 normal OMA frames completed with pixel zero in 1.22822 seconds rather than
the rejected 3.95849-second individual-rectangle route.

## GLES batched-PUT uniform lookup removal, 2026-07-18

The GLES ordered sprite batch links one fixed shader program, but its hot path
was still asking the driver to resolve five uniform names for every batch. The
renderer now resolves `source_image`, surface size, mode, depth, and color mask
once immediately after linking that program. The per-frame path supplies only
the values, so the 30,720 OMA transparent sprite PUTs remain GPU raster work
without repeated program-name lookup on the CPU driver thread.

The ARM64 archive was rebuilt and SHA-256 matched into the isolated Android
runtime before packaging. The connected Adreno 306 completed the normal
30-frame OMA fixture with final pixel zero in 1.13864 seconds and
`FREEBASIC_ANDROID_EXIT:0`. This is a focused physical-device improvement over
the preceding 1.22822-second gated sample, not a general performance claim:
mobile display timing and clock state still affect this end-to-end fixture.

## Exact OpenGL filled CIRCLE batch, 2026-07-18

The standard desktop primitive benchmark had been dispatch-bound for filled
CIRCLE: each of its 256 full circles invoked the exact midpoint compute shader
separately. The new opaque batch runs one midpoint workgroup per circle in a
single dispatch, records the final FIFO winner per pixel, and resolves colors
with a second compute pass. It does not substitute a distance-field circle.

On the current forced-OpenGL run, CIRCLE completed in 0.00584 and 0.00832
seconds, compared with the gfxlib2 reference's 0.00579 seconds and the prior
gfxlib3 matrix sample near 0.04724 seconds. The improvement closes the command
submission gap substantially, but these small visible benchmarks remain
clock-sensitive and are not recorded as a broad win over gfxlib2.

## Owned CPU-image cache generations, 2026-07-18

CPU `FB.IMAGE` objects created by gfxlib3 now carry a private generation in
the unused tail of gfxlib3's 32-byte PUT header. A cache entry can therefore
reuse its GPU source texture without comparing every source byte on every
PUT when gfxlib3 itself knows the image has not changed. All gfxlib3 image
writers advance that generation: PSET, LINE, CIRCLE/ELLIPSE, PAINT, DRAW,
PUT/GET to an image, text output, and BMP BLOAD. `IMAGEINFO` deliberately
marks the image externally writable, which retains the old exact snapshot
comparison forever. This favors correctness for BASIC code that keeps and
writes the returned pixel pointer.

The separate `image-cache-benchmark.bas` makes the one-time upload visible
and is now part of the repeated desktop matrix for gfxlib2, forced OpenGL,
and forced Vulkan. In three completed desktop samples after this change, the
4,096-sprite steady-state PSET section measured 0.02863, 0.03445, and
0.03304 seconds on OpenGL, and 0.04774, 0.04355, and 0.04343 seconds on
Vulkan. Cold-image timings are intentionally reported separately because
they include cache allocation and GPU upload. Display timing still makes this
a measurement point rather than a cross-machine speed claim.

## Ordered Vulkan CIRCLE dispatch batch, 2026-07-18

Vulkan previously allocated a command record and submitted one command buffer
for every ellipse. Adjacent commands now share one host-visible command buffer
and one queue submission, while retaining one exact midpoint dispatch and
compute write-to-read dependency per public CIRCLE. The work remains entirely
in the GPU command path and preserves overlap order.

On the RTX 2060, the forced Vulkan primitive benchmark completed CIRCLE in
0.05639 seconds with its final pixel unchanged. This is materially below the
prior 0.16408-second matrix sample, but remains behind the current OpenGL and
gfxlib2 timings. Vulkan ellipse batching is therefore a verified improvement,
not an end-state performance claim.

## GLES exact filled-CIRCLE span batch, 2026-07-18

OpenGL ES 3.0 deliberately uses a separate path from the desktop compute
backends. The renderer keeps gfxlib2's integer midpoint decisions on the
render thread, then uploads the resulting opaque filled spans as instanced
rectangles. A single GLES draw rasterizes the complete ellipse, rather than
attaching the target and issuing one full-screen primitive draw per scanline.
The fragment shader writes native pixels directly to the GPU-resident surface.
This retains the exact compatibility pixel set without presenting a smooth
distance-field approximation as a legacy CIRCLE.

The path applies only to opaque fills whose two radii are at most 256 pixels.
That bound proves the fixed 1,025-span staging array is sufficient for every
midpoint step. Alpha and outline circles keep the ordered compatibility path,
where duplicate endpoint writes and read-modify-write behavior are observable.
Desktop OpenGL and Vulkan still use their compute paths, where the primitive
math and writes remain on the GPU.

On the attached Android 7 AGM A8, Adreno 306 GLES 3.0, the current physical
primitive fixture measured CIRCLE at 0.01243 seconds with final pixel
`4278190080`. The immediately preceding renderer archive measured 0.35848
seconds for the same gfxlib3 section; the directly packaged gfxlib2 reference
measured 0.37478 seconds. The large change is specific to this previously
submission-bound opaque-fill workload, but is a real GPU-path result rather
than a game-specific optimization.

## File and row compatibility benchmark, 2026-07-18

`tests/gfx3/file-row-benchmark.bas` now completes the command-family matrix
for raw-memory `BSAVE`, `BLOAD`, and `IMAGECONVERTROW`. It opens the same
minimal graphics mode on both libraries because gfxlib2 prepares the active
target even for a raw-memory BSAVE. The fixture separately times 32 writes and
loads of a 256 KiB raw block, then 4,096 conversions of a 4,096-pixel native
32-bit row. It verifies the restored byte checksum and the converted-row
checksum before removing its own file.

The common native 32-bit BGR row conversion in gfxlib3 is now an overlap-safe
`memmove`, after checked byte-count calculation. This is the exact public
`source_is_rgb = 0` identity layout used by FB.IMAGE and BMP decode targets;
other source and destination formats retain the checked per-pixel converter.
The focused desktop samples were 0.00651 seconds for gfxlib2, 0.00097 seconds
for forced OpenGL gfxlib3, and 0.00220 seconds for forced Vulkan gfxlib3. The
checksums matched at `8796093020160`. File-system timings vary with the Windows
cache, so the same single samples do not establish a general BSAVE/BLOAD win.

The rebuilt ARM64 archive and this fixture also completed on the attached AGM
A8, Adreno 306 GLES 3.0 handset with `FREEBASIC_ANDROID_EXIT:0`. Android does
not export the program's printed timing records to logcat, so that result is a
physical-device compatibility check rather than an Android timing claim.

## Repeated opaque PSET coalescing, 2026-07-18

The per-thread PSET staging area now maps each pending coordinate to its point
record. When another opaque PSET reaches that coordinate before an ordering
boundary, gfxlib3 replaces the existing staged colour instead of flushing a
partially filled GPU command. This is valid only for opaque writes: they do
not read their destination and the last write is the only observable result.
Alpha points retain the former flush-before-repeat behaviour because every
alpha operation observes the preceding destination value.

The staging map now holds 131,072 entries, matching the bounded point command
payload. It stores only a coordinate key and a one-based point index, and is
cleared with the command after successful submission. The point records and
two hash arrays occupy 4 MiB per active graphics state, keeping the hot path
bounded while avoiding a scan when a BASIC loop revisits pixels.
`pending-points-order-smoke.bas` verifies a repeated PSET across both a LINE
ordering boundary and ordered POINT reads on forced OpenGL and forced Vulkan.

The current 200,000-PSET desktop fixture retained its final pixel and improved
from the immediately preceding 0.02304 seconds to 0.01517 seconds on OpenGL,
and from 0.02118 to 0.01355 seconds on Vulkan. The rebuilt ARM64 archive ran
`android-pset-throughput-smoke.bas` on the attached AGM A8, Adreno 306 GLES
3.0 handset with final pixel `4278190080`, clean exit, and a logged completed
time of `0.03851` seconds for 20,000 PSET commands. This is a focused command
path result; it does not yet establish broad parity for the remaining command
families.

The queue was subsequently increased from 65,536 to 131,072 entries because
the standard 200,000-PSET fixture otherwise needed four GPU uploads. Three
post-change desktop samples retained final pixel `4278190080` and measured
OpenGL at 0.01655, 0.01403, and 0.01136 seconds, and Vulkan at 0.02004,
0.01790, and 0.01373 seconds. The current ARM64 archive completed the full
40,000-PSET Android primitive fixture on the Adreno 306 in `0.04193` seconds
with clean exit. This trades a fixed 4 MiB compatibility-state allocation for
fewer submissions; it does not alter the shader-based point raster path.

## Vulkan same-colour rectangle compute batch, 2026-07-18

Cached uniform CPU images turn the common opaque `PUT` forms into filled
rectangles. Vulkan previously represented every rectangle row as a transfer
fill command. The runtime now recognizes a same-native-colour run, where
overlap order cannot alter the final pixels, and dispatches the rectangle
compute shader in two dimensions: x owns pixels and y selects one rectangle.
Mixed-colour runs retain the ordered transfer path.

The batch limit now matches the 1,024-command producer queue, allowing one
uniform-sprite frame to reach one compute dispatch. On the RTX 2060, repeated
normal OMA samples were 0.25073, 0.25155, and 0.27837 seconds for 30 frames,
versus the immediately preceding 0.31589-second sample. Intel UHD 630 samples
were near 0.39 seconds, so this is a discrete-adapter gain, not yet a universal
replacement claim.

## GPU-surface command benchmark, 2026-07-18

`gpu-surface-benchmark.bas` now measures extension-only create/destroy, clear,
upload, download, map, blit, and present commands. It is included in the
desktop OpenGL and Vulkan performance matrix and runs at reduced scale on
Android GLES. `benchmark-coverage.md` records the full protocol-to-fixture
mapping and explicitly separates APIs that have no gfxlib2 analogue.

## Conservative cached POINT reads, 2026-07-18

An ordered `POINT` must normally wait until all preceding GPU work is visible.
That is required when a preceding command can touch the requested coordinate,
but it made a common game loop expensive when it repeatedly polled an unchanged
status pixel after drawing elsewhere on the page. gfxlib3 now retains one exact
cached POINT result for each screen page. A later POINT uses it only when the
same page and coordinate are requested outside SCREENLOCK.

The cache is not a framebuffer mirror. Screen PSET invalidates its exact pixel;
LINE and boxes invalidate their bounds; CIRCLE, arcs, text, console writes,
PAINT, VIEW drawing, and SCREENLOCK invalidate conservatively. CPU-image PUT
invalidates only its clipped destination rectangle, while BLOAD and page-copy
invalidate the affected page region. Any uncertain rectangle or pointer-based
SCREENLOCK access discards the cache. This preserves a real GPU readback for
every potentially changed coordinate without placing a CPU raster path in the
normal renderer.

`point-cache-smoke.bas` seeds a read, then verifies invalidation through PSET,
an unrelated LINE, PUT, SCREENLOCK, and CLS. It passed forced OpenGL and Vulkan,
and the rebuilt ARM64 library completed it on the attached AGM A8 with
`FREEBASIC_ANDROID_EXIT:0`. Forced image, BLOAD bitfields, and screen-state
fixtures also retained their established result pixels on both desktop backends.

On the 30-frame OMA transparent-sprite fixture, the normal completed workload
fell from the preceding approximately 0.201 seconds to 0.13277 seconds on
OpenGL and 0.14724 seconds on Vulkan, with final pixel zero. The attached
Adreno 306 GLES device completed the same normal workload in `0.59037` seconds
with final pixel zero and clean exit, compared with the earlier 1.13864-second
physical-device sample. The remaining gap to gfxlib2 is now sprite submission
and page-copy throughput rather than redundant unchanged-pixel readbacks.

## Historical opaque PAINT span storage, 2026-07-18

The exact CPU-assisted screen PAINT path retains a page shadow between
consecutive fills, so repeated normal PAINT calls avoid a GPU download. Its
opaque-solid flood spans now seed one native pixel and duplicate that initialized
byte prefix geometrically. The companion visited interval is marked with one
`memset`. Both operations remain safe for unaligned FB.IMAGE rows and preserve
the existing 8-, 16-, and 32-bit native layouts.

This is deliberately not represented as a GPU compute replacement. PAINT must
retain gfxlib2's border and pattern semantics, and the desktop screen path uses
the CPU flood discovery only where it is cheaper than a GPU readback plus
frontier synchronization. Three forced OpenGL samples retained all benchmark
pixels and measured the 16-fill repeated section at 0.11573, 0.11406, and
0.10146 seconds. Vulkan samples were 0.14551, 0.13183, and 0.10842 seconds.
The focused pattern, depth, and large GPU-surface PAINT smoke fixtures passed
on both forced desktop backends. The current ARM64 archive also completed the
patterned/depth PAINT smoke on the attached AGM A8, Adreno 306 GLES 3.0 device
with `FREEBASIC_ANDROID_EXIT:0`.

## Ordered OpenGL PUT tiles and tight GET readback, 2026-07-18

OpenGL now sends same-source, non-self-referential runs of every standard PUT
mode through the ordered tile compute path. Each 16 by 16 shader workgroup
replays only the commands reaching its destination tile, in FIFO order. This
extends the prior alpha-only path to PSET, PRESET, TRANS, AND, OR, XOR, ALPHA,
BLEND, and ADD without moving their pixel math back to the CPU.

In the 4,096-sprite desktop transfer fixture, repeated OpenGL samples measured
PSET at 0.02889 to 0.03604 seconds, PRESET at 0.01773 to 0.01931 seconds, and
TRANS at 0.01906 to 0.02162 seconds. The preceding raster implementation
measured approximately 0.04087, 0.02581, and 0.02812 seconds respectively.
The result pixels remain unchanged. This is a targeted improvement, not a
claim that every small CPU-image PUT is yet ahead of gfxlib2.

Tightly packed OpenGL GET rows now issue one readback call per requested
rectangle rather than one call per scanline. The same 512-GET fixture fell
from approximately 0.02 to 0.03 seconds to 0.00209 to 0.00388 seconds. Padded
destinations retain the exact per-row fallback, since OpenGL row-stride state
is not otherwise changed.

Vulkan now reuses one fence-safe host-visible download staging buffer. RTX
2060 repeated GET samples were 0.08468 to 0.09771 seconds, versus an earlier
approximately 0.12780-second sample. Vulkan still records one buffer copy per
source row and must synchronously wait for GET, so this is allocation-churn
reduction rather than a final GPU-readback solution.

## Vulkan queue-aligned ordered PUT batches, 2026-07-18

The Vulkan runtime previously capped its ordered compatible PUT batch at 256
commands even though the renderer producer queue holds 1,024. The descriptor
pool, per-submission command-record buffer, and backend staging now share the
1,024-command limit. A compatible sprite frame therefore records one descriptor
update and queue submission instead of four, while still binding and dispatching
each operation in FIFO order for exact overlap and blend semantics.

The 30-frame OMA sprite fixture retained final pixel zero. Forced RTX 2060
Vulkan samples were 0.09085, 0.09071, and 0.10131 seconds; forced Intel UHD
630 samples were 0.14168, 0.15302, and 0.11991 seconds. Earlier comparable
samples were approximately 0.25 to 0.28 seconds on RTX and 0.36 to 0.41
seconds on Intel. The transfer microbenchmark remains dispatch-bound, so this
does not replace the planned single-dispatch Vulkan tile replay.

## Vulkan ordered PUT tile replay, 2026-07-18

The Vulkan general PUT batch now bins commands by 16 by 16 destination tile
and dispatches the shader once for the batch. Shader invocations replay their
tile's FIFO command list, so PSET, PRESET, TRANS, AND, OR, XOR, ALPHA, BLEND,
and ADD still execute their pixel math on the GPU and preserve overlap order.
Self-blits and oversized tile maps retain the earlier ordered path.

With 4,096 32 by 32 sprites, the RTX 2060 measured PSET 0.02734 seconds,
PRESET 0.02238, TRANS 0.02170, and all remaining standard modes 0.01831 to
0.02208 seconds. The immediately preceding per-sprite Vulkan values were
roughly 0.034 to 0.056 seconds. Intel UHD 630 retained the same final pixel
with PSET 0.05691, TRANS 0.04066, and the other modes 0.03742 to 0.06172
seconds. Synchronous GET is unchanged because it is an explicit CPU barrier.

## Vulkan mixed-colour filled rectangle tiles, 2026-07-18

Mixed-colour BOX BF batches formerly fell back to ordered transfer clears
because overlapping colours make command order observable. They now use a
dedicated 16 by 16 Vulkan tile replay shader, while the same-colour specialization
remains unchanged. The primitive fixture retained final pixel `4278190080`.

The RTX filled-box section measured 0.02823 seconds, compared with the
preceding approximately 0.03547-second result. Intel UHD 630 measured 0.03513
seconds, versus the preceding approximately 0.20493 seconds. The result is
especially important on the iGPU, where per-rectangle host transfer submission
had overwhelmed the actual GPU work.

## Vulkan opaque filled CIRCLE winner and resolve, 2026-07-18

The RTX 2060 now uses a two-pass shader implementation for adjacent opaque
filled CIRCLE commands. The first pass retains exact midpoint spans and uses
an atomic maximum command tag for each covered pixel. The resolve pass performs
the final native-colour write. This replaces per-CIRCLE surface write barriers
with two GPU passes while retaining Basic's last-command-wins overlap rule.

On the 256-circle primitive fixture, the RTX CIRCLE section measured
0.00537 to 0.00570 seconds, compared with preceding samples around 0.060
seconds. The final fixture pixel remained `4278190080`. Intel UHD 630 uses the
existing ordered Vulkan midpoint shader, which remains GPU-rendered and
returned the same final pixel in 0.21329 seconds. Its current driver did not
complete the global SSBO atomic version promptly, so gfxlib3 qualifies that
aggressive route to the tested NVIDIA vendor instead of trading correctness or
interactivity for an unverified optimization.

## Producer-side opaque rectangle packets, 2026-07-18

The frequent uniform CPU-image `PUT` specialization already reaches the GPU as
an opaque filled rectangle. Its former producer path still allocated one heap
command per sprite, however, so an OMA frame generated 1,024 commands before
the OpenGL or Vulkan backend could batch the work. gfxlib3 now collects up to
1,024 consecutive opaque rectangles for one target in a bounded protocol
packet. A different target, a normal command, a synchronous operation, queue
capacity, or shutdown flushes the packet first, preserving the public FIFO
ordering boundary.

OpenGL unpacks the packet into its existing GPU rectangle raster batch. Vulkan
clips and compacts its packet before calling the existing rectangle compute
batch, so fully clipped sprites never reach a shader with invalid bounds.
Its compute batch starts at two rectangles; an ordered one-item packet is
routed through the existing GPU clear command. This matters when one filled
box immediately precedes a non-uniform CPU-image PUT, because the transition
must remain a valid Vulkan submission rather than an invalid one-item batch.
GLES deliberately retains the instanced CPU-image BLIT route, which is faster
on the connected Adreno 306 and does not advertise desktop compute support.

The 30-frame OMA fixture still returned pixel zero. Fresh desktop samples were
0.05222 seconds with forced OpenGL and 0.04914 seconds with forced Vulkan,
compared with the immediately preceding roughly 0.076 to 0.089 and 0.105 to
0.114 second ranges. The dense 30,720-rectangle smoke fixture passed on both
desktop backends. The rebuilt Android ARM64 archive completed the unchanged
GLES workload in 0.59998 seconds with final pixel zero. This removes command
allocation and queue traffic from a hot sprite case, but does not yet establish
overall gfxlib3 parity with gfxlib2.

## Vulkan PSET PUT control-flow qualification, 2026-07-19

The all-mode transfer matrix exposed a Vulkan compiler-sensitive control-flow
shape in the PSET and PRESET branches of the compute shaders. Those operations
never read the destination or require the common blend tail, so both shaders
now return immediately after writing their result colour. This keeps their
math on GPU shader units and removes an RTX/Intel PSET stall reproduced by the
new `transfer-path-benchmark.bas` at one command and at full queue depth.

After the change, the RTX all-mode 4,096-sprite fixture completed with PSET
0.02792 seconds, PRESET 0.03987, AND 0.04333, and the remaining built-in modes
0.01967 to 0.02169. Intel completed with PSET 0.04145 and all modes between
0.03103 and 0.04145. Each Vulkan result retained final pixel `4290510847`.
The benchmark measures fully completed work, including 512 synchronous GET
operations, which took 0.09095 seconds on RTX and 0.09844 on Intel.

## One-run packet baseline, 2026-07-19

`run-performance-matrix.ps1 -Runs 1` completed from the packet archive with
the expected final pixel from every gfxlib2, forced OpenGL, and forced Vulkan
fixture before the Vulkan PSET shader control-flow tuning. One sample
identifies the next bottleneck but is not a stable performance claim.

The GPU path is already decisively ahead for repeated clear (0.01100 seconds
OpenGL and 0.01050 Vulkan versus 0.08224 gfxlib2), arcs (0.13993 and 0.10310
versus 0.43036), file-row conversion (0.00108 and 0.00239 versus 0.02273),
and several blend PUT modes. It remains behind gfxlib2 for dense PSET, general
CPU-image PSET/TRANS, text, PAINT, and the OMA fixture. The latest OMA values
were 0.06554 seconds OpenGL and 0.06595 Vulkan versus 0.01618 gfxlib2.

The rebuilt focused Vulkan transfer fixture then reduced PSET to 0.03006
seconds while preserving final pixel `4290510847`; its isolated 4,096-PSET
path measured 0.13102 seconds. The next implementation priority remains
producer-side batching for the general non-uniform CPU-image `PUT` path.
Existing backend tile shaders remain the GPU raster path; the remaining
avoidable overhead is per-sprite command allocation and submission before
those shaders run.

## Producer-side compatible BLIT packets, 2026-07-19

Consecutive, non-self CPU-image `PUT` calls with one source, one destination,
one standard raster mode, and one alpha value now become a bounded `BLITS`
packet before entering the render queue. The packet carries the original
rectangles in FIFO order. OpenGL decodes it into the existing ordered GPU
raster batch; Vulkan decodes it into the existing GPU BLIT batch; GLES decodes
it into clipped instanced raster runs. This removes the public-thread heap
allocation and queue record for each individual PUT; it does not replace a GPU
operation with CPU raster work.

The packet currently covers `TRANS`, `PSET`, `PRESET`, `AND`, `OR`, and `XOR`.
`ALPHA`, `ADD`, and `BLEND` retain their established renderer-side tile batch,
which already combines those ordering-sensitive operations after submission.
Self PUT is deliberately excluded because it requires source snapshot rules.
GLES accepts `TRANS`, `PSET`, and `PRESET` packets, whose framebuffer-independent
results can use its ES 3 instanced draw path without desktop compute support.

Fresh forced desktop transfer runs retained final pixel `4290510847` for every
PUT mode and GET. Their completed-work samples were 0.02740 seconds PSET on
OpenGL and 0.02390 seconds PSET on Vulkan; the 30-frame OMA fixture retained
pixel zero at 0.06138 and 0.06518 seconds respectively. These are single
host samples, not an overall gfxlib2 victory claim: the next comparison pass
must use repeated paired samples before any performance threshold is changed.

The current Android ARM64 archive completed the normal OMA fixture on the
connected Adreno 306 in 0.6018389579840004 seconds with final pixel zero.
That is a GLES compatibility measurement only, not a Vulkan result.

## Dense public PSET qualification, 2026-07-19

`pset-benchmark.bas` issues 200,000 changing-colour public `PSET` operations
without locking the screen. gfxlib3 retains those operations as an ordered GPU
point packet and only synchronizes at the final public `POINT` readback. The
source intentionally avoids a memory surface so that it measures the renderer
command path rather than CPU image conversion.

On the same Win64 build and 640 by 480 target, gfxlib2 completed in 0.3890753
seconds. Forced gfxlib3 OpenGL completed in 0.1326832 seconds, forced Vulkan
on the NVIDIA GeForce RTX 2060 in 0.1807450 seconds, and forced Vulkan on the
Intel integrated GPU in 0.0825465 seconds. Every result read the expected
final pixel `4278190080`. This is a specific dense-point win, not a claim that
all primitive or image paths are now faster than gfxlib2.

The same fixture also ran on the connected Android 7 AGM A8 (Adreno 306,
OpenGL ES 3.0). Its Android-sized 40,000-point stream completed in 0.1162800
seconds with gfxlib2 and 0.0948091 seconds with the freshly rebuilt gfxlib3
GLES archive. Both packages read `4290789568` at the final point and emitted
`FREEBASIC_ANDROID_EXIT:0`. This is one physical-device sample, about an
18 percent improvement, and does not imply mobile wins for every command.

## Coordinate-state command qualification, 2026-07-19

`coordinate-state-benchmark.bas` adds a paired public measurement for VIEW,
WINDOW, and PMAP. VIEW owns visible fill and border work and therefore reaches
the renderer; WINDOW and PMAP change or query compatibility coordinate state
on the calling CPU. On Win64, 512 VIEW operations completed in 0.3724051
seconds with gfxlib2, 0.0365359 with OpenGL, 0.2171363 with RTX Vulkan, and
0.3970123 with Intel Vulkan. Every run read `4294966777` at the ordered pixel.

The same physical Android 7 A8 test completed 64 VIEW operations in 0.3044762
seconds with gfxlib2 and 0.3035354 with gfxlib3 GLES, again with pixel
`4282367417`. The small Android surface does not show a material VIEW
throughput difference. WINDOW and PMAP are deliberately reported separately:
they are state conversion, not a candidate for shader offload, and gfxlib2's
smaller CPU path remains faster in this fixture.

## Main-routine shader pass, 2026-07-21

Built-in glyphs now travel as compact records and are rasterized by an ordered
tile shader. In repeated Win64 samples, the OpenGL console fixture's median was
about 0.137 seconds, compared with about 0.771 seconds before compact glyph
submission. The final Vulkan medians were about 0.261 seconds on the GeForce
RTX 2060 and 0.366 seconds on the Intel UHD 630. The smaller primitive text
section measured about 0.013 seconds on OpenGL, 0.018 on RTX Vulkan, and 0.021
on Intel Vulkan. These timings include the final ordered readback.

PAINT now combines serially exact run discovery with cooperative shader span
writes. Nine RTX Vulkan primitive samples produced a median PAINT time of
0.206 seconds, down from the roughly 0.228-second earlier scanline version in
the same tuning session. OpenGL's 256-lane version produced a roughly
0.138-second median in the dedicated repeated PAINT fixture. Host load and GPU
clock variation were visible, so these numbers describe this machine rather
than a portable threshold.

The following experiments were measured and removed:

* A 65,535-glyph packet increased replay depth and console tail latency.
* Reverse glyph replay with an early exit was slower than forward FIFO replay.
* Vulkan 8 by 8 glyph tiles improved neither backend uniformly and regressed
  the Intel console median to roughly 0.478 seconds. Vulkan therefore retains
  16 by 16 while OpenGL retains 8 by 8.
* Atomically queueing every neighbouring PAINT pixel increased RTX Vulkan's
  median from about 0.206 to 0.213 seconds and discarded the compact-run
  advantage.
* A 128-lane OpenGL PAINT workgroup measured slower than the retained 256-lane
  form.

The final 30-frame OMA sprite workload retained final pixel zero in every run.
Five-run medians were 0.06227 seconds for OpenGL, 0.06313 for Vulkan on the RTX
2060, and 0.05898 for Vulkan on the Intel UHD 630. gfxlib2 remains faster for
this particular CPU-image-heavy fixture in the existing paired matrix, so the
result is recorded as a gfxlib3 improvement and dual-GPU consistency check, not
as a universal speed win.

## GPU-resident transform pass, 2026-07-21

Scaling, rotation, and Mode 7 are new gfxlib3 operations. They do not have a
gfxlib2 entry point with identical sampling, pivot, and projective rules, so a
number labelled as a direct gfxlib2 speedup would compare different programs.
The relevant invariant is that the source and destination are opaque GPU
surfaces, every destination-pixel coordinate is evaluated by shader lanes, and
the timer ends with a one-pixel download that waits for completed GPU work.
Initial asset decoding and upload are outside the timed region.

The retained desktop benchmark performs 1,500 scaled 192 by 160 draws, 750
rotated 128 by 128 draws, and 200 full 640 by 480 Mode 7 draws. A current run
produced:

| Backend and adapter | Scale | Rotation | Mode 7 | Final pixel |
| --- | ---: | ---: | ---: | ---: |
| OpenGL 4.3 compute, RTX 2060 context | 0.08433 s | 0.04076 s | 0.01252 s | 4281135214 |
| Vulkan compute, NVIDIA RTX 2060 | 0.03014 s | 0.01917 s | 0.01453 s | 4281135214 |
| Vulkan compute, Intel UHD 630 | 0.03014 s | 0.01450 s | 0.01406 s | 4281135214 |

These are completed-work samples rather than submission-only timings. Clock
state and desktop compositor load move individual measurements, so the fixture
prints raw values and deliberately has no vendor-specific pass threshold. The
matching final pixel is the cross-backend correctness guard.

The first Vulkan implementation submitted and waited once per transform. On
the RTX 2060 that measured about 0.081 seconds for scaling and 0.0575 seconds
for rotation. The retained path writes up to 1,024 adjacent transform records
into fenced per-slot command storage and records all dispatches and barriers in
one Vulkan submission. In the first post-change sample, scaling fell to 0.01763
seconds and rotation to 0.01056 seconds. Later samples vary with GPU clocks but
retain the one-submission design and exact output. The Intel driver benefits
most on rotation; its full-screen Mode 7 rate is dominated by pixel work rather
than submission count.

The physical Adreno 306 baseline uses reduced iteration counts to remain below
that older driver's watchdog: 150 scales, 75 rotations, and 20 Mode 7 planes.
Before transform instancing it completed those sections in 0.53479, 0.24272,
and 0.67855 seconds with exit zero. The current GLES renderer packs adjacent
destination-independent transforms into one instanced integer-texture draw.
Destination-reading AND, OR, XOR, ALPHA, ADD, and BLEND modes remain exact
single draws because later operations must observe earlier writes. The reduced
counts change only sample duration, not the shader or pixel workload of an
individual transform.

## Ordinary unscaled sprite qualification, 2026-07-21

The earlier OMA microbenchmark used a uniform `IMAGECREATE` source. gfxlib3
correctly recognized that image as a filled rectangle, so it was not evidence
for ordinary sampled-sprite speed. The corrected fixture builds a patterned 13
by 16 RGB565 image with transparent-key and opaque pixels. For the isolated
comparison it omits `SCREENCOPY`, warms the gfxlib3 image cache before timing,
submits ordinary `PUT TRANS` calls, and finishes with one ordered `POINT` from
a pixel covered by the sprites. The source texture is therefore already in
graphics memory during the gfxlib3 timed loop. The measurement includes
completed rendering and excludes scaling, rotation, page copy, upload, and
per-frame synchronization.

The original 30-frame sample was short enough for GPU clock and desktop
scheduling changes to reverse individual comparisons. The stable
qualification uses the fixture's frame-count override to submit 300 frames of
1,024 sprites. Seven-run desktop medians were:

| Source and backend | 307,200 blits | Approximate blits/second | Relative throughput |
| --- | ---: | ---: | ---: |
| gfxlib2, DirectX, ordinary `FB.IMAGE` | 0.118422 s | 2.594 million | baseline |
| gfxlib3, OpenGL, ordinary `FB.IMAGE` | 0.099463 s | 3.089 million | 1.19 times gfxlib2 |
| gfxlib3, Vulkan, NVIDIA RTX 2060, ordinary `FB.IMAGE` | 0.096594 s | 3.180 million | 1.23 times gfxlib2 |
| gfxlib3, Vulkan, Intel UHD 630, ordinary `FB.IMAGE` | 0.135988 s | 2.259 million | 0.87 times gfxlib2 |

All four routes returned pixel `3784439`. The retained OpenGL and RTX Vulkan
paths are now 19.1 and 22.6 percent faster than gfxlib2 by completed-work
throughput. Intel Vulkan improved from the preceding 0.231170-second median to
0.135988 seconds, about 70 percent more throughput, but is still 12.9 percent
below gfxlib2. This is a real two-adapter improvement without hiding the Intel
gap.

Median gfxlib3 producer times were 0.071246 seconds on OpenGL, 0.067300 seconds
on RTX Vulkan, and 0.054438 seconds on Intel Vulkan. The corresponding ordered
GPU completion portions were about 0.028217, 0.029294, and 0.081549 seconds.
The remaining Intel cost is therefore GPU execution and completion rather than
source upload or BASIC-side command production.

The current ARM64 archive was also run on the physical AGM A8 with its Adreno
306 OpenGL ES 3.0 renderer. Five alternating runs produced these medians:

| Android route | 307,200 blits | Approximate blits/second | Relative throughput |
| --- | ---: | ---: | ---: |
| gfxlib2, ordinary `FB.IMAGE` | 1.823304 s | 0.1685 million | baseline |
| gfxlib3, OpenGL ES, ordinary `FB.IMAGE` | 1.477863 s | 0.2079 million | 1.23 times gfxlib2 |

All ten alternating physical-device runs returned pixel `3784439` and exited
cleanly. The retained gfxlib3 GLES path is 23.4 percent faster by completed-work
throughput. Its median producer time was 0.360330 seconds and its ordered GPU
completion portion was about 1.117533 seconds. Depth-specific transparent
fragment shaders account for the change: they operate on existing texture byte
lanes and omit the general PUT mode branch and unpack/repack sequence.

The public compatibility path no longer acquires a second Win32 semaphore for
each sprite while `FB_GRAPHICS_LOCK` already serializes the producer. The
general context entry points remain independently mutex-protected. Vulkan also
retains command, tile, and winner buffers in fenced submission slots rather
than allocating Vulkan memory for each packet. NVIDIA uses a shader-generated
winner image and one resolve; Intel uses compact depth-specific TRANS tile
records. The 8,192-record limit is retained because a measured 16,384-record
experiment caused severe tail latency on both adapters.

The separate 30-frame extension-only upload-once fixture keeps the same source
pattern in device memory. Its direct GPU-surface-to-GPU-surface medians were
0.0101348 seconds on OpenGL, 0.0099030 on RTX Vulkan, and 0.0291048 on Intel
Vulkan. This is the intended asset-resident gfxlib3 route, but it is not
labelled a strict API-for-API comparison because gfxlib2 has no opaque
GPU-surface object. Intel Vulkan remains slower in both forms and is still an
optimization target.

## Partially clipped sprite qualification, 2026-07-22

`put-clipping-benchmark.bas` submits 307,200 patterned RGB565 `PUT TRANS`
operations using a 64 by 64 source. Every command intersects an edge or corner;
the visible portion is one row, one column, or one pixel. It uses a non-visible
work page, performs no page copy or presentation, and ends with one ordered
POINT. This isolates clipping, command production, and completed rendering.
The gfxlib2, OpenGL, and Vulkan variants are part of
`run-performance-matrix.ps1` so later renderer changes cannot omit this path.

The desktop comparison uses gfxlib2 DirectX explicitly because the normal
gfxlib2 Direct2D choice was bimodal on this host. Repeated Direct2D samples
ranged from about 0.036 to 0.59 seconds even with a non-visible work page.
Seven-run medians for the stable routes were:

| Backend | Completed time | Approximate blits/second | Relative to gfxlib2 |
| --- | ---: | ---: | ---: |
| gfxlib2, DirectX | 0.072892 s | 4.214 million | baseline |
| gfxlib3, OpenGL | 0.061444 s | 5.000 million | 1.19 times gfxlib2 |
| gfxlib3, Vulkan, NVIDIA RTX 2060 | 0.072589 s | 4.232 million | 1.00 times gfxlib2 |
| gfxlib3, Vulkan, Intel UHD 630 | 0.134449 s | 2.285 million | 0.54 times gfxlib2 |

All routes returned pixel `3784439`. OpenGL is 18.6 percent faster than the
stable gfxlib2 reference, and RTX Vulkan is effectively tied while retaining
asynchronous command submission. Intel remains 45.8 percent slower by
completed throughput. Selecting the sparse linear winner shader reduced its
median from 0.158077 to 0.134449 seconds, about 17.6 percent more throughput,
without changing the normal 13 by 16 sprite path.

An OpenGL hardware-scissor experiment was also rejected. Seven alternating
samples put shader clipping at a 0.090266-second median and per-run scissor
state changes at 0.102327 seconds. The retained path sends untrimmed geometry
and lets framebuffer and fragment stages clip it on the GPU.

The physical Android 7 AGM A8 produced these five-run medians with the Adreno
306 OpenGL ES 3.0 renderer:

| Android backend | Completed time | Producer time | Approximate blits/second |
| --- | ---: | ---: | ---: |
| gfxlib2 | 0.687879 s | 0.687876 s | 0.4466 million |
| gfxlib3 GLES | 0.894711 s | 0.374467 s | 0.3434 million |

The gfxlib3 BASIC-side producer is 1.84 times faster, confirming that partial
pixel clipping no longer consumes the application thread. The ordered GLES
drain adds about 0.520 seconds, so this deliberately sparse edge workload is
23.1 percent slower by completed throughput. That is an open mobile GPU cost,
not a claimed win. The ordinary fully visible Android sprite fixture remains
fast: the exact final archive measured 1.522110 seconds in its last physical
confirmation, while the preceding five-run median was 1.472110 seconds versus
the unchanged gfxlib2 reference of 1.823304 seconds.

## Immediate packet handoff and measured offload, 2026-07-22

The earlier figures above measured completed packets that were still retained
by the common context until POINT. A full backend-sized `BLITS` packet is now
submitted to the renderer immediately. GLES also performs a nonblocking
`glFlush` after its asynchronous fence so mobile drivers begin the command
stream without waiting for `GL_SYNC_FLUSH_COMMANDS_BIT` at readback.

`sprite-offload-benchmark.bas` draws 300 frames of 1,024 patterned 13 by 16
RGB565 sprites into a non-visible GPU page. There is no scaling, rotation,
projective transform, page copy, or presentation. One final POINT verifies
pixel `3784439`. Nine desktop rounds rotated the four backends through every
run position. The columns below are independent medians, so rounded submission
and completion values need not add exactly to the completed median.

| Desktop backend | Completed time | BASIC submission | Blits/second | Throughput versus gfxlib2 |
| --- | ---: | ---: | ---: | ---: |
| gfxlib2 DirectX | 0.097461 s | 0.097461 s | 3.152 million | reference |
| gfxlib3 OpenGL | 0.057167 s | 0.054964 s | 5.374 million | 70.5% faster |
| gfxlib3 Vulkan, RTX 2060 | 0.062524 s | 0.060334 s | 4.913 million | 55.9% faster |
| gfxlib3 Vulkan, Intel UHD 630 | 0.089560 s | 0.056832 s | 3.430 million | 8.8% faster |

The benchmark's second phase performs 0.250 seconds of deterministic integer
work before POINT. Median residual completion waits were 0.001897 seconds for
OpenGL, 0.001205 for RTX Vulkan, and 0.002076 for Intel Vulkan. The application
completed 95 to 99 percent as many integer iterations as the gfxlib2 phase.
The CPU therefore remains available for game logic while the desktop GPU drains
the sprite stream.

Seven alternating physical-device rounds on the AGM A8 produced:

| Android backend | Completed time | BASIC submission | Completion after submission | Blits/second |
| --- | ---: | ---: | ---: | ---: |
| gfxlib2 | 1.833228 s | 1.833225 s | 0.000003 s | 0.1676 million |
| gfxlib3 GLES | 1.103041 s | 0.379419 s | 0.721045 s | 0.2785 million |

gfxlib3 GLES is 66.2 percent faster by completed throughput and returns the
BASIC thread from the draw stream 4.83 times sooner. During the 0.250-second
CPU phase, its residual wait fell to 0.468407 seconds, 0.252638 seconds below
the baseline drain. The CPU loop completed 11,988,992 iterations versus
12,034,048 for gfxlib2, retaining 99.63 percent of the reference work rate.
This is measured concurrent GPU progress, not just deferred accounting.

The same submission fix changes the clipped mobile result recorded above.
Five fresh gfxlib3 samples produced a 0.543765-second median, comprising
0.387073 seconds of producer time and 0.157068 seconds of completion. That is
approximately 0.5650 million clipped blits per second and 26.5 percent more
throughput than gfxlib2's 0.687856-second reference. The renderer now wins both
the ordinary and deliberately edge-clipped mobile sprite workloads.

## Control-plane offload and compatibility-state cost, 2026-07-22

GPU offload is defeated if an unrelated input query waits for the graphics
timeline. The earlier implementation used a full context barrier for
SCREENEVENT, SETMOUSE, and touch reads. A tight query loop therefore waited for
prior rendering even though its result lived in a mutex-protected CPU input
snapshot.

The control-plane split is explicit:

- MULTIKEY, GETMOUSE, touch, and controller queries read the published input
  snapshot and never synchronize the GPU.
- SETMOUSE publishes its requested state immediately. The periodic platform
  pump applies the native cursor request.
- SCREENEVENT on every platform only reads or peeks the synchronized event
  ring. Desktop native messages are already published by the independent
  renderer-owned window pump, matching gfxlib2's queue-read contract.
- The idle producer considers a platform pump every 10 ms. An activity
  generation suppresses it when ordinary renderer work already pumped the
  window during that interval.
- Win32 caches successful joystick capabilities and rechecks a missing legacy
  device once per second. XInput snapshots are refreshed at most once per 8 ms
  pump interval instead of querying four absent slots for every event read.

The final focused desktop fixture performs 4,096 empty SCREENEVENT calls.
gfxlib2's five-run median is 0.00906 seconds. gfxlib3 OpenGL measures 0.00895
seconds, with Vulkan commonly between 0.0087 and 0.012 seconds. The earlier
renderer-round-trip form required about 0.328 seconds for the same gfxlib3
loop. Empty event polling is therefore at parity with gfxlib2 instead of
merely avoiding a GPU barrier. OpenGL also avoids `GLsync` creation for
CPU-only platform, title, and input-control batches; steady OMA profiling
reduced their backend execution cost from roughly 958 ms to 35 ms in the
focused interval. Exact Win32 input injection still passes on both GPU APIs in
normal and multithreaded runtime builds.

On the physical Adreno 306 device, 256 SCREENEVENT calls fell from 0.011402 to
0.000129 seconds after removing the empty renderer round trip. The same run
measured 0.000217 seconds for 256 GETMOUSE calls and about 0.00019 to 0.00023
seconds for each touch-query family, while gfxlib2 required 0.17864 seconds for
GETMOUSE and 0.08692 to 0.15376 seconds for touch queries. The current Android
key-injection smoke passed, so the faster snapshot route still receives native
events.

WINDOW, PMAP, and POINTCOORD are caller-local compatibility state, not renderer
work. Their public entry points already hold FreeBASIC's graphics lock, so an
additional mode mutex provided no lifetime protection. Removing that redundant
lock changed the focused OpenGL timings from 0.002234 to 0.000079 seconds for
1,024 WINDOW changes, 0.003214 to 0.000048 for the PMAP section, and 0.030636
to 0.000300 for 16,384 POINTCOORD reads. RTX Vulkan measured 0.000029,
0.000026, and 0.000171 seconds respectively. The computed sums and final pixel
remain identical to gfxlib2.

Finally, PAGE_SET and a BLIT into the visible page already mark presentation
dirty. The backend presents the final dirty surface at the ordered end of its
drain, so the compatibility layer no longer allocates a redundant PRESENT
packet for SCREENSET or SCREENCOPY. Page-flip correctness passes on desktop
OpenGL, both Vulkan implementations, and physical Android GLES. The following
page-copy pass removes the remaining per-copy submission and presentation cost
rather than treating the duplicate-packet reduction as the completed result.

## GPU page-copy batching and frame-boundary offload, 2026-07-22

`screen-state-benchmark.bas` now reports two independent values for both page
copy and FLIP:

- submission time ends after the BASIC thread has handed off every request;
- completed time includes a final `SCREENSYNC` and therefore all ordered GPU
  work and required presentation.

The true-colour test has three pages. Pages zero and one contain different
immutable colours, and each operation alternates one of them into visible page
two. This prevents an implementation from winning by repeatedly copying a
page onto itself or by assuming both sources contain the same data.

A SCREENSET that changes the visible page, and every SCREENCOPY or FLIP, now
hands its staged packet to the render thread without waiting. A page-only
renderer drain defers the native swap until an explicit barrier, the periodic
platform poll, or a later non-page command. This keeps frame production
asynchronous without leaving a final visible page unpresented.

Desktop OpenGL uses `glCopyImageSubData` for complete page copies. Vulkan
records complete PSET page copies as `vkCmdCopyBuffer` operations in one command
buffer, with one batch-level memory dependency rather than four barriers per
copy. Both backends track exact page-content tokens within the ordered batch,
skip copies whose source and destination are already identical, and discard a
copy only when a later copy overwrites its result before any retained operation
can observe it. Partial VIEW copies keep the general shader path.

Seven-run medians for 256 alternating requests were:

| Backend and adapter | Page submission | Page completed | FLIP submission | FLIP completed |
| --- | ---: | ---: | ---: | ---: |
| gfxlib3 OpenGL | 0.004351 s | 0.021491 s | 0.003590 s | 0.012420 s |
| gfxlib3 Vulkan, RTX 2060 | 0.004390 s | 0.009391 s | 0.004453 s | 0.008616 s |
| gfxlib3 Vulkan, Intel UHD 630 | 0.003228 s | 0.012484 s | 0.003405 s | 0.008330 s |

The stable gfxlib2 FLIP reference measured 0.005471 seconds for submission and
0.015603 seconds through completion. Its page-copy samples were compositor
bimodal, ranging from 0.011865 to a 0.360212-second median for submission, so
they are retained as raw comparison data rather than used for a precise ratio.
All routes returned final pixel `4283458815`.

The final one-run 62-variant coverage sweep also retained the same pixel for
gfxlib2, OpenGL, and Vulkan. In that run OpenGL submitted the distinct page
stream in 0.003624 seconds and completed it in 0.022107 seconds; Vulkan used
0.006624 and 0.009240 seconds. gfxlib2 used 0.388745 and 0.409078 seconds in
the compositor-heavy sample. FLIP completed in 0.009813 seconds on OpenGL and
0.019905 on Vulkan, versus 0.032216 seconds for gfxlib2.

The physical API 24 AGM A8 submitted 64 page copies in 0.000742 seconds and 64
flips in 0.000752 seconds. Their explicit completion boundaries took 0.107299
and 0.103285 seconds respectively on the Adreno 306. This is the intended
offload result: the BASIC producer returns in about twelve microseconds per
request, while the older mobile GPU and compositor finish the batch
asynchronously. The partial-VIEW page smoke also exited zero on that device.

## Multi-dispatch PAINT completion and offload, 2026-07-22

The isolated PAINT fixture now reports application-thread submission before
its ordered `POINT`, then reports the same repeated section through that
completion boundary. On desktop it performs one completed 1024 by 768 fill,
then fifteen adjacent solid recolours of the same border-enclosed region.

Desktop OpenGL and Vulkan now execute normal screen PAINT through four ordered
compute phases:

1. one workgroup discovers the seed-aligned candidate bounds;
2. 16 by 16 OpenGL or 16 by 8 Vulkan workgroups verify every interior pixel
   and all four adjacent perimeters;
3. the same grid writes independent pixels only if the atomic validity flag
   survived the complete verification dispatch;
4. one workgroup runs the exact scanline algorithm only when verification
   rejected the rectangle.

Storage and image barriers separate all four phases. The common 1024 by 768
case therefore launches thousands of spatially coherent workgroups without
allowing a target write to race topology verification. Irregular, patterned,
native-depth, alpha, and long-serpentine tests still exercise the exact fallback
and pass on both desktop APIs and both physical adapters.

The renderer also combines an adjacent run of opaque solid recolours only when
target, seed, clip, border, and topology are identical and every colour differs
from the border. Such recolours preserve the `pixel != border` reachable set,
and no public command observes an intermediate colour. Pattern, alpha,
border-coloured, or otherwise incompatible operations remain explicit command
barriers. `paint-coalescing-smoke.bas` checks both the permitted reduction and
the border-coloured counterexample on screen and GPU-only surfaces.

The following are five- or seven-run medians. The gfxlib2 row is the stable
seven-run reference recorded before the final shader pass; the final matrix
below provides a fresh same-run check. All durations are seconds.

| Backend and adapter | First fill complete | 15 recolours submit | 15 recolours complete |
| --- | ---: | ---: | ---: |
| gfxlib2 | 0.005830 | 0.080254 | 0.080255 |
| gfxlib3 OpenGL, RTX 2060 | 0.013476 | 0.000216 | 0.003900 |
| gfxlib3 Vulkan, RTX 2060 | 0.004438 | 0.000368 | 0.002114 |
| gfxlib3 Vulkan, Intel UHD 630 | 0.015431 | 0.000205 | 0.006699 |
| gfxlib3 OpenGL, Intel UHD 630 | 0.014326 | 0.000195 | 0.009045 |

Against that conservative gfxlib2 reference, the repeated completed section is
20.6 times faster on default OpenGL, 38.0 times faster on RTX Vulkan, 12.0
times faster on Intel Vulkan, and 8.9 times faster on Intel OpenGL. BASIC-thread
submission is 218 to 411 times shorter. These repeated ratios include both the
faster parallel final fill and legal removal of fourteen unobservable
intermediate recolours. They must not be presented as the throughput of fifteen
independently observable fills.

The single-fill result supplies that second view. RTX Vulkan is 1.31 times
faster than the stable gfxlib2 first-fill reference. OpenGL and both Intel
routes remain 2.3 to 2.6 times slower for a cold isolated fill, so first-use
dispatch and synchronization remain optimization work. Performance offload and
completed performance are both real for the repeated workload, but the single
operation caveat remains explicit.

The final one-run integration matrix built and ran all 62 registered variants
and emitted 251 raw timing records. It measured one gfxlib2 fill at 0.007044
seconds, OpenGL at 0.010260, and RTX Vulkan at 0.004693. The repeated section
was 0.867682, 0.006275, and 0.002450 seconds respectively. The gfxlib2 repeated
sample suffered the same intermittent host-scheduling spike seen in raw runs,
so it is integration evidence rather than the ratio baseline. Every route
returned first pixel `4280226057`, final pixel `4291850288`, and border pixel
`4278190335`.

Android has a different policy. Normal transferable GLES screen pages retain
the exact cached CPU shadow and one deferred dirty upload because ES 3.0 cannot
perform a compute frontier and a long fragment frontier is pathological.
GPU-only GLES surfaces still use the renderer's mask ping-pong path and the
same exact adjacent-command reduction. On the physical AGM A8, five-run paired
medians for three screen recolours were:

| Android backend | First fill complete | Recolours submit | Recolours complete |
| --- | ---: | ---: | ---: |
| gfxlib2 | 0.066923 | 0.146676 | 0.146703 |
| gfxlib3 GLES | 0.094622 | 0.016666 | 0.045599 |

This is an 8.8 times application-thread win and a 3.2 times completed-work win
for the repeated screen workload, while the current first-fill median remains
1.41 times slower. The final benchmark and GPU-only coalescing APKs both
reported `FREEBASIC_ANDROID_EXIT:0`. This result is a GLES hybrid-path claim,
not a claim that normal Android screen PAINT performs its flood math in a
compute shader.

## Adaptive POINT/PSET compatibility offload, 2026-07-22

The complete OMA device pass found a synchronization workload that the normal
primitive benchmarks did not represent. Demolition Derby and Duel 999 perform
software-style destination blending by reading a screen pixel with POINT and
then writing the same coordinate with PSET. A literal GPU implementation of
that dependency must complete a read before the CPU can calculate each write.
The resulting command stream alternated thousands of `READ_PIXEL` and
`POINTS` packets, leaving both the CPU and GPU mostly occupied with hand-off
latency rather than useful rendering.

gfxlib3 now treats this as an explicit CPU ownership phase only after it has
observed two exact POINT/PSET pairs at matching coordinates. Promotion performs
one ordered page download. Further dependent reads and writes use the coherent
shadow, and the next GPU ordering boundary performs one accumulated upload.
The path supports 32-bit and RGB565 pages. Legacy depth 15 modes use the same
RGB565 packing and POINT expansion rules.

This does not move normal rendering back to the CPU. Isolated POINT remains an
ordered one-pixel result query. Ordinary PSET and every shader primitive remain
queued GPU work. A following GPU command first commits a dirty adaptive shadow
so command order remains exact. The optimization is limited to code that has
already made the colour calculation CPU-dependent by calling POINT.

Observed cold-start bounds on the physical API 24 AGM A8 were:

| Program | Before adaptive shadow | Final build |
| --- | --- | --- |
| Demolition Derby | Black at 15 seconds; first useful frame around 30 to 45 seconds | Setup rendered by 5 seconds |
| Duel 999 | Black for minutes while trace emitted repeated read/write packets | Full RGB565 setup rendered by 8 seconds |

The device spends approximately four seconds in GLES and application startup.
These values therefore measure practical game startup, not isolated primitive
throughput, and should not be used as a gfxlib2 ratio. They establish that the
fix removes a real synchronization cliff while retaining GPU primitives for
the rest of each game.

## Stable-image sprite and whole-game checkpoint, 2026-07-27

The current producer keeps a caller-thread draw-state cache and a verified
direct FB.IMAGE cache lookup in front of the exact LRU. Its packed sprite
stream reaches the renderer as one command record per PUT but is submitted to
the GPU in backend-sized packets of as many as 8,192 records. Clipping and
transparent-key decisions remain shader work.

`mixed-sprite-benchmark.bas` creates 32 non-uniform RGB565 images, warms their
GPU residency, performs 61,441 unscaled `PUT TRANS` operations, then uses one
ordered POINT as its completion and correctness boundary. Five-run same-session
medians on the Windows reference machine were:

| Backend and adapter | Completed seconds | Sprites/second | Versus gfxlib2 |
| --- | ---: | ---: | ---: |
| gfxlib2 | 0.116357 | 528,037 | 1.00x |
| gfxlib3 OpenGL, RTX 2060 | 0.026618 | 2,308,241 | 4.37x |
| gfxlib3 Vulkan, RTX 2060 | 0.023567 | 2,607,089 | 4.94x |
| gfxlib3 Vulkan, Intel UHD 630 | 0.052654 | 1,166,875 | 2.21x |

Every route returned final RGB565 pixel `12416487`. The adapter override in
these runs was `FBGFX3_VULKAN_DEVICE_INDEX`; an earlier diagnostic used the
wrong variable name and silently measured the default RTX twice. Those
mislabeled Intel figures are superseded by the table above.

The unchanged Rambo vs Kitty Cat workload verifies a different property. Its
active frame limiter holds approximately 30 frames per second, with about
16,500 transparent sprites, 60 rectangle operations, and 30 presentations per
second. Automatic RTX Vulkan profiling reports roughly 108 to 122 ms of
backend execution per second and no steady GPU completion waits. Total process
CPU is scheduler- and scene-sensitive: repeated samples ranged from near
gfxlib2 parity to approximately 1.3 times the gfxlib2 median. This does not
establish a whole-game total-CPU win, and it must not be described as one. It
does establish that the unchanged game is no longer stalled by per-sprite GPU
round trips and that its renderer work proceeds asynchronously.

Automatic desktop selection now tries Vulkan first because it provides the
best combination of completed sprite throughput and steady renderer cost on
the reference hardware. OpenGL remains the fallback. Explicit flags and
`FBGFX` requests retain their existing force and priority behavior.

The same ARM64 archive ran on the physical API 24 AGM A8 with automatic GLES
fallback. The ordinary OMA sprite fixture completed 30,720 transparent PUTs
in 1.110213 seconds, reported only 0.000000521 seconds at its final completion
boundary, returned pixel `3784439`, and exited zero. The small completion
duration means the renderer consumed the sprite stream concurrently while the
BASIC producer filled later packets.

## Large mutable images and Vulkan submission overlap, 2026-07-27

Profiling the unchanged Rambo vs Kitty Cat game found a compatibility-cache
cost that the small sprite fixtures did not expose. Its expanded `image1.bmp`
is about 3 MiB. `IMAGEINFO` exposes a writable pixel pointer, and the previous
exact-snapshot limit was 1 MiB, so every stable PUT hashed the entire image.
`image_api_hash_source` accounted for 52.41 percent of the sampled process CPU
in that profile.

Mutable images up to 16 MiB now use an exact byte snapshot. The cache-wide
budget is 64 MiB on desktop and 24 MiB on Android. Replacement allocation
succeeds before an old snapshot is released, and cache accounting is checked
for overflow and updated on every retirement. Images outside the budget retain
the exact hash path. This does not introduce an immutable-image assumption:
the new `large-image-cache-smoke.bas` obtains the writable IMAGEINFO pointer,
reuses a stable 1024 by 768 by 32-bit image, edits its first pixel directly,
and requires the next PUT and POINT to observe the edit. It passes OpenGL,
Vulkan on both desktop adapters, and physical Android GLES. The AGM A8
completed 32 transfers in 0.171826 seconds and exited zero.

The Vulkan runtime now records at most three adjacent runtime operations into
one command-buffer submission. Six submission-resource slots allow two such
groups to overlap, avoiding the former three-slot wraparound wait. Sequence
tags, descriptors, and deferred allocations remain operation-local, while the
participating slots share the owner fence. Targeted waits flush a partially
recorded group and retire only the required sequence. Presentation, resize,
idle, and shutdown also flush before observing completion.

The shutdown INFO counter distinguishes actual driver submissions from runtime
operations. A representative unchanged Rambo run used 459 `vkQueueSubmit`
calls for 1,164 completed runtime operations. Disabling the batch through the
diagnostic control used 1,534 submissions for 1,534 operations. A second
shorter sample measured 317 for 795 versus 1,126 for 1,126. Isolated
microbenchmarks varied with host scheduling, so the default is justified by
the whole-game CPU and submission evidence, not by claiming that every small
packet is faster in isolation.

Two clean 15-second paired Rambo runs, with the source and frame limiter
unchanged, measured:

| Runtime | Total process CPU samples | Mean CPU seconds | Main/game-thread mean |
| --- | --- | ---: | ---: |
| gfxlib2 | 3.125000, 3.203125 | 3.164063 | 2.117188 |
| gfxlib3 automatic RTX Vulkan | 2.921875, 2.906250 | 2.914063 | 1.367188 |

This is approximately 7.9 percent less total process CPU and 35.4 percent less
main/game-thread CPU for gfxlib3. The latter is the intended offload result:
the BASIC/game thread spends materially less time preparing and synchronizing
graphics while the render thread and GPU driver consume the command stream.
No game source was changed.

The longer plain-sprite fixture performs 614,401 completed, unscaled,
unrotated transparent PUTs and checks the same final RGB565 pixel
`12416487`. Its current medians are:

| Backend and adapter | Sprites/second | Versus gfxlib2 |
| --- | ---: | ---: |
| gfxlib2 | about 547,700 | 1.00x |
| gfxlib3 OpenGL, RTX 2060 | about 3,379,000 | 6.17x |
| gfxlib3 Vulkan, RTX 2060 | about 3,238,000 | 5.91x |
| gfxlib3 Vulkan, Intel UHD 630 | about 1,359,600 | 2.48x |

These results supersede the shorter 61,441-sprite table above. Clipping,
transparent-key testing, and destination writes remain shader work. The CPU
constructs bounded sprite metadata and does not clip or rasterize sprite
pixels.

The physical AGM A8 comparison used the same 30-frame OMA-style stream in
gfxlib2 and gfxlib3, including 30,720 transparent sprites, page copies, and a
per-frame POINT correctness boundary. gfxlib3 GLES took 0.843750 seconds;
gfxlib2 took 1.217575 seconds. Both returned pixel `3784439` and exited zero.
That is about 1.44 times the throughput, or 30.7 percent less elapsed time, on
the connected Android hardware.

## OMA-driven cache and locked-pixel optimization, 2026-07-27

Profiling the unchanged OMA programs exposed costs which the synthetic
primitive tests did not represent. The renderer profile is enabled only by
`FBGFX3_PROFILE=1` and reports command density, backend execution time,
completion waits, transfer volume, and GPU resource traffic once per second.
No OMA source was changed while finding or fixing these library bottlenecks.

Rambo vs. Kitty Cat alternates many persistent CPU `FB.IMAGE` sprites. The
image-cache replacement scan chose its least-recently-used candidate before it
finished looking for an unused slot, so a nominal 128-entry cache behaved like
a one-entry cache. The game created, uploaded, and destroyed roughly 500
textures per second and presented only one or two frames per second. The cache
now prefers an unused entry, preserves same-sized GPU allocations when pixels
change, and places ordinary small sprites in persistent atlas cells. After the
warm-up uploads, the unchanged game performs no steady-state texture creation,
destruction, or upload. It presents 28 to 29 frames per second, matching its
source-level 30 Hz clock, and submits about 15,400 cached sprite blits per
second.

Kinematics uses `SCREENLOCK` as a frame grouping operation but never requests
`SCREENPTR`. A lock formerly downloaded the complete page even though no CPU
pointer could observe it. `SCREENLOCK` now defers shadow creation until
`SCREENPTR` or an actual locked POINT/PSET dependency appears. The unchanged
game went from approximately 31.3 MiB of readback per second and 60 completion
boundaries per second to zero readback and one normal frame completion at its
intended 30 Hz rate.

Demolition Derby begins its setup frame with a full `CLS`, then implements a
custom antialiased font as POINT, a BASIC colour calculation, and PSET under
`SCREENLOCK`. A full clear now establishes the known clear colour directly in
an existing or locked CPU shadow. It discards superseded dirty data and avoids
downloading pixels which the clear has already replaced. POINT and PSET also
use the public graphics lock they already own instead of taking a second mode
mutex for every pixel. The unchanged setup screen changed from five or six
presents per second to 32 to 40 presents per second, with zero steady-state
readback and zero GPU completion waits.

`locked-point-pset-benchmark.bas` reproduces that compatibility pattern without
game-specific code. Each route performs 393,216 dependent pairs across 48
cleared frames and returns final pixel `4290855361`. Five-run completed medians
were:

| Backend | Seconds | Pairs per second | Relative to gfxlib2 |
| --- | ---: | ---: | ---: |
| gfxlib2 | 0.577231 | 681,214 | 1.00 |
| OpenGL, RTX 2060 | 0.126480 | 3,108,927 | 4.56 |
| Vulkan device 0, RTX 2060 | 0.118154 | 3,328,844 | 4.89 |
| Vulkan device 1, Intel UHD 630 | 0.152001 | 2,586,929 | 3.80 |

This is a compatibility result, not a claim that the BASIC colour calculation
runs in a shader. Once the application calls POINT, the destination value must
return to the CPU before BASIC can calculate its PSET result. gfxlib3 minimizes
that unavoidable CPU-owned interval and transfers the accumulated page once.
Applications which express the same work as PUT ALPHA, GPU-surface operations,
or gfxlib3 primitives keep the pixels on the device and execute the blend or
primitive math in shaders.

Repeated empty SCREENEVENT calls remain visible in some older OMA input
wrappers. A timed poll-coalescing experiment reduced those commands but failed
the immediate posted-message compatibility test and was removed. The retained
optimization skips the native-thread round trip only when an event is already
published in gfxlib3's queue. Correct immediate polling takes precedence over
hiding inefficient application input structure.

## Owned-image generations and mobile sheet residency, 2026-07-28

The normal stable-image path no longer copies or hashes pixels when gfxlib3
owns the new-format image and IMAGEINFO has not exposed its writable pointer.
Compatibility writers, BLOAD, GET, and image primitives advance a private
generation in the backend-reserved header fields. The cache compares that
generation before reusing the GPU allocation. Externally writable images still
use exact snapshot validation, so this optimization does not assume that all
FB.IMAGE memory is immutable.

This change also fixes oversized shallow sprite sheets on GPUs with smaller
texture limits. The cache stores the requested source rectangle in a GPU atlas
when the complete image cannot be allocated. QFAK's 10,600 by 40 and 10,960 by
40 strips now remain usable on the Adreno 306 as resident 40 by 40 regions.
After warm-up, its active renderer interval performs about 18,000 GPU sprite
blits and 65 presentations per second with zero upload, download, READ_PIXEL,
or surface-download commands.

A fresh unchanged-source Win64 OpenGL run produced the following process
samples. These numbers describe the active scenes selected by the harness and
are not paired gfxlib2 speed ratios:

| Game | Total CPU | Main/game thread |
| --- | ---: | ---: |
| Arkanoid | 23.6% | 7.8% |
| Behold | 96.4% | 87.1% |
| Demolition Derby | 66.8% | 45.5% |
| Duel 999 | 137.2% | 92.8% |
| Kinematics | 34.9% | 23.3% |
| Nietzsche Special Edition | 51.2% | 31.9% |
| Quest for a King | 44.1% | 24.1% |
| Rambo vs Kitty Cat | 26.2% | 14.3% |
| Star Phalanx | 12.5% | 1.0% |
| Open Market | 11.4% | 1.6% |

Every active final renderer interval reported zero MiB uploaded and downloaded,
zero READ_PIXEL commands, and zero surface downloads. The same intervals
processed approximately 194,000 blits per second in Duel 999, 18,000 in QFAK,
13,200 in Rambo, and 3,200 in Star Phalanx. This is the useful offload result:
the BASIC thread emits bounded metadata while the GPU performs the per-pixel
sprite work. Raw process CPU remains influenced by each game's simulation,
input polling, frame limiter, and chosen scene.

The focused wide-image test passes on Win64 and Win32 OpenGL, Vulkan device 0
on the RTX 2060, Vulkan device 1 on the Intel UHD 630, and physical Android
OpenGL ES on the AGM A8. It also obtains an IMAGEINFO pointer, modifies one
cached region directly, and proves that the next PUT refreshes the GPU copy.

## Four-game OMA addition, 2026-07-28

The newly added playable projects are OpenHostility, TurboTrek, vtrek, and
OpenWallStreet. The fifth new directory, `OMA/Scorched`, is Dolphin
configuration, cache, and user data used by OpenHostility's Wii validation; it
does not contain another FreeBASIC game entry point.

All CPU figures below are bounded live-scene samples, not fixed-work
microbenchmarks. A game without a frame limiter performs more simulation and
rendering when the library becomes faster, so lower process CPU is not the
only useful result. The main-thread column is the closer measure of graphics
offloading, while the renderer thread and driver work remain part of total
process CPU.

Fresh final-source Win64 OpenGL samples were:

| Game | gfxlib2 total | gfxlib2 main | gfxlib3 total | gfxlib3 main |
| --- | ---: | ---: | ---: | ---: |
| OpenHostility | 7.4 to 12.2% | 4.5 to 8.2% | 20.5 to 20.6% | 7.2 to 8.2% |
| TurboTrek | 27.3% | 21.8% | 38.0% | 17.5% |
| vtrek | 43.8% | 42.7% | 41.7% | 33.3% |
| OpenWallStreet | 63.2% | 61.3% | 36.4% | 22.4% |

TurboTrek therefore moved about 20 percent of its sampled game-thread work
off the producer, vtrek about 22 percent, and OpenWallStreet about 63 percent.
OpenHostility's main-thread result overlaps the gfxlib2 range and its total
process cost is higher. There is no basis for claiming that gfxlib3 is already
faster for every complete game.

TurboTrek is an especially useful throughput example. Its final gfxlib3
renderer intervals contain roughly 56 to 65 presentations, 0.8 to 0.9 million
antialiased point samples, 2,300 to 2,700 lines, and 4,800 to 5,600 rectangles
per second, with zero upload, download, or GPU completion waits in steady
state. The earlier single-page build consumed less work only because
`ScreenCopy` was a no-op and the compositor could see an incomplete frame.
After the game requested two pages, the final Android menu remained
byte-identical across repeated one-second captures.

Profiling that real two-page path exposed a library batching boundary.
`Gfx3DrawPoints` used to flush after every string. Retaining adjacent arrays in
the compatibility point stream reduced a representative Win64 sample from
50.9 percent total and 25.6 percent main-thread CPU to 41.7 and 18.7 percent.
Point packets fell by about 17 percent in the steady renderer intervals while
the presented frame rate stayed in the same range. The shader still performs
clipping and alpha math; the CPU change removes packet preparation and
dispatch overhead.

The fresh current-library Win32 OpenGL matrix also built and entered all four
live scenes:

| Game | gfxlib2 total | gfxlib2 main | gfxlib3 total | gfxlib3 main |
| --- | ---: | ---: | ---: | ---: |
| OpenHostility | 5.4% | 3.7% | 18.3% | 5.8% |
| TurboTrek | 24.5% | 18.5% | 43.0% | 23.3% |
| vtrek | 43.5% | 41.8% | 54.0% | 45.0% |
| OpenWallStreet | 65.0% | 63.0% | 41.9% | 24.7% |

These one-sample Win32 figures are deliberately recorded rather than hidden:
OpenWallStreet benefits substantially, but the other three do not yet show a
whole-process win in that run. gfxlib3's focused sprite benchmarks and several
Win64 game-thread results prove useful GPU throughput and offloading, while
these applications identify remaining command production, presentation, and
CPU-image traffic to optimize.

This matrix was collected before the final OpenHostility null-guard-only
source correction. Fresh gfxlib2 and gfxlib3 Win32 executables were rebuilt
after that edit and both deterministic self-tests passed. The table is
retained because its live sample did not enter the corrected branch.

## 2026-07-28 OpenSlicks race workload

OpenSlicks exposed a workload which the synthetic primitive tests did not:
an immutable 320 by 200 track assembled once, then copied every frame, with
antialiased text and a translucent touch overlay drawn over it. The original
gfxlib3 build was slower than gfxlib2 because the track copy used thousands of
compatibility PSET calls and the overlay used POINT before every alpha write.
That path consumed about 106 to 108 percent total CPU and 84 to 86 percent on
the BASIC thread in the Win64 sample.

The optimized path makes the data ownership explicit:

- The track is uploaded once to a `GFX3_SURFACE_ASSET`.
- `Gfx3SurfaceBlit(NULL, ...)` copies that resident surface to the current work
  page without returning through system memory.
- Antialiased glyphs and the touch overlay are submitted as alpha point
  batches. Clipping, destination blending, and pixel stores run in shaders.
- A normal steady race performs no surface upload, screen download, completion,
  or CPU wait.

The Windows OpenGL comparison used the same live race, input sequence, and
measurement interval:

| Target | Runtime | Total CPU | BASIC/main CPU | Total reduction | Main reduction |
| --- | --- | ---: | ---: | ---: | ---: |
| Win64 | gfxlib2 | 63.9% | 62.1% | | |
| Win64 | gfxlib3 OpenGL | 35.4% | 16.3% | 44.6% | 73.8% |
| Win32, 3-sample average | gfxlib2 | 67.3% | 63.4% | | |
| Win32, 3-sample average | gfxlib3 OpenGL | 34.3% | 15.6% | 49.1% | 75.4% |

The Win32 gfxlib2 samples measured 69.4, 65.7, and 66.9 percent total CPU.
The corresponding gfxlib3 samples measured 36.4, 30.7, and 35.8 percent.
All six processes remained responsive and the captured race frames matched.
The deterministic suite passed 2,872 assertions for both libraries.

OpenGL and Intel Vulkan present about 125 frames per second in this uncapped
loop. The RTX 2060 Vulkan path presents about 274 frames per second, and
therefore uses more whole-process CPU than the slower gfxlib2 run while doing
more than twice as much rendering work. Its producer thread still falls to
36.9 percent CPU. The Intel Vulkan producer uses 16.2 percent. These numbers
must not be compared as equal-throughput whole-process samples until the game
has a presentation cap; they do prove that both adapters execute the complete
GPU path and that the BASIC thread is offloaded.

The AGM A8's Adreno 306 sustains about 44 to 48 presentations per second in
the same race. Before the overlay used GPU alpha batches, it managed about 9
to 10 presentations per second and transferred roughly 4 MiB to and 2.4 MiB
from the GPU each second. The final steady intervals have zero uploads,
downloads, completions, and waits after the one-time 0.244 MiB track upload.

The reusable profiler is `tests/gfx3/profile-openslicks.ps1`. It builds both
libraries, runs the deterministic suite, enters the same race, records total
and main-thread CPU, preserves renderer logs, and can capture the live window.

## Fixed-screen maximize presentation cost

Fixed-screen maximize does not make the BASIC thread redraw at the desktop
resolution. The logical page remains at its original size and all drawing
commands retain their original coordinate and pixel count.

OpenGL presents through an integer-scaled viewport. A GPU clear is added only
when bars are present. Vulkan folds the same scaling and black-bar decision
into the existing presentation compute dispatch. In both cases there is no
CPU scaling loop, replacement logical page, additional upload, or readback.
The native client-size change only marks presentation state dirty.

This makes maximize effectively a presentation policy rather than a new
rendering workload. It still pays the normal compositor and swapchain cost of
displaying a larger native window; no claim is made that the operating
system's composition itself is free.

## 2026-07-28 newJRPG workload

The production editor and complete QFAK graphical battle were profiled from
`C:\Nextcloud\games\newjrpg`. The editor exposed per-packet Vulkan buffer
allocation and full-surface rectangle dispatch as costs which the smaller
primitive fixtures did not make obvious.

Fence-owned persistent point and rectangle packet buffers, compact touched-tile
rectangle dispatch, and a mixed-primitive padding guard raised the live editor
from about 10 to 17-19 application frames per second on the RTX 2060 and from
about 12-13 to 17-19 on Intel UHD 630. The current Vulkan editor remains slower
than gfx3 OpenGL because it still submits many separate primitive commands.
Steady editor profiles report zero downloads.

The complete graphical battle measured 102.06 seconds on gfxlib2, 100.46 on
gfx3 OpenGL, 88.67 on RTX Vulkan, and 96.96 on Intel Vulkan. The Vulkan results
are 13.1 and 5.0 percent faster than gfxlib2 in this run. The complete test and
platform matrix are recorded in
[`newjrpg-qualification.md`](newjrpg-qualification.md).

## 2026-07-29 Vulkan mixed primitive tile coalescing

The next editor profile showed approximately 181 fine-grained primitive
commands per displayed page. Points and lines could already enter the compact
winner batch, but every intervening rectangle ended that packet. Sending all
rectangles through the atomic winner removed those breaks, but made broad
rectangle workloads perform an atomic operation per covered pixel and then pay
for a second resolve pass.

The final selector uses one ordered 16 by 16 tile replay dispatch for an
ellipse-free mixed point, line, and rectangle packet. Per-pixel line and
rectangle coverage remains shader math. The host only builds conservative tile
candidate lists, using persistent scratch storage and fence-owned mapped
buffers. Rectangle-free point, line, and ellipse packets retain the compact
useful-workgroup winner path.

Steady production-editor samples reported:

| Vulkan adapter | Atomic rectangle winner | Mixed tile replay |
| --- | ---: | ---: |
| NVIDIA GeForce RTX 2060 | about 14.0 ms execute/page | 8.25 ms execute/page |
| Intel UHD Graphics 630 | about 15.4 ms execute/page | 8.23 ms execute/page |

The corresponding complete page-set rates were approximately 34 to 37 per
second on the RTX and 26 to 30 per second on Intel. The profiles retained zero
steady screen downloads. This is a library-level improvement over the
immediately preceding mixed-winner build, using unchanged editor source.

The standard 6,000 filled-box section also retained its final pixel and
completed in a three-run median of 0.01719 seconds on RTX Vulkan and 0.01961
seconds on Intel Vulkan. The preceding documented RTX rectangle tile path was
0.02823 seconds. The same-run gfxlib2 memory-buffer loop remained faster for
this isolated small CPU workload at 0.00549 seconds. gfxlib3's gains are in
GPU offload and larger mixed command streams, not every microbenchmark.

The current QFAK graphical battle passed in 119.91 seconds, but it is not used
as evidence for this primitive change. Its revised source performs timed
engine updates, and its profiled expensive intervals contained thousands of
synchronous pixel reads per second with zero points and lines and at most a
few rectangles. A preserved older executable also varied from 88.91 to 105.99
seconds under different runs. QFAK remains an end-to-end correctness
qualification; the editor profile and primitive fixture measure the changed
path directly.

<!-- end of performance.md -->
