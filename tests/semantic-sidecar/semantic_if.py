"""Project: FreeBASIC semantic sidecar tests
File: semantic_if.py
Purpose: Validate original compiler-owned IF and ELSEIF conditions.
Responsibilities: Complete grammar coverage, exact roles and owner closure.
This file intentionally does NOT infer predicates from source or branch nodes.
"""


def validate_if_conditions(model, number, subject_modules):
    expressions = {int(row[1]) for row in model.records['E']}
    inputs, marked = {}, set()
    work_left = 8000000
    for row in model.records['K']:
        work_left -= 1
        if work_left < 0:
            raise ValueError('Original IF input work budget exceeded')
        if not row[3].startswith('if-condition-'):
            continue
        if not row[3].startswith('if-condition-expression:'):
            raise ValueError('Unknown IF condition property')
        statement = number(row[3][24:], 1)
        owner, expression = number(row[2], 1), number(row[4], 1)
        module = subject_modules.get(('symbol', owner))
        start = model.statements.get(statement)
        ending = model.statement_endings.get(statement)
        if (row[1] != 'symbol' or owner not in model.symbols or model.symbols[owner][3] != '3'
                or statement in inputs or start is None or ending is None or module is None
                or expression not in expressions or int(start[4]) != owner
                or int(start[7]) != module or int(start[9]) not in (266, 269)
                or ending[2:4] != ['compound', 'parsed']
                or subject_modules.get(('expression', expression)) != module
                or model.statement_owners.get(('expression', expression)) != statement
                or model.capabilities[module].get('if-condition-inputs') != 'available'):
            raise ValueError('IF condition lacks its complete compiler grammar owner')
        inputs[statement] = owner, expression
    for row in model.records['H']:
        work_left -= 1
        if work_left < 0:
            raise ValueError('Original IF marker work budget exceeded')
        if row[5] != 'if-condition':
            continue
        statement = number(row[6], 1)
        if (row[1] != 'symbol' or row[3] != 'expression' or statement in marked
                or inputs.get(statement) != (number(row[2], 1), number(row[4], 1))):
            raise ValueError('IF condition marker is missing, repeated or inconsistent')
        marked.add(statement)
    for statement, start in model.statements.items():
        work_left -= 1
        if work_left < 0:
            raise ValueError('Original IF coverage work budget exceeded')
        module = int(start[7])
        if (int(start[9]) in (266, 269)
                and model.statement_endings[statement][2:4] == ['compound', 'parsed']
                and model.capabilities[module].get('if-condition-inputs') == 'available'
                and (statement not in inputs or statement not in marked)):
            raise ValueError('Accepted IF grammar lacks its original condition')
    if set(inputs) != marked:
        raise ValueError('IF condition lacks its independent occurrence marker')

# end of semantic_if.py
