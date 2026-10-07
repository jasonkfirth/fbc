"""Project: FreeBASIC semantic sidecar tests
File: semantic_string_declarations.py
Purpose: Validate scalar dynamic String declarations and their original RHS.
Responsibilities: Accepted storage, statement ownership and complete coverage.
This file intentionally does NOT parse declarations or implement lint policy.
"""


def validate_string_declarations(model, number, subject_modules):
    inputs, declarations = {}, {}
    array_symbols = {int(row[1]) for row in model.records['A']}

    def available(module):
        return model.capabilities[module].get('scalar-string-declarations') == 'available'

    def bounded(text, minimum=1):
        value = number(text, minimum)
        if value > len(model.rows):
            raise ValueError('Scalar String identity exceeds the detail stream')
        return value

    def storage(identity):
        symbol = model.symbols.get(identity)
        return symbol is not None and symbol[3] in ('1', '12') and symbol[4:6] == ['17', '0'] and not int(symbol[7]) & 0x40000 and identity not in array_symbols

    for module, capabilities in model.capabilities.items():
        if available(module) and capabilities.get('declaration-typing-inputs') != 'available':
            raise ValueError('Scalar String declarations lack declaration typing coverage')
    for row in model.records['K']:
        domain, owner, key, payload = row[1], int(row[2]), row[3], row[4]
        if not key.startswith('scalar-string-'):
            continue
        module = subject_modules.get((domain, owner))
        fields = payload.split('\t')
        if key != 'scalar-string-declaration' or domain != 'symbol' or not storage(owner) or owner in inputs or not available(module) or len(fields) != 4:
            raise ValueError('Invalid scalar String declaration owner or payload')
        statement, expression, source = bounded(fields[0]), bounded(fields[1], 0), bounded(fields[3])
        record, context = model.statements.get(statement), model.source_contexts.get(source)
        if record is None or context is None or int(record[7]) != module or int(context[4]) != module or int(record[5]) != source or model.statement_endings[statement][3] != 'parsed':
            raise ValueError('Scalar String declaration lacks an accepted same-source statement')
        kind, symbol = fields[2], model.symbols[owner]
        if symbol[3] == '12':
            block = model.constructs.get(int(record[3]))
            body = model.properties['symbol', int(symbol[11])].get('aggregate-body-statement')
            if kind != 'field' or block is None or body != block[3] or model.statement_endings[statement][2] != 'aggregate-member':
                raise ValueError('Scalar String field belongs to another aggregate')
        elif kind not in ('dim', 'static'):
            raise ValueError('Scalar String declaration has another storage grammar')
        if expression and (subject_modules.get(('expression', expression)) != module or model.statement_owners.get(('expression', expression)) != statement):
            raise ValueError('Scalar String initializer belongs to another declaration')
        inputs[owner] = statement, expression, kind
    groups = {}
    for row in model.records['K']:
        if row[3].startswith('declaration-type-group:'):
            fields = row[4].split('\t')
            groups[int(row[3].split(':')[1])] = int(fields[0]), fields[1]
    for row in model.records['K']:
        if not row[3].startswith('declaration-type-input:'):
            continue
        fields = row[4].split('\t')
        owner = int(fields[0])
        statement, kind = groups[int(row[3].split(':')[1])]
        if not available(subject_modules['symbol', owner]) or kind not in ('dim', 'static') or not storage(owner):
            continue
        entry = inputs.get(owner)
        if entry is None or owner in declarations or entry[0] != statement or entry[2] != kind or (fields[2] == '0') != (entry[1] == 0) or fields[2] not in ('0', '2'):
            raise ValueError('Scalar String initialization differs from its declaration grammar')
        declarations[owner] = entry
    for owner, (_, _, kind) in inputs.items():
        if kind != 'field' and owner not in declarations:
            raise ValueError('Scalar String observation lacks its DIM or STATIC declaration')
    for row in model.records['B']:
        owner = int(row[1])
        if row[2] == 'declaration' and available(subject_modules['symbol', owner]) and storage(owner) and model.symbols[owner][3] == '12' and owner not in inputs:
            raise ValueError('Scalar String field lacks its initialization observation')

# end of semantic_string_declarations.py
