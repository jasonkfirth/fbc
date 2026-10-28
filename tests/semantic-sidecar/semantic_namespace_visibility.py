"""Project: FreeBASIC semantic sidecar
File: semantic_namespace_visibility.py
Purpose: Validate source USING order and native namespace import observations.
Responsibilities: Lexical/lookup ownership, relationship lifetime and complete closure.
This file intentionally does NOT infer name resolution or visibility outside a scope.
"""

from collections import Counter, defaultdict
import re

FEATURE = "namespace-import-observations"
CONTEXT_FEATURE = "binding-scope-contexts"
LIMIT = 1000000
PREFIXES = ("namespace-using:", "namespace-using-result:", "namespace-import:", "namespace-import-remove:",
            "namespace-import-state:", "namespace-refresh:", "namespace-refresh-result:", "namespace-visibility-counts:")
KEY = re.compile(r"namespace-(using|using-result|import|import-remove|import-state|refresh|refresh-result|visibility-counts):([1-9][0-9]*)\Z")


def validate_namespace_visibility(model, number, subject_modules):
    groups = defaultdict(dict)
    active_using, active_refresh = {}, {}
    last_binding = defaultdict(int)
    imports_by_symbol = set()
    locations = []
    native_links = {(int(row[2]), int(row[4])) for row in model.relations("imports")
                    if row[1] == row[3] == "symbol" and row[6] == "0"}
    module = 0

    def decimal(text, minimum=0, maximum=LIMIT):
        value = number(text, minimum)
        if str(value) != text or value > maximum:
            raise ValueError("Namespace visibility identity is not canonical or bounded")
        return value

    def symbol(identity, current_module, classes):
        row = model.symbols.get(identity)
        if (row is None or row[3] not in classes
                or subject_modules.get(("symbol", identity)) != current_module):
            raise ValueError("Namespace visibility has a foreign or invalid symbol")
        return row

    def context(fields, current_module, required=False):
        statement, source, construct = (decimal(value, 0, 4294967295) for value in fields)
        if required and (not statement or not source):
            raise ValueError("USING request lacks its source statement")
        if source:
            opened = model.source_contexts.get(source)
            if opened is None or opened[4] != str(current_module):
                raise ValueError("Namespace visibility belongs to another source context")
        if statement:
            native = model.statements.get(statement)
            if native is None or native[7] != str(current_module) or native[5] != str(source):
                raise ValueError("Namespace visibility belongs to another statement")
        if construct:
            block = model.constructs.get(construct)
            opening = None if block is None else model.statements.get(int(block[3]))
            if opening is None or opening[7] != str(current_module):
                raise ValueError("Namespace visibility belongs to another construct")
        return statement, source, construct

    for row in model.rows:
        if row[0] == "M":
            module += 1
            continue
        if row[0] == "LOC" and row[3].startswith("namespace-using:"):
            locations.append((module, row))
        if row[0] != "K" or not row[3].startswith(PREFIXES):
            continue
        match = KEY.fullmatch(row[3])
        if match is None:
            raise ValueError("Unknown namespace visibility property")
        kind, suffix = match.groups()
        identity = decimal(suffix, 1)
        owner = decimal(row[2], 1, 4294967295)
        namespace = symbol(owner, module, ("8",))
        if (row[1] != "symbol" or namespace[11] != "0"
                or model.capabilities[module].get(FEATURE) != "available"):
            raise ValueError("Namespace visibility lacks its global owner or capability")
        facts = groups[module]
        if facts.setdefault("owner", owner) != owner:
            raise ValueError("Namespace visibility changes namespace owner")
        bucket = facts.setdefault(kind, {})
        if identity in bucket or len(bucket) >= LIMIT or "visibility-counts" in facts and kind != "visibility-counts":
            raise ValueError("Repeated, oversized or unclosed namespace visibility sequence")
        fields = row[4].split("\t")
        bucket[identity] = fields
        if kind == "using":
            if identity != len(bucket) or len(fields) != 9 or module in active_using or module in active_refresh:
                raise ValueError("Invalid USING request order or shape")
            statement, source, _ = context(fields[:3], module, required=True)
            lexical, lookup, target = (decimal(value, 1, 4294967295) for value in fields[3:6])
            symbol(lexical, module, ("3", "8", "9", "10", "15"))
            symbol(lookup, module, ("8", "9", "10"))
            symbol(target, module, ("8",))
            first, last = (decimal(value, 0, 4294967295) for value in fields[6:8])
            if (not first) != (not last) or first > last or last > len(model.records["B"]) or fields[8] not in ("written", "expanded"):
                raise ValueError("USING request has an invalid binding interval or source kind")
            if first:
                if first <= last_binding[module]:
                    raise ValueError("USING requests reuse or reorder source bindings")
                last_binding[module] = last
                for binding in range(first, last + 1):
                    if (subject_modules.get(("binding", binding)) != module
                            or model.statement_owners.get(("binding", binding)) != statement):
                        raise ValueError("USING request borrows another statement's bindings")
            active_using[module] = identity
        elif kind == "using-result":
            if (active_using.pop(module, None) != identity or len(fields) != 2
                    or fields[0] not in ("0", "1")):
                raise ValueError("USING outcome closes another request or invents success")
            decimal(fields[1])
        elif kind == "refresh":
            if identity != len(bucket) or len(fields) != 4 or module in active_using or module in active_refresh:
                raise ValueError("Invalid namespace refresh order or shape")
            symbol(decimal(fields[0], 1, 4294967295), module, ("8",))
            context(fields[1:], module)
            active_refresh[module] = identity
        elif kind == "refresh-result":
            if active_refresh.pop(module, None) != identity or len(fields) != 1:
                raise ValueError("Namespace refresh closes another observation")
            decimal(fields[0])
        elif kind == "import":
            if identity != len(bucket) or len(fields) != 10:
                raise ValueError("Native namespace imports are reordered or malformed")
            native, target, destination, lexical = (decimal(value, 1, 4294967295) for value in fields[:4])
            native_symbol = symbol(native, module, ("17",))
            symbol(target, module, ("8", "9", "10"))
            symbol(destination, module, ("8", "9", "10"))
            symbol(lexical, module, ("3", "8", "9", "10", "15"))
            if native_symbol[11] != str(lexical) or (native, target) not in native_links or native in imports_by_symbol:
                raise ValueError("Native import disagrees with its lexical owner or import relationship")
            imports_by_symbol.add(native)
            request, refresh = (decimal(value) for value in fields[4:6])
            context(fields[6:9], module)
            if (request != active_using.get(module, 0) or refresh != active_refresh.get(module, 0)
                    or fields[9] not in ("created", "preexisting")
                    or fields[9] == "preexisting" and (request or refresh)):
                raise ValueError("Native import invents a source request, refresh or creation origin")
        elif kind == "import-remove":
            if identity not in facts.get("import", {}) or len(fields) != 4 or fields[3] not in ("0", "1"):
                raise ValueError("Namespace removal has no live native relationship")
            context(fields[:3], module)
        elif kind == "import-state":
            if identity not in facts.get("import", {}) or fields not in (["removed"], ["retained"]):
                raise ValueError("Namespace lifetime state has no native relationship")
        elif kind == "visibility-counts":
            if identity != module or len(fields) != 4 or module in active_using or module in active_refresh:
                raise ValueError("Namespace visibility closes another module or an open observation")
            for value in fields:
                decimal(value)

    model.namespace_visibility = {}
    expected_locations = {}
    for module, capabilities in model.capabilities.items():
        if capabilities.get(FEATURE) != "available":
            continue
        facts = groups[module]
        counts = facts.get("visibility-counts", {})
        if set(counts) != {module}:
            raise ValueError("Namespace visibility count closure is missing")
        observed = [facts.get(kind, {}) for kind in ("using", "import", "import-remove", "refresh")]
        if [len(rows) for rows in observed] != [int(value) for value in counts[module]]:
            raise ValueError("Namespace visibility counts omit native observations")
        requests, imports, removals, refreshes = observed
        outcomes, refresh_results, states = (facts.get(kind, {}) for kind in ("using-result", "refresh-result", "import-state"))
        if set(outcomes) != set(requests) or set(refresh_results) != set(refreshes) or set(states) != set(imports):
            raise ValueError("Namespace outcomes or lifetime states are incomplete")
        by_request = Counter(int(item[4]) for item in imports.values())
        by_refresh = Counter(int(item[5]) for item in imports.values())
        for identity, fields in requests.items():
            if int(outcomes[identity][1]) != by_request[identity]:
                raise ValueError("USING outcome invents created namespace relationships")
            expected_locations[module, f"namespace-using:{identity}"] = (fields[1], fields[8])
        for identity in refreshes:
            if int(refresh_results[identity][0]) != by_refresh[identity]:
                raise ValueError("Namespace refresh omits its native relationships")
        for identity in imports:
            if states[identity] != (["removed"] if identity in removals else ["retained"]):
                raise ValueError("Namespace lifetime contradicts native removal")
        model.namespace_visibility[module] = facts
    for module, row in locations:
        expected = expected_locations.get((module, row[3]))
        if (expected is None or row[1] != "source-context" or row[2] != expected[0]
                or expected[1] == "expanded"):
            raise ValueError("USING range has a foreign source or makes an expansion editable")

def validate_binding_scope_contexts(model, number, subject_modules):
    observations, counts = defaultdict(dict), {}
    module_bindings = defaultdict(set)
    for (domain, identity), selected in subject_modules.items():
        if domain == "binding":
            module_bindings[selected].add(identity)
    module = 0
    latest_binding = 0
    for row in model.rows:
        if row[0] == "M":
            module += 1
        elif row[0] == "B":
            latest_binding += 1
        elif row[0] == "K" and row[3].startswith(("namespace-binding:", "namespace-binding-count:")):
            owner = number(row[2], 1)
            namespace = model.symbols.get(owner)
            if (row[1] != "symbol" or namespace is None or namespace[3] != "8" or namespace[11] != "0"
                    or subject_modules.get(("symbol", owner)) != module
                    or model.capabilities[module].get(CONTEXT_FEATURE) != "available"):
                raise ValueError("Binding scope context has a foreign owner or unavailable coverage")
            prefix, suffix = row[3].split(":")
            identity = number(suffix, 1)
            if suffix != str(identity) or identity > 4294967295:
                raise ValueError("Binding scope identity is not canonical or bounded")
            if prefix == "namespace-binding-count":
                count = number(row[4], 0)
                if identity != module or module in counts or count > LIMIT:
                    raise ValueError("Binding scope closure is repeated or oversized")
                counts[module] = count
                continue
            fields = row[4].split("\t")
            if (len(fields) != 6 or identity != latest_binding or identity in observations[module]
                    or module in counts or subject_modules.get(("binding", identity)) != module):
                raise ValueError("Binding scope context is repeated, late or belongs to another binding")
            statement, source, construct, lexical, lookup, lookup_hash = [number(value, 0) for value in fields]
            for selected, classes in ((lexical, ("3", "8", "9", "10", "15")),
                                      (lookup, ("8", "9", "10")), (lookup_hash, ("8", "9", "10"))):
                symbol = model.symbols.get(selected)
                if (symbol is None or symbol[3] not in classes
                        or subject_modules.get(("symbol", selected)) != module):
                    raise ValueError("Binding scope context has an invalid lexical or lookup owner")
            if model.statement_owners.get(("binding", identity), 0) != statement:
                raise ValueError("Binding scope context disagrees with creation-time statement ownership")
            if source:
                context = model.source_contexts.get(source)
                if context is None or context[4] != str(module):
                    raise ValueError("Binding scope context has a foreign source occurrence")
            if statement:
                native = model.statements.get(statement)
                if native is None or native[7] != str(module) or native[5] != str(source):
                    raise ValueError("Binding scope context has another statement or source")
            if construct:
                block = model.constructs.get(construct)
                opening = None if block is None else model.statements.get(int(block[3]))
                if opening is None or opening[7] != str(module):
                    raise ValueError("Binding scope context has a foreign source construct")
            observations[module][identity] = tuple(fields)
    model.binding_scope_contexts = {}
    for module, capabilities in model.capabilities.items():
        if capabilities.get(CONTEXT_FEATURE) != "available":
            continue
        expected = module_bindings[module]
        if (module not in counts or set(observations[module]) != expected
                or len(observations[module]) != counts[module]):
            raise ValueError("Binding scope context coverage or closure is incomplete")
        model.binding_scope_contexts[module] = observations[module]

# end of semantic_namespace_visibility.py
