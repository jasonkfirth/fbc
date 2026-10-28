"""Project: FreeBASIC semantic sidecar
File: semantic_qualified_filters.py
Purpose: Validate explicit native qualified-lookup/class-filter links.
Responsibilities: Attempt identities, actual chain handoff and returned symbols.
This file intentionally does NOT join searches by spelling, ranges or native addresses.
"""

from semantic_lookup import scalar, symbol

FEATURE = "native-qualified-filter-links"
PREFIX = "native-qualified-filter"


def validate_qualified_filters(model, number, unescape, owners):
    links, summaries = {}, {}
    used_lookups, used_filters = set(), set()
    for row in model.records["K"]:
        if not row[3].startswith(PREFIX):
            continue
        module = scalar(row[2], number, 1)
        if row[1] != "module" or model.capabilities.get(module, {}).get(FEATURE) != "available":
            raise ValueError("Unavailable or unowned native qualified-filter link")
        if row[3] == PREFIX + "-count":
            if module in summaries:
                raise ValueError("Repeated native qualified-filter summary")
            summaries[module] = scalar(row[4], number, 0, 1000000)
            continue
        if module in summaries:
            raise ValueError("Native qualified-filter link follows its terminal summary")
        parts = row[3].split(":")
        module_links = links.setdefault(module, [])
        if len(parts) != 2 or parts[0] != PREFIX or scalar(parts[1], number, 1, 1000000) != len(module_links) + 1:
            raise ValueError("Native qualified-filter link identities have gaps")
        fields = row[4].split("\t")
        if len(fields) != 11:
            raise ValueError("Malformed native qualified-filter link")
        lookup_id, filter_id, requested = [scalar(value, number) for value in fields[:3]]
        if not 0 < requested <= 17:
            raise ValueError("Native qualified-filter link requests another symbol class")
        fields[3:] = symbol(fields[3:], model, module, number, unescape, owners)
        lookup_key, filter_key = (module, lookup_id), (module, filter_id)
        lookup = model.lookup_queries.get(lookup_key)
        if (lookup is None or lookup["begin"][3] != "qualified" or lookup["end"] is None
                or lookup_key in used_lookups):
            raise ValueError("Native qualified-filter link has no distinct completed qualified lookup")
        used_lookups.add(lookup_key)
        if not lookup["results"]:
            if filter_id or fields[3] != "0":
                raise ValueError("Empty qualified lookup invents a filter or returned symbol")
        else:
            filtered = model.filter_scopes.get(filter_key)
            if (filtered is None or filtered["begin"][3] != "class" or filtered["end"] is None
                    or filter_key in used_filters or filtered["begin"][4] != fields[2]):
                raise ValueError("Native qualified-filter link has no distinct completed class filter")
            used_filters.add(filter_key)
            if filtered["begin"][10:] != lookup["results"][0][4:]:
                raise ValueError("Native qualified class filter received another returned chain head")
            if fields[3:] != filtered["end"][4:]:
                raise ValueError("Native qualified-filter wrapper returned another symbol")
        module_links.append(fields)
    for module, capabilities in model.capabilities.items():
        if capabilities.get(FEATURE) == "available" and summaries.get(module) != len(links.get(module, ())):
            raise ValueError("Native qualified-filter link coverage is incomplete")
    model.qualified_filter_links = links

# end of semantic_qualified_filters.py
