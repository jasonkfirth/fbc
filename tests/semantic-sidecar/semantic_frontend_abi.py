"""Project: FreeBASIC semantic sidecar
File: semantic_frontend_abi.py
Purpose: Validate frontend result policy and live formal ABI snapshots.
Responsibilities: F/G companions, type transformations, markers and closure.
This file intentionally does NOT infer physical registers or final stack usage.
"""

from collections import Counter, defaultdict

FEATURE = "frontend-procedure-abi"
PROCEDURE = "frontend-procedure-abi:"
PARAMETER = "frontend-parameter-abi:"
COUNTS = "frontend-abi-counts:"
LIMIT = 1000000
INVALID_TYPE = 2147483648
# Both raw datatype ABIs retain base bits 0..4, pointer depth 5..8,
# const positions 9..17 and the mangling classifier in bits 20..24.
BASE_MASK, POINTER_MASK, CONST_MASK, MANGLE_MASK = 0x1f, 0x1e0, 0x3fe00, 0x1f00000
POINTER_STEP = 1 << 5


def validate_frontend_abi(model, number, subject_modules):
    observations = defaultdict(lambda: defaultdict(dict))
    expected = defaultdict(Counter)
    closures = {}
    latest_types = {}
    pending_procedure = pending_parameter = None
    module = 0

    def integer(value, minimum=0, maximum=4294967295):
        result = number(value, minimum)
        if str(result) != value or result > maximum:
            raise ValueError("Frontend ABI integer is not canonical or bounded")
        return result

    def symbol(value, raw_class=None, nullable=False):
        identity = integer(value)
        if identity == 0 and nullable:
            return identity
        row = model.symbols.get(identity)
        if (row is None or subject_modules.get(("symbol", identity)) != module
                or raw_class is not None and row[3] != raw_class):
            raise ValueError("Frontend ABI has another symbol owner or class")
        return identity

    def dtype(value):
        result = integer(value)
        if result != INVALID_TYPE and result & BASE_MASK not in model.primitives[module - 1]:
            raise ValueError("Frontend ABI has an unknown native datatype")
        return result

    def address_of(value):
        return ((value & BASE_MASK) | ((value & POINTER_MASK) + POINTER_STEP)
                | ((value & CONST_MASK) << 1) | (value & MANGLE_MASK))

    def pending_type(value, nominal):
        if value == INVALID_TYPE or value & POINTER_MASK:
            return False
        name = model.primitives[module - 1][value & BASE_MASK][2]
        return name == "fwdref" or name == "type" and (not nominal or model.symbols[nominal][3] != "10")

    for row in model.rows:
        tag = row[0]
        if tag == "M":
            if pending_procedure is not None or pending_parameter is not None:
                raise ValueError("Frontend ABI snapshot crosses a module boundary")
            module += 1
            continue
        available = model.capabilities.get(module, {}).get(FEATURE) == "available"
        if available and tag == "T":
            latest_types[int(row[1])] = row
        elif available and tag == "F":
            if pending_procedure is not None:
                raise ValueError("Procedure signature lacks its frontend ABI companion")
            pending_procedure = row
            expected[module]["procedure"] += 1
        elif available and tag == "G":
            if pending_parameter is not None:
                raise ValueError("Formal parameter lacks its frontend ABI companion")
            pending_parameter = row
            expected[module]["parameter"] += 1
        if tag != "K" or not row[3].startswith((PROCEDURE, PARAMETER, COUNTS)):
            continue
        if not available:
            raise ValueError("Frontend ABI property has unavailable coverage")
        owner = symbol(row[2])
        if row[1] != "symbol":
            raise ValueError("Frontend ABI property has another subject domain")
        prefix, suffix = row[3].rsplit(":", 1)
        ordinal = integer(suffix, 1, LIMIT)
        fields = row[4].split("\t")
        if prefix + ":" == COUNTS:
            if (module in closures or ordinal != module or len(fields) != 2
                    or model.symbols[owner][3] != "8" or model.symbols[owner][11] != "0"):
                raise ValueError("Frontend ABI closure is repeated or foreign")
            closures[module] = [integer(value, 0, LIMIT) for value in fields]
            continue
        if module in closures:
            raise ValueError("Frontend ABI snapshot follows closure")
        family = "procedure" if prefix + ":" == PROCEDURE else "parameter"
        bucket = observations[module][family]
        if ordinal != len(bucket) + 1:
            raise ValueError("Frontend ABI ordinals are missing or repeated")
        if family == "procedure":
            native = pending_procedure
            typed = latest_types.get(owner)
            if (len(fields) != 6 or model.symbols[owner][3] != "3" or native is None
                    or native[1] != str(owner) or typed is None or fields[1:3] != typed[6:8]
                    or fields[3:5] != native[7:9]):
                raise ValueError("Procedure ABI disagrees with its live T/F signature")
            state, source, nominal, result, result_nominal, kind = fields
            source, result = dtype(source), dtype(result)
            symbol(nominal, nullable=True)
            symbol(result_nominal, nullable=True)
            if state not in ("available", "pending-type", "pending-layout", "unavailable"):
                raise ValueError("Unknown frontend result ABI state")
            if state == "available":
                if kind not in ("value", "byref", "hidden") or result == INVALID_TYPE or source == INVALID_TYPE:
                    raise ValueError("Available frontend result has no initialized policy")
                if (kind == "byref") != (native[6] == "1"):
                    raise ValueError("Frontend result confuses source Byref and hidden storage")
                name = model.primitives[module - 1][source & BASE_MASK][2]
                if kind == "hidden" and (source & POINTER_MASK or name not in ("type", "integer128", "uinteger128")):
                    raise ValueError("Scalar result invents hidden aggregate storage")
            elif kind:
                raise ValueError("Provisional frontend result invents a finalized policy")
            if state == "pending-layout" and (source & POINTER_MASK or model.primitives[module - 1][source & BASE_MASK][2] != "type"):
                raise ValueError("Nonaggregate result invents an unfinished layout")
            if state == "unavailable" and source != INVALID_TYPE and result != INVALID_TYPE:
                raise ValueError("Initialized result falsely claims unavailable ABI")
            if state == "pending-type" and not pending_type(source, int(nominal)):
                raise ValueError("Initialized result invents an unresolved type")
            pending_procedure = None
        else:
            native = pending_parameter
            if (len(fields) != 13 or model.symbols[owner][3] != "4" or native is None
                    or native[2] != str(owner) or fields[0:2] != native[1:2] + native[3:4]
                    or fields[3] != native[4] or fields[11] != native[8]):
                raise ValueError("Formal ABI disagrees with its adjacent G snapshot")
            proc, source_ordinal, state, mode, source, nominal, abi, abi_nominal, slot, register, descriptor, instance, kind = fields
            symbol(proc, "3")
            integer(source_ordinal, -1)
            source = dtype(source)
            nominal = symbol(nominal, nullable=True)
            slot = integer(slot, 0, 9223372036854775807)
            integer(register, 0, 2)
            descriptor = symbol(descriptor, "10", nullable=True)
            integer(instance, 0, 1)
            if mode != "bydesc" and descriptor:
                raise ValueError("Nondescriptor formal invents descriptor storage")
            if state not in ("available", "variadic", "pending-type", "pending-layout", "pending-descriptor", "unavailable"):
                raise ValueError("Unknown frontend formal ABI state")
            if state == "available":
                if source == INVALID_TYPE:
                    raise ValueError("Available formal ABI has an invalid source type")
                lowered, lowered_nominal = dtype(abi), symbol(abi_nominal, nullable=True)
                if mode == "bydesc":
                    structures = [identity for identity, info in model.primitives[module - 1].items() if info[2] == "type"]
                    if (not descriptor or kind != "descriptor" or lowered != structures[0] + POINTER_STEP
                            or lowered_nominal != descriptor):
                        raise ValueError("BYDESC ABI loses its selected descriptor pointer")
                elif mode == "byref" or kind == "implicit-reference":
                    if (kind != ("reference" if mode == "byref" else "implicit-reference")
                            or lowered != address_of(source) or lowered_nominal != nominal):
                        raise ValueError("Reference ABI loses its native pointer transformation")
                elif mode != "byval" or kind != "value" or (lowered, lowered_nominal) != (source, nominal):
                    raise ValueError("Value ABI changes its declared native type")
                if mode in ("byref", "bydesc") or kind == "implicit-reference":
                    if slot != int(model.contexts[module - 1][6]):
                        raise ValueError("Indirect formal ABI has another frontend slot size")
                elif kind == "value":
                    primitive = model.primitives[module - 1][source & BASE_MASK]
                    width = int(model.contexts[module - 1][6])
                    # The generic TYPE entry is unsized. Its zero is not the
                    # concrete UDT's size, and later layouts cannot validate
                    # an earlier formal snapshot retroactively.
                    size = (width if source & POINTER_MASK else int(primitive[4])
                            if primitive[4] and primitive[3] not in ("aggregate", "procedure", "unknown") else None)
                    if size is not None and slot != (size + width - 1) // width * width:
                        raise ValueError("Value formal ABI has another frontend slot size")
            else:
                if abi or abi_nominal:
                    raise ValueError("Unresolved formal ABI invents a lowered type")
                if state == "variadic":
                    if mode != "vararg" or kind != "variadic" or slot or descriptor:
                        raise ValueError("Variadic marker invents a fixed ABI value")
                elif kind:
                    raise ValueError("Pending formal invents a finalized ABI classification")
                if state == "pending-layout" and (mode != "byval" or source & POINTER_MASK
                        or model.primitives[module - 1][source & BASE_MASK][2] != "type"):
                    raise ValueError("Formal invents an unfinished aggregate layout")
                if state == "pending-descriptor" and (mode != "bydesc" or descriptor):
                    raise ValueError("Formal invents an unfinished descriptor")
                if state == "unavailable" and source != INVALID_TYPE:
                    raise ValueError("Initialized formal falsely claims unavailable ABI")
                if state == "pending-type" and not pending_type(source, nominal):
                    raise ValueError("Initialized formal invents an unresolved type")
            pending_parameter = None
        bucket[ordinal] = (owner, tuple(fields))

    if pending_procedure is not None or pending_parameter is not None:
        raise ValueError("Frontend ABI observation coverage is incomplete")
    model.frontend_abi = {}
    for module, capabilities in model.capabilities.items():
        if capabilities.get(FEATURE) != "available":
            continue
        counts = [len(observations[module][family]) for family in ("procedure", "parameter")]
        if module not in closures or counts != closures[module] or counts != [expected[module][family] for family in ("procedure", "parameter")]:
            raise ValueError("Frontend ABI module closure is incomplete")
        model.frontend_abi[module] = observations[module]

# end of semantic_frontend_abi.py
