<!--
Project: FreeBASIC runtime warning qualification
File: c-emitter-string-header-conversions.md
Purpose: Explain the shared string header conversion repairs and their checks.
Responsibilities: Record length/ownership contracts and distinguish native runtime
diagnostics from the emitted-C corpus queues.
This file intentionally does NOT claim full campaign completion or other OS testing.
-->

# Shared string header conversions

Seven conversion sites in fb_unicode.h and fb_string.h repeat across the native
runtime's translation units. The repair changes the common helpers instead of
adding diagnostic suppressions or editing examples. Public function signatures,
descriptor layout and ownership flag values are unchanged.

The temporary wide allocator rejects negative character counts and bounds the
whole allocation, including its terminator, by PTRDIFF_MAX. It converts to size_t
before adding the terminator. This prevents the old signed addition from
overflowing. The internal zero-character allocation still reserves one character;
the public fb_WstrAlloc entry point still rejects zero and negative lengths.

Copy and move convert counts only inside their existing positive-length guards.
The wide length helper explicitly restores the runtime's signed length type.
Compare returns equality for nonpositive ranges before consulting the C library,
matching the existing packed-field path, and converts positive counts explicitly.
The descriptor setter combines length and ownership bits in size_t before
restoring the signed descriptor representation. It preserves the original bit
semantics rather than masking or redefining accepted descriptor flags.

The permanent boundary test extracts the production function definitions and
descriptor declarations. An allocation recorder checks numerical limits without
requesting large buffers. Widths one, two and four are value models, with actual
host size types; separate Win32 object checks use real 32-bit size types. The
old-header control fails on negative allocation before reaching the old signed
overflow. A second fixture includes the real runtime headers and links the rebuilt
library to verify the public allocation path, ordinary wide values and packed
descriptor flags.

Run the boundary matrix with native Win64 GCC and Clang:

```powershell
python -B build_scripts/test-string-header-conversions.py `
  --gcc C:/path/to/ucrt64/bin/gcc.exe `
  --clang C:/path/to/ucrt64/bin/clang.exe `
  --output E:/existing-audit/string-header-matrix
```

The output directory is a reusable scratch slot. The driver refuses unrelated
files instead of generating a new directory for every test. An optional
--control-headers argument selects unchanged header preimages for the negative
control; no baseline is synthesized by reversing the repair in the test.

The complete native sweep uses the frozen portable runtime source manifest and
changes only the two header providers. Both C tools receive the same aggressive,
unsuppressed flags as the baseline. Identical GCC objects remain shared with the
sealed cold build; changed objects and logs are compressed in one SQLite receipt.
The comparison baseline and installed binaries are never overwritten. The
existing 160 MiB retained-component bound applies to the complete qualification.

An initial forced-include preflight was stopped and rejected because it added
artificial Clang header-search warnings and changed conditional empty units.
Its 517 completed member records remain explicitly unaccepted in
header-conversion-include-preflight.sqlite3. The replacement uses one source
provider tree, with unchanged inputs hard-linked to the sealed package and only
the two repaired headers stored separately. Natural include lookup is preserved;
a macro-prefix map retains the original runtime source filename literals.

Completed qualification: 2,090 accepted native checks remove 3,626 warnings
per ST/MT variant and seven at startup from each C tool. No other warning
class changes. The matrix passes 24 executions, six unchanged controls
which fail as expected, and six Win32 objects. The real-header fixture
passes eight linked executions and four Win32 objects. Both rebuilt normal
archives preserve exports; 224 self-host identities, both backend suites
and four ST/MT smoke executions pass. Eight current canonical error-provider
compiles cover the two providers which differ from the frozen experiment.
The structural guard passes 126 graph cases after four comment-only repairs.

The assembly wrapper is checked without the C-only macro-prefix option;
four corrective tool checks supersede its initial unused-option observations.
Their preimages and exact disposition remain in the native receipt database.

Qualification status and final native warning counts are recorded separately in
header-conversion-native.json and header-conversion-finish.json in the existing
E:/fbc-c-audit-20261105-forward-views-r2-qualification/crt-candidate/canonical
work area. Until those receipts report checks_complete, the full native sweep
and linked-runtime qualification remain unfinished. These results do not change
the recorded emitted-C corpus totals or qualify another platform.

<!-- end of c-emitter-string-header-conversions.md -->
