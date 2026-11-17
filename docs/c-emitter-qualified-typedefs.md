<!--
Project: FreeBASIC C-backend verification
File: c-emitter-qualified-typedefs.md
Purpose: Record C2FB pointer typedef repairs and signed character values.
Responsibilities: Bind source models, normal CLI regressions and native evidence.
Whole-application and broader corpus qualification retain separate evidence.
-->

# Qualified pointer typedefs and character values

The C2FB translator now preserves qualifiers applied to pointer typedef names,
including their use through nested aliases. Character reads through those slots
also retain the mapped signed-byte value. The selected FreeBASIC development
compiler is `9c1ebb28`. The qualified Windows converter is
`build/c-emitter-forward-views-20261105/c2fb-qualified-typedef-after-v4-20261114.exe`.

The complete Win64 converter produces identical C through the GCC and Clang
producers. That native C input compiles with zero GCC warnings and 158 Clang
observations under the saved aggressive options. Clang reports 156 padding
and two startup-registration observations. Both resulting native tool builds
execute the normal raw-source CLI and pass its fifteen-function regression.

## Shared type model

Previously, `PointerAlias const *`, with the alias defined as `char *`, became
`Const ZString Ptr Ptr`. The qualifier belongs to the pointer slot. Its correct
flattened type is `ZString Const Ptr Ptr`. Qualifiers stored on pointer typedefs
were also lost when later aliases added pointer levels.

The resolved model and body alias table now retain a pointer-const mask.
Flattening shifts explicit declarator qualifiers above the typedef's own levels
and joins the saved qualifier masks. The base object's const remains separate.
Duplicate typedef checks include the mask. Header typedef emission retains
outermost pointer const so that subsequent alias uses receive the correct type.

Pointer peeling and depth calculations now account for const between pointer
levels. This allows valid writes to character data reached through a const
pointer slot. Its pointer value retains its readonly storage contract.

## Character values and diagnostics

Native comparisons exposed a second fault. A qualified ZString pointer bypassed
the translator's byte view. Reading the byte 0x80 produced 128 where the native
signed-character result was -128. The shared character-pointer conversion now
recognizes both ordinary and const-qualified pointer slots, retaining const
on the character data where required.

The permanent `tests/test_pointer_const.py` regression remains in C2FB's normal
Make test target. It covers nine header declarations and fifteen translated
bodies, including alias chains, nested const, conditional callback calls and
valid character writes. It also requires byte views in the generated character
accesses. The original translator fails its qualifier controls. The intermediate
typedef repair fails the signed-byte control. The complete repair passes both.

Six negative source checks retain FBC error 119 for writes to const pointer
slots through both C producers. Two native runs reproduce the intermediate
signed-byte failure at 32 and 64 bits. The existing FBC source warnings remain
available.

## Native qualification

Thirty-two repaired native executions cover Windows/Linux, 32/64-bit targets,
both C producers, GCC/Clang and O0/O2. They pass 4,800 comparisons against the
actual original C bodies and 1,056 native signature assertions. Inputs include
-128, -1, zero, one, 65 and 127. Mutable character writes and original pointer
storage also pass independent checks. All 64 repaired generated body/header
module compilations have zero GCC and Clang warnings.

Thirty-two old-type controls fail native assertions for the original misplaced
qualifiers. Both native Linux preprocessing tools pass the complete registered
header/body regression on the current GAS64 converter. The Windows and Linux
converters produce matching generated statements, with their source-path
annotations normalized explicitly. Current normal-CLI outputs are tied to the
qualified native-value inputs through exact C identities.

The complete tool audit found one additional GCC type-limits warning in
`EvaluateGenericBodyArrayBound`. Its Long parameter already has the maximum
representable static-dimension bound. The redundant upper comparison is removed.
The lower bound and evaluated-value limit checks remain active. FBC warning 56
diagnosed the original comparison. Ten other range warnings, eight unread
parameter warnings and five unread-variable warnings retain their source
diagnostics in the converter build.

Strict source-only lint compares seven changed BASIC modules with their exact
preimages. It retains four existing advisories and adds zero findings.

## Reproduction and remaining work

Receipts live under `build/c-emitter-forward-views-20261105`.

| Evidence | File |
| --- | --- |
| Original qualifier shapes | `qualified-typedef-pilot-v1-20261114.json.zlib` |
| Current Windows and GAS64 build | `qualified-typedef-build-v4-20261114.json.zlib` |
| Complete tool C profiles | `qualified-typedef-converter-C-v2-20261114.json.zlib` |
| Native GCC/Clang tool CLI runs | `qualified-typedef-converter-cli-v1-20261114.json.zlib` |
| Normal Linux header/body regressions | `qualified-typedef-native-result-v5-20261114.json.zlib` |
| Windows native signatures and values | `qualified-typedef-values-windows-v2-20261114.json.zlib` |
| Linux native signatures and values | `qualified-typedef-values-linux-v1-20261114.json.zlib` |
| FBC diagnostics and signed-byte controls | `qualified-typedef-source-controls-v2-20261114.json.zlib` |
| Before/after source lint | `qualified-typedef-lint-v3-20261114.json.zlib` |
| Current normal-CLI C identities | `qualified-typedef-current-identities-v1-20261114.json.zlib` |
| Portable source overlay | `qualified-typedef-source-cut-v1-20261114.json.zlib` |
| Final verification | `qualified-typedef-final-v1-20261114.json.zlib` |

The source overlay records the coreutils Git base and preceding source deliveries.
It includes the shared model, resolver, expression helpers, alias storage,
regression and documentation changes. Qualification uses one owned temporary
directory per job and serial native work on the two-core compile guest.

Automatic approval review rejected removal of twelve superseded converter
binaries and transfer copies as blocked by policy. Their 20,872,660 bytes remain
recorded in `qualified-typedef-retired-v1-20261114.json`. The final receipt records
this cleanup limit. Temporary native modules and objects were removed by their
completed job controllers.

The last qualified production graphs retain coreutils GCC 197 / Clang 1,841 and
SQLite GCC 23 / Clang 7,696. The subsequent
[enum-width repair](c-emitter-enum-widths.md) refreshes one version unit and
verifies all other opened-input hashes, retaining those totals.
Full regeneration from the original coreutils C tree remains pending. Windows
header-command quoting, the full Linux C-backend tool link, outermost body-storage
qualification and qualified function-pointer alias limits retain separate open
checks. The wider corpus, parser stack and semantic-linter consumer work remains
active. Templari retains its separately qualified two GCC observations and
pending full Clang census.

<!-- end of c-emitter-qualified-typedefs.md -->
