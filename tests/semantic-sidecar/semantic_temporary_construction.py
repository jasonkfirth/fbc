"""Project: FreeBASIC semantic sidecar
File: semantic_temporary_construction.py
Purpose: Validate original explicit constructor expressions before lowering.
Responsibilities: Selected record/constructor, source receipts and complete counts.
This file intentionally does NOT impose configured linter type policies.
"""

FEATURE = 'temporary-construction-inputs'
PREFIX = 'temporary-construction:'
CLOSURE = 'temporary-construction-count:'
LIMIT = 1000000


def validate_temporary_construction(model, number, subject_modules):
    counts, closures, claimed = {}, {}, set()
    work = 8000000

    def decimal(text, minimum=0):
        value = number(text, minimum)
        if str(value) != text:
            raise ValueError('Temporary construction identity is not canonical')
        return value

    locations = {(row[1], row[2], row[3]): row for row in model.records['LOC']}
    origins = {(row[1], row[2], row[4]): row for row in model.records['MR']}
    for row in model.records['K']:
        work -= 1
        key = row[3]
        if not key.startswith((PREFIX, CLOSURE)):
            continue
        work -= len(key) + len(row[4])
        owner = decimal(row[2], 1)
        module = subject_modules.get(('symbol', owner))
        if (work < 0 or row[1] != 'symbol' or module is None
                or model.capabilities[module].get(FEATURE) != 'available'
                or len(key) > 64 or len(row[4]) > 128):
            raise ValueError('Temporary construction lacks its owner or capability')
        if key.startswith(CLOSURE):
            count = decimal(row[4])
            if (decimal(key[len(CLOSURE):], 1) != module or module in closures or count > LIMIT
                    or model.symbols[owner][3] != '8' or model.symbols[owner][11] != '0'):
                raise ValueError('Invalid temporary construction module closure')
            closures[module] = count
            continue
        ordinal = decimal(key[len(PREFIX):], 1)
        fields = row[4].split('\t')
        if len(fields) != 4 or ordinal > LIMIT or ordinal != counts.get(module, 0) + 1:
            raise ValueError('Invalid temporary construction observation order or shape')
        target, statement, source = [decimal(value, 1) for value in fields[:3]]
        macro = decimal(fields[3])
        selected = model.symbols.get(target)
        selected_type = model.types.get(target)
        native = model.statements.get(statement)
        context = model.source_contexts.get(source)
        if (model.symbols[owner][3] != '10' or selected is None or selected[3] != '3'
                or selected[11] != str(owner) or selected_type is None or not int(selected_type[11]) & 4
                or subject_modules.get(('symbol', target)) != module or native is None
                or native[5] != str(source) or native[7] != str(module)
                or context is None or context[4] != str(module)):
            raise ValueError('Temporary construction contradicts its native selection or source')
        location = locations.get(('source-context', str(source), key))
        origin = origins.get(('symbol', str(owner), key))
        if location is not None:
            if location[4] != str(source) or ('LOC', source, key) in claimed:
                raise ValueError('Temporary construction location has another source')
            claimed.add(('LOC', source, key))
        if macro:
            invocation = model.macro_invocations.get(macro)
            if (origin is None or origin[3] != str(macro) or invocation is None
                    or invocation[4] != str(source) or ('MR', owner, key) in claimed):
                raise ValueError('Temporary construction lacks its matching macro origin')
            claimed.add(('MR', owner, key))
        elif location is None or origin is not None:
            raise ValueError('Temporary construction lacks its physical opening')
        counts[module] = ordinal
    for module, capabilities in model.capabilities.items():
        if capabilities.get(FEATURE) == 'available' and closures.get(module) != counts.get(module, 0):
            raise ValueError('Temporary construction coverage is incomplete')
    for tag, field, subject_field in (('LOC', 3, 2), ('MR', 4, 2)):
        for row in model.records[tag]:
            work -= 1
            if row[field].startswith(PREFIX) and (tag, int(row[subject_field]), row[field]) not in claimed:
                raise ValueError('Temporary construction omits an independently retained opening')
    if work < 0:
        raise ValueError('Temporary construction validation budget exceeded')

# end of semantic_temporary_construction.py
