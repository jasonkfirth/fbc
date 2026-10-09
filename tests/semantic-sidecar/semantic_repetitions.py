"""Project: FreeBASIC semantic sidecar tests
File: semantic_repetitions.py
Purpose: Validate occurrence-specific accepted declaration repetitions.
Responsibilities: Typed ownership, parser coverage, ordering and bounded identities.
This file intentionally does NOT parse source or decide whether a declaration repeats.
"""


def validate_declaration_repetitions(model, number, subject_modules):
    observations, seen_ids, previous, property_keys = {}, set(), set(), {}
    prefix = 'redeclaration-input:'

    def available(module):
        return model.capabilities[module].get('declaration-repetition-inputs') == 'available'

    def bounded(text):
        value = number(text, 1)
        if value > len(model.rows):
            raise ValueError('Declaration repetition identity exceeds the model')
        return value

    def expect(statement, kind, owner):
        key = statement, kind, owner
        entries = observations.get(key)
        if not entries:
            raise ValueError('Declaration repetition coverage is incomplete')
        entries.pop()

    for module, capabilities in model.capabilities.items():
        if available(module) and any(capabilities.get(feature) != 'available' for feature in
                                     ('declaration-typing-inputs', 'procedure-typing-inputs')):
            raise ValueError('Declaration repetitions lack their declaration or statement coverage')
    for row in model.records['K']:
        domain, owner, key, payload = row[1], int(row[2]), row[3], row[4]
        if not key.startswith('redeclaration-'):
            continue
        if not key.startswith(prefix):
            raise ValueError('Unknown declaration repetition property')
        identity = bounded(key[len(prefix):])
        fields = payload.split('\t')
        module = subject_modules.get((domain, owner))
        if identity in seen_ids or domain != 'symbol' or len(fields) != 4 or not available(module):
            raise ValueError('Invalid declaration repetition identity or payload')
        seen_ids.add(identity)
        statement, source = bounded(fields[0]), bounded(fields[3])
        kind, repeated = fields[1:3]
        classes = {'extern': '1', 'typedef': '13', 'prototype': '3'}
        symbol, record, context = model.symbols.get(owner), model.statements.get(statement), model.source_contexts.get(source)
        if kind not in classes or repeated not in ('0', '1') or symbol is None or symbol[3] != classes[kind]:
            raise ValueError('Declaration repetition belongs to another symbol class')
        if record is None or context is None or int(record[7]) != module or int(context[4]) != module or int(record[5]) != source or model.statement_endings[statement][3] != 'parsed':
            raise ValueError('Declaration repetition lacks its accepted source statement')
        history = kind, owner
        if repeated == '1' and history not in previous:
            raise ValueError('Repeated declaration has no previous declaration')
        previous.add(history)
        observations.setdefault((statement, kind, owner), []).append(identity)
        property_keys[identity] = key

    groups = {}
    for row in model.records['K']:
        if row[3].startswith('declaration-type-group:'):
            fields = row[4].split('\t')
            groups[int(row[3].split(':')[1])] = int(fields[0]), fields[1]
    for row in model.records['K']:
        module = subject_modules.get((row[1], int(row[2])))
        if not available(module):
            continue
        if row[3].startswith('declaration-type-input:'):
            fields = row[4].split('\t')
            statement, kind = groups[int(row[3].split(':')[1])]
            if kind == 'extern':
                expect(statement, kind, int(fields[0]))
        elif row[3].startswith('procedure-typing-input:'):
            fields = row[4].split('\t')
            if fields[1] == 'prototype':
                expect(int(fields[0]), 'prototype', int(row[2]))
    for ordinal, row in enumerate(model.records['B'], 1):
        owner = int(row[1])
        if row[2] == 'declaration' and model.symbols[owner][3] == '13' and available(subject_modules['symbol', owner]):
            expect(model.statement_owners['binding', ordinal], 'typedef', owner)
    origins = {(row[1], int(row[2]), row[4]): int(row[3]) for row in model.records['MR']}
    macro_parents = {int(row[1]): int(row[2]) for row in model.records['MI']}
    covered_macros = set()
    for (statement, kind, owner), entries in observations.items():
        if kind != 'typedef':
            continue
        for identity in entries.copy():
            invocation = origins.get(('symbol', owner, property_keys[identity]))
            native = ('symbol', owner, 'declaration-' + str(invocation))
            statement_origin = origins.get(('statement', statement, 'start'))
            current = invocation
            while current and current != statement_origin:
                current = macro_parents[current]
            if not invocation or origins.get(native) != invocation or not statement_origin or current != statement_origin:
                raise ValueError('Generated alias lacks its native declaration and statement origin')
            covered_macros.add(native)
            entries.remove(identity)
    for origin in origins:
        domain, owner, role = origin
        if domain == 'symbol' and role.startswith('declaration-') and model.symbols[owner][3] == '13' and available(subject_modules[domain, owner]) and origin not in covered_macros:
            raise ValueError('Generated alias lacks its repetition observation')
    if any(observations.values()):
        raise ValueError('Declaration repetition lacks its native declaration')

# end of semantic_repetitions.py
