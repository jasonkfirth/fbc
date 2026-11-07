<!--
Project: FreeBASIC C backend qualification
File: c-emitter-native-owners.md
Purpose: Record native activation identity needed by the C continuation replacement.
Responsibilities: Explain registration, restoration, retirement and current evidence.
This file intentionally does NOT claim complete C continuation lowering or promotion.
-->

# Native handler activation ownership, November 3, 2026

The single-frame native bridge could not retain an earlier handler's owner
when a nested native procedure restored it. The current private runtime owns
separate native activation records. Each selected label has an opaque saved
identity pointing to its owner and actual executable target. Recursive uses
of the same label therefore retain different owner frames.

The public Win64 setter keeps its symbol and one-argument call layout. Its
leaf wrapper captures the incoming GAS frame base before a C prologue runs.
Saved identities are restored without assigning the restorer's frame to the
old handler. The throw entry point unwraps a native identity into executable
code; the C worker separately obtains its registered native owner frame.
`FB_ERRORCTX` and `FB_ERROR_FRAME` retain their existing layouts.

The private compiler appends one intrinsic index and uses a new procedure
status bit, without changing symbol or AST layouts. Ordinary Win64 x86-64 GAS
epilogues retire native registrations after local handler restoration and
before their frame expires. This includes early returns. A registration can be
bypassed, so retirement accepts an activation with no records. It rejects
out-of-order retirement and an owner still referenced by live C error state.
The checked compiler hook terminates on such a failed lifetime contract.

Runtime metadata follows live activations. Repeated selections in one frame
reuse the matching registration; normal exit releases its owner and handles.
TLS teardown releases owned records without inspecting expired borrowed
frames. The unchanged original GAS main object still passes all sixteen
worker/tool/standard/optimization cases. Its process lifetime is covered by
TLS teardown; automatic retirement of arbitrary older native objects is not
claimed.

The current private compiler is `b9614799edbbc14fbd971fb97bc9c9880fcae4c7e1d5b9ec79b0f787cdbfe483`.
All 224 selected compiler modules pass the unsuppressed GCC/Clang profiles
and their emitted-C self-hosting fixed point. Exactly 221 C-identical objects
are reused from the preceding owner graph. Compiler totals remain zero GCC
and 1,661 Clang, including the nine previously recorded missing-noreturn
observations on terminating unsupported-resource callbacks.

Qualification:

- Eight native-owner API executions cover recursive label identities,
  restoration, same-frame reselection, C callback interoperation, retirement
  refusal while referenced and 10,000 bounded activation cycles per execution.
- Sixteen allocation-refusal executions terminate with code four and balanced
  observed allocation/free accounting for both owner and handler refusal.
- Eight existing resume API executions retain retry/next, 128 nested records,
  argument checks and TLS retirement.
- Thirty-two mixed native programs pass: sixteen unchanged-root cases and
  sixteen compiler-retired recursive/conditional owner cases. The latter
  execute ninety handled errors per program and return to zero live metadata
  after every outer cycle. No fixture-authored retirement call remains.
- Sixteen four-thread programs pass 8,192 handled errors. Each worker checks
  its own owner array and returns to zero live metadata. Result reads follow
  thread joins. The observer and worker completion slot are thread-local.
- The matching-prefix backend gate, seven procedure-state tests and structural
  gate pass without changing permanent tests or validators.
- Strict source-only lint of the four compiler overrides and three BASIC
  fixtures reports zero errors and zero warnings.

Both frozen C producers, GCC/Clang, GNU11/GNU23 and O0/O2 participate in the
native matrices. All frozen worker C compiles with zero warnings. Runtime,
SDK and harness diagnostics remain visible in the retained databases. No C
warning flag is disabled. Threaded tests use actual MT TLS and a matching
private `libfbmt.a`; only error/error-frame members and the leaf wrapper are
replaced in the copied archives. Installed binaries and the earlier prefixes
are unchanged.

The C worker is still frozen emitted C with recorded edits. Compiler lowering
of its data tokens and checkpoints remains open, as do pending native nested
error state, abandonment/retirement of failing C activations, managed and
partial resource lifetimes, naked/foreign callers and wider targets. These
checks prove Win64 resumable behavior; they do not establish foreign SEH/C++
cleanup. The candidate remains private.

Accepted stable queues remain 2,139 GCC / 8,408 Clang across 2,251 profiles.
No continuation-family warning reduction is credited yet. Thirty-two moving
Optical profiles remain pending; PNG and the user-closed intermittent-emission
investigation remain excluded.

The receipt is `build/c-emitter-native-owners-20261103/summary.json`. Earlier
failed fixture link/parser controls retain their databases and gzip source
preimages. The stage has a 16 MiB cap, reuses scratch objects, runs at most two
heavy workers and bounds native/C calls to ninety seconds.

<!-- end of c-emitter-native-owners.md -->
