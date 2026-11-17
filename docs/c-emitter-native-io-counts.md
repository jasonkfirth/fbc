<!--
Project: FreeBASIC C backend verification
File: c-emitter-native-io-counts.md
Purpose: Record bounded native I/O and pthread size conversions.
Responsibilities: Explain platform types, executable evidence and exact reuse.
Other native platforms retain their independently recorded qualification.
-->

# Native I/O counts, November 15

Two runtime sources make bounded native conversions explicit. The complete
Linux MT runtime and startup build reaches GCC 35 / Clang 10,047, removing nine
observations from each tool. All GCC sign-conversion observations are closed.
The last-qualified disjoint GCC total is 238.

## Native conversion contracts

The POSIX TCP adapters receive requests capped at `INT_MAX` or smaller fixed
buffers. Native calls return an error sentinel or a byte count within that
requested range. The adapter's explicit conversion supplies the existing int
device-helper result. POSIX lengths use `size_t`, and address records retain
their native socket-length field. Winsock retains its int address-length
boundary. Request evaluation, partial results and errno handling preserve their
preceding behavior.

The pthread adapter converts its positive, minimum-clamped reserve to `size_t`
before setting the native thread attribute. The existing refusal of unsupported
attributes and failed creation remains applicable. The manual now describes
the runtime reserve minimum and Linux created-thread stack behavior.
Linux fixes a created thread's stack size at creation. [pthread stack attributes](https://man7.org/linux/man-pages/man3/pthread_attr_setstacksize.3.html)

## Native qualification

Both complete native builds retain all 1,036 preceding object identities,
both archive identities and both startup identities. The preceding 260 runtime
executions are reused through those exact binaries and unchanged regression
inputs. They are recorded as reused executions.

Eight fresh 32/64-bit GCC/Clang O0/O2 component contexts pass 2,336 assertions.
The component executes the actual TCP helper bodies with bounded native
callbacks. It checks zero/partial/maximum byte counts, `INT_MAX` request clipping,
native errors and address lengths without allocating corresponding large
buffers. Two original-source contexts pass the same 584 assertions with their
original warnings enabled.

The existing `tests/file/tcp.bas` program passes eight fresh loopback runs
against the rebuilt archives. Its four captured BASIC producer/optimization
contexts explicitly enable assertions and contain 21 runtime assertion calls
each. The program exercises client/server, text/binary transfers, closure,
wildcard listening and a sustained 4,096-byte byte-at-a-time exchange.
Source-only library lint reports zero errors and zero warnings.

The existing `tests/android/thread-stack.c` fixture passes four fresh executions
on Linux across the native tools and O0/O2. Its five cases check default/small/
negative requests, a 16 MiB request and impossible-reserve refusal. Worker code
queries the actual pthread reserve and touches real stack pages. Each process
has a 512 MiB virtual-address limit and creates its workers sequentially.
Android device execution retains its separate platform evidence.

Twenty fresh current executions supplement the 260 reused contexts. Eight
Windows object compiles cover GCC/Clang and 32/64-bit headers. Each before/after
object pair and warning-class map is identical. The serial Linux worker uses
at most 15,779,629 scratch bytes and removes its temporary tree.

## Reproduction and receipts

The bounded component has a normal repository entry point.

```sh
python3 -B build_scripts/test-native-io-counts.py \
  --cc /usr/bin/gcc --bits 64 --optimization 2 \
  --receipt /path/to/new-io-count-receipt.json
```

Repeat with Clang and both word sizes and optimization levels. The runner accepts
the complete unsuppressed native flag array through `--native-flags-json`, checks
source hashes and removes its products. Receipts are under
`build/c-emitter-forward-views-20261105`.

- `runtime-current-packet-v35-20261115.json.zlib` pins the full source cut,
  existing tests and assertion-enabled BASIC emissions.
- `runtime-current-linux-v30-20261115.json.zlib` records complete builds,
  exact-binary reuse and fresh component/loopback/pthread executions.
- `native-io-count-windows-v1-20261115.json.zlib` records Windows branch
  object and warning equivalence with the actual native headers.
- `native-io-count-qualification-v1-20261115.json.zlib` binds the tested sources,
  native receipts, documentation and protected-product identities.

## Remaining work

GCC runtime classes comprise twenty padding observations, five switch-default,
one terminal-speed conversion, one redundant declaration, six discarded
qualifiers and two cast-qual observations. Seventeen dynamic-format runtime
contracts, four finite-format compiler sites and other Clang source/ABI owners
retain their cleanup scope. Templari's newer concurrent sources/recipe await
replay against its last qualified cut. Other projects retain their preceding
source and native contexts.

<!-- end of c-emitter-native-io-counts.md -->
