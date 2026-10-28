"""Project: FreeBASIC semantic sidecar
File: semantic_lookup.py
Purpose: Validate actual native lookup order and returned chains.
Responsibilities: Probe vocabulary, nested scopes, symbol snapshots and count closure.
This file intentionally does NOT select a binding from lookup results.
"""

FEATURE = "native-name-lookups"
PREFIX = "native-lookup"
PROBES = {"keywords", "keyword-shadow", "lexical", "unqualified-imports", "type-locals",
          "type-own", "type-fallback", "type-imports", "import-list", "global-import-hash", "qualified-own"}
DECISIONS = {"keyword-disabled", "keyword-enabled", "defer-quirk-keyword", "lexical-selected",
             "global-selected", "defer-global-ancestor", "active-enum-selected", "ancestor-before-imports",
             "imports-returned", "ancestor-selected", "local-selected", "local-pass-skipped",
             "skip-type-local", "type-own-selected", "base-member-selected", "inherited-enum-selected",
             "type-fallback-selected", "import-accepted", "nonparent-import-rejected", "active-enum-member",
             "active-hash-enum-member", "export-to-namespace", "skip-qualified-local"}
ROUTES = {"unqualified": {"keyword", "macro-shadow", "namespace", "type-namespace"},
          "qualified": {"enum-without-table", "imports-disabled", "own-symbol", "no-namespace-extension",
                        "no-imports", "global-imports", "parent-imports", "namespace-imports"}}


def scalar(text, number, minimum=0, maximum=2147483647):
    value = number(text, minimum)
    if str(value) != text or value > maximum:
        raise ValueError("Noncanonical or oversized native lookup scalar")
    return value


def text_field(text, unescape):
    value = unescape(text)
    if "\0" in value or len(value.encode("utf-8", errors="surrogateescape")) > 65536:
        raise ValueError("Invalid native lookup text")
    return value


def symbol(fields, model, module, number, unescape, owners):
    if len(fields) != 8:
        raise ValueError("Malformed native lookup symbol snapshot")
    fields = fields.copy()
    present = scalar(fields[0], number, 0, 1)
    kind = scalar(fields[1], number, 0, 17)
    scalar(fields[2], number, 0, 4294967295)
    for value in fields[3:5]:
        scalar(value, number, -9223372036854775808, 18446744073709551615)
    fields[5:7] = [text_field(value, unescape) for value in fields[5:7]]
    identity = scalar(fields[7], number)
    if not present:
        if fields[:5] != ["0", "0", "2147483648", "0", "0"] or any(fields[5:7]) or identity:
            raise ValueError("Absent native lookup result invents a symbol")
    elif not kind:
        raise ValueError("Native lookup symbol has no class")
    if model.expressions_only:
        if identity:
            raise ValueError("Reduced native lookup invents an unavailable S identity")
    elif present:
        if identity not in model.symbols or owners.get(("symbol", identity)) != module or int(model.symbols[identity][3]) != kind:
            raise ValueError("Native lookup snapshot has a missing or foreign S identity")
    return fields


def validate_lookup(model, number, unescape, owners):
    states, summaries = {}, {}
    for row in model.records["K"]:
        if not row[3].startswith(PREFIX):
            continue
        module = scalar(row[2], number, 1)
        if row[1] != "module" or model.capabilities.get(module, {}).get(FEATURE) != "available":
            raise ValueError("Unavailable or unowned native lookup trace")
        state = states.setdefault(module, {"events": [], "stack": [], "queries": {}, "counts": [0] * 7})
        if row[3] == PREFIX + "-counts":
            fields = row[4].split("\t")
            if module in summaries or len(fields) != 7:
                raise ValueError("Repeated or malformed native lookup summary")
            summaries[module] = [scalar(value, number, 0, 1000000) for value in fields]
            continue
        if module in summaries:
            raise ValueError("Native lookup event follows its terminal summary")
        parts = row[3].split(":")
        if len(parts) != 2 or parts[0] != PREFIX or scalar(parts[1], number, 1, 1000000) != len(state["events"]) + 1:
            raise ValueError("Native lookup event identities have gaps")
        fields = row[4].split("\t")
        if len(fields) < 2:
            raise ValueError("Malformed native lookup event")
        event = fields[0]
        identity = scalar(fields[1], number, 1, 1000000)
        stack, queries, counts = (state[key] for key in ("stack", "queries", "counts"))
        if event == "begin":
            if len(fields) != 21 or identity != len(queries) + 1 or len(stack) >= 1024:
                raise ValueError("Malformed native lookup opening")
            parent = scalar(fields[2], number)
            if parent != (stack[-1] if stack else 0) or fields[3] not in ROUTES:
                raise ValueError("Native lookup has a fabricated parent or route")
            fields[4] = text_field(fields[4], unescape)
            for index in (5, 6):
                scalar(fields[index], number, 0, 1)
            for index in (7, 8, 9, 11, 12):
                scalar(fields[index], number)
            statement, configuration, source = map(int, fields[7:10])
            if statement and (statement not in model.statements or int(model.statements[statement][7]) != module):
                raise ValueError("Native lookup has a foreign statement")
            if ((configuration and model.configurations.get(configuration) != module)
                    or (not configuration and not model.expressions_only)):
                raise ValueError("Native lookup has a foreign configuration")
            if source and (source not in model.source_contexts or int(model.source_contexts[source][4]) != module):
                raise ValueError("Native lookup has a foreign source occurrence")
            fields[10] = text_field(fields[10], unescape)
            fields[13:] = symbol(fields[13:], model, module, number, unescape, owners)
            queries[identity] = {"begin": fields, "steps": [], "results": [], "end": None}
            stack.append(identity)
            counts[1] += 1
        else:
            if not stack or identity != stack[-1]:
                raise ValueError("Native lookup event has no current search")
            query = queries[identity]
            if event in ("probe", "decision"):
                if len(fields) != 20 or query["results"]:
                    raise ValueError("Native lookup probe follows returned results")
                ordinal = scalar(fields[2], number, 1, 1000000)
                if ordinal != len(query["steps"]) + 1 or fields[3] not in (PROBES if event == "probe" else DECISIONS):
                    raise ValueError("Native lookup invents a probe or precedence reason")
                fields[4:12] = symbol(fields[4:12], model, module, number, unescape, owners)
                fields[12:20] = symbol(fields[12:20], model, module, number, unescape, owners)
                if event == "decision" and fields[12] != "1":
                    raise ValueError("Native precedence decision has no observed symbol")
                query["steps"].append(fields)
                counts[3 if event == "probe" else 4] += 1
            elif event == "result":
                if len(fields) != 12 or scalar(fields[2], number, 1, 4096) != len(query["results"]) + 1:
                    raise ValueError("Native lookup result chain is incomplete")
                scalar(fields[3], number, 0, 1)
                fields[4:] = symbol(fields[4:], model, module, number, unescape, owners)
                if fields[4] != "1":
                    raise ValueError("Native lookup returned an empty chain node")
                query["results"].append(fields)
                counts[5] += 1
            elif event == "end":
                if len(fields) != 6 or fields[5] not in ROUTES[query["begin"][3]]:
                    raise ValueError("Unknown native lookup return route")
                token = scalar(fields[2], number, -1)
                token_class = scalar(fields[3], number, -1, 7)
                if query["begin"][3] == "qualified" and (token, token_class) != (-1, -1):
                    raise ValueError("Qualified lookup invents lexer token outputs")
                if query["begin"][3] == "unqualified" and (token < 0 or token_class < 0):
                    raise ValueError("Unqualified lookup lost its native token outputs")
                if scalar(fields[4], number, 0, 4096) != len(query["results"]):
                    raise ValueError("Native lookup return count contradicts its copied chain")
                if fields[5] in ("keyword", "macro-shadow", "own-symbol") and len(query["results"]) != 1:
                    raise ValueError("Direct native lookup selection has an impossible chain")
                if fields[5] in ("keyword", "macro-shadow", "own-symbol") and query["results"][0][3] != "0":
                    raise ValueError("Direct native lookup selection invents an import")
                if fields[5] in ("keyword", "macro-shadow"):
                    expected_class = "6" if fields[5] == "keyword" else "5"
                    if query["results"][0][5] != expected_class:
                        raise ValueError("Native keyword or macro return has another symbol class")
                if fields[5] in ("enum-without-table", "imports-disabled", "no-namespace-extension", "no-imports") and query["results"]:
                    raise ValueError("Unavailable native lookup path invents results")
                query["end"] = fields
                stack.pop()
                counts[2] += 1
            else:
                raise ValueError("Unknown native lookup event kind")
        state["events"].append(fields)
        counts[0] += 1
    for module, capabilities in model.capabilities.items():
        if capabilities.get(FEATURE) != "available":
            continue
        state = states.get(module)
        counts = state["counts"].copy() if state else [0] * 7
        counts[6] = len(state["stack"]) if state else 0
        if summaries.get(module) != counts or counts[1] - counts[2] != counts[6]:
            raise ValueError("Native lookup terminal coverage is incomplete")
        if module not in model.recovered_modules and counts[6]:
            raise ValueError("Accepted module retains unfinished native lookups")
    model.lookup_events = {module: state["events"] for module, state in states.items()}
    model.lookup_queries = {(module, identity): query for module, state in states.items() for identity, query in state["queries"].items()}

# end of semantic_lookup.py
