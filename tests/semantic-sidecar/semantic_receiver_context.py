"""Project: FreeBASIC semantic sidecar
File: semantic_receiver_context.py
Purpose: Validate original field receivers in language and preprocessing contexts.
Responsibilities: Nominal identities, native context, links and optional statements.
This file intentionally does NOT infer runtime evaluation of type queries.
"""

from collections import defaultdict

FEATURE = "member-receiver-contexts"


def validate_receiver_contexts(model, number, subject_modules):
    links = defaultdict(list)
    for row in model.relations("member-receiver-type"):
        if row[1] != "expression" or row[3] != "symbol":
            raise ValueError("Invalid member receiver relationship domains")
        links[number(row[2], 1)].append(row)
    for (domain, identity), properties in model.properties.items():
        payload = properties.get("member-receiver-input")
        context = properties.get("member-receiver-context")
        if payload is None:
            if context is not None:
                raise ValueError("Member receiver context has no input")
            continue
        if domain != "expression":
            raise ValueError("Member receiver input has a foreign domain")
        module = subject_modules[domain, identity]
        if model.capabilities[module].get("member-receiver-inputs") != "available":
            raise ValueError("Member receiver input has unavailable coverage")
        fields = payload.split("\t")
        if len(fields) != 4:
            raise ValueError("Invalid member receiver input shape")
        dtype, receiver, field, statement = [number(value, 0) for value in fields]
        receiver_symbol, field_symbol = model.symbols.get(receiver), model.symbols.get(field)
        if (receiver_symbol is None or receiver_symbol[3] != "10" or field_symbol is None or field_symbol[3] != "12"
                or subject_modules.get(("symbol", receiver)) != module
                or subject_modules.get(("symbol", field)) != module
                or dtype & 31 != int(receiver_symbol[4]) & 31):
            raise ValueError("Member receiver type or selected field identity is invalid")
        if len(links[identity]) != 1 or links[identity][0][4:] != [str(receiver), "member-receiver-type", str(field)]:
            raise ValueError("Member receiver input lacks its matching relationship")
        if statement:
            source = model.statements.get(statement)
            if source is None or int(source[7]) != module or model.statement_owners.get((domain, identity)) != statement:
                raise ValueError("Member receiver statement ownership is invalid")
        if model.capabilities[module].get(FEATURE) == "available":
            preprocessor = properties.get("preprocessor-expression") == "1"
            if context != ("preprocessor" if preprocessor else "statement") or (not preprocessor and statement == 0):
                raise ValueError("Member receiver context disagrees with its native expression context")
        elif context is not None or statement == 0:
            raise ValueError("Member receiver context lacks available coverage")
        links.pop(identity)
    if links:
        raise ValueError("Member receiver relationship lacks its input")

# end of semantic_receiver_context.py
