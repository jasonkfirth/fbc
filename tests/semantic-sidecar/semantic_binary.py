"""Project: FreeBASIC semantic sidecar tests
File: semantic_binary.py
Purpose: Independently decode the self-describing binary transport.
Responsibilities: Bound frames, reject unsupported capabilities, preserve wire bytes.
This file intentionally does NOT validate compiler semantic graphs.
"""

from __future__ import annotations

from collections.abc import Iterator
import mmap
from pathlib import Path
import struct

MAGIC = b"FBCSEMB1\n"
MAX_HEADER = 64 * 1024
MAX_STRINGS = 262144
MAX_STRING_BYTES = 64 * 1024 * 1024
MAX_INTEGER = (1 << 61) - 1
CAPABILITIES = {"core", "source-revisions", "native-types", "control-flow", "macro-provenance", "structured-values"}


def iter_decode(data, *, max_record_bytes: int = 64 * 1024 * 1024,
                max_records: int = 24_000_000,
                max_output_bytes: int = 1024 * 1024 * 1024,
                expected_mode: str | None = None) -> Iterator[bytes]:
    """Yield bounded logical rows without retaining the expanded model."""
    if data[:len(MAGIC)] != MAGIC:
        raise ValueError("Not a binary semantic model")
    position = len(MAGIC)
    records: dict[int, tuple[bytes, int]] = {}
    tags: set[bytes] = set()
    capabilities: set[str] = set()
    supported: set[str] = set()
    metadata: dict[bytes, bytes] = {}
    while position < min(len(data), MAX_HEADER):
        ending = data.find(b"\n", position, MAX_HEADER)
        if ending < 0:
            raise ValueError("Unterminated binary semantic header")
        line = data[position:ending]
        if any((byte < 32 and byte != 9) or byte > 126 for byte in line):
            raise ValueError("Invalid binary semantic header byte")
        fields = line.split(b"\t")
        position = ending + 1
        if fields == [b"DATA"]:
            break
        if fields[0] == b"record":
            if len(fields) != 5 or not fields[1].isdigit() or not fields[3].isdigit():
                raise ValueError("Invalid binary record definition")
            identity, arity = int(fields[1]), int(fields[3])
            if not 1 <= identity <= 255 or not 2 <= arity <= 255 or not fields[4]:
                raise ValueError("Invalid binary record definition")
            if identity in records or fields[2] in tags:
                raise ValueError("Duplicate binary record definition")
            records[identity] = fields[2], arity
            tags.add(fields[2])
        elif fields[0] == b"cap":
            if len(fields) != 4 or not fields[1] or fields[3] not in (b"0", b"1"):
                raise ValueError("Invalid binary capability definition")
            if not fields[2].isdigit() or fields[2].startswith(b"0") or len(fields[2]) > 19 or int(fields[2]) > MAX_INTEGER:
                raise ValueError("Invalid binary capability version")
            capability = fields[1].decode("ascii")
            if capability in capabilities:
                raise ValueError("Duplicate binary capability")
            capabilities.add(capability)
            if capability in CAPABILITIES and fields[2] == b"1":
                supported.add(capability)
            if (capability not in CAPABILITIES or fields[2] != b"1") and fields[3] != b"0":
                raise ValueError("Unsupported required capability: " + capability)
        elif fields[0] in (b"schema", b"producer", b"mode", b"layout", b"values", b"dictionary"):
            if len(fields) != 2 or not fields[1] or fields[0] in metadata:
                raise ValueError("Duplicate or invalid binary header metadata")
            metadata[fields[0]] = fields[1]
        else:
            raise ValueError("Unknown binary semantic header element")
    else:
        raise ValueError("Binary semantic header exceeds its byte limit")
    if set(metadata) != {b"schema", b"producer", b"mode", b"layout", b"values", b"dictionary"} or metadata[b"schema"] != b"27":
        raise ValueError("Unsupported binary semantic schema")
    if metadata[b"mode"] not in (b"full", b"bindings", b"expressions"):
        raise ValueError("Unsupported binary semantic export mode")
    if expected_mode is not None and metadata[b"mode"] != expected_mode.encode("ascii"):
        raise ValueError("Binary semantic export mode does not match the requested facts")
    required = {"core", "source-revisions"}
    if metadata.get(b"mode") == b"full":
        required |= {"native-types", "control-flow"}
    if not required <= supported:
        raise ValueError("Missing required binary semantic capabilities")
    strings: list[bytes] = [b""]
    string_bytes = 0

    def varint(ending: int) -> int:
        nonlocal position
        number = 0
        for shift in range(0, 64, 7):
            if position >= ending:
                raise ValueError("Truncated binary semantic integer")
            part = data[position]
            position += 1
            if shift == 63 and part > 1:
                raise ValueError("Overflowing binary semantic integer")
            number |= (part & 127) << shift
            if not part & 128:
                if shift and not part:
                    raise ValueError("Noncanonical binary semantic integer")
                return number
        raise ValueError("Overflowing binary semantic integer")

    def value(ending: int, prefix: bool = False, grouped: bool = False) -> bytes:
        nonlocal position, string_bytes
        token = varint(ending)
        kind, payload = token & 7, token >> 3
        if kind == 0:
            if payload:
                raise ValueError("Invalid empty binary semantic value")
            return b""
        if kind in (1, 2):
            if kind == 2 and not payload:
                raise ValueError("Noncanonical binary negative zero")
            return (b"-" if kind == 2 else b"") + str(payload).encode("ascii")
        if kind == 3:
            if not 1 <= payload < len(strings):
                raise ValueError("Invalid binary semantic string reference")
            return strings[payload]
        if kind in (4, 5):
            if not 1 <= payload <= ending - position:
                raise ValueError("Invalid binary semantic string length")
            result = data[position:position + payload]
            position += payload
            if any(byte < 32 or byte > 126 for byte in result):
                raise ValueError("Unescaped binary semantic string bytes")
            if kind == 4:
                if len(strings) > MAX_STRINGS or payload > MAX_STRING_BYTES - string_bytes:
                    raise ValueError("Binary semantic dictionary exceeds its byte limit")
                strings.append(result)
                string_bytes += payload
            return result
        if kind == 6:
            if payload or prefix:
                raise ValueError("Invalid binary semantic prefix token")
            result = value(ending, True, grouped)
            suffix = varint(ending)
            if not result.endswith(b":") or suffix > MAX_INTEGER:
                raise ValueError("Invalid binary semantic prefix value")
            return result + str(suffix).encode("ascii")
        if kind == 7:
            if prefix or grouped or not 2 <= payload <= 256 or "structured-values" not in supported:
                raise ValueError("Invalid binary semantic structured value")
            parts = []
            length = 0
            for _ in range(payload):
                part = value(ending, grouped=True)
                length += len(part) + (3 if parts else 0)
                if length > max_record_bytes:
                    raise ValueError("Expanded binary semantic tuple exceeds its byte limit")
                parts.append(part)
            return b"%09".join(parts)
        raise ValueError("Unknown binary semantic value encoding")

    header = b"FBCSEM\t27\t" + metadata[b"producer"]
    output_bytes = len(header) + 1
    if output_bytes > max_output_bytes or max_records < 1:
        raise ValueError("Expanded binary semantic model exceeds its byte limit")
    yield header
    count = 1
    while position < len(data):
        if len(data) - position < 4:
            raise ValueError("Truncated binary semantic frame")
        length, = struct.unpack_from("<I", data, position)
        position += 4
        if not 2 <= length <= min(max_record_bytes, len(data) - position):
            raise ValueError("Binary semantic frame exceeds its byte limit")
        ending = position + length
        identity, field_count = data[position:position + 2]
        position += 2
        if identity not in records or records[identity][1] != field_count + 1:
            raise ValueError("Unknown binary semantic record or field count")
        row = bytearray(records[identity][0])
        for _ in range(field_count):
            field = value(ending)
            if len(row) + 1 + len(field) > max_record_bytes:
                raise ValueError("Expanded binary semantic record exceeds its byte limit")
            row.extend(b"\t" + field)
        if position != ending:
            raise ValueError("Trailing bytes inside binary semantic record")
        count += 1
        output_bytes += len(row) + 1
        if count > max_records or output_bytes > max_output_bytes:
            raise ValueError("Expanded binary semantic model exceeds its byte limit")
        yield bytes(row)


def decode(data: bytes, **limits) -> bytes:
    """Return exact schema-27 TSV bytes for the independent graph validator."""
    if not data.startswith(MAGIC):
        return data
    output = bytearray()
    for row in iter_decode(data, **limits):
        output.extend(row)
        output.append(10)
    return bytes(output)


def read_rows(path: Path, *, max_bytes: int = 1024 * 1024 * 1024) -> Iterator[bytes]:
    """Stream either transport for receipt readers that only need a few tags.

    Mapping the compact file keeps dependency scans from allocating a second
    expanded model. Consumers must exhaust the iterator before trusting it.
    This validates framing; semantic ownership still belongs to Model.
    """
    with path.open("rb") as source:
        size = source.seek(0, 2)
        if not 0 < size <= max_bytes:
            raise ValueError("Semantic model exceeds its byte limit")
        source.seek(0)
        if source.read(len(MAGIC)) == MAGIC:
            with mmap.mmap(source.fileno(), 0, access=mmap.ACCESS_READ) as data:
                yield from iter_decode(data)
        else:
            source.seek(0)
            for row in iter(lambda: source.readline(64 * 1024 * 1024 + 1), b""):
                if len(row) > 64 * 1024 * 1024 or not row.endswith(b"\n"):
                    raise ValueError("Invalid text semantic row")
                yield row.rstrip(b"\r\n")


# end of semantic_binary.py
