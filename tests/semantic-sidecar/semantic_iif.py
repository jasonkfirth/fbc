"""Project: FreeBASIC semantic sidecar tests
File: semantic_iif.py
Purpose: Validate original compiler-owned conditional-expression operands.
Responsibilities: Complete occurrences, ordered identities and module closure.
This file intentionally does NOT infer IIf roles from physical source spans.
"""


def validate_iif_inputs(model, number, subject_modules):
    expressions = {int(row[1]): row for row in model.records['E']}
    inputs, marked = {}, set()
    work_left = 8000000
    for row in model.records['K']:
        work_left -= 1
        if work_left < 0:
            raise ValueError('Original IIf input work budget exceeded')
        if row[3] != 'original-iif-inputs':
            continue
        identity = number(row[2], 1)
        module = subject_modules.get(('expression', identity))
        fields = row[4].split('\t')
        statement = model.statement_owners.get(('expression', identity))
        if (row[1] != 'expression' or identity not in expressions or identity in inputs
                or module is None or len(fields) != 3 or statement not in model.statements
                or model.capabilities[module].get('original-iif-inputs') != 'available'
                or int(model.statements[statement][7]) != module
                or model.statement_endings[statement][3] != 'parsed'):
            raise ValueError('Original IIf input has no complete compiler occurrence')
        arguments = [number(field, 1) for field in fields]
        if not 0 < arguments[0] < arguments[1] < arguments[2] < identity:
            raise ValueError('Original IIf operands are repeated or out of parse order')
        for argument in arguments:
            if (argument not in expressions or subject_modules.get(('expression', argument)) != module
                    or model.statement_owners.get(('expression', argument)) != statement):
                raise ValueError('Original IIf operand belongs to another statement or module')
        inputs[identity] = module
    for row in model.records['H']:
        work_left -= 1
        if work_left < 0:
            raise ValueError('Original IIf occurrence work budget exceeded')
        if row[5] != 'parsed-iif':
            continue
        owner, identity = number(row[2], 1), number(row[4], 1)
        module = inputs.get(identity)
        if (row[1] != 'symbol' or row[3] != 'expression' or row[6] != '0'
                or owner not in model.symbols or model.symbols[owner][3] != '8'
                or identity in marked or module is None
                or subject_modules.get(('symbol', owner)) != module):
            raise ValueError('Original IIf occurrence marker is invalid or lacks its operands')
        marked.add(identity)
    if set(inputs) != marked:
        raise ValueError('Original IIf input lacks its independent occurrence marker')

# end of semantic_iif.py
