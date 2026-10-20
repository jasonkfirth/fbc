<!--
Project: FreeBASIC gfxlib3
File: drivers/README.md
Purpose: Map native window, input and graphics-context adapters.
Responsibilities: Explain platform ownership and backend integration.
This file intentionally does NOT implement BASIC drawing or common GPU commands.
-->

# Native graphics adapters

The [shared runtime map](../README.md) describes API, queue, compatibility and
backend ownership. This directory contains the OS-facing side of that flow.

| Directory | Native integration |
| --- | --- |
| `win32` | Win32 events, WGL contexts, Vulkan surfaces and controller access |
| `linux` | X11 windows and input, optional DRM/GBM/EGL presentation and GLES dispatch |
| `android` | NativeActivity, EGL/GLES, input and Vulkan surface integration |
| `darwin` | Cocoa windows and events, OpenGL contexts and Vulkan integration |

The producer and renderer use the root `gfx3_platform*.h` contracts. The
adapter owns native windows and context integration. GL entry points and
device operations follow the renderer thread's lifetime; creating a native
window does not authorize GPU calls from a producer thread.

Make selects the target adapter by basename, replacing shared unsupported
stubs. CPU-independent renderers live in `backend`; shared mode policy and
unsupported-platform adapters live in `platform`. BSD currently uses gfxlib2
for native windows and has no gfxlib3 adapter.

Linux's GLES adapter includes the Android GLES implementation when the headers
are available and the common unsupported backend otherwise. The wrappers keep
native context and symbol loading separate from the included renderer.
Windows and X11 drop handlers share gfxlib2 driver headers. Keep those relative
include targets intact across both libraries.

Darwin adapter files require the platform's Blocks compiler flags. Their
special Make patterns must follow the `drivers/darwin` object paths. Generic
compile rules cannot silently replace those recipes.

Run `make gfx-structure-test` after adding or moving an adapter. See the
[maintenance guide](../../../docs/gfx-maintenance.md) for native and GPU tests.

<!-- end of drivers/README.md -->
