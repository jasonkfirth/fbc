"""Project: FreeBASIC semantic sidecar tests
File: semantic_selects.py
Purpose: Validate accepted SELECT CASE grammar observation groups.
Responsibilities: Ownership, ordered clauses, original operands and coverage.
This file intentionally does NOT infer case values or execution from source.
"""

WORK_LIMIT = 8000000
WIRE_ID_LIMIT = 4294967295
SELECT_TOKEN = 270
CASE_TOKEN = 271


def validate_select_inputs(model, number, subject_modules):
    constructs = {int(row[1]): row for row in model.records["BLK"]}
    inputs, clauses, alternatives, endings = {}, {}, {}, {}
    work_left = WORK_LIMIT

    def integer(text, minimum=0):
        value = number(text, minimum)
        if value > WIRE_ID_LIMIT:
            raise ValueError("SELECT observation exceeds the wire identity limit")
        return value

    def same_module(domain, identity, module):
        observed = subject_modules.get((domain, identity))
        if domain == "statement":
            row = model.statements.get(identity)
            observed = int(row[7]) if row is not None else None
        elif domain == "construct":
            block = constructs.get(identity)
            row = model.statements.get(int(block[3])) if block is not None else None
            observed = int(row[7]) if row is not None else None
        if observed != module:
            raise ValueError("SELECT observation has a missing or foreign identity")

    for (domain, owner), properties in model.properties.items():
        work_left -= len(properties)
        if work_left < 0:
            raise ValueError("SELECT validation work budget exceeded")
        for key, value in properties.items():
            if not key.startswith("select-case-"):
                continue
            module = subject_modules.get((domain, owner))
            if domain != "symbol" or module is None or model.capabilities[module].get("select-case-inputs") != "available":
                raise ValueError("SELECT observation has no available compiler capability")
            if int(model.symbols[owner][3]) not in (3, 8):
                raise ValueError("SELECT observation has no procedure or namespace owner")
            parts, fields = key.split(":"), value.split("\t")
            if parts[0] == "select-case-input" and len(parts) == 2 and len(fields) == 4:
                construct = integer(parts[1], 1)
                statement, expression, storage = (integer(field, 1) for field in fields[:3])
                same_module("construct", construct, module)
                same_module("statement", statement, module)
                same_module("expression", expression, module)
                same_module("symbol", storage, module)
                block = constructs[construct]
                if block[5] != "select" or int(block[3]) != statement or int(model.statements[statement][9]) != SELECT_TOKEN:
                    raise ValueError("SELECT input has no accepted SELECT header")
                expected_owner = int(block[4])
                if int(model.statements[statement][4]) != expected_owner:
                    raise ValueError("SELECT header belongs to another procedure")
                if expected_owner != owner and (expected_owner != 0 or int(model.symbols[owner][3]) != 8):
                    raise ValueError("SELECT input belongs to another procedure")
                if model.statement_owners.get(("expression", expression)) != statement or int(model.symbols[storage][3]) != 1:
                    raise ValueError("SELECT input lacks its original expression or selected storage")
                if fields[3] not in ("normal", "constant") or construct in inputs:
                    raise ValueError("Invalid or duplicate SELECT input mode")
                inputs[construct] = owner, module, fields[3]
            elif parts[0] == "select-case-clause" and len(parts) == 2 and len(fields) == 4:
                statement = integer(parts[1], 1)
                construct, ordinal, is_else, count = (integer(field) for field in fields)
                same_module("statement", statement, module)
                if construct < 1 or ordinal < 1 or is_else not in (0, 1) or (count == 0) != bool(is_else):
                    raise ValueError("Invalid SELECT clause bounds")
                if int(model.statements[statement][3]) != construct or int(model.statements[statement][9]) != CASE_TOKEN:
                    raise ValueError("SELECT clause has no accepted CASE statement")
                if statement in clauses:
                    raise ValueError("SELECT clause repeats its statement identity")
                clauses[statement] = owner, module, construct, ordinal, is_else, count
            elif parts[0] == "select-case-alternative" and len(parts) == 3 and len(fields) == 6:
                statement, ordinal = (integer(part, 1) for part in parts[1:])
                construct, kind, operation, first, last, is_last = fields
                construct, operation = integer(construct, 1), integer(operation)
                first, last, is_last = integer(first, 1), integer(last), integer(is_last)
                same_module("expression", first, module)
                if last:
                    same_module("expression", last, module)
                if kind not in ("value", "range", "is") or (kind == "range") != bool(last) or is_last not in (0, 1):
                    raise ValueError("Invalid SELECT alternative shape")
                if not 45 <= operation <= 50 or kind != "is" and operation != 45:
                    raise ValueError("Invalid SELECT comparison operation")
                if model.statement_owners.get(("expression", first)) != statement or last and model.statement_owners.get(("expression", last)) != statement:
                    raise ValueError("SELECT alternative belongs to another statement")
                if (statement, ordinal) in alternatives:
                    raise ValueError("SELECT alternative repeats its statement ordinal")
                alternatives[statement, ordinal] = owner, module, construct, kind, is_last
            elif parts[0] == "select-case-end" and len(parts) == 2 and len(fields) == 2:
                construct = integer(parts[1], 1)
                count, is_else = (integer(field) for field in fields)
                if count < 1 or is_else not in (0, 1):
                    raise ValueError("Invalid SELECT completion receipt")
                if construct in endings:
                    raise ValueError("SELECT completion repeats its construct identity")
                endings[construct] = owner, module, count, is_else
            else:
                raise ValueError("Unknown or malformed SELECT observation")
    if set(inputs) != set(endings):
        raise ValueError("SELECT input or completion coverage is incomplete")
    grouped = {construct: {} for construct in inputs}
    for statement, (owner, module, construct, ordinal, is_else, count) in clauses.items():
        if construct not in inputs or inputs[construct][:2] != (owner, module):
            raise ValueError("SELECT clause has no matching input owner")
        if int(model.statements[statement][4]) != int(constructs[construct][4]):
            raise ValueError("SELECT clause belongs to another procedure")
        ending = endings[construct]
        maximum = 8192 if inputs[construct][2] == "constant" else 1024
        if count > maximum or ordinal > ending[2] or ordinal in grouped[construct]:
            raise ValueError("SELECT clause exceeds its grammar bounds or repeats an ordinal")
        grouped[construct][ordinal] = statement, is_else, count
    for construct, parts in grouped.items():
        owner, module, count, has_else = endings[construct]
        if inputs[construct][:2] != (owner, module) or len(parts) != count:
            raise ValueError("SELECT clause coverage is incomplete")
        previous = int(constructs[construct][3])
        for ordinal in sorted(parts):
            statement, is_else, _ = parts[ordinal]
            if statement <= previous or is_else and ordinal != count:
                raise ValueError("SELECT clause ordering is inconsistent")
            previous = statement
        if sum(is_else for _, is_else, _ in parts.values()) != has_else:
            raise ValueError("SELECT ELSE completion is inconsistent")
    observed_counts = {}
    for (statement, ordinal), (owner, module, construct, kind, is_last) in alternatives.items():
        clause = clauses.get(statement)
        if clause is None or clause[:3] != (owner, module, construct) or ordinal > clause[5] or is_last != (ordinal == clause[5]):
            raise ValueError("SELECT alternative has no matching clause")
        if inputs[construct][2] == "constant" and kind == "is":
            raise ValueError("Constant SELECT cannot have an IS alternative")
        observed_counts[statement] = observed_counts.get(statement, 0) + 1
    for statement, clause in clauses.items():
        if observed_counts.get(statement, 0) != clause[5]:
            raise ValueError("SELECT alternative coverage is incomplete")
    for construct, block in constructs.items():
        if block[5] != "select":
            continue
        header = model.statements.get(int(block[3]))
        if header is None:
            raise ValueError("Accepted SELECT has no original header")
        module = int(header[7])
        if model.capabilities[module].get("select-case-inputs") == "available" and construct not in inputs:
            raise ValueError("Accepted SELECT has no compiler input observation")

# end of semantic_selects.py
