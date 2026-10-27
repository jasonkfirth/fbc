"""Project: FreeBASIC semantic sidecar
File: semantic_call_abi.py
Purpose: Validate actual C/LLVM call components and carried source identities.
Responsibilities: Types, formal correspondence, emitted order and complete closure.
This file intentionally does NOT infer source arguments or machine locations.
"""

from collections import defaultdict
import re

FEATURE = "selected-call-abi"
BUILTIN_FEATURE = "llvm-builtin-call-lowering"
LIMIT, TEXT_BYTES = 1000000, 65536
# Native and stable-v1 reserve bits 5..8 for pointer depth.
POINTER_MASK = 0x1e0
PREFIXES = ("emitted-abi-call:", "emitted-abi-argument:", "emitted-abi-call-end:", "emitted-abi-call-counts:",
            "emitted-abi-builtin:", "emitted-abi-builtin-count:")
KINDS = frozenset(("value", "variadic-value", "hidden-result", "unmapped-value", "i386-int128-low",
                   "i386-int128-high", "memory-byval", "va-list-decay", "array-callback", "libc-adjusted"))


def validate_call_abi(model, number, subject_modules):
    calls, arguments, ends = defaultdict(dict), defaultdict(dict), defaultdict(dict)
    selected = defaultdict(list)
    formal_owners, formals = defaultdict(set), defaultdict(dict)
    closures = {}
    builtins, builtin_closures = defaultdict(dict), {}
    active = 0
    module = 0
    for row in model.records["G"]:
        formal_owners[int(row[2])].add(int(row[1]))
        formals[int(row[1])][int(row[2])] = row[4]

    def integer(value, minimum=0, maximum=4294967295):
        result = number(value, minimum)
        if str(result) != value or result > maximum:
            raise ValueError("Selected call ABI integer is not canonical or bounded")
        return result

    def symbol(value, raw_class=None, nullable=False):
        identity = integer(value)
        if identity == 0 and nullable:
            return identity
        info = model.symbols.get(identity)
        if (info is None or subject_modules.get(("symbol", identity)) != module
                or raw_class is not None and info[3] != raw_class):
            raise ValueError("Selected call ABI has a foreign symbol or another class")
        return identity

    def native_type(dtype, nominal, optional=False):
        if not dtype and not nominal and optional:
            return None
        code = integer(dtype)
        symbol(nominal, nullable=True)
        if code == 2147483648 or code & 31 not in model.primitives[module - 1]:
            raise ValueError("Selected call ABI has an invalid native datatype")
        return code

    def text(value, empty=False):
        size = len(value.encode("utf-8", errors="surrogateescape"))
        if (not empty and not size) or size > TEXT_BYTES or "\0" in value:
            raise ValueError("Selected call ABI text is empty, excessive or contains NUL")

    def context(caller, expression, statement, source, target):
        symbol(caller, "3", nullable=True)
        expression, statement, source = (integer(value) for value in (expression, statement, source))
        if expression and subject_modules.get(("expression", expression)) != module:
            raise ValueError("Selected call ABI carries a foreign source expression")
        if statement and subject_modules.get(("statement", statement)) != module:
            raise ValueError("Selected call ABI carries a foreign source statement")
        if source and (source not in model.source_contexts or int(model.source_contexts[source][4]) != module):
            raise ValueError("Selected call ABI carries a foreign source context")
        return symbol(target, "3", nullable=True)

    for row in model.rows:
        if row[0] == "M":
            if active:
                raise ValueError("Selected call ABI crosses a module boundary")
            module += 1
            continue
        if row[0] != "K" or not row[3].startswith(PREFIXES):
            continue
        if model.capabilities[module].get(FEATURE) != "available":
            raise ValueError("Selected call ABI has unavailable coverage")
        owner = symbol(row[2], "8")
        if row[1] != "symbol" or model.symbols[owner][11] != "0":
            raise ValueError("Selected call ABI lacks its module namespace owner")
        prefix, suffix = row[3].split(":")
        identity = integer(suffix, 1, LIMIT)
        fields = row[4].split("\t")
        if prefix.startswith("emitted-abi-builtin"):
            if (model.capabilities[module].get(BUILTIN_FEATURE) != "available"
                    or model.contexts[module - 1][5] != "llvm" or active):
                raise ValueError("LLVM builtin route has unavailable coverage or overlaps a formatter")
            if prefix == "emitted-abi-builtin-count":
                if identity != module or module in builtin_closures or len(fields) != 1:
                    raise ValueError("LLVM builtin route closure is repeated or foreign")
                builtin_closures[module] = integer(fields[0], 0, LIMIT)
                continue
            if (module in closures or module in builtin_closures or len(fields) != 8
                    or identity != len(builtins[module]) + 1 or fields[1] != "llvm-builtin-lowering"):
                raise ValueError("LLVM builtin route is malformed, closed or reordered")
            symbol(fields[0], "3")
            if context(*fields[2:7]):
                raise ValueError("LLVM direct builtin route invents a virtual target")
            integer(fields[7], 0, LIMIT)
            builtins[module][identity] = tuple(fields)
            continue
        if prefix == "emitted-abi-call-counts":
            if identity != module or module in closures or len(fields) != 2 or active:
                raise ValueError("Selected call ABI module closure is repeated, foreign or open")
            closures[module] = [integer(value, 0, LIMIT) for value in fields]
            continue
        if module in closures:
            raise ValueError("Selected call ABI observation follows module closure")
        if prefix == "emitted-abi-call":
            if len(fields) != 15 or active or identity != len(calls[module]) + 1:
                raise ValueError("Selected call ABI is malformed, nested or reordered")
            proc = symbol(fields[0], "3")
            stage, dispatch = fields[1:3]
            backend = model.contexts[module - 1][5]
            if backend not in ("gcc", "clang", "llvm") or stage != ("llvm" if backend == "llvm" else "c"):
                raise ValueError("Selected call ABI has another backend stage")
            if dispatch not in ("direct", "indirect"):
                raise ValueError("Selected call ABI has an unknown dispatch kind")
            signature = model.signatures.get(proc)
            if signature is None or (dispatch == "indirect") != (signature[2] == "procedure-pointer"):
                raise ValueError("Selected call dispatch confuses a procedure type with a resolved target")
            target = context(*fields[3:8])
            if target and dispatch != "indirect":
                raise ValueError("Direct call invents a virtual static target")
            if target and (target not in model.signatures or int(model.signatures[target][12]) <= 0):
                raise ValueError("Selected virtual call invents a nonvirtual static target")
            integer(fields[8], 0, 1)
            native_type(*fields[9:11], optional=stage == "c")
            text(fields[11], empty=stage == "c")
            text(fields[12], empty=True)
            if stage == "c" and (fields[11] or fields[12]):
                raise ValueError("C call invents explicit result or convention tokens")
            integer(fields[13], 0, LIMIT)
            integer(fields[14], 0, 9223372036854775807)
            calls[module][identity] = tuple(fields)
            active = identity
        elif prefix == "emitted-abi-argument":
            if len(fields) != 13 or identity != len(arguments[module]) + 1:
                raise ValueError("Selected call ABI argument is malformed or reordered")
            frame = integer(fields[0], 1, LIMIT)
            if frame != active or not active:
                raise ValueError("Selected call ABI argument has no active call")
            call = calls[module][frame]
            param = symbol(fields[1], "4", nullable=True)
            if param and formal_owners[param] and int(call[0]) not in formal_owners[param]:
                raise ValueError("Selected call ABI argument belongs to another procedure")
            components = selected[module, frame]
            position = integer(fields[2], 0, LIMIT)
            part, parts = integer(fields[3], 0, 1), integer(fields[4], 1, 2)
            kind = fields[5]
            if position != len(components) or kind not in KINDS:
                raise ValueError("Selected call ABI component order or kind is invalid")
            native_type(*fields[6:8])
            dtype = native_type(*fields[8:10], optional=kind == "array-callback")
            spelling, attributes, value = fields[10:13]
            text(spelling, empty=call[1] == "c")
            text(attributes, empty=True)
            text(value)
            if call[1] == "c" and spelling:
                raise ValueError("C argument invents an explicit LLVM type token")
            if kind in ("hidden-result", "unmapped-value"):
                if param or part or parts != 1 or kind == "hidden-result" and position:
                    raise ValueError("Hidden/unmapped argument invents a source formal or split")
                if kind == "hidden-result" and not dtype & POINTER_MASK:
                    raise ValueError("Hidden result argument loses its selected pointer")
            elif not param:
                raise ValueError("Selected ordinary call argument lacks its formal")
            mode = formals[int(call[0])].get(param)
            if param and mode and (kind == "variadic-value") != (mode == "vararg"):
                raise ValueError("Selected variadic value contradicts its actual formal mode")
            if kind.startswith("i386-int128-"):
                if (call[1] != "llvm" or parts != 2 or part != (kind == "i386-int128-high")
                        or spelling != "i64" or attributes or fields[9] != "0"
                        or model.primitives[module - 1][dtype & 31][2] != "ulongint"):
                    raise ValueError("Selected call integer128 word has another type or part")
            elif part or parts != 1:
                raise ValueError("Selected ordinary call argument invents split components")
            if kind in ("memory-byval", "va-list-decay"):
                if call[1] != "llvm" or not spelling.endswith("*") or not dtype & POINTER_MASK:
                    raise ValueError("LLVM call memory/va_list argument loses its selected pointer")
            if kind == "memory-byval" and re.fullmatch(r" byval\(.+\) align ([1-9][0-9]*)", attributes) is None:
                raise ValueError("LLVM call memory argument loses its actual byval attributes")
            if kind in ("array-callback", "libc-adjusted") and call[1] != "c":
                raise ValueError("C call adaptation appears in another backend")
            if kind == "array-callback" and dtype is not None:
                raise ValueError("C callback adaptation invents a BASIC selected type")
            if attributes and kind not in ("hidden-result", "memory-byval"):
                raise ValueError("Selected call argument invents unsupported attributes")
            arguments[module][identity] = tuple(fields)
            components.append(tuple(fields))
        else:
            if len(fields) != 2 or identity != active or not active:
                raise ValueError("Selected call ABI closes another observation")
            components = selected[module, identity]
            if integer(fields[0], 0, LIMIT) != len(components):
                raise ValueError("Selected call ABI component coverage is incomplete")
            text(fields[1])
            call = calls[module][identity]
            if call[11] and call[11] not in fields[1] or call[12] and call[12] not in fields[1]:
                raise ValueError("Selected call text contradicts its result or convention")
            groups = defaultdict(list)
            unmapped = False
            for component in components:
                token = (component[10] + component[11] + " " if call[1] == "llvm" else "") + component[12]
                if token not in fields[1]:
                    raise ValueError("Selected call component contradicts its emitted text")
                if component[1] != "0":
                    groups[int(component[1])].append(component)
                unmapped |= component[5] == "unmapped-value"
            for param, values in groups.items():
                if values[0][4] == "2":
                    if (len(values) != 2 or values[0][5] != "i386-int128-low" or values[1][5] != "i386-int128-high"
                            or int(values[1][2]) != int(values[0][2]) + 1 or values[0][6:8] != values[1][6:8]):
                        raise ValueError("Selected call integer128 word pair is incomplete")
                elif len(values) != 1 and formals[int(call[0])].get(param) != "vararg":
                    raise ValueError("Selected call ABI repeats a fixed formal")
            known = formals[int(call[0])]
            required = {param for param, mode in known.items() if mode != "vararg"}
            if not unmapped and not required <= set(groups):
                raise ValueError("Selected call ABI omits a fixed formal")
            ends[module][identity] = tuple(fields)
            active = 0

    if active:
        raise ValueError("Selected call ABI formatter remains open")
    model.selected_call_abi = {}
    for module, capabilities in model.capabilities.items():
        if capabilities.get(BUILTIN_FEATURE) == "available" and (capabilities.get(FEATURE) != "available"
                or model.contexts[module - 1][5] != "llvm"):
            raise ValueError("LLVM builtin route invents another backend or unavailable call coverage")
        if capabilities.get(FEATURE) != "available":
            continue
        counts = len(calls[module]), len(arguments[module])
        if (module not in closures or list(counts) != closures[module]
                or set(calls[module]) != set(ends[module])):
            raise ValueError("Selected call ABI module closure is incomplete")
        if capabilities.get(BUILTIN_FEATURE) == "available" and (module not in builtin_closures or builtin_closures[module] != len(builtins[module])):
            raise ValueError("LLVM builtin call route coverage is incomplete")
        model.selected_call_abi[module] = {"call": calls[module], "argument": arguments[module], "end": ends[module],
                                          "builtin": builtins[module]}

# end of semantic_call_abi.py
