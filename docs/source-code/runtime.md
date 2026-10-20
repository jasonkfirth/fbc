<!--
Project: FreeBASIC source documentation
File: source-code/runtime.md
Purpose: Explain runtime source ownership and its connection to generated code.
Responsibilities: Map portable groups, startup, native layers, and ABI contracts.
This file intentionally does NOT define language syntax or claim platform parity.
-->

# Core runtime source tree

[src/rtlib](../../src/rtlib/) implements the core services used by compiled
BASIC programs. The compiler's runtime call builders describe these functions;
the runtime implements them. Its [local README](../../src/rtlib/README.md)
contains the full directory map and memory, ABI and threading contracts.

```text
rtlib/
    fb.h, fb_*.h            Shared runtime types, configuration and ABI contracts
    core/                  Initialization, shutdown and control-flow support
    arrays/                Descriptors, bounds, allocation, REDIM and destruction
    strings/               Byte strings and temporary descriptors
        wide/              WSTRING operations
        unicode/           Counted UTF-8 USTRING operations
        encoding/          Encoding conversion
    console/               PRINT formatting, console input and cursor behavior
    hooks/                 Dispatch to console services or installed graphics hooks
    files/                 BASIC file numbers, policy, typed I/O and VFS entry points
    devices/               Disk, pipe, serial, TCP, printer and other file hooks
    data/ errors/           DATA records and runtime error behavior
    datetime/ math/        Date/time and numeric operations
    memory/ objects/       Memory helpers, object support and logical serialization
    threads/               Thread contexts and call support
    services/              Notifications, idle polling, randomness and drop payloads
    system/                Process, environment and filesystem services
    profiling/ legacy/     Profiling and compatibility support
    static/                Startup sources linked as separate objects
    drivers/               Native OS, architecture and startup replacements
```

## Read the shared contract before the implementation

[fb.h](../../src/rtlib/fb.h) assembles the runtime interfaces.
[fb_config.h](../../src/rtlib/fb_config.h) establishes platform and CRT feature
configuration. Headers such as [fb_string.h](../../src/rtlib/fb_string.h),
[fb_array.h](../../src/rtlib/fb_array.h) and
[fb_file.h](../../src/rtlib/fb_file.h) describe storage shared across many
translation units and, in some cases, with generated code.

Dynamic strings have descriptors and temporary-lifetime rules; arrays have
descriptors, dimensions and bounds. They cannot be treated as plain C strings
or a raw allocation without checking those rules. A function's declaration,
the compiler's call builder and the selected runtime implementation must agree
on widths, calling convention and parameter ownership.

For example, [strings/str_concat.c](../../src/rtlib/strings/str_concat.c)
implements byte-string concatenation. Follow its descriptor handling and
temporary cleanup along with the caller. A change that returns the right
characters but leaks a temporary or frees a borrowed value is still incorrect.

## Policy, devices and graphics hooks

The `files` layer owns BASIC file behavior. The `devices` layer supplies the
operations for a particular stream type. Start with
[files/file_open.c](../../src/rtlib/files/file_open.c) and `fb_file.h` when
tracing a numbered file operation, then follow the chosen device hooks.

Console operations can also be redirected to the active graphics runtime.
[hooks/hook_printstr.c](../../src/rtlib/hooks/hook_printstr.c), for example,
checks the installed `printbuffproc` hook and otherwise calls the native
console path. [fb_hook.h](../../src/rtlib/fb_hook.h) defines that shared hook
contract. The graphics library owns its drawing state and native window;
rtlib owns the common dispatch and fallback.

## Native implementations are selected at build time

[mk/rtlib-sources.mk](../../mk/rtlib-sources.mk) owns portable source groups
and replacement precedence. For a C basename, selection prefers:

```text
OS/architecture > OS > shared Unix where enabled > portable subsystem
```

The [driver map](../../src/rtlib/drivers/README.md) explains the individual
platform directories. A same-name native file replaces the whole lower-layer
translation unit. It is not an extra runtime plugin loaded later. Keep native
API handling in that layer and reusable BASIC policy in the portable owner.
Assembly selection and C/assembly member collisions also need checking.

The shared Unix layer is enabled for an explicit set of targets. Cygwin uses
the Win32 source layer, while illumos uses Solaris sources. Feature flags can
also compile a selected source into an unavailable-service stub, so the mere
presence of an API file does not establish platform support.

## Startup objects and archive variants

[static/fbrt0.c](../../src/rtlib/static/fbrt0.c) and its neighboring startup
sources are compiled into separate objects. Native `drivers/<OS>/static`
implementations can replace them. These startup objects and the runtime
archive have separate link responsibilities.

Make derives ordinary, position-independent (PIC), multithreaded (MT), and
combined PIC/MT object lists from the selected source graph. The enabled
variants depend on target policy. Their object directories and archive names
are defined in [mk/build-layout.mk](../../mk/build-layout.mk) and
[mk/archives.mk](../../mk/archives.mk). Keep a fix applicable to every enabled
variant, including any per-file flags needed by a platform implementation.

For a runtime change, inspect the compiler declaration, shared headers,
portable owner, native replacements and focused tests together. The
[maintenance guide](../../src/rtlib/README.md) describes the relevant locks,
allocation rules and validation commands.

<!-- end of source-code/runtime.md -->
