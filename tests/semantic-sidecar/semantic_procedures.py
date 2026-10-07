"""Project: FreeBASIC semantic sidecar tests
File: semantic_procedures.py
Purpose: Validate original named procedure typing and visibility observations.
Responsibilities: Accepted headers, selected procedures and complete definitions.
This file intentionally does NOT parse procedure source or decide lint policy.
"""

def validate_procedure_types(model, number, subject_modules):
    headers, identities, visibility = {}, set(), {}

    def available(module):
        return model.capabilities[module].get('procedure-typing-inputs') == 'available'

    def identity(text):
        value = number(text, 1)
        if value > len(model.rows):
            raise ValueError('Procedure input exceeds the bounded detail stream')
        return value

    for row in model.records['K']:
        domain, owner, key, payload = row[1], int(row[2]), row[3], row[4]
        module = subject_modules.get((domain, owner))
        fields = payload.split('\t')
        if key.startswith('procedure-written-visibility'):
            statement = identity(key[len('procedure-written-visibility:'):])
            header = model.statements.get(statement)
            if domain != 'symbol' or header is None or int(header[7]) != module or statement in visibility or not available(module):
                raise ValueError('Invalid procedure visibility owner or capability')
            if fields != [payload] or payload not in ('default', 'public', 'private'):
                raise ValueError('Invalid procedure visibility grammar')
            visibility[statement] = owner, payload
        elif key.startswith('procedure-typing-input:'):
            receipt = identity(key[len('procedure-typing-input:'):])
            symbol = model.symbols.get(owner)
            if receipt in identities or domain != 'symbol' or symbol is None or symbol[3] != '3' or not available(module) or len(fields) != 5:
                raise ValueError('Invalid procedure typing owner or capability')
            identities.add(receipt)
            statement = identity(fields[0])
            header = model.statements.get(statement)
            if statement in headers or header is None or int(header[7]) != module or model.statement_endings[statement][3] != 'parsed':
                raise ValueError('Procedure typing has no unique accepted statement')
            role, kind, form, exported = fields[1:]
            if role not in ('prototype', 'definition') or exported not in ('0', '1'):
                raise ValueError('Invalid procedure role or export flag')
            selected = model.types.get(owner)
            selected_kind = model.signatures.get(owner, ['', '', ''])[2]
            if selected is None:
                raise ValueError('Procedure typing lacks completed symbol metadata')
            dtype = int(selected[6])
            if kind == 'sub':
                valid = form == 'none' and dtype == 0
            elif kind == 'function':
                valid = form in ('as', 'suffix', 'implicit') and dtype != 0
            elif kind == 'property':
                valid = form == 'special' and selected_kind in ('property-get', 'property-set')
            elif kind in ('constructor', 'destructor', 'operator'):
                valid = form == 'special'
            else:
                valid = False
            if not valid or kind != 'property' and kind != selected_kind:
                raise ValueError('Procedure grammar does not match its selected kind')
            headers[statement] = owner, role, kind, form
        elif key.startswith('procedure-typing-'):
            raise ValueError('Unknown procedure typing property')
    definitions = {statement for statement, header in headers.items() if header[1] == 'definition'}
    if definitions != set(visibility):
        raise ValueError('Procedure visibility coverage is incomplete')
    for statement, (owner, written) in visibility.items():
        if headers[statement][0] != owner:
            raise ValueError('Procedure visibility selects another procedure')
    for row in model.records['DCL']:
        if row[3] in ('procedure-definition', 'procedure-prototype') and available(int(row[11])):
            statement = model.statement_owners.get(('declaration', int(row[1])))
            header = headers.get(statement)
            if header is None or header[:2] != (int(row[2]), row[3][len('procedure-'):]):
                raise ValueError('Original named declaration lacks its typing receipt')
    for statement, record in model.statements.items():
        route = model.statement_endings[statement][2]
        if available(int(record[7])) and int(record[9]) in (334, 345, 346) and route in ('declaration', 'aggregate-member') and statement not in headers:
            raise ValueError('Accepted procedure lacks its typing receipt')

# end of semantic_procedures.py
