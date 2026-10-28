"""Project: FreeBASIC semantic sidecar
File: semantic_default_types.py
Purpose: Validate actual native DEF table state and committed range changes.
Responsibilities: ABI mapping, native clamping, context history and complete counts.
This file intentionally does NOT infer defaults from declared symbol names.
"""

from semantic_contexts import DEFAULT_FEATURE, DEFAULT_KEYS, default_wire_type
from semantic_lookup import scalar, text_field

PREFIX = "native-default-type"


def validate_default_types(model, number, unescape):
    initial, tables, changes, summaries, schedules = {}, {}, {}, {}, {}
    positions = {int(row[1]): position for position, row in enumerate(model.rows) if row[0] == "CTX"}
    for position, row in enumerate(model.rows):
        if row[0] != "K" or not row[3].startswith(PREFIX):
            continue
        module = scalar(row[2], number, 1)
        if row[1] != "module" or model.capabilities.get(module, {}).get(DEFAULT_FEATURE) != "available" or model.bindings_only:
            raise ValueError("Unavailable or foreign native DEF policy")
        if module in summaries:
            raise ValueError("DEF observation follows its terminal count")
        fields = row[4].split("\t")
        if row[3] == PREFIX + "-initial":
            if module in initial or len(fields) != 66 or fields[:2] != ["65", "95"]:
                raise ValueError("Repeated or malformed initial native DEF table")
            raw = [scalar(value, number, 0, 31) for value in fields[2:34]]
            wire = [scalar(value, number, 0, 31) for value in fields[34:]]
            if wire != [default_wire_type(model, module, value) for value in raw]:
                raise ValueError("Initial DEF table confuses native and wire type codes")
            initial[module] = {"raw": raw, "wire": wire}
            tables[module], changes[module], schedules[module] = raw.copy(), [], []
            continue
        if module not in initial:
            raise ValueError("Native DEF policy has no initial state")
        if row[3] == PREFIX + "-count":
            summaries[module] = scalar(row[4], number, 0, 1000000)
            continue
        parts = row[3].split(":")
        if len(parts) != 2 or parts[0] != PREFIX or scalar(parts[1], number, 1, 1000000) != len(changes[module]) + 1 or len(fields) != 20:
            raise ValueError("DEF range identities have gaps or invalid payloads")
        raw_first, raw_last = [scalar(value, number, -(1 << 63), (1 << 63) - 1) for value in fields[:2]]
        first, last = [scalar(value, number, 65, 95) for value in fields[2:4]]
        # ASC returns native UINTEGER. The original mixed comparison treats
        # negative setter arguments as unsigned and clamps them to underscore.
        normalized = sorted(95 if value < 0 else max(65, min(95, value)) for value in (raw_first, raw_last))
        if [first, last] != normalized:
            raise ValueError("DEF range does not preserve native clamping and reversal")
        native, wire = [scalar(value, number, 0, 31) for value in fields[4:6]]
        if wire != default_wire_type(model, module, native):
            raise ValueError("DEF range confuses native and wire type codes")
        changed = scalar(fields[6], number, 0, 1)
        before, after = [scalar(value, number, 0 if model.expressions_only else 1) for value in fields[7:9]]
        statement, source = [scalar(value, number, 1) for value in fields[9:11]]
        if (statement not in model.statements or int(model.statements[statement][7]) != module
                or source not in model.source_contexts or int(model.source_contexts[source][4]) != module):
            raise ValueError("DEF range has a missing or foreign native source owner")
        fields[11] = text_field(fields[11], unescape)
        for index in (12, 13, 14, 15, 17, 18):
            scalar(fields[index], number)
        for index in (16, 19):
            scalar(fields[index], number, 0, 1)
        if fields[16] == "1" and (not fields[11] or int(fields[12]) < 1 or (int(fields[14]), int(fields[15])) < (int(fields[12]), int(fields[13]))):
            raise ValueError("DEF range invents a physical source point")
        macro = int(fields[17])
        if macro:
            invocation = model.macro_invocations.get(macro)
            occurrence = model.source_contexts.get(int(invocation[4])) if invocation else None
            if occurrence is None or int(occurrence[4]) != module or fields[16] != "0":
                raise ValueError("DEF range has a missing or foreign expansion")
        elif fields[18:20] != ["0", "0"]:
            raise ValueError("DEF range invents an unavailable macro offset")
        previous = tables[module]
        if changed != int(any(previous[index - 65] != native for index in range(first, last + 1))):
            raise ValueError("DEF change flag contradicts its actual previous table")
        updated = previous.copy()
        updated[first - 65:last - 64] = [native] * (last - first + 1)
        if model.expressions_only:
            if before or after:
                raise ValueError("Expression-only DEF change invents a configuration")
        else:
            for context, table in ((before, previous), (after, updated)):
                if model.configurations.get(context) != module or positions[context] >= position:
                    raise ValueError("DEF change has a missing, late or foreign configuration")
                actual = [model.options[context].get(("language-default", key)) for key in DEFAULT_KEYS]
                if actual != [default_wire_type(model, module, value) for value in table]:
                    raise ValueError("DEF context does not preserve its native table")
            if (before != after) != bool(changed) or changed and positions[after] <= positions[before]:
                raise ValueError("DEF change does not preserve context cache invalidation")
            if changed:
                schedules[module].append((positions[after], updated.copy()))
        tables[module] = updated
        changes[module].append(fields)
    for module, capabilities in model.capabilities.items():
        if capabilities.get(DEFAULT_FEATURE) == "available" and (module not in initial or summaries.get(module) != len(changes.get(module, ()))):
            raise ValueError("Native DEF policy coverage is incomplete")
    # Other options can create contexts between DEF declarations. Validate
    # every table at its construction position, including snapshots that no
    # later DEF action directly references.
    for module, start in initial.items():
        schedule, index, table = sorted(schedules[module]), 0, start["raw"]
        contexts = sorted((positions[identity], identity) for identity, owner in model.configurations.items() if owner == module)
        for position, context in contexts:
            while index < len(schedule) and schedule[index][0] <= position:
                table, index = schedule[index][1], index + 1
            actual = [model.options[context].get(("language-default", key)) for key in DEFAULT_KEYS]
            if actual != [default_wire_type(model, module, value) for value in table]:
                raise ValueError("Effective DEF table contradicts native change history")
    model.default_type_initial, model.default_type_changes = initial, changes

# end of semantic_default_types.py
