"""Project: FreeBASIC semantic sidecar
File: semantic_prototypes.py
Purpose: Validate actual declaration/prototype comparisons.
Responsibilities: Native arity/property gates, reversed parameter stages and identities.
This file intentionally does NOT score overloads or infer executed calls.
"""

from semantic_lookup import scalar, symbol

FEATURE = "native-prototype-comparisons"
PREFIX = "native-prototype"


def validate_prototypes(model, number, unescape, owners):
    states, policies, summaries = {}, {}, {}
    for row in model.records["K"]:
        if not row[3].startswith(PREFIX):
            continue
        module = scalar(row[2], number, 1)
        if row[1] != "module" or model.capabilities.get(module, {}).get(FEATURE) != "available":
            raise ValueError("Unavailable or unowned native prototype comparison")
        state = states.setdefault(module, {"events": [], "stack": [], "scopes": {}, "counts": [0] * 6})
        if row[3] == PREFIX + "-policy":
            if module in policies or module in summaries:
                raise ValueError("Repeated or late native prototype policy")
            policy = row[4].split("\t")
            # Native METHOD, PROPERTY and PROPGET bits are independent of the
            # selected raw datatype ABI and the declaration's spelling.
            if policy != ["2", "32", "1"]:
                raise ValueError("Unknown native prototype method/property policy")
            policies[module] = tuple(map(int, policy))
            continue
        if row[3] == PREFIX + "-counts":
            if module in summaries:
                raise ValueError("Repeated native prototype summary")
            fields = row[4].split("\t")
            if len(fields) != 6:
                raise ValueError("Malformed native prototype summary")
            summaries[module] = [scalar(value, number, 0, 1000000) for value in fields]
            continue
        if module in summaries:
            raise ValueError("Native prototype event follows its terminal summary")
        parts = row[3].split(":")
        if len(parts) != 2 or parts[0] != PREFIX or scalar(parts[1], number, 1, 1000000) != len(state["events"]) + 1:
            raise ValueError("Native prototype event identities have gaps")
        fields = row[4].split("\t")
        if len(fields) < 2:
            raise ValueError("Malformed native prototype event")
        event = fields[0]
        identity = scalar(fields[1], number, 1, 1000000)
        stack, scopes, counts = (state[key] for key in ("stack", "scopes", "counts"))
        if event == "begin":
            if len(fields) != 25 or identity != len(scopes) + 1 or len(stack) >= 1024:
                raise ValueError("Malformed native prototype opening")
            if scalar(fields[2], number) != (stack[-1] if stack else 0):
                raise ValueError("Native prototype has another parent")
            for index in (3, 4, 5, 6):
                scalar(fields[index], number)
            statement, configuration, source = map(int, fields[4:7])
            if statement and (statement not in model.statements or int(model.statements[statement][7]) != module):
                raise ValueError("Native prototype has a foreign statement")
            if ((configuration and model.configurations.get(configuration) != module)
                    or (not configuration and not model.expressions_only)):
                raise ValueError("Native prototype has a foreign configuration")
            if source and (source not in model.source_contexts or int(model.source_contexts[source][4]) != module):
                raise ValueError("Native prototype has a foreign source occurrence")
            fields[7:15] = symbol(fields[7:15], model, module, number, unescape, owners)
            fields[15:23] = symbol(fields[15:23], model, module, number, unescape, owners)
            for offset, index in ((7, 23), (15, 24)):
                count = scalar(fields[index], number, -1, 32767)
                if (fields[offset + 1] == "3") != (count >= 0):
                    raise ValueError("Native prototype reads a nonprocedure parameter union")
            scopes[identity] = {"begin": fields, "candidates": [], "end": None}
            stack.append(identity)
            counts[1] += 1
        else:
            if not stack or identity != stack[-1]:
                raise ValueError("Native prototype event has no current scope")
            scope = scopes[identity]
            candidates = scope["candidates"]
            if event == "candidate":
                if (len(fields) != 15 or scope["begin"][8] != "3" or scope["begin"][16] != "3"
                        or (candidates and candidates[-1]["stage"] != "failed")):
                    raise ValueError("Native prototype candidate follows an unfinished comparison")
                if scalar(fields[2], number, 1, 1000000) != len(candidates) + 1:
                    raise ValueError("Native prototype candidate order is incomplete")
                expected = scalar(fields[3], number, 0, 32767)
                scalar(fields[4], number, 0, 32767)
                effective = scalar(fields[5], number, -1, 32767)
                scalar(fields[6], number, 0, 1)
                fields[7:] = symbol(fields[7:], model, module, number, unescape, owners)
                if fields[8] != "3":
                    raise ValueError("Native prototype candidate is not a procedure")
                stage = "failed" if expected != effective else "matched" if effective == 0 else "bydesc"
                candidates.append({"event": fields, "stage": stage, "remaining": effective, "parameters": []})
                counts[3] += 1
            elif event == "parameter":
                if len(fields) != 42 or not candidates or fields[2] != candidates[-1]["event"][2]:
                    raise ValueError("Native prototype parameter has another candidate")
                candidate = candidates[-1]
                if fields[4] != candidate["stage"] or scalar(fields[3], number, 1, 32767) != candidate["remaining"]:
                    raise ValueError("Native prototype reversed parameter stage is incomplete")
                distinct = scalar(fields[5], number, 0, 1)
                for mode_index, dims_index in ((6, 7), (8, 9)):
                    mode = scalar(fields[mode_index], number, 1, 4)
                    dimensions = scalar(fields[dims_index], number, -1, 8)
                    if mode != 3 and dimensions:
                        raise ValueError("Native prototype invents non-BYDESC dimensions")
                for offset in (10, 18, 26, 34):
                    fields[offset:offset + 8] = symbol(fields[offset:offset + 8], model, module, number, unescape, owners)
                if fields[11] != "4" or fields[19] != "4":
                    raise ValueError("Native prototype comparison lacks formal parameters")
                previous = candidate["parameters"][-1] if candidate["parameters"] else None
                if previous and fields[4] != "bydesc" and fields[6:] != previous[6:]:
                    raise ValueError("Native prototype stages compare different parameter pairs")
                if fields[4] == "bydesc":
                    left, right = int(fields[6]), int(fields[8])
                    expected = ((left == 3) != (right == 3) or
                                left == right == 3 and int(fields[7]) > 0 and int(fields[9]) > 0
                                and fields[7] != fields[9])
                    if bool(distinct) != expected:
                        raise ValueError("Native BYDESC distinction contradicts its copied modes/ranks")
                elif fields[4] == "type":
                    if bool(distinct) != (fields[12] != fields[20]):
                        raise ValueError("Native prototype datatype distinction contradicts its copied types")
                else:
                    left_present, right_present = fields[26] == "1", fields[34] == "1"
                    if not left_present or not right_present:
                        expected = left_present != right_present
                    elif not model.expressions_only:
                        expected = fields[33] != fields[41]
                    else:
                        # Reduced output deliberately lacks canonical nominal
                        # identities. Same-spelled type hints cannot decide
                        # pointer equality; retain the actual native result.
                        expected = bool(distinct)
                    if bool(distinct) != expected:
                        raise ValueError("Native prototype nominal distinction contradicts its identities")
                candidate["parameters"].append(fields)
                if distinct:
                    candidate["stage"] = "failed"
                elif fields[4] == "bydesc":
                    candidate["stage"] = "type"
                elif fields[4] == "type":
                    candidate["stage"] = "subtype"
                else:
                    candidate["remaining"] -= 1
                    candidate["stage"] = "matched" if candidate["remaining"] == 0 else "bydesc"
                counts[4] += 1
            elif event == "end":
                if len(fields) != 12 or fields[3] not in ("selected", "not-found", "invalid-input"):
                    raise ValueError("Unknown native prototype outcome")
                selected = scalar(fields[2], number, 0, 1000000)
                fields[4:] = symbol(fields[4:], model, module, number, unescape, owners)
                if fields[3] == "selected":
                    if (not candidates or candidates[-1]["stage"] != "matched" or selected != len(candidates)
                            or fields[4:] != candidates[-1]["event"][7:]):
                        raise ValueError("Native prototype return has no matched candidate")
                else:
                    if selected or fields[4] != "0":
                        raise ValueError("Unselected native prototype invents a result")
                    if fields[3] == "not-found":
                        if not candidates or candidates[-1]["stage"] != "failed":
                            raise ValueError("Native prototype absence discards an unfinished or matched candidate")
                    elif candidates or scope["begin"][8] == scope["begin"][16] == "3":
                        raise ValueError("Native prototype invents invalid input")
                scope["end"] = fields
                stack.pop()
                counts[2] += 1
            else:
                raise ValueError("Unknown native prototype event")
        state["events"].append(fields)
        counts[0] += 1
    for module, capabilities in model.capabilities.items():
        if capabilities.get(FEATURE) != "available":
            continue
        state = states.get(module)
        counts = state["counts"].copy() if state else [0] * 6
        counts[5] = len(state["stack"]) if state else 0
        if module not in policies or summaries.get(module) != counts or counts[1] - counts[2] != counts[5]:
            raise ValueError("Native prototype terminal coverage is incomplete")
        if module not in model.recovered_modules and counts[5]:
            raise ValueError("Accepted module retains unfinished native prototype comparisons")
        method_bit, property_bit, getter_bit = policies[module]
        for scope in state["scopes"].values():
            begin = scope["begin"]
            query_params = int(begin[24]) - bool(int(begin[19]) & method_bit)
            is_property = bool(int(begin[11]) & property_bit)
            for candidate in scope["candidates"]:
                event = candidate["event"]
                effective = int(event[4]) - bool(int(event[11]) & method_bit)
                if is_property:
                    # VOID is ABI slot0 in both supported raw datatype ABIs.
                    # The native predicate strips const/mangling flags and
                    # retains the nine datatype/pointer bits.
                    returns_void = int(event[9]) & 511 == 0
                    if (int(begin[3]) & getter_bit and returns_void
                            or not int(begin[3]) & getter_bit and not returns_void):
                        effective = -1
                if int(event[3]) != query_params or int(event[5]) != effective or bool(int(event[6])) != is_property:
                    raise ValueError("Native prototype arity/property gate contradicts its copied declaration")
    model.prototype_events = {module: state["events"] for module, state in states.items()}
    model.prototype_scopes = {(module, identity): scope for module, state in states.items() for identity, scope in state["scopes"].items()}

# end of semantic_prototypes.py
