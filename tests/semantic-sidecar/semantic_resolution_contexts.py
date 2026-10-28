"""Project: FreeBASIC semantic sidecar
File: semantic_resolution_contexts.py
Purpose: Validate explicit identities attached to native resolver observations.
Responsibilities: Nominal type snapshots, canonical references and actual callee uses.
This file intentionally does NOT join candidates to bindings by spelling or ranges.
"""

from semantic_resolution import snapshot, scalar, text_field

FEATURE = "native-resolution-contexts"
CONTEXT = "native-resolution-context"
USE = "native-resolution-use"
BOUND = "native-resolution-bound-use"
MACRO = "native-resolution-macro-use"
ROLES = {"begin", "candidate", "argument", "end", "access"}


def identity_reference(model, subject_modules, module, identity, kind=None):
    if not identity or identity not in model.symbols or subject_modules.get(("symbol", identity)) != module:
        raise ValueError("Resolution context has a missing or foreign symbol identity")
    if kind is not None and int(model.symbols[identity][3]) != kind:
        raise ValueError("Resolution context has another native symbol class")


def validate_resolution_contexts(model, number, unescape, subject_modules):
    contexts, uses, bounds, macros, summaries = {}, {}, {}, {}, {}
    expressions = {int(row[1]): row for row in model.records["E"]}
    end_events = {(module, int(event[1])): index for module, events in model.resolution_events.items()
                  for index, event in enumerate(events, 1) if event[0] == "end"}
    for row in model.records["K"]:
        if not row[3].startswith((CONTEXT, USE, BOUND, MACRO)):
            continue
        module = scalar(row[2], number, 1)
        if row[1] != "module" or model.capabilities.get(module, {}).get(FEATURE) != "available":
            raise ValueError("Unavailable or unowned native resolution context")
        if row[3] == CONTEXT + "-counts":
            if module in summaries:
                raise ValueError("Repeated native resolution context summary")
            fields = row[4].split("\t")
            if len(fields) != 3:
                raise ValueError("Malformed native resolution context summary")
            summaries[module] = [scalar(value, number, 0, 1000000) for value in fields]
            continue
        parts = row[3].split(":")
        if len(parts) != 2 or parts[0] not in (CONTEXT, USE, BOUND, MACRO):
            raise ValueError("Unknown native resolution context role")
        identity = scalar(parts[1], number, 1, 1000000)
        key = module, identity
        full = not model.expressions_only and not model.bindings_only
        if parts[0] == CONTEXT:
            events = model.resolution_events.get(module, ())
            if key in contexts or identity > len(events):
                raise ValueError("Repeated or dangling native event context")
            event = events[identity - 1]
            if event[0] not in ROLES:
                raise ValueError("Native scoring/ranking event invents a symbol context")
            fields = row[4].split("\t")
            if len(fields) != 20:
                raise ValueError("Malformed native resolution identity context")
            refs = [scalar(value, number) for value in fields[:4]]
            fields[4:12] = snapshot(fields[4:12], number, unescape)
            fields[12:20] = snapshot(fields[12:20], number, unescape)
            if not full and any(refs[:3]):
                raise ValueError("Reduced resolution context invents unavailable symbol identities")
            for reference, payload in zip(refs[1:3], (fields[4:12], fields[12:20])):
                present = payload[0] == "1"
                if full and present:
                    identity_reference(model, subject_modules, module, reference, int(payload[1]))
                elif reference:
                    raise ValueError("Absent resolution nominal type invents an identity")
            role = event[0]
            subject_present, subject_kind = (True, 4) if role == "argument" else (
                event[18] == "1", int(event[19])) if role == "begin" else (
                event[5] == "1", int(event[6])) if role == "candidate" else (
                event[7] == "1", 3) if role == "end" else (event[14] == "1", int(event[15]))
            if full and subject_present:
                identity_reference(model, subject_modules, module, refs[0], subject_kind)
            elif refs[0]:
                raise ValueError("Absent native resolver subject invents an identity")
            if role != "argument" and refs[3]:
                raise ValueError("Nonargument resolver context invents a source expression")
            if role == "argument":
                if fields[4:12][6] != event[11] or fields[12:20][6] != event[15]:
                    raise ValueError("Native argument subtype hints contradict the actual score receipt")
                if event[12] == "0" and (refs[3] or fields[12] != "0"):
                    raise ValueError("Omitted argument invents a source expression or subtype")
                if refs[3]:
                    expression = expressions.get(refs[3])
                    if expression is None or subject_modules.get(("expression", refs[3])) != module:
                        raise ValueError("Resolution argument has a missing or foreign expression identity")
                    # Unwind can refine an E concept while the immutable AST
                    # association survives. Its raw type still belongs to this
                    # observed argument; no range-based parent is inferred.
                    if expression[12] != event[14]:
                        raise ValueError("Resolution argument expression has another native datatype")
            if role not in ("begin", "argument") and fields[12] != "0":
                raise ValueError("Resolver context invents a requested nominal type")
            contexts[key] = fields
        else:
            scope = model.resolution_scopes.get(key)
            if scope is None or scope["end"] is None or scope["begin"][4] != "closest":
                raise ValueError("Native callee use has no completed closest-procedure search")
            if parts[0] == USE:
                if key in uses:
                    raise ValueError("Repeated native overload source use")
                fields = row[4].split("\t")
                if len(fields) != 12:
                    raise ValueError("Malformed native overload source use")
                fields[1] = text_field(fields[1], unescape)
                for index in range(12):
                    if index != 1:
                        scalar(fields[index], number)
                if fields[0] not in ("0", "1") or fields[7] not in ("0", "1"):
                    raise ValueError("Invalid native overload source-use presence flag")
                source = int(fields[2])
                if source and (source not in model.source_contexts or int(model.source_contexts[source][4]) != module):
                    raise ValueError("Native overload source use has a foreign source occurrence")
                if fields[0] == "0" and (fields[1] or any(value != "0" for value in fields[2:])):
                    raise ValueError("Absent native callee site invents source coordinates")
                if fields[7] == "1" and (not source or not fields[1] or int(fields[8]) < 1):
                    raise ValueError("Native callee use has no raw source origin")
                if fields[7] == "0" and fields[8:] != ["0"] * 4:
                    raise ValueError("Native callee use invents unavailable raw coordinates")
                uses[key] = fields
            elif parts[0] == MACRO:
                if key in macros or key in bounds or not full:
                    raise ValueError("Repeated or unavailable native macro callee reference")
                reference = scalar(row[4], number, 1)
                ending = scope["end"]
                linked = contexts.get((module, end_events[key]))
                symbol = int(linked[0]) if linked else 0
                if ending[7] != "1" or not symbol or ("symbol", symbol, "reference-" + str(reference)) not in model.macro_origins:
                    raise ValueError("Native macro callee does not name the returned procedure reference")
                macros[key] = reference
            else:
                if key in bounds or key in macros or not full:
                    raise ValueError("Repeated or unavailable native callee binding")
                binding = scalar(row[4], number, 1)
                records = model.records["B"]
                if binding > len(records) or subject_modules.get(("binding", binding)) != module:
                    raise ValueError("Native overload use has a missing or foreign binding")
                target = records[binding - 1]
                ending = scope["end"]
                end_event = end_events[key]
                bound_context = contexts.get((module, end_event))
                if (ending[7] != "1" or target[2] != "reference" or bound_context is None
                        or target[1] != bound_context[0]):
                    raise ValueError("Native callee binding does not name the returned procedure")
                bounds[key] = binding
    for module, capabilities in model.capabilities.items():
        if capabilities.get(FEATURE) != "available":
            continue
        expected = {(module, index) for index, event in enumerate(model.resolution_events.get(module, ()), 1)
                    if event[0] in ROLES}
        actual = {key for key in contexts if key[0] == module}
        if expected != actual:
            raise ValueError("Native resolution identity coverage is incomplete")
        counts = [len(actual), sum(key[0] == module for key in uses), sum(key[0] == module for key in bounds) + sum(key[0] == module for key in macros)]
        if summaries.get(module) != counts:
            raise ValueError("Native resolution context closure is incomplete")
    for key in bounds.keys() | macros.keys():
        if key not in uses:
            raise ValueError("Native callee binding has no observed parser use")
    model.resolution_contexts = contexts
    model.resolution_uses = uses
    model.resolution_bound_uses = bounds
    model.resolution_macro_uses = macros

# end of semantic_resolution_contexts.py
