# gfxlib3 benchmark coverage

This document maps the renderer command protocol to the repeatable test that
measures it. A benchmark result is meaningful only after its final ordered
readback or synchronization boundary; every listed timing fixture supplies
one. Tests are run by `tests/gfx3/run-performance-matrix.ps1` unless noted.

| Renderer command | Public path | Throughput fixture | gfxlib2 reference |
| --- | --- | --- | --- |
| renderer init/shutdown, surface create/destroy | `SCREENRES`, GPU surface extension | `mode-open-benchmark.bas`, `gpu-surface-benchmark.bas` | `mode-open-benchmark.bas` where applicable |
| surface upload/download | `GET`, map, GPU-surface extension | `transfer-benchmark.bas`, `gpu-surface-benchmark.bas` | `transfer-benchmark.bas` for `GET` |
| read pixel, barrier | `POINT`, `SCREENSYNC` | all timing fixtures, `primitive-benchmark.bas`, `control-query-benchmark.bas` | same public fixture |
| clear | `LINE ..., BF` and page initialization | `primitive-benchmark.bas` | same public fixture |
| points | `PSET`, console pixels | `pset-benchmark.bas`, `primitive-benchmark.bas`, `console-benchmark.bas` | same public fixture |
| line, rectangle, ellipse | `LINE`, `BOX`, `CIRCLE` | `primitive-benchmark.bas`, `arc-benchmark.bas` | same public fixture |
| blit | all standard `PUT` modes, `SCREENCOPY` | `transfer-benchmark.bas`, `transfer-path-benchmark.bas`, `oma-sprite-benchmark.bas`, `gpu-sprite-benchmark.bas`, `screen-state-benchmark.bas` | same public fixture; gfxlib2 has no GPU-surface source for the extension-only fixture |
| blits (producer packet) | consecutive compatible non-self `PUT` operations | `transfer-path-benchmark.bas`, `transfer-benchmark.bas`, `oma-sprite-benchmark.bas`, `gpu-sprite-benchmark.bas` | `oma-sprite-benchmark.bas` is the like-for-like ordinary unscaled sprite comparison |
| transform blit | `Gfx3SurfaceBlitScaled`, `Gfx3SurfaceBlitRotated`, `Gfx3SurfaceMode7` | `gpu-transform-benchmark.bas` | none: gfxlib2 has no equivalent API or sampling contract |
| glyphs | `DRAW STRING`, console | `primitive-benchmark.bas`, `console-benchmark.bas` | same public fixture |
| palette, page set, present | `PALETTE`, `SCREENSET`, `SCREENCOPY`, `FLIP` | `palette-family-benchmark.bas`, `screen-state-benchmark.bas`, `gpu-surface-benchmark.bas` | same public fixture; screen state reports producer handoff and explicit completion separately |
| coordinate state | `VIEW`, `WINDOW`, `PMAP`, `POINTCOORD` | `coordinate-state-benchmark.bas` | same public fixture |
| paint | `PAINT` | `paint-benchmark.bas`, `primitive-benchmark.bas` | same public fixture; isolated test reports producer and explicit completion separately |
| input poll | `SCREENEVENT` | `control-query-benchmark.bas` plus platform input smokes | same public fixture |

## Public API performance coverage

The protocol table proves that each renderer operation has a workload. The
table below performs the complementary audit against the public gfxlib2 and QB
surface. It prevents a fast renderer subset from being mistaken for a fast
drop-in library.

| Public API family | Timed fixture | Correctness boundary |
| --- | --- | --- |
| `SCREEN`, `SCREENRES`, mode close and replacement | `mode-open-benchmark.bas` | mode lifecycle and renderer-selection smokes |
| `SCREENINFO`, `SCREENLIST`, `SCREENCONTROL`, `WINDOWTITLE`, `SCREENEVENT`, `SCREENGLPROC`, `SCREENSYNC` | `control-query-benchmark.bas` | control, input, interop, and mode smokes |
| `SCREENSET`, `SCREENCOPY`, `FLIP`, `SCREENLOCK`, `SCREENUNLOCK`, `SCREENPTR` | `screen-state-benchmark.bas` | page-flip presentation and lock-depth smokes |
| `PSET`, `PRESET`, `POINT` | `pset-benchmark.bas`, `primitive-benchmark.bas` | primitive and point-cache smokes |
| `LINE`, `LINE ... B`, `LINE ... BF`, `CIRCLE` and arcs | `primitive-benchmark.bas`, `arc-benchmark.bas` | backend pixel-reference smokes |
| `PAINT` | `paint-benchmark.bas` | pattern, alpha, depth, and large-surface smokes |
| `DRAW`, `DRAW STRING` | `draw-benchmark.bas`, `primitive-benchmark.bas`, `console-benchmark.bas` | command, custom-font, and console smokes |
| `VIEW`, `WINDOW`, `PMAP`, `POINTCOORD` | `coordinate-state-benchmark.bas` | stateful compatibility tests |
| `PALETTE`, `PALETTE GET`, `PALETTE USING`, `PALETTE GET USING` | `palette-family-benchmark.bas`, `screen-state-benchmark.bas` | indexed presentation and palette smokes |
| `IMAGECREATE`, `IMAGEDESTROY`, `IMAGEINFO` | `image-allocation-benchmark.bas` | unchanged image-expression suite |
| `GET`, built-in `PUT` modes, CPU-image cache, GPU-source `PUT` | `transfer-benchmark.bas`, `transfer-path-benchmark.bas`, `image-cache-benchmark.bas`, sprite benchmarks | image, depth-conversion, overlap, and clipping smokes |
| `BLOAD`, `BSAVE`, row conversion | `file-row-benchmark.bas` | BMP/raw/RLE/bitfield and format round trips |
| `MULTIKEY`, `GETMOUSE`, `SETMOUSE`, `GETJOYSTICK`, `GETXPAD`, `GETTOUCHCOUNT`, `GETTOUCH`, `GETTOUCHHIT` | `control-query-benchmark.bas` | Win32, X11, Android, fallback, and QB input smokes |
| graphics console hooks including `COLOR`, `CLS`, `WIDTH`, `LOCATE`, `PRINT`, `SCREEN(row,col)` | `console-benchmark.bas` | console, depth, font, scrolling, and line-input smokes |
| gfxlib3 GPU surfaces and scaling, rotation, Mode 7 | `gpu-surface-benchmark.bas`, `gpu-sprite-benchmark.bas`, `gpu-transform-benchmark.bas` | GPU-surface, map, asset, transform, and self-blit smokes |

Blocking `GETKEY`, `INKEY` waiting behavior, line input editing, physical input
latency, user `PUT CUSTOM` callbacks, and port I/O are correctness workloads,
not repeatable renderer-throughput measurements. They remain in the smoke and
unchanged test suites. This is an explicit exclusion, not missing API parity.

`WINDOW_TITLE`, `PLATFORM_POLL`, `INPUT_POLL`, and `INTEROP_CALLBACK` are
control-plane commands, not GPU rendering work. `WINDOWTITLE` and
`SCREENEVENT` now have public latency columns in `control-query-benchmark.bas`;
their ordering and exact behavior remain covered by the smoke suite rather
than being used to claim a shader-throughput win.
`PUT CUSTOM` deliberately remains a synchronized CPU callback and is excluded
from GPU speed comparisons. It is documented as a compatibility boundary, not
silently routed through an incorrect shader approximation.

`screen-state-benchmark.bas` uses three true-colour pages. Two immutable source
pages contain different colours and alternate into a third visible page. It
reports page-copy and FLIP producer time before SCREENSYNC, then reports the
same section through the explicit completion boundary. This guards both BASIC
thread offload and actual GPU completion. `page-flip-presentation-smoke.bas`
adds a partial VIEW copy so a full-page transfer optimization cannot silently
replace clipped BLIT semantics.

`transfer-path-benchmark.bas` is the focused 4,096-PSET companion to the
all-mode transfer fixture. Its command-count argument makes a renderer stall
reproducible at one, one batch, or a full queued workload without changing the
public PUT implementation.

`BLITS` is an internal bounded command packet rather than a new BASIC API.
The PUT fixtures above exercise it on desktop compute backends and GLES,
including the one-item flush boundary that occurs before the ordered `POINT`
readback. The public test workload and correctness pixel are unchanged, so its
gfxlib2 reference remains a valid like-for-like comparison.

`oma-sprite-benchmark.bas` uses a non-uniform 13 by 16 RGB565 image containing
both transparent-key and opaque pixels. Its isolated build omits page copy and
per-frame synchronization, then performs one ordered read from a pixel touched
by the sprite stream. The normal 30-frame run measures 30,720 ordinary,
unscaled `PUT TRANS` calls and completed shader work. Defining
`OMA_BENCHMARK_FRAME_COUNT` permits a longer run without changing the sprite
or command stream; the current qualification uses 300 frames and 307,200
blits. `gpu-sprite-benchmark.bas` repeats the same pattern after one upload to
an opaque gfxlib3 surface. It reports both the normal public `PUT` route and
direct GPU-surface-to-surface blit, but has no gfxlib2 GPU-surface column.

`pset-benchmark.bas` is the dense direct-point companion. It uses public PSET
and a final POINT on a screen surface, has no private renderer calls, and
appears in the matrix for gfxlib2, forced OpenGL, and forced Vulkan. It is the
regression guard for the queued GPU point path rather than CPU image PUT
handling. `GFX2_REFERENCE` suppresses its Android gfxlib3 selection so the
same source can be packaged as a physical gfxlib2 reference.

For Android, `primitive-benchmark.bas` accepts `GFX2_REFERENCE` so the same
source can be packaged once with gfxlib2 and once with GLES gfxlib3. The
extension-only `gpu-surface-benchmark.bas` runs on Android GLES but has no
gfxlib2 column because gfxlib2 has no corresponding API.

`gpu-transform-benchmark.bas` keeps both surfaces resident, times long adjacent
scale, rotation, and Mode 7 runs, and ends every section with a one-pixel
download. It therefore includes completed shader work and exposes Vulkan
submission or GLES draw-call regressions. The Null backend is a correctness
reference rather than a performance reference. A gfxlib2 number is omitted
because no gfxlib2 command defines the same transform and sampling result.

On GLES, a consecutive opaque `LINE ... , BF` run with one target and clip is
measured through the instanced integer-texture rectangle path. The fixture's
Android `BOX BF` section is therefore both its public API benchmark and the
regression guard for ordered rectangle batching; different clips, alpha, and
outlined rectangles intentionally exercise the exact fallback path.

Desktop glyph coverage is now specifically a compact-command and tile-replay
regression guard. `console-benchmark.bas` stresses repeated opaque cell
overdraw, while the DRAW STRING section of `primitive-benchmark.bas` measures
sparser glyphs. Both end at an ordered readback, so they include queued shader
work rather than timing producer submission alone.

The PAINT fixtures guard both exact fast and general paths. Separate dispatches
discover and verify a candidate rectangular interior and perimeter, then many
workgroups write it only after a device-wide barrier. Otherwise lane zero owns
exact scanline topology while its workgroup writes each discovered span.
`paint-benchmark.bas` reports first-fill, repeated producer, and repeated
explicit-completion durations. `paint-coalescing-smoke.bas` proves compatible
recolours may collapse while a border-coloured intermediate operation remains
an ordering and topology barrier. Pattern, depth, alpha, and large-GPU-surface
smokes cover cases where a faster but approximate algorithm would diverge.

<!-- end of benchmark-coverage.md -->
