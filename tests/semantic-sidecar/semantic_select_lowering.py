"""Project: FreeBASIC semantic sidecar tests
File: semantic_select_lowering.py
Purpose: Validate compiler-selected CASE comparisons and constant tables.
Responsibilities: Complete lowering receipts, operand ownership and bounded slots.
This file intentionally does NOT reproduce coercion or parse BASIC source.
"""

U64_MASK = (1 << 64) - 1
TABLE_SLOTS = 8192
INVERSE_RELATIONS = {45: 48, 48: 45, 46: 50, 50: 46, 47: 49, 49: 47}


def validate_select_lowering(model, inputs, clauses, alternatives, observations,
                             integer, number, same_module, work_left):
    comparisons, constants, tables = {}, {}, {}
    expressions = {int(row[1]): row for row in model.records["E"]}

    def unsigned(text):
        if len(text) > 20:
            raise ValueError("CASE unsigned value exceeds 64 bits")
        value = number(text, 0)
        if value > U64_MASK:
            raise ValueError("CASE unsigned value exceeds 64 bits")
        return value

    def dtype(text, module):
        value = integer(text, 1)
        if value & 31 not in model.primitives[module - 1]:
            raise ValueError("CASE lowering has no selected primitive type")
        return value

    def group(construct, owner, module, constant):
        if inputs.get(construct) != (owner, module, "constant" if constant else "normal"):
            raise ValueError("CASE lowering has no matching SELECT owner or mode")

    def route_type(kind, selected_type, module):
        # FBC's Y codes distinguish narrow/fixed text, wide units and UTF-8
        # descriptors. Pointer depth is independent of that primitive code.
        base = selected_type & 31
        if kind == "scalar":
            return selected_type & 0x1e0 or model.primitives[module - 1][base][3] in ("integer", "float")
        if selected_type & 0x1e0:
            return False
        if kind == "unicode":
            return base == 26
        return base in ((4, 7, 17, 18) if kind == "wide" else (4, 17, 18))

    for owner, module, parts, fields in observations:
        work_left -= 1
        if work_left < 0 or model.capabilities[module].get("select-case-lowering-inputs") != "available":
            raise ValueError("CASE lowering lacks an available capability or work budget")
        if parts[0] == "select-case-table" and len(parts) == 2 and len(fields) == 5:
            construct = integer(parts[1], 1)
            group(construct, owner, module, True)
            converted, stored = (dtype(field, module) for field in fields[:2])
            bias, span = (unsigned(field) for field in fields[2:4])
            count = integer(fields[4])
            if converted > 31 or model.primitives[module - 1][converted][3] != "integer" or \
                    stored & 511 not in (9, 12, 14) or span >= TABLE_SLOTS or count > TABLE_SLOTS:
                raise ValueError("Invalid selected CASE table types or bounds")
            header = model.properties["symbol", owner][f"select-case-input:{construct}"].split("\t")
            if int(model.symbols[int(header[2])][4]) != stored or construct in tables:
                raise ValueError("CASE table changes its selected storage or repeats its construct")
            tables[construct] = converted, bias, span, count
        elif parts[0] in ("select-case-comparison", "select-case-constant"):
            expected_parts, expected_fields = (4, 8) if parts[0] == "select-case-comparison" else (3, 5)
            if len(parts) != expected_parts or len(fields) != expected_fields:
                raise ValueError("Malformed selected CASE alternative")
            statement, ordinal = (integer(part, 1) for part in parts[1:3])
            construct = integer(fields[0], 1)
            alternative = alternatives.get((statement, ordinal))
            if alternative is None or alternative[:3] != (owner, module, construct):
                raise ValueError("CASE lowering has no matching original alternative")
            if parts[0] == "select-case-constant":
                group(construct, owner, module, True)
                converted = dtype(fields[1], module)
                if converted > 31 or model.primitives[module - 1][converted][3] != "integer":
                    raise ValueError("Invalid selected CASE constant type")
                if (statement, ordinal) in constants:
                    raise ValueError("CASE constant repeats its statement ordinal")
                constants[statement, ordinal] = converted, *(unsigned(field) for field in fields[2:])
            else:
                group(construct, owner, module, False)
                bound = integer(parts[3], 1)
                original = model.properties["symbol", owner][f"select-case-alternative:{statement}:{ordinal}"].split("\t")
                kind, is_last = alternative[3:]
                if kind == "range":
                    expected = (47, "fallthrough") if bound == 1 else (46, "fallthrough") if is_last else (50, "jump")
                    maximum = 2
                else:
                    expected = (INVERSE_RELATIONS[int(original[2])], "fallthrough") if is_last else (int(original[2]), "jump")
                    maximum = 1
                if bound > maximum or (integer(fields[1]), fields[2]) != expected:
                    raise ValueError("CASE comparison changes the actual branch direction")
                if fields[3] == "unclassified":
                    if fields[4:] != ["0"] * 4:
                        raise ValueError("Unclassified CASE comparison claims selected operands")
                elif fields[3] in ("scalar", "narrow", "wide", "unicode"):
                    selected_types = []
                    for type_text, expression_text in zip(fields[4:6], fields[6:8]):
                        selected_type = dtype(type_text, module)
                        if not route_type(fields[3], selected_type, module):
                            raise ValueError("Selected CASE operand type changes its comparison route")
                        selected_types.append(selected_type & 31)
                        expression = integer(expression_text, 1)
                        same_module("expression", expression, module)
                        if model.statement_owners.get(("expression", expression)) != statement or expressions[expression][2] != "0":
                            raise ValueError("Selected CASE operand lacks its logical statement owner")
                    if fields[3] == "wide" and 7 not in selected_types:
                        raise ValueError("Wide CASE comparison has no selected wide operand")
                else:
                    raise ValueError("Invalid selected CASE comparison kind")
                if (statement, ordinal, bound) in comparisons:
                    raise ValueError("CASE comparison repeats its bound ordinal")
                comparisons[statement, ordinal, bound] = fields
        else:
            raise ValueError("Malformed CASE lowering observation")

    occupied, initial_bias = {}, {}
    for construct, (owner, module, mode) in inputs.items():
        if model.capabilities[module].get("select-case-lowering-inputs") != "available":
            continue
        if mode == "constant":
            if construct not in tables:
                raise ValueError("Constant SELECT has no selected table receipt")
            occupied[construct] = set()

    for (statement, ordinal), (owner, module, construct, kind, _) in alternatives.items():
        if model.capabilities[module].get("select-case-lowering-inputs") != "available":
            continue
        if inputs[construct][2] == "normal":
            if any((statement, ordinal, bound) not in comparisons for bound in range(1, 3 if kind == "range" else 2)):
                raise ValueError("CASE comparison coverage is incomplete")
            continue
        selected = constants.get((statement, ordinal))
        if selected is None or selected[0] != tables[construct][0]:
            raise ValueError("CASE constant coverage or conversion type is inconsistent")
        converted, first, last, bias = selected
        if construct in initial_bias and initial_bias[construct] != bias:
            raise ValueError("CASE constant changes the parser's initial bias")
        initial_bias[construct] = bias
        if clauses[statement][3] == 1 and ordinal == 1 and bias != (first - TABLE_SLOTS) & U64_MASK:
            raise ValueError("CASE constant has an inconsistent initial bias")
        lower, upper = (first - bias) & U64_MASK, (last - bias) & U64_MASK
        if lower > upper or upper >= TABLE_SLOTS * 2 or kind != "range" and first != last:
            raise ValueError("CASE constant has an invalid parser range")
        final_bias, span, count = tables[construct][1:]
        lower, upper = (first - final_bias) & U64_MASK, (last - final_bias) & U64_MASK
        if lower > upper or upper > span:
            raise ValueError("CASE constant lies outside its selected table")
        work_left -= upper - lower + 1
        if work_left < 0:
            raise ValueError("CASE table validation work budget exceeded")
        slots = occupied[construct]
        for slot in range(lower, upper + 1):
            if slot in slots:
                raise ValueError("CASE table repeats a selected slot")
            slots.add(slot)
    for construct, slots in occupied.items():
        _, bias, span, count = tables[construct]
        if len(slots) != count or slots and (min(slots) != 0 or max(slots) != span) or not slots and (bias or span):
            raise ValueError("CASE table completion is inconsistent")
    for capabilities in model.capabilities.values():
        if capabilities.get("select-case-lowering-inputs") == "available" and capabilities.get("select-case-inputs") != "available":
            raise ValueError("CASE lowering has no available original-input capability")

# end of semantic_select_lowering.py
