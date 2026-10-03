"""Project: FreeBASIC semantic sidecar tests
File: sidecar.py
Purpose: Read and independently validate schema 27 sidecars.
Responsibilities: Check shapes, references, ranges, escaping, and completion totals.
This file intentionally does NOT contain: compiler invocation or inferred semantics.
"""

from __future__ import annotations

from collections import Counter, defaultdict
import hashlib
from pathlib import Path
import re


SCHEMA = "27"
SHAPES = {
    "FBCSEM": 3, "M": 2, "D": 2, "S": 12, "B": 9, "I": 12,
    "P": 9, "V": 5, "N": 13, "E": 16, "R": 3, "END": 13,
    "RECOVERY": 8, "T": 19, "A": 7, "F": 13, "G": 9, "U": 12,
    "C": 5, "H": 7, "K": 5, "J": 5, "O": 10, "Q": 10, "Y": 7, "Z": 5, "ASM": 6, "DCL": 13,
    "CTX": 3, "OPT": 5, "USE": 4,
    "FILE": 7, "SRC": 14, "SRE": 3, "INC": 12, "MAP": 11, "ORIG": 4, "LOC": 12,
    "PPB": 14, "PPD": 11, "PPE": 9, "PPT": 13, "PPS": 10,
    "MD": 10, "MT": 6, "MI": 15, "MA": 12, "MS": 8, "MC": 5, "ME": 13, "ML": 13, "MR": 5,
    "ST": 17, "STE": 10, "BLK": 14, "BEND": 9, "OWN": 4, "CAP": 4, "ACC": 3, "SOP": 3,
    "EX": 13, "NT": 11, "PH": 5, "NP": 3, "EV": 5,
    "CB": 4, "CN": 4, "CE": 6, "CL": 4, "DI": 12,
}
PROVENANCE_TAGS = frozenset(("FILE", "SRC", "SRE", "INC", "MAP", "ORIG", "LOC", "PPB", "PPD", "PPE", "PPT", "PPS",
                            "MD", "MT", "MI", "MA", "MS", "MC", "ME", "ML", "MR", "ST", "STE", "BLK", "BEND", "OWN", "CAP", "ACC", "SOP", "EX", "DI"))
DETAIL_TAGS = frozenset(("T", "A", "F", "G", "U", "C", "H", "K", "J", "O", "Q", "Y", "Z", "ASM", "DCL", "CTX", "OPT", "USE", "NT", "PH", "NP", "EV", "CB", "CN", "CE", "CL")) | PROVENANCE_TAGS
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
NODE_KINDS = (
    "nop", "load", "assignment", "binary-operator", "unary-operator", "conversion",
    "address-of", "branch", "jump-table", "call", "construction", "stack", "memory",
    "loop", "comparison", "sequence", "constant", "variable", "index", "field",
    "dereference", "label", "argument", "offset", "declaration", "array", "conditional",
    "literal-text", "assembly", "data", "debug", "bounds-check", "pointer-check",
    "scope-begin", "scope-end", "scope-exit", "initializer", "initializer-padding",
    "initializer-assignment", "initializer-construction", "initializer-construction-list",
    "initializer-scope-begin", "initializer-scope-end", "procedure", "runtime-macro", "unicode-index",
)
NODE_REQUIRED = {
    "constant": {"literal-suffix"}, "load": {"result-load"},
    "assignment": {"operator-options", "initialization"},
    "call": {"call-kind", "argument-count", "copyback-count"},
    "argument": {"passing-mode", "bytes", "default-argument"},
    "conversion": {"conversion", "float-narrowing", "const-conversion"},
    "variable": {"byte-offset"}, "index": {"byte-offset", "index-scale"},
    "dereference": {"byte-offset"}, "offset": {"byte-offset"},
    "binary-operator": {"operator-options", "left-pointer-arithmetic", "right-pointer-arithmetic", "inverse-branch"},
    "unary-operator": {"operator-options", "left-pointer-arithmetic", "right-pointer-arithmetic", "inverse-branch"},
    "branch": {"operator-options", "left-pointer-arithmetic", "right-pointer-arithmetic", "inverse-branch", "branch-kind"},
    "jump-table": {"jump-bias", "jump-span"},
    "memory": {"memory-operation", "bytes", "fill-byte"},
    "stack": {"stack-operation"}, "runtime-macro": {"macro-operation"},
    "sequence": {"result-edge"},
    "initializer": {"byte-offset", "bytes"}, "initializer-padding": {"byte-offset", "bytes"},
    "initializer-assignment": {"byte-offset", "bytes"}, "initializer-construction": {"byte-offset", "bytes"},
    "initializer-construction-list": {"byte-offset", "element-count"},
    "initializer-scope-begin": {"byte-offset", "bytes", "array-initializer"},
    "initializer-scope-end": {"byte-offset", "bytes"},
    "assembly": {"assembly-token-count", "assembly-effects"},
}
NODE_FLAGS = frozenset(("literal-suffix", "result-load", "initialization", "conversion",
                         "float-narrowing", "const-conversion", "left-pointer-arithmetic",
                         "right-pointer-arithmetic", "inverse-branch", "array-initializer", "default-argument"))
NODE_UNSIGNED = frozenset(("operator-options", "argument-count", "copyback-count", "bytes",
                            "fill-byte", "element-count", "jump-bias", "jump-span", "assembly-token-count",
                            "auxiliary-ordinal", "vector-width"))
NODE_SIGNED = frozenset(("byte-offset", "index-scale"))
SOURCE_OPERATORS = {
    "binary": frozenset(("add", "subtract", "multiply", "divide", "integer-divide", "modulo", "and", "or",
                          "logical-and", "logical-or", "xor", "equivalence", "implication", "shift-left", "shift-right",
                          "power", "concatenate", "equal", "greater-than", "less-than", "not-equal",
                          "greater-or-equal", "less-or-equal", "identity-test")),
    "unary": frozenset(("negate", "unary-plus", "not")),
    "intrinsic-unary": frozenset(("absolute-value", "sign", "sine", "arcsine", "cosine", "arccosine", "tangent",
                                   "arctangent", "square-root", "logarithm", "exponential", "floor", "truncate", "fractional-part")),
    "intrinsic-binary": frozenset(("arctangent2",)),
    "group": frozenset(("parentheses",)),
    "cast": frozenset(("cast",)),
}
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


def preprocessing_location(row: list[str], offset: int) -> tuple[str, int, int, int, int]:
    """Generated tokens can retain a logical point, never an editable extent."""
    if row[offset - 1] == "1":
        return source_range(row, offset)
    path = row[offset]
    start_line = number(row[offset + 1], 1)
    start_column = number(row[offset + 2], 0)
    end_line = number(row[offset + 3], start_line)
    end_column = number(row[offset + 4], 0)
    if not path or end_line == start_line and end_column < start_column:
        raise ValueError("Reversed or missing preprocessing location")
    return path, start_line, start_column, end_line, end_column


class Model:
    def __init__(self, text: str, *, expressions_only: bool = False,
                 bindings_only: bool = False,
                 allow_recovery: bool = False) -> None:
        if expressions_only and bindings_only:
            raise ValueError("Sidecar cannot be both expressions-only and bindings-only")
        self.expressions_only = expressions_only
        self.bindings_only = bindings_only
        self.rows: list[list[str]] = []
        self.records: dict[str, list[list[str]]] = defaultdict(list)
        self.symbols: dict[int, list[str]] = {}
        self.types: dict[int, list[str]] = {}
        self.normalized_types: dict[tuple[str, int, str], list[str]] = {}
        self.diagnostics: dict[int, list[str]] = {}
        self.nodes: dict[int, list[str]] = {}
        self.properties: dict[tuple[str, int], dict[str, str]] = defaultdict(dict)
        self.constants: dict[tuple[str, int], tuple[str, str]] = {}
        self.signatures: dict[int, list[str]] = {}
        self.parameters: dict[int, dict[int, list[str]]] = defaultdict(dict)
        self.layouts: dict[int, list[str]] = {}
        self.configurations: dict[int, int] = {}
        self.options: dict[int, dict[tuple[str, str], int]] = defaultdict(dict)
        self.context_uses: dict[tuple[str, int], int] = {}
        self.files: dict[int, list[str]] = {}
        self.source_contexts: dict[int, list[str]] = {}
        self.source_endings: dict[int, str] = {}
        self.origins: dict[tuple[str, int], int] = {}
        self.physical_locations: dict[tuple[str, int, str], list[str]] = {}
        self.statements: dict[int, list[str]] = {}
        self.statement_endings: dict[int, list[str]] = {}
        self.constructs: dict[int, list[str]] = {}
        self.construct_endings: dict[int, list[str]] = {}
        self.statement_owners: dict[tuple[str, int], int] = {}
        self.capabilities: dict[int, dict[str, str]] = defaultdict(dict)
        self.access_roles: dict[int, str] = {}
        self.statement_operations: dict[int, list[str]] = defaultdict(list)
        self.expression_operands: dict[int, list[str]] = {}
        statement_stack: list[int] = []
        construct_stack: list[int] = []
        remap_ids: set[int] = set()
        self.conditional_branches: dict[int, list[str]] = {}
        self.conditional_decisions: dict[int, list[str]] = {}
        self.conditional_groups: dict[int, list[int]] = defaultdict(list)
        self.macro_definitions: dict[int, list[str]] = {}
        self.macro_tokens: dict[int, list[list[str]]] = defaultdict(list)
        self.macro_invocations: dict[int, list[str]] = {}
        self.macro_arguments: dict[int, dict[int, list[str]]] = defaultdict(dict)
        self.macro_segments: dict[int, list[list[str]]] = defaultdict(list)
        self.macro_callbacks: dict[int, list[str]] = {}
        self.macro_results: dict[int, list[str]] = {}
        self.macro_origins: dict[tuple[str, int, str], int] = {}
        macro_lifecycle_ids: set[int] = set()
        conditional_stack: list[int] = []
        conditional_endings: set[int] = set()
        conditional_probe_ids: set[int] = set()
        conditional_skip_ids: set[int] = set()
        self.contexts: list[list[str] | None] = []
        self.primitives: list[dict[int, list[str]]] = []
        if not text.endswith("\n"):
            raise ValueError("Sidecar is truncated")
        counts: Counter[str] = Counter()
        expression_ids: set[int] = set()
        declaration_ids: set[int] = set()
        child_edges: set[tuple[int, str]] = set()
        dependencies: set[str] = set()
        references: list[tuple[str, int]] = []
        subject_modules: dict[tuple[str, int], int] = {}
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
            if expressions_only and tag not in ("M", "D", "E", "R", "END", "RECOVERY") and tag not in PROVENANCE_TAGS:
                raise ValueError("Full-model fact in expression-only output")
            if bindings_only and tag not in ("M", "D", "S", "B", "I", "END") and tag not in PROVENANCE_TAGS:
                raise ValueError("Non-binding fact in bindings-only output")
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
            elif tag == "DI":
                identity = number(row[1], 1)
                if identity in self.diagnostics or row[2] not in ("error", "warning"):
                    raise ValueError("Invalid or repeated diagnostic")
                number(row[3], 0)
                number(row[8])
                source, statement = number(row[9], 0), number(row[10], 0)
                if source and (source not in self.source_contexts or int(self.source_contexts[source][4]) != counts["M"]):
                    raise ValueError("Diagnostic lacks its source occurrence")
                if statement and (statement not in self.statements or int(self.statements[statement][7]) != counts["M"]):
                    raise ValueError("Diagnostic lacks its source statement")
                reference("symbol", row[11])
                self.diagnostics[identity] = row
            elif tag == "NT":
                if row[1] not in ("symbol", "node", "expression") or row[3] != "value":
                    raise ValueError("Invalid normalized type subject or role")
                reference(row[1], row[2], nullable=False)
                reference("symbol", row[5])
                pointers = number(row[6], 0)
                flag(row[7])
                if pointers > 8 or len(row[8]) != pointers + 1 or re.fullmatch(r"[01]+", row[8]) is None:
                    raise ValueError("Invalid normalized type qualifiers")
                if row[4] not in PRIMITIVE_NAMES or row[10] and row[10] not in PRIMITIVE_NAMES:
                    raise ValueError("Unknown normalized type name")
                if row[9]:
                    number(row[9], 1)
                elif pointers:
                    raise ValueError("Pointer type lacks its storage width")
                key = row[1], int(row[2]), row[3]
                # Symbol snapshots can be refreshed after a forward type or
                # procedure is completed, just like the existing T records.
                if row[1] != "symbol" and key in self.normalized_types:
                    raise ValueError("Repeated normalized type subject")
                self.normalized_types[key] = row
            elif tag == "B":
                subject_modules["binding", counts["B"]] = counts["M"]
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
                subject_modules["node" if tag == "N" else "expression", identity] = counts["M"]
                if tag == "N":
                    if identity in self.nodes:
                        raise ValueError("Duplicate AST identity")
                    self.nodes[identity] = row
                    if row[3] not in ("root", "left", "right", "copyback", "profile-begin", "profile-end"):
                        raise ValueError("Unknown child edge")
                    if row[3] != "root":
                        parent = number(row[2], 1)
                        edge = parent, row[3]
                        if parent >= identity or (row[3] in ("left", "right") and edge in child_edges):
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
            elif tag == "CAP":
                module = number(row[1], 1)
                if module != len(self.contexts) or not row[2] or row[2] in self.capabilities[module]:
                    raise ValueError("Invalid or repeated capability owner")
                if row[3] not in ("available", "partial", "unavailable"):
                    raise ValueError("Unknown semantic capability coverage")
                self.capabilities[module][row[2]] = row[3]
            elif tag == "EX":
                identity = number(row[1], 1)
                left, right, sourceid = [number(value, 0) for value in row[4:7]]
                if identity in self.expression_operands or row[2] not in SOURCE_OPERATORS or row[3] not in SOURCE_OPERATORS[row[2]]:
                    raise ValueError("Repeated source operands or unknown operator family/code")
                if not (left or right) or left >= identity or right >= identity:
                    raise ValueError("Source operands are missing, cyclic, or forward references")
                if row[2] not in ("binary", "intrinsic-binary") and right:
                    raise ValueError("Unary source operation advertises a right operand")
                if sourceid not in self.source_contexts or int(self.source_contexts[sourceid][4]) != counts["M"]:
                    raise ValueError("Source operation lacks its module/source occurrence")
                reference("expression", row[1], nullable=False)
                reference("expression", row[4])
                reference("expression", row[5])
                flag(row[7])
                preprocessing_location(row, 8)
                self.expression_operands[identity] = row
            elif tag == "ACC":
                reference("binding", row[1], nullable=False)
                identity = int(row[1])
                if identity in self.access_roles or row[2] not in ("read", "write", "read-write", "address", "byref", "callee"):
                    raise ValueError("Invalid or repeated source access role")
                self.access_roles[identity] = row[2]
            elif tag == "SOP":
                identity = number(row[1], 1)
                if identity not in self.statements or row[2] not in ("file-open", "file-close", "file-seek", "file-get", "file-put", "file-lock", "file-unlock", "file-rename", "line-input"):
                    raise ValueError("Invalid source statement operation")
                self.statement_operations[identity].append(row[2])
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
                domains = ("symbol", "node", "expression") if tag in ("C", "H") else ("symbol", "node")
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
                    if row[3] not in ("symbol", "node", "expression", "binding") or not row[5]:
                        raise ValueError("Invalid relationship target or kind")
                    reference(row[3], row[4], nullable=False)
                    number(row[6], 0)
            elif tag == "J":
                reference("node", row[1], nullable=False)
                reference("symbol", row[4], nullable=False)
                number(row[2], 0)
                number(row[3], 0)
            elif tag == "ASM":
                reference("node", row[1], nullable=False)
                number(row[2], 0)
                if row[3] not in ("text", "symbol"):
                    raise ValueError("Unknown assembly token kind")
                reference("symbol", row[4], nullable=row[3] == "text")
                if (row[3] == "text" and row[4] != "0") or (row[3] == "symbol" and row[5]):
                    raise ValueError("Assembly token has an incompatible payload")
            elif tag == "DCL":
                identity = number(row[1], 1)
                subject_modules["declaration", identity] = counts["M"]
                if identity in declaration_ids:
                    raise ValueError("Repeated declaration occurrence identity")
                declaration_ids.add(identity)
                reference("symbol", row[2], nullable=False)
                if row[3] not in ("parameter-prototype", "parameter-definition", "procedure-prototype", "procedure-definition", "implicit-variable"):
                    raise ValueError("Unknown declaration role")
                flag(row[5])
                source_range(row, 6)
                if number(row[11], 1) != counts["M"]:
                    raise ValueError("Declaration belongs to another module")
                number(row[12], 0)
            elif tag == "CTX":
                identity = number(row[1], 1)
                module = number(row[2], 1)
                if identity in self.configurations or module != counts["M"]:
                    raise ValueError("Duplicate or misplaced configuration context")
                self.configurations[identity] = module
            elif tag == "OPT":
                identity = number(row[1], 1)
                key = row[2], row[3]
                if identity not in self.configurations or row[2] not in ("compiler", "language-default", "language-policy") or not row[3]:
                    raise ValueError("Option lacks a declared configuration context")
                if key in self.options[identity]:
                    raise ValueError("Repeated configuration option")
                self.options[identity][key] = number(row[4])
            elif tag == "USE":
                if row[1] not in ("symbol", "node", "expression", "declaration"):
                    raise ValueError("Unknown configuration use domain")
                reference(row[1], row[2], nullable=False)
                context = number(row[3], 1)
                if context not in self.configurations:
                    raise ValueError("Configuration use is dangling")
                key = row[1], number(row[2], 1)
                if key in self.context_uses:
                    raise ValueError("Repeated configuration attachment")
                self.context_uses[key] = context
            elif tag == "O":
                if not row[1] or row[2] not in ("builtin", "overloaded"):
                    raise ValueError("Unknown resolved source operator")
                reference("symbol", row[3], nullable=row[2] == "builtin")
                if row[2] == "builtin" and row[3] != "0":
                    raise ValueError("Builtin operator advertises a procedure target")
                flag(row[4])
                source_range(row, 5)
            elif tag == "FILE":
                identity = number(row[1], 1)
                if identity in self.files or not row[2]:
                    raise ValueError("Duplicate or nameless file revision")
                number(row[3], 0)
                if row[5] not in ("unmarked-bytes", "utf-8-bom", "utf-16le", "utf-16be", "utf-32le", "utf-32be", "unknown"):
                    raise ValueError("Unknown source encoding")
                if row[6] not in ("regular", "stream"):
                    raise ValueError("Unknown source file kind")
                if row[6] == "regular" and re.fullmatch(r"[0-9a-f]{64}", row[4]) is None:
                    raise ValueError("Regular source has no exact SHA-256 revision")
                if row[6] == "stream" and (row[4] or row[3] != "0"):
                    raise ValueError("Stream source falsely advertises a file revision")
                self.files[identity] = row
            elif tag == "SRC":
                identity = number(row[1], 1)
                parent = number(row[2], 0)
                fileid = number(row[3], 1)
                module = number(row[4], 1)
                depth = number(row[6], 0)
                if identity in self.source_contexts or fileid not in self.files or module != counts["M"]:
                    raise ValueError("Source context lacks a declared file/module")
                if row[5] not in ("module", "include", "preinclude") or depth > 16:
                    raise ValueError("Invalid source context kind/depth")
                if parent:
                    if parent not in self.source_contexts or depth != int(self.source_contexts[parent][6]) + 1:
                        raise ValueError("Source include parent/depth is invalid")
                    if row[5] == "module":
                        raise ValueError("Module source has an include parent")
                elif row[5] != "module" or depth != 0:
                    raise ValueError("Included source has no parent")
                flag(row[8])
                if row[9]:
                    source_range(row, 9)
                elif row[8:] != ["0", "", "0", "0", "0", "0"]:
                    raise ValueError("Unanchored source context has invented coordinates")
                self.source_contexts[identity] = row
            elif tag == "SRE":
                identity = number(row[1], 1)
                if identity not in self.source_contexts or identity in self.source_endings:
                    raise ValueError("Unknown or multiply closed source context")
                if row[2] not in ("verified", "unverified-stream", "changed-or-unreadable"):
                    raise ValueError("Unknown source revision verification state")
                self.source_endings[identity] = row[2]
            elif tag == "INC":
                number(row[1], 1)
                parent = number(row[2], 1)
                if parent not in self.source_contexts or not row[3]:
                    raise ValueError("Include outcome lacks parent/request")
                if row[5] not in ("opened", "include-once", "pragma-once", "not-found", "open-failed", "depth-limit"):
                    raise ValueError("Unknown include outcome")
                if row[5] in ("opened", "include-once", "pragma-once", "open-failed") and not row[4]:
                    raise ValueError("Resolved include has no file path")
                flag(row[6])
                if row[7]:
                    source_range(row, 7)
                elif row[6:] != ["0", "", "0", "0", "0", "0"]:
                    raise ValueError("Unanchored include has invented coordinates")
            elif tag == "MAP":
                if number(row[1], 1) not in self.source_contexts or not row[3]:
                    raise ValueError("Remap lacks physical source or logical file")
                number(row[2], 0)
                flag(row[4])
                source_range(row, 5)
                identity = number(row[10], 1)
                if identity in remap_ids:
                    raise ValueError("Repeated source remap identity")
                remap_ids.add(identity)
            elif tag == "LOC":
                identity, sourceid = number(row[2], 1), number(row[4], 1)
                if sourceid not in self.source_contexts or not row[3]:
                    raise ValueError("Physical location lacks its revision context or role")
                start_line, start_column, end_line, end_column = [number(value, 0) for value in row[5:9]]
                if start_line < 1 or end_line < start_line or end_line == start_line and end_column < start_column:
                    raise ValueError("Physical location is reversed or empty")
                start_byte, end_byte = number(row[9], -1), number(row[10], -1)
                if row[11] == "mapped":
                    fileid = int(self.source_contexts[sourceid][3])
                    if self.files[fileid][6] != "regular" or start_byte < 0 or end_byte < start_byte or end_byte > int(self.files[fileid][3]):
                        raise ValueError("Physical location exceeds its captured source revision")
                elif row[11] != "unverified" or start_byte != -1 or end_byte != -1:
                    raise ValueError("Unverified physical location advertises byte coordinates")
                if row[1] in ("binding", "declaration", "expression"):
                    reference(row[1], row[2], nullable=False)
                elif row[1] == "conditional":
                    if identity not in self.conditional_branches:
                        raise ValueError("Physical conditional location is dangling")
                elif row[1] == "conditional-end":
                    if identity not in conditional_endings:
                        raise ValueError("Physical conditional closure is dangling")
                elif row[1] == "defined-probe":
                    if identity not in conditional_probe_ids:
                        raise ValueError("Physical defined-name probe is dangling")
                elif row[1] in ("macro-attempt", "macro-argument"):
                    if identity not in self.macro_invocations:
                        raise ValueError("Physical macro location is dangling")
                    if row[1] == "macro-argument":
                        match = re.fullmatch(r"formal-([0-9]+)", row[3])
                        if match is None or int(match[1]) not in self.macro_arguments[identity]:
                            raise ValueError("Physical macro argument location lacks its formal")
                elif row[1] == "macro-lifetime":
                    if identity not in macro_lifecycle_ids:
                        raise ValueError("Physical macro lifetime location is dangling")
                elif row[1] == "source-context":
                    if identity not in self.source_contexts:
                        raise ValueError("Physical include context location is dangling")
                elif row[1] == "include":
                    if not any(int(item[1]) == identity for item in self.records["INC"]):
                        raise ValueError("Physical include occurrence location is dangling")
                elif row[1] == "remap":
                    if identity not in remap_ids:
                        raise ValueError("Physical remap location is dangling")
                elif row[1] == "statement":
                    if identity not in self.statement_endings:
                        raise ValueError("Physical statement range has no observed end")
                    if sourceid != int(self.statements[identity][5]):
                        raise ValueError("Physical statement range belongs to another source occurrence")
                elif row[1] == "construct":
                    if identity not in self.construct_endings:
                        raise ValueError("Physical construct range has no observed end")
                    if sourceid != int(self.constructs[identity][6]):
                        raise ValueError("Physical construct range belongs to another source occurrence")
                else:
                    raise ValueError("Unknown physical location domain")
                key = row[1], identity, row[3]
                if key in self.physical_locations:
                    raise ValueError("Repeated physical location attachment")
                self.physical_locations[key] = row
            elif tag == "ORIG":
                if row[1] not in ("node", "declaration", "expression", "binding"):
                    raise ValueError("Unknown source origin domain")
                reference(row[1], row[2], nullable=False)
                sourceid = number(row[3], 1)
                if sourceid not in self.source_contexts:
                    raise ValueError("Source origin is dangling")
                key = row[1], number(row[2], 1)
                if key in self.origins:
                    raise ValueError("Repeated source origin attachment")
                self.origins[key] = sourceid
            elif tag == "ST":
                identity, parent, construct, owner, sourceid, context, module, ordinal, token, token_class = [number(value, 0) for value in row[1:11]]
                if identity == 0 or identity in self.statements or parent != (statement_stack[-1] if statement_stack else 0):
                    raise ValueError("Statement identity or nesting is invalid")
                if construct != (construct_stack[-1] if construct_stack else 0):
                    raise ValueError("Statement belongs to a different active construct")
                if sourceid not in self.source_contexts or sourceid in self.source_endings or module != counts["M"]:
                    raise ValueError("Statement lacks a live source/module context")
                if int(self.source_contexts[sourceid][4]) != module:
                    raise ValueError("Statement source belongs to another module")
                if context and self.configurations.get(context) != module:
                    raise ValueError("Statement configuration is dangling")
                if not expressions_only and not bindings_only and context == 0:
                    raise ValueError("Statement lacks configuration")
                reference("symbol", row[4])
                flag(row[11])
                preprocessing_location(row, 12)
                self.statements[identity] = row
                statement_stack.append(identity)
            elif tag == "STE":
                identity = number(row[1], 1)
                if not statement_stack or statement_stack[-1] != identity or identity in self.statement_endings:
                    raise ValueError("Statement end is dangling or out of order")
                if row[2] not in ("declaration", "compound", "call-or-assignment", "intrinsic", "assembly",
                                  "pointer-or-assignment", "label", "aggregate-member", "enumerator", "assembly-line", "unmatched"):
                    raise ValueError("Unknown parser statement route")
                if row[3] not in ("parsed", "recovered", "unmatched") or (row[2] == "unmatched") != (row[3] == "unmatched"):
                    raise ValueError("Statement outcome disagrees with its route")
                flag(row[4])
                preprocessing_location(row, 5)
                statement_stack.pop()
                self.statement_endings[identity] = row
            elif tag == "BLK":
                identity, parent, statement, owner = [number(value, 0) for value in row[1:5]]
                sourceid, context = number(row[6], 1), number(row[7], 0)
                if identity == 0 or identity in self.constructs or parent != (construct_stack[-1] if construct_stack else 0):
                    raise ValueError("Construct identity or nesting is invalid")
                if statement != (statement_stack[-1] if statement_stack else 0):
                    raise ValueError("Construct has no active opening statement")
                if row[5] not in ("if", "for", "do", "while", "select", "with", "scope", "namespace", "extern", "procedure", "type", "union", "enum", "assembly"):
                    raise ValueError("Unknown source construct kind")
                if sourceid not in self.source_contexts or sourceid in self.source_endings:
                    raise ValueError("Construct lacks a live source context")
                if int(self.source_contexts[sourceid][4]) != counts["M"]:
                    raise ValueError("Construct source belongs to another module")
                if context and self.configurations.get(context) != counts["M"]:
                    raise ValueError("Construct configuration is dangling")
                if not expressions_only and not bindings_only and context == 0:
                    raise ValueError("Construct lacks configuration")
                reference("symbol", row[4])
                flag(row[8])
                preprocessing_location(row, 9)
                construct_stack.append(identity)
                self.constructs[identity] = row
            elif tag == "BEND":
                identity, statement = number(row[1], 1), number(row[2], 0)
                if not construct_stack or construct_stack[-1] != identity or identity in self.construct_endings:
                    raise ValueError("Construct end is dangling or out of order")
                if statement != (statement_stack[-1] if statement_stack else 0):
                    raise ValueError("Construct closure belongs to another statement")
                flag(row[3])
                preprocessing_location(row, 4)
                construct_stack.pop()
                self.construct_endings[identity] = row
            elif tag == "OWN":
                subject, statement = number(row[2], 1), number(row[3], 1)
                if statement not in self.statements or row[1] not in ("node", "binding", "declaration", "expression"):
                    raise ValueError("Statement fact ownership is dangling or unknown")
                if row[1] != "node" and statement != (statement_stack[-1] if statement_stack else 0):
                    raise ValueError("Source fact belongs to another active statement")
                reference(row[1], row[2], nullable=False)
                key = row[1], subject
                if key in self.statement_owners:
                    raise ValueError("Repeated statement ownership attachment")
                self.statement_owners[key] = statement
            elif tag == "PPB":
                identity, parent, group, sourceid, configuration, ordinal = [number(value, 0) for value in row[1:7]]
                if identity == 0 or identity in self.conditional_branches:
                    raise ValueError("Duplicate or empty conditional branch")
                if sourceid not in self.source_contexts or sourceid in self.source_endings:
                    raise ValueError("Conditional branch lacks a live source context")
                if int(self.source_contexts[sourceid][4]) != counts["M"]:
                    raise ValueError("Conditional branch belongs to another module")
                if configuration and configuration not in self.configurations:
                    raise ValueError("Conditional branch configuration is dangling")
                if configuration and self.configurations[configuration] != counts["M"]:
                    raise ValueError("Conditional branch configuration belongs to another module")
                if not expressions_only and not bindings_only and configuration == 0:
                    raise ValueError("Conditional branch lacks its configuration")
                if row[7] not in ("if", "ifdef", "ifndef", "elseif", "elseifdef", "elseifndef", "else"):
                    raise ValueError("Unknown conditional directive")
                flag(row[8])
                preprocessing_location(row, 9)
                if ordinal == 0:
                    expected_parent = self.conditional_groups[conditional_stack[-1]][-1] if conditional_stack else 0
                    if group != identity or group in self.conditional_groups or parent != expected_parent:
                        raise ValueError("Invalid conditional nesting or first branch")
                    if row[7] not in ("if", "ifdef", "ifndef"):
                        raise ValueError("Conditional group starts with a successor directive")
                    conditional_stack.append(group)
                else:
                    if not conditional_stack or conditional_stack[-1] != group:
                        raise ValueError("Conditional successor has no active group")
                    members = self.conditional_groups[group]
                    if ordinal != len(members) or parent != int(self.conditional_branches[members[0]][2]):
                        raise ValueError("Conditional branch ordinal or parent changed")
                    previous = self.conditional_branches[members[-1]]
                    previous_decision = self.conditional_decisions.get(members[-1])
                    inactive_scan = previous_decision is not None and previous_decision[2] == "parent-inactive"
                    if row[7] in ("if", "ifdef", "ifndef") or previous[7] == "else" and not inactive_scan:
                        raise ValueError("Invalid conditional successor order")
                self.conditional_branches[identity] = row
                self.conditional_groups[group].append(identity)
            elif tag == "PPD":
                identity = number(row[1], 1)
                if identity not in self.conditional_branches or identity in self.conditional_decisions:
                    raise ValueError("Conditional decision is dangling or repeated")
                if row[2] not in ("evaluated", "invalid", "parent-inactive", "unconditional"):
                    raise ValueError("Unknown conditional evaluation state")
                if row[2] == "evaluated":
                    flag(row[3])
                elif row[3]:
                    raise ValueError("Unevaluated condition advertises a result")
                flag(row[4])
                if row[2] in ("invalid", "parent-inactive") and row[4] != "0":
                    raise ValueError("Invalid or inactive condition selected a branch")
                is_else = self.conditional_branches[identity][7] == "else"
                if (row[2] == "unconditional") != is_else and row[2] != "parent-inactive":
                    raise ValueError("Condition evaluation disagrees with directive kind")
                flag(row[5])
                preprocessing_location(row, 6)
                self.conditional_decisions[identity] = row
            elif tag == "PPE":
                group, sourceid = number(row[1], 1), number(row[2], 1)
                if not conditional_stack or conditional_stack[-1] != group or group in conditional_endings:
                    raise ValueError("Conditional closure is dangling or out of order")
                if sourceid not in self.source_contexts or sourceid in self.source_endings:
                    raise ValueError("Conditional closure lacks a live source context")
                flag(row[3])
                preprocessing_location(row, 4)
                conditional_stack.pop()
                conditional_endings.add(group)
            elif tag == "PPT":
                identity, branch, sourceid, symbol = [number(value, 0) for value in row[1:5]]
                if identity == 0 or identity in conditional_probe_ids:
                    raise ValueError("Duplicate or empty defined-name probe")
                conditional_probe_ids.add(identity)
                if branch and branch not in self.conditional_branches:
                    raise ValueError("Defined-name probe conditional is dangling")
                expected_branch = self.conditional_groups[conditional_stack[-1]][-1] if conditional_stack else 0
                if branch != expected_branch:
                    raise ValueError("Defined-name probe belongs to a different active conditional")
                if sourceid not in self.source_contexts or sourceid in self.source_endings:
                    raise ValueError("Defined-name probe lacks a live source context")
                flag(row[6])
                if row[6] == "0" and symbol or row[6] == "1" and not expressions_only and symbol == 0:
                    raise ValueError("Defined-name probe fabricates or loses a symbol")
                reference("symbol", row[4])
                flag(row[7])
                preprocessing_location(row, 8)
            elif tag == "PPS":
                identity, branch, sourceid = [number(value, 1) for value in row[1:4]]
                if identity in conditional_skip_ids or branch not in self.conditional_branches:
                    raise ValueError("Inactive region is repeated or dangling")
                if branch not in {self.conditional_groups[group][-1] for group in conditional_stack}:
                    raise ValueError("Inactive region belongs to a closed conditional")
                conditional_skip_ids.add(identity)
                if sourceid not in self.source_contexts or sourceid in self.source_endings:
                    raise ValueError("Inactive region lacks a live source context")
                flag(row[4])
                source_range(row, 5)
            elif tag == "MD":
                identity, symbol, module, context = [number(value, 0) for value in row[1:5]]
                if identity == 0 or identity in self.macro_definitions or module != counts["M"] or not row[5]:
                    raise ValueError("Duplicate, misplaced or nameless macro definition")
                if row[6] not in ("text", "tokens", "define-callback", "macro-callback"):
                    raise ValueError("Unknown macro definition kind")
                if context and self.configurations.get(context) != module:
                    raise ValueError("Macro definition configuration is dangling")
                if not expressions_only and not bindings_only and context == 0:
                    raise ValueError("Macro definition lacks configuration")
                if not expressions_only and symbol == 0:
                    raise ValueError("Macro definition lacks its selected symbol")
                reference("symbol", row[2])
                if number(row[7], 0) > 32 or number(row[8], 0) > 15:
                    raise ValueError("Macro definition exceeds its parameter or flag vocabulary")
                flag(row[9])
                self.macro_definitions[identity] = row
            elif tag == "MT":
                identity = number(row[1], 1)
                if identity not in self.macro_definitions:
                    raise ValueError("Macro token definition is dangling")
                number(row[2], 0)
                if row[3] not in ("parameter", "text", "wide-text", "parameter-reference", "stringify-reference"):
                    raise ValueError("Unknown macro definition token")
                flag(row[5])
                if row[3] in ("parameter-reference", "stringify-reference"):
                    number(row[4], 0)
                if row[3] == "wide-text":
                    self.macro_units("wide-units", row[4])
                self.macro_tokens[identity].append(row)
            elif tag == "MI":
                identity, parent, definition, sourceid, context, conditional = [number(value, 0) for value in row[1:7]]
                if identity == 0 or identity in self.macro_invocations or definition not in self.macro_definitions:
                    raise ValueError("Macro attempt identity or definition is invalid")
                if parent and parent not in self.macro_invocations:
                    raise ValueError("Macro expansion parent is dangling or cyclic")
                if sourceid not in self.source_contexts or sourceid in self.source_endings:
                    raise ValueError("Macro attempt lacks a live source context")
                if context and self.configurations.get(context) != counts["M"]:
                    raise ValueError("Macro attempt configuration is dangling")
                if not expressions_only and not bindings_only and context == 0:
                    raise ValueError("Macro attempt lacks configuration")
                if conditional and conditional not in self.conditional_branches:
                    raise ValueError("Macro attempt conditional is dangling")
                if row[7] not in ("normal", "inactive", "evaluation") or row[8] not in ("root", "replacement", "argument", "callback"):
                    raise ValueError("Unknown macro expansion phase or parent relation")
                if (parent == 0) != (row[8] == "root"):
                    raise ValueError("Macro parent relation contradicts its parent")
                flag(row[9])
                preprocessing_location(row, 10)
                self.macro_invocations[identity] = row
            elif tag == "MA":
                identity, ordinal = number(row[1], 1), number(row[2], 0)
                if identity not in self.macro_invocations or ordinal in self.macro_arguments[identity]:
                    raise ValueError("Macro argument is dangling or repeated")
                self.macro_units(row[3], row[4])
                flag(row[5])
                flag(row[6])
                if row[5] == "1":
                    preprocessing_location(row, 7)
                elif row[6] != "0" or row[7] or any(number(value, 0) for value in row[8:]):
                    raise ValueError("Absent macro argument fabricates a source range")
                self.macro_arguments[identity][ordinal] = row
            elif tag == "MS":
                identity = number(row[1], 1)
                if identity not in self.macro_invocations:
                    raise ValueError("Macro substitution is dangling")
                number(row[2], 0)
                number(row[3], -1)
                number(row[4], -1)
                if row[5] not in ("text", "parameter", "stringify", "callback", "definition-text", "restored-delimiter"):
                    raise ValueError("Unknown macro substitution kind")
                number(row[6], 0)
                number(row[7], 0)
                self.macro_segments[identity].append(row)
            elif tag == "MC":
                identity = number(row[1], 1)
                if identity not in self.macro_invocations or identity in self.macro_callbacks:
                    raise ValueError("Macro callback output is dangling or repeated")
                self.macro_units(row[2], row[4])
                number(row[3], 0)
                self.macro_callbacks[identity] = row
            elif tag == "ME":
                identity = number(row[1], 1)
                if identity not in self.macro_invocations or identity in self.macro_results:
                    raise ValueError("Macro result is dangling or repeated")
                if row[2] not in ("expanded", "not-invoked", "unsupported", "recursive", "failed", "recovered"):
                    raise ValueError("Unknown macro expansion outcome")
                if len(self.macro_units(row[3], row[5])) != number(row[4], 0):
                    raise ValueError("Macro output length disagrees with its replacement")
                number(row[6], 0)
                flag(row[7])
                preprocessing_location(row, 8)
                self.macro_results[identity] = row
            elif tag == "ML":
                identity, definition = number(row[1], 1), number(row[2], 0)
                if identity in macro_lifecycle_ids or definition and definition not in self.macro_definitions:
                    raise ValueError("Macro lifetime identity or definition is invalid")
                macro_lifecycle_ids.add(identity)
                if row[3] not in ("define", "identical", "definition-rejected", "undef", "undef-missing"):
                    raise ValueError("Unknown macro lifetime action")
                if row[3] in ("define", "identical", "undef") and definition == 0:
                    raise ValueError("Macro lifetime action lacks its definition")
                if row[3] == "undef-missing" and definition:
                    raise ValueError("Missing undef advertises a selected macro")
                sourceid, context = number(row[4], 1), number(row[5], 0)
                if sourceid not in self.source_contexts or sourceid in self.source_endings:
                    raise ValueError("Macro lifetime action lacks a live source context")
                if context and self.configurations.get(context) != counts["M"]:
                    raise ValueError("Macro lifetime configuration is dangling")
                if not row[6]:
                    raise ValueError("Macro lifetime action has no spelling")
                flag(row[7])
                preprocessing_location(row, 8)
            elif tag == "MR":
                subject, expansion = number(row[2], 1), number(row[3], 1)
                if expansion not in self.macro_invocations or not row[4]:
                    raise ValueError("Macro origin is dangling or lacks its role")
                if row[1] in ("expression", "binding", "declaration", "symbol"):
                    reference(row[1], row[2], nullable=False)
                elif row[1] == "conditional":
                    if subject not in self.conditional_branches:
                        raise ValueError("Macro conditional origin is dangling")
                elif row[1] == "defined-probe":
                    if subject not in conditional_probe_ids:
                        raise ValueError("Macro defined-name origin is dangling")
                elif row[1] == "macro-argument":
                    if subject not in self.macro_invocations:
                        raise ValueError("Macro argument origin is dangling")
                elif row[1] == "statement":
                    if subject not in self.statements:
                        raise ValueError("Macro statement origin is dangling")
                elif row[1] == "construct":
                    if subject not in self.constructs:
                        raise ValueError("Macro construct origin is dangling")
                else:
                    raise ValueError("Unknown macro origin domain")
                key = row[1], subject, row[4]
                if key in self.macro_origins:
                    raise ValueError("Repeated macro origin attachment")
                self.macro_origins[key] = expansion
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
        if footer[0] == "END" and (conditional_stack or set(self.conditional_branches) != set(self.conditional_decisions)):
            raise ValueError("Conditional graph is incomplete")
        self.validate_conditionals()
        if footer[0] == "END" and (statement_stack or construct_stack):
            raise ValueError("Source construct graph is incomplete")
        self.validate_macros(complete=footer[0] == "END")
        if set(self.source_contexts) != set(self.source_endings):
            raise ValueError("Source contexts are not completely closed")
        if any(state == "changed-or-unreadable" for state in self.source_endings.values()):
            raise ValueError("Sidecar claims completion after its source changed")
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
            available = (self.symbols if domain == "symbol" else self.nodes if domain == "node" else
                         declaration_ids if domain == "declaration" else range(1, counts["B"] + 1)
                         if domain == "binding" else expression_ids)
            if identity not in available:
                raise ValueError(f"Dangling {domain} reference: {identity}")
        for subject, statement in self.statement_owners.items():
            if subject_modules[subject] != int(self.statements[statement][7]):
                raise ValueError("Statement fact belongs to another module")
        for identity, row in self.expression_operands.items():
            module = subject_modules["expression", identity]
            if module != int(self.source_contexts[int(row[6])][4]):
                raise ValueError("Source operation result belongs to another module")
            for child in (int(row[4]), int(row[5])):
                if child and subject_modules["expression", child] != module:
                    raise ValueError("Source operand belongs to another module")
        if expression_ids != set(range(1, counts["E"] + 1)):
            raise ValueError("Expression identity sequence has gaps")
        if set(self.symbols) != set(range(1, counts["S"] + 1)):
            raise ValueError("Symbol identity sequence has gaps")
        if set(self.nodes) != set(range(1, counts["N"] + 1)):
            raise ValueError("Node identity sequence has gaps")
        for identity in self.nodes:
            if "kind" not in self.properties["node", identity]:
                raise ValueError("AST node lacks its stable kind")
        self.validate_nodes()
        self.validate_flow()
        if not expressions_only and not bindings_only:
            self.validate_metadata()

    def validate_flow(self) -> None:
        """Validate phase ownership and transfers without inventing targets."""
        phases: dict[int, int] = {}
        memberships: dict[int, int] = {}
        blocks: dict[int, tuple[int, int]] = {}
        block_nodes: dict[int, int] = {}
        labels: set[tuple[int, int]] = set()
        for row in self.records["PH"]:
            identity, procedure = number(row[1], 1), number(row[2], 1)
            if identity in phases or procedure not in self.signatures or row[3] != "pre-load" or row[4] not in ("0", "1"):
                raise ValueError("Invalid or repeated AST phase")
            phases[identity] = procedure
        for row in self.records["NP"]:
            node, phase = number(row[1], 1), number(row[2], 1)
            if node in memberships or node not in self.nodes or phase not in phases:
                raise ValueError("Invalid or repeated AST phase membership")
            memberships[node] = phase
        initializer_roots = {int(row[4]) for row in self.records["H"]
                             if row[1] == "symbol" and row[3] == "node"
                             and row[5] in ("initializer", "default-initializer")}
        # Parents have smaller identities. Cache roots once so a deeply nested
        # initializer does not make membership validation quadratic.
        node_roots: dict[int, int] = {}
        for identity in sorted(self.nodes):
            row = self.nodes[identity]
            node_roots[identity] = identity if row[3] == "root" else node_roots[int(row[2])]
            if identity not in memberships and node_roots[identity] not in initializer_roots:
                raise ValueError("AST phase membership is incomplete")
        for node, phase in memberships.items():
            parent = int(self.nodes[node][2])
            if self.nodes[node][3] == "root":
                if parent != phases[phase]:
                    raise ValueError("AST phase belongs to another procedure")
            elif memberships.get(parent) != phase:
                raise ValueError("AST child belongs to another phase")
        evaluation_edges: set[tuple[int, int]] = set()
        for row in self.records["EV"]:
            parent, child = number(row[1], 1), number(row[2], 1)
            number(row[3], 0)
            edge = parent, child
            if parent == child or parent not in memberships or child not in memberships or memberships[parent] != memberships[child]:
                raise ValueError("Evaluation edge lacks its AST phase")
            if edge in evaluation_edges or row[4] not in (
                    "always", "argument", "profile-begin", "call-target", "profile-end",
                    "copyback", "condition", "true", "false", "result"):
                raise ValueError("Invalid or repeated evaluation edge")
            evaluation_edges.add(edge)
        for row in self.records["CB"]:
            block, phase, ordinal = [number(value, 1 if index < 2 else 0) for index, value in enumerate(row[1:])]
            if block in blocks or phase not in phases:
                raise ValueError("Invalid or repeated control-flow block")
            blocks[block] = phase, ordinal
        for phase in phases:
            ordinals = sorted(ordinal for owner, ordinal in blocks.values() if owner == phase)
            if ordinals != list(range(len(ordinals))):
                raise ValueError("Control-flow block order is incomplete")
        for row in self.records["CN"]:
            block, node = number(row[1], 1), number(row[2], 1)
            if block in block_nodes or block not in blocks or node not in memberships or row[3] != "0":
                raise ValueError("Invalid or repeated control-flow node")
            if self.nodes[node][3] != "root" or memberships[node] != blocks[block][0]:
                raise ValueError("Control-flow node lacks its phase root")
            block_nodes[block] = node
        roots = {identity for identity in memberships if self.nodes[identity][3] == "root"}
        if set(block_nodes) != set(blocks) or len(set(block_nodes.values())) != len(block_nodes) or set(block_nodes.values()) != roots:
            raise ValueError("Control-flow root membership is incomplete")
        for row in self.records["CL"]:
            phase, label, block = [number(value, 1) for value in row[1:]]
            if block not in blocks or blocks[block][0] != phase or label not in self.symbols or (phase, label) in labels:
                raise ValueError("Invalid or repeated control-flow label")
            if self.properties["node", block_nodes[block]]["kind"] != "label" or int(self.nodes[block_nodes[block]][9]) != label:
                raise ValueError("Control-flow label lacks its AST definition")
            labels.add((phase, label))
        for row in self.records["CE"]:
            phase, source, target = number(row[1], 1), number(row[2], 1), number(row[3], 0)
            kind, label = row[4], number(row[5], 0)
            if source not in blocks or blocks[source][0] != phase or label and label not in self.symbols:
                raise ValueError("Control-flow edge lacks its phase or label")
            if kind == "fallthrough":
                if target not in blocks or blocks[target][0] != phase or blocks[target][1] != blocks[source][1] + 1 or label:
                    raise ValueError("Invalid control-flow fallthrough")
            elif kind in ("label", "conditional-label", "case-label", "default-label", "subroutine-call"):
                if target or not label and kind != "subroutine-call":
                    raise ValueError("Invalid control-flow label transfer")
            elif kind in ("unknown-indirect", "subroutine-return", "unknown-assembly", "procedure-exit"):
                if target or label:
                    raise ValueError("Unknown or exit transfer advertises a target")
            else:
                raise ValueError("Unknown control-flow transfer kind")

    @staticmethod
    def macro_units(kind: str, value: str) -> bytes | tuple[int, ...]:
        if kind == "bytes":
            return value.encode("utf-8", errors="surrogateescape")
        if kind != "wide-units" or len(value) % 8 or re.fullmatch(r"[0-9A-F]*", value) is None:
            raise ValueError("Invalid macro text unit representation")
        return tuple(int(value[index:index + 8], 16) for index in range(0, len(value), 8))

    def validate_macros(self, *, complete: bool) -> None:
        if complete and set(self.macro_invocations) != set(self.macro_results):
            raise ValueError("Macro expansion graph is incomplete")
        for identity, definition in self.macro_definitions.items():
            parameters = [row for row in self.macro_tokens[identity] if row[3] == "parameter"]
            body = [row for row in self.macro_tokens[identity] if row[3] != "parameter"]
            count = int(definition[7])
            if [int(row[2]) for row in parameters] != list(range(count)):
                raise ValueError("Macro formal parameters are incomplete or out of order")
            if [int(row[2]) for row in body] != list(range(len(body))):
                raise ValueError("Macro replacement tokens are out of order")
            if definition[6].endswith("callback") and body:
                raise ValueError("Callback definition advertises a replacement token body")
            if definition[6] == "text" and (len(body) != 1 or body[0][3] not in ("text", "wide-text")):
                raise ValueError("Text definition lacks its replacement")
            for token in body:
                if token[3] in ("parameter-reference", "stringify-reference") and int(token[4]) >= count:
                    raise ValueError("Macro token refers to an absent formal parameter")
        for identity, result in self.macro_results.items():
            invocation = self.macro_invocations[identity]
            definition = self.macro_definitions[int(invocation[3])]
            argument_count = int(definition[7])
            args = self.macro_arguments[identity]
            pieces = self.macro_segments[identity]
            output = self.macro_units(result[3], result[5])
            if result[2] in ("not-invoked", "unsupported", "recursive") and (output or args or pieces or identity in self.macro_callbacks):
                raise ValueError("Unexpanded macro advertises substitutions or output")
            if result[2] in ("expanded", "failed", "recovered") and set(args) != set(range(argument_count)):
                raise ValueError("Macro argument mapping does not match its formals")
            if [int(row[2]) for row in pieces] != list(range(len(pieces))):
                raise ValueError("Macro substitution sequence has gaps or duplicates")
            offset = 0
            body = [row for row in self.macro_tokens[int(invocation[3])] if row[3] != "parameter"]
            for piece in pieces:
                token, parameter, kind, start, length = int(piece[3]), int(piece[4]), piece[5], int(piece[6]), int(piece[7])
                if start != offset or start + length > len(output):
                    raise ValueError("Macro substitution offsets do not partition its output")
                offset += length
                expected: bytes | tuple[int, ...] | None = None
                if kind in ("parameter", "stringify"):
                    if parameter not in args or token < 0 or token >= len(body):
                        raise ValueError("Macro substitution lacks its formal or definition token")
                    if body[token][3] != ("parameter-reference" if kind == "parameter" else "stringify-reference") or int(body[token][4]) != parameter:
                        raise ValueError("Macro substitution contradicts its definition token")
                    expected = self.macro_units(args[parameter][3], args[parameter][4])
                    if kind == "stringify":
                        if isinstance(expected, bytes):
                            expected = b'$"' + expected.replace(b'"', b'""') + b'"' if expected else b'""'
                        else:
                            escaped = tuple(item for unit in expected for item in ((unit, unit) if unit == 34 else (unit,)))
                            expected = (36, 34) + escaped + (34,) if expected else (34, 34)
                elif parameter != -1:
                    raise ValueError("Non-parameter substitution advertises a formal")
                if kind == "restored-delimiter" and (token != -1 or length != 1):
                    raise ValueError("Invalid restored caller delimiter")
                if kind == "definition-text" and (token != -1 or definition[6] != "text"):
                    raise ValueError("Text replacement advertises an invalid definition token")
                if kind == "callback" and (token != -1 or identity not in self.macro_callbacks):
                    raise ValueError("Macro substitution fabricates callback output")
                if kind == "text" and (token < 0 or token >= len(body) or body[token][3] not in ("text", "wide-text")):
                    raise ValueError("Macro text substitution lacks its definition token")
                if expected is not None and output[start:start + length] != expected:
                    raise ValueError("Macro substitution output contradicts its actual argument")
            if result[2] in ("expanded", "failed", "recovered") and offset != len(output):
                raise ValueError("Macro output is not completely covered by substitutions")
            callback = self.macro_callbacks.get(identity)
            if callback is not None and not definition[6].endswith("callback"):
                raise ValueError("Ordinary macro advertises a callback invocation")
            if complete and result[2] in ("failed", "recursive", "unsupported", "recovered"):
                raise ValueError("Macro failure was claimed complete")

    def validate_conditionals(self) -> None:
        """Check selection separately from evaluation; neither implies the other."""
        for members in self.conditional_groups.values():
            selected = False
            for identity in members:
                if identity not in self.conditional_decisions:
                    continue  # A recovery artifact may end during a condition.
                decision = self.conditional_decisions[identity]
                if decision[2] == "parent-inactive":
                    parent = int(self.conditional_branches[identity][2])
                    if parent == 0 or parent not in self.conditional_decisions or self.conditional_decisions[parent][4] != "0":
                        raise ValueError("Inactive condition has no unselected parent")
                elif decision[2] in ("evaluated", "unconditional"):
                    expected = not selected and (decision[2] == "unconditional" or decision[3] == "1")
                    if (decision[4] == "1") != expected:
                        raise ValueError("Conditional selection contradicts its result or prior selection")
                selected |= decision[4] == "1"
        for row in self.records["PPS"]:
            branch = int(row[2])
            if branch not in self.conditional_decisions or self.conditional_decisions[branch][4] != "0":
                raise ValueError("Selected branch advertises an inactive region")

    def validate_nodes(self) -> None:
        """Check payload meaning independently of the exporter's record totals."""
        auxiliary: dict[tuple[int, str], list[int]] = defaultdict(list)
        assembly: dict[int, list[list[str]]] = defaultdict(list)
        for row in self.records["ASM"]:
            assembly[int(row[1])].append(row)
        for identity, node in self.nodes.items():
            properties = self.properties["node", identity]
            raw_class = number(node[4], 0)
            if raw_class >= len(NODE_KINDS) or properties["kind"] != NODE_KINDS[raw_class]:
                raise ValueError("AST class disagrees with its stable kind")
            kind = properties["kind"]
            if not NODE_REQUIRED.get(kind, set()) <= properties.keys():
                raise ValueError("AST node lacks required payload: " + kind)
            for key, value in properties.items():
                if key in NODE_FLAGS and value not in ("0", "1"):
                    raise ValueError("Invalid node payload flag: " + key)
                if key in NODE_UNSIGNED:
                    number(value, 0)
                elif key in NODE_SIGNED:
                    number(value)
            if "operator-options" in properties and number(properties["operator-options"], 0) > 255:
                raise ValueError("Unknown operator option bits")
            if kind == "constant" and ("node", identity) not in self.constants:
                raise ValueError("Constant node lacks its exact value")
            if kind == "call" and properties["call-kind"] not in ("direct", "runtime", "indirect", "virtual"):
                raise ValueError("Unknown call dispatch kind")
            if kind == "argument" and properties["passing-mode"] not in ("default", "byval", "byref", "bydesc"):
                raise ValueError("Unknown argument passing mode")
            if kind == "assembly":
                tokens = assembly.pop(identity, [])
                if [number(row[2], 0) for row in tokens] != list(range(number(properties["assembly-token-count"], 0))):
                    raise ValueError("Assembly token order/count is incomplete")
                if properties["assembly-effects"] != "unknown-memory-registers-control":
                    raise ValueError("Assembly effects must retain uncertainty")
            if node[3] in ("copyback", "profile-begin", "profile-end"):
                parent = number(node[2], 1)
                if self.properties["node", parent]["kind"] != "call" or "auxiliary-ordinal" not in properties:
                    raise ValueError("Auxiliary action lacks call ownership or order")
                auxiliary[parent, node[3]].append(number(properties["auxiliary-ordinal"], 0))
                if node[3] == "copyback":
                    links = [row for row in self.relations("copyback-temporary") if row[1] == "node" and int(row[2]) == identity]
                    if len(links) != 1 or int(links[0][6]) != int(properties["auxiliary-ordinal"]):
                        raise ValueError("Copyback action lacks its temporary/order relationship")
        if assembly:
            raise ValueError("Assembly tokens belong to a non-assembly node")
        for identity in self.nodes:
            properties = self.properties["node", identity]
            if properties["kind"] == "call":
                if sorted(auxiliary.get((identity, "copyback"), [])) != list(range(number(properties["copyback-count"], 0))):
                    raise ValueError("Call copyback actions are incomplete")
                for role in ("profile-begin", "profile-end"):
                    if auxiliary.get((identity, role), []) not in ([], [0]):
                        raise ValueError("Profiling action is duplicated or incorrectly ordered")

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

    def validate_source_revisions(self) -> None:
        """Require current files to match captured content before applying edits.

        A historical model remains readable after its input changes. This
        explicit freshness check rejects using it as current source evidence.
        Streams provide no seekable revision and cannot pass the editing gate.
        """
        for row in self.files.values():
            if row[6] != "regular":
                raise ValueError("Stream source has no immutable file revision")
            data = Path(row[2]).read_bytes()
            if len(data) != int(row[3]) or hashlib.sha256(data).hexdigest() != row[4]:
                raise ValueError("Source revision is stale: " + row[2])

    def validate_physical_locations(self) -> None:
        """Verify bytes and UTF-16 coordinates independently before projection."""
        self.validate_source_revisions()
        contents = {identity: Path(row[2]).read_bytes() for identity, row in self.files.items() if row[6] == "regular"}
        codecs_by_encoding = {"unmarked-bytes": ("utf-8", 0), "utf-8-bom": ("utf-8", 3),
                              "utf-16le": ("utf-16-le", 2), "utf-16be": ("utf-16-be", 2),
                              "utf-32le": ("utf-32-le", 4), "utf-32be": ("utf-32-be", 4)}
        for row in self.records["LOC"]:
            if row[11] != "mapped":
                continue
            fileid = int(self.source_contexts[int(row[4])][3])
            data = contents[fileid]
            encoding = self.files[fileid][5]
            if encoding not in codecs_by_encoding:
                raise ValueError("Physical coordinate decoder is unavailable")
            codec, bom = codecs_by_encoding[encoding]
            for line_column, byte_column in ((5, 9), (7, 10)):
                try:
                    prefix = data[bom:int(row[byte_column])].decode(codec)
                except UnicodeDecodeError as error:
                    raise ValueError("Physical byte offset splits an encoded character") from error
                lines = re.split(r"\r\n|\r|\n", prefix)
                actual_line = len(lines)
                actual_column = len(lines[-1].encode("utf-16-le")) // 2
                if (actual_line, actual_column) != (int(row[line_column]), int(row[line_column + 1])):
                    raise ValueError("Physical bytes disagree with their UTF-16 coordinates")

    def named(self, name: str, kind: str | None = None) -> list[int]:
        return [identity for identity, row in self.types.items()
                if row[2].casefold() == name.casefold() and (kind is None or row[3] == kind)]

    def symbol_name(self, identity: str | int) -> str:
        value = int(identity)
        return self.types[value][2] if value in self.types else self.symbols[value][2]

    def relations(self, kind: str) -> list[list[str]]:
        return [row for row in self.records["H"] if row[5] == kind]


# end of sidecar.py
