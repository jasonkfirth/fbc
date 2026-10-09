"""Project: FreeBASIC compiler observations
File: semantic_callbacks.py
Purpose: Independently validate callback origins and their containing headers.
Responsibilities: Counts, native owners, capabilities and occurrence coverage.
This file intentionally does NOT parse BASIC or infer calling conventions.
"""
from collections import defaultdict


def validate_procedure_callbacks(model, number, subject_modules):
    headers, callbacks, identities = {}, defaultdict(list), set()
    needs_call = set()

    def available(module):
        return model.capabilities[module].get('procedure-callback-inputs') == 'available'

    for module in model.capabilities:
        if available(module) and model.capabilities[module].get('procedure-typing-inputs') != 'available':
            raise ValueError('Callback observations lack procedure typing coverage')

    for row in model.records['K']:
        domain, owner, key, payload = row[1], int(row[2]), row[3], row[4]
        header = key.startswith('procedure-abi-input:')
        callback = key.startswith('callback-convention-input:')
        if not header and not callback:
            if key.startswith(('procedure-abi-', 'callback-convention-')):
                raise ValueError('Unknown callback property')
            continue
        identity = number(key.split(':', 1)[1], 1)
        fields = payload.split('\t')
        if identity > len(model.rows) or identity in identities or domain != 'symbol' or len(fields) != (6 if header else 3):
            raise ValueError('Invalid or duplicate callback occurrence')
        identities.add(identity)
        statement = number(fields[0], 1)
        source = number(fields[4 if header else 2], 1)
        module = subject_modules.get(('symbol', owner))
        native = model.symbols.get(owner)
        start = model.statements.get(statement)
        if (not available(module) or native is None or native[3] != '3' or start is None
                or int(start[7]) != module or int(start[5]) != source
                or source not in model.source_contexts or int(model.source_contexts[source][4]) != module):
            raise ValueError('Callback occurrence lacks native statement ownership')
        ending = model.statement_endings[statement]
        if header:
            count = number(fields[5], 0)
            if (ending[3] != 'parsed' or statement in headers or fields[1] not in ('prototype', 'definition')
                    or fields[2] not in ('0', '1') or fields[3] not in ('0', '1')
                    or count > 16000000 or (fields[1] == 'definition' and fields[2] != '0')):
                raise ValueError('Invalid procedure ABI header')
            headers[statement] = (owner, fields[1], count)
        else:
            if ending[3] != 'parsed':
                if ending[2:4] != ['unmatched', 'unmatched']:
                    raise ValueError('Callback origin lacks a parsed or native call statement')
                needs_call.add((statement, owner))
            signature = model.signatures.get(owner)
            if fields[1] not in ('0', '1') or signature is None or signature[2] != 'procedure-pointer':
                raise ValueError('Callback origin lacks a selected procedure-pointer type')
            callbacks[statement].append(owner)
    # Some consumed indirect SUB calls retain the dispatcher's unmatched route.
    # Their typed AST, statement owner and selected signature closure prove the
    # anonymous types were used. An unmatched header never gets this exception.
    for identity, node in model.nodes.items():
        if not needs_call:
            break
        if node[4] != '9' or node[8] != '0' or model.properties['node', identity].get('call-kind') != 'indirect':
            continue
        statement = model.statement_owners.get(('node', identity))
        seen, queue = set(), [int(node[9])]
        while queue:
            selected = queue.pop()
            if selected in seen or selected not in model.signatures:
                continue
            seen.add(selected)
            types = [model.types[selected]]
            types += [model.types[int(formal[2])] for formal in model.parameters[selected].values()]
            for native_type in types:
                subtype = int(native_type[7])
                if subtype in model.signatures and model.signatures[subtype][2] == 'procedure-pointer':
                    queue.append(subtype)
        needs_call.difference_update((statement, selected) for selected in seen)
    if needs_call:
        raise ValueError('Unmatched callback occurrence lacks an indirect void call proof')
    matched = set()
    for row in model.records['K']:
        if row[3].startswith('procedure-typing-input:'):
            fields = row[4].split('\t')
            statement, owner = int(fields[0]), int(row[2])
            if not available(subject_modules['symbol', owner]):
                continue
            if statement in matched or headers.get(statement) != (owner, fields[1], len(callbacks[statement])):
                raise ValueError('Procedure ABI header coverage is incomplete')
            matched.add(statement)
    if set(headers) != matched:
        raise ValueError('Procedure ABI header lacks its native typing occurrence')

# end of semantic_callbacks.py
