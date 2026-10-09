"""Project: FreeBASIC semantic sidecar
File: semantic_assignment_inputs.py
Purpose: Validate original operands of accepted source assignments.
Responsibilities: Statement counts, selected operations, origins and ownership.
This file intentionally does NOT recognize assignment syntax in source text.
"""

PREFIXES = ('assignment-count:', 'assignment-input:', 'let-slot:')
COMPOUND_CODES = frozenset((
    'add', 'subtract', 'multiply', 'divide',
    'integer-divide', 'modulo', 'and', 'or',
    'logical-and', 'logical-or', 'xor',
    'equivalence', 'implication', 'shift-left',
    'shift-right', 'power', 'concatenate'))
MAX_INPUTS = 2097152


def validate_assignment_inputs(model, number, subject_modules):
    properties, markers, counts, inputs, positions = {}, set(), {}, {}, {}
    let_slots = {}
    work_left = 8000000

    def decimal(text, minimum=0):
        value = number(text, minimum)
        if str(value) != text:
            raise ValueError('Assignment input identity is not canonical')
        return value

    for position, row in enumerate(model.records['K']):
        work_left -= 1
        if work_left < 0:
            raise ValueError('Assignment property work budget exceeded')
        if not row[3].startswith(PREFIXES):
            continue
        work_left -= len(row[3]) + len(row[4])
        owner = decimal(row[2], 1)
        module = subject_modules.get(('symbol', owner))
        if (work_left < 0 or row[1] != 'symbol' or owner not in model.symbols
                or model.symbols[owner][3] != '3'
                or model.capabilities.get(module, {}).get('source-assignment-inputs') != 'available'
                or (owner, row[3]) in properties):
            raise ValueError('Assignment property is repeated, unavailable or lacks its procedure')
        parts = row[3].split(':')
        kind = parts[0]
        if len(parts) != (2 if kind == 'assignment-count' else 3):
            raise ValueError('Assignment property key has an invalid shape')
        statement = decimal(parts[1], 1)
        start = model.statements.get(statement)
        ending = model.statement_endings.get(statement)
        if (start is None or ending is None or int(start[4]) != owner
                or int(start[7]) != module):
            raise ValueError('Assignment property belongs to another statement or module')
        properties[owner, row[3]] = row
        positions[owner, row[3]] = position
        if kind == 'assignment-count':
            count = decimal(row[4])
            if count > MAX_INPUTS or statement in counts:
                raise ValueError('Assignment count is repeated or exceeds its bound')
            counts[statement] = owner, count
        elif kind == 'let-slot':
            ordinal = decimal(parts[2], 1)
            fields = row[4].split('\t')
            if (len(fields) != 2 or ordinal > MAX_INPUTS
                    or model.capabilities[module].get('let-destination-inputs') != 'available'):
                raise ValueError('LET slot payload or capability is invalid')
            slot, field = decimal(fields[0], 1), decimal(fields[1], 1)
            if (slot > 65536 or field not in model.symbols
                    or model.symbols[field][3] != '12'
                    or subject_modules.get(('symbol', field)) != module):
                raise ValueError('LET slot lacks its selected field')
            let_slots[statement, ordinal] = slot, field, row
        else:
            ordinal = decimal(parts[2], 1)
            fields = row[4].split('\t')
            if len(fields) != 6 or ordinal > MAX_INPUTS or ending[3] != 'parsed':
                raise ValueError('Assignment input payload or statement outcome is invalid')
            left, right = decimal(fields[0], 1), decimal(fields[1], 1)
            target = decimal(fields[5])
            if (fields[2] not in ('copy', 'compound', 'initializer', 'let')
                    or (fields[2] in ('copy', 'initializer', 'let') and fields[3] != 'assign')
                    or (fields[2] == 'let' and
                        (model.capabilities[module].get('let-destination-inputs') != 'available'
                         or ending[2] != 'pointer-or-assignment'))
                    or (fields[2] == 'initializer' and
                        (model.capabilities[module].get('assignment-initializers') != 'available'
                         or fields[4] != 'builtin' or target != 0 or ending[2] != 'declaration'))
                    or (fields[2] == 'compound' and fields[3] not in COMPOUND_CODES)
                    or fields[4] not in ('builtin', 'overloaded')
                    or (fields[4] == 'builtin' and target != 0)):
                raise ValueError('Assignment input has an invalid selected operation')
            if fields[4] == 'overloaded':
                signature = model.signatures.get(target)
                if (signature is None or signature[2] != 'operator'
                        or subject_modules.get(('symbol', target)) != module):
                    raise ValueError('Assignment input lacks its selected operator')
            for operand in (left, right):
                if (subject_modules.get(('expression', operand)) != module
                        or model.statement_owners.get(('expression', operand)) != statement):
                    raise ValueError('Assignment operand belongs to another statement or module')
            inputs[statement, ordinal] = row

    for row in model.records['H']:
        work_left -= 1 + len(row[5])
        if work_left < 0:
            raise ValueError('Assignment marker work budget exceeded')
        role = row[5]
        if role == 'assignment-count':
            key = role + ':' + row[6]
        elif role.startswith(('assignment-input:', 'let-slot:')):
            kind, suffix = role.split(':', 1)
            ordinal = decimal(suffix, 1)
            key = kind + ':' + row[6] + ':' + str(ordinal)
        else:
            continue
        owner = decimal(row[2], 1)
        decimal(row[6], 1)
        if (row[1] != 'symbol' or row[3] != 'symbol' or row[4] != row[2]
                or (owner, key) not in properties or (owner, key) in markers):
            raise ValueError('Assignment marker is repeated or lacks its property')
        markers.add((owner, key))
    if set(properties) != markers:
        raise ValueError('Assignment property lacks its independent occurrence marker')

    expressions = {int(row[1]): row for row in model.records['E']}
    expected_let = {key for key, row in inputs.items() if row[4].split('\t')[2] == 'let'}
    if set(let_slots) != expected_let:
        raise ValueError('LET destination slot coverage is incomplete')
    previous_slots = {}
    for key, (slot, field, row) in let_slots.items():
        statement, ordinal = key
        right = expressions[int(inputs[key][4].split('\t')[1])]
        if (right[8] != '19' or int(right[13]) != field
                or slot <= previous_slots.get(statement, 0)
                or positions[int(row[2]), row[3]] >= positions[int(row[2]), 'assignment-count:' + str(statement)]):
            raise ValueError('LET slot order or original field is invalid')
        previous_slots[statement] = slot

    origins = set()
    for tag, role_field in (('LOC', 3), ('MR', 4)):
        for row in model.records[tag]:
            work_left -= 1
            if work_left < 0:
                raise ValueError('Assignment origin work budget exceeded')
            if not row[role_field].startswith('assignment-operator:'):
                continue
            statement = decimal(row[2], 1)
            ordinal = decimal(row[role_field][len('assignment-operator:'):], 1)
            key = statement, ordinal
            if row[1] != 'statement' or key not in inputs:
                raise ValueError('Assignment operator origin lacks its accepted input')
            module = int(model.statements[statement][7])
            source = (int(row[4]) if tag == 'LOC'
                      else int(model.macro_invocations[int(row[3])][4]))
            if int(model.source_contexts[source][4]) != module:
                raise ValueError('Assignment operator origin belongs to another module')
            origins.add(key)
    if set(inputs) != origins:
        raise ValueError('Assignment input lacks its original operator location')

    sequences = {}
    for statement, ordinal in inputs:
        sequences.setdefault(statement, []).append(ordinal)
    for statement, (owner, count) in counts.items():
        sequence = sorted(sequences.get(statement, []))
        if (len(sequence) != count
                or any(ordinal != index for index, ordinal in enumerate(sequence, 1))):
            raise ValueError('Assignment statement coverage is incomplete')
        for ordinal in sequence:
            if positions[owner, 'assignment-input:' + str(statement) + ':' + str(ordinal)] >= positions[owner, 'assignment-count:' + str(statement)]:
                raise ValueError('Assignment input follows its closed statement count')
    if set(sequences) - set(counts):
        raise ValueError('Assignment statement lacks its complete input count')
    for statement, start in model.statements.items():
        if (model.capabilities[int(start[7])].get('source-assignment-inputs') == 'available'
                and statement not in counts):
            raise ValueError('Observed statement lacks its assignment input count')

# end of semantic_assignment_inputs.py
