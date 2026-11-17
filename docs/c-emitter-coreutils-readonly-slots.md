<!--
Project FreeBASIC C-emitter corpus qualification
File c-emitter-coreutils-readonly-slots.md
Purpose Record borrowed pointer-slot views in Coreutils callbacks.
Responsibilities Bind library types, regeneration, native values and diagnostics.
Whole-command application execution retains its independent qualification.
-->

# Coreutils borrowed pointer slots, November 17

Coreutils' 498 measured profiles reach GCC 156 / Clang 1,779. The repair removes
three qualifier observations from each native tool. Ten Coreutils qualifier
observations retain their original owners. SQLite remains GCC zero / Clang
7,693. Templari retains GCC 2 and the retained corpus GCC 14, bringing the
disjoint GCC census to 172.

## Library and regeneration changes

The `env_vector_slot_hash`, `env_vector_slot_compare` and `userid_compare`
helpers borrow pointer slots while reading the pointed-to characters. Their
local slot views now use `ZString Const Ptr Ptr`. Slot storage is read-only
through these views, and character storage keeps its existing mutability.
Function arguments, returns, pointer values and callback ABIs keep their
existing declarations.

Upstream `env` already spells the hash slot as `char *const *` and uses the
same qualified slot view in its comparison. Upstream `users` uses writable
slot casts in its comparator. The FreeBASIC port gives those comparator reads
the narrower borrowed-slot view. The current C2FB converter already retains
intermediate const where the C source supplies it and preserves explicit C
casts. [Upstream env helpers](https://github.com/coreutils/coreutils/blob/master/src/env.c), [upstream users comparator](https://github.com/coreutils/coreutils/blob/master/src/users.c)

The port's existing source-profile mechanism retains the three read-only
views during regeneration. Its profile is idempotent and validates the expected
function bodies and slot-view counts. Five registered profile tests cover
source selection, complete regenerated bodies, repeated application, stale
shapes and the normal regeneration entry with both context modes. Six existing
obstack profile tests continue to pass.

The incremental five-file port patch applies to the exact recorded preimages,
produces identical repaired bytes and passes reverse checking. Those preimages
include the preceding execvp, obstack and C2FB test-registration repairs.
Publication of the accumulated port patches remains open.

## Complete-module diagnostics

Both C producers emit both revisions of the two owning modules. GCC 14.2 and
Clang 19.1.7 compile sixteen complete native contexts at `-O0` and `-O2`.
Every before/after object pair is byte-identical, giving eight exact native
object comparisons. Removing only the added slot qualifications reconstructs
the complete preceding BASIC sources.

The original GCC C identities, diagnostic maps, native flag sets and optimized
objects match the preceding complete census. Four fresh current native
contexts replace the two changed modules. The other 496 profiles retain their
992 qualified native results. Both affected complete modules are GCC-clean.
Clang retains each other diagnostic class.

The first replay omitted `env`'s existing per-module `-mno-red-zone` option.
Comparison with the original object and command caught that omission. The
accepted v2 replay restores the recorded per-module flags for both native
tools and passes the original-object checks.

## Callback values and frontend controls

The public BASIC fixture includes complete production bodies for the three
callbacks, `entry_has_name` and the captured `hash_pjw_bare` provider. The
native observer checks actual string comparisons, environment-name equality,
hash results, slot identity and character bytes. It also calls the production
comparator through native `qsort`. Its input matrix includes empty names,
high-bit character bytes, changed values and seven table sizes.

Sixteen Linux LP64 executions cover both revisions, both C producers, GCC,
Clang and both optimization levels. They pass 278,544 cases and 821,248 checks,
including 262,144 direct comparator cases and 14,336 hash cases. This scope
matches the captured LP64 hash provider and production port census.

The C observer is GCC-clean. Its Clang builds retain 38
`-Wunsafe-buffer-usage` observations on the bounded array and pointer operations.
These fixture diagnostics remain visible in the execution receipts.

All 128 FreeBASIC assignment controls pass their expected outcomes across
Windows/Linux, both pointer widths and both C producers. Sixteen slot-write
controls report error 119, `Cannot modify a constant`. Pointer-view assignment,
slot reads and character writes retain their previous acceptance.

Strict source-only lint passes both changed production files and the BASIC
fixture with zero errors. The production files retain their nine existing
advisories, and the fixture has zero findings. The largest complete-module
scratch footprint is 289,673 bytes. Callback execution peaks at 53,612 bytes.
All worker scratch directories are automatically removed.

## Reproduction and receipts

- `build_scripts/test-coreutils-readonly-slots.py` compiles and executes an
  emitted fixture and native observer under a supplied aggressive flag list.
- `tests/compiler-support/coreutils-readonly-slots.bas` exports the actual
  included production callbacks to the native observer.
- `tests/compiler-support/coreutils-readonly-slots.c` checks values, identity,
  unchanged characters and the native sorting entry.

The following receipts reside under `build/c-emitter-forward-views-20261105`.

- `coreutils-slot-packet-v1-20261117.json.zlib`
- `coreutils-slot-native-v2-20261117.json.zlib`
- `coreutils-slot-census-v1-20261117.json.zlib`
- `coreutils-slot-vectors-packet-v1-20261117.json.zlib`
- `coreutils-slot-vectors-linux-v1-20261117.json.zlib`
- `coreutils-slot-proof-v3-20261117.json.zlib`
- `coreutils-slot-lint-v1-20261117.json.zlib`
- `coreutils-slot-delivery-v1-20261117.json.zlib`
- `coreutils-readonly-slots-port-v1-20261117.patch`
- `coreutils-slot-qualification-v1-20261117.json.zlib`

The remaining compiler goal retains the two finite branch-join format cases,
other GCC owners, broader Clang classes, native platform checks, full C2FB
integration and whole-application execution.

<!-- end of c-emitter-coreutils-readonly-slots.md -->
