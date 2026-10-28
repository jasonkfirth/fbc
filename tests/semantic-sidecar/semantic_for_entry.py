"""Project: FreeBASIC semantic sidecar
File: semantic_for_entry.py
Purpose: Validate selected scalar FOR entry decisions and counter coverage.
Responsibilities: Folded input ownership, available decisions and complete closure.
This file intentionally does NOT reconstruct conversions or runtime loop behavior.
"""

from semantic_datatypes import NUMERIC_NAMES, primitive_name


def validate_for_entry(model, number, subject_modules):
    """Known decisions describe the parser's actual folded initial test."""
    prefix = "for-entry-result:"
    for (domain, identity), properties in model.properties.items():
        symbol = model.symbols.get(identity) if domain == "symbol" else None
        module = subject_modules[domain, identity]
        scalar = symbol is not None and int(symbol[3]) == 1 and (
            (not int(symbol[5]) and not int(symbol[4]) & 0x1e0
             and primitive_name(model, module, int(symbol[4])) in NUMERIC_NAMES) or
            (int(symbol[4]) & 0x1ff == 10 and
             int(model.symbols.get(int(symbol[5]), [0, 0, 0, 0])[3]) == 9))
        module = subject_modules[domain, identity]
        available = model.capabilities[module].get("scalar-for-entry-results") == "available"
        for key, result in properties.items():
            if key.startswith(prefix):
                statement = number(key[len(prefix):], 1)
                record = model.statements.get(statement)
                if (not scalar or not available or result not in ("skip", "enter", "unknown")
                        or record is None or int(record[9]) != 281
                        or model.statement_endings.get(statement, [0, 0, 0, ""])[3] != "parsed"
                        or subject_modules["statement", statement] != module
                        or f"for-counter:{statement}" not in properties):
                    raise ValueError("Invalid scalar FOR entry decision")
                inputs = ("for-start-expression:", "for-limit-expression:", "for-step-selected-expression:")
                values = [model.constants.get(("expression", int(properties.get(p + str(statement), "0"))))
                          for p in inputs]
                constant = all(v is not None and v[0] in ("signed", "unsigned", "float64-bits") for v in values)
                if (result != "unknown") != constant:
                    raise ValueError("Scalar FOR entry knowledge disagrees with constant inputs")
            if scalar and available and key.startswith("for-counter:"):
                if prefix + key[12:] not in properties:
                    raise ValueError("Scalar FOR lacks entry decision coverage")

# end of semantic_for_entry.py
