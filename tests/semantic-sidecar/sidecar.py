"""Project: FreeBASIC semantic sidecar tests
File: sidecar.py
Purpose: Read and independently validate schema 27 sidecars.
Responsibilities: Check shapes, references, ranges, escaping, and completion totals.
This file intentionally does NOT contain: compiler invocation or inferred semantics.
"""

from __future__ import annotations

import codecs
from collections import Counter, defaultdict
from collections.abc import Iterable
from dataclasses import dataclass, fields
import hashlib
from pathlib import Path
import re
import stat
from semantic_flow import validate_flow, validate_sequence_branches
from semantic_array_storage import validate_array_storage_inputs
from semantic_literals import validate_wide_literals
from semantic_queries import validate_query_inputs
from semantic_selects import validate_select_inputs
from semantic_declarations import validate_declaration_types
from semantic_procedures import validate_procedure_types
from semantic_aggregate_access import validate_aggregate_access
from semantic_string_declarations import validate_string_declarations
from semantic_repetitions import validate_declaration_repetitions
from aggregate_fields import validate_aggregate_fields
from semantic_callbacks import validate_procedure_callbacks
from semantic_abi_policy import validate_abi_policy_inputs
from semantic_header_policy import validate_header_policy
from semantic_enums import validate_enum_declarations
from semantic_iif import validate_iif_inputs
from semantic_if import validate_if_conditions
from semantic_if_arms import PREFIXES as IF_ARM_PREFIXES, validate_if_arms
from semantic_assignment_inputs import PREFIXES as ASSIGNMENT_INPUT_PREFIXES, validate_assignment_inputs
from semantic_assignment_storage import PREFIXES as ASSIGNMENT_STORAGE_PREFIXES, validate_assignment_storage


SCHEMA = "27"


@dataclass(frozen=True)
class ReaderLimits:
    """Resource limits for untrusted semantic-model and source input."""

    max_sidecar_bytes: int = 512 * 1024 * 1024
    max_record_bytes: int = 64 * 1024 * 1024
    max_records: int = 24_000_000
    max_source_bytes: int = 512 * 1024 * 1024
    max_cached_source_bytes: int = 512 * 1024 * 1024
    max_edits: int = 100_000
    max_planned_output_bytes: int = 512 * 1024 * 1024

    def __post_init__(self) -> None:
        for item in fields(self):
            value = getattr(self, item.name)
            if isinstance(value, bool) or not isinstance(value, int) or value <= 0:
                raise ValueError("Reader limit must be a positive integer: " + item.name)


@dataclass(frozen=True)
class SourceEdit:
    """One exact replacement requested against a mapped LOC attachment."""

    domain: str
    identity: int
    role: str
    replacement: bytes


@dataclass(frozen=True)
class PlannedSource:
    """Fresh source bytes and their non-writing replacement result."""

    path: Path
    expected_sha256: str
    updated_sha256: str
    original: bytes
    updated: bytes


SOURCE_CODECS = {
    "unmarked-bytes": ("utf-8", 0),
    "utf-8-bom": ("utf-8", 3),
    "utf-16le": ("utf-16-le", 2),
    "utf-16be": ("utf-16-be", 2),
    "utf-32le": ("utf-32-le", 4),
    "utf-32be": ("utf-32-be", 4),
}
SOURCE_BOMS = {
    "utf-8-bom": codecs.BOM_UTF8,
    "utf-16le": codecs.BOM_UTF16_LE,
    "utf-16be": codecs.BOM_UTF16_BE,
    "utf-32le": codecs.BOM_UTF32_LE,
    "utf-32be": codecs.BOM_UTF32_BE,
}
# The producer's physical-line index uses the same byte limit.
MAX_COORDINATE_LINE_BYTES = 16 * 1024 * 1024


SHAPES = {
    "FBCSEM": 3, "M": 2, "D": 2, "S": 12, "B": 9, "I": 12,
    "P": 9, "V": 5, "N": 13, "E": 16, "R": 3, "END": 13,
    "RECOVERY": 8, "T": 19, "A": 7, "F": 13, "G": 9, "U": 12,
    "C": 5, "H": 7, "K": 5, "J": 5, "O": 10, "Q": 10, "Y": 7, "Z": 5, "ASM": 6, "DCL": 13,
    "CTX": 3, "OPT": 5, "USE": 4,
    "NT": 11, "PH": 5,
    "FILE": 7, "SRC": 14, "SRE": 3, "INC": 12, "MAP": 11, "ORIG": 4, "LOC": 12,
    "PPB": 14, "PPD": 11, "PPE": 9, "PPT": 13, "PPS": 10,
    "MD": 10, "MT": 6, "MI": 15, "MA": 12, "MS": 8, "MC": 5, "ME": 13, "ML": 13, "MR": 5,
    "ST": 17, "STE": 10, "BLK": 14, "BEND": 9, "OWN": 4, "CAP": 4, "ACC": 3, "SOP": 3,
    "EX": 13,
    "NP": 3, "EV": 5, "CB": 4, "CN": 4, "CE": 6, "CL": 4, "DI": 12,
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


def complete_ordinals(values: list[int], count: int, *, ordered: bool = False) -> bool:
    """Check actual records without allocating from an untrusted wire count."""
    if len(values) != count:
        return False
    if not ordered:
        values = sorted(values)
    return all(value == ordinal for ordinal, value in enumerate(values))


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
                 allow_recovery: bool = False,
                 limits: ReaderLimits | None = None) -> None:
        if expressions_only and bindings_only:
            raise ValueError("Sidecar cannot be both expressions-only and bindings-only")
        self.limits = limits or ReaderLimits()
        if not text.isascii():
            raise ValueError("Semantic sidecar wire data is not ASCII")
        if len(text) > self.limits.max_sidecar_bytes:
            raise ValueError("Semantic sidecar exceeds the configured byte limit")
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
        include_ids: set[int] = set()
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
        expression_property_modules: dict[tuple[int, str], int] = {}
        storage_property_modules: dict[tuple[str, int, str], int] = {}
        array_record_modules: dict[int, int] = {}
        construct_observation_modules: list[tuple[list[str], int]] = []
        size_query_origins: list[tuple[int, int, int]] = []
        parsed_expression_origins: list[tuple[int, int, int]] = []
        footer: list[str] | None = None
        module_expressions = 0

        def reference(domain: str, value: str, *, nullable: bool = True) -> None:
            identity = number(value, 0 if nullable else 1)
            if identity:
                references.append((domain, identity))

        def flag(value: str) -> None:
            if value not in ("0", "1"):
                raise ValueError("Invalid boolean: " + value)

        cursor = 0
        line_number = 0
        while cursor < len(text):
            newline = text.find("\n", cursor)
            if newline < 0:
                raise ValueError("Sidecar is truncated")
            line_number += 1
            if line_number > self.limits.max_records:
                raise ValueError("Semantic sidecar exceeds the configured record limit")
            if newline - cursor + 1 > self.limits.max_record_bytes:
                raise ValueError("Semantic sidecar record exceeds the configured byte limit")
            line = text[cursor:newline]
            cursor = newline + 1
            if line.endswith("\r"):
                line = line[:-1]
            if "\r" in line:
                raise ValueError(f"Line {line_number}: stray carriage return")
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
                subject_modules["symbol", identity] = counts["M"]
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
                array_record_modules[number(row[1], 1)] = counts["M"]
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
                if identity not in self.statements or row[2] not in ("file-open", "file-close", "file-seek", "file-get", "file-put", "file-lock", "file-unlock", "file-rename", "line-input", "namespace-import", "goto", "gosub", "gosub-return", "gosub-return-label", "procedure-return", "on-goto", "on-gosub", "on-error-set", "on-error-clear"):
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
                domains = ("symbol", "node", "expression")
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
                    if row[3] == "written-override":
                        if row[1] != "symbol" or row[4] not in ("0", "1"):
                            raise ValueError("Invalid written override receipt")
                        previous = self.properties[key].get(row[3])
                        if previous is not None and previous != row[4]:
                            raise ValueError("Conflicting written override receipts")
                    if row[3] == "formal-span-kind":
                        if row[1] != "symbol" or row[4] not in ("physical", "generated"):
                            raise ValueError("Invalid formal span kind")
                        previous = self.properties[key].get(row[3])
                        if previous is not None and previous != row[4]:
                            raise ValueError("Conflicting formal span kinds")
                    if row[3] == "declared-field-count":
                        if row[1] != "symbol" or number(row[4], 0) > 1000000:
                            raise ValueError("Invalid declared field count")
                        previous = self.properties[key].get(row[3])
                        if previous is not None and previous != row[4]:
                            raise ValueError("Conflicting declared field counts")
                    if row[3] == "field-array-rank":
                        if row[1] != "symbol" or not -1 <= number(row[4]) <= 8:
                            raise ValueError("Invalid field array rank")
                    if row[3].startswith("array-storage-") and row[3] in self.properties[key]:
                        raise ValueError("Repeated array storage property")
                    if row[3] in ("constant-symbol", "constant-atom-kind", "bound-value-symbol", "assignment-target-dtype", "assignment-kind", "source-assignment-symbol",
                                   "source-assignment-kind", "unevaluated-query-input") or row[3].startswith(("size-query-", "numeric-selected-", "numeric-literal-", "file-transfer-", "string-intrinsic-", "string-initializer-", "array-subscript-", "array-bound-", "array-initializer-", "array-storage-")):
                        if row[1] != "expression":
                            raise ValueError("Invalid expression receipt domain")
                        previous = self.properties[key].get(row[3])
                        if previous is not None and previous != row[4]:
                            raise ValueError("Conflicting expression receipts")
                        receipt_key = int(row[2]), row[3]
                        old_module = expression_property_modules.get(receipt_key)
                        if old_module is not None and old_module != counts["M"]:
                            raise ValueError("Expression receipt changes module")
                        expression_property_modules[receipt_key] = counts["M"]
                    if row[3] in ("call-argument-expression", "formal-default-expression") or row[3].startswith(("pointer-address-", "pointer-dereference-", "pointer-index-", "memory-new-", "memory-release-", "procedure-prototype-statement-", "for-step-", "for-start-expression:", "for-limit-expression:", "loop-condition-")):
                        expected_domain = "node" if row[3] == "call-argument-expression" else "symbol" if row[3] == "formal-default-expression" or row[3].startswith(("memory-new-", "procedure-prototype-statement-", "for-step-", "for-start-expression:", "for-limit-expression:", "loop-condition-")) else "expression"
                        if row[1] != expected_domain:
                            raise ValueError("Invalid storage receipt domain")
                        previous = self.properties[key].get(row[3])
                        if previous is not None and previous != row[4]:
                            raise ValueError("Conflicting storage receipts")
                        receipt_key = row[1], int(row[2]), row[3]
                        old_module = storage_property_modules.get(receipt_key)
                        if old_module is not None and old_module != counts["M"]:
                            raise ValueError("Storage receipt changes module")
                        storage_property_modules[receipt_key] = counts["M"]
                    if row[3].startswith("literal-target-wide-"):
                        if row[1] != "symbol" or row[3] in self.properties[key]:
                            raise ValueError("Duplicate or foreign wide literal prefix property")
                        storage_property_modules[row[1], int(row[2]), row[3]] = counts["M"]
                    if row[3].startswith("unevaluated-query-range-"):
                        if row[1] != "symbol" or row[3] in self.properties[key]:
                            raise ValueError("Duplicate or foreign unevaluated query range")
                        storage_property_modules[row[1], int(row[2]), row[3]] = counts["M"]
                    if row[3].startswith("select-case-"):
                        if row[1] != "symbol" or row[3] in self.properties[key]:
                            raise ValueError("Duplicate or foreign SELECT input property")
                        storage_property_modules[row[1], int(row[2]), row[3]] = counts["M"]
                    if row[3] in ("enum-declaration-input", "enum-element-input"):
                        if row[1] != "symbol" or row[3] in self.properties[key]:
                            raise ValueError("Duplicate or foreign enum input property")
                        storage_property_modules[row[1], int(row[2]), row[3]] = counts["M"]
                    if row[3].startswith("if-condition-"):
                        if row[1] != "symbol" or row[3] in self.properties[key]:
                            raise ValueError("Duplicate or foreign IF condition property")
                        storage_property_modules[row[1], int(row[2]), row[3]] = counts["M"]
                    if row[3].startswith(IF_ARM_PREFIXES):
                        if row[1] != "symbol" or row[3] in self.properties[key]:
                            raise ValueError("Duplicate or foreign IF arm property")
                        storage_property_modules[row[1], int(row[2]), row[3]] = counts["M"]
                    if row[3].startswith(ASSIGNMENT_INPUT_PREFIXES + ASSIGNMENT_STORAGE_PREFIXES):
                        if row[1] != "symbol" or row[3] in self.properties[key]:
                            raise ValueError("Duplicate or foreign assignment input property")
                        storage_property_modules[row[1], int(row[2]), row[3]] = counts["M"]
                    if row[3] == "original-iif-inputs":
                        if row[1] != "expression" or row[3] in self.properties[key]:
                            raise ValueError("Duplicate or foreign original IIf input property")
                        storage_property_modules[row[1], int(row[2]), row[3]] = counts["M"]
                    if row[1] == "statement":
                        storage_property_modules[row[1], int(row[2]), row[3]] = counts["M"]
                    self.properties[key][row[3]] = row[4]
                else:
                    if row[3] not in ("symbol", "node", "expression", "binding") or not row[5]:
                        raise ValueError("Invalid relationship target or kind")
                    reference(row[3], row[4], nullable=False)
                    number(row[6], 0)
                    if row[5] == "assignment-count" or row[5].startswith(("assignment-input:",) + ASSIGNMENT_STORAGE_PREFIXES):
                        storage_property_modules[row[1], int(row[2]), "marker:" + row[5] + ":" + row[6]] = counts["M"]
                    if row[5] in ("loop-condition", "if-condition", "array-subscript", "array-bound-query",
                                  "pointer-dereference-input", "pointer-index-input", "array-storage-root",
                                  "array-storage-selection", "array-storage-input"):
                        construct_observation_modules.append((row, counts["M"]))
                    if row[5] in ("if-construct", "if-end", "if-arm-statement") or row[5].startswith(("if-arm:", "if-arm-end:", "if-arm-transfer:")):
                        construct_observation_modules.append((row, counts["M"]))
                    if row[5] == "size-query-source":
                        if row[1] != "expression" or row[3] != "expression" or row[6] != "0":
                            raise ValueError("Invalid size query source domain")
                        size_query_origins.append((int(row[2]), int(row[4]), counts["M"]))
                    if row[5] == "parsed-expression-source":
                        if row[1] != "expression" or row[3] != "expression" or row[6] != "0":
                            raise ValueError("Invalid parsed expression source domain")
                        parsed_expression_origins.append((int(row[2]), int(row[4]), counts["M"]))
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
            elif tag == "PH":
                number(row[1], 1)
                reference("symbol", row[2], nullable=False)
                if not row[3]:
                    raise ValueError("Flow phase has no name")
                flag(row[4])
            elif tag in ("NP", "EV", "CB", "CN", "CE", "CL"):
                # Validate their closed phase/block graph after all identities
                # have arrived. No control-flow target is inferred here.
                pass
            elif tag == "DI":
                identity = number(row[1], 1)
                if identity in self.diagnostics or row[2] not in ("error", "warning"):
                    raise ValueError("Invalid or repeated diagnostic")
                number(row[3], 0)
                number(row[8])
                sourceid, statement = number(row[9], 0), number(row[10], 0)
                if sourceid and (sourceid not in self.source_contexts or sourceid in self.source_endings
                                 or int(self.source_contexts[sourceid][4]) != counts["M"]):
                    raise ValueError("Diagnostic source is absent, closed or from another module")
                if statement and (statement not in self.statements or int(self.statements[statement][7]) != counts["M"]):
                    raise ValueError("Diagnostic statement is absent or from another module")
                reference("symbol", row[11])
                self.diagnostics[identity] = row
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
                identity = number(row[1], 1)
                if identity in include_ids:
                    raise ValueError("Duplicate include occurrence identity")
                include_ids.add(identity)
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
                    if identity not in include_ids:
                        raise ValueError("Physical include occurrence location is dangling")
                elif row[1] == "remap":
                    if identity not in remap_ids:
                        raise ValueError("Physical remap location is dangling")
                elif row[1] == "statement":
                    if row[3].startswith(("assignment-operator:", "assignment-destination:")):
                        if identity not in self.statements or not statement_stack or statement_stack[-1] != identity:
                            raise ValueError("Assignment operator has no active statement")
                        if int(self.source_contexts[sourceid][4]) != counts["M"]:
                            raise ValueError("Assignment operator belongs to another module")
                    else:
                        if identity not in self.statement_endings:
                            raise ValueError("Physical statement range has no observed end")
                        if sourceid != int(self.statements[identity][5]):
                            raise ValueError("Physical statement range belongs to another source occurrence")
                elif row[1] == "construct":
                    if row[3].startswith("if-arm:"):
                        if identity not in self.constructs or not construct_stack or construct_stack[-1] != identity:
                            raise ValueError("IF arm opening token has no active construct")
                        if int(self.source_contexts[sourceid][4]) != counts["M"]:
                            raise ValueError("IF arm opening token belongs to another module")
                    else:
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
                subject_modules["statement", identity] = int(row[7])
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
                         if domain == "binding" else self.statements if domain == "statement" else expression_ids)
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
        self.validate_source_ownership(subject_modules)
        for identity in self.nodes:
            if "kind" not in self.properties["node", identity]:
                raise ValueError("AST node lacks its stable kind")
        self.validate_nodes()
        if not expressions_only and not bindings_only:
            storage_roots = validate_array_storage_inputs(self, number, subject_modules)
            validate_flow(self, number, storage_roots)
            self.validate_procedure_exits(subject_modules)
            validate_sequence_branches(self, number, subject_modules)
            self.validate_metadata()
            self.validate_formal_spans()
            for (identity, _), module in expression_property_modules.items():
                if subject_modules.get(("expression", identity)) != module:
                    raise ValueError("Expression receipt belongs to another module")
            self.validate_numeric_assignments(subject_modules)
            self.validate_source_assignments(subject_modules)
            self.validate_formal_defaults(subject_modules)
            self.validate_call_atoms(subject_modules)
            self.validate_numeric_selections(subject_modules)
            self.validate_numeric_literals(subject_modules)
            self.validate_size_queries(subject_modules)
            for (domain, identity, _), module in storage_property_modules.items():
                if subject_modules.get((domain, identity)) != module:
                    raise ValueError("Storage receipt belongs to another module")
            self.validate_storage_receipts(subject_modules)
            validate_query_inputs(self, number, subject_modules)
            validate_select_inputs(self, number, subject_modules)
            validate_declaration_types(self, number, subject_modules)
            validate_procedure_types(self, number, subject_modules)
            validate_aggregate_access(self, number, subject_modules)
            validate_string_declarations(self, number, subject_modules)
            validate_declaration_repetitions(self, number, subject_modules)
            validate_enum_declarations(self, number, subject_modules)
            validate_iif_inputs(self, number, subject_modules)
            validate_if_conditions(self, number, subject_modules)
            validate_if_arms(self, number, subject_modules)
            validate_assignment_inputs(self, number, subject_modules)
            validate_assignment_storage(self, number, subject_modules, len(NODE_KINDS))
            validate_procedure_callbacks(self, number, subject_modules)
            validate_abi_policy_inputs(self, number, subject_modules)
            validate_header_policy(self, number, unescape, subject_modules)
            validate_aggregate_fields(self, number, subject_modules)
            validate_wide_literals(self, number, subject_modules)
            self.validate_for_steps(subject_modules)
            self.validate_for_inputs(subject_modules)
            self.validate_file_transfers(subject_modules)
            self.validate_string_intrinsics(subject_modules)
            self.validate_string_initializers(subject_modules)
            self.validate_loop_conditions(subject_modules)
            self.validate_array_subscripts(subject_modules, array_record_modules)
            self.validate_array_initializer_inputs(subject_modules, array_record_modules)
            self.validate_array_bounds(subject_modules, array_record_modules)
            for row, module in construct_observation_modules:
                if subject_modules.get((row[1], int(row[2]))) != module or subject_modules.get((row[3], int(row[4]))) != module:
                    raise ValueError("Construct observation marker belongs to another module")
            for subject, source, module in parsed_expression_origins:
                if source >= subject or subject_modules.get(("expression", subject)) != module or subject_modules.get(("expression", source)) != module:
                    raise ValueError("Invalid parsed expression predecessor")
            for subject, query, module in size_query_origins:
                if query >= subject or subject_modules.get(("expression", subject)) != module or subject_modules.get(("expression", query)) != module:
                    raise ValueError("Missing, cyclic or foreign size query source")
                if "size-query-kind" not in self.properties["expression", query]:
                    raise ValueError("Size query source lacks its original input receipt")

    def validate_source_ownership(self, subject_modules: dict[tuple[str, int], int]) -> None:
        """A byte location must describe the same occurrence as its source fact.

        Repeated includes can have identical bytes and coordinates. Revision
        checks alone therefore cannot detect a location attached to the wrong
        include or module. Logical remaps affect names and lines, not ORIG.
        """
        declarations = {int(row[1]): row for row in self.records["DCL"]}
        for subject, sourceid in self.origins.items():
            if subject_modules[subject] != int(self.source_contexts[sourceid][4]):
                raise ValueError("Source origin belongs to another module")
            statement = self.statement_owners.get(subject)
            if subject[0] != "node" and statement is not None:
                if sourceid != int(self.statements[statement][5]):
                    raise ValueError("Source origin differs from its statement occurrence")
        for row in self.records["LOC"]:
            domain, identity, sourceid = row[1], int(row[2]), int(row[4])
            if domain not in ("binding", "declaration", "expression"):
                continue
            if self.origins.get((domain, identity)) != sourceid:
                raise ValueError("Physical location differs from its source origin")
            if domain == "binding":
                fact, flag_column, range_column = self.records["B"][identity - 1], 3, 4
            elif domain == "expression":
                fact, flag_column, range_column = self.records["E"][identity - 1], 2, 3
            else:
                fact, flag_column, range_column = declarations[identity], 5, 6
            if row[3] == "range" and fact[flag_column] == "1":
                fileid = int(self.source_contexts[sourceid][3])
                written = [int(value) for value in fact[range_column + 1:range_column + 5]]
                observed = [int(value) for value in row[5:9]]
                # Procedure declarations can describe a whole header while
                # their LOC preserves only the observed opening token.
                matches = written == observed
                if domain == "declaration":
                    matches = (tuple(written[:2]) <= tuple(observed[:2])
                               and tuple(observed[2:]) <= tuple(written[2:]))
                if fact[range_column] != self.files[fileid][2] or not matches:
                    raise ValueError("Physical location differs from its written source range")

    def validate_procedure_exits(self, subject_modules: dict[tuple[str, int], int]) -> None:
        """Routine labels are selected compiler identities, including cleanup."""
        exits = {}
        for row in self.relations("procedure-exit-label"):
            if row[1] != "symbol" or row[3] != "symbol" or row[6] != "0":
                raise ValueError("Invalid procedure exit relation domain")
            procedure, label_id = int(row[2]), int(row[4])
            label = self.types.get(label_id)
            procedure_type = self.types.get(procedure)
            module = subject_modules.get(("symbol", procedure))
            if (procedure not in self.signatures or label is None or label[3] != "label"
                    or procedure_type is None or procedure in exits
                    or subject_modules.get(("symbol", label_id)) != module
                    or self.capabilities[module].get("procedure-exit-labels") != "available"):
                raise ValueError("Invalid, repeated or foreign procedure exit label")
            # Generated module constructors keep their label in the module
            # table; actual CL membership below proves body ownership.
            if int(label[8]) != procedure:
                if (procedure_type[18] != "compiler" or label[18] != "compiler"
                        or label[8] != procedure_type[8]):
                    raise ValueError("Procedure exit label belongs to another scope")
            exits[procedure] = label_id
        labels = {(int(row[1]), int(row[2])) for row in self.records["CL"]}
        for phase in self.records["PH"]:
            procedure = int(phase[2])
            module = subject_modules[("symbol", procedure)]
            if self.capabilities[module].get("procedure-exit-labels") == "available":
                if (int(phase[1]), exits.get(procedure)) not in labels:
                    raise ValueError("Procedure phase lacks its owned exit label")

    def validate_array_bounds(self, subject_modules: dict[tuple[str, int], int], array_modules: dict[int, int]) -> None:
        """Bound query identity survives fixed-array constant folding."""
        marked = set()
        for row in self.relations("array-bound-query"):
            if row[1] != "symbol" or row[3] != "expression" or row[6] != "0":
                raise ValueError("Invalid array bound query marker")
            symbol, identity = number(row[2], 1), number(row[4], 1)
            if identity in marked or self.properties["expression", identity].get("array-bound-symbol") != str(symbol):
                raise ValueError("Missing or repeated array bound marker")
            marked.add(identity)
        expressions = {int(row[1]): row for row in self.records["E"]}
        expected = {"array-bound-kind", "array-bound-symbol", "array-bound-dimension", "array-bound-selected-dimension"}
        optional = {"array-bound-dimension-explicit"}
        for (domain, identity), properties in self.properties.items():
            keys = {key for key in properties if key.startswith("array-bound-")}
            if not keys:
                continue
            if domain != "expression" or identity not in marked or not expected <= keys or keys - (expected | optional):
                raise ValueError("Incomplete or unknown array bound group")
            module = subject_modules.get((domain, identity))
            result = expressions.get(identity)
            symbol = number(properties["array-bound-symbol"], 1)
            target = self.symbols.get(symbol)
            if properties["array-bound-kind"] not in ("lower", "upper") or target is None or int(target[3]) not in (1, 12) or subject_modules.get(("symbol", symbol)) != module or array_modules.get(symbol) != module:
                raise ValueError("Array bound query selects another array")
            if self.capabilities[module].get("array-bound-query-inputs") != "available" or result is None or int(result[12]) & 511 != 8 or result[14] != "0":
                raise ValueError("Array bound result lacks its selected integer type")
            statement = self.statement_owners.get((domain, identity))
            if statement is None or self.statement_endings[statement][3] != "parsed":
                raise ValueError("Array bound query lacks accepted statement ownership")
            # Older receipts always carried an input expression. New producers
            # explicitly mark an omitted argument with zero, while retaining
            # the compiler-selected dimension as a real expression.
            explicit = properties.get("array-bound-dimension-explicit", "1")
            if explicit not in ("0", "1"):
                raise ValueError("Invalid array bound dimension presence")
            original = number(properties["array-bound-dimension"], 0)
            if (explicit == "0") != (original == 0):
                raise ValueError("Array bound dimension presence contradicts its input")
            selected = number(properties["array-bound-selected-dimension"], 1)
            if not original <= selected < identity:
                raise ValueError("Array bound dimensions are cyclic or reordered")
            for operand in ((selected,) if original == 0 else (original, selected)):
                if subject_modules.get(("expression", operand)) != module or self.statement_owners.get(("expression", operand)) != statement:
                    raise ValueError("Array bound query has a missing or foreign dimension")
            value = expressions.get(selected)
            if value is None or int(value[12]) & 511 != 8 or value[14] != "0":
                raise ValueError("Array bound query lacks its converted integer dimension")

    def validate_array_initializer_inputs(self, subject_modules: dict[tuple[str, int], int], array_modules: dict[int, int]) -> None:
        """Original elements retain their source array before TYPEINI consumes them."""
        required = {"array-initializer-symbol", "array-initializer-dimension", "array-initializer-kind"}
        ranks = {int(row[1]): int(row[2]) for row in self.records["A"]}
        for (domain, identity), properties in self.properties.items():
            observed = {key for key in properties if key.startswith("array-initializer-")}
            if not observed:
                continue
            if domain != "expression" or observed != required:
                raise ValueError("Incomplete or unknown array initializer input group")
            module = subject_modules.get((domain, identity))
            symbol = number(properties["array-initializer-symbol"], 1)
            dimension = number(properties["array-initializer-dimension"], 1)
            target = self.symbols.get(symbol)
            target_type = self.types.get(symbol)
            if (target is None or target_type is None or int(target[3]) not in (1, 12)
                    or target_type[18] != "source" or subject_modules.get(("symbol", symbol)) != module
                    or array_modules.get(symbol) != module or not 1 <= dimension <= 8
                    or dimension != ranks.get(symbol)):
                raise ValueError("Array initializer input selects another array or dimension")
            if (self.capabilities[module].get("original-array-initializer-inputs") != "available"
                    or properties["array-initializer-kind"] not in ("assignment", "constructor")):
                raise ValueError("Array initializer input capability or kind is unavailable")
            statement = self.statement_owners.get((domain, identity))
            if statement is None or self.statement_endings[statement][3] != "parsed":
                raise ValueError("Array initializer input lacks accepted statement ownership")

    def validate_array_subscripts(self, subject_modules: dict[tuple[str, int], int], array_modules: dict[int, int]) -> None:
        """Resolved arrays retain each original input before offset arithmetic."""
        ranks = {int(row[1]): int(row[2]) for row in self.records["A"]}
        marked = set()
        for row in self.relations("array-subscript"):
            if row[1] != "symbol" or row[3] != "expression" or row[6] != "0":
                raise ValueError("Invalid array subscript marker")
            symbol, identity = number(row[2], 1), number(row[4], 1)
            if identity in marked or self.properties["expression", identity].get("array-subscript-symbol") != str(symbol):
                raise ValueError("Missing or repeated array subscript marker")
            marked.add(identity)
        expressions = {int(row[1]): row for row in self.records["E"]}
        for (domain, identity), properties in self.properties.items():
            keys = {key for key in properties if key.startswith("array-subscript-")}
            if not keys:
                continue
            if domain != "expression" or identity not in marked:
                raise ValueError("Array subscript lacks its selected element")
            symbol = number(properties.get("array-subscript-symbol", ""), 1)
            rank = number(properties.get("array-subscript-rank", ""), 1)
            expected = {"array-subscript-symbol", "array-subscript-rank"} | {f"array-subscript-index:{dimension}" for dimension in range(rank)} | {f"array-subscript-selected-index:{dimension}" for dimension in range(rank)}
            if rank > 8 or keys != expected:
                raise ValueError("Incomplete or unknown array subscript group")
            module = subject_modules.get((domain, identity))
            target = self.symbols.get(symbol)
            result = expressions.get(identity)
            if target is None or int(target[3]) not in (1, 12) or subject_modules.get(("symbol", symbol)) != module or array_modules.get(symbol) != module or ranks.get(symbol) not in (-1, rank):
                raise ValueError("Array subscript selects another array or rank")
            if self.capabilities[module].get("array-subscript-inputs") != "available" or result is None:
                raise ValueError("Array subscript capability unavailable")
            if int(result[12]) & ~512 != int(target[4]) & ~512 or result[14] != target[5]:
                raise ValueError("Array subscript selects another element type")
            statement = self.statement_owners.get((domain, identity))
            if statement is None or self.statement_endings[statement][3] != "parsed":
                raise ValueError("Array subscript lacks accepted statement ownership")
            for dimension in range(rank):
                operand = number(properties[f"array-subscript-index:{dimension}"], 1)
                if operand >= identity or subject_modules.get(("expression", operand)) != module or self.statement_owners.get(("expression", operand)) != statement:
                    raise ValueError("Array subscript has a missing, cyclic or foreign input")
                selected = number(properties[f"array-subscript-selected-index:{dimension}"], 1)
                value = expressions.get(selected)
                if value is None or not operand <= selected < identity or int(value[12]) & 511 != 8 or value[14] != "0" or subject_modules.get(("expression", selected)) != module or self.statement_owners.get(("expression", selected)) != statement:
                    raise ValueError("Array subscript lacks its selected integer input")

    def validate_loop_conditions(self, subject_modules: dict[tuple[str, int], int]) -> None:
        """Accepted loop grammar retains predicates before branch folding."""
        inputs: dict[int, tuple[int, str, int]] = {}
        for (domain, owner), properties in self.properties.items():
            groups: dict[int, dict[str, str]] = defaultdict(dict)
            for key, value in properties.items():
                if not key.startswith("loop-condition-"):
                    continue
                role, separator, statement_text = key.partition(":")
                if domain != "symbol" or not separator or role not in ("loop-condition-kind", "loop-condition-expression"):
                    raise ValueError("Unknown loop condition receipt")
                groups[number(statement_text, 1)][role] = value
            for statement, group in groups.items():
                if len(group) != 2 or statement in inputs:
                    raise ValueError("Incomplete or repeated loop condition receipt")
                start, ending = self.statements.get(statement), self.statement_endings.get(statement)
                target = self.symbols.get(owner)
                module = subject_modules.get(("symbol", owner))
                if target is None or int(target[3]) != 3 or start is None or int(start[4]) != owner or int(start[7]) != module:
                    raise ValueError("Loop condition has another procedure or module")
                if self.capabilities[module].get("loop-condition-inputs") != "available" or ending is None or ending[2:4] != ["compound", "parsed"]:
                    raise ValueError("Loop condition lacks accepted grammar")
                kind = group["loop-condition-kind"]
                token = {"while": 273, "do": 278, "do-while": 278, "do-until": 278,
                         "loop": 279, "loop-while": 279, "loop-until": 279}.get(kind)
                if token is None or int(start[9]) != token:
                    raise ValueError("Loop condition kind disagrees with its statement")
                expression = number(group["loop-condition-expression"], 0)
                if (expression == 0) != (kind in ("do", "loop")):
                    raise ValueError("Loop condition lacks its original predicate")
                if expression and (subject_modules.get(("expression", expression)) != module or self.statement_owners.get(("expression", expression)) != statement):
                    raise ValueError("Loop predicate belongs to another statement")
                inputs[statement] = owner, kind, expression
        marked = set()
        for row in self.relations("loop-condition"):
            statement = number(row[6], 1)
            entry = inputs.get(statement)
            if row[1] != "symbol" or row[3] != "expression" or entry is None or statement in marked:
                raise ValueError("Missing or repeated loop predicate marker")
            if number(row[2], 1) != entry[0] or number(row[4], 1) != entry[2]:
                raise ValueError("Loop predicate marker has another input")
            marked.add(statement)
        openings, closings = {}, {}
        for block in self.constructs.values():
            if block[5] not in ("while", "do"):
                continue
            statement = int(block[3])
            if statement in openings:
                raise ValueError("Repeated loop opening")
            openings[statement] = block
            if block[5] == "do":
                statement = int(self.construct_endings[int(block[1])][2])
                if statement in closings:
                    raise ValueError("Repeated loop closure")
                closings[statement] = block
        for statement, start in self.statements.items():
            token, module = int(start[9]), int(start[7])
            if token not in (273, 278, 279) or self.statement_endings[statement][2:4] != ["compound", "parsed"] or self.capabilities[module].get("loop-condition-inputs") != "available":
                continue
            entry = inputs.get(statement)
            block = (closings if token == 279 else openings).get(statement)
            if entry is None or block is None or int(block[4]) != entry[0] or block[5] != ("while" if token == 273 else "do"):
                raise ValueError("Loop grammar lacks complete condition observations")
            if (statement in marked) != (entry[2] != 0):
                raise ValueError("Loop predicate lacks its marker")

    def validate_string_initializers(self, subject_modules: dict[tuple[str, int], int]) -> None:
        """Element capacity and initializer destination survive TYPEINI lowering."""
        expected = {"string-initializer-symbol", "string-initializer-dtype", "string-initializer-bytes"}
        marked = set()
        for row in self.records["H"]:
            if row[5] != "string-initializer":
                continue
            if row[1] != "symbol" or row[3] != "expression" or row[6] != "0":
                raise ValueError("Invalid string initializer marker")
            symbol, expression = number(row[2], 1), number(row[4], 1)
            if expression in marked or ("expression", expression) not in subject_modules:
                raise ValueError("Missing or repeated string initializer")
            properties = self.properties["expression", expression]
            if properties.get("string-initializer-symbol") != str(symbol) or subject_modules.get(("symbol", symbol)) != subject_modules["expression", expression]:
                raise ValueError("String initializer marker has another target")
            marked.add(expression)
        for (domain, identity), properties in self.properties.items():
            keys = {key for key in properties if key.startswith("string-initializer-")}
            if not keys:
                continue
            module = subject_modules.get((domain, identity))
            has_copy = self.capabilities.get(module, {}).get("string-initializer-copy-contracts") == "available"
            required = expected | ({"string-initializer-copy"} if has_copy else set())
            if domain != "expression" or keys != required or identity not in marked:
                raise ValueError("Incomplete or unknown string initializer receipt")
            module = subject_modules[domain, identity]
            if self.capabilities[module].get("string-initializer-targets") != "available":
                raise ValueError("String initializer capability unavailable")
            if has_copy and properties["string-initializer-copy"] not in {
                "unselected", "ambiguous", "runtime-terminated", "runtime-counted",
                "runtime-wide", "static-bytes", "static-wide"
            }:
                raise ValueError("Unknown string initializer copy contract")
            symbol = number(properties["string-initializer-symbol"], 1)
            dtype = number(properties["string-initializer-dtype"], 0)
            capacity = number(properties["string-initializer-bytes"], 0)
            target = self.symbols.get(symbol)
            if target is None or subject_modules.get(("symbol", symbol)) != module or int(target[3]) not in (1, 12):
                raise ValueError("String initializer has no declaration target")
            if dtype & 511 not in (4, 7, 18) or dtype != int(target[4]) or capacity != int(target[9]) or int(target[7]) & 0x40000:
                raise ValueError("String initializer capacity or type disagrees with storage")
            statement = self.statement_owners.get((domain, identity))
            start, ending = self.statements.get(statement), self.statement_endings.get(statement)
            if start is None or int(start[7]) != module or ending is None or ending[3] != "parsed":
                raise ValueError("String initializer lacks accepted statement ownership")

    def validate_string_intrinsics(self, subject_modules: dict[tuple[str, int], int]) -> None:
        """Inputs survive literal folding and runtime argument conversion."""
        expressions = {int(row[1]): row for row in self.records["E"]}
        marked = set()
        for row in self.relations("parsed-string-intrinsic"):
            if row[1] != "symbol" or row[3] != "expression" or row[6] != "0":
                raise ValueError("Invalid string intrinsic module marker")
            owner, identity = number(row[2], 1), number(row[4], 1)
            module = subject_modules["expression", identity]
            if int(self.symbols[owner][3]) != 8 or subject_modules["symbol", owner] != module or identity in marked:
                raise ValueError("String intrinsic marker has invalid ownership")
            if "string-intrinsic-kind" not in self.properties["expression", identity]:
                raise ValueError("String intrinsic marker lacks its observation")
            marked.add(identity)
        for (domain, identity), properties in self.properties.items():
            keys = {key for key in properties if key.startswith("string-intrinsic-")}
            if not keys:
                continue
            if domain != "expression" or identity not in marked:
                raise ValueError("String intrinsic lacks a module marker")
            module = subject_modules[domain, identity]
            if self.capabilities[module].get("parsed-string-intrinsics") != "available":
                raise ValueError("String intrinsic capability is unavailable")
            kind = properties.get("string-intrinsic-kind")
            count = number(properties.get("string-intrinsic-count", ""), 1)
            is_any = properties.get("string-intrinsic-any")
            if kind not in ("chr", "wchr", "uchr", "trim", "ltrim", "rtrim") or count > 32 or is_any not in ("0", "1"):
                raise ValueError("Invalid parsed string intrinsic selection")
            is_trim = kind in ("trim", "ltrim", "rtrim")
            if (is_trim and count > 2) or (is_any == "1" and (not is_trim or count != 2)):
                raise ValueError("Invalid string intrinsic argument shape")
            expected = {"string-intrinsic-kind", "string-intrinsic-count", "string-intrinsic-any"}
            expected.update(f"string-intrinsic-argument-{ordinal}" for ordinal in range(1, count + 1))
            if keys != expected:
                raise ValueError("Incomplete or unknown string intrinsic properties")
            statement = self.statement_owners.get((domain, identity))
            start, ending = self.statements.get(statement), self.statement_endings.get(statement)
            if start is None or int(start[7]) != module or ending is None or ending[3] != "parsed":
                raise ValueError("String intrinsic lacks accepted statement ownership")
            if int(expressions[identity][12]) & 511 not in (4, 7, 17, 26):
                raise ValueError("String intrinsic has a non-string result")
            expression = expressions[identity]
            if int(expression[8]) == 17:
                symbol = self.symbols[int(expression[13])]
                if is_trim or kind == "uchr" or not int(symbol[7]) & 1024:
                    raise ValueError("String intrinsic has an unrelated variable result")
                value_kind = self.constants.get(("expression", identity), (None, None))[0]
                if (kind == "chr" and (int(expression[12]) != 4 or value_kind != "bytes")) or (
                        kind == "wchr" and (int(expression[12]) != 7 or value_kind != "wide-units")):
                    raise ValueError("Folded character intrinsic has an inconsistent literal result")
            elif int(expression[8]) == 9:
                selected = self.types[int(expression[13])]
                if selected[18] != "runtime" or selected[3] != "procedure":
                    raise ValueError("String intrinsic lacks a selected runtime")
                if is_trim:
                    family = kind.upper() + ("ANY" if is_any == "1" else "EX" if count == 2 else "")
                    aliases = {"FB_" + family, "FB_WSTR" + family, "FB_USTR" + family}
                else:
                    aliases = {{"chr": "FB_CHR", "wchr": "FB_WSTRCHR", "uchr": "FB_USTRCHR"}[kind]}
                if selected[15].upper() not in aliases:
                    raise ValueError("String intrinsic selects an unrelated runtime")
            else:
                raise ValueError("String intrinsic has an invalid result class")
            for ordinal in range(1, count + 1):
                operand = number(properties[f"string-intrinsic-argument-{ordinal}"], 1)
                if operand >= identity or subject_modules.get(("expression", operand)) != module or self.statement_owners.get(("expression", operand)) != statement:
                    raise ValueError("Missing, cyclic or foreign string intrinsic input")

    def validate_file_transfers(self, subject_modules: dict[tuple[str, int], int]) -> None:
        """A transfer input is the typed object, before BYREF AS ANY lowering."""
        expressions = {int(row[1]): row for row in self.records["E"]}
        put_aliases = {"FB_FILEPUT", "FB_FILEPUTLARGE", "FB_FILEPUTSTR", "FB_FILEPUTSTRLARGE", "FB_FILEPUTARRAY", "FB_FILEPUTARRAYLARGE"}
        get_aliases = {"FB_FILEGET", "FB_FILEGETLARGE", "FB_FILEGETSTR", "FB_FILEGETWSTR", "FB_FILEGETSTRLARGE", "FB_FILEGETWSTRLARGE",
                       "FB_FILEGETARRAY", "FB_FILEGETARRAYLARGE", "FB_FILEGETIOB", "FB_FILEGETLARGEIOB", "FB_FILEGETSTRIOB", "FB_FILEGETWSTRIOB",
                       "FB_FILEGETSTRLARGEIOB", "FB_FILEGETWSTRLARGEIOB", "FB_FILEGETARRAYIOB", "FB_FILEGETARRAYLARGEIOB"}
        for (domain, identity), properties in self.properties.items():
            keys = {key for key in properties if key.startswith("file-transfer-")}
            if not keys:
                continue
            if domain != "expression" or keys != {"file-transfer-kind", "file-transfer-storage", "file-transfer-operand"}:
                raise ValueError("Incomplete or unknown file transfer input")
            if properties["file-transfer-kind"] not in ("get", "put") or properties["file-transfer-storage"] not in ("scalar", "array"):
                raise ValueError("Invalid file transfer input kind")
            operand = number(properties["file-transfer-operand"], 1)
            if operand >= identity or subject_modules.get(("expression", operand)) != subject_modules[domain, identity]:
                raise ValueError("Missing, cyclic or foreign file transfer input")
            expression = expressions[identity]
            if int(expression[8]) != 9:
                raise ValueError("File transfer has no selected call")
            if (properties["file-transfer-storage"] == "array") != (int(expressions[operand][8]) == 25):
                raise ValueError("File transfer shape disagrees with original input")
            module = subject_modules[domain, identity]
            statement = self.statement_owners.get((domain, identity))
            start = self.statements.get(statement)
            ending = self.statement_endings.get(statement)
            if start is None or int(start[7]) != module or ending is None or ending[3] != "parsed":
                raise ValueError("File transfer lacks accepted statement ownership")
            if self.statement_owners.get(("expression", operand)) != statement:
                raise ValueError("File transfer input belongs to another statement")
            selected = self.types[int(expression[13])]
            allowed = get_aliases if properties["file-transfer-kind"] == "get" else put_aliases
            if selected[18] != "runtime" or selected[3] != "procedure" or selected[15].upper() not in allowed:
                raise ValueError("File transfer selects an unrelated procedure")
            if self.capabilities[module].get("file-transfer-inputs") != "available":
                raise ValueError("File transfer capability unavailable")
        covered = set()
        for row in self.records["H"]:
            if row[1] != "node" or row[3] != "expression" or row[5] != "source-expression":
                continue
            node, expression = int(row[2]), int(row[4])
            if "file-transfer-kind" in self.properties["expression", expression] and self.nodes[node][9] == expressions[expression][13]:
                covered.add(node)
        for identity, node in self.nodes.items():
            module = subject_modules["node", identity]
            if int(node[4]) != 9 or self.capabilities[module].get("file-transfer-inputs") != "available":
                continue
            selected = self.types[int(node[9])]
            if selected[18] == "runtime" and selected[15].upper() in put_aliases | get_aliases and identity not in covered:
                raise ValueError("Selected file call lacks its typed input receipt")

    def validate_for_steps(self, subject_modules: dict[tuple[str, int], int]) -> None:
        """Keep accepted scalar steps distinct from converted counter values."""
        for (domain, identity), properties in self.properties.items():
            keys = {key for key in properties if key.startswith("for-step-") and not key.startswith("for-step-selected-")}
            for key in keys:
                prefix = next((item for item in ("for-step-explicit:", "for-step-expression:")
                               if key.startswith(item)), None)
                if domain != "symbol" or prefix is None:
                    raise ValueError("Invalid scalar FOR step property")
                statement = number(key[len(prefix):], 1)
                explicit_key = f"for-step-explicit:{statement}"
                expression_key = f"for-step-expression:{statement}"
                if explicit_key not in properties or expression_key not in properties:
                    raise ValueError("Incomplete scalar FOR step pair")
                symbol = self.symbols[identity]
                dtype = int(symbol[4]) & 0x1ff
                enum_counter = dtype == 10 and int(self.symbols.get(int(symbol[5]), [0, 0, 0, 0])[3]) == 9
                ordinary_counter = not int(symbol[5]) and dtype in (2, 3, 5, 6, 8, 9, 11, 12, 13, 14, 15, 16)
                if int(symbol[3]) != 1 or not (ordinary_counter or enum_counter):
                    raise ValueError("FOR step has no scalar counter")
                start = self.statements.get(statement)
                ending = self.statement_endings.get(statement)
                if start is None or int(start[9]) != 281 or ending is None or ending[3] != "parsed" or f"for-counter:{statement}" not in properties:
                    raise ValueError("FOR step lacks accepted counter ownership")
                module = subject_modules[domain, identity]
                if enum_counter and self.capabilities[module].get("enum-for-step-inputs") != "available":
                    raise ValueError("Enum FOR step lacks producer coverage")
                if int(start[7]) != module:
                    raise ValueError("FOR step belongs to another statement module")
                explicit = number(properties[explicit_key], 0)
                if explicit > 1:
                    raise ValueError("Invalid explicit FOR step flag")
                expression = number(properties[expression_key], 0)
                if bool(explicit) != bool(expression):
                    raise ValueError("Implicit FOR step has an expression or explicit step has none")
                if expression and (subject_modules.get(("expression", expression)) != module or
                                   self.statement_owners.get(("expression", expression)) != statement):
                    raise ValueError("FOR step expression is missing or belongs to another statement")
            if domain == "symbol":
                symbol = self.symbols[identity]
                module = subject_modules[domain, identity]
                dtype = int(symbol[4]) & 0x1ff
                enum_counter = (dtype == 10 and int(self.symbols.get(int(symbol[5]), [0, 0, 0, 0])[3]) == 9
                                and self.capabilities[module].get("enum-for-step-inputs") == "available")
                ordinary_counter = not int(symbol[5]) and dtype in (2, 3, 5, 6, 8, 9, 11, 12, 13, 14, 15, 16)
                scalar = int(symbol[3]) == 1 and (ordinary_counter or enum_counter)
                if scalar and self.capabilities[module].get("scalar-for-step-inputs") == "available":
                    for key in properties:
                        if key.startswith("for-counter:"):
                            statement = number(key[12:], 1)
                            if f"for-step-explicit:{statement}" not in properties or f"for-step-expression:{statement}" not in properties:
                                raise ValueError("Scalar FOR lacks original step coverage")
                            if self.capabilities[module].get("scalar-for-step-sites") == "available":
                                site = self.physical_locations.get(("statement", statement, "for-step"))
                                explicit = properties[f"for-step-explicit:{statement}"] == "1"
                                macro_site = any(row[1] == "statement" and int(row[2]) == statement and row[4] == "for-step"
                                                 for row in self.records["MR"])
                                if explicit != (site is not None or macro_site):
                                    raise ValueError("FOR STEP source site disagrees with explicit clause")

    def validate_for_inputs(self, subject_modules: dict[tuple[str, int], int]) -> None:
        """Require original bounds and selected STEP snapshots from their producer."""
        expressions = {int(row[1]): row for row in self.records["E"]}
        prefixes = ("for-start-expression:", "for-limit-expression:",
                    "for-step-selected-dtype:", "for-step-selected-expression:")
        for (domain, identity), properties in self.properties.items():
            keys = {key for key in properties if key.startswith(prefixes)}
            for key in keys:
                if domain != "symbol":
                    raise ValueError("FOR input belongs to another domain")
                prefix = next(item for item in prefixes if key.startswith(item))
                statement = number(key[len(prefix):], 1)
                symbol = self.symbols[identity]
                dtype = int(symbol[4]) & 0x1ff
                scalar = int(symbol[3]) == 1 and (
                    (not int(symbol[5]) and dtype in (2, 3, 5, 6, 8, 9, 11, 12, 13, 14, 15, 16)) or
                    (dtype == 10 and int(self.symbols.get(int(symbol[5]), [0, 0, 0, 0])[3]) == 9))
                start = self.statements.get(statement)
                module = subject_modules[domain, identity]
                if not scalar or start is None or int(start[9]) != 281 or int(start[7]) != module or \
                        self.statement_endings[statement][3] != "parsed" or f"for-counter:{statement}" not in properties:
                    raise ValueError("FOR input lacks accepted scalar ownership")
                bound = prefix in prefixes[:2]
                capability = "scalar-for-bound-inputs" if bound else "scalar-for-selected-steps"
                if self.capabilities[module].get(capability) != "available":
                    raise ValueError("FOR input lacks producer coverage")
                group = prefixes[:2] if bound else prefixes[2:]
                if any(item + str(statement) not in properties for item in group):
                    raise ValueError("Incomplete scalar FOR input group")
                expression_key = f"for-step-selected-expression:{statement}"
                expression = number(properties[key] if bound else properties[expression_key], int(bound))
                if expression:
                    row = expressions.get(expression)
                    if row is None or subject_modules.get(("expression", expression)) != module or \
                            self.statement_owners.get(("expression", expression)) != statement:
                        raise ValueError("FOR input expression belongs to another statement")
                if not bound:
                    selected_dtype = number(properties[f"for-step-selected-dtype:{statement}"], 0)
                    selected_type = selected_dtype & 0x1ff
                    if selected_type not in (2, 3, 5, 6, 8, 9, 10, 11, 12, 13, 14, 15, 16) or \
                            self.primitives[module - 1][selected_type][4] != self.primitives[module - 1][dtype][4]:
                        raise ValueError("Selected FOR step has another storage width")
                    explicit = properties.get(f"for-step-explicit:{statement}")
                    if not expression:
                        original = int(properties.get(f"for-step-expression:{statement}", "0"))
                        if explicit != "1" or ("expression", original) in self.constants:
                            raise ValueError("Known FOR step lacks selected constant")
                    else:
                        row = expressions[expression]
                        value = self.constants.get(("expression", expression))
                        if int(row[8]) != 16 or int(row[12]) != selected_dtype or value is None or \
                                value[0] not in ("signed", "unsigned", "float64-bits"):
                            raise ValueError("Selected FOR step is not a typed constant")
                        if explicit == "0" and value[1] != ("0x3FF0000000000000" if value[0] == "float64-bits" else "1"):
                            raise ValueError("Implicit FOR step is not the selected unit")
            if domain == "symbol":
                symbol = self.symbols[identity]
                dtype = int(symbol[4]) & 0x1ff
                scalar = int(symbol[3]) == 1 and (
                    (not int(symbol[5]) and dtype in (2, 3, 5, 6, 8, 9, 11, 12, 13, 14, 15, 16)) or
                    (dtype == 10 and int(self.symbols.get(int(symbol[5]), [0, 0, 0, 0])[3]) == 9))
                if not scalar:
                    continue
                module = subject_modules[domain, identity]
                for key in properties:
                    if not key.startswith("for-counter:"):
                        continue
                    statement = key[12:]
                    for capability, group in (("scalar-for-bound-inputs", prefixes[:2]),
                                             ("scalar-for-selected-steps", prefixes[2:])):
                        if self.capabilities[module].get(capability) == "available" and \
                                any(item + statement not in properties for item in group):
                            raise ValueError("Scalar FOR lacks complete input coverage")

    def validate_numeric_selections(self, subject_modules: dict[tuple[str, int], int]) -> None:
        required = {"numeric-selected-left-dtype", "numeric-selected-right-dtype",
                    "numeric-selected-left-expression", "numeric-selected-right-expression"}
        expressions = {int(row[1]): row for row in self.records["E"]}
        for (domain, identity), properties in self.properties.items():
            selected = {key for key in properties if key.startswith("numeric-selected-")}
            if not selected:
                continue
            if domain != "expression" or selected != required:
                raise ValueError("Incomplete numeric operand selections")
            operands = self.expression_operands.get(identity)
            if operands is None or operands[2] != "binary":
                raise ValueError("Numeric operand selections have no original binary operation")
            for operand_id in operands[4:6]:
                operand = expressions.get(int(operand_id))
                if operand is None or int(operand[14]) or int(operand[12]) & 0x1e0 or int(operand[12]) & 31 not in (1, 2, 3, 5, 6, 8, 9, 11, 12, 13, 14, 15, 16):
                    raise ValueError("Numeric selection has no original primitive operand")
            module = subject_modules[domain, identity]
            for side in ("left", "right"):
                dtype = number(properties[f"numeric-selected-{side}-dtype"], 0)
                if dtype & 0x1e0 or dtype & 31 not in (1, 2, 3, 5, 6, 8, 9, 11, 12, 13, 14, 15, 16):
                    raise ValueError("Numeric selection has an unsupported dtype")
                if dtype & 31 not in self.primitives[module - 1]:
                    raise ValueError("Numeric selection lacks a primitive type receipt")
                selected_id = number(properties[f"numeric-selected-{side}-expression"], 1)
                selected_expression = expressions.get(selected_id)
                if selected_expression is None or selected_id >= identity or subject_modules["expression", selected_id] != module:
                    raise ValueError("Selected numeric operand is missing, forward or foreign")
                if int(selected_expression[14]) or int(selected_expression[12]) & 0x1ff != dtype & 0x1ff:
                    raise ValueError("Selected numeric operand has a different type")

    def validate_numeric_literals(self, subject_modules: dict[tuple[str, int], int]) -> None:
        """Literal origin is distinct from the value of an arbitrary constant."""
        required = {"numeric-literal-kind", "numeric-literal-base", "numeric-literal-text", "numeric-literal-text-complete"}
        expressions = {int(row[1]): row for row in self.records["E"]}
        for (domain, identity), properties in self.properties.items():
            keys = {key for key in properties if key.startswith("numeric-literal-")}
            if not keys:
                continue
            if domain != "expression" or keys != required:
                raise ValueError("Incomplete numeric literal observations")
            expression = expressions.get(identity)
            if expression is None or int(expression[14]) or int(expression[12]) & 0x1e0:
                raise ValueError("Numeric literal is not a primitive scalar")
            dtype = int(expression[12]) & 31
            module = subject_modules[domain, identity]
            if dtype not in (2, 3, 5, 6, 8, 9, 11, 12, 13, 14, 15, 16) or dtype not in self.primitives[module - 1]:
                raise ValueError("Numeric literal lacks a primitive type")
            value = self.constants.get((domain, identity))
            kind = properties["numeric-literal-kind"]
            if value is None or (dtype in (15, 16) and (kind != "float" or value[0] != "float64-bits")) or (dtype not in (15, 16) and (kind != "integer" or value[0] not in ("signed", "unsigned"))):
                raise ValueError("Numeric literal kind disagrees with its compiler value")
            text = properties["numeric-literal-text"]
            complete = properties["numeric-literal-text-complete"]
            if not 1 <= len(text) <= 1024 or complete not in ("0", "1") or (complete == "1" and len(text) >= 64):
                raise ValueError("Invalid numeric literal text bounds")
            radix = properties["numeric-literal-base"]
            if radix == "decimal":
                allowed = "0123456789.eEdD+-" if kind == "float" else "0123456789"
            else:
                prefixes = {"hex": ("&H", "0123456789abcdefABCDEF"), "octal": ("&O", "01234567"), "binary": ("&B", "01")}
                if radix not in prefixes or text[:2].upper() != prefixes[radix][0]:
                    raise ValueError("Numeric literal radix disagrees with token text")
                allowed = prefixes[radix][1]
                text = text[2:]
            if not text or any(character not in allowed for character in text):
                raise ValueError("Invalid canonical numeric token characters")

    def validate_numeric_assignments(self, subject_modules: dict[tuple[str, int], int]) -> None:
        """Destination properties supplement, never replace, the RHS type."""
        for (domain, identity), properties in self.properties.items():
            dtype = properties.get("assignment-target-dtype")
            kind = properties.get("assignment-kind")
            if dtype is None and kind is None:
                continue
            if domain != "expression" or dtype is None or kind not in ("assignment", "initializer"):
                raise ValueError("Incomplete numeric assignment destination")
            target = number(dtype, 0)
            primitive_types = self.primitives[subject_modules["expression", identity] - 1]
            if target & 0x1E0 or target & 31 not in primitive_types:
                raise ValueError("Invalid numeric assignment destination type")
            if target & 31 not in (1, 2, 3, 5, 6, 8, 9, 11, 12, 13, 14, 15, 16):
                raise ValueError("Assignment destination is not a primitive numeric scalar")

    def validate_source_assignments(self, subject_modules: dict[tuple[str, int], int]) -> None:
        """Named source destinations must be complete compiler-owned pairs."""
        parameter_variable_attributes = 0x4000 | 0x8000 | 0x10000
        parameter_variables = {int(row[7]) for parameters in self.parameters.values()
                               for row in parameters.values() if int(row[7]) > 0}
        required = {"source-assignment-symbol", "source-assignment-kind"}
        for (domain, identity), properties in self.properties.items():
            observed = required & properties.keys()
            if not observed:
                continue
            if domain != "expression" or observed != required:
                raise ValueError("Incomplete source assignment destination")
            module = subject_modules[domain, identity]
            target_id = number(properties["source-assignment-symbol"], 1)
            target = self.symbols.get(target_id)
            target_type = self.types.get(target_id)
            if (self.capabilities[module].get("source-assignment-targets") != "available"
                    or properties["source-assignment-kind"] not in ("assignment", "initializer")
                    or target is None or target_type is None
                    or int(target[3]) not in (1, 2, 4, 12)
                    or subject_modules.get(("symbol", target_id)) != module):
                raise ValueError("Invalid source assignment destination")
            if target_type[18] != "source":
                if (int(target[3]) != 1 or not int(target[7]) & parameter_variable_attributes
                        or target_id not in parameter_variables):
                    raise ValueError("Generated assignment destination lacks a source formal")

    def validate_call_atoms(self, subject_modules: dict[tuple[str, int], int]) -> None:
        """Keep original bound identities separate from folded expression values."""
        expressions = {int(row[1]): row for row in self.records["E"]}
        for (domain, identity), properties in self.properties.items():
            observed = {"constant-symbol", "constant-atom-kind", "bound-value-symbol"} & properties.keys()
            if not observed:
                continue
            expression = expressions.get(identity)
            module = subject_modules.get((domain, identity))
            if domain != "expression" or expression is None or module is None:
                raise ValueError("Invalid original call atom domain")
            if "bound-value-symbol" in observed:
                symbol_id = number(properties["bound-value-symbol"], 1)
                symbol = self.symbols.get(symbol_id)
                if (symbol is None or int(symbol[3]) not in (1, 12)
                        or int(expression[8]) not in (17, 19, 20)
                        or subject_modules.get(("symbol", symbol_id)) != module
                        or self.capabilities[module].get("bound-expression-symbols") != "available"):
                    raise ValueError("Invalid original bound value symbol")
            constant_keys = {"constant-symbol", "constant-atom-kind"} & observed
            if not constant_keys:
                continue
            if constant_keys != {"constant-symbol", "constant-atom-kind"}:
                raise ValueError("Incomplete original constant atom")
            symbol_id = number(properties["constant-symbol"], 1)
            symbol = self.symbols.get(symbol_id)
            kind = properties["constant-atom-kind"]
            if (symbol is None or int(symbol[3]) != 2 or int(expression[8]) not in (16, 17)
                    or kind not in ("named-constant", "boolean-literal")
                    or subject_modules.get(("symbol", symbol_id)) != module
                    or self.capabilities[module].get("parsed-constant-symbols") != "available"):
                raise ValueError("Invalid original constant atom")
            boolean_literal = int(symbol[4]) == 1 and bool(int(symbol[7]) & 0x800)
            if (kind == "boolean-literal") != boolean_literal:
                raise ValueError("Constant atom kind disagrees with selected symbol")
            if boolean_literal and (expression[8] != "16" or expression[12] != "1"):
                raise ValueError("Boolean literal lacks its original Boolean input")
        static_targets: dict[int, str] = {}
        for row in self.records["H"]:
            if row[1] == "node" and row[3] == "symbol" and row[5] == "static-target":
                node_id = int(row[2])
                if node_id in static_targets:
                    raise ValueError("Repeated static call target")
                static_targets[node_id] = row[4]
        for row in self.records["H"]:
            if row[1] != "node" or row[3] != "binding" or row[5] != "source-binding":
                continue
            node_id = int(row[2])
            node = self.nodes[node_id]
            if node[4] != "9" or self.properties["node", node_id].get("call-kind") not in ("direct", "virtual"):
                continue
            binding_id = number(row[4], 1)
            binding = self.records["B"][binding_id - 1]
            target_id = static_targets.get(node_id) if self.properties["node", node_id]["call-kind"] == "virtual" else node[9]
            if (binding[2] != "reference" or binding[1] != target_id
                    or subject_modules.get(("binding", binding_id)) != subject_modules["node", node_id]):
                raise ValueError("Source call binding disagrees with selected procedure")

    def validate_formal_defaults(self, subject_modules: dict[tuple[str, int], int]) -> None:
        """Original optional expressions belong to source formal parameters."""
        parameter_rows = {int(row[2]): row for parameters in self.parameters.values()
                          for row in parameters.values()}
        # Reused headers retain original defaults and typed source formals,
        # while G describes the selected signature. Resolve only explicit
        # replacement edges and reject cycles before following that contract.
        canonical = {int(row[2]): int(row[4]) for row in self.relations("canonical-symbol")
                     if row[1] == row[3] == "symbol"}
        resolved: dict[int, int] = {}
        for identity in canonical:
            target = identity
            visited: set[int] = set()
            while target in canonical and target not in resolved:
                if target in visited:
                    raise ValueError("Cycle in canonical formal relationships")
                visited.add(target)
                target = canonical[target]
            target = resolved.get(target, target)
            for previous in visited:
                resolved[previous] = target
        default_nodes = {int(row[2]) for row in self.records["H"]
                         if row[1] == "symbol" and row[3] == "node"
                         and row[5] == "default-initializer" and row[6] == "0"}
        observed: set[int] = set()
        for (domain, identity), properties in self.properties.items():
            value = properties.get("formal-default-expression")
            if value is None:
                continue
            observed.add(identity)
            module = subject_modules.get((domain, identity))
            expression = number(value, 1)
            parameter = self.symbols.get(identity)
            selected = resolved.get(identity, identity)
            parameter_row = parameter_rows.get(selected)
            if (domain != "symbol" or parameter is None or int(parameter[3]) != 4
                    or module is None
                    or self.capabilities[module].get("formal-default-inputs") != "available"
                    or subject_modules.get(("expression", expression)) != module
                    or subject_modules.get(("symbol", selected)) != module
                    or parameter_row is None or parameter_row[4] == "vararg"
                    or resolved.get(int(parameter[11]), int(parameter[11])) != int(parameter_row[1])
                    or parameter_row[5] != "1" or identity not in default_nodes):
                raise ValueError("Invalid formal default input")
        for identity in default_nodes:
            module = subject_modules.get(("symbol", identity))
            symbol_type = self.types.get(identity)
            if (module is not None and symbol_type is not None and symbol_type[18] == "source"
                    and self.capabilities[module].get("formal-default-inputs") == "available"
                    and identity not in observed):
                raise ValueError("Source formal default lacks its original input")

    def validate_size_queries(self, subject_modules: dict[tuple[str, int], int]) -> None:
        """Preserve the selected input independently of the folded size result."""
        required = {"size-query-kind", "size-query-dtype", "size-query-subtype",
                    "size-query-operand", "size-query-input"}
        for (domain, identity), properties in self.properties.items():
            observed = {key for key in properties if key.startswith("size-query-")}
            if not observed:
                continue
            if domain != "expression" or observed != required:
                raise ValueError("Incomplete or unknown size query input receipt")
            if properties["size-query-kind"] not in ("len", "sizeof") or properties["size-query-input"] not in ("type", "expression", "array"):
                raise ValueError("Invalid size query input kind")
            module = subject_modules["expression", identity]
            dtype = number(properties["size-query-dtype"], 0)
            if dtype & 31 not in self.primitives[module - 1] or (dtype & 0x1E0) >> 5 > 8:
                raise ValueError("Invalid size query input type")
            subtype = number(properties["size-query-subtype"], 0)
            if subtype and subtype not in self.symbols:
                raise ValueError("Missing size query nominal type")
            operand = number(properties["size-query-operand"], 0)
            if operand and subject_modules.get(("expression", operand)) != module:
                raise ValueError("Missing or foreign size query operand")
            if properties["size-query-input"] == "type" and operand:
                raise ValueError("Type-only size query has an expression operand")

    def validate_storage_receipts(self, subject_modules: dict[tuple[str, int], int]) -> None:
        """Keep parser-selected storage meaning distinct from generated calls."""
        expressions = {int(row[1]): row for row in self.records["E"]}
        address_keys = {"kind", "dtype", "subtype", "operand", "temporary"}
        new_keys = {"kind", "dtype", "subtype", "elements", "clear", "placement", "placement-operand"}
        call_owners = {}
        call_work_left = len(self.nodes) * 4
        for (domain, identity), properties in self.properties.items():
            for key, value in properties.items():
                if key == "call-argument-expression":
                    operand = number(value, 1)
                    node = self.nodes.get(identity)
                    module = subject_modules.get((domain, identity))
                    if domain != "node" or node is None or node[4] != "22" or subject_modules.get(("expression", operand)) != module:
                        raise ValueError("Missing or foreign selected call argument input")
                    if properties.get("default-argument") != "0":
                        raise ValueError("Default argument claims a selected caller expression")
                    parameter = self.symbols.get(int(node[9]))
                    call = node
                    while call is not None and call[4] == "22":
                        call_work_left -= 1
                        if call_work_left < 0:
                            raise ValueError("Selected argument traversal budget exceeded")
                        if int(call[1]) in call_owners:
                            call = call_owners[int(call[1])]
                            break
                        call = self.nodes.get(int(call[2]))
                    if parameter is None or parameter[3] != "4" or call is None or call[4] != "9" or call[9] != parameter[11]:
                        raise ValueError("Selected call argument does not belong to its formal procedure")
                    call_owners[identity] = call
                if key.startswith("procedure-prototype-statement-"):
                    statement = number(value, 1)
                    if domain != "symbol" or int(self.symbols[identity][3]) != 3 or key[30:] != value:
                        raise ValueError("Invalid procedure prototype statement owner")
                    if statement not in self.statements or int(self.statements[statement][7]) != subject_modules[domain, identity]:
                        raise ValueError("Missing or foreign procedure prototype statement")
        for row in self.records["H"]:
            if row[1] == "node" and row[3] == "expression" and row[5] == "source-expression":
                identity = int(row[2])
                module = subject_modules["node", identity]
                if self.nodes[identity][4] == "22" and self.capabilities[module].get("selected-call-argument-inputs") == "available":
                    if self.properties["node", identity].get("call-argument-expression") != row[4]:
                        raise ValueError("Missing selected call argument receipt")
        for (domain, identity), properties in self.properties.items():
            for prefix, keys in (("pointer-address-", address_keys), ("pointer-dereference-", {"operand", "count"}), ("pointer-index-", {"operand", "index"}), ("memory-new-", new_keys),
                                 ("memory-release-", {"kind"})):
                observed = {key[len(prefix):] for key in properties if key.startswith(prefix)}
                if not observed:
                    continue
                if observed != keys:
                    raise ValueError("Incomplete or unknown storage receipt")
                values = {key: properties[prefix + key] for key in keys}
                module = subject_modules[domain, identity]
                if prefix == "pointer-index-":
                    operand = number(values["operand"], 1)
                    index = number(values["index"], 1)
                    if domain != "expression" or operand >= identity or index >= identity or any(
                            subject_modules.get(("expression", item)) != module for item in (operand, index)):
                        raise ValueError("Missing, cyclic or foreign pointer index input")
                    if not int(expressions[operand][12]) & 0x1E0 or int(expressions[index][12]) & 0x1E0:
                        raise ValueError("Pointer index inputs contradict their selected types")
                    if self.primitives[module - 1][int(expressions[index][12]) & 31][3] not in ("integer", "float"):
                        raise ValueError("Pointer index is not numeric")
                    continue
                if prefix == "pointer-dereference-":
                    operand = number(values["operand"], 1)
                    count = number(values["count"], 1)
                    if domain != "expression" or operand >= identity or subject_modules.get(("expression", operand)) != module:
                        raise ValueError("Missing, cyclic or foreign dereference input")
                    input_type = int(expressions[operand][12]) & 0x1FF
                    if count > 8 or count > (input_type & 0x1E0) >> 5:
                        raise ValueError("Dereference count exceeds its typed input")
                    if int(expressions[identity][12]) & 0x1FF != input_type - count * 32 or expressions[identity][14] != expressions[operand][14]:
                        raise ValueError("Dereference result contradicts its selected input")
                    continue
                if prefix == "memory-new-":
                    symbol = self.symbols[identity]
                    if domain != "symbol" or int(symbol[3]) != 1 or not int(symbol[7]) & 0x1000:
                        raise ValueError("NEW storage is not a compiler temporary variable")
                    if values["kind"] not in ("scalar", "array") or values["clear"] not in ("0", "1") or values["placement"] not in ("0", "1"):
                        raise ValueError("Invalid NEW selection")
                    if values["elements"] != "unknown" and (len(values["elements"]) > 20 or number(values["elements"], 0) > 2**64 - 1):
                        raise ValueError("NEW element count exceeds unsigned storage")
                    placement = number(values["placement-operand"], 0)
                    if bool(placement) != (values["placement"] == "1"):
                        raise ValueError("NEW placement flag contradicts its operand")
                    if placement and (subject_modules.get(("expression", placement)) != module or not int(expressions[placement][12]) & 0x1E0):
                        raise ValueError("NEW placement lacks its selected pointer expression")
                else:
                    if domain != "expression" or not int(expressions[identity][12]) & 0x1E0:
                        raise ValueError("Storage receipt result is not a pointer expression")
                    if prefix == "memory-release-":
                        if values["kind"] not in ("scalar", "array"):
                            raise ValueError("Invalid DELETE selection")
                        continue
                    if values["kind"] not in ("address-of", "varptr", "strptr") or values["temporary"] not in ("0", "1"):
                        raise ValueError("Invalid address selection")
                    operand = number(values["operand"], 1)
                    if operand >= identity or subject_modules.get(("expression", operand)) != module:
                        raise ValueError("Missing, cyclic or foreign address input")
                dtype = number(values["dtype"], 0)
                subtype = number(values["subtype"], 0)
                if dtype & 31 not in self.primitives[module - 1] or (dtype & 0x1E0) >> 5 > 8:
                    raise ValueError("Invalid selected storage type")
                if subtype and subtype not in self.symbols:
                    raise ValueError("Missing selected storage nominal type")
                if prefix == "memory-new-":
                    temporary_type = int(symbol[4])
                    if temporary_type & 31 != dtype & 31 or temporary_type & 0x1E0 != (dtype & 0x1E0) + 0x20 or int(symbol[5]) != subtype:
                        raise ValueError("NEW temporary type contradicts its selected pointee")

        self.validate_pointer_origins(subject_modules)

    def validate_pointer_origins(self, subject_modules: dict[tuple[str, int], int]) -> None:
        """Independent origins detect property groups lost during transport."""
        observed: set[tuple[int, str]] = set()
        for row in self.records["H"]:
            if row[5] not in ("pointer-dereference-input", "pointer-index-input"):
                continue
            result, operand, detail = map(int, (row[2], row[4], row[6]))
            module = subject_modules.get(("expression", result))
            prefix = "pointer-dereference-" if row[5] == "pointer-dereference-input" else "pointer-index-"
            key = "count" if prefix == "pointer-dereference-" else "index"
            if (row[1] != "expression" or row[3] != "expression" or module is None
                    or subject_modules.get(("expression", operand)) != module or operand >= result
                    or self.capabilities[module].get("pointer-access-origins") != "available"
                    or (result, prefix) in observed):
                raise ValueError("Invalid original pointer access relation")
            properties = self.properties["expression", result]
            if properties.get(prefix + "operand") != row[4] or properties.get(prefix + key) != row[6]:
                raise ValueError("Original pointer access differs from its property group")
            statement = self.statement_owners.get(("expression", result))
            if (statement is None or self.statement_endings[statement][3] != "parsed"
                    or self.statement_owners.get(("expression", operand)) != statement):
                raise ValueError("Original pointer access crosses statement ownership")
            if key == "index" and (subject_modules.get(("expression", detail)) != module or detail >= result
                                   or self.statement_owners.get(("expression", detail)) != statement):
                raise ValueError("Original pointer index lacks its accepted input owner")
            observed.add((result, prefix))
        for (domain, identity), properties in self.properties.items():
            module = subject_modules[domain, identity]
            if self.capabilities[module].get("pointer-access-origins") != "available":
                continue
            for prefix in ("pointer-dereference-", "pointer-index-"):
                if prefix + "operand" in properties and (identity, prefix) not in observed:
                    raise ValueError("Pointer property group lacks its original access relation")

    def validate_formal_spans(self) -> None:
        declarations = {int(row[1]): row for row in self.records["DCL"]}
        physical_parameters: set[int] = set()
        for (domain, identity), properties in self.properties.items():
            kind = properties.get("formal-span-kind")
            if kind is None:
                continue
            if domain != "symbol" or int(self.symbols[identity][3]) != SYMBOL_CLASSES.index("parameter") + 1:
                raise ValueError("Formal span kind belongs to a nonparameter")
            if kind not in ("physical", "generated"):
                raise ValueError("Unknown formal span kind")
            if kind == "physical":
                physical_parameters.add(identity)
        observed: set[int] = set()
        for (domain, identity, role), span in self.physical_locations.items():
            if domain != "declaration" or role not in ("formal", "formal-generated"):
                continue
            declaration = declarations[identity]
            parameter = int(declaration[2])
            if declaration[3] not in ("parameter-prototype", "parameter-definition") or int(self.symbols[parameter][3]) != SYMBOL_CLASSES.index("parameter") + 1:
                raise ValueError("Formal span belongs to a nonparameter declaration")
            if int(span[4]) != self.origins.get(("declaration", identity)):
                raise ValueError("Formal span belongs to another source occurrence")
            other = "formal-generated" if role == "formal" else "formal"
            if (domain, identity, other) in self.physical_locations:
                raise ValueError("Formal has contradictory physical and generated spans")
            if role == "formal":
                name = self.physical_locations.get((domain, identity, "range"))
                if name is None or span[4] != name[4] or tuple(map(int, span[5:7])) > tuple(map(int, name[5:7])) or tuple(map(int, span[7:9])) < tuple(map(int, name[7:9])):
                    raise ValueError("Complete formal span does not contain its name or first token")
                observed.add(parameter)
            kind = self.properties["symbol", parameter].get("formal-span-kind")
            if kind != ("physical" if role == "formal" else "generated"):
                raise ValueError("Formal span disagrees with its parser-owned kind")
        if physical_parameters != observed:
            raise ValueError("Physical formal lacks its complete declaration span")
        if any(features.get("formal-parameter-spans") == "available" for features in self.capabilities.values()):
            for declaration in declarations.values():
                if declaration[3] in ("parameter-prototype", "parameter-definition") and "formal-span-kind" not in self.properties["symbol", int(declaration[2])]:
                    raise ValueError("Parameter declaration lacks its formal span observation")

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
            if not complete_ordinals([int(row[2]) for row in parameters], count, ordered=True):
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
            if (result[2] in ("expanded", "failed", "recovered")
                    and not complete_ordinals(list(args), argument_count)):
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
                if not complete_ordinals([number(row[2], 0) for row in tokens],
                                         number(properties["assembly-token-count"], 0), ordered=True):
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
                if not complete_ordinals(auxiliary.get((identity, "copyback"), []),
                                         number(properties["copyback-count"], 0)):
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

        source_field_counts = Counter(int(row[8]) for row in self.types.values()
                                      if row[3] == "field" and row[18] == "source")
        array_ranks = {int(array[1]): int(array[2]) for array in self.records["A"]}
        for identity, row in self.types.items():
            # Optional K receipts preserve old producers. When present, they
            # describe completed source membership, not hidden storage slots.
            properties = self.properties.get(("symbol", identity), {})
            count = properties.get("declared-field-count")
            if count is not None:
                if row[3] not in ("type", "union") or properties.get("layout-finalized") != "1":
                    raise ValueError("Declared field count lacks a finalized aggregate")
                if int(count) != source_field_counts[identity]:
                    raise ValueError("Declared field count disagrees with source membership")
            rank = properties.get("field-array-rank")
            if rank is not None:
                if row[3] != "field" or int(rank) != array_ranks.get(identity, 0):
                    raise ValueError("Field array rank disagrees with its array contract")
            marker = self.properties.get(("symbol", identity), {}).get("written-override")
            if marker is not None:
                owner = self.types.get(int(row[8]))
                if row[3] != "procedure" or owner is None or owner[3] not in ("type", "union"):
                    raise ValueError("Written override receipt lacks a member declaration")
                signature = self.signatures.get(identity)
                if marker == "1" and (signature is None or signature[11] == "0"):
                    raise ValueError("Written override check lacks its resolved base method")
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
            if not complete_ordinals(list(explicit), int(signature[4])):
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
    def read(cls, path: Path, *, limits: ReaderLimits | None = None,
             **options: bool) -> Model:
        """Read a regular ASCII sidecar without crossing configured bounds."""
        limits = limits or ReaderLimits()
        path = Path(path)
        try:
            metadata = path.stat()
            if not stat.S_ISREG(metadata.st_mode):
                raise ValueError("Semantic sidecar is not a regular file")
            if metadata.st_size > limits.max_sidecar_bytes:
                raise ValueError("Semantic sidecar exceeds the configured byte limit")
            with path.open("rb") as stream:
                data = stream.read(limits.max_sidecar_bytes + 1)
        except OSError as error:
            raise ValueError("Semantic sidecar is unreadable: " + str(path)) from error
        if len(data) > limits.max_sidecar_bytes:
            raise ValueError("Semantic sidecar exceeds the configured byte limit")
        if len(data) != metadata.st_size:
            raise ValueError("Semantic sidecar changed while it was being read")
        try:
            text = data.decode("ascii")
        except UnicodeDecodeError as error:
            raise ValueError("Semantic sidecar wire data is not ASCII") from error
        return cls(text, limits=limits, **options)

    def verified_source_contents(self) -> dict[int, bytes]:
        """Read each current source revision once and verify its captured digest.

        The cache is deliberately scoped to this call. A later validation or
        edit plan reopens the files, so stale bytes cannot survive between
        consumer operations.
        """
        contents: dict[int, bytes] = {}
        cache: dict[tuple[Path, int, str], bytes] = {}
        cached_bytes = 0
        for identity, row in self.files.items():
            if row[6] != "regular":
                raise ValueError("Stream source has no immutable file revision")
            expected_size = int(row[3])
            if expected_size > self.limits.max_source_bytes:
                raise ValueError("Source revision exceeds the configured per-file byte limit: " + row[2])
            path = Path(row[2])
            try:
                metadata = path.stat()
                if not stat.S_ISREG(metadata.st_mode):
                    raise ValueError("Source revision is not a regular file: " + row[2])
                if metadata.st_size != expected_size:
                    raise ValueError("Source revision is stale: " + row[2])
                resolved = path.resolve(strict=True)
            except (OSError, RuntimeError) as error:
                raise ValueError("Source revision is stale or unreadable: " + row[2]) from error
            key = resolved, expected_size, row[4]
            data = cache.get(key)
            if data is None:
                if cached_bytes + expected_size > self.limits.max_cached_source_bytes:
                    raise ValueError("Source revisions exceed the configured cache byte limit")
                try:
                    with resolved.open("rb") as stream:
                        data = stream.read(expected_size + 1)
                except OSError as error:
                    raise ValueError("Source revision is stale or unreadable: " + row[2]) from error
                if len(data) != expected_size or hashlib.sha256(data).hexdigest() != row[4]:
                    raise ValueError("Source revision is stale: " + row[2])
                cache[key] = data
                cached_bytes += expected_size
            marker = SOURCE_BOMS.get(row[5])
            if marker is not None and not data.startswith(marker):
                raise ValueError("Source revision disagrees with its advertised encoding: " + row[2])
            if row[5] == "unmarked-bytes" and any(data.startswith(item) for item in SOURCE_BOMS.values()):
                raise ValueError("Unmarked source revision contains a Unicode byte-order mark: " + row[2])
            contents[identity] = data
        return contents

    def validate_source_revisions(self) -> None:
        """Require current files to match captured content before applying edits.

        A historical model remains readable after its input changes. This
        explicit freshness check rejects using it as current source evidence.
        Streams provide no seekable revision and cannot pass the editing gate.
        """
        self.verified_source_contents()

    def _validate_physical_location_rows(self, contents: dict[int, bytes]) -> None:
        """Check offsets once per verified byte revision and source encoding."""
        expected: dict[int, dict[int, tuple[int, int]]] = defaultdict(dict)
        revisions: dict[tuple[str, str, str], int] = {}
        for row in self.records["LOC"]:
            if row[11] != "mapped":
                continue
            fileid = int(self.source_contexts[int(row[4])][3])
            file_row = self.files[fileid]
            fileid = revisions.setdefault((file_row[3], file_row[4], file_row[5]), fileid)
            for line_column, byte_column in ((5, 9), (7, 10)):
                offset = int(row[byte_column])
                coordinate = int(row[line_column]), int(row[line_column + 1])
                previous = expected[fileid].get(offset)
                if previous is not None and previous != coordinate:
                    raise ValueError("Physical byte offset has conflicting UTF-16 coordinates")
                expected[fileid][offset] = coordinate

        for fileid, offsets in expected.items():
            encoding = self.files[fileid][5]
            if encoding not in SOURCE_CODECS:
                raise ValueError("Physical coordinate decoder is unavailable")
            codec, bom = SOURCE_CODECS[encoding]
            data = contents[fileid]
            # The compiler checks complete physical lines independently. An
            # invalid sequence on an unqueried line must not invalidate later
            # mapped lines. Find line endings in aligned encoded units before
            # asking the strict decoder to check a requested line.
            decoder = codecs.getincrementaldecoder(codec)(errors="strict")
            carriage_return, line_feed = "\r".encode(codec), "\n".encode(codec)
            unit_bytes = len(carriage_return)
            endings = re.compile(re.escape(carriage_return + line_feed) + b"|"
                                 + re.escape(carriage_return) + b"|" + re.escape(line_feed))
            requested = sorted(offsets)
            if requested[0] < bom:
                raise ValueError("Physical byte offset exceeds the source revision")
            pending = 0
            cursor = bom
            line = 1
            while pending < len(requested):
                ending = endings.search(data, cursor)
                while ending is not None and (ending.start() - bom) % unit_bytes:
                    ending = endings.search(data, ending.start() + 1)
                end = ending.start() if ending is not None else len(data)
                if requested[pending] <= end:
                    if end - cursor > MAX_COORDINATE_LINE_BYTES:
                        raise ValueError("Physical coordinate line exceeds the producer byte limit")
                    decoder.reset()
                    try:
                        decoder.decode(data[cursor:end], final=True)
                    except UnicodeDecodeError as error:
                        raise ValueError("Physical coordinate lies on a malformed encoded line") from error
                    decoder.reset()
                    column = 0
                    decoded_to = cursor
                    while pending < len(requested) and requested[pending] <= end:
                        offset = requested[pending]
                        try:
                            decoded = decoder.decode(data[decoded_to:offset], final=False)
                        except UnicodeDecodeError as error:
                            raise ValueError("Physical byte offset splits an encoded character") from error
                        if decoder.getstate()[0]:
                            raise ValueError("Physical byte offset splits an encoded character")
                        column += sum(2 if ord(character) > 0xFFFF else 1 for character in decoded)
                        if (line, column) != offsets[offset]:
                            raise ValueError("Physical bytes disagree with their UTF-16 coordinates")
                        decoded_to = offset
                        pending += 1
                if pending == len(requested):
                    break
                if ending is None or requested[pending] < ending.end():
                    raise ValueError("Physical byte offset lies inside a line ending or beyond the source")
                cursor = ending.end()
                line += 1

    def validate_physical_locations(self) -> None:
        """Verify bytes and UTF-16 coordinates independently before projection."""
        contents = self.verified_source_contents()
        self._validate_physical_location_rows(contents)

    def plan_edits(self, edits: Iterable[SourceEdit]) -> list[PlannedSource]:
        """Validate exact LOC replacements and return updated bytes without writing.

        Callers must compare ``expected_sha256`` immediately before an atomic
        replacement. This separation prevents a planner from silently writing
        through a stale model or a source path changed after planning.
        """
        if self.footer[0] != "END":
            raise ValueError("Source edits require a complete semantic model")
        requested: list[SourceEdit] = []
        for edit in edits:
            if len(requested) >= self.limits.max_edits:
                raise ValueError("Edit request exceeds the configured edit limit")
            if not isinstance(edit, SourceEdit):
                raise TypeError("Every edit must be a SourceEdit")
            if (not edit.domain or not edit.role or isinstance(edit.identity, bool)
                    or not isinstance(edit.identity, int) or edit.identity <= 0):
                raise ValueError("Edit location identity is invalid")
            if not isinstance(edit.replacement, bytes):
                raise TypeError("Edit replacement must be bytes")
            requested.append(edit)
        if not requested:
            return []

        contents = self.verified_source_contents()
        self._validate_physical_location_rows(contents)
        grouped: dict[Path, tuple[str, str, bytes, list[tuple[int, int, bytes]]]] = {}
        declarations = {int(row[1]): row for row in self.records["DCL"]}
        for edit in requested:
            location = self.physical_locations.get((edit.domain, edit.identity, edit.role))
            if location is None:
                raise ValueError("Edit has no exact physical location attachment")
            if location[11] != "mapped":
                raise ValueError("Edit location is not mapped to verified source bytes")
            if edit.domain == "binding":
                eligible = (edit.identity <= len(self.records["B"])
                            and self.records["B"][edit.identity - 1][3] == "1")
            elif edit.domain == "expression":
                eligible = (edit.identity <= len(self.records["E"])
                            and self.records["E"][edit.identity - 1][2] == "1")
            elif edit.domain == "declaration":
                eligible = (edit.identity in declarations and declarations[edit.identity][5] == "1")
            else:
                eligible = False
            if not eligible:
                raise ValueError("Edit subject is not an eligible written source fact")
            sourceid = int(location[4])
            if self.source_endings.get(sourceid) != "verified":
                raise ValueError("Edit source occurrence was not verified by the compiler")
            fileid = int(self.source_contexts[sourceid][3])
            file_row = self.files[fileid]
            encoding = file_row[5]
            if encoding not in SOURCE_CODECS:
                raise ValueError("Edit source encoding is unavailable")
            try:
                edit.replacement.decode(SOURCE_CODECS[encoding][0])
            except UnicodeDecodeError as error:
                raise ValueError("Edit replacement is invalid for the source encoding") from error
            start, end = int(location[9]), int(location[10])
            if start == end:
                raise ValueError("Edit location is empty")
            try:
                path = Path(file_row[2]).resolve(strict=True)
            except (OSError, RuntimeError) as error:
                raise ValueError("Edit source is unreadable: " + file_row[2]) from error
            group = grouped.get(path)
            if group is None:
                group = file_row[4], encoding, contents[fileid], []
                grouped[path] = group
            elif group[:3] != (file_row[4], encoding, contents[fileid]):
                raise ValueError("Edit path has conflicting source revisions")
            group[3].append((start, end, edit.replacement))

        planned: list[PlannedSource] = []
        total_output_bytes = 0
        for path, (digest, _encoding, original, replacements) in sorted(
                grouped.items(), key=lambda item: str(item[0]).casefold()):
            replacements.sort(key=lambda item: (item[0], item[1]))
            previous_end = -1
            projected_size = len(original)
            for start, end, replacement in replacements:
                if start < previous_end:
                    raise ValueError("Edit locations overlap")
                if start < 0 or end > len(original) or end <= start:
                    raise ValueError("Edit location exceeds the source revision")
                projected_size += len(replacement) - (end - start)
                previous_end = end
            total_output_bytes += projected_size
            if total_output_bytes > self.limits.max_planned_output_bytes:
                raise ValueError("Planned source output exceeds the configured byte limit")
            updated = bytearray(original)
            for start, end, replacement in reversed(replacements):
                updated[start:end] = replacement
            updated_bytes = bytes(updated)
            planned.append(PlannedSource(path=path,
                                         expected_sha256=digest,
                                         updated_sha256=hashlib.sha256(updated_bytes).hexdigest(),
                                         original=original,
                                         updated=updated_bytes))
        return planned

    def named(self, name: str, kind: str | None = None) -> list[int]:
        return [identity for identity, row in self.types.items()
                if row[2].casefold() == name.casefold() and (kind is None or row[3] == kind)]

    def symbol_name(self, identity: str | int) -> str:
        value = int(identity)
        return self.types[value][2] if value in self.types else self.symbols[value][2]

    def relations(self, kind: str) -> list[list[str]]:
        return [row for row in self.records["H"] if row[5] == kind]


# end of sidecar.py
