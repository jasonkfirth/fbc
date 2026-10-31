"""Project: FreeBASIC semantic sidecar
File: semantic_evaluations.py
Purpose: Validate temporary native evaluation and query lexer lifetimes.
Responsibilities: Input/result code units, nesting, replay and coverage closure.
This file intentionally does NOT evaluate inputs or resolve queried symbols.
"""

from semantic_lookup import scalar, text_field

FEATURE = "native-evaluation-contexts"
BUFFER_FEATURE = "native-evaluation-lexer-buffers"
PREFIX = "native-evaluation"


def payload(fields, number, unescape, absent=False):
    encoding, raw_units, raw_width, raw_text = fields
    units = scalar(raw_units, number, 0, 65536)
    width = scalar(raw_width, number, 0, 4)
    text = unescape(raw_text)
    if absent:
        if (encoding, units, width, text) != ("none", 0, 0, ""):
            raise ValueError("Replayed evaluation invents a returned result")
        return None
    if encoding == "bytes":
        value = text.encode("utf-8", errors="surrogateescape")
        if width != 1 or len(value) != units:
            raise ValueError("Evaluation byte payload has an incorrect length or width")
    elif encoding == "wide":
        if width not in (2, 4):
            raise ValueError("Evaluation invents a host wide code-unit width")
        value = tuple(scalar(unit, number, 0, (1 << (8 * width)) - 1) for unit in text.split(",")) if text else ()
        if len(value) != units:
            raise ValueError("Evaluation wide payload has an incorrect unit count")
    else:
        raise ValueError("Unknown evaluation payload encoding")
    return value


def validate_evaluations(model, number, unescape):
    events, scopes, summaries, states = {}, {}, {}, {}
    for row in model.records["K"]:
        if not row[3].startswith(PREFIX):
            continue
        module = scalar(row[2], number, 1)
        if row[1] != "module" or model.bindings_only or model.capabilities.get(module, {}).get(FEATURE) != "available":
            raise ValueError("Unavailable or unowned native evaluation receipt")
        state = states.setdefault(module, {"stack": [], "begins": 0, "ends": 0, "pending": None, "max_depth": 0})
        buffers_available = model.capabilities[module].get(BUFFER_FEATURE) == "available"
        stream = events.setdefault(module, [])
        if row[3] == PREFIX + "-counts":
            fields = row[4].split("\t")
            if module in summaries or len(fields) != 5:
                raise ValueError("Repeated or malformed evaluation summary")
            summaries[module] = [scalar(value, number, 0, 1000000) for value in fields]
            continue
        parts = row[3].split(":")
        if module in summaries or len(parts) != 2 or parts[0] != PREFIX or scalar(parts[1], number, 1, 1000000) != len(stream) + 1:
            raise ValueError("Evaluation receipts have gaps or follow their summary")
        fields = row[4].split("\t")
        kind = fields[0]
        if kind not in ("begin", "end", "buffer") or len(fields) != {"begin": 20, "end": 10, "buffer": 6}[kind]:
            raise ValueError("Unknown or malformed evaluation event")
        identity = scalar(fields[1], number, 1, 1000000)
        if kind == "begin":
            if buffers_available and state["stack"] and state["stack"][-1]["buffer_event"] is None:
                raise ValueError("Nested evaluation precedes its parent's prepared buffer")
            state["begins"] += 1
            parent, replay = scalar(fields[2], number), scalar(fields[3], number)
            if identity != state["begins"] or parent != (state["stack"][-1]["id"] if state["stack"] else 0):
                raise ValueError("Evaluation identities are not densely nested")
            if fields[4] not in ("constant-expression", "symbol-query"):
                raise ValueError("Unknown native evaluation purpose")
            statement, context, source = [scalar(value, number) for value in fields[5:8]]
            native_parent = scalar(fields[8], number, 0, 2)
            if (native_parent == 2) != bool(parent):
                raise ValueError("Evaluation parent lexer kind contradicts its scope")
            if statement and (statement not in model.statements or int(model.statements[statement][7]) != module):
                raise ValueError("Evaluation has a missing or foreign statement owner")
            if model.expressions_only:
                if context:
                    raise ValueError("Expression-only evaluation invents a configuration")
            elif model.configurations.get(context) != module:
                raise ValueError("Evaluation has a missing or foreign configuration")
            occurrence = model.source_contexts.get(source)
            if occurrence is None or int(occurrence[4]) != module:
                raise ValueError("Evaluation has a missing or foreign source occurrence")
            if native_parent != 2 and (native_parent == 1) != (occurrence[5] in ("include", "preinclude")):
                raise ValueError("Evaluation lexer kind disagrees with its source occurrence")
            copied = payload(fields[9:13], number, unescape)
            fields[13] = text_field(fields[13], unescape)
            start_line, start_column, end_line, end_column = [scalar(value, number) for value in fields[14:18]]
            physical = scalar(fields[18], number, 0, 1)
            if physical and (not fields[13] or not start_line or (end_line, end_column) < (start_line, start_column)):
                raise ValueError("Evaluation invents a physical entry anchor")
            loader = scalar(fields[19], number)
            if loader:
                invocation = model.macro_invocations.get(loader)
                owner = model.source_contexts.get(int(invocation[4])) if invocation else None
                if owner is None or int(owner[4]) != module:
                    raise ValueError("Evaluation has a missing or foreign loader invocation")
            previous = state["pending"]
            if replay:
                if previous is None or previous["id"] != replay or previous["begin"][2] != fields[2] or fields[4] != "symbol-query":
                    raise ValueError("Evaluation replay has no actual retired query context")
                if previous["begin"][4] != fields[4] or previous["input"] != copied or previous["begin"][7:12] != fields[7:12]:
                    raise ValueError("Evaluation replay changes its source or copied input")
            elif previous is not None:
                raise ValueError("Evaluation replay loses its replacement context")
            state["pending"] = None
            scope = {"id": identity, "begin": fields, "input": copied, "buffer": None, "buffer_event": None,
                     "end": None, "result": None, "native_error_observed": None}
            scopes[module, identity] = scope
            state["stack"].append(scope)
            state["max_depth"] = max(state["max_depth"], len(state["stack"]))
        elif kind == "buffer":
            if not buffers_available or not state["stack"] or state["stack"][-1]["id"] != identity or state["pending"] is not None:
                raise ValueError("Evaluation buffer has no available active context")
            scope = state["stack"][-1]
            if scope["buffer_event"] is not None:
                raise ValueError("Evaluation buffer was copied more than once")
            copied = payload(fields[2:6], number, unescape)
            if not copied or copied[-1] != 10:
                raise ValueError("Evaluation buffer loses its native end-of-expression marker")
            replay = int(scope["begin"][3])
            if replay and scopes[module, replay]["buffer"] != copied:
                raise ValueError("Evaluation replay changes its actual lexer buffer")
            scope["buffer"], scope["buffer_event"] = copied, fields
        else:
            if not state["stack"] or identity != state["stack"][-1]["id"] or state["pending"] is not None:
                raise ValueError("Evaluation result has no active context")
            scope = state["stack"].pop()
            if buffers_available and scope["buffer_event"] is None:
                raise ValueError("Evaluation result precedes its prepared lexer buffer")
            reason = fields[2]
            if reason not in ("completed", "replay") or reason == "replay" and scope["begin"][4] != "symbol-query":
                raise ValueError("Unknown or fabricated evaluation completion reason")
            error, before, after = [scalar(value, number, 0, 2147483647) for value in fields[3:6]]
            if after < before:
                raise ValueError("Evaluation error counts go backwards")
            # The parser can report an error and still return a folded node.
            # Conversely the callback can return an error before its caller
            # reports it. Preserve both channels without changing the result.
            scope["native_error_observed"] = bool(error or after > before)
            scope["end"] = fields
            scope["result"] = payload(fields[6:10], number, unescape, absent=reason == "replay")
            if reason == "completed" and fields[6] != scope["begin"][9]:
                raise ValueError("Evaluation result changes native callback encoding")
            if reason == "replay":
                state["pending"] = scope
            state["ends"] += 1
        stream.append(fields)
    for module, capabilities in model.capabilities.items():
        if capabilities.get(BUFFER_FEATURE) == "available" and capabilities.get(FEATURE) != "available":
            raise ValueError("Evaluation buffers lack their evaluation context capability")
        if capabilities.get(FEATURE) != "available":
            continue
        state = states.get(module, {"stack": [], "begins": 0, "ends": 0, "pending": None, "max_depth": 0})
        summary = summaries.get(module)
        if summary is None or summary[:4] != [len(events.get(module, ())), state["begins"], state["ends"], len(state["stack"])]:
            raise ValueError("Native evaluation coverage is incomplete")
        if not 1 <= summary[4] <= 1024 or state["max_depth"] > summary[4]:
            raise ValueError("Native evaluation depth exceeds its producer bound")
        if module not in model.recovered_modules and (state["stack"] or state["pending"]):
            raise ValueError("Accepted evaluation output retains an unfinished context")
    model.evaluation_events, model.evaluation_contexts = events, scopes

# end of semantic_evaluations.py
