<!--
Project: FreeBASIC C backend qualification
File: c-emitter-abi-observations.md
Purpose: Bind native layout and module-registration observations to contracts.
Responsibilities: Record exact C identities, layout guards and native controls.
Other native diagnostic classes retain their own source reviews.
-->

# Native layout and module registrations

The compiler checks each emitted BASIC record's C size against its calculated
BASIC size. Named fields receive `__builtin_offsetof` checks against their BASIC
offsets. Promoted anonymous members are checked in the enclosing record. These
guards reject a mismatched layout during native compilation.

Clang's `-Wpadded` reports alignment bytes within these declarations. The ABI
ledger associates each observation with the actual compiled C identity and its
size/field guards. Padding in a promoted anonymous record is checked through
the enclosing record and its flattened members. The fifteen private
cleanup-carrier observations describe C-owned records beginning with
`FB_ERROR_FRAME`. Generated C owns their allocation and named-member access.
Their cleanup behavior retains the [managed-field qualification](c-emitter-cleanup-dependencies.md).

The completed current ledger covers 1,591 profiles through selected compiler
`6634540c`. It reuses
native diagnostic evidence only after reproducing the exact compiled C bytes.
Compact native parents retain their original flags and counts. When a padding
message was absent from a compact parent, fresh Clang compilation supplies it
and must reproduce every recorded warning class/count.

| Qualified cut | Exact compiled C profiles | Guarded padding | Private C carrier padding | Module registration observations |
| --- | ---: | ---: | ---: | ---: |
| Compiler | 224 | 1,327 | 0 | 13 |
| fbctests | 703 | 56 | 0 | 712 |
| Templari paired cut v8 | 664 | 52,413 | 15 | 681 |

These checks account for 55,217 layout/registration observations. The current
Templari ledger reuses 648 exact parent C identities and reads the sixteen new
or changed C inputs from their archives. Both producers emit identical C in all
664 profiles. The qualified source cut has 1,177 inputs and a twenty-file delta.
Other diagnostic classes remain counted, including Templari's two callback casts.

## Startup and finalization controls

Native C constructor/destructor registrations implement BASIC module lifetime.
Clang reports both kinds through `-Wglobal-constructors`. The compiler's
`semantic-expressions.bas` module requires an exit registration for its global
state and has a destructor-only observation.

Sixty-four native executions cover Windows/Linux, x86/x86_64, both C producers,
GCC/Clang, O0/O2 and normal/debug builds. An independent C record checks field
values, size and offsets. A BASIC global object's constructor runs once before
C main and records its initialization token. Its destructor runs once after
main. A priority-101 native finalizer verifies the destructor count and token.

Both tools reject all 128 deliberately incorrect layout compilations. One
control reorders fields. The other changes the required size. Errors must come
from the generated constant-expression guards. The negative checker accounts
for BASIC source-line mappings and each compiler's diagnostic wording.

The BASIC fixture has zero source-only strict lint findings. Each native worker
uses one automatically cleaned directory within an 8 MiB bound. Windows peaks
at 146,039 bytes and Linux at 45,897 bytes. The ledger peaks at 1,773,568 bytes.
All original diagnostic options remain enabled. The controls also explicitly
enable GCC's padding diagnostic. Protected installed products retain their hashes.

## Receipts

Artifacts reside under `build/c-emitter-forward-views-20261105`.

- `abi-observations-packet-v2-20261115.json.zlib` contains both fixture sources and all emitted C.
- `abi-observations-windows-v5-20261115.json.zlib` and `abi-observations-linux-v5-20261115.json.zlib` retain native values, diagnostics and rejected layouts.
- `abi-observations-verified-v2-20261115.json.zlib` binds both native workers and the canonical implementation.
- `abi-observations-ledger-v1-20261115.sqlite3` retains each exact C identity and diagnostic association, with its explicit controller transition.
- `abi-observations-resolved-v1-20261115.json.zlib` records nested members, private carriers and the destructor-only module.
- `abi-observations-templari-current-v1-20261115.json.zlib` closes the current 664-profile Templari ledger.
- `abi-observations-templari-source-cut-v1-20261115.json.zlib` archives the current opened inputs and source delta.

<!-- end of c-emitter-abi-observations.md -->
