<!--
Project: FreeBASIC C backend verification
File: c-emitter-keyboard-input.md
Purpose: Record X11 keyboard and Unix console input conversion repairs.
Responsibilities: Describe native API types, input spans and executable checks.
Physical desktop input retains its platform application qualification.
-->

# Keyboard and line-input contracts, November 15

Five runtime sources repair X11 function types, mapping lifetime and checked
input conversions used by the common runtime interfaces.

The complete Linux MT runtime and startup build reaches GCC 44 / Clang 10,056,
removing eight observations from each tool. All 1,036 native object profiles
build, with only io_multikey, scancodes_x11 and io_readstr changing object
identity under either native tool. The last-qualified disjoint GCC total is 247.

## X11 keyboard mapping

The shared header declares `XOpenDisplay` with a const display name and
`XQueryKeymap` with an int result and a 32-byte char output buffer. Native
compile-time assertions compare the loaded function types to the installed
Xlib declarations. The keyboard handler interprets each returned byte as
unsigned before testing its key bits. The Xlib specification describes the
same byte/bit ordering. [Xlib keyboard API](https://www.x.org/releases/X11R7.6/doc/libX11/specs/libX11/libX11.html)

Each scan-code table rebuild clears earlier entries. Keycode bounds are clipped
to the 256-entry native table before conversion to `KeyCode`. Missing mappings
remain unmapped. Returned mapping storage with a nonpositive element count is
released without reading an element. Successful storage is released after its
first keysym is inspected. The mapping table's scan-code values are qualified
within 0 through 127.

## Kernel input and console lines

The Linux keyboard handler retains `read` counts and loop indexes as `ssize_t`.
Press/release flags retain their 0/0x80 byte representation. Character bytes and
packed extended keys retain their two-byte queue representation. Failed kernel
mapping queries produce an unmapped result. The termcap key-description table
uses the same short code representation as its lookup nodes.

The Unix line reader checks destination/capacity before dispatch, bounds libc
capacities to `INT_MAX`, and accepts byte-valued keys for storage. Negative and
extended key-service results keep waiting for a byte-valued key. Editing,
backspace, ignored tabs, high-bit bytes and full buffers retain their established
behavior. The ordinary read dispatcher already applies the same capacity limit.

## Regression entry point

`tests/rtlib/x11-keycode-values.c` exercises every mapped symbol at every native
keycode, unavailable mappings, nonpositive mapping counts, clipped ranges and
stale entries. `linux-keyboard-values.c` covers all 256 raw keyboard bytes,
all 256 X11 key bits, failed kernel lookups, read errors and a full 128-byte read.
`unix-readstr-values.c` covers capacity boundaries, high-bit input, editing,
full buffers and negative/extended key results.

The fixtures execute the production implementations with deterministic callbacks.
The normal runner supports both actual native word sizes and optimization levels.

```sh
python3 -B build_scripts/test-unix-keyboard-warning-fixes.py \
  --cc /usr/bin/gcc --bits 64 --optimization 2 \
  --receipt /path/to/new-keyboard-receipt.json
```

Repeat with Clang and optimization levels 0 and 2. `--native-flags-json` accepts
the full audit flag array. The runner rejects suppression flags, checks source
hashes, disables core dumps and removes its temporary products.

The original-source control uses the original callback types with its test
callbacks. Current-source type assertions remain enabled in every current
native context. The initial full run identified a weak zero-count test case,
whose unmapped symbol also passed with the old implementation. The corrected
case supplies a known mapped symbol and requires the empty mapping to remain
unmapped.

## Qualified result and remaining work

Eight GCC/Clang, 32/64-bit, O0/O2 contexts pass 96 new executions with
1,597,824 assertions. Two original-source contexts reproduce twelve failing
controls and pass twelve positive controls. The full worker reruns its preceding
164 current runtime executions against both rebuilt archives. All 260 current
Linux runtime executions pass. Its scratch peaks at 19,085,751 bytes and the
temporary tree is removed.

Receipts are under `build/c-emitter-forward-views-20261105`.

- `runtime-current-packet-v34-20261115.json.zlib` pins all 1,330 runtime/Make
  inputs, original production sources and normal regression entry points.
- `runtime-current-linux-v29-20261115.json.zlib` contains both complete native
  builds, new and preceding current controls, and original failing controls.
- `keyboard-input-qualification-v1-20261115.json.zlib` binds the exact tested
  sources, native receipts, documentation and protected-product identities.

Remaining GCC runtime classes comprise twenty padding observations, five
conversions, five sign conversions, five switch-default, six discarded
qualifiers, two cast-qual and one redundant declaration. Native terminal-speed,
thread-stack and TCP count/length owners retain their remaining conversions.
Seventeen dynamic-format runtime contracts, four finite-format compiler sites
and other Clang source/ABI classes retain their cleanup scope. Templari's newer
concurrent sources/recipe await replay against its last qualified cut. Other
project results retain their preceding source and native contexts. Solaris and
other native runtime execution retain their independently recorded evidence.

<!-- end of c-emitter-keyboard-input.md -->
