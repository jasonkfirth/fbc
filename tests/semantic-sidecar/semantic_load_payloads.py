"""Project: FreeBASIC semantic sidecar
File: semantic_load_payloads.py
Purpose: Validate active native payloads captured immediately before AST loading.
Responsibilities: Type encoding, union/class contracts and call/table/token closure.
This file intentionally does NOT reconstruct payloads from earlier snapshots.
"""

from collections import defaultdict
import re

FEATURE = "native-ast-load-payloads"
ASSEMBLY_FEATURE = "native-asm-load-emission"
PREFIXES = ("native-load-shape:", "native-load-payload:", "native-load-argument:",
            "native-load-jump:", "native-load-assembly:", "native-load-payload-counts:")
LIMIT = 1000000
KINDS = {"constant": ("constant", 3), "load": ("result-load", 1), "assignment": ("assignment", 1),
         "binary-operator": ("operator", 4), "unary-operator": ("operator", 4), "branch": ("operator", 4),
         "address-of": ("address-of", 1), "call": ("call", 5), "conversion": ("conversion", 3),
         "variable": ("offset", 1), "dereference": ("offset", 1), "offset": ("offset", 1),
         "index": ("index", 2), "label": ("label", 1), "conditional": ("conditional", 1),
         "sequence": ("sequence", 1), "memory": ("memory", 4), "stack": ("stack", 2),
         "runtime-macro": ("runtime-macro", 2), "loop": ("loop", 2), "jump-table": ("jump-table", 4),
         "assembly": ("assembly", 2), "literal-text": ("text", 1), "debug": ("debug", 3)}


def validate_load_payloads(model, number, subject_modules, node_kinds):
    modules = {}
    module = 0

    def integer(value, minimum=0, maximum=4294967295):
        result = number(value, minimum)
        if str(result) != value or result > maximum:
            raise ValueError("Load payload scalar is not canonical or bounded")
        return result

    def signed(value):
        result = number(value, -9223372036854775808)
        if str(result) != value or result > 9223372036854775807:
            raise ValueError("Load payload integer exceeds native signed storage")
        return result

    def symbol(value, classes=None, nullable=True):
        identity = integer(value)
        if identity == 0 and nullable:
            return identity
        native = model.symbols.get(identity)
        if (native is None or subject_modules.get(("symbol", identity)) != module
                or classes is not None and native[3] not in classes):
            raise ValueError("Load payload symbol has another class or module")
        return identity

    def dtype(value):
        raw = integer(value)
        if raw == 2147483648:
            return raw
        if raw > 33554431 or (raw & 31) not in model.primitives[module - 1]:
            raise ValueError("Load payload datatype has unknown bits or a missing classifier")
        modifier = (raw >> 20) & 31
        if modifier and modifier not in model.primitives[module - 1]:
            raise ValueError("Load payload has another mangling classifier")
        return raw

    for row in model.rows:
        if row[0] == "M":
            module += 1
            continue
        if row[0] != "K" or not row[3].startswith(PREFIXES):
            continue
        if model.capabilities.get(module, {}).get(FEATURE) != "available":
            raise ValueError("Load payload lacks its capability")
        namespace = symbol(row[2], ("8",), nullable=False)
        if row[1] != "symbol" or model.symbols[namespace][11] != "0":
            raise ValueError("Load payload lacks its module namespace")
        state = modules.setdefault(module, {"shapes": {}, "payloads": {}, "arguments": defaultdict(dict),
                                            "jumps": defaultdict(dict), "assembly": defaultdict(dict), "closed": False})
        if state["closed"]:
            raise ValueError("Load payload follows module closure")
        pieces = row[3].split(":")
        if len(pieces) not in (2, 3):
            raise ValueError("Malformed load payload key")
        family, identity = pieces[0], integer(pieces[1], 1, LIMIT)
        fields = tuple(row[4].split("\t"))
        if family == "native-load-payload-counts":
            if len(pieces) != 2 or identity != module or len(fields) != 5:
                raise ValueError("Malformed load payload count closure")
            expected = (len(state["shapes"]), len(state["payloads"]), sum(map(len, state["arguments"].values())),
                        sum(map(len, state["jumps"].values())), sum(map(len, state["assembly"].values())))
            if tuple(integer(value, 0, LIMIT) for value in fields) != expected:
                raise ValueError("Load payload count closure is incomplete")
            state["closed"] = True
            continue
        if identity not in model.native_loads.get(module, {}).get("enters", {}):
            raise ValueError("Load payload has no native load interval")
        actual_kind = node_kinds[int(model.native_loads[module]["enters"][identity][4])]
        if family in ("native-load-shape", "native-load-payload"):
            if len(pieces) != 2:
                raise ValueError("Scalar load payload has an unexpected ordinal")
            key = "shapes" if family == "native-load-shape" else "payloads"
            if identity in state[key]:
                raise ValueError("Repeated native load shape or payload")
            if key == "shapes":
                if len(fields) != 6:
                    raise ValueError("Malformed native load shape")
                dtype(fields[0])
                symbol(fields[1])
                symbol(fields[2])
                integer(fields[3], 0, 128)
                if fields[4] not in ("0", "1") or fields[5] not in ("0", "1"):
                    raise ValueError("Native load child flags are not boolean")
            else:
                expected_kind, count = KINDS.get(actual_kind, ("none", 1))
                if len(fields) != count + 1 or fields[0] != expected_kind:
                    raise ValueError("Native load payload reads another class union")
                values = fields[1:]
                if expected_kind in ("assignment", "address-of", "result-load", "label", "stack", "runtime-macro", "operator", "loop", "memory"):
                    integer(values[0])
                if expected_kind in ("result-load", "label") and values[0] not in ("0", "1"):
                    raise ValueError("Native load payload flag is not boolean")
                if expected_kind == "assignment" and integer(values[0]) > 255:
                    raise ValueError("Native assignment has unknown option bits")
                if expected_kind == "operator":
                    if integer(values[2]) > 255:
                        raise ValueError("Native operator has unknown option bits")
                    symbol(values[3])
                elif expected_kind == "call":
                    integer(values[0], 0, LIMIT)
                    if values[1] not in ("0", "1") or values[4] not in ("0", "1"):
                        raise ValueError("Native call flags are not boolean")
                    symbol(values[2], ("1",))
                    symbol(values[3], ("3",))
                elif expected_kind == "conversion":
                    if any(value not in ("0", "1") for value in values):
                        raise ValueError("Native conversion flags are not boolean")
                elif expected_kind == "offset":
                    signed(values[0])
                elif expected_kind == "index":
                    signed(values[0]); signed(values[1])
                elif expected_kind == "conditional":
                    symbol(values[0], ("7",), nullable=False)
                elif expected_kind == "sequence":
                    if values[0] not in ("left", "right", "none"):
                        raise ValueError("Native sequence has another result edge")
                elif expected_kind == "memory":
                    signed(values[2]); signed(values[3])
                elif expected_kind == "loop":
                    symbol(values[1], ("7",), nullable=False)
                elif expected_kind == "jump-table":
                    integer(values[0], 0, LIMIT)
                    integer(values[1], 0, 18446744073709551615); integer(values[2], 0, 18446744073709551615)
                    symbol(values[3], ("7",), nullable=False)
                elif expected_kind == "assembly":
                    integer(values[0], 0, LIMIT)
                    if values[1] != "unknown-memory-registers-control":
                        raise ValueError("Native assembly lost its conservative effect contract")
                elif expected_kind == "debug":
                    signed(values[0]); signed(values[1])
                elif expected_kind == "constant":
                    kind, value, suffix = values
                    if suffix not in ("0", "1"):
                        raise ValueError("Native constant suffix flag is not boolean")
                    if kind == "signed": signed(value)
                    elif kind == "unsigned": integer(value, 0, 18446744073709551615)
                    elif kind == "float64-bits":
                        if re.fullmatch(r"0x[0-9A-F]{16}", value) is None: raise ValueError("Native floating constant lacks exact bits")
                    elif kind in ("bytes", "wide-units"):
                        if len(value) % (2 if kind == "bytes" else 8) or re.fullmatch(r"[0-9A-F]*", value) is None:
                            raise ValueError("Native literal lacks exact bytes or code units")
                    elif kind != "unavailable" or value:
                        raise ValueError("Unknown or invented native constant value")
                elif expected_kind == "none" and values != ("",):
                    raise ValueError("Payload-free loader acquired invented union data")
            state[key][identity] = fields
        else:
            if len(pieces) != 3:
                raise ValueError("Native load sequence lacks its ordinal")
            ordinal = integer(pieces[2], 0, LIMIT)
            key = {"native-load-argument": "arguments", "native-load-jump": "jumps", "native-load-assembly": "assembly"}[family]
            if actual_kind != {"arguments": "call", "jumps": "jump-table", "assembly": "assembly"}[key] or ordinal != len(state[key][identity]):
                raise ValueError("Native load sequence belongs to another class or order")
            if key == "arguments":
                if len(fields) != 7 or fields[1] not in ("byval", "byref", "bydesc", "default") or fields[6] not in ("0", "1"):
                    raise ValueError("Malformed native call argument")
                symbol(fields[0], ("4",))
                integer(fields[2], 0, 9223372036854775807)
                dtype(fields[3]); symbol(fields[4])
                origin = integer(fields[5])
                if origin and (origin not in model.nodes or subject_modules[("node", origin)] != module):
                    raise ValueError("Native call argument has a foreign snapshot origin")
            elif key == "jumps":
                if len(fields) != 2: raise ValueError("Malformed native jump table entry")
                integer(fields[0], 0, 18446744073709551615); symbol(fields[1], ("7",), nullable=False)
            else:
                if len(fields) != 3 or fields[0] not in ("text", "symbol"):
                    raise ValueError("Malformed native assembly token")
                if fields[0] == "text" and fields[1] != "0" or fields[0] == "symbol" and fields[2]:
                    raise ValueError("Native assembly token mixes text and symbol storage")
                symbol(fields[1], nullable=fields[0] == "text")
            state[key][identity][ordinal] = fields

    for module, capabilities in model.capabilities.items():
        if capabilities.get(FEATURE) != "available": continue
        state = modules.get(module)
        enters = model.native_loads.get(module, {}).get("enters", {})
        if state is None or not state["closed"] or set(state["shapes"]) != set(enters) or set(state["payloads"]) != set(enters):
            raise ValueError("Native load shape/payload coverage is incomplete")
        for identity, payload in state["payloads"].items():
            if payload[0] == "call":
                shape = state["shapes"][identity]
                symbol(shape[2], ("3",), nullable=False)
                if len(state["arguments"][identity]) != int(payload[1]) or shape[4] != payload[5] or shape[5] != str(int(payload[1] != "0")):
                    raise ValueError("Native call shape or argument count is incomplete")
            elif payload[0] in ("jump-table", "assembly"):
                key = "jumps" if payload[0] == "jump-table" else "assembly"
                if len(state[key][identity]) != int(payload[1]): raise ValueError("Native table/token count is incomplete")
    model.native_load_payloads = modules

def validate_load_assembly(model, number, subject_modules, node_kinds):
    modules = {}
    module = 0
    stack = []
    prefixes = ("native-load-assembly-output:", "native-load-assembly-output-token:", "native-load-assembly-output-counts:")

    def integer(value, minimum=0, maximum=4294967295):
        result = number(value, minimum)
        if str(result) != value or result > maximum:
            raise ValueError("Emitted assembly scalar is not canonical or bounded")
        return result

    for row in model.rows:
        if row[0] == "M":
            module += 1
            stack = []
            continue
        if row[0] != "K":
            continue
        if row[3].startswith("native-load-enter:"):
            stack.append(int(row[3].split(":")[1]))
            continue
        if row[3].startswith("native-load-end:"):
            if stack: stack.pop()
            continue
        if not row[3].startswith(prefixes):
            continue
        if model.capabilities.get(module, {}).get(ASSEMBLY_FEATURE) != "available":
            raise ValueError("Emitted assembly lacks its capability")
        owner = integer(row[2], 1)
        native = model.symbols.get(owner)
        if (row[1] != "symbol" or native is None or native[3] != "8" or native[11] != "0"
                or subject_modules[("symbol", owner)] != module):
            raise ValueError("Emitted assembly lacks its module namespace")
        state = modules.setdefault(module, {"groups": {}, "tokens": defaultdict(dict), "closed": False})
        if state["closed"]:
            raise ValueError("Emitted assembly follows module closure")
        pieces = row[3].split(":")
        if len(pieces) not in (2, 3):
            raise ValueError("Malformed emitted assembly key")
        kind, identity = pieces[0], integer(pieces[1], 1, LIMIT)
        fields = tuple(row[4].split("\t"))
        if kind == "native-load-assembly-output-counts":
            if identity != module or len(pieces) != 2 or len(fields) != 2:
                raise ValueError("Malformed emitted assembly count closure")
            if tuple(integer(value, 0, LIMIT) for value in fields) != (len(state["groups"]), sum(map(len, state["tokens"].values()))):
                raise ValueError("Emitted assembly count closure is incomplete")
            if set(state["tokens"]) != set(state["groups"]):
                raise ValueError("Emitted assembly tokens lack their native groups")
            state["closed"] = True
            continue
        if not stack:
            raise ValueError("Emitted assembly has no active loader")
        entry = model.native_loads[module]["enters"][stack[-1]]
        if entry[8] != "1" or node_kinds[int(entry[4])] != "assembly":
            raise ValueError("Emitted assembly belongs to another load class or emission state")
        if kind == "native-load-assembly-output":
            if len(pieces) != 2 or len(fields) != 2 or identity != len(state["groups"]) + 1:
                raise ValueError("Emitted assembly group order is incomplete")
            if integer(fields[0], 1, LIMIT) != stack[-1] or integer(fields[1], 1, LIMIT) != len(state["tokens"][identity]):
                raise ValueError("Emitted assembly group has another owner or token count")
            state["groups"][identity] = fields
        else:
            if len(pieces) != 3 or len(fields) != 3 or identity in state["groups"]:
                raise ValueError("Emitted assembly token follows its group or is malformed")
            ordinal = integer(pieces[2], 0, LIMIT)
            if ordinal != len(state["tokens"][identity]) or fields[0] not in ("text", "symbol"):
                raise ValueError("Emitted assembly token has another kind or order")
            symbol_id = integer(fields[1])
            if fields[0] == "text":
                if symbol_id: raise ValueError("Emitted assembly text acquired a symbol")
            elif (fields[2] or symbol_id not in model.symbols
                    or subject_modules[("symbol", symbol_id)] != module):
                raise ValueError("Emitted assembly symbol has text or a foreign identity")
            state["tokens"][identity][ordinal] = fields
    for module, capabilities in model.capabilities.items():
        if capabilities.get(ASSEMBLY_FEATURE) == "available" and (module not in modules or not modules[module]["closed"]):
            raise ValueError("Missing emitted assembly module closure")
    model.native_assembly_outputs = modules

# end of semantic_load_payloads.py
