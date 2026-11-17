<!--
Project: FreeBASIC C backend verification
File: c-emitter-unix-driver-contracts.md
Purpose: Record Unix driver callback, declaration and focus-lifetime repairs.
Responsibilities: Explain source contracts, native evidence and remaining work.
Live desktop rendering retains its platform application qualification.
-->

# Unix driver contracts, November 15

The complete Linux MT runtime and startup build reaches GCC 52 / Clang 10,064.
This cohort removes twenty GCC and fourteen net Clang observations. The
last-qualified disjoint GCC total is 255. All 1,036 native object profiles build.
Only hinit, io_inkey and io_xfocus change object identity under either tool.

## Callback and declaration ownership

The runtime retains the signatures required by pthread entry points, signal
handlers, shutdown callbacks and platform-independent console interfaces.
Thirteen intentionally unused parameters are marked as consumed in their
implementations. Unsupported page, mouse-position and ordinal-symbol requests
retain their established return values. The complete runtime has zero GCC or
Clang unused-parameter observations.

`fb_private_xfocus.h` owns the shared focus declarations. Both Unix and Solaris
private console headers include that contract, and the implementation includes
it before its definitions. The complete runtime has zero missing-prototype
observations. Solaris execution retains its independent platform evidence.

## Focus lifetime

The focus module uses Xlib's actual `const char *` display-name signature.
The native regression checks its function-pointer type against the installed
Xlib declaration.

Each keyboard or mouse initialization attempt retains a matching release under
the console background lock. Failed attempts keep that balancing reference.
A later attempt retries loading or opening the connection. Queries return false
while the connection is unavailable. Failed display opens release the library.
The last release closes an opened display once, clears the display/function
state and unloads the library. Repeated release after shutdown is harmless.

Eight GCC/Clang, 32/64-bit, O0/O2 contexts execute nine groups each. Their
72 executions pass 154,856 assertions covering missing libraries, failed display
opens, recovery, shared ownership, repeated teardown and no-X builds. Two original
source contexts reproduce twelve failing focus controls. Their six no-X and
termcap controls pass. The current tests also exercise unsupported console and
ordinal-symbol entry points across signed integer limits.

## Legacy termcap arguments

`fb_private_termcap.h` copies each two-character capability ID into three local
bytes including its terminator. This supplies writable arguments to legacy
termcap declarations and preserves the caller's literal/table storage. The
returned sequence and area pointer retain the external API's ownership.

Both const and mutable prototype variants run in every native context. The
mutable variant deliberately changes its borrowed ID and the caller's bytes
remain intact. The production input audit checks all seventeen sequence IDs,
twenty-four key IDs and the two direct lookup literals. Every ID has two
characters. These checks qualify the four new Clang buffer-use observations
at the adapter's indexed reads. Those observations remain counted.

## Reproduction and receipts

The normal component entry point uses the current production sources and
installed native Xlib headers. Its deterministic callbacks exercise failure
and recovery without requiring a desktop session.

```sh
python3 -B build_scripts/test-unix-driver-warning-fixes.py \
  --cc /usr/bin/gcc --bits 64 --optimization 2 \
  --receipt /path/to/new-driver-receipt.json
```

Repeat for both native tools, word sizes and optimization levels 0 and 2.
`--native-flags-json` accepts the full unsuppressed audit flag array. The runner
verifies input hashes, disables core dumps and removes its temporary products.

The complete worker reruns the preceding 92 current runtime executions against
the rebuilt archives. Together with the new groups, 164 current Linux runtime
executions pass. It uses at most 18,454,342 scratch bytes and removes its tree.
Receipts are under `build/c-emitter-forward-views-20261105`.

- `runtime-current-packet-v31-20261115.json.zlib` pins 1,330 runtime/Make inputs,
  the original sources and normal regression inputs.
- `runtime-current-linux-v27-20261115.json.zlib` records both full builds,
  current executions and original failing controls.
- `unix-driver-contract-qualification-v1-20261115.json.zlib` binds the source,
  native receipts, documentation and protected-product checks.

## Remaining work

GCC runtime classes comprise twenty padding observations, thirteen conversions,
five sign conversions, five switch-default, six discarded qualifiers, two
cast-qual and one redundant declaration. The shared keyboard X11 function-pointer
declarations still need review against Xlib, alongside the remaining scan-code
conversions. Seventeen dynamic-format runtime contracts, four finite-format
compiler sites and other Clang source/ABI owners retain their cleanup scope.
Templari's newer concurrent source/recipe edits await replay against its last
qualified 674-profile cut. Other project totals retain their preceding source
and native contexts.

<!-- end of c-emitter-unix-driver-contracts.md -->
