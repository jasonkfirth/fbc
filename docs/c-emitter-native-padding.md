<!--
Project: FreeBASIC C backend verification
File: c-emitter-native-padding.md
Purpose: Record the remaining native runtime padding repairs and ABI checks.
Responsibilities: Bind named storage, unchanged field layouts and native tests.
Other targets retain their platform-specific qualification.
-->

# Native runtime padding, November 15

Eight runtime sources name eleven existing alignment gaps in nine types. The
complete Linux MT runtime and startup builds reach GCC 15 / Clang 10,034,
removing twenty GCC and thirteen Clang observations. All GCC padding
observations in this build are closed. The last-qualified disjoint GCC total
is 218.

## Storage and initialization

The affected types hold TCP device state, profiler state, serialization nodes,
saved signal handlers, FORMAT mask state, Unicode reader state, terminal key
nodes, terminal key descriptions and X11 key descriptions. Each added byte
array occupies a preceding native alignment gap. Existing semantic members keep
their types, offsets and extents. Whole-type sizes, alignments and array strides
also match their original definitions.

The terminal and X11 tables use designated initialization so reserved members
receive zero initialization. All 25 terminal entries and all 105 X11 mapping entries
retain their values and order, including the terminal sentinel. Terminal nodes
use `calloc` to initialize reserved bytes along with their existing members.
Allocation failure preserves the existing tree. The node fixture checks lookup
results, mouse dispatch, refusal behavior, zeroed reserved bytes and balanced
ownership of 29 allocated nodes.

## Native qualification

Eight Linux GCC/Clang 32/64-bit O0/O2 contexts compare 64 layout properties
between original and current production definitions. Each context executes both
layout probes and both key-node fixtures. Current node fixtures pass 3,216
assertions, and original fixtures pass 1,752 assertions. Original/current field
layouts match in every context.

Eight Windows GCC/Clang 32/64-bit O0/O2 contexts execute sixteen layout probes
against the actual MinGW platform headers. The six shared types retain all 53
layout properties in each context. The 32-bit serialization node retains its
native Windows eight-byte alignment and its independently measured Linux
four-byte alignment. Existing unrelated Windows header padding warnings remain
in these diagnostic receipts.

The complete native builds contain 1,036 object profiles. GCC changes only
`str_format.o` and `io_inkey.o`. Clang changes only `ustr_io.o` and `io_inkey.o`.
Both startup objects keep their hashes. Both runtime archives change, and all
280 preceding current execution contexts run again against the rebuilt code.

Thirty-six additional current executions cover layout probes, terminal nodes,
the existing BASIC serialization roundtrip, the existing Unicode runtime unit
and the existing profiler filename unit. All 316 current Linux execution
contexts pass. Sixteen original-source layout/node executions provide positive
controls. Source-only library lint of the serialization program reports zero
errors and zero warnings.

The existing `tests/string/format.bas` suite subsequently passes eight native
executions through the actual fbcunit framework and `tests/fbc-tests.bas` entry
point. Both C producers, native tools and O0/O2 pass all 316 numeric/date assertions
per execution, totaling 2,528 assertions. The supplemental complete builds retain
all object, archive and startup hashes from the padding build. They rerun 56
preceding contexts and reuse 260 through exact binary and unchanged test-input
identities. Eight additional FORMAT contexts bring the qualified current total
to 324. An initial supplemental runner used an undefined helper name. Its corrected
successor completes every check, with the failed runner and log retained.

The serial worker uses at most 21,639,921 scratch bytes and removes its temporary
tree. The Windows layout controller also removes its products. Compiler and
installed product identities remain pinned separately from these runtime builds.

## Reproduction and receipts

The regression runner accepts an original source tree and the current tree.
Both trees supply their actual headers and verbatim internal type definitions.

```sh
python3 -B build_scripts/test-native-padding-layouts.py \
  --cc /usr/bin/gcc --bits 64 --optimization 2 \
  --before-root /path/to/original/fbc \
  --receipt /path/to/new-padding-receipt.json
```

Repeat with Clang, both word sizes and both optimization levels. Windows uses
`--platform windows`, native MinGW headers and a matching linker driver.
`--native-flags-json` accepts the complete unsuppressed native flag array.
Receipts are under `build/c-emitter-forward-views-20261105`.

- `runtime-current-packet-v36-20261115.json.zlib` freezes all runtime inputs,
  regression sources, BASIC emissions and exact original bytes for eight files.
- `runtime-current-linux-v31-20261115.json.zlib` records both complete builds
  and the fresh regression executions.
- `runtime-current-packet-v37-20261115.json.zlib` and
  `runtime-current-linux-v33-20261115.json.zlib` record the unchanged FORMAT
  suite, real fbcunit framework and supplemental native executions.
- `native-padding-windows-v1-20261115.json.zlib` records the Windows layouts.
- `native-padding-qualification-v2-20261115.json.zlib` binds source, diagnostic,
  documentation and protected-product hashes.

## Remaining work

The runtime's fifteen GCC observations comprise five switch-default, one
terminal-speed conversion, one redundant declaration, six discarded qualifiers
and two cast-qual observations. Coreutils retains its 131 nonliteral-format
observations, seventeen pending dynamic-format runtime contracts and four
finite-format compiler sites. Other Clang source/ABI owners remain open.
Templari retains its last-qualified 674-profile cut while newer concurrent
inputs await replay. Other project queues retain their preceding qualified cuts.

<!-- end of c-emitter-native-padding.md -->
