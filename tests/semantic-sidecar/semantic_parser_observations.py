"""Project: FreeBASIC semantic sidecar
File: semantic_parser_observations.py
Purpose: Validate native parser observations independently of extra snapshots.
Responsibilities: Ordered closure, expression links, values, and module ownership.
This file intentionally does NOT infer parser events from expression ranges.
"""

from collections import defaultdict
import hashlib
import re

FEATURE = "parser-expression-observations"
PREFIX = "parser-observation:"
COUNT = "parser-observation-count"
MAX_EVENTS = 4000000


def is_parser_property(key):
    return key == COUNT or key.startswith(PREFIX)


def validate_parser_observations(model, number, unescape, subject_modules, property_modules, node_kinds):
    expressions = {int(row[1]): row for row in model.records["E"]}
    module_expressions = defaultdict(list)
    for identity in expressions:
        module_expressions[subject_modules["expression", identity]].append(identity)
    events = defaultdict(dict)
    closures = {}
    for row in model.records["K"]:
        if not is_parser_property(row[3]):
            continue
        identity = number(row[2], 1)
        module = subject_modules.get(("expression", identity))
        if (row[1] != "expression" or identity not in expressions or module is None
                or property_modules.get((identity, row[3])) != module
                or model.capabilities[module].get(FEATURE) != "available"):
            raise ValueError("Parser observation has a foreign, missing, or unavailable result")
        if row[3] == COUNT:
            count = number(row[4], 0)
            if count > MAX_EVENTS or module in closures or identity != max(module_expressions[module]):
                raise ValueError("Invalid parser observation closure")
            closures[module] = count
            continue
        ordinal = number(row[3][len(PREFIX):], 1)
        if ordinal > MAX_EVENTS or row[3] != PREFIX + str(ordinal) or ordinal in events[module]:
            raise ValueError("Repeated or noncanonical parser observation ordinal")
        fields = tuple(unescape(value) for value in row[4].split("\t"))
        if len(fields) != 12 or not fields[0]:
            raise ValueError("Invalid parser observation payload")
        for index in (1, 2, 3, 4):
            number(fields[index], 0)
        kind, op, dtype = number(fields[5], 0), number(fields[6], -1), number(fields[7], 0)
        result = expressions[identity]
        source_operation = model.expression_operands.get(identity)
        # Statement-only Cva_Start/Copy/End use native MACRO class 44 and
        # the unsigned INVALID datatype sentinel. Their accepted EX operation
        # independently proves why no value type exists; Cva_Arg stays typed.
        untyped_statement = (kind == 44 and op == -1 and dtype == 2147483648
                             and source_operation is not None and source_operation[2] == "runtime-macro"
                             and source_operation[3] in ("va-start", "va-copy", "va-end")
                             and result[10:12] == [source_operation[3], "builtin"])
        if kind >= node_kinds or not fields[8] and not untyped_statement:
            raise ValueError("Invalid parser observation AST kind or type")
        number(fields[9], -1)
        if [str(kind), str(op), str(dtype), fields[8]] != [result[8], result[9], result[12], result[15]]:
            raise ValueError("Parser observation disagrees with its typed result")
        value_kind, value = fields[10:]
        if untyped_statement and (value_kind or value):
            raise ValueError("Statement-only parser observation invents a value")
        if value_kind:
            if value_kind != "sha256" or re.fullmatch(r"[0-9a-f]{64}", value) is None:
                raise ValueError("Invalid parser observation value fingerprint")
        elif value:
            raise ValueError("Unknown parser observation value fingerprint")
        retained = model.constants.get(("expression", identity))
        if retained is not None:
            digest = hashlib.sha256((retained[0] + "\t" + retained[1]).encode("ascii")).hexdigest()
            if (value_kind, value) != ("sha256", digest):
                raise ValueError("Parser observation disagrees with its retained value")
        events[module][ordinal] = fields
    model.parser_observations = {}
    for module, capabilities in model.capabilities.items():
        if capabilities.get(FEATURE) != "available":
            continue
        if module_expressions[module] and module not in closures:
            raise ValueError("Parser observation closure is missing")
        count = closures.get(module, 0)
        if len(events[module]) != count or sorted(events[module]) != list(range(1, count + 1)):
            raise ValueError("Parser observation sequence is incomplete")
        model.parser_observations[module] = tuple(events[module][ordinal] for ordinal in range(1, count + 1))

# end of semantic_parser_observations.py
