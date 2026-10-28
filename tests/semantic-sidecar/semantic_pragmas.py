"""Project: FreeBASIC semantic sidecar
File: semantic_pragmas.py
Purpose: Validate native pragma stacks, option applications and callback actions.
Responsibilities: Scoped ordering, native policy, source ownership and count closure.
This file intentionally does NOT execute directives or infer reservation success.
"""

from semantic_lookup import scalar, text_field

FEATURE = "native-pragma-history"
PREFIX = "native-pragma"
# Index, group, raw option and flags are the captured native pragma vocabulary.
OPTIONS = (("msbitfields", 1, 35, 27), ("once", 0, 0, 4), ("constness", 3, 128, 27),
           ("reserve", 0, 0, 4), ("gui", 1, 43, 27), ("private", 2, 2, 27), ("profile", 2, 1, 27))
REASONS = {"completed", "unknown-option", "disallowed-stack", "disallowed-assignment"}


def effective(model, context, option):
    options = model.options[context]
    if option == 0:
        return options["compiler", "msbitfields"]
    if option == 2:
        return -int(bool(options["compiler", "pdcheckopt"] & 128))
    if option == 4:
        return options["compiler", "modeview"]
    if option == 5:
        return -int(options["language-default", "procpublic"] == 0)
    if option == 6:
        return -int(options["language-default", "procprofile"] != 0)
    raise ValueError("Callback pragma invents an effective scalar value")


def validate_pragmas(model, number, unescape, owners):
    events, summaries, states, scopes = {}, {}, {}, {}
    def context(value, module):
        identity = scalar(value, number, 0 if model.expressions_only else 1)
        if model.expressions_only:
            if identity:
                raise ValueError("Expression pragma invents a configuration")
        elif model.configurations.get(identity) != module:
            raise ValueError("Pragma has a missing or foreign configuration")
        return identity
    for row in model.records["K"]:
        if not row[3].startswith(PREFIX):
            continue
        module = scalar(row[2], number, 1)
        if row[1] != "module" or model.capabilities.get(module, {}).get(FEATURE) != "available" or model.bindings_only:
            raise ValueError("Unavailable or foreign native pragma observation")
        state = states.setdefault(module, {"stack": [], "options": [[] for _ in OPTIONS], "begins": 0, "ends": 0})
        module_events = events.setdefault(module, [])
        if row[3] == PREFIX + "-counts":
            fields = row[4].split("\t")
            if module in summaries or len(fields) != 5:
                raise ValueError("Repeated or malformed pragma summary")
            summaries[module] = [scalar(value, number, 0, 1000000) for value in fields]
            continue
        parts = row[3].split(":")
        if module in summaries or len(parts) != 2 or parts[0] != PREFIX or scalar(parts[1], number, 1, 1000000) != len(module_events) + 1:
            raise ValueError("Pragma observations are late or have ordinal gaps")
        fields = row[4].split("\t")
        lengths = {"begin": 16, "select": 8, "unknown": 3, "push": 7, "pop": 7, "apply": 9, "callback": 4, "reserve": 6, "end": 6}
        kind = fields[0]
        if kind not in lengths or len(fields) != lengths[kind]:
            raise ValueError("Unknown or malformed pragma event")
        identity = scalar(fields[1], number, 1, 1000000)
        if kind == "begin":
            state["begins"] += 1
            parent = scalar(fields[2], number)
            if identity != state["begins"] or len(state["stack"]) >= 1024 or parent != (state["stack"][-1]["id"] if state["stack"] else 0):
                raise ValueError("Pragma scopes are not densely nested")
            fields[3], fields[7] = text_field(fields[3], unescape), text_field(fields[7], unescape)
            statement, source = scalar(fields[4], number), scalar(fields[6], number, 1)
            context(fields[5], module)
            if statement and (statement not in model.statements or int(model.statements[statement][7]) != module):
                raise ValueError("Pragma has another statement owner")
            if source not in model.source_contexts or int(model.source_contexts[source][4]) != module:
                raise ValueError("Pragma has another source owner")
            for index in (8, 9, 10, 11, 13, 14):
                scalar(fields[index], number)
            for index in (12, 15):
                scalar(fields[index], number, 0, 1)
            if fields[12] == "1" and (not fields[7] or int(fields[8]) < 1 or (int(fields[10]), int(fields[11])) < (int(fields[8]), int(fields[9]))):
                raise ValueError("Pragma invents a physical source point")
            if int(fields[13]):
                invocation = model.macro_invocations.get(int(fields[13]))
                origin = model.source_contexts.get(int(invocation[4])) if invocation else None
                if origin is None or int(origin[4]) != module or fields[12] != "0":
                    raise ValueError("Pragma has a missing or foreign expansion")
            elif fields[14:16] != ["0", "0"]:
                raise ValueError("Pragma invents an unavailable macro offset")
            scope = {"id": identity, "begin": fields, "selection": None, "stack": None, "apply": None, "callback": None, "unknown": None, "reservations": [], "end": None}
            state["stack"].append(scope)
            scopes[module, identity] = scope
        else:
            if not state["stack"] or identity != state["stack"][-1]["id"]:
                raise ValueError("Pragma action has no active native scope")
            scope = state["stack"][-1]
            selected = scope["selection"]
            option = int(selected[2]) if selected is not None else None
            if kind == "select":
                p, group, code, flags = [scalar(value, number) for value in fields[2:6]]
                fields[7] = text_field(fields[7], unescape)
                if p >= len(OPTIONS) or (fields[7], group, code, flags) != OPTIONS[p] or fields[6] not in ("set", "push", "pop") or selected is not None or scope["unknown"] is not None:
                    raise ValueError("Pragma selection contradicts native policy")
                expected = scope["begin"][3]
                if fields[6] != (expected if expected in ("push", "pop") else "set"):
                    raise ValueError("Pragma selection has another native route")
                if expected not in ("push", "pop") and fields[7] != expected:
                    raise ValueError("Pragma selection differs from its actual entry spelling")
                scope["selection"] = fields
            elif kind == "unknown":
                fields[2] = text_field(fields[2], unescape)
                if selected is not None or scope["unknown"] is not None or fields[2] in {item[0] for item in OPTIONS}:
                    raise ValueError("Unknown pragma invents or loses a native selection")
                if scope["begin"][3] not in ("push", "pop") and fields[2] != scope["begin"][3]:
                    raise ValueError("Unknown pragma differs from its actual entry spelling")
                scope["unknown"] = fields[2]
            elif kind in ("push", "pop"):
                p, before, after, accepted = [scalar(value, number, 0, 8) for value in fields[2:6]]
                value = scalar(fields[6], number, -(1 << 63), (1 << 63) - 1)
                if selected is None or option != p or selected[6] != kind or not int(selected[5]) & 1 or scope["stack"] is not None or accepted not in (0, 1):
                    raise ValueError("Pragma stack action has another selection")
                stack = state["options"][p]
                expected = len(stack) < 8 if kind == "push" else bool(stack)
                if before != len(stack) or accepted != int(expected):
                    raise ValueError("Pragma stack result contradicts native depth")
                if kind == "push" and expected:
                    stack.append(value)
                elif kind == "pop":
                    expected_value = stack.pop() if expected else 0
                    if value != expected_value:
                        raise ValueError("Pragma pop invents a saved value or underflow result")
                if after != len(stack):
                    raise ValueError("Pragma stack result has another resulting depth")
                scope["stack"] = fields
            elif kind == "apply":
                p = scalar(fields[2], number, 0, 6)
                requested, supplied, before, after = [scalar(value, number, -(1 << 63), (1 << 63) - 1) for value in fields[3:7]]
                entry, committed = context(fields[7], module), context(fields[8], module)
                if option != p or p in (1, 3) or scope["apply"] is not None or selected[6] in ("push", "pop") and scope["stack"] is None:
                    raise ValueError("Pragma application has no matching scalar route")
                if selected[6] == "pop" and requested != int(scope["stack"][6]):
                    raise ValueError("Pragma application loses its native pop result")
                if requested not in (-1, 0, 1):
                    raise ValueError("Boolean pragma invents an unnormalized value")
                if p != 2 and supplied != requested:
                    raise ValueError("Pragma setter argument differs from its native value")
                expected = int(requested != 0) if p == 4 else -int(requested != 0)
                if p != 4 and after != expected or p == 4 and after not in (before, expected):
                    raise ValueError("Pragma effect contradicts its native setter")
                if entry:
                    if effective(model, entry, p) != before or effective(model, committed, p) != after:
                        raise ValueError("Pragma application contradicts its effective configurations")
                    if p == 2 and supplied != ((requested & 128) | (model.options[entry]["compiler", "pdcheckopt"] & ~128)):
                        raise ValueError("Constness pragma loses unrelated pedantic bits")
                scope["apply"] = fields
            elif kind == "callback":
                if option not in (1, 3) or fields[2:4] != [str(option), OPTIONS[option][0]] or scope["callback"] is not None:
                    raise ValueError("Pragma callback has another selection")
                scope["callback"] = fields
            elif kind == "reserve":
                fields[3] = text_field(fields[3], unescape)
                accepted = scalar(fields[4], number, 0, 1)
                symbol = scalar(fields[5], number)
                if option != 3 or scope["callback"] is None or fields[2] not in ("asm", "extern", "symbol") or not fields[3]:
                    raise ValueError("Pragma reservation has no native callback")
                if fields[2] == "symbol" and accepted and not model.expressions_only:
                    target = model.symbols.get(symbol)
                    if target is None or int(target[3]) != 16 or owners.get(("symbol", symbol)) != module:
                        raise ValueError("Pragma reservation has a missing or foreign reserved symbol")
                elif symbol:
                    raise ValueError("Pragma reservation invents an unavailable symbol")
                scope["reservations"].append(fields)
            elif kind == "end":
                if fields[2] not in REASONS:
                    raise ValueError("Unknown pragma completion reason")
                scalar(fields[3], number, 0, 2147483647)
                scalar(fields[4], number, 0, 2147483647)
                context(fields[5], module)
                reason = fields[2]
                if reason == "unknown-option":
                    valid = selected is None and scope["unknown"] is not None
                elif reason == "disallowed-stack":
                    valid = selected is not None and selected[6] in ("push", "pop") and not int(selected[5]) & 1
                elif reason == "disallowed-assignment":
                    valid = selected is not None and selected[6] == "set" and not int(selected[5]) & 2
                else:
                    valid = selected is not None and (scope["callback"] is not None if option in (1, 3) else scope["apply"] is not None)
                    if option == 3:
                        reservations = scope["reservations"]
                        roles = [item[2] for item in reservations]
                        if roles not in ([], ["asm"], ["extern"], ["asm", "extern"], ["symbol"]):
                            raise ValueError("Pragma reservation actions have an impossible native order")
                        if reservations and (len({item[3] for item in reservations}) != 1 or any(item[4] == "0" for item in reservations[:-1])):
                            raise ValueError("Pragma reservation continues after native failure")
                        if module not in model.recovered_modules and fields[3] == fields[4] and not reservations:
                            raise ValueError("Successful reserve callback has no actual reservation action")
                if not valid or reason != "completed" and any(scope[key] is not None for key in ("stack", "apply", "callback")):
                    raise ValueError("Pragma completion loses or fabricates a native action")
                scope["end"] = fields
                state["stack"].pop()
                state["ends"] += 1
        module_events.append(fields)
    for module, capabilities in model.capabilities.items():
        if capabilities.get(FEATURE) != "available":
            continue
        state = states.get(module, {"stack": [], "begins": 0, "ends": 0})
        if summaries.get(module) != [len(events.get(module, ())), state["begins"], state["ends"], len(state["stack"]), 8]:
            raise ValueError("Native pragma coverage is incomplete")
        if state["stack"] and module not in model.recovered_modules:
            raise ValueError("Accepted pragma output retains an unfinished scope")
    model.pragma_events, model.pragma_scopes = events, scopes

# end of semantic_pragmas.py
