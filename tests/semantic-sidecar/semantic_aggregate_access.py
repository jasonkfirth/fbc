"""Project: FreeBASIC semantic sidecar tests
File: semantic_aggregate_access.py
Purpose: Validate complete parser-owned aggregate visibility sections.
Responsibilities: Body ownership, section order and accepted grammar.
This file intentionally does NOT reconstruct source access frames or lint policy.
"""


def validate_aggregate_access(model, number, subject_modules):
    bodies, blocks, sections, statements = {}, {}, {}, set()

    def available(module):
        return model.capabilities[module].get('aggregate-access-sections') == 'available'

    def bounded(text, minimum=1):
        value = number(text, minimum)
        if value > len(model.rows):
            raise ValueError('Aggregate access identity exceeds the detail stream')
        return value

    headers = {}
    for identity, record in model.constructs.items():
        if record[5] in ('type', 'union'):
            header = int(record[3])
            if header in headers:
                raise ValueError('Duplicate aggregate header block')
            headers[header] = identity
    for row in model.records['K']:
        domain, owner, key, payload = row[1], int(row[2]), row[3], row[4]
        if key != 'aggregate-body-statement':
            continue
        module = subject_modules.get((domain, owner))
        header = bounded(payload)
        symbol = model.symbols.get(owner)
        block = headers.get(header)
        count_text = model.properties.get((domain, owner), {}).get('aggregate-access-count')
        if domain != 'symbol' or symbol is None or symbol[3] != '10' or not available(module) or block is None or count_text is None:
            raise ValueError('Invalid aggregate body owner or capability')
        count = bounded(count_text, 0)
        layout = model.layouts.get(owner)
        if owner not in model.types or layout is None or layout[2] != model.constructs[block][5] or layout[2] == 'union' and count:
            raise ValueError('Aggregate body disagrees with its selected layout')
        if model.statement_endings[header][3] != 'parsed' or int(model.statements[header][7]) != module:
            raise ValueError('Aggregate body lacks an accepted same-module statement')
        if owner in bodies or block in blocks:
            raise ValueError('Duplicate aggregate body observation')
        bodies[owner], blocks[block] = (block, count), owner
    if sum(count for _, count in bodies.values()) > len(model.rows):
        raise ValueError('Aggregate section total exceeds the detail stream')
    for row in model.records['K']:
        domain, owner, key, payload = row[1], int(row[2]), row[3], row[4]
        if key == 'aggregate-access-count':
            if domain != 'symbol' or owner not in bodies:
                raise ValueError('Aggregate count has no body')
        elif key.startswith('aggregate-access-section:'):
            ordinal = bounded(key[len('aggregate-access-section:'):])
            fields = payload.split('\t')
            if domain != 'symbol' or owner not in bodies or len(fields) != 2 or ordinal > bodies[owner][1]:
                raise ValueError('Invalid aggregate section owner or ordinal')
            statement = bounded(fields[0])
            record = model.statements.get(statement)
            ending = model.statement_endings.get(statement)
            module = subject_modules.get((domain, owner))
            if record is None or ending is None or ending[2:4] != ['aggregate-member', 'parsed'] or int(record[7]) != module or int(record[3]) != bodies[owner][0]:
                raise ValueError('Aggregate section selects another body or statement')
            if fields[1] not in ('public', 'private', 'protected') or int(record[9]) != {'public': 380, 'private': 381, 'protected': 382}[fields[1]]:
                raise ValueError('Aggregate visibility differs from accepted grammar')
            if statement in statements or (owner, ordinal) in sections:
                raise ValueError('Duplicate aggregate section observation')
            statements.add(statement)
            sections[owner, ordinal] = statement
        elif key != 'aggregate-body-statement' and key.startswith(('aggregate-body-', 'aggregate-access-')):
            raise ValueError('Unknown aggregate access property')
    for owner, (_, count) in bodies.items():
        if count > len(model.rows) or any((owner, ordinal) not in sections for ordinal in range(1, count + 1)):
            raise ValueError('Aggregate section sequence is incomplete')
    for identity, record in model.constructs.items():
        if record[5] in ('type', 'union') and available(int(record[8])) and identity not in blocks:
            raise ValueError('Accepted aggregate lacks its body observation')
    for identity, record in model.statements.items():
        if 380 <= int(record[9]) <= 382 and available(int(record[7])) and model.statement_endings[identity][2] == 'aggregate-member' and identity not in statements:
            raise ValueError('Accepted visibility section lacks its observation')

# end of semantic_aggregate_access.py
