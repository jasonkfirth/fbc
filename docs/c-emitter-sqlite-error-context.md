<!--
Project: FreeBASIC C-backend qualification
File: c-emitter-sqlite-error-context.md
Purpose: Record the SQLite error helper's connection-lifetime repair.
Responsibilities: Bind the source invariant, diagnostics and native regression.
Full SQL application and other source owners retain separate qualification.
-->

# SQLite error-path connection ownership

The audited SQLite port's `sqlite3VdbeExecError` now saves its validated
connection pointer before error notification and VM cleanup. Its corruption
handling reads that connection's current autocommit state and updates its
flags. The initial null guards, callback order, error-action selection and
return codes are preserved.

The port keeps a connection alive while statements reference it.
`sqlite3ConnectionClose` returns BUSY for an ordinary close with outstanding
statements. Deferred close marks the connection as a zombie and retains the
handle. The error helper uses the same locally retained connection pattern as
the surrounding VDBE helpers. Logging and halt callbacks can update connection
state while that association remains valid.

GCC's original optimized output reloads `pVdbe->db` after callbacks. The inlined
finish helper accepts a null value on one path, followed by a dereference in
the corruption branch. The source repair expresses the existing lifetime
invariant directly. It adds no warning suppression or compiler exception.
The three observations receive a library-source resolution. The other port
null observations retain their source-contract review.

## Diagnostics and execution

Development compiler `626a5305` emits the complete changed module through both
C producers, yielding identical C. GCC 14.2 and Clang 19.1.7 compile the
unmodified complete before/after modules under the original unsuppressed
diagnostic options at O0 and O2. At O2, the module's GCC observations fall
from three to zero. Its 52 Clang padding and fourteen buffer-usage observations
remain visible.

Sixteen native regression runs cover both revisions, both C producers, both
native compilers and both optimizations. Each run checks 23,040 cases and
149,760 assertions, totaling 368,640 cases and 2,396,160 assertions. The driver
uses the actual canonical database, VM and instruction records and a verbatim
copy of the private production helper. The independent C observer verifies
pointer identity and callback order, null entry handling, original result
codes, default error actions, callback-updated autocommit, the corruption flag
and final return codes. The observer supplies the four external callbacks.
Whole-engine SQL execution retains its separate project checks.

Strict source-only lint of the changed port module and BASIC driver reports
zero errors and one pre-existing whitespace advisory at line 445. The fixture
has no lint findings. The original FBC variable-not-read warning is retained
by the full-module emission.

The complete 1,053-profile port census reuses the preceding exact native
contexts for the other 1,052 unchanged C modules and replaces this module's
diagnostics. Coreutils remains GCC 165 / Clang 1,784. SQLite reaches GCC 18 /
Clang 7,696. The disjoint GCC total reaches 199. Application and platform
follow-ups retain their recorded scope.

## Reproduction

The source repair is applied in the local audited checkout under
`build/c-emitter-forward-views-20261105/github-projects/freebasic-sqlite`.
Publication to the port's GitHub repository remains pending. The portable
`sqlite-vdbe-error-port-v1-20261116.patch` applies independently to the captured
preimage, with the resulting source verified byte-for-byte.

The permanent fixtures are
`tests/compiler-support/sqlite-vdbe-error-context.bas` and its C observer.
Supply `sqlite-vdbe-error-body.bi` containing the verbatim
`SQLITE3_VDBE_DBFLAG_CORRUPT_RDONLY` definition and `sqlite3VdbeExecError` body
from the selected port revision. Emit the BASIC fixture with that directory
and the port's `fb_src` directory on the include path, using the production
target and defines. The generated C module can then be checked on the matching
native host with the permanent runner.

```sh
python3 -B build_scripts/test-sqlite-vdbe-error.py \
  --module /path/to/emitted-fixture.c \
  --cc /usr/bin/gcc --native-flags-json /path/to/original-flags.json \
  --optimization 2 --scratch-root /path/to/scratch \
  --receipt /path/to/result.json
```

The runner owns one temporary directory, checks its input hashes after
execution and retains all native diagnostics. Its scratch limit is 32 MiB.
The qualification uses one low-priority serial guest worker, with no installed
compiler or application replacement. The source packet preserves complete
before/after production C, four emitted fixture bodies and source hashes.

Receipts under `build/c-emitter-forward-views-20261105` are
`sqlite-vdbe-packet-v1-20261116.json.zlib`,
`sqlite-vdbe-native-v1-20261116.json.zlib` and
`sqlite-vdbe-patch-v1-20261116.json.zlib`. The capture controller and native
worker retain the exact emission, native build and execution commands.

<!-- end of c-emitter-sqlite-error-context.md -->
