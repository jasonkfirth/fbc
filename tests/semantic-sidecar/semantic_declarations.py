"""Project: FreeBASIC semantic sidecar tests
File: semantic_declarations.py
Purpose: Validate original declaration typing observations.
Responsibilities: Group completion, native symbols, formal coverage and bounds.
This file intentionally does NOT parse source or infer declaration policy.
"""

def validate_declaration_types(model, number, subject_modules):
    groups, endings, inputs, formals, statements = {}, {}, {}, {}, set()
    array_symbols = {int(row[1]) for row in model.records['A']}
    work_left = 8000000

    def integer(text, minimum=1):
        value = number(text, minimum)
        if value > len(model.rows):
            raise ValueError('Declaration input exceeds the bounded detail stream')
        return value

    for row in model.records['K']:
        work_left -= 1
        if work_left < 0:
            raise ValueError('Declaration typing work budget exceeded')
        domain, owner, key, payload = row[1], int(row[2]), row[3], row[4]
        if not key.startswith('declaration-type-') and key != 'formal-written-type':
            continue
        module = subject_modules.get((domain, owner))
        symbol = model.symbols.get(owner)
        if domain != 'symbol' or symbol is None or model.capabilities[module].get('declaration-typing-inputs') != 'available':
            raise ValueError('Declaration observation has no available compiler capability')
        parts, fields = key.split(':'), payload.split('\t')
        if parts[0] == 'declaration-type-group' and len(parts) == 2 and len(fields) == 2:
            identity, statement = integer(parts[1]), integer(fields[0])
            header = model.statements.get(statement)
            if identity in groups or statement in statements or header is None or int(header[7]) != module:
                raise ValueError('Missing or repeated declaration statement')
            if int(symbol[3]) not in (3, 8) or int(header[4]) != owner and (header[4] != '0' or symbol[3] != '8'):
                raise ValueError('Declaration group belongs to another procedure')
            if fields[1] not in ('dim', 'static', 'common', 'extern', 'const'):
                raise ValueError('Unknown declaration grammar kind')
            groups[identity] = owner, module, statement, fields[1]
            statements.add(statement)
        elif parts[0] == 'declaration-type-end' and len(parts) == 2 and len(fields) == 1:
            identity, count = integer(parts[1]), integer(fields[0])
            if identity in endings:
                raise ValueError('Repeated declaration completion')
            endings[identity] = owner, module, count
        elif parts[0] == 'declaration-type-input' and len(parts) == 3 and len(fields) == 4:
            identity, ordinal, declared = integer(parts[1]), integer(parts[2]), integer(fields[0])
            if (identity, ordinal) in inputs:
                raise ValueError('Repeated declaration input ordinal')
            if fields[2] not in ('0', '1', '2', '3'):
                raise ValueError('Invalid declaration initializer grammar')
            source = integer(fields[3])
            context = model.source_contexts.get(source)
            if context is None or int(context[4]) != module:
                raise ValueError('Declaration input has a foreign source context')
            inputs[identity, ordinal] = owner, module, declared, fields[1], fields[2]
        elif key == 'formal-written-type':
            if owner in formals or symbol[3] != '4' or fields != [payload] or payload not in ('as', 'suffix', 'implicit', 'vararg'):
                raise ValueError('Invalid or repeated formal written type')
            properties = model.properties['symbol', owner]
            if 'formal-span-kind' not in properties or (payload == 'vararg') != (properties.get('formal-accepted-mode') == '4'):
                raise ValueError('Formal type has no matching original formal')
            formals[owner] = payload
        else:
            raise ValueError('Malformed declaration typing observation')
    if set(groups) != set(endings):
        raise ValueError('Declaration input or completion coverage is incomplete')
    observed = {identity: set() for identity in groups}
    for (identity, ordinal), (owner, module, declared, form, initializer) in inputs.items():
        group = groups.get(identity)
        symbol = model.symbols.get(declared)
        if group is None or (owner, module) != group[:2] or symbol is None or subject_modules.get(('symbol', declared)) != module:
            raise ValueError('Declaration input belongs to another group or module')
        if group[3] == 'const':
            if symbol[3] != '2' or initializer != '2' or form not in ('as', 'suffix', 'inferred'):
                raise ValueError('Invalid constant declaration type')
        elif symbol[3] != '1' or form not in ('as', 'suffix', 'implicit') or group[3] == 'extern' and initializer != '0':
            raise ValueError('Invalid variable declaration type')
        if initializer == '3':
            dtype = int(symbol[4])
            if declared in array_symbols or not dtype & 0x1e0 and not 1 <= dtype & 31 <= 16:
                raise ValueError('Simple initializer does not target a numeric scalar')
        observed[identity].add(ordinal)
    for identity, (owner, module, count) in endings.items():
        if (owner, module) != groups[identity][:2] or len(observed[identity]) != count or min(observed[identity], default=0) != 1 or max(observed[identity], default=0) != count:
            raise ValueError('Incomplete declaration ordinal coverage')
    for identity, symbol in model.symbols.items():
        module = subject_modules['symbol', identity]
        if model.capabilities[module].get('declaration-typing-inputs') == 'available' and 'formal-span-kind' in model.properties['symbol', identity] and identity not in formals:
            raise ValueError('Original formal lacks its written type')
    for identity, statement in model.statements.items():
        if int(statement[9]) in (306, 307, 308, 311, 335) and model.statement_endings[identity][2] == 'declaration' and model.capabilities[int(statement[7])].get('declaration-typing-inputs') == 'available' and identity not in statements:
            raise ValueError('Accepted declaration lacks its type group')

# end of semantic_declarations.py
