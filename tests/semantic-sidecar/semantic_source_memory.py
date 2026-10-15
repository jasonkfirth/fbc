"""Project: FreeBASIC semantic sidecar tests
File: semantic_source_memory.py
Purpose: Validate selected source allocation/deallocation applications.
Responsibilities: Complete occurrences, declaration contexts and macro anchors.
This file intentionally does NOT infer callers or allocation ownership policy.
"""
from pathlib import PureWindowsPath

FEATURE = 'source-memory-applications'
PREFIX = 'source-memory-application:'
CLOSURE = 'source-memory-application-count'
ROLES = {'allocate', 'allocate-array', 'deallocate', 'deallocate-array'}
RELATIONS = {'source-memory-target', 'source-memory-default-formal', 'source-memory-field'}
LIMIT = 1000000


def validate_source_memory(model, number, subject_modules):
    applications, closures, owners, counts = {}, {}, {}, {}
    relations, locations, origins = {}, {}, {}
    module = 0
    work_left = len(model.rows) * 4 + 8000000

    def decimal(text, minimum=0):
        value = number(text, minimum)
        if str(value) != text or value > 4294967295:
            raise ValueError('Source memory identity is not canonical or bounded')
        return value

    def owner(identity, current_module):
        symbol = model.symbols.get(identity)
        if (symbol is None or symbol[3] != '8' or symbol[11] != '0'
                or subject_modules.get(('symbol', identity)) != current_module
                or model.capabilities[current_module].get(FEATURE) != 'available'):
            raise ValueError('Source memory application lacks its module owner or capability')
        if owners.setdefault(current_module, identity) != identity:
            raise ValueError('Source memory application changes namespace owner')

    for index, row in enumerate(model.rows):
        work_left -= 1
        if work_left < 0:
            raise ValueError('Source memory validation work limit exceeded')
        if row[0] == 'M':
            module += 1
        elif row[0] == 'K' and (row[3] == CLOSURE or row[3].startswith(PREFIX)):
            namespace = decimal(row[2], 1)
            owner(namespace, module)
            if row[1] != 'symbol':
                raise ValueError('Source memory observation has an invalid domain')
            if row[3] == CLOSURE:
                value = decimal(row[4])
                if module in closures or value > LIMIT:
                    raise ValueError('Source memory closure is repeated or oversized')
                closures[module] = value
                continue
            ordinal = decimal(row[3][len(PREFIX):], 1)
            if ordinal != counts.get(module, 0) + 1 or ordinal > LIMIT or len(row[4]) > 160:
                raise ValueError('Source memory occurrences are reordered or oversized')
            fields = row[4].split('\t')
            if len(fields) != 5 or fields[0] not in ROLES or fields[4] not in ('direct', 'formal-default', 'field-initializer'):
                raise ValueError('Source memory application has an invalid payload')
            statement, source, macro = [decimal(value, 1 if slot < 2 else 0) for slot, value in enumerate(fields[1:4])]
            native = model.statements.get(statement)
            context = model.source_contexts.get(source)
            if (native is None or context is None or native[5] != str(source)
                    or native[7] != str(module) or context[4] != str(module)):
                raise ValueError('Source memory application has another statement or source')
            if index < 1 or model.rows[index - 1][0] != 'O':
                raise ValueError('Source memory application lacks its adjacent selected operation')
            operation = model.rows[index - 1]
            if operation[1] != fields[0] or operation[2] != 'overloaded':
                raise ValueError('Source memory application contradicts its selected operation')
            applications[module, ordinal] = (namespace, fields[0], source, macro, fields[4], operation)
            counts[module] = ordinal
        elif row[0] == 'H' and row[5].startswith('source-memory-'):
            if row[5] not in RELATIONS or row[1] != 'symbol' or row[3] != 'symbol':
                raise ValueError('Unknown source memory relation or domain')
            namespace, ordinal, target = decimal(row[2], 1), decimal(row[6], 1), decimal(row[4], 1)
            owner(namespace, module)
            key = module, ordinal, row[5]
            if key in relations or subject_modules.get(('symbol', target)) != module:
                raise ValueError('Source memory relation is repeated or has another module')
            relations[key] = target
        elif row[0] == 'LOC' and row[3].startswith(PREFIX):
            if row[1] != 'source-context':
                raise ValueError('Source memory location has an invalid domain')
            key = module, decimal(row[3][len(PREFIX):], 1)
            if key in locations:
                raise ValueError('Source memory location is repeated')
            locations[key] = row
        elif row[0] == 'MR' and row[4].startswith(PREFIX):
            if row[1] != 'symbol':
                raise ValueError('Source memory origin has an invalid domain')
            key = module, decimal(row[4][len(PREFIX):], 1)
            if key in origins:
                raise ValueError('Source memory origin is repeated')
            origins[key] = row
        elif row[0] == 'O' and row[1] in ROLES and row[2] == 'overloaded':
            if model.capabilities[module].get(FEATURE) == 'available' and (
                    index + 1 >= len(model.rows) or model.rows[index + 1][0] != 'K'
                    or not model.rows[index + 1][3].startswith(PREFIX)):
                raise ValueError('Selected memory operation lacks its source application')

    for (module, ordinal), (namespace, role, source, macro, kind, operation) in applications.items():
        target = relations.pop((module, ordinal, 'source-memory-target'), None)
        signature = model.signatures.get(target)
        if (signature is None or signature[2] != 'operator' or signature[10] != role
                or operation[3] != str(target)):
            raise ValueError('Source memory target is not its selected operator')
        formal = relations.pop((module, ordinal, 'source-memory-default-formal'), None)
        field = relations.pop((module, ordinal, 'source-memory-field'), None)
        if kind == 'direct':
            if formal is not None or field is not None:
                raise ValueError('Direct source memory application has a declaration context')
        elif kind == 'formal-default':
            if formal is None or field is not None or model.symbols[formal][3] != '4':
                raise ValueError('Source memory default lacks its formal')
        elif field is None or formal is not None or model.symbols[field][3] != '12':
            raise ValueError('Source memory initializer lacks its field')
        location = locations.pop((module, ordinal), None)
        if location is None or location[2] != str(source) or location[4] != str(source):
            raise ValueError('Source memory application lacks its source location')
        path = model.files[int(model.source_contexts[source][3])][2]
        if PureWindowsPath(operation[5]) != PureWindowsPath(path):
            raise ValueError('Source memory operation path disagrees with its observed source')
        if operation[4] == '1' and (location[11] != 'mapped' or operation[6:10] != location[5:9]):
            raise ValueError('Source memory operation range disagrees with its observed source')
        origin = origins.pop((module, ordinal), None)
        if macro:
            invocation = model.macro_invocations.get(macro)
            if (origin is None or origin[2:4] != [str(namespace), str(macro)]
                    or invocation is None or invocation[4] != str(source)):
                raise ValueError('Source memory macro provenance is incomplete')
        elif origin is not None:
            raise ValueError('Direct source memory application has an extra macro origin')
    if relations or locations or origins:
        raise ValueError('Source memory metadata has no matching occurrence')
    for module, capabilities in model.capabilities.items():
        if capabilities.get(FEATURE) == 'available' and closures.get(module) != counts.get(module, 0):
            raise ValueError('Source memory observation coverage is incomplete')

# end of semantic_source_memory.py
