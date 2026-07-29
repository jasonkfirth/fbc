# newJRPG gfxlib3 qualification

## Scope

The JRPG engine at `C:\Nextcloud\games\newjrpg` was built and exercised with
gfxlib2 and gfxlib3. The qualification covers the production editor and player,
the engine's graphics-focused smoke tests, both desktop Vulkan adapters, Win64,
Win32, and the connected Android device.

The purpose was not only to make the engine run. Its real editor workload was
profiled to find renderer-library costs which synthetic tests had not exposed.
The resulting Vulkan repairs apply to every gfxlib3 program.

## Engine programs

These production programs build without warnings under both gfxlib2 and
gfxlib3 on Win64:

- `jrpg.bas`
- `jrpg_Player_Main.bas`

Both also build as Win32 gfxlib3 programs. The player no longer calls
`ScreenPtr` merely to confirm that `ScreenRes` succeeded. `ScreenRes` is the
authoritative result, while `ScreenPtr` is a request for writable CPU access to
the screen. Avoiding that probe lets a normal gfxlib3 player retain GPU
ownership from startup.

The Win32 build exposed a separate omaNet portability defect. The server used a
64-bit integer cast to carry a client slot through `ThreadCreate`. It now passes
the address of a stable pointer-sized thread argument stored in the fixed
server-client array. The thread validates both the pointer and recovered slot
before using it. The corresponding installed and source copies of
`omanet_server.bi` were updated together.

## Win64 correctness matrix

The following tests compile under gfxlib2 and gfxlib3:

- `jrpg_RuntimeRenderer_Smoke`
- `jrpg_Lighting_Smoke`
- `jrpg_OverlayBitmap_Smoke`
- `jrpg_ScreenPicture_Smoke`
- `jrpg_DoubleBuffer_Smoke`
- `jrpg_BackendDoubleBuffer_Smoke`
- `omaGui_BackendDoubleBuffer_Smoke`
- `jrpg_ClientRenderer_Smoke`
- `jrpg_PlayerScreenshot_Smoke`
- `jrpg_EngineSession_Smoke`
- `jrpg_QFAKGraphicalBattle_Smoke`

All eleven gfx3 executables pass with forced OpenGL, forced Vulkan device 0,
and forced Vulkan device 1. The selected devices are:

| Renderer selection | Physical result |
| --- | --- |
| OpenGL | OpenGL 4.3 on the NVIDIA desktop driver |
| Vulkan device 0 | NVIDIA GeForce RTX 2060, vendor `10de`, device `1f11` |
| Vulkan device 1 | Intel UHD Graphics 630, vendor `8086`, device `3e9b` |

The first ten tests include direct pixel assertions, image load and readback,
page switching, screenshots, and a live engine session. The QFAK battle test
adds the complete animated graphical battle scenario and its own state and
graphics assertions.

The gfxlib2 versions also pass. Battle screenshots from independent executions
are not byte-identical because the scenario contains dynamic state. No
byte-identical screenshot claim is made. The deterministic pixel assertions
inside the focused tests and the graphical battle test's own assertions are
the parity evidence.

## Win32 correctness matrix

All eleven gfxlib3 tests and both production programs compile as Win32
executables from the current sources. The first ten tests pass with:

- forced OpenGL;
- forced Vulkan on the RTX 2060;
- forced Vulkan on the Intel UHD 630.

The long QFAK battle executable was compiled for Win32 but was not included in
the three-backend Win32 execution sweep. Its Win64 builds passed all three
backends.

## Android device result

`tests/gfx3/build-newjrpg-android.sh` packages the asset-independent runtime
renderer smoke against the current Android AArch64 threaded PIC archive. The
APK was installed on connected device `b857d433`, an AGM A8.

The device exposes no Vulkan HAL. Automatic selection logged the expected
Vulkan unsupported result `-6`, then initialized:

```text
OpenGL ES 3.0 V@145.0 AU@ (GIT@I503d7f4db7)
```

The complete test logged:

```text
runtime renderer smoke OK
FREEBASIC_ANDROID_EXIT:0
```

The process then shut down cleanly. This proves the JRPG renderer, direct pixel
checks, clipping, effects, and readback on the physical GLES device. It is not
Android Vulkan coverage.

## Vulkan repair

The editor submits approximately 11,800 points, 84 lines, 187 rectangles, and
361 blits per application frame. It performs no steady screen download. The
original Vulkan cost was therefore command execution overhead, not a hidden
RAM framebuffer.

The completed repair addresses that workload at four levels:

1. Point and rectangle packets use fence-owned, persistently mapped buffers in
   each Vulkan submission slot. The renderer no longer creates and destroys
   `VkBuffer` and `VkDeviceMemory` objects for every packet.
2. Mixed-colour rectangle replay now builds a compact list of touched 16 by 16
   tiles. One workgroup is dispatched per touched tile rather than across the
   complete destination surface for every small UI packet.
3. Adjacent opaque points, styled lines, and rectangles now share one mixed
   primitive packet. A tile invocation replays the candidates for one
   destination pixel in FIFO order and writes that pixel once.
4. Rectangle-free packets use a compact table of useful 64-pixel workgroups
   instead of padding every point and short line to the longest primitive.

The CPU produces conservative command and tile metadata. The Vulkan shader
still performs every per-pixel clip test, styled-outline coverage decision,
last-writer ordering decision, colour mask, and destination write.

The primitive command, range, index, tile-coordinate, useful-workgroup, and
winner buffers are owned by the same submission-slot fence as the command
buffer which consumes them. They cannot be rewritten while the GPU still has
access. The backend's temporary flattening array is also persistent and grows
with checked arithmetic instead of allocating once per packet.

## Performance result

The editor's fixed command density allows application frame rate to be inferred
from page-set and presentation counts:

| Live editor path | Before repair | Persistent buffers and compact rectangles | Mixed primitive tiles |
| --- | ---: | ---: | ---: |
| Vulkan RTX 2060 | about 10 frames/s | about 17 to 19 frames/s | about 34 to 37 frames/s |
| Vulkan Intel UHD 630 | about 12 to 13 frames/s | about 17 to 19 frames/s | about 26 to 30 frames/s |

The persistent rectangle buffers provided the largest RTX gain. Compact
touched-tile dispatch reduced the Intel render execution interval further. The
new mixed packet then reduced median Vulkan execution from about 14.0 to 8.25
milliseconds per page on the RTX, and from about 15.4 to 8.23 milliseconds on
Intel. Steady profiles continue to report zero screen downloads.

The original 2026-07-28 complete QFAK graphical battle gave this end-to-end
comparison:

| Runtime/backend | Completion |
| --- | ---: |
| gfxlib2 | 102.06 s |
| gfxlib3 OpenGL | 100.46 s |
| gfxlib3 Vulkan, RTX 2060 | 88.67 s |
| gfxlib3 Vulkan, Intel UHD 630 | 96.96 s |

In that 2026-07-28 run, gfx3 OpenGL was 1.6 percent faster than gfxlib2,
RTX Vulkan was 13.1 percent faster, and Intel Vulkan was 5.0 percent faster.
These are single controlled observations on this machine, not universal
thresholds or measurements of the later revised QFAK source.

The revised QFAK source also passed after mixed primitive coalescing. Its
119.91-second wall time is not used as a primitive-performance claim. Profiled
expensive intervals contain thousands of synchronous `POINT` reads per second,
zero point and line draws, and at most a few rectangles. Its timed engine
updates and thread scheduling make wall time variable; a preserved older
executable varied from 88.91 to 105.99 seconds without changing the primitive
renderer. QFAK remains valuable correctness coverage, while the production
editor directly exercises this optimization.

The profiled editor remained GPU-resident with zero download traffic. POINT
readbacks in the runtime renderer smoke are intentional correctness assertions
and are not representative of the production editor loop.

## Additional renderer regressions

After the Vulkan changes, each of these existing gfxlib3 tests passed on
OpenGL, Vulkan RTX, and Vulkan Intel:

- `mixed-rectangle-primitive-order-smoke.bas`
- `small-filled-rectangle-batch-smoke.bas`
- `mixed-primitive-order-smoke.bas`
- `pending-points-order-smoke.bas`
- `page-flip-presentation-smoke.bas`
- `put-clipping-smoke.bas`

This specifically covers mixed point, line, rectangle, and ellipse fallback
order, the changed rectangle scheduler, point stream order, page presentation,
and GPU clipping. The new mixed-order test also passes as a Win32 Vulkan
executable on both physical desktop adapters.

## Rebuilt artifact identities

```text
Win64 libfbgfx3.a           7A2E240B93358B10D37AC0C1D68CE6964C980C5BC18FE10C1B5787FBD9BEE99B
Win64 libfbgfx3mt.a         DE4BD5FC3D459209BB480E4315B2A8635E3F5ECC18761995476FA678F5811CBB
Win32 libfbgfx3.a           B637DD93016D1D7C69BA9A2A8461595858B93B16BAC439640A5F2CA40EBB2D22
Win32 libfbgfx3mt.a         4CC031A30622729655AB2D8A03697DE0963A97ED64102D0EE4E7B6417A704A5C
Android libfbgfx3pic.a      C398E0EA485972FD5A65D86CE315D031A2ACC1DB47B8BCE025678657617AF501
Android libfbgfx3mtpic.a    F97E197D4B65BA6F5B5DA56AF0F1BD06C0A966F46391418382A21203A2D5E791
```

The physical AGM A8 result above remains the most recent installed JRPG GLES
smoke. The current change is Vulkan-only, so its rebuilt Android archive has no
different execution path on that non-Vulkan device.

<!-- end of newjrpg-qualification.md -->
