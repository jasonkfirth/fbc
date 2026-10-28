"""Project: FreeBASIC semantic sidecar
File: semantic_resolution.py
Purpose: Validate observed overload attempts without resolving source again.
Responsibilities: Nested scopes, candidate scores, ranking receipts and access checks.
This file intentionally does NOT classify a considered candidate as an executed call.
"""

FEATURE = "native-overload-explanations"
PREFIX = "native-resolution"
ARGUMENT_REASONS = {
    "optional-default", "required-omission", "descriptor-mode", "descriptor-type",
    "descriptor-rank", "descriptor-match", "byref-descriptor", "byref-any",
    "byref-byval-width", "invalid-mode", "argument-descriptor", "same-type",
    "compatible-const", "incompatible-const", "ctor-conversion", "cast-conversion",
    "udt-no-conversion", "enum-mismatch", "type-difference", "trailing-optional", "trailing-required",
}
CAST_REASONS = {"cast-exact", "cast-const", "cast-pointer-subtype", "cast-explicit-mismatch",
                "cast-nominal-result", "cast-nominal-target", "cast-type-difference"}


# -------------------------------------------------------------------------
# Copied payloads and context ownership
# -------------------------------------------------------------------------

def scalar(text, number, minimum=0, maximum=2147483647):
    value = number(text, minimum)
    if str(value) != text or value > maximum:
        raise ValueError("Noncanonical or oversized native resolution scalar")
    return value


def text_field(text, unescape):
    value = unescape(text)
    if "\0" in value or len(value.encode("utf-8", errors="surrogateescape")) > 65536:
        raise ValueError("Invalid native resolution name")
    return value


def snapshot(fields, number, unescape, procedure=False):
    if len(fields) != 8:
        raise ValueError("Malformed native resolution symbol snapshot")
    fields = fields.copy()
    present = scalar(fields[0], number, 0, 1)
    kind = scalar(fields[1], number, 0, 17)
    scalar(fields[2], number, 0, 4294967295)
    for value in fields[3:5]:
        scalar(value, number, -9223372036854775808, 18446744073709551615)
    params = scalar(fields[5], number, -1)
    fields[6:] = [text_field(value, unescape) for value in fields[6:]]
    if not present:
        if fields[:6] != ["0", "0", "2147483648", "0", "0", "-1"] or any(fields[6:]):
            raise ValueError("Absent resolution symbol invents a payload")
    elif not kind or (procedure and kind != 3) or (kind == 3 and params < 0) or (kind != 3 and params != -1):
        raise ValueError("Resolution symbol has an impossible native class or parameter count")
    return fields


def context(model, module, statement, configuration, number):
    statement, configuration = scalar(statement, number), scalar(configuration, number)
    if statement and (statement not in model.statements or int(model.statements[statement][7]) != module):
        raise ValueError("Resolution observation has a foreign statement")
    if ((configuration and model.configurations.get(configuration) != module)
            or (not configuration and not model.expressions_only)):
        raise ValueError("Resolution observation has a foreign configuration")


def point(fields, model, module, number, unescape):
    if len(fields) != 7:
        raise ValueError("Malformed native resolution source point")
    fields = fields.copy()
    fields[0] = text_field(fields[0], unescape)
    values = [scalar(value, number) for value in fields[1:]]
    source, line, column, raw_line, raw_column, physical = values
    if source and (source not in model.source_contexts or int(model.source_contexts[source][4]) != module):
        raise ValueError("Resolution observation has a foreign source point")
    if physical not in (0, 1) or (physical and (not source or not fields[0] or raw_line < 1)):
        raise ValueError("Invalid native resolution physical point")
    if not physical and (raw_line or raw_column):
        raise ValueError("Nonphysical resolution observation invents physical coordinates")
    return fields


# -------------------------------------------------------------------------
# Ordered native event lifetimes
# -------------------------------------------------------------------------

def validate_resolution(model, number, unescape, node_kinds):
    states, summaries, policies = {}, {}, {}
    for row in model.records["K"]:
        if row[3] != PREFIX + "-policy":
            continue
        module = scalar(row[2], number, 1)
        if row[1] != "module" or model.capabilities.get(module, {}).get(FEATURE) != "available" or module in policies:
            raise ValueError("Unavailable or repeated native resolution policy")
        values = row[4].split("\t")
        if len(values) != 5:
            raise ValueError("Malformed native resolution policy")
        values = [scalar(value, number, 1, 1000000) for value in values]
        scale, type_match, full_match, depth, events = values
        if type_match % (scale * 4) or full_match * 4 != type_match * 5 or type_match // (scale * 4) > 32 or depth > 1024:
            raise ValueError("Contradictory native resolution score policy")
        if model.primitives[module - 1] and len(model.primitives[module - 1]) != type_match // (scale * 4):
            raise ValueError("Native resolution policy disagrees with the primitive vocabulary")
        policies[module] = values
    for row in model.records["K"]:
        if not row[3].startswith(PREFIX):
            continue
        if row[3].startswith(("native-resolution-context", "native-resolution-use", "native-resolution-bound-use", "native-resolution-macro-use")):
            continue
        module = scalar(row[2], number, 1)
        if row[1] != "module" or model.capabilities.get(module, {}).get(FEATURE) != "available":
            raise ValueError("Unavailable or unowned native resolution explanation")
        if module not in policies:
            raise ValueError("Missing native resolution score policy")
        if row[3] == PREFIX + "-policy":
            continue
        state = states.setdefault(module, {"events": [], "stack": [], "scopes": {}, "candidates": {},
                                           "counts": [0] * 9, "access": []})
        if row[3] == PREFIX + "-counts":
            if module in summaries:
                raise ValueError("Repeated native resolution summary")
            fields = row[4].split("\t")
            if len(fields) != 9:
                raise ValueError("Malformed native resolution summary")
            summaries[module] = [scalar(value, number, 0, 1000000) for value in fields]
            continue
        parts = row[3].split(":")
        if (len(parts) != 2 or parts[0] != PREFIX
                or scalar(parts[1], number, 1, 1000000) != len(state["events"]) + 1):
            raise ValueError("Native resolution event identities are not dense")
        fields = row[4].split("\t")
        event = fields[0]
        stack, scopes, candidates, counts = (state[key] for key in ("stack", "scopes", "candidates", "counts"))
        scope_id = scalar(fields[1], number, 0, 1000000) if len(fields) > 1 else 0
        frame = scopes.get(scope_id)
        if event == "begin":
            if len(fields) != 26 or scope_id != len(scopes) + 1 or len(stack) >= policies[module][3]:
                raise ValueError("Malformed native resolution opening")
            parent, parent_candidate = map(lambda value: scalar(value, number), fields[2:4])
            if parent != (stack[-1] if stack else 0) or parent_candidate != (scopes[parent]["active"] if parent else 0):
                raise ValueError("Resolution nesting invents a parent or considered candidate")
            if fields[4] not in ("closest", "cast", "cast-precision"):
                raise ValueError("Unknown native resolver kind")
            scalar(fields[5], number)
            scalar(fields[6], number, 0, 4294967295)
            scalar(fields[7], number, 0, 4294967295)
            context(model, module, fields[8], fields[9], number)
            source = scalar(fields[10], number)
            if source and (source not in model.source_contexts or int(model.source_contexts[source][4]) != module):
                raise ValueError("Resolution opening has a foreign current source")
            fields[11:18] = point(fields[11:18], model, module, number, unescape)
            fields[18:26] = snapshot(fields[18:26], number, unescape, procedure=True)
            frame = {"begin": fields, "active": 0, "candidates": [], "pending_rank": 0,
                     "maximum": 0, "ties": 0, "winner": 0, "end": None, "access": []}
            scopes[scope_id] = frame
            stack.append(scope_id)
            counts[1] += 1
        elif event == "access":
            if len(fields) != 22 or fields[3] not in ("member", "type-chain"):
                raise ValueError("Malformed native access check")
            expected_scope = stack[-1] if stack else 0
            if scope_id != expected_scope or scalar(fields[2], number) != (scopes[scope_id]["active"] if scope_id else 0):
                raise ValueError("Native access check has a fabricated resolver owner")
            scalar(fields[4], number, 0, 1)
            context(model, module, fields[5], fields[6], number)
            fields[7:14] = point(fields[7:14], model, module, number, unescape)
            fields[14:] = snapshot(fields[14:], number, unescape)
            if fields[14] != "1":
                raise ValueError("Native access check has no symbol")
            if frame is not None:
                frame["access"].append(fields)
            state["access"].append(fields)
            counts[7] += 1
        else:
            if not stack or scope_id != stack[-1]:
                raise ValueError("Native resolution event has no active scope")
            if event == "candidate":
                if len(fields) != 13 or frame["active"] or frame["pending_rank"]:
                    raise ValueError("Overlapping native resolution candidates")
                identity = scalar(fields[2], number, 1, 1000000)
                ordinal = scalar(fields[3], number, 1, 1000000)
                allowed = {"closest": ("screen", "confirm"), "cast": ("cast",), "cast-precision": ("precision",)}[frame["begin"][4]]
                if identity != len(candidates) + 1 or ordinal != len(frame["candidates"]) + 1 or fields[4] not in allowed:
                    raise ValueError("Candidate order or native pass is invalid")
                if frame["begin"][18] != "1":
                    raise ValueError("Headless resolver invents considered candidates")
                fields[5:] = snapshot(fields[5:], number, unescape, procedure=True)
                if fields[5] != "1":
                    raise ValueError("Native resolver considered an absent procedure")
                candidates[identity] = {"begin": fields, "arguments": [], "score": None, "rank": None}
                frame["active"] = identity
                frame["candidates"].append(identity)
                counts[3] += 1
            elif event in ("argument", "score", "rank"):
                identity = scalar(fields[2], number, 1, 1000000) if len(fields) > 2 else 0
                candidate = candidates.get(identity)
                if candidate is None or int(candidate["begin"][1]) != scope_id:
                    raise ValueError("Resolution event has a foreign candidate")
                if event == "argument":
                    validate_argument(fields, candidate, frame, identity, number, unescape, node_kinds, policies[module])
                    counts[5] += 1
                elif event == "score":
                    validate_score(fields, candidate, frame, identity, number, policies[module])
                    frame["active"] = 0
                    if candidate["begin"][4] != "confirm":
                        frame["pending_rank"] = identity
                    counts[4] += 1
                else:
                    validate_rank(fields, candidate, frame, identity, number)
                    frame["pending_rank"] = 0
                    counts[6] += 1
            elif event == "end":
                validate_end(fields, frame, candidates, number)
                stack.pop()
                counts[2] += 1
            else:
                raise ValueError("Unknown native resolution event kind")
        state["events"].append(fields)
        counts[0] += 1
    for module, capabilities in model.capabilities.items():
        if capabilities.get(FEATURE) != "available":
            continue
        state = states.get(module)
        counts = state["counts"].copy() if state else [0] * 9
        counts[8] = len(state["stack"]) if state else 0
        if module not in policies or summaries.get(module) != counts or counts[1] - counts[2] != counts[8]:
            raise ValueError("Native resolution coverage or terminal stack is incomplete")
        if module not in model.recovered_modules and (counts[8] or counts[3] != counts[4]):
            raise ValueError("Accepted module retains unfinished native resolution")
    model.resolution_policy = policies
    model.resolution_events = {module: state["events"] for module, state in states.items()}
    model.resolution_scopes = {(module, identity): frame for module, state in states.items() for identity, frame in state["scopes"].items()}
    model.resolution_candidates = {(module, identity): candidate for module, state in states.items() for identity, candidate in state["candidates"].items()}
    model.resolution_accesses = {module: state["access"] for module, state in states.items()}


# -------------------------------------------------------------------------
# Candidate scoring, ranking and returned choices
# -------------------------------------------------------------------------

def validate_argument(fields, candidate, frame, identity, number, unescape, node_kinds, policy):
    if len(fields) != 17 or frame["active"] != identity or fields[5] not in ARGUMENT_REASONS:
        raise ValueError("Malformed or inactive native parameter check")
    for index in (3, 4, 6, 7, 8, 9, 10, 12, 13, 14):
        scalar(fields[index], number, -1 if index in (4, 6, 9) else 0,
               4294967295 if index in (10, 14) else 9223372036854775807 if index == 4 else 2147483647)
    if fields[6] not in ("-1", "1", "2", "3", "4") or fields[7] not in ("1", "2", "3", "4"):
        raise ValueError("Unknown native argument or parameter mode")
    if fields[8] not in ("0", "1") or fields[12] not in ("0", "1") or int(fields[13]) >= len(node_kinds):
        raise ValueError("Invalid resolution parameter flag or AST class")
    for index in (11, 15, 16):
        fields[index] = text_field(fields[index], unescape)
    if fields[5].startswith("descriptor-"):
        if fields[7] != "3" or fields[12] != "1":
            raise ValueError("Descriptor explanation has another parameter or no argument")
        if fields[5] == "descriptor-mode":
            if fields[6] == "3":
                raise ValueError("Descriptor mode rejection contradicts its actual mode")
        elif fields[6] != "3":
            raise ValueError("Descriptor matching omitted its native passing mode")
        if fields[5] in ("descriptor-match", "descriptor-rank") and int(fields[13]) not in (17, 19, 25):
            raise ValueError("Descriptor matching reads an impossible AST shape")
    if fields[7] != "3" and fields[9] != "0":
        raise ValueError("Nondescriptor parameter reads an inactive dimension payload")
    if fields[12] == "0" and (fields[13:15] != ["0", "2147483648"] or fields[15]):
        raise ValueError("Absent resolution argument invents a root")
    arguments = candidate["arguments"]
    ordinal = int(fields[3])
    if ordinal < 1 or (arguments and ordinal <= int(arguments[-1][3])):
        raise ValueError("Native parameter checks are repeated or out of order")
    trailing = fields[5].startswith("trailing-")
    if trailing != (fields[4] == "-1") or (trailing and (fields[12] != "0" or fields[6] != "-1")):
        raise ValueError("Unevaluated trailing parameter invents an argument score")
    if fields[5] in ("optional-default", "required-omission", "trailing-optional", "trailing-required"):
        optional = fields[5] in ("optional-default", "trailing-optional")
        if fields[12] != "0" or fields[8] != str(int(optional)):
            raise ValueError("Resolution omission contradicts the native optional flag")
    if fields[5] == "optional-default" and int(fields[4]) != policy[2]:
        raise ValueError("Optional omission has an impossible native full-match score")
    if fields[5] == "required-omission" and fields[4] != "0":
        raise ValueError("Required omission has a matching score")
    arguments.append(fields)


def validate_score(fields, candidate, frame, identity, number, policy):
    if len(fields) != 10 or frame["active"] != identity or candidate["score"] is not None:
        raise ValueError("Malformed, repeated or inactive native candidate score")
    score = scalar(fields[3], number, -1, 9223372036854775807)
    matches, known = (scalar(value, number, 0, 1) for value in fields[4:6])
    exact, params = (scalar(value, number, -1) for value in fields[6:8])
    scalar(fields[9], number)
    if not known and exact != -1 or known and exact < 0:
        raise ValueError("Resolution score reads an uninitialized exact-match count")
    pass_name = candidate["begin"][4]
    if pass_name in ("cast", "precision"):
        reasons = CAST_REASONS if pass_name == "cast" else {"precision-better", "precision-skip"}
        if known or fields[8] not in reasons or candidate["arguments"] or params != int(candidate["begin"][10]):
            raise ValueError("Cast resolver advertises parameter scoring")
        if pass_name == "cast" and (score < 0 or matches != (score > 0)):
            raise ValueError("Cast match status contradicts its native score")
        if pass_name == "precision" and (score != -1 or matches != (fields[8] == "precision-better")):
            raise ValueError("Precision choice invents a conversion score")
    else:
        args = int(frame["begin"][5])
        if params < -1 or params > int(candidate["begin"][10]) or known != (args <= params):
            raise ValueError("Native parameter-count gate contradicts its score")
        real = [arg for arg in candidate["arguments"] if not arg[5].startswith("trailing-")]
        trailing = [arg for arg in candidate["arguments"] if arg[5].startswith("trailing-")]
        if not known:
            expected_reason = "property-accessor" if params == -1 else "argument-count"
            if score or matches or real or trailing or fields[8] != expected_reason:
                raise ValueError("Skipped candidate invents argument checks")
        else:
            failed = bool(real and real[-1][4] == "0")
            if [int(arg[3]) for arg in real] != list(range(1, len(real) + 1)) or len(real) > args or (len(real) < args and not failed):
                raise ValueError("Native real-argument scoring coverage is incomplete")
            if [int(arg[3]) for arg in trailing] != list(range(args + 1, args + 1 + len(trailing))):
                raise ValueError("Native trailing-parameter coverage has gaps")
            missing = bool(trailing and trailing[-1][5] == "trailing-required")
            if any(arg[5] == "trailing-required" for arg in trailing[:-1]) or (len(trailing) != params - args and not missing):
                raise ValueError("Native optional-parameter walk is incomplete")
            actual_score = 0 if failed else sum(int(arg[4]) for arg in real)
            actual_exact = sum(int(arg[4]) >= policy[1] for arg in real)
            expected_reason = "missing-required" if missing else "argument-mismatch" if failed else "parameter-checks"
            if (score != actual_score or exact != actual_exact or matches != ((args == 0 or actual_score > 0) and not missing)
                    or fields[8] != expected_reason):
                raise ValueError("Native procedure score contradicts its actual parameter receipts")
    candidate["score"] = fields


def validate_rank(fields, candidate, frame, identity, number):
    if len(fields) != 7 or frame["pending_rank"] != identity or candidate["rank"] is not None:
        raise ValueError("Repeated or unordered native ranking receipt")
    maximum = scalar(fields[4], number, 0, 9223372036854775807)
    ties, chosen = (scalar(value, number) for value in fields[5:7])
    score = candidate["score"]
    action = fields[3]
    if candidate["begin"][4] == "precision":
        expected = "precision-better" if score[4] == "1" else "precision-skip"
        if action != expected:
            raise ValueError("Precision ranking contradicts its observed branch")
        if action == "precision-better":
            frame["winner"] = identity
    elif score[4] == "0":
        if action != "rejected":
            raise ValueError("Rejected native candidate acquired a ranking choice")
    else:
        value = int(score[3])
        if not frame["ties"] or value > frame["maximum"]:
            if action == "ineligible":
                if frame["begin"][4] != "closest" or not int(frame["begin"][6]) & 2 or int(score[6]) >= 1:
                    raise ValueError("Native operator eligibility was fabricated")
            elif action == "better":
                frame["maximum"], frame["ties"], frame["winner"] = value, 1, identity
            else:
                raise ValueError("Native better-candidate ranking contradicts its score")
        elif value == frame["maximum"]:
            if action != "tie":
                raise ValueError("Native equal-score candidate lost its ambiguity receipt")
            frame["ties"] += 1
        elif action != "inferior":
            raise ValueError("Native lower-score candidate acquired a better rank")
    if (maximum, ties, chosen) != (frame["maximum"], frame["ties"], frame["winner"]):
        raise ValueError("Native ranking state contradicts its candidate receipt")
    candidate["rank"] = fields


def validate_end(fields, frame, candidates, number):
    if len(fields) != 8 or frame["active"] or frame["pending_rank"] or frame["end"] is not None:
        raise ValueError("Native resolution ended with an unfinished candidate")
    chosen, confirmation = (scalar(value, number) for value in fields[2:4])
    maximum = scalar(fields[4], number, 0, 9223372036854775807)
    ties, error, present = (scalar(value, number) for value in fields[5:8])
    if present not in (0, 1) or (maximum, ties) != (frame["maximum"], frame["ties"]):
        raise ValueError("Native resolution outcome has a contradictory score")
    if present:
        if not chosen or chosen != frame["winner"] or ties > 1 or error:
            raise ValueError("Native resolution invents a unique selected procedure")
    elif chosen or ties > 1 and not error:
        raise ValueError("Failed native resolution invents a selected candidate")
    if confirmation:
        confirmed = candidates.get(confirmation)
        if (confirmation not in frame["candidates"] or confirmed["begin"][4] != "confirm"
                or confirmed["score"] is None or confirmed["rank"] is not None):
            raise ValueError("Native resolution invents winner confirmation")
        if bool(present) != (confirmed["score"][4] == "1"):
            raise ValueError("Native confirmation disagrees with the returned procedure")
        if present and candidates[chosen]["begin"][5:] != confirmed["begin"][5:]:
            raise ValueError("Native confirmation silently changed its procedure")
    elif present and frame["begin"][4] == "closest":
        raise ValueError("Native closest-procedure choice omitted its confirmation pass")
    confirmations = [identity for identity in frame["candidates"] if candidates[identity]["begin"][4] == "confirm"]
    if confirmations != ([confirmation] if confirmation else []):
        raise ValueError("Native resolution confirmation coverage is incomplete")
    frame["end"] = fields
    frame["decision"] = "selected" if present else "ambiguous" if ties > 1 else "rejected" if error else "no-match"

# end of semantic_resolution.py
