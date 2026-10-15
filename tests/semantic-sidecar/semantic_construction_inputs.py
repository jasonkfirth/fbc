"""Project: FreeBASIC semantic sidecar
File: semantic_construction_inputs.py
Purpose: Validate selected construction even when generated spans are unavailable.
Responsibilities: Owner/type identities, source ownership and complete module counts.
This file intentionally does NOT decide whether static construction is good policy.
"""
from collections import Counter

FEATURE = 'implicit-construction-inputs'
PREFIX = 'construction-observation:'
CLOSURE = 'construction-observation-count:'
KINDS = frozenset(('default-constructor', 'initializer-constructor',
                  'new-constructor', 'argument-constructor', 'return-constructor',
                  'redim-constructor'))
LIMIT = 1000000


def validate_construction_inputs(model, number, subject_modules):
    observed, closures = {}, {}
    selections, macro_selections = Counter(), Counter()
    work_left = 8000000

    def decimal(value, minimum=0):
        result = number(value, minimum)
        if str(result) != value:
            raise ValueError('Construction identity is not canonical')
        return result

    def canonical(identity):
        return aliases.get(identity, identity)

    aliases = {int(row[2]): int(row[4]) for row in model.records['H']
               if row[1] == 'symbol' and row[3] == 'symbol' and row[5] == 'canonical-symbol'}
    for identity in list(aliases):
        target = aliases[identity]
        visits = 0
        while target in aliases:
            visits += 1
            work_left -= 1
            if work_left < 0 or visits > len(aliases):
                raise ValueError('Construction symbol alias is cyclic or excessive')
            target = aliases[target]
        aliases[identity] = target

    for row in model.records['K']:
        work_left -= 1
        key = row[3]
        if not key.startswith((PREFIX, CLOSURE)):
            continue
        work_left -= len(key) + len(row[4])
        owner = decimal(row[2], 1)
        module = subject_modules.get(('symbol', owner))
        if (work_left < 0 or row[1] != 'symbol' or module is None
                or model.capabilities[module].get(FEATURE) != 'available'
                or len(key) > 64 or len(row[4]) > 256):
            raise ValueError('Construction observation lacks its owner or capability')
        if key.startswith(CLOSURE):
            declared_module = decimal(key[len(CLOSURE):], 1)
            count = decimal(row[4])
            symbol = model.symbols[owner]
            if (declared_module != module or module in closures or count > LIMIT
                    or symbol[3] != '8' or symbol[11] != '0'):
                raise ValueError('Invalid construction module closure')
            closures[module] = count
            continue
        ordinal = decimal(key[len(PREFIX):], 1)
        fields = row[4].split('\t')
        if len(fields) != 6 or ordinal > LIMIT:
            raise ValueError('Invalid construction observation shape')
        target, owning_type = [decimal(value, 1) for value in fields[:2]]
        kind = fields[2]
        statement, source, macro = [decimal(value) for value in fields[3:]]
        slot = module, ordinal
        target_symbol = model.symbols.get(canonical(target))
        type_symbol = model.symbols.get(canonical(owning_type))
        owner_type = owner if model.symbols[owner][3] == '10' else int(model.symbols[owner][5])
        if (slot in observed or kind not in KINDS or target_symbol is None or target_symbol[3] != '3'
                or type_symbol is None or type_symbol[3] != '10'
                or canonical(owner_type) != canonical(owning_type)
                or canonical(int(target_symbol[11])) != canonical(owning_type)
                or not int(model.types.get(canonical(target), ['0'] * 12)[11]) & 4
                or any(subject_modules.get(('symbol', value)) != module for value in (target, owning_type))):
            raise ValueError('Construction selection has contradictory native identities')
        if statement:
            native = model.statements.get(statement)
            if native is None or int(native[7]) != module or native[5] != str(source):
                raise ValueError('Construction statement or source belongs to another owner')
        if kind == 'redim-constructor' and (not statement or not source):
            raise ValueError('REDIM construction lacks its source statement')
        owner_snapshot = model.types.get(canonical(owner))
        if (owner_snapshot is not None and owner_snapshot[3] == 'variable'
                and owner_snapshot[17] in ('static', 'shared', 'global') and owner_snapshot[18] == 'source'
                and kind in ('default-constructor', 'initializer-constructor') and (not statement or not source)):
            raise ValueError('Static source construction lacks declaration ownership')
        if source and (source not in model.source_contexts or int(model.source_contexts[source][4]) != module):
            raise ValueError('Construction source belongs to another module')
        if macro:
            invocation = model.macro_invocations.get(macro)
            if invocation is None or invocation[4] != str(source):
                raise ValueError('Construction macro lacks its source invocation')
        observed[slot] = owner
        selections[module, canonical(owner), canonical(target), canonical(owning_type), kind] += 1
        if macro:
            macro_selections[canonical(target), macro] += 1
    for module, capabilities in model.capabilities.items():
        if capabilities.get(FEATURE) == 'available':
            count = closures.get(module)
            ordinals = {ordinal for observed_module, ordinal in observed if observed_module == module}
            if count is None or len(ordinals) != count or (ordinals and (min(ordinals) != 1 or max(ordinals) != count)):
                raise ValueError('Construction observation coverage is incomplete')
    # Independent legacy selections and macro origins catch missing observations
    # even if a damaged file also reduces its advertised completion count.
    for row in model.records['I']:
        work_left -= 1
        module = subject_modules.get(('symbol', int(row[1])))
        if row[4] in KINDS and model.capabilities[module].get(FEATURE) == 'available':
            key = module, canonical(int(row[1])), canonical(int(row[2])), canonical(int(row[3])), row[4]
            selections[key] -= 1
            if selections[key] < 0:
                raise ValueError('Construction observation omits a selected implicit call')
    for row in model.records['MR']:
        work_left -= 1
        if row[1] == 'symbol' and row[4].startswith('construction-'):
            target, macro = int(row[2]), int(row[3])
            module = subject_modules.get(('symbol', target))
            if model.capabilities[module].get(FEATURE) == 'available' and not macro_selections[canonical(target), macro]:
                raise ValueError('Construction observation omits a selected macro call')
    if work_left < 0:
        raise ValueError('Construction validation work budget exceeded')

# end of semantic_construction_inputs.py
