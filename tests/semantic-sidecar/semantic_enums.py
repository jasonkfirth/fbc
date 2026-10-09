"""Project: FreeBASIC semantic sidecar tests
File: semantic_enums.py
Purpose: Validate compiler-owned enum members and original initializers.
Responsibilities: Complete groups, native statement roles and module closure.
This file intentionally does NOT infer membership from namespace ownership.
"""


def validate_enum_declarations(model, number, subject_modules):
    groups, members, headers = {}, {}, set()
    enum_constructs = {}
    for construct in model.constructs.values():
        if construct[5] == 'enum':
            statement = int(construct[3])
            if statement in enum_constructs:
                raise ValueError('Repeated native enum construct for one header')
            enum_constructs[statement] = int(construct[1])
    expressions = {int(row[1]): row for row in model.records['E']}
    work_left = 8000000

    def integer(text, minimum=1):
        value = number(text, minimum)
        if value > len(model.rows):
            raise ValueError('Enum input exceeds the bounded detail stream')
        return value

    for row in model.records['K']:
        work_left -= 1
        if work_left < 0:
            raise ValueError('Enum declaration work budget exceeded')
        domain, identity, key, payload = row[1], int(row[2]), row[3], row[4]
        if key not in ('enum-declaration-input', 'enum-element-input'):
            continue
        module = subject_modules.get((domain, identity))
        symbol = model.symbols.get(identity)
        if (domain != 'symbol' or symbol is None or module is None
                or model.capabilities[module].get('enum-declaration-inputs') != 'available'):
            raise ValueError('Enum input has no available compiler capability')
        fields = payload.split('\t')
        if key == 'enum-declaration-input':
            if len(fields) != 2 or symbol[3] != '9' or identity in groups:
                raise ValueError('Invalid or repeated enum declaration')
            statement, count = integer(fields[0]), integer(fields[1])
            header = model.statements.get(statement)
            layout = model.layouts.get(identity)
            if (header is None or int(header[7]) != module or statement not in enum_constructs
                    or model.statement_endings[statement][2] not in ('declaration', 'aggregate-member')
                    or model.statement_endings[statement][3] != 'parsed'
                    or statement in headers or layout is None or layout[2] != 'enum'
                    or int(layout[9]) != count):
                raise ValueError('Enum declaration disagrees with its native header or count')
            groups[identity] = module, statement, count
            headers.add(statement)
        else:
            if len(fields) != 5 or symbol[3] != '2' or symbol[4] != '10' or identity in members:
                raise ValueError('Invalid or repeated enum member')
            owner, ordinal, statement = (integer(value) for value in fields[:3])
            initializer = integer(fields[4], 0)
            if fields[3] not in ('0', '1') or (fields[3] == '1') != bool(initializer):
                raise ValueError('Enum initializer presence disagrees with its original input')
            body = model.statements.get(statement)
            if (int(symbol[5]) != owner or body is None or int(body[7]) != module
                    or model.statement_endings[statement][2:4] != ['enumerator', 'parsed']):
                raise ValueError('Enum member disagrees with its native subtype or statement')
            if initializer:
                expression = expressions.get(initializer)
                if (expression is None or expression[8] != '16'
                        or ('expression', initializer) not in model.constants
                        or subject_modules.get(('expression', initializer)) != module
                        or model.statement_owners.get(('expression', initializer)) != statement):
                    raise ValueError('Enum initializer lacks its original constant expression')
            members[identity] = module, owner, ordinal, statement
    observed = {identity: set() for identity in groups}
    for module, owner, ordinal, statement in members.values():
        group = groups.get(owner)
        if (group is None or group[0] != module or ordinal > group[2]
                or ordinal in observed[owner] or int(model.statements[statement][2]) != group[1]
                or int(model.statements[statement][3]) != enum_constructs[group[1]]):
            raise ValueError('Enum member belongs to another group or repeats an ordinal')
        observed[owner].add(ordinal)
    for identity, (_, _, count) in groups.items():
        if len(observed[identity]) != count:
            raise ValueError('Enum membership is incomplete')
    for identity, symbol in model.symbols.items():
        module = subject_modules['symbol', identity]
        if (symbol[3] == '9' and model.capabilities[module].get('enum-declaration-inputs') == 'available'
                and identity not in groups):
            raise ValueError('Native enum lacks its parser declaration')
    for identity, statement in model.statements.items():
        if (identity in enum_constructs and model.statement_endings[identity][2] in ('declaration', 'aggregate-member')
                and model.statement_endings[identity][3] == 'parsed'
                and model.capabilities[int(statement[7])].get('enum-declaration-inputs') == 'available'
                and identity not in headers):
            raise ValueError('Accepted enum header lacks its completed member group')

# end of semantic_enums.py
