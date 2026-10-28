"""Project: FreeBASIC semantic sidecar
File: semantic_filters.py
Purpose: Validate actual native symbol-filter decisions and returned identities.
Responsibilities: Candidate order, native access receipts, policy and module closure.
This file intentionally does NOT perform name lookup or overload selection.
"""

from semantic_lookup import scalar, symbol

FEATURE = "native-symbol-filters"
PREFIX = "native-symbol-filter"
REASONS = {"class": {"class"}, "class-and-type": {"class-and-type"},
           "suffix": {"suffix-string", "suffix-type"},
           "default-type": {"default-string", "default-type", "default-unsuffixed"}}


def validate_filters(model, number, unescape, owners):
    states, summaries, policies = {}, {}, {}
    for row in model.records["K"]:
        if not row[3].startswith(PREFIX):
            continue
        module = scalar(row[2], number, 1)
        if row[1] != "module" or model.capabilities.get(module, {}).get(FEATURE) != "available":
            raise ValueError("Unavailable or foreign native symbol filter")
        state = states.setdefault(module, {"events": [], "stack": [], "scopes": {}, "counts": [0] * 6})
        if row[3] == PREFIX + "-policy":
            if module in policies:
                raise ValueError("Repeated native symbol-filter policy")
            fields = row[4].split("\t")
            if len(fields) != 6:
                raise ValueError("Malformed native symbol-filter policy")
            policy = [scalar(value, number, 0, 4294967295) for value in fields]
            if policy[0] != 511 or policy[1] < 1 or policy[1] & (policy[1] - 1) or len(set(policy[2:])) != 4:
                raise ValueError("Invalid native symbol-filter policy")
            policies[module] = policy
            continue
        if row[3] == PREFIX + "-counts":
            if module in summaries:
                raise ValueError("Repeated native symbol-filter summary")
            fields = row[4].split("\t")
            if len(fields) != 6:
                raise ValueError("Malformed native symbol-filter summary")
            summaries[module] = [scalar(value, number, 0, 1000000) for value in fields]
            continue
        if module in summaries:
            raise ValueError("Native symbol filter follows its terminal summary")
        parts = row[3].split(":")
        if len(parts) != 2 or parts[0] != PREFIX or scalar(parts[1], number, 1, 1000000) != len(state["events"]) + 1:
            raise ValueError("Native symbol-filter event identities have gaps")
        fields = row[4].split("\t")
        if len(fields) < 2:
            raise ValueError("Malformed native symbol-filter event")
        event = fields[0]
        identity = scalar(fields[1], number, 1, 1000000)
        stack, scopes, counts = (state[key] for key in ("stack", "scopes", "counts"))
        if event == "begin":
            if len(fields) != 18 or identity != len(scopes) + 1 or len(stack) >= 1024:
                raise ValueError("Malformed native symbol-filter opening")
            if scalar(fields[2], number) != (stack[-1] if stack else 0) or fields[3] not in REASONS:
                raise ValueError("Native symbol filter has another parent or kind")
            scalar(fields[4], number, 0, 17)
            scalar(fields[5], number, 0, 4294967295)
            for index in (6, 7, 8):
                scalar(fields[index], number)
            statement, configuration, source = map(int, fields[6:9])
            if statement and (statement not in model.statements or int(model.statements[statement][7]) != module):
                raise ValueError("Native symbol filter has a foreign statement")
            if ((configuration and model.configurations.get(configuration) != module)
                    or (not configuration and not model.expressions_only)):
                raise ValueError("Native symbol filter has a foreign configuration")
            if source and (source not in model.source_contexts or int(model.source_contexts[source][4]) != module):
                raise ValueError("Native symbol filter has a foreign source occurrence")
            scalar(fields[9], number, 0, 1)
            fields[10:] = symbol(fields[10:], model, module, number, unescape, owners)
            if fields[9] != fields[10]:
                raise ValueError("Native symbol-filter chain has no head symbol")
            if fields[3] in ("suffix", "default-type") and fields[4] != "1":
                raise ValueError("Native variable filter requests another symbol class")
            if fields[3] == "class" and fields[5] != "2147483648":
                raise ValueError("Class-only native filter invents a requested datatype")
            scopes[identity] = {"begin": fields, "candidates": [], "access": {}, "end": None}
            stack.append(identity)
            counts[1] += 1
        else:
            if not stack or identity != stack[-1]:
                raise ValueError("Native symbol filter has no current scope")
            scope = scopes[identity]
            candidates, accesses = scope["candidates"], scope["access"]
            if event == "candidate":
                if len(fields) != 13 or scope["begin"][9] == "0" or (candidates and candidates[-1][3] == "1"):
                    raise ValueError("Native symbol filter continues after its first matching candidate")
                if scalar(fields[2], number, 1, 1000000) != len(candidates) + 1:
                    raise ValueError("Native symbol-filter candidate order is incomplete")
                scalar(fields[3], number, 0, 1)
                if fields[4] not in REASONS[scope["begin"][3]]:
                    raise ValueError("Native symbol filter invents a predicate")
                fields[5:] = symbol(fields[5:], model, module, number, unescape, owners)
                if fields[5] != "1":
                    raise ValueError("Native symbol-filter candidate is absent")
                if not candidates and fields[5:] != scope["begin"][10:]:
                    raise ValueError("Native symbol filter starts with another chain head")
                candidates.append(fields)
                counts[3] += 1
            elif event == "access":
                if (len(fields) != 4 or not candidates or fields[2] != candidates[-1][2]
                        or candidates[-1][3] != "1" or candidates[-1][6] != "1" or int(fields[2]) in accesses):
                    raise ValueError("Native symbol-filter access has no pending variable")
                scalar(fields[3], number, 0, 1)
                accesses[int(fields[2])] = fields[3]
                counts[4] += 1
            elif event == "end":
                if len(fields) != 12 or fields[3] not in ("selected", "denied", "not-found"):
                    raise ValueError("Unknown native symbol-filter outcome")
                selected = scalar(fields[2], number, 0, 1000000)
                fields[4:] = symbol(fields[4:], model, module, number, unescape, owners)
                last = candidates[-1] if candidates else None
                allowed = accesses.get(len(candidates))
                if fields[3] == "selected":
                    if (not last or last[3] != "1" or selected != len(candidates) or fields[4:] != last[5:]
                            or (last[6] == "1" and allowed != "1")):
                        raise ValueError("Native symbol-filter return is not its actual eligible candidate")
                else:
                    if selected or fields[4] != "0":
                        raise ValueError("Unselected native symbol filter invents a result")
                    if fields[3] == "denied":
                        if not last or last[3] != "1" or allowed != "0":
                            raise ValueError("Native symbol filter invents an access denial")
                    elif last and last[3] != "0":
                        raise ValueError("Native symbol-filter absence discards a matching candidate")
                scope["end"] = fields
                stack.pop()
                counts[2] += 1
            else:
                raise ValueError("Unknown native symbol-filter event")
        state["events"].append(fields)
        counts[0] += 1
    for module, capabilities in model.capabilities.items():
        if capabilities.get(FEATURE) != "available":
            continue
        state = states.get(module)
        counts = state["counts"].copy() if state else [0] * 6
        counts[5] = len(state["stack"]) if state else 0
        if summaries.get(module) != counts or counts[1] - counts[2] != counts[5] or module not in policies:
            raise ValueError("Native symbol-filter terminal coverage is incomplete")
        if module not in model.recovered_modules and counts[5]:
            raise ValueError("Accepted module retains open native symbol filters")
        mask, suffix_attribute, *string_types = policies[module]
        for scope in state["scopes"].values():
            expected_class, expected_type = map(int, scope["begin"][4:6])
            for candidate in scope["candidates"]:
                matched, reason, kind, dtype, attrib = (candidate[index] for index in (3, 4, 6, 7, 8))
                kind, dtype, attrib = int(kind), int(dtype), int(attrib)
                if reason.startswith("suffix-"):
                    expected_reason = "suffix-string" if expected_type == string_types[0] else "suffix-type"
                    if reason != expected_reason:
                        raise ValueError("Native suffix filter invents a string/numeric branch")
                if reason in ("default-string", "default-type"):
                    expected_reason = "default-string" if expected_type == string_types[0] else "default-type"
                    if reason != expected_reason:
                        raise ValueError("Native default-type filter invents a string/numeric branch")
                if reason == "class":
                    expected = kind == expected_class
                elif reason == "class-and-type":
                    expected = kind == expected_class and dtype == expected_type
                elif reason == "suffix-string":
                    expected = kind == 1 and dtype & mask in string_types
                elif reason == "suffix-type":
                    expected = kind == 1 and dtype & mask == expected_type
                elif reason == "default-unsuffixed":
                    expected = kind == 1 and not attrib & suffix_attribute
                elif reason == "default-string":
                    expected = kind == 1 and bool(attrib & suffix_attribute) and dtype in string_types
                else:
                    expected = kind == 1 and bool(attrib & suffix_attribute) and dtype & mask == expected_type
                if (matched == "1") != expected:
                    raise ValueError("Native symbol-filter predicate contradicts its copied inputs")
    model.filter_events = {module: state["events"] for module, state in states.items()}
    model.filter_scopes = {(module, identity): scope for module, state in states.items() for identity, scope in state["scopes"].items()}

# end of semantic_filters.py
