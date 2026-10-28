"""Project: FreeBASIC semantic sidecar
File: semantic_backend_abi.py
Purpose: Validate ABI components actually selected during backend formatting.
Responsibilities: Signature identity, component order, split values and closure.
This file intentionally does NOT infer machine registers from C or LLVM types.
"""

from collections import defaultdict
import re

FEATURE = "selected-backend-abi"
LIMIT = 1000000
TYPE_BYTES = 65536
PREFIXES = ("emitted-abi-signature:", "emitted-abi-parameter:", "emitted-abi-end:", "emitted-abi-counts:")
KINDS = frozenset(("value", "argv", "libc-callback", "libc-adjusted", "hidden-result", "variadic",
                   "i386-int128-low", "i386-int128-high", "memory-byval", "va-list-decay"))


def validate_backend_abi(model, number, subject_modules):
    signatures, components, ends = defaultdict(dict), defaultdict(dict), defaultdict(dict)
    frame_components = defaultdict(list)
    closures = {}
    formal_owners = defaultdict(set)
    procedure_formals = defaultdict(set)
    for row in model.records["G"]:
        formal_owners[int(row[2])].add(int(row[1]))
        procedure_formals[int(row[1])].add(int(row[2]))
    module = 0

    def integer(value, minimum=0, maximum=4294967295):
        result = number(value, minimum)
        if str(result) != value or result > maximum:
            raise ValueError("Selected ABI integer is not canonical or bounded")
        return result

    def symbol(value, raw_class=None, nullable=False):
        identity = integer(value)
        if identity == 0 and nullable:
            return identity
        native = model.symbols.get(identity)
        if (native is None or subject_modules.get(("symbol", identity)) != module
                or raw_class is not None and native[3] != raw_class):
            raise ValueError("Selected ABI has a foreign symbol or inappropriate class")
        return identity

    def spelling(value, empty=False):
        size = len(value.encode("utf-8", errors="surrogateescape"))
        if not empty and not size or size > TYPE_BYTES or "\0" in value:
            raise ValueError("Selected ABI spelling is empty, excessive or contains NUL")

    for row in model.rows:
        if row[0] == "M":
            module += 1
            continue
        if row[0] != "K" or not row[3].startswith(PREFIXES):
            continue
        if model.capabilities[module].get(FEATURE) != "available":
            raise ValueError("Selected backend ABI has unavailable coverage")
        owner = symbol(row[2], "8")
        if row[1] != "symbol" or model.symbols[owner][11] != "0":
            raise ValueError("Selected ABI lacks its module namespace owner")
        prefix, suffix = row[3].split(":")
        identity = integer(suffix, 1, LIMIT)
        fields = row[4].split("\t")
        if prefix == "emitted-abi-counts":
            if identity != module or module in closures or len(fields) != 2:
                raise ValueError("Selected ABI closure is repeated or foreign")
            closures[module] = [integer(value, 0, LIMIT) for value in fields]
            continue
        if module in closures:
            raise ValueError("Selected ABI observation follows module closure")
        if prefix == "emitted-abi-signature":
            if len(fields) != 5 or identity != len(signatures[module]) + 1:
                raise ValueError("Selected ABI signature is malformed or reordered")
            proc = symbol(fields[0], "3")
            stage, role, result, convention = fields[1:]
            expected_stage = "llvm" if model.contexts[module - 1][5] == "llvm" else "c"
            if stage != expected_stage or role not in ("declaration", "definition", "pointer-type"):
                raise ValueError("Selected ABI signature has another backend stage or role")
            spelling(result)
            spelling(convention, empty=True)
            if stage == "llvm" and role == "pointer-type" and convention:
                raise ValueError("LLVM function type invents an attached convention")
            if role == "definition" and model.signatures.get(proc, [None, None, None])[2] == "procedure-pointer":
                raise ValueError("Procedure pointer type invents an emitted definition")
            signatures[module][identity] = tuple(fields)
        elif prefix == "emitted-abi-parameter":
            if len(fields) != 10 or identity != len(components[module]) + 1:
                raise ValueError("Selected ABI component is malformed or reordered")
            frame = integer(fields[0], 1, LIMIT)
            signature = signatures[module].get(frame)
            if signature is None or frame in ends[module]:
                raise ValueError("Selected ABI component has no open signature")
            proc = int(signature[0])
            param = symbol(fields[1], "4", nullable=True)
            if param and formal_owners[param] and proc not in formal_owners[param]:
                raise ValueError("Selected ABI component belongs to another procedure")
            position = integer(fields[2], 0, LIMIT)
            part, parts = integer(fields[3], 0, 1), integer(fields[4], 0, 2)
            kind, text, dtype, nominal, attributes = fields[5:]
            selected = frame_components[module, frame]
            if position != len(selected) or kind not in KINDS:
                raise ValueError("Selected ABI component order or kind is invalid")
            spelling(text)
            spelling(attributes, empty=True)
            if bool(dtype) != bool(nominal):
                raise ValueError("Selected ABI component has a partial native type")
            if dtype:
                code = integer(dtype)
                if code & 31 not in model.primitives[module - 1] or code == 2147483648:
                    raise ValueError("Selected ABI has an invalid native datatype")
                symbol(nominal, nullable=True)
            if kind == "hidden-result":
                if param or position or part or parts != 1 or not dtype:
                    raise ValueError("Hidden result invents a source formal or another component")
            elif not param:
                raise ValueError("Selected parameter component lacks its actual formal")
            if kind == "variadic":
                if part or parts or text != "..." or dtype or attributes:
                    raise ValueError("Variadic marker invents a fixed ABI component")
            elif kind.startswith("i386-int128-"):
                if (signature[1] != "llvm" or parts != 2 or text != "i64" or attributes
                        or part != (kind == "i386-int128-high") or not dtype or nominal != "0"):
                    raise ValueError("Split integer128 has another selected word component")
                primitive = model.primitives[module - 1][int(dtype) & 31]
                if primitive[2] != "ulongint":
                    raise ValueError("Split integer128 word has another native type")
            elif parts != 1 or part:
                raise ValueError("Ordinary selected ABI value invents split components")
            if kind in ("argv", "libc-callback", "libc-adjusted") and signature[1] != "c":
                raise ValueError("C parameter adaptation appears in another backend")
            if kind == "argv" and (text != "char**" or dtype or attributes):
                raise ValueError("C argv adaptation has another selected type")
            if kind == "libc-callback" and (dtype or attributes):
                raise ValueError("C callback override invents a BASIC ABI type")
            if kind == "memory-byval":
                if signature[1] != "llvm" or not dtype or not text.endswith("*"):
                    raise ValueError("LLVM byval memory argument lacks its selected pointer")
                if signature[2] == "pointer-type":
                    if attributes:
                        raise ValueError("LLVM function type invents parameter attributes")
                elif re.fullmatch(r" byval\(.+\) align ([1-9][0-9]*)", attributes) is None:
                    raise ValueError("LLVM memory argument loses its byval attributes")
            if kind == "va-list-decay" and (signature[1] != "llvm" or not dtype or not text.endswith("*")):
                raise ValueError("va_list decay lacks its selected LLVM pointer")
            if kind in ("value", "libc-adjusted") and not dtype:
                raise ValueError("Selected ordinary ABI value lacks its native type")
            if attributes and kind not in ("memory-byval", "hidden-result"):
                raise ValueError("Selected ABI component invents unsupported attributes")
            components[module][identity] = tuple(fields)
            selected.append(tuple(fields))
        else:
            signature = signatures[module].get(identity)
            if len(fields) != 2 or signature is None or identity in ends[module]:
                raise ValueError("Selected ABI signature closes another observation")
            count = integer(fields[0], 0, LIMIT)
            selected = frame_components[module, identity]
            if count != len(selected):
                raise ValueError("Selected ABI signature has incomplete components")
            spelling(fields[1])
            if signature[3] not in fields[1] or signature[4] and signature[4] not in fields[1]:
                raise ValueError("Selected ABI header contradicts its result or convention")
            groups = defaultdict(list)
            for component in selected:
                if component[1] != "0":
                    groups[int(component[1])].append(component)
            for values in groups.values():
                if values[0][4] == "2":
                    if (len(values) != 2 or values[0][5] != "i386-int128-low" or values[1][5] != "i386-int128-high"
                            or int(values[1][2]) != int(values[0][2]) + 1):
                        raise ValueError("Selected integer128 word pair is incomplete")
                elif len(values) != 1:
                    raise ValueError("Selected ABI repeats a source formal")
            proc = int(signature[0])
            known = procedure_formals[proc]
            if known and set(groups) != known:
                raise ValueError("Selected ABI signature omits an actual formal")
            ends[module][identity] = tuple(fields)

    model.selected_backend_abi = {}
    for module, capabilities in model.capabilities.items():
        if capabilities.get(FEATURE) != "available":
            continue
        counts = len(signatures[module]), len(components[module])
        if (module not in closures or list(counts) != closures[module]
                or set(signatures[module]) != set(ends[module])):
            raise ValueError("Selected backend ABI module closure is incomplete")
        model.selected_backend_abi[module] = {"signature": signatures[module], "parameter": components[module], "end": ends[module]}

# end of semantic_backend_abi.py
