"""Project: FreeBASIC semantic sidecar tests
File: semantic_type_spellings.py
Purpose: Validate original type and enum declaration spellings independently.
Responsibilities: Native identity, occurrence coverage and source/macro ownership.
This file intentionally does NOT parse declarations or apply naming conventions.
"""

import re


PREFIX = "type-declaration-spelling:"
FEATURE = "type-declaration-spellings"


def validate_type_spellings(model, number, subject_modules):
    spellings, locations, expansions = {}, set(), set()
    work_left = 8_000_000

    def eligible(symbol):
        return symbol is not None and (symbol[3] in ("9", "10", "13") or (
            symbol[3] == "2" and symbol[4] == "10"
            and model.symbols.get(int(symbol[5]), [None] * 4)[3] == "9"
            and "enum-element-input" in model.properties.get(("symbol", int(symbol[1])), {})))

    for row in model.records["K"]:
        work_left -= 1
        if work_left < 0:
            raise ValueError("Type declaration spelling work budget exceeded")
        domain, identity, key, payload = row[1], int(row[2]), row[3], row[4]
        if not key.startswith(PREFIX):
            continue
        detail = number(key[len(PREFIX):], 1)
        fields = payload.split("\t")
        symbol = model.symbols.get(identity)
        module = subject_modules.get((domain, identity))
        if (domain != "symbol" or not eligible(symbol) or len(fields) != 4
                or len(payload) > 256 or detail > len(model.rows) or (identity, key) in spellings
                or model.capabilities[module].get(FEATURE) != "available"):
            raise ValueError("Invalid type declaration spelling identity or capability")
        statement, source = (number(value, 1) for value in fields[:2])
        kind, written = fields[2:]
        if (len(written) > 128 or re.fullmatch(r"[A-Za-z_][A-Za-z_0-9.]*", written) is None
                or written.upper() != symbol[2].upper()
                or symbol[3] != {"type": "10", "union": "10", "alias": "13",
                                  "enum": "9", "enum-member": "2"}.get(kind)
                or statement not in model.statements or source not in model.source_contexts
                or subject_modules.get(("statement", statement)) != module
                or int(model.source_contexts[source][4]) != module):
            raise ValueError("Type declaration spelling disagrees with its native ownership")
        ending = model.statement_endings[statement]
        if ending[3] != "parsed":
            raise ValueError("Type declaration spelling has no accepted statement")
        if kind == "enum-member":
            member = model.properties["symbol", identity].get("enum-element-input", "").split("\t")
            if ending[2] != "enumerator" or len(member) != 5 or int(member[2]) != statement:
                raise ValueError("Enum spelling disagrees with its actual member input")
        elif ending[2] not in ("declaration", "aggregate-member"):
            raise ValueError("Type spelling does not belong to a declaration")
        location = model.physical_locations.get(("source-context", source, key))
        macro = model.macro_origins.get(("symbol", identity, key))
        if location is None and macro is None:
            raise ValueError("Type declaration spelling has no native origin")
        if location is not None:
            if int(location[4]) != source:
                raise ValueError("Type spelling location changes source identity")
            locations.add((identity, tuple(location[4:11])))
        if macro is not None:
            attempt = model.macro_invocations.get(macro)
            if attempt is None or attempt[7] != "normal" or int(model.source_contexts[int(attempt[4])][4]) != module:
                raise ValueError("Type declaration spelling has no normal macro origin")
            expansions.add((identity, macro))
        spellings[identity, key] = statement, source, kind, written
    for ordinal, binding in enumerate(model.records["B"], 1):
        identity = int(binding[1])
        module = subject_modules["symbol", identity]
        if (binding[2] == "declaration" and eligible(model.symbols.get(identity))
                and model.capabilities[module].get(FEATURE) == "available"):
            location = model.physical_locations.get(("binding", ordinal, "range"))
            if location is None or (identity, tuple(location[4:11])) not in locations:
                raise ValueError(f"Accepted type declaration {identity} at binding {ordinal} lacks its original spelling occurrence")
    for origin in model.records["MR"]:
        if origin[1] != "symbol" or not origin[4].startswith("declaration-"):
            continue
        identity = int(origin[2])
        module = subject_modules["symbol", identity]
        if (eligible(model.symbols.get(identity)) and model.capabilities[module].get(FEATURE) == "available"
                and (identity, int(origin[3])) not in expansions):
            raise ValueError("Generated type declaration lacks its original spelling")
    model.type_declaration_spellings = spellings

# end of semantic_type_spellings.py
