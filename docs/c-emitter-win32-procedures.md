<!--
Project: FreeBASIC compiler verification
File: c-emitter-win32-procedures.md
Purpose: Record Windows procedure bindings and console page-loop qualification.
Responsibilities: Explain Windows ABI contracts, diagnostics and remaining limits.
This file intentionally does NOT qualify complete printer or condition lifecycles.
-->

# Windows corpus procedure contracts

Two FBRuntime translation units cast GetProcAddress's generic FARPROC directly
to different function-pointer types. Four ST/MT profiles produce GCC and Clang
cast warnings. The printer path also passes a generic-text export name to
GetProcAddress. Export names are narrow strings even when the export ends in W;
the original Unicode declaration is rejected before C emission.

`win32_proc_address.bi` now checks the destination's code-pointer size and copies
the FARPROC representation into its declared API type. Windows uses uniform
code-pointer representations. The caller supplies the exact signature and keeps
the module loaded for every call. This is a typed binding operation, not a call
through FARPROC or a disabled cast diagnostic. SignalObjectAndWait and
GetDefaultPrinter explicitly declare stdcall and by-value API arguments. Printer
export names use LPCSTR for both A and W lookups.

The console width implementation uses distinct window_page and buffer_page
indices. Replacing only those emitted local tokens with their original names
reproduces the complete old C bytes in both profiles. No console was resized.

## Qualification

The native fixture contains the exact production procedure types and lookup
statements, plus the shared binding helper. The installed Windows SDK supplies
the other side of the calls. Both C producers, ANSI/Unicode, GCC/Clang,
GNU11/GNU23 and O0/O2 pass 32 native runs. Real procedure-address bits match SDK
lookups. SignalObjectAndWait operates on two private events, which are closed.
A correctly typed printer callback stub checks BOOL results, text storage and
updates through the original DWORD pointer. Actual printer procedures are
resolved for identity checks and never invoked. No printer is opened.

Forty emitted-C and forty SDK objects pass, including eight of each for Win32.
Four Win32 LLVM-IR checks verify x86_stdcallcc on the indirect printer callback.
The dependency closures bind 356 SDK headers. Win32 object and IR checks do not
establish execution on a 32-bit Windows system.

Six historical controls cover four ANSI native cases retaining the original
cast diagnostics and two original Unicode compile rejections. An initial
harness expected the latter to reach native execution; its compiler rejection
was reclassified explicitly and retained. All 156 completed current native,
object and IR checks were preserved as a bound checkpoint with unchanged
source, compiler, SDK, runtime and flag inputs. The historical-control harness
was corrected without treating missing transient files as cached outputs.

Strict source-only fblint reports four files, zero errors and seven unchanged
findings: six existing presentation-whitespace findings and one existing
large-file sectioning suggestion. Three original file-ending findings are
resolved by the required footers. No new lint finding is introduced. Broader
compiler-backed semantic lint remains open.

## Selected queues and limits

| Queue | Profiles | GCC warnings | Clang warnings |
| --- | ---: | ---: | ---: |
| Compiler | 224 | 0 | 4,802 |
| Unit tests | 703 | 16 | 14,347 |
| Production | 1,117 | 443 | 11,045 |
| Additional projects | 239 | 1,683 | 39,758 |
| Total | 2,283 | 2,142 | 69,952 |

Six changed selected outputs pass their original unsuppressed diagnostics;
another 2,053 project outputs are byte-identical. All 224 compiler C outputs
close the fixed point. Compiler sources and the accepted compiler binary remain
unchanged from the destructor checkpoint. Twelve PNG profiles remain excluded.
The unrelated GUI image-unit warning was located but no image source was edited.

The packet qualifies the binding and callback contracts. Full printer-name
allocation/retry cleanup and Unicode conversion, complete printer operation,
and condition-variable implementation/threading behavior remain open. In
particular, the inspected printer-name allocation loop does not stop immediately
on allocation failure or require final API success before returning its buffer.
Those paths were preserved and are recorded for a separate repair.

Installed binaries and unrelated source/changelog bytes are preserved. The
stage retains evidence under a 64 MiB cap. Receipts, source preimages and native
controls are in `build/c-emitter-win32-procedure-contracts-20261102/summary.json`.
The [current status](c-emitter-current-status.md) records the wider unfinished
warning-cleanup campaign.

<!-- end of c-emitter-win32-procedures.md -->
