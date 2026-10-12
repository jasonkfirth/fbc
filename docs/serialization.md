# Compiler-assisted logical serialization

Include `fbserialize.bi` to enable `Serialize(value)` and
`Deserialize(payload, value)`. These names are header macros; ordinary programs
can continue to declare functions or variables called `Serialize` and
`Deserialize` without including the header. Existing `PUT`, `GET`, assignment,
and UDT layouts are unchanged.

The manual contains [Serialize](../doc/manual/cache/KeyPgSerialize.wakka) and
[Deserialize](../doc/manual/cache/KeyPgDeserialize.wakka) reference pages and a
[serialization guide](../doc/manual/cache/ProPgSerialization.wakka).
Runnable examples demonstrate
[deep UDT ownership](../examples/manual/udt/serialize.bas),
[whole arrays and failed decoding](../examples/manual/array/deserialize.bas),
[scalars, fixed strings and fixed arrays](../examples/manual/array/serialization-fixed.bas),
[binary files](../examples/manual/fileio/serialization.bas), and
[application version migration](../examples/manual/proguide/serialization-version.bas).
The file example takes a new or empty output filename as its command-line argument.

```freebasic
#include once "fbserialize.bi"

type PlayerState
    player_name as string
    health as integer
    inventory(any) as integer
end type

dim as PlayerState player1, player2
player1.player_name = "Player One"
player1.health = 100
redim player1.inventory(-2 to 9)
player1.inventory(-2) = 42

dim as string payload = Serialize(player1)
if err <> 0 then end 1

if Deserialize(payload, player2) then
    print player2.player_name
    print player2.inventory(-2)
else
    print "Invalid serialization payload or allocation failure: "; err
end if
```

The returned binary `STRING` owns its bytes. It can contain null bytes, so use
its length when handing it to an application transport. Binary file `PUT` and
`GET`, TCP streams, and application buffers can carry the same bytes. The codec
does not open files, sockets, or devices. Framing multiple payloads in a stream
belongs to the transport; pass exactly one complete payload to `Deserialize`.

For whole arrays use `Serialize(values())` and
`Deserialize(payload, values())`. Known dimensions and bounds of fixed arrays
are part of their schemas. Dynamic bounds come from the payload. Unknown-rank
dynamic arrays and BYDESC parameters are supported, but a borrowed descriptor
cannot be enlarged beyond its caller's capacity or violate its fixed rank.

## Supported values and ownership

The compiler traverses public data fields, including nested UDTs, fixed arrays,
dynamic arrays, and arrays of UDTs. It supports `BOOLEAN`, all signed and
unsigned integer widths, enums, IEEE binary32/binary64 floats, fixed `STRING * N`,
and dynamic `STRING`. Strings preserve embedded nulls; fixed strings preserve
all their stored bytes, including spaces. Unallocated dynamic arrays retain a
valid empty descriptor and can subsequently be used with `REDIM`.

Pointers, procedure pointers, BYREF fields, bitfields, unions, inheritance,
non-public fields, `ZSTRING`, `WSTRING`, `USTRING`, and UDTs with user-defined
constructors, destructors, or operators are rejected at compilation with a
serialization-specific diagnostic. Their ownership or representation needs an
explicit application codec. Such a codec can convert an owned pointer graph or
external object into a supported wire UDT and serialize that UDT. Raw addresses
are never encoded automatically. Dynamic STRING/array members' compiler-generated
constructors, copy operations, and destructors are supported.

`Serialize` accepts a variable, array, member, dereference, or supported function
result. `Deserialize` requires a writable variable or array. Both operations
expect valid ordinary FreeBASIC values; arbitrary memory obtained by overwriting
descriptors with raw bytes is outside this contract. CONST sources can be
serialized; destinations containing CONST fields are rejected because
reconstruction cannot write those fields.

Deserialization builds a separate zero-initialized ownership tree. Every length,
bound, rank, schema and trailing byte is checked before the existing destination
is destroyed. Commit transfers allocations directly, without a fallible UDT
assignment or another allocation. On failure the destination is unchanged and
all temporary allocations are freed. Reconstructed strings and arrays never
borrow storage from the input payload or the serialized source. Packed UDTs are
supported through alignment-safe descriptor and scalar accesses.

Each call keeps its plan, cursor, allocation budget and traversal state locally.
As with ordinary assignment, callers must synchronize concurrent access to the
same source or destination. The codec introduces no global schema registry.

## Compiler/runtime boundary

The compiler emits a type-specific traversal program and a separate canonical
schema into the native program. Layout instructions contain the actual target's
offsets, sizes and descriptor capacities. The runtime executes those instructions
through shared scalar/array/string operations. It does not discover fields,
resolve type names, or inspect a runtime reflection registry. No member
registration or generated source include is needed.

This implementation uses a compact traversal program rather than duplicating
native procedures for every type. The runtime parses only compiler-owned layout
instructions, allocating a node table sized to the actual program. Payloads
cannot supply or modify layout instructions. The shared executor reduces code
duplication; per-type native routine generation could be added without changing
the wire format.

## Version 1 binary format

All multi-byte wire values use little-endian order. Padding, descriptor bytes,
pointer values, and native field offsets are never sent.

| Bytes | Meaning |
| --- | --- |
| 0 through 3 | ASCII `FBS1`, including format version 1 |
| 4 through 7 | Unsigned 32-bit canonical schema byte count |
| Next schema-length bytes | Exact canonical schema, without a terminator |
| Remaining bytes | Logical contents in field declaration/array storage order |

The schema records member names and order, supported scalar categories,
fixed-width sizes, enum member names/values, fixed string lengths, array kinds,
fixed bounds and declared ranks. UDT names themselves are not nominal identities:
structurally identical public wire definitions are compatible. Packing and
native INTEGER width are deliberately excluded. All schema bytes must match;
there is no hash collision or silent member omission.

| Value | Wire representation |
| --- | --- |
| BYTE/UBYTE, SHORT/USHORT, LONG/ULONG, LONGINT/ULONGINT | Declared 1, 2, 4 or 8 bytes; signed integers use two's complement |
| INTEGER/UINTEGER | Signed/unsigned 64-bit value; a narrower reader checks the destination range |
| Enum | Signed 64-bit value with member definitions included in its schema |
| BOOLEAN | One byte, 0 or 1; other values are invalid |
| SINGLE/DOUBLE | 4/8 IEEE representation bytes, preserving zero signs and NaN payload bits |
| Dynamic STRING | Unsigned 32-bit length followed by exactly that many bytes |
| Fixed STRING | Exactly its declared byte count |
| UDT/fixed array | Consecutive logical fields/elements; no padding |
| Dynamic/root array | One-byte allocated rank, or 0 for unallocated; then signed 64-bit lower/upper bounds for each dimension, then elements |

Instruction `R` describes an array accessed through a descriptor, including a
root fixed array; instruction `A` describes inline fixed array storage. `R0`
in a schema means unknown declared rank, not a zero-dimensional allocated array.
Unknown rank and explicitly declared rank use distinct schemas. Array storage
order follows FreeBASIC's contiguous element order, with the final dimension
varying fastest.

Incompatible schemas or format versions are rejected. To evolve an application
record, keep a stable envelope containing an application version and a binary
STRING payload. Dispatch by that version, deserialize into the corresponding
old/new wire UDT, then migrate explicitly. This makes migration policy visible
and keeps unsupported ownership transformations out of the codec.

## Limits and errors

Version 1 enforces these per-call limits:

| Resource | Limit |
| --- | --- |
| Complete payload | 16 MiB |
| Aggregate decode allocations, including metadata/scratch | 64 MiB |
| Canonical schema | 64 KiB |
| Traversal instructions | 4,096 |
| Nested instruction depth | 32 levels |
| Array dimensions | 8 |
| Array elements and total encode/decode value visits | 1,048,576 |

Array extent products, signed index biases, native bound conversions and byte
counts are checked before allocation or descriptor reconstruction. Truncated
arrays are checked against their minimum element wire size before allocation.
The visit budget also bounds arrays of records with no encoded fields. The
compiler rejects types exceeding its schema, instruction or depth limits.

Success sets `ERR` to zero. `Serialize` returns an empty string on failure;
`Deserialize` returns zero on failure and `TRUE` on success.
`FB_SERIALIZE_INVALID` (runtime error 1) covers malformed, incompatible or
over-budget data; `FB_SERIALIZE_OUT_OF_MEMORY` (runtime error 4) covers allocator
or temporary-descriptor exhaustion. A successful serialized empty STRING still
has a nonempty format header.

## Build and regression tests

Build the compiler and runtime from matching source:

```text
make -j4 compiler rtlib
python build_scripts/test-serialization.py --fbc bin/fbc.exe --backend gcc
```

To keep a candidate compiler separate, pass `FBC_EXE=bin/fbc-serialization.exe`
and `BUILD_FBC=bin/fbc.exe` to make, then pass that candidate to the test runner.
The runner compiles the runtime codec into isolated objects, tests allocation
failures with the real runtime string services, and removes its temporary
directory afterwards. It checks round trips, every truncated prefix, pointer
and object diagnostics, packed fields, descriptor capacities, malformed input,
resource limits and independent little-endian reference bytes.
Supply `--baseline bin/fbc.exe` to compare unchanged identifier, assignment and
file-I/O behavior with the working compiler. Supply an existing Clang executable
with `--clang PATH` to run the runtime fault harness with undefined-behavior
traps; this does not require installing a sanitizer runtime.

For real Windows 32/64 exchange, supply an installed 32-bit SDK and C compiler:

```text
python build_scripts/test-serialization.py --fbc bin/fbc-serialization.exe --backend gcc --backend gas64 --cc32 C:/msys64/mingw32/bin/gcc.exe --prefix32 C:/FreeBASIC
```

Without those explicit options the runner reports the 32/64 exchange as skipped.
The wire design does not rely on host byte order, but Windows 32/64 exchange is
not execution evidence for a big-endian target or every supported architecture.

<!-- end of serialization.md -->
