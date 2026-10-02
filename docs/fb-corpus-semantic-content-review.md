<!--
Project: FreeBASIC compiler verification
File: fb-corpus-semantic-content-review.md
Purpose: Compare real BASIC constructs with the information exported for them.
Responsibilities: Record source questions, supporting facts, omissions, and priorities.
This file intentionally does NOT equate a valid sidecar with complete program semantics.
-->

# Source-to-export content review

The follow-up [exporter implementation](semantic-sidecar-improvements.md)
addresses causal expression/node links, binding access roles, optional argument
origin, file statement operations, and explicit capability reporting. The
observations below identify the frozen schema 23 state that motivated it.

Reviewed October 2, 2026. This supplements the
[corpus build and export audit](fb-corpus-semantic-audit.md) with manual inspection
of the BASIC and its actual exported facts.

The export supplies useful symbol, type, dispatch, argument, layout, macro, and
lowered-operation facts. It still lacks several connections needed for source
diagnostics, source control flow, and lifetime analysis. A successful `END`
certifies a completed artifact under the current contract; it does not certify
those missing capabilities.

## Reviewed inputs

The review uses the same frozen, GCC-qualified corpus source as the preceding
audit, with a separately frozen schema 23 compiler and reader. This avoids
reporting newly implemented macro and coordinate information as still absent.
Eight real source invocations pass disabled/full/expression export checks with
this newer compiler. Four small source reductions also pass those checks.
Those checks establish that the inspected artifacts are valid and preserve
emission; the judgments below come from comparing source meaning with records.

| Source | Constructs inspected | Review question |
| --- | --- | --- |
| fb-ext-lib memory driver | `MFput`, `MFclose`, `newMemoryFileDriver` | Are unsigned bounds arithmetic, indirect cleanup, and callback table assignments represented? |
| BASICVERSE renderer | `rt_worker`, `render_init` | Can worker entry, lock arguments, repeated unlocks, defaults, and the guards around writes be identified? |
| fbJson | `ContainsKey`, `JsonItem.Value` getter/setter | Are property dispatch, field uses, and constant case selection preserved? |
| fb-ext-lib socket receive worker | `socket.recv_proc` | Can scope guards and their cleanup on both `CONTINUE DO` paths be recovered? |
| fb-ext-lib mutex wrapper | `Mutex.lock`, `Mutex.unlock`, constructors/destructor | Do macro-generated types retain identity and call arguments? |
| ToyBASIC | `OpenFile`, `BuildAstFromTokens` | Are file operations and heap-backed tree assignments connected to source constructs? |
| OHRRPGCE pathfinder | `calculate`, `set_result_path`, `getnode` | Are dynamic arrays, BYREF results, vector macros, status guards, and field mutation distinguishable? |
| PCEM_FB486 memory code | `mem_updatecache`, memory read/write helpers | Are case values, label targets, pointer access, and integer widths preserved? |

## Facts that are already useful

### Unsigned arithmetic is retained well enough to expose a bounds risk

`libext-file-memorydriver.bas:119-121` contains:

```basic
if (x->l + n) <= x->dlen then
    memcpy(@x->d[x->l], p, n)
    x->l += n
```

In the inspected model, E294 represents `x->l + n` as builtin `add` with data
type 9, `uinteger`. Y9 identifies an eight-byte unsigned integer on this target.
E297 retains the `less-or-equal` result, and the `L`, `DLEN`, and `D` field
identities and offsets are present in T records. This is enough to identify
an unsigned-addition overflow candidate before the bounds comparison. The
export should not invent a guarantee that this expression cannot overflow.

The remaining obstacle is associating that exact source guard with the
lowered memory transfer and cursor update. The source and lowered facts exist
in separate graphs without a compiler-provided connection between them.

### Argument-to-formal identity is present

N records for argument nodes already put the selected formal's symbol ID in
their symbol field. In `MFput`, the indirect `fsseek` call therefore retains
its signature and exact formal identities. In BASICVERSE, the `THREADCREATE`
call retains its three lowered formals and the address of `rt_worker`, even
though the source writes two arguments.

This information must not be counted as missing merely because there is no
separate `argument-formal` H row. `passing-mode=default` means use the formal's
passing convention. It does not mean that the source omitted that argument.

### Member and property identity survive lowering

FIELD nodes retain the field symbol and their receiver subtree. The memory
driver's FF callback remains an indirect call through its declared procedure
pointer signature, with its receiver field preserved. No concrete cleanup
routine is guessed for a callback supplied by the caller.

fbJson's VALUE procedures have distinct F kinds, `property-set` and
`property-get`, distinct signatures, and exact selected targets at call nodes.
The SETMALFORMED calls and `_DATATYPE`/`_VALUE` field identities are also present.
This supports method/type inspection and selected-call analysis.

### Case values and resolved branch targets are available

PCem's `mem_updatecache` selects cases 0 through 4. The lowered jump-table node
retains its selector, bias/span, J value-to-label rows, and default target.
fbJson's constant character dispatch similarly retains its normalized values
and compiler-selected labels. The absence of source CASE construct records
does not mean that all lowered dispatch information is lost.

### Scope-exit destructor selection is available

The socket receive worker declares a `socket_lock` at line 44. Its constructor
locks the mutex, and its destructor unlocks it. The model has I records for
construction, ordinary destruction, and the same variable's
`scope-exit-destructor` at the `CONTINUE` tokens on lines 65 and 72.
It distinguishes the second lock variable at line 111.

These are concrete compiler-selected cleanup facts. What remains missing is
an explicit source lifetime/action graph that connects construction, guarded
exits, destruction order, and object instance ownership. A consumer can inspect
lowered branches and calls; the exporter does not yet expose that source graph.

### Macro provenance has improved

OHR's `v_new`, `v_resize`, `v_heappush`, and `v_free` expansions now have actual
definition, invocation, argument, substitution, replacement, and expression
origin records. The generated mutex template also preserves its actual macro
arguments and resulting type identities. These are improvements over the
schema 20 corpus audit.

However, the inspected lowered nodes have no MR links to expansion attempts.
The graph reaches typed expressions and selected symbols, then stops before
the corresponding lowered operations. This is another concrete use case for
source-to-lowering links.

## Reproduced information gaps

| Priority | Gap | Real consequence | Existing work items |
| --- | --- | --- | --- |
| High | No source-expression/statement-to-lowered-node links | A source guard cannot be directly joined to the writes, calls, or exits it controls. | G10, G11, G34 |
| High | No precise source statement or compound ownership | Multiple operations, branches, and generated actions on one line cannot be identified as source constructs. | G10 |
| High | Binding use roles remain declaration/reference | Writes, reads, compound updates, address escape, and callee use are not described at their written occurrences. | G15 |
| Medium | No per-call omitted/default argument origin | A supplied value and the same compiler-inserted default have identical argument metadata. | G31 |
| High | Builtin statements lack source operation events | OPEN, LINE INPUT, and CLOSE survive as runtime calls, without their own written operation/range. | G12 |
| High | Source lifetime and execution relationships remain implicit | Scope-exit selections exist, but guarded action ownership and order are not explicit source facts. | G33, G34 |
| High | Completion lacks a semantic capability inventory | Consumers cannot discover which semantic relationships are complete from END alone. | G40 |

These are information requirements, not a request to manufacture a runtime
callee, external library behavior, alias target, or value the compiler does not
know. For example, mutex/condition targets and arguments are retained; the
operating system's synchronization contract still belongs to the runtime/API
model used by the consumer.

### A source guard and its AST have no explicit join

BASICVERSE's `rt_worker` has 237 lowered nodes, all with source line zero under
its ordinary non-debug build. `render_init` has 75 such nodes. Typed expressions
and bindings do have physical LOC ranges, but neither procedure has node LOC
ranges or expression-to-node H relations.

The memory driver and OHR use `-g` in their normal recipes and retain line
markers. Those markers still do not identify which of several operations on a
line produced a node. Across all eight reviewed models there are zero
expression/node H links and zero node LOC records. Source overlap is not a
substitute for a compiler-selected relationship.

### Omitted and explicitly supplied defaults are indistinguishable

The reduction declares `OptionalValue(value as long = 7)` and calls it from
two procedures, once as `OptionalValue()` and once as `OptionalValue(7)`.
Both exports have:

- the same selected callee and formal symbol;
- `argument-count=1`;
- argument `passing-mode=default` and `bytes=0`;
- a child C value of signed 7.

F/G say that the formal is optional. They do not say which call omitted it.
The compiler knows this in `astNewARG` before cloning the optional initializer.
A useful addition is an argument-origin/defaulted flag captured there, plus
source actual-argument identity when available. This is needed to report API
default changes without attributing them to explicit arguments.

### Source typing is not proof of execution

The short-circuit reduction contains:

```basic
if 1 orelse UnevaluatedValue() then print 1
if 0 andalso UnevaluatedValue() then print 2
```

Both calls have selected, typed E records. Neither remains as a call N after
constant short-circuit folding. That is correct stage behavior. What is absent
is an explicit retained/erased/conditional lowering relationship for those
source expressions. A call graph built from E or B alone would fabricate two
runtime calls.

### Four distinct uses are all generic references

The access-role reduction writes `value = 1`, updates `value += 2`, reads it in
PRINT, and passes it to a BYREF formal. All four source B rows are `reference`.
The AST distinguishes assignment from argument passing, but there is no
source binding-to-node relationship to carry those roles back to the occurrences.
BYREF itself must not be treated as proof that an unknown callee writes.

### File statements become unanchored runtime calls

ToyBASIC's `OpenFile` exports FREEFILE, FB_FILEOPEN, FB_FILELINEINPUT,
FB_FILECLOSE, and helper calls. Under its normal build all 376 nodes have source
line zero. The small file-statement reduction exports the same runtime calls
and no O rows for OPEN, LINE INPUT, or CLOSE.

Lowered formal values remain inspectable, but the model does not identify
the source operation, its complete statement extent, or the distinction
between a written operation and supporting runtime work.

## Source risks used to evaluate sufficiency

The memory-driver addition above is an overflow candidate because unsigned
wrap can invalidate an addition-based bounds check. Its numeric types are
exported well; attaching a path-aware diagnostic to the guarded transfer needs
the missing source/control relationships.

OHR's pathfinder skips nodes whose status is OPENED at line 95 and later tests
for OPENED again inside its reparenting branch at line 119. This merits source
review, but it is not proved unreachable from these artifacts: intervening
calls may have effects on aliased state, and their bodies/effect summaries are
not all present in this translation unit. The export preserves the field and
enum identities and the BYREF `getnode` result. Stronger proof requires flow,
alias, and effect information without assuming external calls are pure.

No corpus program was patched as part of this review.

## Follow-up and regression coverage

Implement source construct/occurrence identities and links to typed expressions
and lowered nodes first. That supplies the missing association needed by
source access roles, builtin operation events, and conditional action ownership.
Capture call argument origin at the existing argument construction decision.
Publish explicit capability/phase coverage so consumers can reject analyses
whose required relationships are unavailable.

The new `sufficiency.bas` contract regression passes on GCC, LLVM, and Clang.
It checks exact actual-to-formal identities, optional signature metadata,
default values, and the distinction between typed source calls and retained
runtime calls. It preserves useful current behavior without asserting that
the missing relationships must remain absent.

Evidence is under `out/fb-corpus-content-review-20261002/`:

- frozen schema 23 compiler/reader identities and the prior GCC-qualified plan;
- `latest-models/` for eight real invocations in all three export modes;
- procedure/source views and `question-facts.json` with actual identities;
- `reductions/` and `reduction-results/` for the four reproductions;
- `sufficiency-contract.log` for the maintained regression.

The reviewed compiler SHA-256 is
`116fd4152d70da5d8ccae0f453487d3986caf4e077091aa21f2e3cb8fc8e9d32`.
This is a focused manual content review of the listed constructs. It does not
claim exhaustive inspection of every function in the 389-unit corpus.

<!-- end of fb-corpus-semantic-content-review.md -->
