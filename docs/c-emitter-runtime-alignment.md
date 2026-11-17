<!--
Project: FreeBASIC runtime qualification
File: c-emitter-runtime-alignment.md
Purpose: Record explicit alignment gaps in shared runtime records.
Responsibilities: Bind original layouts, native diagnostics and object comparisons.
Installed libraries retain their existing product ownership.
-->

# Runtime list, printer and console alignment

`FB_LIST`, `DEV_LPT_PROTOCOL` and `DEV_LPT_INFO` name their existing four-byte
alignment gaps when native pointers occupy eight bytes. The following pointer
or reference-count member retains its original offset. `DEV_LPT_PROTOCOL.raw`
keeps its flexible-tail offset and allocation size. The temporary string-list
initializer names its original members and initializes the added gap to zero.

The Unix console record names its existing stream-pointer gap and the byte
between saved-state flags and native colors. Its complete original declaration
is exercised through the actual runtime and system headers at both pointer
widths. Eight Linux layout executions check 32 original metrics and four
incorrect-record controls fail their numeric assertions.

The current Linux multithreaded runtime rebuild removes 1,548 GCC padding
observations and thirteen Clang padding observations. GCC falls from 1,811 to
263 and Clang from 10,242 to 10,229. Every other warning class retains its count.
The console delta removes another 54 GCC and 34 Clang observations. The combined
reduction is 1,602 GCC and 47 Clang observations, reaching GCC 209 / Clang 10,195.
The Make-selected runtime archives and startup objects match their originals
byte for byte under both tools. All 1,036 actual native object comparisons pass.
The two `.build-config/fbrt0.o` paths hold Make command signatures and have
separate metadata identities.

Windows and Linux GCC/Clang executions at both pointer widths check 24 size,
alignment and original-member offset metrics. Sixteen original/changed layout
executions pass. Eight widened-count controls fail their numeric layout assertions.

The complete source cut contains 1,328 files. The first three-file delta has
coherent opened source hashes. The console cut also captures two concurrent
Windows loader edits, outside this Linux build's selected sources.
One serial native worker
owns a temporary source/build tree within 64 MiB. Its measured peak is below
15 MiB, and the tree is removed after completion.

The subsequent complete v5 source refresh captures six concurrent runtime
input changes. Its GCC 200 / Clang 10,192 counts have no input drift. Four GCC
objects and one Clang object differ across that recorded source delta. The
alignment comparisons above retain their exact original-object qualification.
After this source cut completed, concurrent work changed seven further runtime
inputs. Those later inputs have no whole-runtime result in this batch. The
200 / 10,192 counts identify the captured v5 source cut.

The final raw-diagnostic audit includes eighteen GCC escape-sequence warnings
that have no bracketed warning-option label. Earlier option-only counters omitted
them. The totals above include these diagnostics at every source cut. The measured
1,602-warning alignment reduction is unchanged.

## Evidence

The existing `build/c-emitter-forward-views-20261105` work area retains
`runtime-current-packet-v2-20261115.json.zlib`,
`runtime-current-packet-v3-20261115.json.zlib` and both corresponding Linux
build receipts. The subsequent v4 packet/build pair records the console delta.
The layout packet and Linux receipt use the
`runtime-header-layout-*-v1-20261115` names. These retain declaration bytes,
numeric contracts, commands, actual native output and incorrect-record controls.
The console's complete-header layout controls use
`runtime-console-layout-linux-v1-20261115.json.zlib`. The v5 packet/build pair
records the subsequent concurrent source refresh and its changed objects.

<!-- end of c-emitter-runtime-alignment.md -->
