"""Project: FreeBASIC semantic sidecar
File: semantic_procedure_state.py
Purpose: Validate native hidden error-handler and GOSUB state receipts.
Responsibilities: Procedure/storage ownership, actual transitions and bounded closure.
This file intentionally does NOT claim generated actions executed at runtime.
"""

from semantic_lookup import scalar

FEATURE = "native-procedure-runtime-state"
PREFIX = "native-procedure-state"
KINDS = {"begin", "handler-allocated", "handler-save", "module-name-save", "function-name-save",
         "before-error-exit", "after-error-exit", "gosub-allocated", "gosub-direct", "gosub-computed", "gosub-return", "gosub-exit"}


def validate_procedure_state(model, number, owners):
    events, summaries, procedures = {}, {}, {}
    for row in model.records["K"]:
        if not row[3].startswith(PREFIX):
            continue
        module = scalar(row[2], number, 1)
        if row[1] != "module" or model.capabilities.get(module, {}).get(FEATURE) != "available":
            raise ValueError("Unavailable or foreign native procedure state")
        module_events = events.setdefault(module, [])
        if row[3] == PREFIX + "-count":
            if module in summaries:
                raise ValueError("Repeated native procedure-state summary")
            summaries[module] = scalar(row[4], number, 0, 1000000)
            continue
        if module in summaries:
            raise ValueError("Native procedure state follows its terminal summary")
        parts = row[3].split(":")
        if len(parts) != 2 or parts[0] != PREFIX or scalar(parts[1], number, 1, 1000000) != len(module_events) + 1:
            raise ValueError("Native procedure-state identities have gaps")
        fields = row[4].split("\t")
        if len(fields) != 13 or fields[1] not in KINDS or fields[10] not in ("none", "call-ret", "setjmp"):
            raise ValueError("Malformed native procedure-state action")
        if model.expressions_only or model.bindings_only:
            raise ValueError("Reduced model invents unavailable hidden-storage identities")
        ids = {}
        for index in (0, 5, 6, 7, 8, 9, 11, 12):
            identity = scalar(fields[index], number, 1 if index == 0 else 0)
            ids[index] = identity
            if identity:
                symbol = model.symbols.get(identity)
                kind = 3 if index == 0 else 7 if index == 12 else 1
                if symbol is None or int(symbol[3]) != kind or owners.get(("symbol", identity)) != module:
                    raise ValueError("Native procedure state has a missing or foreign symbol")
        statement, context, source = [scalar(value, number) for value in fields[2:5]]
        if statement and (statement not in model.statements or int(model.statements[statement][7]) != module):
            raise ValueError("Native procedure state has another statement owner")
        if not context or model.configurations.get(context) != module:
            raise ValueError("Native procedure state has another configuration")
        if source and (source not in model.source_contexts or int(model.source_contexts[source][4]) != module):
            raise ValueError("Native procedure state has another source occurrence")
        if bool(ids[5]) != bool(ids[8]):
            raise ValueError("Native local-handler storage lacks its saved-state flag")
        key = module, ids[0]
        kind = fields[1]
        state = procedures.get(key)
        if kind == "begin":
            if state is not None or any(ids[index] for index in (5, 6, 7, 8, 9, 11, 12)) or fields[10] != "none":
                raise ValueError("Native procedure begins with invented or stale hidden state")
            state = {"events": [], "storage": [0] * 5, "gosub_mode": None, "before_exit": False, "after_exit": False}
            procedures[key] = state
        elif state is None:
            raise ValueError("Native hidden state has no procedure initialization")
        storage = [ids[index] for index in (5, 6, 7, 8, 9)]
        previous = state["storage"]
        if kind == "handler-allocated":
            if previous[0] or previous[3] or not ids[5] or not ids[8] or storage[1:3] != previous[1:3] or storage[4] != previous[4]:
                raise ValueError("Native handler allocation contradicts previous storage")
        elif kind == "module-name-save":
            if not ids[6] or any(storage[i] != previous[i] for i in (0, 2, 3, 4)):
                raise ValueError("Native module-name save changed unrelated storage")
        elif kind == "function-name-save":
            if not ids[7] or any(storage[i] != previous[i] for i in (0, 1, 3, 4)):
                raise ValueError("Native function-name save changed unrelated storage")
        elif kind == "gosub-allocated":
            if previous[4] or not ids[9] or storage[:4] != previous[:4] or fields[10] == "none":
                raise ValueError("Native GOSUB allocation contradicts previous storage")
            state["gosub_mode"] = fields[10]
        elif kind == "after-error-exit":
            if not state["before_exit"] or state["after_exit"] or any(storage[:4]) or storage[4] != previous[4]:
                raise ValueError("Native error-state cleanup lacks its pre-cleanup snapshot")
            state["after_exit"] = True
        elif storage != previous:
            raise ValueError("Native action changed storage without its allocation/save receipt")
        if kind == "handler-save":
            if not ids[5] or not ids[8] or not ids[11] or not ids[12]:
                raise ValueError("Native local-handler save lacks temporary/guarded target")
        elif ids[11]:
            raise ValueError("Native procedure action invents a handler-save temporary")
        if kind == "before-error-exit":
            if state["before_exit"]:
                raise ValueError("Native error-state epilogue has repeated entry")
            state["before_exit"] = True
        if kind.startswith("gosub-"):
            if not ids[9] or fields[10] == "none" or fields[10] != state["gosub_mode"]:
                raise ValueError("Native GOSUB action has no allocated implementation state")
            if kind in ("gosub-direct", "gosub-computed") and not ids[12]:
                raise ValueError("Native GOSUB dispatch has no actual target")
        elif fields[10] != "none":
            raise ValueError("Native error state invents a GOSUB implementation")
        if kind not in ("handler-save", "gosub-direct", "gosub-computed", "gosub-return") and ids[12]:
            raise ValueError("Native procedure action invents a control-flow target")
        state["storage"] = storage
        state["events"].append(fields)
        module_events.append(fields)
    for module, capabilities in model.capabilities.items():
        if capabilities.get(FEATURE) == "available" and summaries.get(module) != len(events.get(module, ())):
            raise ValueError("Native procedure-state terminal coverage is incomplete")
    model.procedure_state_events = events
    model.procedure_states = procedures

# end of semantic_procedure_state.py
