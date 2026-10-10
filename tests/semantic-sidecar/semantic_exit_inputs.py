"""Project: FreeBASIC semantic sidecar
File: semantic_exit_inputs.py
Purpose: Validate accepted explicit routine exits and body count closure.
Responsibilities: Actual exit labels, statement operations and capability gates.
This file intentionally does NOT infer transfers from source spelling.
"""
FEATURE = 'procedure-exit-inputs'
PREFIX = 'procedure-exit-input:'


def validate_exit_inputs(model, number, subject_modules):
    inputs, operations, counts = {}, set(), {}
    properties = {(row[1], int(row[2]), row[3]): row[4] for row in model.records['K']}
    exits = {int(row[2]): int(row[4]) for row in model.records['H'] if row[5] == 'procedure-exit-label'}
    for row in model.records['K']:
        if not row[3].startswith(PREFIX):
            continue
        statement_id = number(row[3][len(PREFIX):], 1)
        procedure_id, target_id = number(row[2], 1), number(row[4], 1)
        statement = model.statements.get(statement_id)
        module = subject_modules.get(('symbol', procedure_id))
        if (row[1] != 'symbol' or row[3] != PREFIX + str(statement_id) or row[4] != str(target_id)
                or statement is None or statement[4] != row[2] or statement_id in inputs
                or subject_modules.get(('statement', statement_id)) != module
                or model.capabilities[module].get(FEATURE) != 'available'
                or exits.get(procedure_id) != target_id):
            raise ValueError('Invalid explicit routine exit input')
        inputs[statement_id] = procedure_id
        counts[procedure_id] = counts.get(procedure_id, 0) + 1
    for row in model.records['SOP']:
        if row[2] == 'procedure-explicit-exit':
            statement_id = number(row[1], 1)
            if statement_id not in inputs or statement_id in operations:
                raise ValueError('Invalid explicit routine exit operation')
            operations.add(statement_id)
    if operations != set(inputs):
        raise ValueError('Missing explicit routine exit operation')
    for row in model.records['P']:
        procedure_id = int(row[1])
        module = subject_modules[('symbol', procedure_id)]
        if model.capabilities[module].get(FEATURE) == 'available':
            value = properties.get(('symbol', procedure_id, 'procedure-exit-input-count'))
            if value is None or str(number(value, 0)) != value or int(value) != counts.get(procedure_id, 0):
                raise ValueError('Invalid explicit routine exit count closure')
# end of semantic_exit_inputs.py
