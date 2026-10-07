"""Project: FreeBASIC semantic sidecar tests
File: semantic_literals.py
Purpose: Validate selected C-backend wide-literal prefix contracts.
Responsibilities: Coverage, literal identities, target widths and termination.
This file intentionally does NOT derive target text from host wide units.
"""
import re


def validate_wide_literals(model, number, subject_modules):
    required = {"literal-target-wide-unit-bytes", "literal-target-wide-kind", "literal-target-wide-prefix"}
    marked = set()
    work_left = 8000000
    for (domain, identity), properties in model.properties.items():
        work_left -= len(properties)
        if work_left < 0:
            raise ValueError("Wide literal validation work budget exceeded")
        keys = {key for key in properties if key.startswith("literal-target-wide-")}
        if not keys:
            continue
        symbol = model.symbols.get(identity)
        module = subject_modules.get((domain, identity))
        if domain != "symbol" or symbol is None or keys != required:
            raise ValueError("Incomplete or unknown wide literal prefix contract")
        if int(symbol[3]) != 1 or int(symbol[4]) & 0x1ff != 7 or not int(symbol[7]) & 0x400:
            raise ValueError("Wide literal prefix has no literal variable")
        if model.capabilities[module].get("c-target-wide-literal-prefixes") != "available":
            raise ValueError("Wide literal prefix capability unavailable")
        unit_bytes = number(properties["literal-target-wide-unit-bytes"], 1)
        if unit_bytes not in (1, 2, 4) or str(unit_bytes) != model.primitives[module - 1][7][4]:
            raise ValueError("Wide literal prefix has another target unit width")
        if model.constants.get((domain, identity), (None,))[0] != "wide-units":
            raise ValueError("Wide literal prefix lacks its literal value")
        if properties["literal-target-wide-kind"] not in ("terminated", "unterminated"):
            raise ValueError("Unknown wide literal prefix termination")
        prefix = properties["literal-target-wide-prefix"]
        work_left -= len(prefix)
        if work_left < 0:
            raise ValueError("Wide literal validation work budget exceeded")
        if len(prefix) > 1048576 * 8 or len(prefix) % 8 or re.fullmatch(r"[0-9A-F]*", prefix) is None:
            raise ValueError("Invalid wide literal prefix payload")
        maximum = (1 << (unit_bytes * 8)) - 1
        if any(not 0 < int(prefix[index:index + 8], 16) <= maximum for index in range(0, len(prefix), 8)):
            raise ValueError("Wide literal prefix contains zero or an oversized unit")
        marked.add(identity)
    for identity, symbol in model.symbols.items():
        work_left -= 1
        if work_left < 0:
            raise ValueError("Wide literal validation work budget exceeded")
        if int(symbol[3]) == 1 and int(symbol[4]) & 0x1ff == 7 and int(symbol[7]) & 0x400:
            module = subject_modules["symbol", identity]
            if model.capabilities[module].get("c-target-wide-literal-prefixes") == "available" and identity not in marked:
                raise ValueError("Wide literal prefix coverage is incomplete")

# end of semantic_literals.py
