"""Project: FreeBASIC semantic sidecar tests.
File: semantic_variable_names.py
Purpose: Independently validate original variable declaration spellings.
Responsibilities: Native ownership, complete counts and physical/macro origins.
This file intentionally does NOT parse declarations or choose a naming convention.
"""

import re

PREFIX = "variable-declaration-spelling:"
COUNT_PREFIX = "accepted-variable-name-count:"
FEATURE = "variable-declaration-spellings"
MODULE_PROC = 0x2000  # FB_SYMBSTATS_MODLEVELPROC, not a procedure-name guess.


def global_variable(model, identity):
    symbol, snapshot = model.symbols.get(identity), model.types.get(identity)
    if symbol is None or symbol[3] != "1" or snapshot is None or snapshot[18] != "source":
        return False
    owner = int(snapshot[8])
    owner_symbol, owner_snapshot = model.symbols.get(owner), model.types.get(owner)
    return owner_symbol is not None and (
        owner_symbol[3] == "8" or (owner_symbol[3] == "3" and owner_snapshot is not None
                                  and int(owner_snapshot[12]) & MODULE_PROC))


def validate_variable_names(model, number, subject_modules):
    spellings, counts, receipts = {}, {}, {}
    locations, expansions = set(), set()
    work_left = 8_000_000
    for row in model.records["K"]:
        work_left -= 1
        if work_left < 0:
            raise ValueError("Variable spelling work budget exceeded")
        domain, identity, key, payload = row[1], int(row[2]), row[3], row[4]
        module = subject_modules.get((domain, identity))
        if key.startswith(COUNT_PREFIX):
            count_module = number(key[len(COUNT_PREFIX):], 1)
            if (domain != "symbol" or count_module != module or count_module in receipts
                    or model.symbols.get(identity, [None] * 4)[3] != "8"
                    or model.capabilities[module].get(FEATURE) != "available"):
                raise ValueError("Variable name coverage has a foreign or duplicate receipt")
            receipts[count_module] = number(payload, 0)
        if not key.startswith(PREFIX):
            continue
        detail = number(key[len(PREFIX):], 1)
        fields = payload.split("\t")
        symbol = model.symbols.get(identity)
        work_left -= len(key) + len(payload)
        if (work_left < 0 or domain != "symbol" or symbol is None or symbol[3] != "1"
                or len(fields) != 4 or len(key) > 64 or len(payload) > 256
                or detail > len(model.rows) or (identity, key) in spellings
                or len(spellings) >= 100_000
                or model.capabilities[module].get(FEATURE) != "available"):
            raise ValueError("Invalid variable spelling identity or capability")
        statement, source = (number(value, 1) for value in fields[:2])
        kind, written = fields[2:]
        snapshot = model.types.get(identity)
        owner = model.symbols.get(int(snapshot[8])) if snapshot is not None else None
        if snapshot is None or owner is None:
            raise ValueError("Variable spelling lacks a complete native owner snapshot")
        if owner[3] == "3":
            owner_snapshot = model.types.get(int(owner[1]))
            if owner_snapshot is None:
                raise ValueError("Variable owner lacks its current native procedure snapshot")
        if (len(written) > 128 or re.fullmatch(r"[A-Za-z_][A-Za-z_0-9.]*", written) is None
                or written.upper() != symbol[2].upper() or kind not in ("declared", "inferred", "implicit")
                or statement not in model.statements or source not in model.source_contexts
                or subject_modules.get(("statement", statement)) != module
                or int(model.source_contexts[source][4]) != module
                or model.statement_endings[statement][3] != "parsed"):
            raise ValueError("Variable spelling disagrees with accepted native ownership")
        location = model.physical_locations.get(("source-context", source, key))
        macro = model.macro_origins.get(("symbol", identity, key))
        if location is None and macro is None:
            raise ValueError("Variable spelling lacks its source or macro origin")
        if location is not None and int(location[4]) != source:
            raise ValueError("Variable spelling location changes source identity")
        if location is not None:
            locations.add((identity, tuple(location[4:11])))
        if macro is not None:
            attempt = model.macro_invocations.get(macro)
            if (attempt is None or attempt[7] != "normal"
                    or int(model.source_contexts[int(attempt[4])][4]) != module):
                raise ValueError("Variable spelling has no normal owned expansion")
            expansions.add((identity, macro))
        spellings[identity, key] = statement, source, kind, written
        counts[module] = counts.get(module, 0) + 1
    for module, capabilities in model.capabilities.items():
        if capabilities.get(FEATURE) == "available" and receipts.get(module) != counts.get(module, 0):
            raise ValueError("Variable name count is missing or contradictory")
    observed = {identity for identity, _ in spellings}
    for identity in model.symbols:
        work_left -= 1
        if work_left < 0:
            raise ValueError("Global variable spelling closure budget exceeded")
        module = subject_modules["symbol", identity]
        if (global_variable(model, identity) and model.capabilities[module].get(FEATURE) == "available"
                and identity not in observed):
            raise ValueError("Accepted global variable lacks its original spelling")
    for ordinal, binding in enumerate(model.records["B"], 1):
        identity = int(binding[1])
        module = subject_modules["symbol", identity]
        if (binding[2] == "declaration" and global_variable(model, identity)
                and model.capabilities[module].get(FEATURE) == "available"):
            location = model.physical_locations.get(("binding", ordinal, "range"))
            if location is None or (identity, tuple(location[4:11])) not in locations:
                raise ValueError("Accepted global declaration lacks its original spelling occurrence")
    for origin in model.records["MR"]:
        if origin[1] != "symbol" or not origin[4].startswith("declaration-"):
            continue
        identity = int(origin[2])
        module = subject_modules["symbol", identity]
        if (global_variable(model, identity) and model.capabilities[module].get(FEATURE) == "available"
                and (identity, int(origin[3])) not in expansions):
            raise ValueError("Generated global declaration lacks its original spelling")
    model.variable_declaration_spellings = spellings

# end of semantic_variable_names.py
