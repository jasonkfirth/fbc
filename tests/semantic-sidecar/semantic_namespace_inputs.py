"""Project: FreeBASIC semantic sidecar
File: semantic_namespace_inputs.py
Purpose: Validate parser-selected namespace occurrences.
Responsibilities: Canonical identities, module ownership and complete block coverage.
This file intentionally does NOT infer anonymity from names or missing bindings.
"""

FEATURE = 'namespace-declaration-inputs'
PREFIX = 'namespace-declaration:'


def validate_namespace_inputs(model, number, subject_modules):
    observed = set()
    work_left = 8000000
    for row in model.records['K']:
        work_left -= 1
        if not row[3].startswith(PREFIX):
            continue
        fields = row[4].split('\t')
        work_left -= len(row[3]) + len(row[4])
        if work_left < 0 or len(row[3]) > 64 or len(row[4]) > 128 or len(fields) != 3:
            raise ValueError('Namespace declaration input exceeds its bounded shape')
        block_id = number(row[3][len(PREFIX):], 1)
        statement_id, namespace_id = [number(value, 1) for value in fields[:2]]
        owner_id = number(row[2], 1)
        block = model.constructs.get(block_id)
        statement = model.statements.get(statement_id)
        namespace = model.symbols.get(namespace_id)
        module = subject_modules.get(('symbol', owner_id))
        if (row[1] != 'symbol' or row[3] != PREFIX + str(block_id)
                or fields[:2] != [str(statement_id), str(namespace_id)]
                or fields[2] not in ('0', '1') or block_id in observed
                or block is None or block[5] != 'namespace'
                or block[3:5] != [str(statement_id), str(owner_id)]
                or statement is None or statement[4] != str(owner_id)
                or int(statement[7]) != module or namespace is None or namespace[3] != '8'
                or subject_modules.get(('symbol', namespace_id)) != module
                or model.capabilities[module].get(FEATURE) != 'available'):
            raise ValueError('Invalid namespace declaration identity, owner or capability')
        observed.add(block_id)
    for identity, block in model.constructs.items():
        work_left -= 1
        if work_left < 0:
            raise ValueError('Namespace declaration work budget exceeded')
        module = int(model.statements[int(block[3])][7])
        if block[5] == 'namespace' and model.capabilities[module].get(FEATURE) == 'available':
            if identity not in observed:
                raise ValueError('Namespace declaration coverage is incomplete')

# end of semantic_namespace_inputs.py
