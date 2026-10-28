"""Project: FreeBASIC semantic sidecar
File: semantic_flags.py
Purpose: Validate self-describing named flags and primitive type policies.
Responsibilities: Class-aware masks, snapshot timing, enums and bounded closure.
This file intentionally does NOT assign public meaning to fixed private bit positions.
"""

from collections import Counter, defaultdict

FEATURE = "named-symbol-flags"
POLICY_FEATURE = "primitive-type-policy"
LIMIT = 1000000
GROUPS = ("attributes", "procedure-attributes", "status", "aggregate-options", "procedure-status")
SYMBOL_CLASSES = ("variable", "constant", "procedure", "parameter", "define", "keyword", "label",
                  "namespace", "enum", "type", "class", "field", "typedef", "forward-type",
                  "scope", "reserved", "namespace-import")
PUBLIC_FLAGS = {
    "attributes": {
        "shared": "*",
        "static": "*",
        "dynamic": "*",
        "common": "*",
        "external": "*",
        "public": "*",
        "private": "*",
        "local": "*",
        "exported": "*",
        "imported": "*",
        "literal": "*",
        "const": "*",
        "temporary": ["variable"],
        "array-descriptor": ["variable", "type", "field"],
        "parameter-variable-bydesc": ["variable"],
        "parameter-variable-byval": ["variable"],
        "parameter-variable-byref": ["variable"],
        "function-result": ["variable"],
        "reference-storage": ["variable", "field"],
        "instance-parameter": ["variable", "parameter"],
        "type-suffixed": "*",
        "member-private": "*",
        "member-protected": "*",
        "internal": "*",
        "anonymous": "*",
        "thread-local": ["variable"],
        "c-thread-local": ["variable"],
    },
    "procedure-attributes": {
        "overloaded": ["procedure"],
        "instance-method": ["procedure"],
        "constructor": ["procedure"],
        "complete-destructor": ["procedure"],
        "operator": ["procedure"],
        "property": ["procedure"],
        "static-locals": ["procedure"],
        "naked": ["procedure"],
        "virtual": ["procedure"],
        "abstract": ["procedure"],
        "ignore-this-constness": ["procedure"],
        "return-by-reference": ["procedure"],
        "deleting-destructor": ["procedure"],
        "sound-intrinsic": ["procedure"],
        "graphics-intrinsic": ["procedure"],
    },
    "status": {
        "accessed": "*",
        "declared": "*",
        "runtime": "*",
        "throwable": "*",
        "parsed": "*",
        "has-alias": "*",
        "cannot-duplicate": "*",
        "cannot-undefine": "*",
        "emitted": "*",
        "being-emitted": "*",
        "constructor-initialized": ["procedure"],
        "main-procedure": ["procedure"],
        "module-procedure": ["procedure"],
        "procedure-pointer-type": ["procedure"],
        "global-constructor": ["procedure"],
        "global-destructor": ["procedure"],
        "call-cloneable": ["procedure"],
        "utf8-arguments": ["procedure"],
        "no-return": ["procedure"],
        "procedure-emitted": ["procedure"],
        "exclude-mangling-parent": ["procedure"],
        "implicit-variable": ["variable"],
        "rtti-storage": ["variable"],
        "vtable-storage": ["variable"],
        "jump-table-storage": ["variable"],
        "temporary-lifetime": ["variable"],
        "do-not-initialize": ["variable", "field", "parameter"],
        "argv-parameter": ["variable", "parameter"],
        "redefinable-constant": ["constant"],
        "fixed-length-string-alias": ["typedef"],
        "has-rtti": ["type"],
        "unused-vtable": ["variable"],
        "union-field": ["field"],
        "declared-as-any": ["parameter"],
    },
    "aggregate-options": {
        "union": ["type"],
        "anonymous": ["type"],
        "has-pointer-field": ["type"],
        "has-constructor-field": ["type"],
        "has-destructor-field": ["type"],
        "recursive-byval-parameter": ["type"],
        "recursive-byval-result": ["type"],
        "get-property": ["type"],
        "set-property": ["type"],
        "index-get-property": ["type"],
        "index-set-property": ["type"],
        "has-keyword-field": ["type"],
        "has-initialized-field": ["type"],
        "has-anonymous-union": ["type"],
        "has-static-variable": ["type"],
        "has-bitfield": ["type"],
        "wstring-wrapper": ["type"],
        "zstring-wrapper": ["type"],
        "nested-type": ["type"],
        "has-zeroed-field": ["type"],
        "has-filled-field": ["type"],
    },
    "procedure-status": {
        "return-used": ["procedure"],
        "result-assignment-used": ["procedure"],
        "gosub-used": ["procedure"],
    },
}
RETURN_METHODS = frozenset(("fpu", "sse", "default"))
AGGREGATE_RETURNS = frozenset(("none", "integer", "integer-integer", "integer-float",
                               "float", "float-integer", "float-float"))
SIZE_CATEGORIES = frozenset(("none", "boolean", "signed-8", "unsigned-8", "signed-16", "unsigned-16",
                             "signed-32", "unsigned-32", "signed-64", "unsigned-64",
                             "signed-128", "unsigned-128", "float-32", "float-64"))


def validate_flags(model, number, subject_modules):
    definitions, closures, enums = defaultdict(dict), {}, defaultdict(lambda: defaultdict(dict))
    scalars, policy_rows = defaultdict(dict), defaultdict(dict)
    snapshots = defaultdict(lambda: defaultdict(dict))
    expected = defaultdict(Counter)
    enum_counts = defaultdict(Counter)
    module = 0
    recognized = {}

    def decimal(value, minimum=0, maximum=4294967295):
        result = number(value, minimum)
        if str(result) != value or result > maximum:
            raise ValueError("Flag identity or mask is not canonical or bounded")
        return result

    def owner(row, current_module, global_owner=False):
        identity = decimal(row[2], 1)
        symbol = model.symbols.get(identity)
        if (row[1] != "symbol" or symbol is None
                or subject_modules.get(("symbol", identity)) != current_module):
            raise ValueError("Named flags have a foreign symbol owner")
        if global_owner and (symbol[3] != "8" or symbol[11] != "0"):
            raise ValueError("Flag dictionary or closure has no global namespace owner")
        return identity

    def dictionary(current_module):
        result = definitions[current_module]
        count = scalars[current_module].get("flag-definition-count")
        if (scalars[current_module].get("named-flag-contract") != "1" or count is None
                or len(result) != decimal(count, 0, LIMIT)
                or list(result) != list(range(len(result)))):
            raise ValueError("Named flag dictionary is incomplete or has another version")
        expected_keys = {(group, name) for group, names in PUBLIC_FLAGS.items() for name in names}
        if len(result) != len(expected_keys) or {(row[0], row[1]) for row in result.values()} != expected_keys:
            raise ValueError("Named flag vocabulary is missing or unknown")
        selected = defaultdict(list)
        masks = defaultdict(int)
        for group, name, mask, classes in result.values():
            names = SYMBOL_CLASSES if classes == "*" else classes.split(",")
            if classes != "*" and (len(set(names)) != len(names) or any(name not in SYMBOL_CLASSES for name in names)):
                raise ValueError("Flag definition has invalid class applicability")
            declared = PUBLIC_FLAGS[group][name]
            if (declared == "*" and classes != "*"
                    or declared != "*" and set(names) != set(declared)):
                raise ValueError("Flag definition changes a class-specific meaning")
            bits = decimal(mask, 1)
            if bits & (bits - 1):
                raise ValueError("Flag definition is not a single native bit")
            for symbol_class in names:
                key = group, symbol_class
                if masks[key] & bits:
                    raise ValueError("Several flag names apply to the same bit and class")
                masks[key] |= bits
                selected[key].append((name, bits))
        return selected, masks

    def decode(current_module, group, symbol_class, raw, names, unmapped):
        if current_module not in recognized:
            recognized[current_module] = dictionary(current_module)
        selected, masks = recognized[current_module]
        expected_names = ",".join(name for name, bit in selected[group, symbol_class] if raw & bit)
        expected_unknown = raw & ~masks[group, symbol_class] & 4294967295
        if names != expected_names or decimal(unmapped) != expected_unknown:
            raise ValueError("Named flags contradict their native mask or symbol class")

    for index, row in enumerate(model.rows):
        if row[0] == "M":
            module += 1
            continue
        available = model.capabilities.get(module, {}).get(FEATURE) == "available"
        if available and row[0] == "T":
            expected[module]["symbol"] += 1
        if available and row[0] == "U" and row[2] in ("type", "union"):
            expected[module]["aggregate"] += 1
        if available and row[0] == "K" and row[3] == "procedure-status":
            expected[module]["procedure"] += 1
        if available and row[0] == "F":
            enum_counts[module]["return-method"] += 1
        if available and row[0] == "K" and row[3] == "aggregate-register-return":
            enum_counts[module]["aggregate-return"] += 1
        if row[0] != "K":
            continue
        key = row[3]
        global_property = (key.startswith(("flag-definition:", "return-method-definition:",
                                          "aggregate-return-definition:", "named-flag-counts:",
                                          "primitive-type-policy:"))
                           or key in ("flag-definition-count", "named-flag-contract", "integer-promotion-policy"))
        flag_property = (global_property or key.startswith(("symbol-flags:", "aggregate-flags:", "procedure-flags:"))
                         or key in ("return-method-kind", "aggregate-register-return-kind"))
        if not flag_property:
            continue
        policy = key.startswith("primitive-type-policy:") or key == "integer-promotion-policy"
        feature = POLICY_FEATURE if policy else FEATURE
        if model.capabilities[module].get(feature) != "available":
            raise ValueError("Flag or type policy has unavailable coverage")
        identity = owner(row, module, global_property)
        fields = row[4].split("\t")
        previous = model.rows[index - 1]
        if key in ("flag-definition-count", "named-flag-contract", "integer-promotion-policy"):
            if key in scalars[module]:
                raise ValueError("Repeated flag dictionary scalar")
            scalars[module][key] = row[4]
        elif key.startswith("flag-definition:"):
            ordinal = decimal(key.partition(":")[2], 0, LIMIT)
            if (len(fields) != 4 or ordinal != len(definitions[module])
                    or fields[0] not in PUBLIC_FLAGS or fields[1] not in PUBLIC_FLAGS[fields[0]]):
                raise ValueError("Malformed or reordered flag definition")
            definitions[module][ordinal] = fields
        elif key.startswith(("return-method-definition:", "aggregate-return-definition:")):
            family, ordinal = key.split(":")
            ordinal = decimal(ordinal, 0, LIMIT)
            bucket = enums[module][family]
            if len(fields) != 2 or ordinal != len(bucket):
                raise ValueError("Malformed or reordered return-method definition")
            decimal(fields[0])
            bucket[ordinal] = fields
        elif key.startswith("primitive-type-policy:"):
            primitive = decimal(key.partition(":")[2], 0)
            if primitive in policy_rows[module] or len(fields) != 4:
                raise ValueError("Repeated or malformed primitive type policy")
            policy_rows[module][primitive] = fields
        elif key.startswith("named-flag-counts:"):
            if decimal(key.partition(":")[2], 1) != module or module in closures or len(fields) != 3:
                raise ValueError("Repeated or foreign named flag closure")
            closures[module] = [decimal(value, 0, LIMIT) for value in fields]
        elif key.startswith(("symbol-flags:", "aggregate-flags:", "procedure-flags:")):
            family, ordinal = key.split(":")
            family = family.split("-")[0]
            ordinal = decimal(ordinal, 1, LIMIT)
            bucket = snapshots[module][family]
            if ordinal != len(bucket) + 1 or module in closures:
                raise ValueError("Flag snapshots are repeated, reordered or follow closure")
            if family == "symbol":
                if previous[:2] != ["T", str(identity)] or len(fields) != 6:
                    raise ValueError("Symbol flags lack their adjacent live type snapshot")
                symbol_class = "type" if previous[3] == "union" else previous[3]
                for slot, group in enumerate(GROUPS[:3]):
                    decode(module, group, symbol_class, decimal(previous[10 + slot]),
                           fields[slot * 2], fields[slot * 2 + 1])
            elif family == "aggregate":
                if (previous[:2] != ["U", str(identity)] or previous[2] not in ("type", "union")
                        or len(fields) != 2):
                    raise ValueError("Aggregate flags read another symbol payload")
                decode(module, GROUPS[3], "type", decimal(previous[6]), *fields)
            else:
                if (previous[:4] != ["K", "symbol", str(identity), "procedure-status"]
                        or model.symbols[identity][3] != "3" or len(fields) != 2):
                    raise ValueError("Procedure flags read an absent or different payload")
                decode(module, GROUPS[4], "procedure", decimal(previous[4]), *fields)
            bucket[ordinal] = (identity, tuple(fields))
        elif key == "return-method-kind":
            if previous[:2] != ["F", str(identity)]:
                raise ValueError("Return method lacks its adjacent signature")
            mapping = {raw: name for raw, name in enums[module]["return-method-definition"].values()}
            if mapping.get(previous[9]) != row[4]:
                raise ValueError("Named return method disagrees with the native signature")
            enum_counts[module]["return-observed"] += 1
        else:
            if previous[:4] != ["K", "symbol", str(identity), "aggregate-register-return"]:
                raise ValueError("Aggregate return kind lacks its initialized native classifier")
            mapping = {raw: name for raw, name in enums[module]["aggregate-return-definition"].values()}
            if mapping.get(previous[4]) != row[4]:
                raise ValueError("Aggregate return kind disagrees with native classification")
            enum_counts[module]["aggregate-observed"] += 1

    model.named_flag_snapshots = {}
    model.flag_definitions = {}
    model.type_policies = {}
    for module, capabilities in model.capabilities.items():
        if capabilities.get(FEATURE) == "available":
            dictionary(module)
            if (module not in closures or closures[module] != [expected[module][family] for family in ("symbol", "aggregate", "procedure")]
                    or [len(snapshots[module][family]) for family in ("symbol", "aggregate", "procedure")] != closures[module]):
                raise ValueError("Named flag snapshot coverage is incomplete")
            for family, names in (("return-method-definition", RETURN_METHODS), ("aggregate-return-definition", AGGREGATE_RETURNS)):
                rows = enums[module][family]
                if (len(rows) != len(names) or {value[1] for value in rows.values()} != names
                        or len({value[0] for value in rows.values()}) != len(rows)):
                    raise ValueError("Return-method vocabulary is incomplete or ambiguous")
            if (enum_counts[module]["return-method"] != enum_counts[module]["return-observed"]
                    or enum_counts[module]["aggregate-return"] != enum_counts[module]["aggregate-observed"]):
                raise ValueError("Named return classification coverage is incomplete")
            model.flag_definitions[module] = definitions[module]
            model.named_flag_snapshots[module] = snapshots[module]
        if capabilities.get(POLICY_FEATURE) == "available":
            policies = policy_rows[module]
            primitives = model.primitives[module - 1]
            names = {row[2] for row in primitives.values()}
            if set(policies) != set(primitives):
                raise ValueError("Primitive type policy coverage is incomplete")
            for primitive, fields in policies.items():
                if (fields[0] != primitives[primitive][2] or fields[2] not in names
                        or fields[3] not in SIZE_CATEGORIES):
                    raise ValueError("Primitive type policy has another type or storage category")
                decimal(fields[1], 0)
            expected_policy = "disabled-qb" if model.contexts[module - 1][4] == "qb" else "rank-based"
            if scalars[module].get("integer-promotion-policy") != expected_policy:
                raise ValueError("Integer promotion policy disagrees with the language")
            model.type_policies[module] = policies

# end of semantic_flags.py

