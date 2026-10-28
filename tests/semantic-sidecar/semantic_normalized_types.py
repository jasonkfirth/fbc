"""Project: FreeBASIC semantic sidecar
File: semantic_normalized_types.py
Purpose: Check normalized types against their actual native snapshots.
Responsibilities: Type identity, qualifiers, target width and complete coverage.
This file intentionally does NOT infer a type from its display spelling.
"""

FEATURE = "normalized-type-closure"
MODIFIER_PREFIX = "mangling-modifier-definition:"
MODIFIER_NAMES = {"integer": "c-long", "uinteger": "c-unsigned-long", "zstring": "c-char",
                  "boolean": "c-bool", "va_list": "c-va-list"}
# Native and stable-v1 encode the same independent dtype fields: base 0..4,
# pointer depth 5..8, const positions from bit 9, ByRef at 19 and mangling
# modifier 20..24. The ninth-level address temporary can use const bit 18.
# Y supplies the actual base/modifier names; display spellings are irrelevant.
BASE_MASK = 31
POINTER_SHIFT, POINTER_MASK = 5, 15
CONST_SHIFT = 9
REFERENCE_MASK = 1 << 19
MANGLE_SHIFT = 20
MAX_DATATYPE = 4294967295
INVALID_TYPE = 2147483648


def validate_normalized_types(model, number, subject_modules):
    # A symbol NT precedes its T. Node and expression NT follow their native
    # record, with parser receipts permitted between E and its delayed NT.
    # Consume each pair once so a final symbol snapshot cannot validate an
    # earlier forward-type observation retroactively.
    symbols, nodes, expressions = {}, {}, {}
    definitions = {}
    module = 0

    def check(normalized, native, dtype, nominal):
        identity = number(normalized[2], 1)
        if subject_modules.get((normalized[1], identity)) != module:
            raise ValueError("Normalized type belongs to another module")
        if dtype < 0 or dtype > MAX_DATATYPE or dtype == INVALID_TYPE:
            raise ValueError("Normalized type has no valid native datatype")
        primitives = model.primitives[module - 1]
        base = primitives.get(dtype & BASE_MASK)
        if base is None or normalized[4] != base[2] or normalized[5] != str(nominal):
            raise ValueError("Normalized type disagrees with native type identity")
        if nominal and subject_modules.get(("symbol", nominal)) != module:
            raise ValueError("Normalized nominal type belongs to another module")
        pointers = (dtype >> POINTER_SHIFT) & POINTER_MASK
        qualifiers = "".join("1" if dtype & (1 << (CONST_SHIFT + position)) else "0"
                             for position in range(pointers + 1))
        reference = "1" if dtype & REFERENCE_MASK else "0"
        modifier = (dtype >> MANGLE_SHIFT) & BASE_MASK
        if modifier and modifier not in primitives:
            raise ValueError("Native type has an unknown mangling modifier")
        if (modifier and model.capabilities[module].get(FEATURE) == "available"
                and modifier not in definitions.get(module, {})):
            raise ValueError("Native type lacks a named mangling classifier")
        modifier_name = primitives[modifier][2] if modifier else ""
        if normalized[6:9] != [str(pointers), reference, qualifiers] or normalized[10] != modifier_name:
            raise ValueError("Normalized type loses native indirection or qualifiers")
        if pointers:
            size = model.contexts[module - 1][6]
        elif base[2] == "fixed-string":
            # Only symbols supply a width in every historical producer.
            # An expression's selected storage can be unavailable at capture.
            if native[0] != "T":
                return
            size = native[13] if number(native[13]) > 0 else ""
        elif base[2] == "type":
            # Finalization is an independent layout observation. A later U
            # must not fill an earlier incomplete NT's unknown storage width.
            return
        else:
            size = base[4] if base[4] and number(base[4]) > 0 else ""
        if normalized[9] != size:
            raise ValueError("Normalized type storage contradicts its native target")

    for row in model.rows:
        tag = row[0]
        if tag == "M":
            if symbols or nodes or expressions:
                raise ValueError("Normalized type pairing crosses a module boundary")
            module += 1
            continue
        available = model.capabilities.get(module, {}).get(FEATURE) == "available"
        if tag == "K" and row[3].startswith(MODIFIER_PREFIX):
            if not available:
                raise ValueError("Named mangling definition has unavailable coverage")
            ordinal_text = row[3][len(MODIFIER_PREFIX):]
            ordinal = number(ordinal_text, 0)
            bucket = definitions.setdefault(module, {})
            identity = number(row[2], 1)
            owner = model.symbols.get(identity)
            fields = row[4].split("\t")
            if (ordinal_text != str(ordinal) or ordinal != len(bucket) or row[1] != "symbol"
                    or owner is None or owner[3] != "8" or owner[11] != "0"
                    or subject_modules.get(("symbol", identity)) != module or len(fields) != 2):
                raise ValueError("Named mangling definition has an invalid owner or ordinal")
            primitive = number(fields[0], 0)
            info = model.primitives[module - 1].get(primitive)
            if (primitive in bucket or info is None or info[2] not in MODIFIER_NAMES
                    or fields[1] != MODIFIER_NAMES[info[2]] or fields[0] != str(primitive)):
                raise ValueError("Named mangling definition has another native classifier")
            bucket[primitive] = fields[1]
        elif tag in ("N", "E"):
            dtype = number(row[8] if tag == "N" else row[12], 0)
            if available and dtype != INVALID_TYPE:
                (nodes if tag == "N" else expressions)[number(row[1], 1)] = row
        elif tag == "T":
            normalized = symbols.pop(number(row[1], 1), None)
            if normalized is not None:
                check(normalized, row, number(row[6], 0), number(row[7], 0))
            elif available and number(row[6], 0) != INVALID_TYPE:
                raise ValueError("Native symbol snapshot lacks its normalized type")
        elif tag == "NT":
            if model.capabilities[module].get(FEATURE) == "unavailable":
                raise ValueError("Normalized type has unavailable coverage")
            identity = number(row[2], 1)
            if row[1] == "symbol":
                if identity in symbols:
                    raise ValueError("Several normalized types precede one symbol snapshot")
                symbols[identity] = row
            else:
                if row[1] == "node":
                    native = nodes.pop(identity, None) if available else model.nodes[identity]
                    if native is None:
                        raise ValueError("Normalized node type lacks its native observation")
                    check(row, native, number(native[8], 0), number(native[10], 0))
                else:
                    native = expressions.pop(identity, None) if available else None
                    if native is None and not available:
                        # Older producers have no completeness promise. Keep
                        # their structural NT validation without fabricating
                        # a missing snapshot or requiring a new capability.
                        continue
                    if native is None:
                        raise ValueError("Normalized expression type lacks its native observation")
                    check(row, native, number(native[12], 0), number(native[14], 0))
    if symbols or nodes or expressions:
        raise ValueError("Normalized type snapshot coverage is incomplete")
    model.mangling_modifiers = {}
    for module, capabilities in model.capabilities.items():
        if capabilities.get(FEATURE) == "available":
            bucket = definitions.get(module, {})
            if set(bucket.values()) != set(MODIFIER_NAMES.values()):
                raise ValueError("Named mangling classifier vocabulary is incomplete")
            model.mangling_modifiers[module] = bucket

# end of semantic_normalized_types.py
