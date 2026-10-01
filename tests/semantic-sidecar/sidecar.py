"""Project: FreeBASIC semantic sidecar tests
File: sidecar.py
Purpose: Read and independently validate schema 20 sidecars.
Responsibilities: Check shapes, references, ranges, escaping, and completion totals.
This file intentionally does NOT contain: compiler invocation or inferred semantics.
"""

from __future__ import annotations

from collections import Counter, defaultdict
from pathlib import Path
import re


SCHEMA = "20"
SHAPES = {
    "FBCSEM": 3, "M": 2, "D": 2, "S": 12, "B": 9, "I": 12,
    "P": 9, "V": 5, "N": 13, "E": 16, "R": 3, "END": 13,
    "RECOVERY": 8, "T": 19, "A": 7, "F": 13, "G": 9, "U": 12,
    "C": 5, "H": 7, "K": 5, "J": 5, "O": 10, "Q": 10, "Y": 7, "Z": 5,
}
DETAIL_TAGS = frozenset(("T", "A", "F", "G", "U", "C", "H", "K", "J", "O", "Q", "Y", "Z"))
TYPE_KINDS = frozenset(("pointer", "numeric", "dynamic-string", "fixed-string",
                        "aggregate", "procedure", "other"))
SYMBOL_CLASSES = ("variable", "constant", "procedure", "parameter", "define", "keyword",
                  "label", "namespace", "enum", "type", "class", "field", "typedef",
                  "forward-type", "scope", "reserved", "namespace-import")
PROCEDURE_KINDS = frozenset(("sub", "function", "constructor", "destructor", "property-get",
                             "property-set", "operator", "procedure-pointer"))
CALLING_CONVENTIONS = frozenset(("cdecl", "stdcall", "stdcall-ms", "pascal", "thiscall", "fastcall"))
STORAGE_KINDS = frozenset(("parameter", "field", "constant", "external", "static", "local",
                          "shared", "global", "none"))
# The schema records internal datatype slots too. Keep the full vocabulary so
# removing the last slot cannot pass as a shorter, still contiguous table.
PRIMITIVE_NAMES = ("any", "boolean", "byte", "ubyte", "zstring", "short", "ushort",
                   "wstring", "integer", "uinteger", "enum", "long", "ulong",
                   "longint", "ulongint", "single", "double", "string", "fixed-string",
                   "va_list", "type", "namepace", "function", "fwdref", "pointer",
                   "xmmword", "ustring")
IMPLICIT_KINDS = frozenset((
    "default-constructor", "initializer-constructor", "new-constructor",
    "destructor-call", "argument-constructor", "temporary-destructor",
    "scope-exit-destructor", "delete-destructor", "return-constructor",
))
def unescape(field: str) -> str:
    # Compiler strings contain bytes, including accepted legacy source bytes
    # and filesystem names. Decode valid UTF-8 normally and retain every other
    # byte with Python's reversible filesystem convention. No second unescape
    # pass is allowed: a literal "%E9" is serialized as "%25E9".
    value = bytearray()
    offset = 0
    while offset < len(field):
        character = field[offset]
        if character == "%":
            code = field[offset + 1:offset + 3]
            if len(code) != 2 or re.fullmatch(r"[0-9A-F]{2}", code) is None:
                raise ValueError("Truncated or malformed percent escape")
            value.append(int(code, 16))
            offset += 3
        else:
            if ord(character) < 32 or ord(character) >= 127:
                raise ValueError("Unescaped byte in sidecar field")
            value.append(ord(character))
            offset += 1
    return value.decode("utf-8", errors="surrogateescape")


def number(value: str, minimum: int | None = None) -> int:
    if re.fullmatch(r"-?(?:0|[1-9][0-9]*)", value) is None:
        raise ValueError("Invalid integer: " + repr(value))
    result = int(value)
    if minimum is not None and result < minimum:
        raise ValueError("Integer below permitted minimum: " + value)
    return result


def source_range(row: list[str], offset: int) -> tuple[str, int, int, int, int]:
    path = row[offset]
    start_line = number(row[offset + 1], 1)
    start_column = number(row[offset + 2], 0)
    end_line = number(row[offset + 3], start_line)
    end_column = number(row[offset + 4], 0)
    if not path or (end_line == start_line and end_column <= start_column):
        raise ValueError("Empty, reversed, or missing source range")
    return path, start_line, start_column, end_line, end_column


class Model:
    def __init__(self, text: str, *, expressions_only: bool = False,
                 allow_recovery: bool = False) -> None:
        self.rows: list[list[str]] = []
        self.records: dict[str, list[list[str]]] = defaultdict(list)
        self.symbols: dict[int, list[str]] = {}
        self.types: dict[int, list[str]] = {}
        self.nodes: dict[int, list[str]] = {}
        self.properties: dict[tuple[str, int], dict[str, str]] = defaultdict(dict)
        self.constants: dict[tuple[str, int], tuple[str, str]] = {}
        self.signatures: dict[int, list[str]] = {}
        self.parameters: dict[int, dict[int, list[str]]] = defaultdict(dict)
        self.layouts: dict[int, list[str]] = {}
        self.contexts: list[list[str] | None] = []
        self.primitives: list[dict[int, list[str]]] = []
        if not text.endswith("\n"):
            raise ValueError("Sidecar is truncated")
        counts: Counter[str] = Counter()
        expression_ids: set[int] = set()
        child_edges: set[tuple[int, str]] = set()
        dependencies: set[str] = set()
        references: list[tuple[str, int]] = []
        footer: list[str] | None = None
        module_expressions = 0

        def reference(domain: str, value: str, *, nullable: bool = True) -> None:
            identity = number(value, 0 if nullable else 1)
            if identity:
                references.append((domain, identity))

        def flag(value: str) -> None:
            if value not in ("0", "1"):
                raise ValueError("Invalid boolean: " + value)

        for line_number, line in enumerate(text.splitlines(), 1):
            raw = line.split("\t")
            tag = raw[0]
            if footer is not None:
                raise ValueError("Record after completion footer")
            if tag not in SHAPES or len(raw) != SHAPES[tag]:
                raise ValueError(f"Line {line_number}: unknown record or incorrect field count: {tag}")
            row = [unescape(field) for field in raw]
            self.rows.append(row)
            self.records[tag].append(row)
            if line_number == 1:
                if tag != "FBCSEM" or row[1] != SCHEMA or not row[2]:
                    raise ValueError("Invalid or unsupported header")
                continue
            if tag == "FBCSEM":
                raise ValueError("Repeated header")
            if expressions_only and tag not in ("M", "D", "E", "R", "END", "RECOVERY"):
                raise ValueError("Full-model fact in expression-only output")
            if tag in ("END", "RECOVERY"):
                footer = row
                continue
            counts[tag] += 1
            if tag == "M":
                if not row[1]:
                    raise ValueError("Module has no source path")
                module_expressions = 0
                self.contexts.append(None)
                self.primitives.append({})
            elif tag == "D":
                if not row[1] or row[1] in dependencies:
                    raise ValueError("Empty or duplicate dependency")
                dependencies.add(row[1])
            elif tag == "S":
                identity = number(row[1], 1)
                if identity in self.symbols:
                    raise ValueError("Duplicate symbol identity")
                self.symbols[identity] = row
                for value in row[3:11]:
                    number(value)
                reference("symbol", row[5])
                reference("symbol", row[11])
            elif tag == "T":
                identity = number(row[1], 1)
                reference("symbol", row[1], nullable=False)
                if row[4] not in TYPE_KINDS:
                    raise ValueError("Unknown type kind")
                if row[3] not in (*SYMBOL_CLASSES, "union") or row[17] not in STORAGE_KINDS:
                    raise ValueError("Unknown symbol class or storage")
                for value in row[6:15]:
                    number(value)
                for value in row[7:10]:
                    reference("symbol", value)
                if row[16] not in ("private", "protected", "public"):
                    raise ValueError("Unknown visibility")
                if row[18] not in ("source", "compiler", "runtime"):
                    raise ValueError("Unknown symbol origin")
                self.types[identity] = row
            elif tag == "B":
                reference("symbol", row[1], nullable=False)
                if row[2] not in ("declaration", "reference"):
                    raise ValueError("Unknown binding role")
                flag(row[3])
                source_range(row, 4)
            elif tag == "I":
                for value in row[1:4]:
                    reference("symbol", value, nullable=False)
                if row[4] not in IMPLICIT_KINDS or not row[11].startswith("sig2"):
                    raise ValueError("Unknown implicit selection or missing signature")
                flag(row[5])
                source_range(row, 6)
            elif tag == "P":
                reference("symbol", row[1], nullable=False)
                reference("symbol", row[5])
                number(row[6], 0)
                number(row[7], 0)
            elif tag == "V":
                reference("symbol", row[1], nullable=False)
                if not row[3] or row[4] not in TYPE_KINDS:
                    raise ValueError("Invalid variable type fact")
            elif tag in ("N", "E"):
                identity = number(row[1], 1)
                if tag == "N":
                    if identity in self.nodes:
                        raise ValueError("Duplicate AST identity")
                    self.nodes[identity] = row
                    if row[3] not in ("root", "left", "right"):
                        raise ValueError("Unknown child edge")
                    if row[3] != "root":
                        parent = number(row[2], 1)
                        edge = parent, row[3]
                        if parent >= identity or edge in child_edges:
                            raise ValueError("AST parent cycle or duplicate child edge")
                        child_edges.add(edge)
                    reference("symbol" if row[3] == "root" else "node", row[2], nullable=False)
                    code, kind = row[6:8]
                    for value in row[9:11]:
                        reference("symbol", value)
                else:
                    if identity in expression_ids:
                        raise ValueError("Duplicate expression identity")
                    expression_ids.add(identity)
                    module_expressions += 1
                    flag(row[2])
                    source_range(row, 3)
                    code, kind = row[10:12]
                    if not row[15]:
                        raise ValueError("Expression has no type spelling")
                    for value in row[13:15]:
                        reference("symbol", value)
                    if expressions_only and row[13:15] != ["0", "0"]:
                        raise ValueError("Expression-only output retains symbol identities")
                if kind not in ("none", "builtin", "overloaded") or (not code) != (kind == "none"):
                    raise ValueError("Inconsistent conceptual operator")
            elif tag == "R":
                if not expressions_only or not allow_recovery or row[1] != SCHEMA:
                    raise ValueError("Recovered module in complete-only output")
                if number(row[2], 0) != module_expressions:
                    raise ValueError("Recovered-module expression total mismatch")
            elif tag == "A":
                reference("symbol", row[1], nullable=False)
                rank = number(row[2], -1)
                dimension = number(row[3], -1)
                if row[4] == "runtime":
                    if dimension != -1 or row[5:] != ["", ""]:
                        raise ValueError("Runtime array advertises fixed bounds")
                elif row[4] in ("fixed", "unknown"):
                    if dimension < 0 or dimension >= rank:
                        raise ValueError("Array dimension outside rank")
                    number(row[5])
                    if row[4] == "fixed":
                        number(row[6])
                    elif row[6]:
                        raise ValueError("Unknown bound advertises an upper bound")
                else:
                    raise ValueError("Unknown array-bound kind")
            elif tag == "F":
                reference("symbol", row[1], nullable=False)
                if row[2] not in PROCEDURE_KINDS or row[3] not in CALLING_CONVENTIONS:
                    raise ValueError("Unknown procedure kind or calling convention")
                flag(row[6])
                reference("symbol", row[8])
                reference("symbol", row[11])
                for value in (row[4], row[5], row[12]):
                    number(value, 0)
                self.signatures[int(row[1])] = row
            elif tag == "G":
                reference("symbol", row[1], nullable=False)
                reference("symbol", row[2], nullable=False)
                reference("symbol", row[7])
                number(row[3], -1)
                if row[4] not in ("byval", "byref", "bydesc", "vararg"):
                    raise ValueError("Invalid parameter mode")
                flag(row[5])
                flag(row[8])
                number(row[6], -1)
                self.parameters[int(row[1])][int(row[3])] = row
            elif tag == "U":
                reference("symbol", row[1], nullable=False)
                if row[2] not in ("type", "union", "enum", "scope"):
                    raise ValueError("Unknown layout kind")
                reference("symbol", row[3])
                for value in row[4:]:
                    number(value, 0)
                self.layouts[int(row[1])] = row
            elif tag in ("C", "K", "H"):
                domains = ("symbol", "node", "expression") if tag == "C" else ("symbol", "node")
                if row[1] not in domains:
                    raise ValueError("Invalid metadata identity domain")
                reference(row[1], row[2], nullable=False)
                key = row[1], int(row[2])
                if tag == "C":
                    if row[3] in ("signed", "unsigned"):
                        number(row[4], 0 if row[3] == "unsigned" else None)
                    elif row[3] in ("bytes", "wide-units"):
                        width = 2 if row[3] == "bytes" else 8
                        if len(row[4]) % width or re.fullmatch(r"[0-9A-F]*", row[4]) is None:
                            raise ValueError("Invalid literal value encoding")
                    elif row[3] == "float64-bits":
                        if re.fullmatch(r"0x[0-9A-F]{16}", row[4]) is None:
                            raise ValueError("Invalid floating-point bits")
                    else:
                        raise ValueError("Unknown constant value kind")
                    self.constants[key] = row[3], row[4]
                elif tag == "K":
                    if not row[3]:
                        raise ValueError("Empty metadata property key")
                    self.properties[key][row[3]] = row[4]
                else:
                    if row[3] not in ("symbol", "node") or not row[5]:
                        raise ValueError("Invalid relationship target or kind")
                    reference(row[3], row[4], nullable=False)
                    number(row[6], 0)
            elif tag == "J":
                reference("node", row[1], nullable=False)
                reference("symbol", row[4], nullable=False)
                number(row[2], 0)
                number(row[3], 0)
            elif tag == "O":
                if not row[1] or row[2] not in ("builtin", "overloaded"):
                    raise ValueError("Unknown resolved source operator")
                reference("symbol", row[3], nullable=row[2] == "builtin")
                if row[2] == "builtin" and row[3] != "0":
                    raise ValueError("Builtin operator advertises a procedure target")
                flag(row[4])
                source_range(row, 5)
            elif tag == "Q":
                if not all(row[1:6]) or row[6] not in ("4", "8") or row[7] not in ("little", "big"):
                    raise ValueError("Invalid target context")
                number(row[8], 1)
                number(row[9], 0)
                if not self.contexts or self.contexts[-1] is not None or row[1] != self.records["M"][-1][1]:
                    raise ValueError("Missing, duplicate, or mismatched module context")
                self.contexts[-1] = row
            elif tag == "Y":
                identity = number(row[1], 0)
                if not row[2] or row[3] not in ("integer", "float", "string", "aggregate", "procedure", "unknown"):
                    raise ValueError("Invalid primitive datatype")
                for value in row[4:6]:
                    if value:
                        number(value, 0)
                flag(row[6])
                if not self.contexts or self.contexts[-1] is None:
                    raise ValueError("Primitive datatype precedes its target context")
                if identity in self.primitives[-1] or any(value[2] == row[2] for value in self.primitives[-1].values()):
                    raise ValueError("Duplicate primitive datatype identity or name")
                self.primitives[-1][identity] = row
            elif tag == "Z":
                reference("symbol", row[1], nullable=False)
                number(row[2], 0)
                if row[3] not in ("parameter", "parameter-reference", "stringify-reference", "text", "wide-text"):
                    raise ValueError("Invalid preprocessor token kind")
                if row[3] in ("parameter-reference", "stringify-reference"):
                    number(row[4], 0)
                if row[3] == "wide-text" and (len(row[4]) % 8 or re.fullmatch(r"[0-9A-F]*", row[4]) is None):
                    raise ValueError("Invalid wide preprocessor text")

        if footer is None or footer[1] != SCHEMA:
            raise ValueError("Missing or mismatched completeness footer")
        self.footer = footer
        if footer[0] == "END":
            expected = [counts[tag] for tag in ("M", "P", "S", "V", "N", "E", "B", "I", "D")]
            if [number(value, 0) for value in footer[2:11]] != expected:
                raise ValueError("Completion totals do not match records")
            if number(footer[12], 0) != sum(counts[tag] for tag in DETAIL_TAGS):
                raise ValueError("Metadata total does not match records")
            if counts["R"]:
                raise ValueError("Recovered module claimed complete")
            flag(footer[11])
        else:
            if not expressions_only or not allow_recovery:
                raise ValueError("Recovery is not a complete semantic model")
            expected = [counts[tag] for tag in ("M", "E", "R", "B", "D")]
            if [number(value, 0) for value in footer[2:7]] != expected or counts["R"] == 0:
                raise ValueError("Recovery totals do not match records")
            flag(footer[7])
        for domain, identity in references:
            available = self.symbols if domain == "symbol" else self.nodes if domain == "node" else expression_ids
            if identity not in available:
                raise ValueError(f"Dangling {domain} reference: {identity}")
        if expression_ids != set(range(1, counts["E"] + 1)):
            raise ValueError("Expression identity sequence has gaps")
        if set(self.symbols) != set(range(1, counts["S"] + 1)):
            raise ValueError("Symbol identity sequence has gaps")
        if set(self.nodes) != set(range(1, counts["N"] + 1)):
            raise ValueError("Node identity sequence has gaps")
        for identity in self.nodes:
            if "kind" not in self.properties["node", identity]:
                raise ValueError("AST node lacks its stable kind")
        if not expressions_only:
            self.validate_metadata()

    def validate_metadata(self) -> None:
        """Check completeness independently of the exporter's own record totals.

        Repeated metadata snapshots are legal. A retired procedure header can
        have only an S record, provided its canonical replacement has metadata.
        """
        canonical = {int(row[2]): int(row[4]) for row in self.relations("canonical-symbol")
                     if row[1] == row[3] == "symbol"}
        resolved: dict[int, int] = {}
        for identity in canonical:
            target = identity
            visited: set[int] = set()
            while target in canonical and target not in resolved:
                if target in visited:
                    raise ValueError("Cycle in canonical symbol relationships")
                visited.add(target)
                target = canonical[target]
            target = resolved.get(target, target)
            for previous in visited:
                resolved[previous] = target
        for identity, symbol in self.symbols.items():
            if identity not in self.types:
                if resolved.get(identity, identity) not in self.types:
                    raise ValueError(f"Symbol {identity} lacks completed metadata")
            else:
                symbol_class = number(symbol[3], 1)
                # TYPE and UNION share the compiler's aggregate symbol class.
                metadata_class = self.types[identity][3]
                if metadata_class == "union":
                    metadata_class = "type"
                if symbol_class > len(SYMBOL_CLASSES) or metadata_class != SYMBOL_CLASSES[symbol_class - 1]:
                    raise ValueError("Symbol class disagrees with its metadata")

        for identity, row in self.types.items():
            if row[3] == "procedure" and identity not in self.signatures:
                raise ValueError("Procedure lacks its signature")
            if row[3] in ("type", "union", "enum", "scope") and identity not in self.layouts:
                raise ValueError("Type or scope lacks its layout")
            if row[3] == "constant" and ("symbol", identity) not in self.constants:
                raise ValueError("Named constant lacks its exact value")
        for identity, signature in self.signatures.items():
            if identity not in self.types or self.types[identity][3] != "procedure":
                raise ValueError("Signature belongs to a non-procedure symbol")
            parameters = self.parameters[identity]
            explicit = {ordinal: row for ordinal, row in parameters.items() if ordinal >= 0}
            if set(explicit) != set(range(int(signature[4]))):
                raise ValueError("Signature parameter ordinals or arity are incomplete")
            if sum(int(row[5]) for row in explicit.values()) != int(signature[5]):
                raise ValueError("Optional parameter count disagrees with its signature")
            for ordinal, row in parameters.items():
                if (ordinal == -1) != (row[8] == "1"):
                    raise ValueError("Instance parameter has an incorrect ordinal")
                parameter = self.types.get(int(row[2]))
                if parameter is None or parameter[3] != "parameter" or int(parameter[8]) != identity:
                    raise ValueError("Formal parameter metadata or lexical owner is missing")

        for context, primitives in zip(self.contexts, self.primitives):
            if context is None or set(primitives) != set(range(len(PRIMITIVE_NAMES))):
                raise ValueError("Module target context or primitive table is incomplete")
            if tuple(primitives[identity][2] for identity in range(len(PRIMITIVE_NAMES))) != PRIMITIVE_NAMES:
                raise ValueError("Primitive datatype vocabulary is incomplete")
            names = {row[2]: row for row in primitives.values()}
            pointer_bytes = int(context[6])
            for name in ("pointer", "integer", "uinteger"):
                if name not in names or names[name][4] != str(pointer_bytes):
                    raise ValueError("Primitive storage disagrees with the target pointer width")
            for name in ("string", "ustring"):
                if name not in names or names[name][4:6] != [str(pointer_bytes * 3), str(pointer_bytes)]:
                    raise ValueError("Dynamic string descriptor storage is incomplete")

    @classmethod
    def read(cls, path: Path, **options: bool) -> Model:
        return cls(path.read_text(encoding="utf-8"), **options)

    def named(self, name: str, kind: str | None = None) -> list[int]:
        return [identity for identity, row in self.types.items()
                if row[2].casefold() == name.casefold() and (kind is None or row[3] == kind)]

    def symbol_name(self, identity: str | int) -> str:
        value = int(identity)
        return self.types[value][2] if value in self.types else self.symbols[value][2]

    def relations(self, kind: str) -> list[list[str]]:
        return [row for row in self.records["H"] if row[5] == kind]


# end of sidecar.py
