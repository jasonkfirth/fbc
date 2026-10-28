"""Project: FreeBASIC semantic sidecar
File: semantic_object_model.py
Purpose: Validate copied native virtual slots and RTTI initialization phases.
Responsibilities: Ownership, inherited contents, member indices and full closure.
This file intentionally does NOT resolve an object's runtime dynamic type.
"""

from collections import defaultdict, deque

FEATURE = "native-object-model"
LIMIT = 1000000
PREFIXES = ("object-model-snapshot:", "virtual-table-build:", "virtual-table-slot:",
            "virtual-table-member:", "virtual-table-end:", "rtti-initialization:", "object-model-counts:")


def validate_object_model(model, number, subject_modules):
    snapshots, builds, rtti = defaultdict(dict), defaultdict(dict), defaultdict(dict)
    slots, members = defaultdict(dict), defaultdict(dict)
    ends, closures = defaultdict(dict), {}
    pending = defaultdict(deque)
    latest_types, latest_snapshots = {}, {}
    tables_by_type, rtti_by_type = {}, {}
    expected_snapshots = defaultdict(int)
    slot_origins = defaultdict(dict)
    rtti_histories = defaultdict(list)
    flag_masks = {owner: {(info[0], info[1]): int(info[2]) for info in definitions.values()}
                  for owner, definitions in model.flag_definitions.items()}
    base_rtti_links = []
    assigned_members = defaultdict(set)
    slot_totals, member_totals = defaultdict(int), defaultdict(int)
    for proc, signature in model.signatures.items():
        typed = model.types.get(proc)
        if typed is not None and int(signature[12]) > 0:
            assigned_members[int(typed[8])].add(proc)
    module = 0

    def integer(value, minimum=0, maximum=4294967295):
        result = number(value, minimum)
        if str(result) != value or result > maximum:
            raise ValueError("Object model integer is not canonical or bounded")
        return result

    def symbol(value, raw_class=None, nullable=False):
        identity = integer(value)
        if not identity and nullable:
            return identity
        info = model.symbols.get(identity)
        if (info is None or subject_modules.get(("symbol", identity)) != module
                or raw_class is not None and info[3] != raw_class):
            raise ValueError("Object model symbol is foreign or has another class")
        return identity

    def global_owner(row):
        owner = symbol(row[2], "8")
        if row[1] != "symbol" or model.symbols[owner][11] != "0":
            raise ValueError("Object model event lacks its module namespace owner")

    def flag_mask(group, name):
        return flag_masks.get(module, {}).get((group, name), 0)

    for row in model.rows:
        if row[0] == "M":
            module += 1
            continue
        available = model.capabilities.get(module, {}).get(FEATURE) == "available"
        if available and row[0] == "T":
            latest_types[int(row[1])] = row
        elif available and row[0] == "U" and row[2] in ("type", "union"):
            pending[int(row[1])].append(row)
            expected_snapshots[module] += 1
        if row[0] != "K" or not row[3].startswith(PREFIXES):
            continue
        if not available:
            raise ValueError("Object model observation has unavailable coverage")
        parts, fields = row[3].split(":"), row[4].split("\t")
        if len(parts) not in (2, 3):
            raise ValueError("Object model property key is malformed")
        identity = integer(parts[1], 1, LIMIT)
        if parts[0] == "object-model-counts":
            global_owner(row)
            if len(parts) != 2 or identity != module or module in closures or len(fields) != 5:
                raise ValueError("Object model module closure is repeated or foreign")
            closures[module] = [integer(value, 0, LIMIT) for value in fields]
            continue
        if module in closures:
            raise ValueError("Object model observation follows module closure")
        if parts[0] == "object-model-snapshot":
            owner = symbol(row[2], "10")
            if (row[1] != "symbol" or len(parts) != 2 or len(fields) != 9 or not pending[owner]
                    or identity != len(snapshots[module]) + 1):
                raise ValueError("Object model snapshot has no native U companion")
            native = pending[owner].popleft()
            state = fields[0]
            base = symbol(fields[1], "10", nullable=True)
            table = symbol(fields[2], "1", nullable=True)
            info = symbol(fields[3], "1", nullable=True)
            count, abstracts = (integer(value, 0, LIMIT) for value in fields[4:6])
            table_unused, rtti_unused, has_rtti = (integer(value, 0, 1) for value in fields[6:9])
            typed = latest_types.get(owner)
            mask = flag_mask("status", "has-rtti")
            if (fields[1] != native[3] or fields[5] != native[8] or typed is None or not mask
                    or bool(int(typed[12]) & mask) != bool(has_rtti)):
                raise ValueError("Object snapshot disagrees with its live type/base/status")
            if state == "plain":
                if has_rtti or table or info or count or abstracts or table_unused or rtti_unused:
                    raise ValueError("Plain storage invents virtual table or RTTI state")
            elif state == "pending":
                if not has_rtti or table and info:
                    raise ValueError("Pending object snapshot invents completed ownership")
            elif state == "owned":
                if not has_rtti or not table or not info or count < 2 or abstracts > count - 2:
                    raise ValueError("Completed object snapshot lacks its table, RTTI or slots")
            elif state in ("runtime", "runtime-pending"):
                if (not has_rtti or base or table or count != 2 or abstracts or table_unused
                        or bool(info) != (state == "runtime")):
                    raise ValueError("Runtime object invents compiler-owned table contents")
            else:
                raise ValueError("Unknown native object snapshot state")
            if info:
                previous = rtti_by_type.setdefault(owner, info)
                if previous != info:
                    raise ValueError("Object type changes its native RTTI identity")
            snapshots[module][identity] = (owner, tuple(fields))
            latest_snapshots[owner] = tuple(fields)
            continue
        global_owner(row)
        if parts[0] == "virtual-table-build":
            if len(parts) != 2 or len(fields) != 9 or identity != len(builds[module]) + 1:
                raise ValueError("Virtual table build is malformed or reordered")
            owner = symbol(fields[0], "10")
            table = symbol(fields[1], "1")
            info = symbol(fields[2], "1")
            base = symbol(fields[3], "10")
            base_table = symbol(fields[4], "1", nullable=True)
            count, base_count, copied = (integer(value, 0, LIMIT) for value in fields[5:8])
            integer(fields[8], 0, 1)
            if (owner in tables_by_type or count < 2 or base_count > count
                    or copied != max(base_count - 2, 0) or copied and not base_table):
                raise ValueError("Virtual table build has another count or inherited payload source")
            if owner in rtti_by_type and rtti_by_type[owner] != info:
                raise ValueError("Virtual table selects another type's RTTI")
            tables_by_type[owner] = (module, identity)
            builds[module][identity] = tuple(fields)
        elif parts[0] in ("virtual-table-slot", "virtual-table-member"):
            if len(parts) != 3 or identity not in builds[module] or identity in ends[module]:
                raise ValueError("Virtual table component has no open build")
            build = builds[module][identity]
            ordinal = integer(parts[2], 0 if parts[0] == "virtual-table-slot" else 1, LIMIT)
            bucket = slots[module, identity] if parts[0] == "virtual-table-slot" else members[module, identity]
            if ordinal in bucket or len(fields) != 4:
                raise ValueError("Virtual table component is repeated or malformed")
            if parts[0] == "virtual-table-slot":
                if ordinal != len(bucket) or ordinal >= int(build[5]) or fields[0] not in ("null", "address"):
                    raise ValueError("Virtual slot order or kind is invalid")
                target = symbol(fields[1], nullable=True)
                offset = integer(fields[2], 0, 9223372036854775807)
                size = integer(fields[3], 0, 9223372036854775807)
                width = int(model.contexts[module - 1][6])
                if (offset != ordinal * width or size != width or bool(target) != (fields[0] == "address")
                        or ordinal == 0 and target or ordinal == 1 and fields[1] != build[2]
                        or ordinal >= 2 and target and model.symbols[target][3] != "3"):
                    raise ValueError("Virtual slot contradicts its actual pointer layout or target")
            else:
                if ordinal != len(bucket) + 1:
                    raise ValueError("Virtual member order is incomplete")
                slot = integer(fields[0], 2, LIMIT)
                proc = symbol(fields[1], "3")
                abstract = integer(fields[2], 0, 1)
                overridden = symbol(fields[3], "3", nullable=True)
                signature, typed = model.signatures.get(proc), model.types.get(proc)
                mask = flag_mask("procedure-attributes", "abstract")
                if (slot >= int(build[5]) or signature is None or typed is None or not mask
                        or signature[12] != fields[0] or signature[11] != fields[3]
                        or typed[8] != build[0] or bool(int(typed[11]) & mask) != bool(abstract)):
                    raise ValueError("Virtual member contradicts its assigned slot, owner or override")
            bucket[ordinal] = tuple(fields)
            if parts[0] == "virtual-table-slot":
                slot_totals[module] += 1
            else:
                member_totals[module] += 1
        elif parts[0] == "virtual-table-end":
            if len(parts) != 2 or len(fields) != 2 or identity not in builds[module] or identity in ends[module]:
                raise ValueError("Virtual table closes another build")
            build = builds[module][identity]
            current_slots, current_members = slots[module, identity], members[module, identity]
            if [integer(value, 0, LIMIT) for value in fields] != [len(current_slots), len(current_members)] or fields[0] != build[5]:
                raise ValueError("Virtual table slot/member coverage is incomplete")
            own = {}
            for info in current_members.values():
                slot = int(info[0])
                if slot in own:
                    raise ValueError("Virtual table repeats a member's assigned slot")
                own[slot] = info
            if {int(info[1]) for info in current_members.values()} != assigned_members[int(build[0])]:
                raise ValueError("Virtual table omits an assigned native member")
            base_build_key = tables_by_type.get(int(build[3]))
            base_slots = slots[base_build_key] if base_build_key is not None else {}
            base_origins = slot_origins[base_build_key[0]].get(base_build_key[1], {}) if base_build_key is not None else {}
            if int(build[7]):
                if base_build_key is None or builds[base_build_key[0]][base_build_key[1]][1] != build[4]:
                    raise ValueError("Copied virtual slots lack their actual base table")
            origins = {}
            for slot, actual in current_slots.items():
                if slot < 2:
                    continue
                member = own.get(slot)
                if member is not None and member[2] == "0":
                    if actual[1] != member[1]:
                        raise ValueError("Concrete virtual member lost its selected native pointer")
                    selection = "override" if member[3] != "0" else "declared"
                    copied_from = 0
                elif slot < int(build[7]) + 2:
                    if slot not in base_slots or actual[:2] != base_slots[slot][:2]:
                        raise ValueError("Inherited virtual slot differs from its selected base payload")
                    selection, copied_from = "inherited", int(build[4])
                else:
                    if actual[0] != "null":
                        raise ValueError("Unselected virtual slot invents a procedure pointer")
                    selection, copied_from = "null-fill", 0
                inherited = base_origins.get(slot, {})
                declaration = int(member[1]) if member is not None else inherited.get("declaration", 0)
                origins[slot] = {"declaration": declaration, "selected": int(actual[1]),
                                 "selection": selection, "copied_from": copied_from}
            slot_origins[module][identity] = origins
            ends[module][identity] = tuple(fields)
        elif parts[0] == "rtti-initialization":
            if len(parts) != 2 or len(fields) != 11 or identity != len(rtti[module]) + 1:
                raise ValueError("RTTI initialization is malformed or reordered")
            owner, info = symbol(fields[0], "10"), symbol(fields[1], "1")
            state = fields[2]
            base = symbol(fields[3], "10", nullable=True)
            base_info = symbol(fields[4], "1", nullable=True)
            literal = symbol(fields[5], "1", nullable=True)
            count = integer(fields[7], 0, 3)
            history = rtti_histories[owner]
            if state == "deferred":
                if history or base_info or literal or fields[6:] != ["unavailable", "0", "", "", ""]:
                    raise ValueError("Deferred RTTI invents initialized payload fields")
            elif state in ("initialized", "activated"):
                if ((state == "initialized" and history) or (state == "activated" and (len(history) != 1 or history[0][2] != "deferred"))
                        or not literal or fields[6] != "null" or count != 3):
                    raise ValueError("RTTI initialization has another native phase or payload")
                value = model.constants.get(("symbol", literal))
                if value is None or value[0] != "bytes" or not value[1]:
                    raise ValueError("RTTI type name lacks its actual native literal")
                if not base and base_info:
                    raise ValueError("RTTI initializer links another base type's descriptor")
                if base:
                    base_rtti_links.append((base, base_info))
                width = int(model.contexts[module - 1][6])
                if [integer(value, 0, 9223372036854775807) for value in fields[8:]] != [0, width, width * 2]:
                    raise ValueError("RTTI pointer fields have another native layout")
            else:
                raise ValueError("Unknown RTTI initialization phase")
            if owner in rtti_by_type and rtti_by_type[owner] != info:
                raise ValueError("RTTI initialization changes its owning type")
            rtti_by_type[owner] = info
            rtti[module][identity] = tuple(fields)
            history.append(tuple(fields))
        else:
            raise ValueError("Unknown object model observation family")

    if any(pending.values()):
        raise ValueError("Native UDT lacks its object model snapshot")
    # Native initialization can precede the first complete symbol snapshot
    # for OBJECT's external RTTI. Resolve these copied identities after the
    # module's snapshots, without filling a provisional payload retroactively.
    if any(rtti_by_type.get(base) != info for base, info in base_rtti_links):
        raise ValueError("RTTI initializer links another base type's descriptor")
    for owner, snapshot in latest_snapshots.items():
        if snapshot[0] != "owned":
            continue
        key = tables_by_type.get(owner)
        if key is None:
            raise ValueError("Owned object type lacks its actual virtual table build")
        build = builds[key[0]][key[1]]
        if (build[1:3] != snapshot[2:4] or build[3] != snapshot[1] or build[5] != snapshot[4]):
            raise ValueError("Completed object snapshot contradicts its native table build")
        history = rtti_histories[owner]
        if not history or (history[-1][2] == "deferred") != (snapshot[7] == "1"):
            raise ValueError("Owned object type lacks its current RTTI initialization state")
    model.object_models = {}
    for module, capabilities in model.capabilities.items():
        if capabilities.get(FEATURE) != "available":
            continue
        counts = [len(snapshots[module]), len(builds[module]),
                  slot_totals[module], member_totals[module], len(rtti[module])]
        if (module not in closures or closures[module] != counts or counts[0] != expected_snapshots[module]
                or set(builds[module]) != set(ends[module])):
            raise ValueError("Native object model module coverage is incomplete")
        model.object_models[module] = {"snapshot": snapshots[module], "build": builds[module],
                                       "slots": {key: value for (owner, key), value in slots.items() if owner == module},
                                       "members": {key: value for (owner, key), value in members.items() if owner == module},
                                       "rtti": rtti[module], "origins": slot_origins[module]}

# end of semantic_object_model.py
