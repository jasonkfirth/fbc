<!--
Project: FreeBASIC compiler qualification
File: c-emitter-thread-context.md
Purpose: Record contextual THREAD and existing example build-context repairs.
Responsibilities: Separate verified coverage from whole-queue warning totals.
This file intentionally does NOT claim private ownership promotion or deployment.
-->

# Contextual THREAD and example build contexts, November 5

The compiler again accepts ordinary identifiers named `Thread`, including
procedures, namespaces, imported names and overloads. Keyword recognition
retains the token used by `THREAD LOCAL`; ordinary symbol lookup skips that
keyword entry. Actual duplicate declarations and explicit reservations still
fail. The repair is confined to `src/compiler/symbols/symb.bas`.

No existing example was edited. All 24 affected examples pass their checked
and default profiles under both canonical and private candidates. The
reusable audit also supplies the existing variant/SDL3 include owners, the
variant makefile's `VARIANT_ISBUILDING` definition and module mode, the option
example's `TEST` definition, and nonmain mode for the winmm helper. These
settings recover another 102 examples, or 204 profiles per candidate.

| Newly covered profiles per candidate | Canonical GCC / Clang | Private GCC / Clang |
| --- | ---: | ---: |
| 48 contextual Thread profiles | 253 / 259 | 0 / 2 |
| 204 corrected build-context profiles | 0 / 4,440 | 0 / 4,440 |

These are newly exposed diagnostic observations. The private Thread pair is
padding. The context observations comprise 4,318 padding, 118 global startup
and four reserved-identifier warnings. They remain unsuppressed. No record
layout, initialization order or example behavior was changed to lower counts.
The canonical Thread observations include 249 GNU-label warnings per tool;
the private candidate's earlier lowering work already removes that class.

There are 1,008 successful GCC/Clang object checks across the two candidates
and 252 recovered profiles each. The corrected context receipts reuse 88
already passing profiles and freshly check 116 variant-library profiles per
candidate. The reusable harness then freshly re-emits all 204 profiles per
candidate and compares their exact C hashes to those qualified objects.

The canonical candidate is `6f6a303989de9408e23f89ed0c1ce450bf06edee16390e0333d671d7e01cbc79`;
the private candidate is `c346b57f1afc78498b978968dd4cbce037e49879ffe3691f80dc7e20101de7b6`.
Each retains 224 compiler C fixed points and 0 GCC / 1,652 Clang observations.
Fresh emission from the promoted root symbol source matches all 224 qualified
C identities per candidate, and the rebuilt binaries match the tested bytes.

Qualification also includes:

- 32 fresh executions across both C producers, GCC/Clang, GNU11/GNU23 and
  O0/O2, each with two live workers and independently initialized TLS;
- 44 successful identifier-role executions and 24 expected frontend refusals;
- four backend suites, including the new permanent procedure, namespace and
  import regressions through C and GAS64, and existing mixed-header/LTO checks;
- 14 private semantic checks, the compiler structure guard, and strict Windows
  lint of 54 source/generated files with zero findings.

Initial prototype namespace/import failures were compiler defects and led to
the shared lookup repair. A negative harness expected the wrong `Len` message;
the compiler correctly rejected redefinition of the runtime command. An early
root binding accidentally gave canonical headers precedence over private
headers; the accepted binding preserves the original include order. A harness
C comparison initially changed the nonmain output basename, which changes its
initializer name; repeating it with the original basename restores exact C
identity. Earlier incomplete variant contexts are retained as failed attempts.

Both components use reusable scratch files, at most two heavy workers, a
512 MiB free-space floor and 32 MiB retained component limits. Closed audit
artifacts were compressed with byte hashes checked before and after. Obsolete
trial binaries were retired only after recording their exact hashes, and
identical owned candidates share immutable hard links. Installed compiler and
runtime archives remain unchanged; source preimages and unrelated dirty work
are preserved.

Evidence is under `build/c-emitter-thread-context-20261105` and
`build/c-emitter-example-contexts-20261105`, with owned compiler scratch on E:.
The [last complete paired census](c-emitter-current-queues.md) remains the
whole-body reference. This batch adds verified coverage; it does not refresh
every existing profile against these new compiler cuts or finish the campaign.
The 20 remaining frontend profiles per cut retain their explicit C-backend,
foreign-language, negative-example or other-target classifications. The two
DOS-only C-object failures, PNG exclusions, concurrent Optical exclusion and
private ownership promotion boundary remain explicit.

<!-- end of c-emitter-thread-context.md -->
