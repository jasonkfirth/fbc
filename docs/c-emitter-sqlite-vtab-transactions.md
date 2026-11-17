<!--
Project FreeBASIC C-backend qualification
File c-emitter-sqlite-vtab-transactions.md
Purpose Record the SQLite virtual-table transaction pointer repair.
Responsibilities Describe source ownership, retained diagnostics and reproduction.
Whole-engine SQL execution retains its project qualification.
-->

# SQLite virtual-table transaction ownership

The audited port's `sqlite3VtabCallCreate` retains the `SqliteVTable` pointer
checked after its constructor succeeds. It passes that same instance to
`addToVTrans` after successfully growing the transaction array. The connection
owns the virtual-table instance during this operation. The transaction array
can move independently of the instance and its containing table list.

The previous code checked one `sqlite3GetVTable` result and called that lookup
again after `growVTrans`. The second result could appear null to GCC when
inlined into `sqlite3VtabLock`. Retaining the checked instance expresses the
existing lifetime contract. Missing instances, constructor failures and array
growth failures retain their return codes and skip the append and reference
increment.

The complete before/after modules emit identical C through both C producers
of development compiler `4a477330`. The before emission also matches the
complete preceding port census. GCC 14.2 and Clang 19.1.7 compile both revisions
at O0 and O2 with the original unsuppressed production recipes.

At O2, this module's four GCC null-dereference observations are removed.
Its nested-pointer qualifier diagnostic remains. Clang retains 38 padding,
one qualifier and one buffer-usage observations at both optimizations.
The existing FBC variable-not-read warning messages are preserved.

## Native regression

The permanent BASIC and C fixtures are
`tests/compiler-support/sqlite-vtab-transaction.bas` and
`tests/compiler-support/sqlite-vtab-transaction.c`.
The BASIC fixture uses the canonical port records and verbatim bodies of
`sqlite3VtabCallCreate`, `sqlite3GetVTable`, `sqlite3VtabLock`, `growVTrans`
and `addToVTrans`. Controlled providers supply module lookup, constructor
results and a moving transaction-array allocator. Native libc supplies memset.

Sixteen runs cover both revisions, both C producers, GCC/Clang and O0/O2.
They pass 122,880 cases and 4,057,360 checks. Each run varies valid and missing
modules, missing constructor or destructor, four constructor outcomes, matching
instances at three list positions or absent, allocation success or failure,
fifteen existing transaction counts and four initial reference counts. The
largest reference count is `INT_MAX - 1`, keeping the successful increment
within the supported C integer range.

Checks cover callback arguments, return codes, error messages, array identity,
preserved entries, zeroed new slots, final transaction count and the exact
instance reference increment. The allocator moves the array into independent
storage and preserves its existing prefix. Allocation failure leaves the
original array and counts intact. Whole-engine SQL execution remains pending
under the port's application qualification.

Strict source-only lint reports two files, zero errors and three existing
production advisories. The fixture has no lint findings. The serial native
worker runs at reduced priority, owns one temporary directory and removes it
after completion. Its peak scratch set was 201,362 bytes.

## Source delivery and census

The source repair is applied in the audited local checkout under
`build/c-emitter-forward-views-20261105/github-projects/freebasic-sqlite`.
Publication to the port's GitHub repository remains pending. The portable
`sqlite-vtab-transaction-port-v1-20261116.patch` is checked by applying it to
the captured preimage and comparing the complete resulting file byte-for-byte.

The complete 1,053-profile port census replaces this module's diagnostic
result and retains the other 1,052 unchanged native contexts. SQLite reaches
GCC 14 and retains Clang 7,696. Coreutils remains GCC 159 and Clang 1,782.
The disjoint GCC total reaches 189. The two finite compiler format cases,
other source faults and native platform checks remain open.

The final current-source check also refreshes eight Templari server contexts
after intervening edits. Both C producers and both native tools pass. The other
669 contexts retain exact current input hashes. The complete 677-context queue
remains GCC 2 and Clang 54,317. No application binary is replaced by this audit.

## Reproduction

Provide `sqlite-vtab-transaction-body.bi` containing the five verbatim procedure
bodies listed above from the selected port revision. Emit the BASIC fixture
with the canonical `fb_src` directory and this include directory on its search
path. Use the port's production target, threading defines and ABI options.
Compile the emitted C and the permanent C observer with the matching native
compiler, link them and run the executable. Repeat for both C producers and
both native compilers at O0 and O2. A successful run prints
`SQLITE_VTAB_PASS cases=7680 checks=253585 messages=5760`.

The audit directory retains `sqlite-vtab-packet-v1-20261116.json.zlib`,
`sqlite-vtab-native-v2-20261116.json.zlib` and
`sqlite-vtab-qualification-v1-20261116.json.zlib`. The packet records source
hashes and exact emission commands. The native worker records complete build,
link and execution commands and their original diagnostics.

<!-- end of c-emitter-sqlite-vtab-transactions.md -->
