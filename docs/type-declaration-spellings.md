<!--
Project: FreeBASIC compiler tooling
File: type-declaration-spellings.md
Purpose: Describe the optional original type-declaration spelling contract.
Responsibilities: Record producer hooks, wire layout, compatibility and verification.
This file intentionally does NOT specify naming conventions or automatic source edits.
-->

# Original type declaration spellings

Schema 27 full models advertise `CAP <module> type-declaration-spellings available`.
Bindings/expression-only modes advertise it as unavailable. This optional extension
does not change the schema number, canonical `S`/`T` names, name lookup, grammar,
visibility, mangling, emitted program or runtime interfaces.

The type, typedef and enum parsers retain the accepted identifier before its
temporary buffer is reused. This supplies original case for named types/unions,
aliases and enums, and actual enum members. It includes macro replacement,
argument and nested expansion names. An ordinary typed `Const` alias is not an
enum-member declaration. Anonymous generated aggregate names are not invented
as written identifiers.

Each occurrence has a unique detail role:

```
K   symbol   <symbol-id>   type-declaration-spelling:<detail-id>   <payload>
```

The decoded payload contains four tab-separated fields, in this order:

| Field | Meaning |
| --- | --- |
| 0 | Native parsed statement identity |
| 1 | Active physical source-context identity |
| 2 | `type`, `union`, `alias`, `enum` or `enum-member` |
| 3 | Accepted original ASCII identifier spelling, at most `FB_MAXNAMELEN` |

Normal transport escaping applies to the entire payload. It is not a second
round of BASIC tokenization. The role is attached to `LOC source-context
<source-id> <role>` and, for generated tokens, `MR symbol <symbol-id>
<expansion-id> <role>`. A macro invocation is a reporting anchor, not an editable
span of the generated identifier. Identical alias redeclarations retain separate
occurrences even when they share one canonical symbol.

The independent reader in `tests/semantic-sidecar/semantic_type_spellings.py`
validates class, original spelling versus lookup identity, statement/source/module
ownership, actual enum membership, availability, locations and normal expansion
ancestry. Every native type declaration binding and generated declaration origin
must have a matching occurrence when the capability is available. Missing records
cannot be interpreted as clean names. Consumers needing presentation advice also
compare physical original case against current revision-verified source bytes.

Existing clients may ignore this optional property. New clients accept older
models without it, but a requested original-spelling operation must report
unavailable analysis rather than substitute uppercase lookup names. These are
type-family facts, not original procedure, field or formal spelling coverage.

The first consumer is fblint FBL-NAME-011 in `C:\fblint`. Its audit checkpoint
`.159` records the source-read-qualified private compiler image, independent
native models, damaged-record refusals, unchanged emissions, frontend dialect and
encoding checks, and the separately preserved installed tools. Linux checks are
frontend/backend emission checks, not native Linux execution or CI qualification.

The `.159` independent native gate validates 11 full models with 44 original
declaration occurrences each, including five macro-generated names. Eight damaged
contracts per model are refused, and all 11 full/off output pairs are identical.
The source-qualified private compiler image has SHA-256
`B46DA414FE790B8E6D4D6F99C5428BA5F10463161D7F5A13E4CBC5C13F586A12`.
The consumer's 51 focused cases cover all kinds, API visibility, macros, physical
revision/case checks, marked encodings and selected unavailable-analysis failure.
These receipts do not certify an installed replacement or later concurrent edits.

<!-- end of type-declaration-spellings.md -->
