<!-- Project: FreeBASIC C backend and Windows variant library
File: c-emitter-variant-abi.md
Purpose: Record the shared native layout, text and ownership repair.
Responsibilities: Explain the source changes and qualification boundaries.
This file intentionally does NOT claim whole-campaign completion. -->

# Windows variant library contracts

The library under `examples/win32/variants/libvariant` and its public header
now use the same Windows `VARIANT` definition. Existing caller programs are
unchanged. Before this repair, the public header substituted a byte array:

| Target | Public raw / wrapper / containing record | Implementation |
| --- | --- | --- |
| Win64 | 16 / 16 / 17 | 24 / 24 / 32 |
| Win32 | 16 / 16 / 17 | 16 / 16 / 24 |

The native record alias replaces that placeholder. Both build modes now
preserve native size, alignment and type identity, including records passed
by value. The wrapper name and existing API remain available.

One internal routine now converts narrow strings through `CP_ACP` into an
owned Unicode BSTR. All 17 narrow-text overloads use it. Windows' queried
length includes the terminator; the BSTR length excludes it. Null inputs and
failed conversion or allocation return a null BSTR. This corrects both the
constructor's extra character and the other overloads' use of the binary
[SysAllocStringByteLen allocator](https://learn.microsoft.com/en-us/windows/win32/api/oleauto/nf-oleauto-sysallocstringbytelen),
which does not convert ANSI text to Unicode.

Copy assignment checks for the same native source and destination. It lets
[VariantCopy](https://learn.microsoft.com/en-us/windows/win32/api/oleauto/nf-oleauto-variantcopy)
release the initialized destination instead of clearing the source first.
The previously declared native-value cast now returns an independent copy;
the caller releases that native result with `VariantClear`. The existing
wide-string cast continues returning an independently owned BSTR.

`tests/compiler-support/variant-native-abi.bas` checks public layout, native
values, enclosing records, copy ownership, and both native and wide-string
casts. `variant-string-overloads.bas` exercises every narrow overload,
positive and negative comparisons, empty/null text and self-assignment.
Each executable repeats its checks 32 times. A baseline run confirmed the
extra constructor character, truncated narrow assignment and cleared
self-assignment; the native cast originally failed to link.

The qualification batch builds all 59 Make-selected library modules, with
both C producers and unsuppressed GCC/Clang diagnostics. Both Win64 and Win32
library builds have zero C warnings. Across the canonical and private
compiler cuts, 64 ordinary native runs and eight mixed GCC LTO runs pass
under GNU11/GNU23 and O0/O2. Separately, 32 Win32 caller objects and eight
mixed caller/implementation LTO objects pass, with no Win32 execution claim.
The LTO checks retain the library's GIMPLE type information; native archives
alone cannot establish cross-unit C type compatibility.

`variant-native-abi.py` is the permanent Windows regression runner, integrated
into `build_scripts/test-compiler-backends.py`. It builds the library once in
a temporary directory, materializes a native object for both C compilers,
and keeps the per-module LTO objects for mixed-type checks. It links the
specified runtime directly instead of selecting an installed variant archive.
The default two-tool run executes eight ordinary and four mixed-LTO cases.
Scratch storage is capped at 16 MiB and removed after the test.

Receipts, exact inputs, compressed preimages and source bindings are under
`build/c-emitter-variant-abi-20261105`; owned artifacts are on E:. The batch
uses at most two heavy workers, a 512 MiB volume free-space floor and a
32 MiB retained-artifact limit. Installed compiler/runtime/variant archives
and unrelated dirty work are preserved. Numeric conversion policy and the
library's existing HRESULT handling are outside this focused repair.

The complete broad and Exampleageddon warning queues still need rebinding
to the current compiler cuts before publishing new global counts. This is
a library correctness repair, not a claimed reduction of thousands of warnings.
PNG, concurrent Optical work and user-closed optional checks remain excluded.

<!-- end of c-emitter-variant-abi.md -->
