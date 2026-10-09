"""Project: FreeBASIC semantic sidecar tests
File: semantic_array_storage.py
Purpose: Validate selected array storage before descriptor and bound lowering.
Responsibilities: Exact field receivers, independent coverage and snapshot ownership.
This file intentionally does NOT infer whether an array element is read.
"""


def validate_array_storage_inputs(model, number, subject_modules):
    roots = {}
    selections = {}
    inputs = {}
    arrays = {int(row[1]) for row in model.records['A']}
    expected = {'array-storage-symbol', 'array-storage-kind', 'array-storage-receiver'}

    def module_of(domain, identity):
        module = subject_modules.get((domain, identity))
        if module is None or model.capabilities[module].get('array-storage-inputs') != 'available':
            raise ValueError('Array storage capability or subject is unavailable')
        return module

    for row in model.records['H']:
        role = row[5]
        if role not in ('array-storage-root', 'array-storage-selection', 'array-storage-input'):
            continue
        if row[6] != '0':
            raise ValueError('Array storage relation has an unknown ordinal')
        source, target = number(row[2], 1), number(row[4], 1)
        module = module_of(row[1], source)
        if subject_modules.get((row[3], target)) != module:
            raise ValueError('Array storage relation selects a foreign subject')
        if role == 'array-storage-root':
            node = model.nodes.get(target)
            symbol = model.symbols.get(source)
            statement = model.statement_owners.get(('node', target))
            if (row[1] != 'symbol' or row[3] != 'node' or target in roots
                    or symbol is None or symbol[3] != '12' or source not in arrays
                    or node is None or node[3] != 'root' or node[2] != str(source)
                    or not number(node[8], 0) & 0x1E0 or statement is None
                    or model.statement_endings[statement][3] != 'parsed'):
                raise ValueError('Array storage root lacks its field or accepted owner')
            roots[target] = source, statement, module
        elif role == 'array-storage-selection':
            if (row[1] != 'symbol' or row[3] != 'expression' or target in selections
                    or source not in arrays or model.symbols[source][3] not in ('1', '12')):
                raise ValueError('Array storage selection is missing or repeated')
            selections[target] = source
        else:
            if row[1] != 'expression' or row[3] != 'node' or source in inputs:
                raise ValueError('Array storage input is missing or repeated')
            inputs[source] = target

    used_roots = set()
    groups = set()
    for (domain, identity), properties in model.properties.items():
        keys = {key for key in properties if key.startswith('array-storage-')}
        if not keys:
            continue
        module = module_of(domain, identity)
        if domain != 'expression' or keys != expected or identity not in selections:
            raise ValueError('Array storage group has incomplete or unknown properties')
        symbol = number(properties['array-storage-symbol'], 1)
        receiver = number(properties['array-storage-receiver'], 0)
        if selections[identity] != symbol:
            raise ValueError('Array storage properties differ from the selected array')
        statement = model.statement_owners.get((domain, identity))
        if statement is None or model.statement_endings[statement][3] != 'parsed':
            raise ValueError('Array storage expression lacks accepted ownership')
        is_field = model.symbols[symbol][3] == '12'
        if properties['array-storage-kind'] != ('field' if is_field else 'variable'):
            raise ValueError('Array storage kind differs from the compiler symbol')
        if is_field:
            if roots.get(receiver) != (symbol, statement, module) or inputs.get(identity) != receiver:
                raise ValueError('Array field lacks its exact original receiver')
            used_roots.add(receiver)
        elif receiver or identity in inputs:
            raise ValueError('Plain array storage has a fabricated receiver')
        for key in ('array-subscript-symbol', 'array-bound-symbol'):
            if key in properties and properties[key] != str(symbol):
                raise ValueError('Array storage differs from the subscript or bound query')
        groups.add(identity)
    if set(selections) != groups or set(inputs) != {identity for identity in groups
                                                  if model.symbols[selections[identity]][3] == '12'}:
        raise ValueError('Array storage relation coverage is incomplete')
    if used_roots != set(roots):
        raise ValueError('Array storage root has no selected source input')
    for (domain, identity), properties in model.properties.items():
        if ('array-subscript-symbol' in properties or 'array-bound-symbol' in properties):
            module = subject_modules.get((domain, identity))
            if model.capabilities[module].get('array-storage-inputs') == 'available' and identity not in groups:
                raise ValueError('Array subscript or query lost its original storage group')

    # A parser snapshot is evidence about the receiver, not an emitted load.
    # Every descendant has the same accepted statement and module as its root.
    phased = {int(row[1]) for row in model.records['NP']}
    tree_roots = {}
    for identity in sorted(model.nodes):
        node = model.nodes[identity]
        parent = number(node[2], 1)
        root = identity if node[3] == 'root' else tree_roots.get(parent)
        tree_roots[identity] = root
        if root in roots:
            _, statement, module = roots[root]
            if (identity in phased or model.statement_owners.get(('node', identity)) != statement
                    or subject_modules.get(('node', identity)) != module):
                raise ValueError('Array receiver snapshot has emitted or foreign descendants')
    return set(roots)

# end of semantic_array_storage.py
