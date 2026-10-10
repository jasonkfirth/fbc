"""Project: FreeBASIC semantic sidecar
File: semantic_option_routes.py
Purpose: Validate the accepted OPTION grammar route and occurrence coverage.
Responsibilities: Native keyword identity, committed contexts and route domain.
This file intentionally does NOT interpret source directives.
"""
FEATURE = 'option-declaration-routes'
PREFIX = 'option-route-'


def validate_option_routes(model, number, subject_modules):
    tokens = {int(model.properties['symbol', identity]['keyword-token'])
              for identity, row in model.types.items()
              if row[3] == 'keyword' and row[2].upper() == 'OPTION'}
    inputs = set()
    for row in model.records['OPT']:
        if not row[3].startswith(PREFIX):
            continue
        statement_id = number(row[3][len(PREFIX):], 1)
        route, context = number(row[4], 1), int(row[1])
        statement = model.statements.get(statement_id)
        module = model.configurations[context]
        if (row[2] != 'language-default' or row[3] != PREFIX + str(statement_id)
                or row[4] != str(route) or route > 10 or statement is None
                or statement_id in inputs or int(statement[9]) not in tokens
                or subject_modules['statement', statement_id] != module
                or model.statement_endings[statement_id][3] != 'parsed'
                or model.capabilities[module].get(FEATURE) != 'available'
                or model.options[context].get(('language-default', 'option-statement-' + str(statement_id))) != int(route == 8)):
            raise ValueError('Invalid accepted OPTION route')
        inputs.add(statement_id)
    for statement_id, statement in model.statements.items():
        module = int(statement[7])
        if (model.capabilities[module].get(FEATURE) == 'available'
                and int(statement[9]) in tokens and statement_id not in inputs):
            raise ValueError('Missing accepted OPTION route')
# end of semantic_option_routes.py
