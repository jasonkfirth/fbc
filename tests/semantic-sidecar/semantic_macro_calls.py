"""Project: FreeBASIC semantic sidecar tests
File: semantic_macro_calls.py
Purpose: Validate original macro-reference identities on surviving source calls.
Responsibilities: Selected callees, exact MR origins and per-module closures.
This file intentionally does NOT infer calls from names or source coordinates.
"""

FEATURE = "source-call-macro-references"
LINK = "source-call-macro-reference"
COUNT = "source-call-macro-count:"
PREFIX = "source-call-macro-"
LIMIT = 1000000


def validate_macro_calls(model, number, subject_modules):
    def decimal(text, minimum=0):
        value = number(text, minimum)
        if str(value) != text or value > 2147483647:
            raise ValueError("Macro call identity is not canonical or bounded")
        return value

    def available(module):
        return model.capabilities.get(module, {}).get(FEATURE) == "available"

    references = {}
    if any(available(module) for module in model.capabilities):
        for (domain, target, role), expansion in model.macro_origins.items():
            if domain != "symbol" or not role.startswith("reference-"):
                continue
            identity = decimal(role[len("reference-"):], 1)
            if identity in references:
                raise ValueError("Macro reference identity is ambiguous")
            references[identity] = target, expansion

    static = {int(row[2]): int(row[4]) for row in model.records["H"]
              if row[1] == "node" and row[3] == "symbol" and row[5] == "static-target"}
    physical = {int(row[2]) for row in model.records["H"]
                if row[1] == "node" and row[3] == "binding" and row[5] == "source-binding"}
    keys, closures, observed = set(), {}, {}
    for row in model.records["K"]:
        if not row[3].startswith(PREFIX):
            continue
        key = tuple(row[1:4])
        if key in keys:
            raise ValueError("Duplicate immutable macro call observation")
        keys.add(key)
        owner = decimal(row[2], 1)
        module = subject_modules.get((row[1], owner))
        if not available(module):
            raise ValueError("Macro call observation has no available capability")
        if row[3].startswith(COUNT):
            symbol = model.symbols.get(owner)
            if (row[1] != "symbol" or symbol is None or symbol[3] != "8"
                    or symbol[11] != "0" or decimal(row[3][len(COUNT):], 1) != module
                    or module in closures):
                raise ValueError("Macro call closure has an invalid owner")
            closures[module] = decimal(row[4])
            if closures[module] > LIMIT:
                raise ValueError("Macro call closure exceeds its bound")
            continue
        if row[1] != "node" or row[3] != LINK:
            raise ValueError("Unknown macro call observation")
        node = model.nodes.get(owner)
        kind = model.properties["node", owner].get("call-kind")
        if node is None or node[4] != "9" or kind not in ("direct", "virtual", "runtime") or owner in physical:
            raise ValueError("Macro call link is not an unambiguous selected call")
        reference = decimal(row[4], 1)
        target = static.get(owner, 0) if kind == "virtual" else decimal(node[9], 1)
        symbol = model.symbols.get(target)
        origin = references.get(reference)
        if (symbol is None or symbol[3] != "3" or origin is None or origin[0] != target
                or subject_modules.get(("symbol", target)) != module):
            raise ValueError("Macro call reference disagrees with the selected procedure")
        # ASSERT selects an RTL procedure through the source-call parser.
        # Keep its exact origin; generated helpers without MR remain unlinked.
        if kind == "runtime":
            target_type = model.types.get(target)
            if target_type is None or target_type[18] != "runtime":
                raise ValueError("Macro runtime call does not select a runtime procedure")
        invocation = model.macro_invocations.get(origin[1])
        if (invocation is None or model.source_contexts[int(invocation[4])][4] != str(module)):
            raise ValueError("Macro call reference lacks its original module origin")
        observed[module] = observed.get(module, 0) + 1
        if observed[module] > LIMIT:
            raise ValueError("Macro call observations exceed their bound")
    for module in model.capabilities:
        if available(module) and closures.get(module) != observed.get(module, 0):
            raise ValueError("Macro call observation coverage is incomplete")


# end of semantic_macro_calls.py
