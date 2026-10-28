"""Project: FreeBASIC semantic sidecar
File: semantic_extern.py
Purpose: Validate committed EXTERN selections and native policy restoration.
Responsibilities: Scope/context ownership, fallback distinction and terminal coverage.
This file intentionally does NOT infer emitted names or accept unobserved closures.
"""

from semantic_contexts import EXTERN_OPTION
from semantic_lookup import scalar, text_field

FEATURE = "extern-mangling-contexts"
PREFIX = "extern-mangling"
MODES = {"c": 1, "windows": 2, "windows-ms": 3, "c++": 4, "rtlib": 6}


def validate_extern(model, number, unescape):
    events, summaries, stacks, scopes = {}, {}, {}, {}
    for row in model.records["K"]:
        if not row[3].startswith(PREFIX):
            continue
        module = scalar(row[2], number, 1)
        if row[1] != "module" or model.capabilities.get(module, {}).get(FEATURE) != "available":
            raise ValueError("Unavailable or foreign EXTERN transition")
        if model.bindings_only:
            raise ValueError("Reduced model invents EXTERN context identities")
        module_events = events.setdefault(module, [])
        stack = stacks.setdefault(module, [])
        if row[3] == PREFIX + "-counts":
            fields = row[4].split("\t")
            if module in summaries or len(fields) != 4:
                raise ValueError("Repeated or malformed EXTERN terminal counts")
            summaries[module] = [scalar(value, number, 0, 1000000) for value in fields]
            continue
        parts = row[3].split(":")
        if module in summaries or len(parts) != 2 or parts[0] != PREFIX or scalar(parts[1], number, 1, 1000000) != len(module_events) + 1:
            raise ValueError("EXTERN transitions are late or have identity gaps")
        fields = row[4].split("\t")
        if len(fields) != 20 or fields[0] not in ("begin", "end"):
            raise ValueError("Malformed native EXTERN transition")
        for index in (1, 6, 7):
            scalar(fields[index], number, 1)
        for index in (4, 5):
            scalar(fields[index], number, 0 if model.expressions_only else 1)
        previous, selected = [scalar(value, number, 0, 6) for value in fields[2:4]]
        for index in (8, 9, 16, 19):
            scalar(fields[index], number, 0, 1)
        for index in (12, 13, 14, 15, 17, 18):
            scalar(fields[index], number)
        fields[10] = unescape(fields[10])  # A native string token may contain NUL.
        fields[11] = text_field(fields[11], unescape)
        if len(fields[10].encode("utf-8", "surrogateescape")) > 1024:
            raise ValueError("EXTERN spelling exceeds the native literal buffer")
        construct, entry, committed, statement, source = [int(fields[index]) for index in (1, 4, 5, 6, 7)]
        block = model.constructs.get(construct)
        if (block is None or block[5] != "extern" or int(model.source_contexts[int(block[6])][4]) != module
                or statement not in model.statements or int(model.statements[statement][7]) != module
                or source not in model.source_contexts or int(model.source_contexts[source][4]) != module):
            raise ValueError("EXTERN transition has missing or foreign source/scope ownership")
        for identity, mode in ((entry, previous), (committed, selected)):
            if model.expressions_only:
                if identity:
                    raise ValueError("Expression-only EXTERN transition invents a configuration identity")
                continue
            if model.configurations.get(identity) != module or model.options[identity].get(EXTERN_OPTION) != mode:
                raise ValueError("EXTERN transition contradicts its effective configuration")
        if fields[16] == "1" and (not fields[11] or int(fields[12]) < 1
                                   or (int(fields[14]), int(fields[15])) < (int(fields[12]), int(fields[13]))):
            raise ValueError("EXTERN transition invents a physical source point")
        macro = int(fields[17])
        if macro:
            invocation = model.macro_invocations.get(macro)
            occurrence = model.source_contexts.get(int(invocation[4])) if invocation else None
            if occurrence is None or int(occurrence[4]) != module or fields[16] != "0":
                raise ValueError("EXTERN transition has another macro expansion owner")
        elif fields[18:20] != ["0", "0"]:
            raise ValueError("EXTERN transition invents a macro offset")
        key = module, construct
        if fields[0] == "begin":
            if key in scopes or len(stack) >= 1024 or (stack and int(stack[-1][3]) != previous):
                raise ValueError("EXTERN scope entry is repeated or contradicts its outer policy")
            if int(block[3]) != statement or int(block[7]) != entry or int(block[6]) != source:
                raise ValueError("EXTERN entry contradicts its native construct")
            present, recovered = fields[8] == "1", fields[9] == "1"
            expected = MODES.get(fields[10].lower()) if present else None
            if (not present and fields[10]) or recovered != (expected is None) or selected != (expected if expected is not None else 1):
                raise ValueError("EXTERN fallback or written selection disagrees with the native grammar")
            stack.append(fields)
            scopes[key] = {"begin": fields, "end": None}
        else:
            if not stack or int(stack[-1][1]) != construct or fields[8:11] != ["0", "0", ""]:
                raise ValueError("EXTERN restoration has no matching native scope")
            opening = stack.pop()
            ending = model.construct_endings.get(construct)
            if (previous != int(opening[3]) or selected != int(opening[2]) or ending is None
                    or int(ending[2]) != statement):
                raise ValueError("EXTERN restoration contradicts its native closure")
            scopes[key]["end"] = fields
        module_events.append(fields)
    for module, capabilities in model.capabilities.items():
        if capabilities.get(FEATURE) != "available":
            continue
        module_events = events.get(module, [])
        opened = sum(fields[0] == "begin" for fields in module_events)
        closed = len(module_events) - opened
        stack = stacks.get(module, [])
        if summaries.get(module) != [len(module_events), opened, closed, len(stack)]:
            raise ValueError("EXTERN terminal coverage is incomplete")
        expected = {identity for identity, block in model.constructs.items()
                    if block[5] == "extern" and int(model.source_contexts[int(block[6])][4]) == module}
        if expected != {identity for owner, identity in scopes if owner == module}:
            raise ValueError("EXTERN native construct coverage is incomplete")
        if any((identity in model.construct_endings) != (scopes[module, identity]["end"] is not None) for identity in expected):
            raise ValueError("EXTERN restoration coverage is incomplete")
        if stack and module not in model.recovered_modules:
            raise ValueError("Accepted EXTERN model retains an unfinished scope")
    model.extern_transitions = events
    model.extern_scopes = scopes

# end of semantic_extern.py
