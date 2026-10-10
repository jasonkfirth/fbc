"""Project: FreeBASIC semantic sidecar
File: semantic_result_inputs.py
Purpose: Validate accepted result values before implicit address conversion.
Responsibilities: Selected routine, statement, input form and body count closure.
This file intentionally does NOT infer returns from source keywords.
"""
FEATURE = 'function-result-inputs'
PREFIX = 'function-result-input:'


def validate_result_inputs(model, number, subject_modules):
    inputs, counts, operations = {}, {}, set()
    signatures = {int(row[1]): row for row in model.records['F']}
    properties = {(row[1], int(row[2]), row[3]): row[4] for row in model.records['K']}
    work_left = 8000000
    for row in model.records['K']:
        key = row[3]
        work_left -= 1
        if not key.startswith(PREFIX):
            continue
        fields = row[4].split('\t')
        statement_id = number(key[len(PREFIX):], 1)
        procedure_id = number(row[2], 1)
        statement = model.statements.get(statement_id)
        module = subject_modules.get(('symbol', procedure_id))
        work_left -= len(key) + len(row[4])
        if (work_left < 0 or row[1] != 'symbol' or key != PREFIX + str(statement_id)
                or statement is None or statement_id in inputs or statement[4] != row[2]
                or procedure_id not in signatures or len(fields) != 4
                or model.capabilities[module].get(FEATURE) != 'available'
                or subject_modules.get(('statement', statement_id)) != module):
            raise ValueError('Invalid function result input owner')
        style, form = fields[:2]
        expression_id, variable_id = (number(value, 0) for value in fields[2:])
        if (style not in ('return', 'assignment') or form not in ('value', 'reference', 'pointer')
                or list(map(str, (expression_id, variable_id))) != fields[2:]
                or (signatures[procedure_id][6] == '0') != (form == 'value')):
            raise ValueError('Invalid function result input form')
        if expression_id and subject_modules.get(('expression', expression_id)) != module:
            raise ValueError('Invalid function result expression')
        if variable_id and (subject_modules.get(('symbol', variable_id)) != module
                            or model.symbols[variable_id][3] != '1'):
            raise ValueError('Invalid function result variable')
        inputs[statement_id] = style
        counts[procedure_id] = counts.get(procedure_id, 0) + 1
    for row in model.records['SOP']:
        work_left -= 1
        if not row[2].startswith('function-result-'):
            continue
        statement_id = number(row[1], 1)
        if work_left < 0 or statement_id in operations or row[2] != 'function-result-' + inputs.get(statement_id, ''):
            raise ValueError('Invalid function result operation closure')
        operations.add(statement_id)
    if operations != set(inputs):
        raise ValueError('Missing function result operation')
    for row in model.records['P']:
        procedure_id = int(row[1])
        module = subject_modules[('symbol', procedure_id)]
        if model.capabilities[module].get(FEATURE) == 'available':
            value = properties.get(('symbol', procedure_id, 'function-result-input-count'))
            if value is None or str(number(value, 0)) != value or int(value) != counts.get(procedure_id, 0):
                raise ValueError('Invalid function result body count closure')
# end of semantic_result_inputs.py
