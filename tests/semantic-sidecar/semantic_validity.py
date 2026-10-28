"""Project: FreeBASIC semantic sidecar
File: semantic_validity.py
Purpose: Validate native parser status and expose explicit failure boundaries.
Responsibilities: Statement counters, unfinished stacks and bounded diagnostic loci.
This file intentionally does NOT extrapolate physical ranges from logical lines.
"""

FEATURE = "native-source-validity"


def validate_validity(model, number):
    opens, closes, summaries, owners = {}, {}, {}, {}
    for row in model.records["K"]:
        if not row[3].startswith(("native-statement-", "native-source-validity")):
            continue
        module = number(row[2], 1)
        if row[1] != "module" or model.capabilities.get(module, {}).get(FEATURE) != "available":
            raise ValueError("Unavailable or unowned native validity observation")
        if row[3] == "native-source-validity-counts":
            if module in summaries:
                raise ValueError("Repeated native source validity summary")
            fields = row[4].split("\t")
            if len(fields) != 8 or fields[0] != "native-parser":
                raise ValueError("Malformed native validity phase or summary")
            for text in fields[1:]:
                value = number(text, 0)
                if str(value) != text or value > 2147483647:
                    raise ValueError("Noncanonical or oversized native validity count")
            summaries[module] = fields
            continue
        parts = row[3].split(":")
        if len(parts) != 2 or parts[0] not in ("native-statement-open", "native-statement-close"):
            raise ValueError("Unknown native validity role")
        statement = number(parts[1], 1)
        if str(statement) != parts[1] or statement not in model.statements or int(model.statements[statement][7]) != module:
            raise ValueError("Dangling or foreign native validity statement")
        owners[statement] = module
        if parts[0] == "native-statement-open":
            if statement in opens:
                raise ValueError("Repeated native statement validity opening")
            value = number(row[4], 0)
            if value > 2147483647 or str(value) != row[4]:
                raise ValueError("Invalid native statement initial counter")
            opens[statement] = value
        else:
            if statement in closes or statement not in opens:
                raise ValueError("Unopened or repeated native statement validity close")
            fields = row[4].split("\t")
            if len(fields) != 4:
                raise ValueError("Malformed native statement validity close")
            before, after = (number(text, 0) for text in fields[:2])
            if any(str(value) != text or value > 2147483647 for value, text in zip((before, after), fields)):
                raise ValueError("Invalid native statement comparison counter")
            ending = model.statement_endings.get(statement)
            if ending is None or fields[2:] != ending[2:4]:
                raise ValueError("Native validity disagrees with the parser statement end")
            expected = "unmatched" if fields[2] == "unmatched" else "parsed" if before == after else "recovered"
            if fields[3] != expected:
                raise ValueError("Native validity outcome contradicts native error counters")
            closes[statement] = fields
    statement_groups, compound_groups, opened_groups, closed_groups = {}, {}, {}, {}
    for identity in opens:
        opened_groups.setdefault(owners[identity], set()).add(identity)
    for identity in closes:
        closed_groups.setdefault(owners[identity], set()).add(identity)
    for statement, row in model.statements.items():
        statement_groups.setdefault(int(row[7]), set()).add(statement)
    for identity, row in model.constructs.items():
        module = int(model.source_contexts[int(row[6])][4])
        compound_groups.setdefault(module, set()).add(identity)
    for module, capabilities in model.capabilities.items():
        if capabilities.get(FEATURE) != "available":
            continue
        statements = statement_groups.get(module, set())
        compounds = compound_groups.get(module, set())
        ended_statements = statements & model.statement_endings.keys()
        ended_compounds = compounds & model.construct_endings.keys()
        if statements != opened_groups.get(module, set()):
            raise ValueError("Native statement validity opening coverage is incomplete")
        if ended_statements != closed_groups.get(module, set()):
            raise ValueError("Native statement validity closing coverage is incomplete")
        summary = summaries.get(module)
        expected = [len(statements), len(ended_statements), len(statements - ended_statements),
                    len(compounds), len(ended_compounds), len(compounds - ended_compounds)]
        if summary is None or summary[1:7] != list(map(str, expected)):
            raise ValueError("Native source validity stack counts are incomplete")
        diagnostic_total = model.diagnostic_totals.get(module)
        if diagnostic_total is not None and summary[7] != diagnostic_total[4]:
            raise ValueError("Native validity and diagnostic terminal counters disagree")
        if module not in model.recovered_modules and (expected[2] or expected[5]):
            raise ValueError("Accepted source retains unfinished native parser boundaries")
    model.native_statement_opens = opens
    model.native_statement_closes = closes
    model.source_validity = summaries
    model.failure_regions = []
    diagnostics_by_statement = {}
    for identity, diagnostic in model.diagnostics.items():
        diagnostics_by_statement.setdefault(int(diagnostic[10]), []).append(identity)
    for identity, row in model.statements.items():
        module = int(row[7])
        if model.capabilities.get(module, {}).get(FEATURE) != "available":
            continue
        ending = model.statement_endings.get(identity)
        if ending is not None and ending[3] == "parsed":
            continue
        model.failure_regions.append({"domain": "statement", "identity": identity, "module": module,
                                      "status": ending[3] if ending is not None else "unfinished",
                                      "start": row[11:], "end": ending[4:] if ending is not None else None,
                                      "diagnostics": diagnostics_by_statement.get(identity, [])})
    for identity, row in model.constructs.items():
        module = int(model.source_contexts[int(row[6])][4])
        if model.capabilities.get(module, {}).get(FEATURE) == "available" and identity not in model.construct_endings:
            model.failure_regions.append({"domain": "construct", "identity": identity, "module": module,
                                          "status": "unfinished", "start": row[8:], "end": None, "diagnostics": []})
    for identity, row in model.diagnostics.items():
        if row[2] == "error" and row[10] == "0":
            source = int(row[9])
            module = int(model.source_contexts[source][4]) if source in model.source_contexts else 0
            model.failure_regions.append({"domain": "diagnostic", "identity": identity, "module": module,
                                          "status": "reported", "source": row[7], "line": int(row[8]), "extent": None})

# end of semantic_validity.py
