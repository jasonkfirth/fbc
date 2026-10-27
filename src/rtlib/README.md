<!--
Project: FreeBASIC runtime library
File: README.md
Purpose: Explain runtime ownership, interfaces and maintenance workflows.
Responsibilities: Map code to contracts, lifetime rules, ports and regression tests.
This file intentionally does NOT contain language syntax or qualification results.
-->

# Runtime library structure

Start with the BASIC operation being implemented, then follow its runtime
entry point. The compiler's `src/compiler/runtime` modules describe calls to
`fb_*` functions. The implementations live here. Console graphics and sound
belong to `src/gfxlib2`, `src/gfxlib3` and `src/sfxlib`; they use runtime hooks
and shared descriptors rather than adding graphics drivers to this library.

## Directory ownership

Portable C sources are grouped by the state or interface they own. The
established filenames remain useful when comparing an implementation with a
platform replacement. `fb.h` and the `fb_*.h` contracts retain their root
include paths because the other runtime libraries include them as well.
Native code lives under [`drivers`](drivers/README.md), so the top level shows
portable responsibilities rather than a list of platforms.

| Directory | Responsibility and useful starting points |
| --- | --- |
| `core` | Startup and shutdown in `init.c`; compiler control-flow support in `gosub.c` |
| `arrays` | Array descriptors, bounds, REDIM, PRESERVE and element destruction; start at `array_core.c` and `fb_array.h` |
| `console` | Console input/output, cursor state, PRINT and PRINT USING; start at `con_readline.c` and `io_printusg.c` |
| `hooks` | Route BASIC console operations to installed graphics callbacks or console services; see `fb_hook.h` |
| `data` | READ/RESTORE and compiler-emitted DATA records; see `fb_data.h` |
| `devices` | File-hook implementations for disk, console, pipe, printer, serial and TCP streams; private device headers stay here |
| `errors` | Runtime error state, assertions, bound/pointer diagnostics and signal integration |
| `files` | BASIC file policy, numbered handles, typed I/O, path attributes and VFS entry points; see `fb_file.h` |
| `datetime` | Date serial values, calendar arithmetic, time and locale formatting |
| `math` | Numeric intrinsics, portable 128-bit and C `long double` operations, and the language RND/RANDOMIZE state |
| `memory` | Raw memory operations and internal intrusive-list storage |
| `objects` | Object type checks and logical serialization; serialization owns no transport |
| `profiling` | Profiling measurements, call records and report output |
| `strings` | Byte-string descriptors, temporary descriptors, allocation, conversions and VB-compatible operations |
| `strings/wide` | WSTRING operations in the target's `FB_WCHAR` units |
| `strings/unicode` | Counted USTRING storage, UTF-8 indexing and Unicode case mappings |
| `strings/encoding` | Byte/wide/UTF encoding conversion helpers |
| `threads` | Thread identity, call marshalling and per-thread runtime contexts |
| `services` | Checked notifications, idle polling, secure randomness and copied drop payloads |
| `system` | Environment, command line, processes, filesystem navigation and GPIO entry points |
| `legacy` | QuickBASIC and legacy API compatibility |
| `static` | Startup objects linked separately from `libfb`; constructor ordering lives here |
| `drivers` | OS adapters, shared Unix code, architecture variants and native startup overrides; see the [driver map](drivers/README.md) |

## Source selection and builds

[`mk/rtlib-sources.mk`](../../mk/rtlib-sources.mk) is the only portable runtime
directory list. [`mk/source-graph.mk`](../../mk/source-graph.mk) consumes that
selection and derives the normal, PIC, multithreaded and PIC/multithreaded
object lists. Compile and archive recipes remain in `mk/build`.

For a given C filename, the highest available layer wins:

```text
OS/architecture > OS > shared Unix (where enabled) > portable subsystem
```

For example, `drivers/win32/file_copy.c` supplies a native implementation while
`files/file_copy_crt.c` is a separate helper, not a replacement for that name.
Adding a same-name file in another portable subsystem is an error. An archive
records member basenames, so different object directories alone cannot make
duplicate names safe. Make rejects collisions before compiling.

The Unix layer is an explicit target list, not an inference from the build
machine. Cygwin uses the Win32 source layer; illumos uses Solaris sources.
`DISABLE_X11` removes the Unix X11 focus/scancode helpers. Target feature
macros can still compile stubs inside selected files. A selected source is
therefore not evidence that a target implements every API in that source.

Object directories mirror source directories. Keep special per-file rules in
step with moves: Win64 GOSUB needs unwind tables in every runtime variant.
Startup objects have their own platform replacement rule and remain separate
link inputs. A runtime archive cannot substitute for a missing `fbrt0.o`.

## ABI and include rules

Include `fb.h` before system headers unless the file has a documented reason
to use a narrower contract. `fb_config.h` sets platform and CRT feature macros
before libc headers are parsed. Adding a system header first can silently
change large-file declarations or platform types.

Use paths relative to `src/rtlib` for shared/private runtime includes. A local
platform header can remain local to its implementation. Shared contract
headers belong at the root; a header describing device internals belongs in
`devices`. Other libraries should use the shared contracts, not device state.

`FBCALL`, integer and pointer widths, `FB_WCHAR`, structure field order and
compiler-generated descriptors are ABI decisions. The compiler does not
discover the C layout dynamically. Before changing a declaration, inspect its
compiler call builder, BASIC-facing declaration, all platform replacements
and tests. Adding a field to a shared descriptor is not a local refactor.

BASIC truth values use `FB_TRUE` (-1), while a native C/OS API can require 1.
Pass the representation required by the receiving interface, especially for
varargs calls where an incorrect type or width cannot be repaired by a cast
inside the callee.

`fb_h*` marks an internal helper by convention. Some helpers are visible
between translation units or to the graphics/sound runtimes; the prefix alone
does not establish a private linkage boundary. Keep file-local helpers `static`
when their consumers permit it.

## State, ownership and synchronization

`core/init.c` owns `__fb_ctx`, including arguments, the file table and installed
hooks. Startup first creates platform/TLS support, then permits BASIC global
constructors to use the runtime. Shutdown releases file/device state and TLS
before the platform exits. Global constructor/destructor ordering is specified
in `static/fbrt0.c`; it cannot be replaced by an arbitrary `atexit` callback.
Windows loader-lock restrictions also prevent waiting for graphics workers
from a DLL global destructor. Graphics cleanup has its own `fb_End` path.

Runtime TLS stores error, INPUT, DATA and other contexts defined by the keys
in `fb_thread.h`. In an MT runtime each thread owns its contexts; a context's
destructor runs when the runtime frees that thread's table. OS TLS does not
necessarily run those destructors for us. See `threads/thread_ctx.c` and the
platform thread wrappers before changing teardown. Native stack minimums and
the NetBSD TLS-key lifetime exception are documented beside their definitions.

With MT enabled, `FB_LOCK` protects shared runtime/file state and `FB_STRLOCK`
protects the temporary byte-string descriptor pool. When both are needed,
acquire `FB_LOCK` first. A helper may rely on its caller holding a lock, so
follow the complete call path before adding another lock or moving a callback.
When both are needed, acquire `FB_LOCK` before `FB_GRAPHICS_LOCK`. The
[runtime/graphics lock contract](../../docs/gfx-locking.md) traces input,
allocation and mode teardown and explains driver/caller restrictions.
Non-MT macros are empty and do not prove that code is safe in an MT archive.

| Storage | Owner and release rule |
| --- | --- |
| Dynamic STRING data | Its descriptor owns allocated bytes; string helpers manage capacity and the trailing NUL |
| Temporary STRING descriptor | The process descriptor pool owns the descriptor; callers follow the temporary-result contract, not `free(descriptor)` |
| WSTRING | Wide elements use target `FB_WCHAR` width; count bytes and elements separately |
| USTRING | Uses the `FBSTRING` descriptor ABI with byte lengths/capacities, but Unicode text operations use scalar positions; see `fb_ustring.h` and `ustr_core.c` |
| Array data | Descriptor `ptr` identifies the allocation; `data` includes the lower-bound adjustment and is not the address to free |
| Object/string array elements | Run the matching destructor path before releasing the array allocation |
| Numbered file | The runtime file table owns the handle slot; the installed device owns `opaque` state and closes it through its hooks |
| Drop/service input | Borrowed inputs have counted lengths; retained drop items copy payloads into bounded owned storage |

File policy and device mechanisms are separate. `files/file_open.c` validates
the language operation and selects a device. `FB_FILE_HOOKS` dispatches reads,
writes and closing to `devices`. `opaque` is meaningful only to that device.
Callbacks carry explicit sizes; changing their units or short-read behavior
changes INPUT/LINE INPUT/GET semantics for every device using them.

Arrays have compiler-visible leading fields and a variable dimension table.
`FBARRAY.data` must stay at offset zero. Do not replace it with `ptr`, assume
zero lower bounds, or free an adjusted element address. The descriptor flags
also distinguish fixed dimensions from fixed storage.

## Making a bounded change

1. Find the ABI name in `fb_*.h` and the compiler's runtime declarations.
2. Read the owning subsystem above, then inspect same-name platform files.
3. Document caller ownership, units, error behavior and required locks where
   the implementation relies on them. Check size arithmetic before allocating;
   preserve the established failure/temporary cleanup path.
4. Add a focused regression that fails for the actual behavior being repaired.
   A native OS bug needs a native OS check; source selection alone cannot
   validate sockets, GUI events, TLS, signals or device behavior.
5. Run the structure check and rebuild the runtime variants that use the code.
   Use two jobs on the shared test VMs. Reuse one owned workspace per host and
   retain compact receipts instead of copying previous build directories.
6. Run the applicable language/unit tests, then the cross-platform gates when
   shared contracts or broad source ownership change.

Useful commands from the repository root:

```sh
make rtlib-structure-test
make -j2 compiler rtlib fbrt gfxlib2 gfxlib3 sfxlib
make -j2 unit-tests
make log-tests
python3 build_scripts/exampleageddon-freebasic.py --root "$PWD" \
    --fbc "$PWD/bin/fbc" --prefix "$PWD" --jobs 2 \
    --outdir out/exampleageddon-rtlib --fail-on-self-contained
```

Use `bin/fbc.exe` and `--target-os windows` on Windows. The structure check runs
in quick/full Make workflows and exercises 21 source policies, three
architecture values, X11 toggles, all four object variants, include resolution
and synthetic collision/replacement cases. Unusual architecture/OS pairings
exercise source selection only and are not cross-compilation claims.

The fbctests gate means fbcunit execution plus all four log-test dialect lanes.
Exampleageddon's failure exit gates its self-contained examples. Its other
rows describe platform, dependency, interactive and intentional-failure cases;
report those classifications separately from runnable examples.
SETDATE and SETTIME examples compile but do not execute in a sweep because
they can change the host clock. `--run-all` also keeps them compile-only.

The current restructuring receipts and native results belong in
[`docs/rtlib-restructure-verification.md`](../../docs/rtlib-restructure-verification.md).
This guide explains the design; that report identifies exactly what was tested.

<!-- end of README.md -->
