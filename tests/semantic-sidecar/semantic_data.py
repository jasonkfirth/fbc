"""Project: FreeBASIC semantic sidecar
File: semantic_data.py
Purpose: Validate source DATA streams and native READ/RESTORE selections.
Responsibilities: Bounded closure, values, descriptor chains and source ownership.
This file intentionally does NOT infer runtime execution or cursor positions.
"""

from collections import defaultdict
import re

FEATURE = "source-data-streams"
LIMIT = 1000000
KEY = re.compile(r"data-(counts|group|item|payload|descriptor|read|read-runtime|restore):([1-9][0-9]*)(?::(0|[1-9][0-9]*))?\Z")


def validate_data(model, number, unescape, subject_modules):
    modules = defaultdict(dict)
    expressions = {int(row[1]): row for row in model.records["E"]}
    next_links, read_links, locations, origins = {}, {}, [], []
    module = 0

    def decimal(text, minimum=0, maximum=LIMIT):
        value = number(text, minimum)
        if str(value) != text or value > maximum:
            raise ValueError("DATA identity or count is not canonical or bounded")
        return value

    def symbol(identity, current_module, kind=None, nullable=False):
        if identity == 0 and nullable:
            return
        row = model.symbols.get(identity)
        if (row is None or subject_modules.get(("symbol", identity)) != current_module
                or kind is not None and row[3] != kind):
            raise ValueError("DATA observation has an invalid or foreign symbol")

    def source(fields, current_module):
        statement, context = (decimal(value, 1, 4294967295) for value in fields[:2])
        native = model.statements.get(statement)
        opened = model.source_contexts.get(context)
        if (native is None or opened is None or native[5] != str(context)
                or native[7] != str(current_module) or opened[4] != str(current_module)):
            raise ValueError("DATA observation has another statement or source")
        return statement, context

    for row in model.rows:
        if row[0] == "M":
            module += 1
        elif row[0] == "K" and row[3].startswith("data-"):
            match = KEY.fullmatch(row[3])
            if match is None:
                raise ValueError("Unknown DATA property")
            kind, identity, ordinal = match.groups()
            identity = decimal(identity, 1, 4294967295 if kind == "descriptor" else LIMIT)
            if (ordinal is not None) != (kind in ("item", "payload")):
                raise ValueError("Invalid DATA property ordinal")
            ordinal = decimal(ordinal) if ordinal is not None else None
            owner = decimal(row[2], 1, 4294967295)
            symbol(owner, module, "8")
            if (row[1] != "symbol" or model.symbols[owner][11] != "0"
                    or model.capabilities[module].get(FEATURE) != "available"):
                raise ValueError("DATA property lacks its global namespace or capability")
            facts = modules[module]
            if facts.setdefault("owner", owner) != owner:
                raise ValueError("DATA properties change namespace owner")
            bucket = facts.setdefault(kind, {})
            key = (identity, ordinal) if ordinal is not None else identity
            if key in bucket or len(bucket) >= LIMIT:
                raise ValueError("Repeated or oversized DATA property family")
            bucket[key] = row[4].split("\t")
        elif row[0] == "H" and (row[5] == "data-next" or row[5].startswith("data-read-runtime:")):
            if row[1] != "symbol" or row[3] != "symbol" or row[6] != "0":
                raise ValueError("Invalid DATA relationship domains or ordinal")
            start, target = (decimal(value, 1, 4294967295) for value in (row[2], row[4]))
            symbol(start, module)
            symbol(target, module)
            links = next_links if row[5] == "data-next" else read_links
            key = (module, start) if row[5] == "data-next" else (module, row[5])
            if key in links:
                raise ValueError("Repeated DATA relationship")
            links[key] = (start, target)
        elif row[0] == "LOC" and row[3].startswith(("data-item:", "data-read:")):
            locations.append((module, row))
        elif row[0] == "MR" and row[4].startswith(("data-item:", "data-read:")):
            origins.append((module, row))

    model.data_streams = {}
    model.data_ast_payloads = {}
    expected_ranges = {}
    for module, capabilities in model.capabilities.items():
        if capabilities.get(FEATURE) != "available":
            if module in modules:
                raise ValueError("Unavailable DATA family has observations")
            continue
        facts = modules[module]
        closures = facts.get("counts", {})
        if set(closures) != {module} or len(closures[module]) != 5:
            raise ValueError("DATA count closure is missing or belongs to another module")
        counts = [decimal(value) for value in closures[module]]
        groups, items = facts.get("group", {}), facts.get("item", {})
        descriptors, payloads = facts.get("descriptor", {}), facts.get("payload", {})
        reads, runtimes, restores = facts.get("read", {}), facts.get("read-runtime", {}), facts.get("restore", {})
        if ([len(groups), len(items), len(descriptors), len(reads), len(restores)] != counts
                or list(groups) != list(range(1, counts[0] + 1))
                or list(reads) != list(range(1, counts[3] + 1))
                or list(restores) != list(range(1, counts[4] + 1)) or set(runtimes) != set(reads)):
            raise ValueError("DATA observation sequence is incomplete or reordered")
        expected_items = []
        group_descriptors = {}
        for identity, fields in groups.items():
            if len(fields) != 4:
                raise ValueError("Invalid source DATA group shape")
            statement, context = source(fields, module)
            array, count = decimal(fields[2], 1, 4294967295), decimal(fields[3])
            symbol(array, module, "1")
            if array in group_descriptors:
                raise ValueError("Several DATA groups claim the same native descriptor")
            group_descriptors[array] = identity
            if count > counts[1] - len(expected_items):
                raise ValueError("DATA group item counts exceed the module closure")
            for ordinal in range(count):
                expected_items.append((identity, ordinal))
                expected_ranges[module, f"data-item:{identity}:{ordinal}"] = (facts["owner"], context)
        if list(items) != expected_items or list(payloads) != expected_items:
            raise ValueError("DATA items or payloads are missing, uncounted or reordered")
        previous = 0
        seen_groups = set()
        for array, fields in descriptors.items():
            if len(fields) != 4:
                raise ValueError("Invalid native DATA descriptor shape")
            prior, label, group, cells = (decimal(value, 0, 4294967295) for value in fields)
            symbol(array, module, "1")
            symbol(label, module, "7", nullable=True)
            if prior != previous or cells < 1 or cells > LIMIT + 1:
                raise ValueError("DATA descriptor chain is not in native construction order")
            if prior:
                if next_links.pop((module, prior), None) != (prior, array):
                    raise ValueError("DATA descriptor lacks its actual forward link")
            if group:
                if (group in seen_groups or group_descriptors.get(array) != group
                        or int(groups[group][3]) + 1 != cells):
                    raise ValueError("DATA descriptor disagrees with its source group")
                seen_groups.add(group)
            elif cells != 1:
                raise ValueError("Synthetic empty DATA descriptor claims source items")
            previous = array
        if seen_groups != set(groups):
            raise ValueError("Source DATA group lacks its native descriptor")
        for key, fields in items.items():
            if len(fields) != 8 or len(payloads[key]) != 2:
                raise ValueError("Invalid DATA item or payload shape")
            kind, dtype, subtype, expression, value_kind, value_text, address, offset = fields
            if kind not in ("number", "string", "wide-string", "address", "qb-raw"):
                raise ValueError("Unknown source DATA item kind")
            dtype, subtype, expression, address = [decimal(value, 0, 4294967295)
                                                   for value in (dtype, subtype, expression, address)]
            symbol(subtype, module, nullable=True)
            number(offset)
            value_text = unescape(value_text)
            tag, numeric_text = decimal(payloads[key][0], 0, 65535), unescape(payloads[key][1])
            if kind == "qb-raw":
                if expression != 0 or value_kind != "bytes":
                    raise ValueError("QB DATA invents a parsed expression")
            else:
                selected = expressions.get(expression)
                if (selected is None or subject_modules.get(("expression", expression)) != module
                        or selected[12] != str(dtype) or selected[14] != str(subtype)
                        or model.statement_owners.get(("expression", expression)) != int(groups[key[0]][0])):
                    raise ValueError("DATA item disagrees with its original expression type")
                retained = model.constants.get(("expression", expression))
                if retained != ((value_kind, value_text) if value_kind else None):
                    raise ValueError("DATA item disagrees with its original constant")
            if kind in ("string", "wide-string", "qb-raw"):
                unit = 8 if kind == "wide-string" else 2
                if (value_kind != ("wide-units" if unit == 8 else "bytes")
                        or len(value_text) % unit or re.fullmatch(r"[0-9A-F]*", value_text) is None
                        or tag != (len(value_text) // unit + (32768 if unit == 8 else 0)) % 65536
                        or numeric_text):
                    raise ValueError("DATA string bytes, units or lowered length tag are invalid")
            elif kind == "number":
                if (value_kind not in ("signed", "unsigned", "float64-bits", "")
                        or value_kind == "" and value_text or not numeric_text
                        or tag != len(numeric_text.encode("ascii")) % 65536):
                    raise ValueError("Numeric DATA payload or lowered length tag is invalid")
                if value_kind in ("signed", "unsigned"):
                    if re.fullmatch(r"-?[0-9]+", numeric_text) is None or str(int(numeric_text)) != value_text:
                        raise ValueError("Numeric DATA text disagrees with its original integer")
            elif (not address or tag != 65534 or value_kind or value_text or numeric_text):
                raise ValueError("Address DATA item has an invalid native target or tag")
            if kind == "address":
                symbol(address, module)
            elif address or offset != "0":
                raise ValueError("Non-address DATA item invents an address target")
        statement_ordinals = defaultdict(int)
        for identity, fields in reads.items():
            if len(fields) != 4 or len(runtimes[identity]) != 1:
                raise ValueError("Invalid READ input or selected runtime shape")
            statement, context = source(fields, module)
            ordinal = decimal(fields[2])
            expression = decimal(fields[3], 1, 4294967295)
            if (ordinal != statement_ordinals[statement] or subject_modules.get(("expression", expression)) != module
                    or model.statement_owners.get(("expression", expression)) != statement):
                raise ValueError("READ destination order or expression ownership is invalid")
            statement_ordinals[statement] += 1
            proc = decimal(runtimes[identity][0], 1, 4294967295)
            symbol(proc, module, "3")
            if read_links.pop((module, f"data-read-runtime:{identity}"), None) != (facts["owner"], proc):
                raise ValueError("READ lacks its selected runtime relationship")
            expected_ranges[module, f"data-read:{identity}"] = (facts["owner"], context)
        for fields in restores.values():
            if len(fields) != 4:
                raise ValueError("Invalid RESTORE selection shape")
            source(fields, module)
            label, array = (decimal(value, 0, 4294967295) for value in fields[2:])
            symbol(label, module, "7", nullable=True)
            symbol(array, module, "1")
            native = descriptors.get(array)
            if label and native is not None and native[1] != str(label):
                raise ValueError("RESTORE label disagrees with its selected DATA descriptor")
            if not label and descriptors and array != next(iter(descriptors)):
                raise ValueError("Blank RESTORE does not select the first native descriptor")
        model.data_streams[module] = facts
        # These are interpretations of captured transient payloads, not new
        # AST nodes. DATA item kinds are selected from the active native id
        # before lowering; descriptor cells copy the root's elmts after its
        # final LINK cell was appended. NULL is only a lowered sentinel.
        native_ids = {"string": 1, "qb-raw": 1, "number": 2, "wide-string": 32768, "address": 65534}
        for group, fields in groups.items():
            descriptor = int(fields[2])
            count = int(fields[3])
            model.data_ast_payloads[module, group] = {
                "item_ids": tuple(native_ids[items[group, ordinal][0]] for ordinal in range(count)),
                "trailer_id": 65535,
                "container_elements": int(descriptors[descriptor][3]),
            }
    if next_links or read_links:
        raise ValueError("DATA relationship has no matching native observation")
    for module, row in locations:
        expected = expected_ranges.get((module, row[3]))
        if expected is None or row[1] != "source-context" or row[2] != str(expected[1]):
            raise ValueError("DATA physical range has a foreign or missing source item")
    for module, row in origins:
        expected = expected_ranges.get((module, row[4]))
        if expected is None or row[1] != "symbol" or row[2] != str(expected[0]):
            raise ValueError("DATA macro origin has a foreign or missing source item")

# end of semantic_data.py
