"""Project: FreeBASIC semantic sidecar
File: semantic_literal_inputs.py
Purpose: Validate parsed array dimensions and individual string tokens.
Responsibilities: Typed inputs, statement-local ordinals and complete counts.
This file intentionally does NOT interpret source bounds or escape sequences.
"""
FAMILIES = ('array-dimension', 'string-literal')


def validate_literal_inputs(model, number, subject_modules):
    counts = {}
    expressions = {int(row[1]): row for row in model.records['E']}
    literal_ids = set()
    for row in model.records['K']:
        family = next((name for name in FAMILIES if row[3].startswith(name + '-input:')
                       or row[3].startswith(name + '-count:')), None)
        if family is None:
            continue
        is_count = row[3].startswith(family + '-count:')
        tail = row[3][len(family + ('-count:' if is_count else '-input:')):].split(':')
        if len(tail) != (1 if is_count else 2):
            raise ValueError('Invalid parsed literal input key')
        statement_id = number(tail[0], 1)
        if str(statement_id) != tail[0]:
            raise ValueError('Noncanonical parsed literal statement')
        statement = model.statements.get(statement_id)
        module = subject_modules.get(('statement', statement_id))
        if (row[1] != 'symbol' or statement is None or row[2] != statement[4]
                or model.capabilities[module].get(family + '-inputs') != 'available'):
            raise ValueError('Invalid parsed literal input owner')
        if is_count:
            if str(number(row[4], 0)) != row[4] or int(row[4]) > 2097152:
                raise ValueError('Invalid parsed literal count')
            continue
        ordinal = number(tail[1], 1)
        if str(ordinal) != tail[1] or ordinal != counts.get((family, statement_id), 0) + 1:
            raise ValueError('Invalid parsed literal input ordinal')
        counts[family, statement_id] = ordinal
        fields = row[4].split('\t')
        if len(fields) != 4 or model.statement_endings[statement_id][3] != 'parsed':
            raise ValueError('Invalid parsed literal input payload')
        options = model.options[int(statement[6])]
        if family == 'array-dimension':
            dimension, form, expression_id, base = fields
            if (number(dimension, 0) > 7 or form not in ('implicit', 'explicit', 'placeholder')
                    or int(base) != options['language-default', 'base']):
                raise ValueError('Invalid array dimension input')
            expression_id = number(expression_id, 0)
            if form == 'placeholder' and expression_id:
                raise ValueError('Placeholder has an array bound expression')
        else:
            expression_id, form, slash, escapes = fields
            expression_id = number(expression_id, 1)
            if (form not in ('plain', 'escaped', 'noescape') or slash not in ('0', '1')
                    or escapes not in ('0', '1') or int(escapes) != abs(options['language-default', 'escapestr'] != 0)):
                raise ValueError('Invalid string literal input')
            literal_ids.add(expression_id)
            if model.capabilities[module].get('string-literal-lexemes') == 'available':
                lexeme = model.properties.get(('expression', expression_id), {}).get('string-literal-lexeme')
                fields = lexeme.split('\t') if lexeme is not None else []
                if (len(fields) != 4 or fields[0] not in ('bytes', 'wide-units')
                        or fields[3] not in ('complete', 'truncated')):
                    raise ValueError('Missing accepted string literal lexeme')
                width = 2 if fields[0] == 'bytes' else 8
                units = number(fields[1], 0)
                if (str(units) != fields[1] or units > 1024 or len(fields[2]) != units * width
                        or any(char not in '0123456789ABCDEF' for char in fields[2])):
                    raise ValueError('Invalid accepted string literal lexeme')
                has_slash = any(fields[2][index:index + width] == format(92, '0' + str(width) + 'X')
                                for index in range(0, len(fields[2]), width))
                # The old scanner receipt includes discarded tail units.
                # A truncated prefix need not contain its reported slash.
                if (has_slash and not int(slash)) or (fields[3] == 'complete' and has_slash != bool(int(slash))):
                    raise ValueError('Literal lexeme disagrees with lexer slash receipt')
        if expression_id:
            expression = expressions.get(expression_id)
            if (expression is None or subject_modules.get(('expression', expression_id)) != module
                    or model.statement_owners.get(('expression', expression_id)) != statement_id):
                raise ValueError('Parsed literal expression lacks its statement owner')
            if family == 'array-dimension':
                if int(expression[12]) & 511 != 8:
                    raise ValueError('Array upper bound is not a typed integer')
            else:
                symbol_id = int(expression[13])
                symbol = model.symbols.get(symbol_id)
                constant = model.constants.get(('symbol', symbol_id))
                if (int(expression[8]) != 17 or symbol is None or int(symbol[7]) & 1024 == 0
                        or constant is None or constant[0] not in ('bytes', 'wide-units')):
                    raise ValueError('String token has no literal value')
    for row in model.records['K']:
        if row[3] == 'string-literal-lexeme':
            identity = number(row[2], 1)
            module = subject_modules.get(('expression', identity))
            if (row[1] != 'expression' or identity not in literal_ids or module is None
                    or model.capabilities[module].get('string-literal-lexemes') != 'available'):
                raise ValueError('Unowned accepted string literal lexeme')
    for statement_id, statement in model.statements.items():
        for family in FAMILIES:
            if model.capabilities[int(statement[7])].get(family + '-inputs') == 'available':
                value = model.properties['symbol', int(statement[4])].get(family + '-count:' + str(statement_id))
                if value is None or number(value, 0) != counts.get((family, statement_id), 0):
                    raise ValueError('Invalid parsed literal input count closure')
# end of semantic_literal_inputs.py
