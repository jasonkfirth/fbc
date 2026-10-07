"""Project: FreeBASIC semantic sidecar tests
File: semantic_queries.py
Purpose: Validate compiler-owned unevaluated query operand intervals.
Responsibilities: Complete markers, expression identities and module ownership.
This file intentionally does NOT parse query expressions from source text.
"""

EXPRESSION_LIMIT = 1000000
VALIDATION_WORK_LIMIT = 8000000
NAMESPACE_CLASS = 8


def validate_query_inputs(model, number, subject_modules):
    marked = set()
    covered = set()
    ranges = []
    work_left = VALIDATION_WORK_LIMIT
    for (domain, identity), properties in model.properties.items():
        for key, value in properties.items():
            work_left -= 1
            if work_left < 0:
                raise ValueError("Unevaluated query validation work budget exceeded")
            if not key.startswith("unevaluated-query-"):
                continue
            module = subject_modules.get((domain, identity))
            if module is None or model.capabilities[module].get("unevaluated-query-inputs") != "available":
                raise ValueError("Unevaluated query capability unavailable")
            if key == "unevaluated-query-input":
                if domain != "expression" or value != "1":
                    raise ValueError("Invalid unevaluated query input")
                marked.add(identity)
            elif key.startswith("unevaluated-query-range-"):
                if number(key[len("unevaluated-query-range-"):], 1) > 4294967295:
                    raise ValueError("Unevaluated query range identity exceeds the wire limit")
                owner = model.symbols.get(identity)
                if domain != "symbol" or owner is None or int(owner[3]) != NAMESPACE_CLASS:
                    raise ValueError("Unevaluated query range has no namespace owner")
                fields = value.split("\t")
                if len(fields) != 3 or fields[2] not in ("typeof", "sizeof"):
                    raise ValueError("Invalid unevaluated query range payload")
                first, last = number(fields[0], 1), number(fields[1], 1)
                if not first <= last <= EXPRESSION_LIMIT:
                    raise ValueError("Invalid unevaluated query range bounds")
                ranges.append((first, last, module))
            else:
                raise ValueError("Unknown unevaluated query property")
    # A parser range also covers original operands erased by constant folding.
    # Checking each ID avoids accepting a truncated but internally closed graph.
    for first, last, module in ranges:
        work_left -= last - first + 1
        if work_left < 0:
            raise ValueError("Unevaluated query validation work budget exceeded")
        for identity in range(first, last + 1):
            if identity not in marked or subject_modules.get(("expression", identity)) != module:
                raise ValueError("Unevaluated query input coverage is incomplete")
            covered.add(identity)
    if marked != covered:
        raise ValueError("Unevaluated query input lacks its parser range")

# end of semantic_queries.py
