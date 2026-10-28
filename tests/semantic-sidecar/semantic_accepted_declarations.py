"""Project: FreeBASIC semantic sidecar
File: semantic_accepted_declarations.py
Purpose: Validate accepted declaration order across all published source routes.
Responsibilities: Receipt identity, class/context ownership, order and closed coverage.
This file intentionally does NOT sort declarations by source coordinates or IDs.
"""

from collections import defaultdict

FEATURE = "accepted-source-declaration-order"
MACRO_FEATURE = "accepted-macro-declaration-order"
PREFIX = "accepted-source-declaration:"
CLOSURE = "accepted-source-declaration-count:"
LIMIT = 1000000


def validate_declarations(model, number, subject_modules):
    observations, expected, received = defaultdict(list), defaultdict(list), defaultdict(list)
    closures, bindings, declarations = {}, {}, {}
    macro_points = defaultdict(set)
    module = binding_id = 0

    def integer(value, minimum=0):
        result = number(value, minimum)
        if str(result) != value or result > 4294967295:
            raise ValueError("Accepted declaration integer is not canonical or bounded")
        return result

    # B identities are file-wide encounter identities, unlike per-module
    # acceptance ordinals. Keep those distinct when modules share one sidecar.
    for row in model.rows:
        if row[0] == "M":
            module += 1
        elif row[0] == "B":
            binding_id += 1
            if row[2] == "declaration":
                bindings[binding_id] = row
                expected[module].append(("binding", binding_id))
        elif row[0] == "DCL":
            identity = int(row[1])
            declarations[identity] = row
            expected[module].append(("declaration", identity))
        elif row[0] == "K" and row[3].startswith((PREFIX, CLOSURE)):
            if model.capabilities.get(module, {}).get(FEATURE) != "available":
                raise ValueError("Accepted declaration has unavailable coverage")
            owner = integer(row[2], 1)
            symbol = model.symbols.get(owner)
            if (row[1] != "symbol" or symbol is None or symbol[3] != "8" or symbol[11] != "0"
                    or subject_modules.get(("symbol", owner)) != module):
                raise ValueError("Accepted declaration lacks its module namespace owner")
            parts = row[3].split(":")
            if len(parts) != 2:
                raise ValueError("Malformed accepted declaration key")
            identity = integer(parts[1], 1)
            if row[3].startswith(CLOSURE):
                if identity != module or module in closures:
                    raise ValueError("Accepted declaration closure is repeated or foreign")
                closures[module] = integer(row[4])
                continue
            if identity != len(observations[module]) + 1 or identity > LIMIT or module in closures:
                raise ValueError("Accepted declaration order is not dense or is already closed")
            fields = row[4].split("\t")
            if len(fields) != 8 or fields[0] not in ("binding", "declaration", "macro-point"):
                raise ValueError("Malformed accepted declaration receipt")
            kind = fields[0]
            receipt, target, raw_class = (integer(value, 1) for value in fields[1:4])
            native_statement, statement, source, context = (integer(value) for value in fields[4:])
            selected = model.symbols.get(target)
            if (selected is None or selected[3] != str(raw_class)
                    or subject_modules.get(("symbol", target)) != module):
                raise ValueError("Accepted declaration symbol has another class or module")
            if statement and (statement not in model.statements or model.statements[statement][7] != str(module)):
                raise ValueError("Accepted declaration statement belongs elsewhere")
            if source and (source not in model.source_contexts or model.source_contexts[source][4] != str(module)):
                raise ValueError("Accepted declaration source belongs elsewhere")
            if context and model.configurations.get(context) != module:
                raise ValueError("Accepted declaration configuration belongs elsewhere")
            if kind == "macro-point":
                if model.capabilities.get(module, {}).get(MACRO_FEATURE) != "available":
                    raise ValueError("Accepted macro declaration has unavailable coverage")
                macro_points[module].add((target, receipt))
                if model.macro_origins.get(("symbol", target, "declaration-" + str(receipt))) != receipt:
                    raise ValueError("Accepted macro declaration lacks its real expansion")
            else:
                key = kind, receipt
                original = bindings.get(receipt) if kind == "binding" else declarations.get(receipt)
                original_symbol = original[1 if kind == "binding" else 2] if original else None
                if original_symbol != str(target) or subject_modules.get(key) != module:
                    raise ValueError("Accepted declaration is not its published source receipt")
                if kind == "declaration" and original[12] != str(native_statement):
                    raise ValueError("Accepted declaration disagrees with its native statement counter")
                if model.statement_owners.get(key, 0) != statement or model.origins.get(key, 0) != source:
                    raise ValueError("Accepted declaration disagrees with its source provenance")
                if kind == "declaration" and model.context_uses.get(key, 0) != context:
                    raise ValueError("Accepted declaration disagrees with its selected configuration")
                received[module].append(key)
            observations[module].append(fields)

    expected_macros = defaultdict(set)
    for (domain, target, role), expansion in model.macro_origins.items():
        if domain == "symbol" and role == "declaration-" + str(expansion):
            expected_macros[subject_modules["symbol", target]].add((target, expansion))
    model.accepted_declarations = dict(observations)
    for module in range(1, len(model.records["M"]) + 1):
        available = model.capabilities.get(module, {}).get(FEATURE) == "available"
        if available and (closures.get(module) != len(observations[module]) or received[module] != expected[module]):
            raise ValueError("Accepted source declaration coverage or encounter order is incomplete")
        if model.capabilities.get(module, {}).get(MACRO_FEATURE) == "available":
            if not available or macro_points[module] != expected_macros[module]:
                raise ValueError("Accepted macro declaration coverage is incomplete")
        if not available and (module in closures or observations[module]):
            raise ValueError("Unavailable accepted declaration coverage has receipts")

# end of semantic_accepted_declarations.py
