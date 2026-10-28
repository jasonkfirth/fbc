"""Project: FreeBASIC semantic sidecar
File: semantic_layout.py
Purpose: Validate original packing inputs and actual native layout decisions.
Responsibilities: Capture-time identities, storage dispositions and complete closure.
This file intentionally does NOT infer final layout from source type spelling.
"""

from collections import Counter, defaultdict

FEATURE = "native-aggregate-layout"
PACKING_FEATURE = "aggregate-packing-inputs"
LIMIT = 1000000
MAX_LENGTH = 9223372036854775807
FAMILIES = ("packing", "begin", "field", "insert", "promote", "end", "snapshot")
PREFIXES = {"aggregate-packing-input:": "packing"}
PREFIXES.update({"aggregate-layout-" + family + ":": family for family in FAMILIES[1:]})
COUNTS = "aggregate-layout-counts:"
SUBJECT = "aggregate-layout-subject:"
INSERT_RESULT = "aggregate-layout-insert-result:"


def validate_layout(model, number, subject_modules):
    observations = defaultdict(lambda: defaultdict(dict))
    subjects, closures = defaultdict(dict), {}
    expected = defaultdict(lambda: defaultdict(Counter))
    first_seen = defaultdict(dict)
    packing, natural, progress, field_offsets = {}, {}, {}, {}
    latest_types, pending_layouts, native_properties = {}, {}, defaultdict(dict)
    last_finalized, begins, fields, insertions = {}, {}, {}, {}
    member_fields = defaultdict(set)
    insertion_members, insertion_fields = {}, {}
    expected_insert_fields = {}
    native_fields = defaultdict(list)
    for identity, native in model.symbols.items():
        if native[3] == "12":
            native_fields[subject_modules.get(("symbol", identity))].append((identity, native))
    active_insert = None
    module = 0
    descriptor_masks = {module: next(int(mask) for group, name, mask, classes in definitions.values()
                                     if group == "attributes" and name == "array-descriptor")
                        for module, definitions in model.flag_definitions.items()}

    def integer(value, minimum=0, maximum=MAX_LENGTH):
        result = number(value, minimum)
        if str(result) != value or result > maximum or result < -MAX_LENGTH - 1:
            raise ValueError("Layout integer is not canonical or bounded")
        return result

    def flag(value):
        return integer(value, 0, 1)

    def symbol(value, raw_class=None, nullable=False):
        identity = integer(value)
        if identity == 0 and nullable:
            return 0
        row = model.symbols.get(identity)
        if (row is None or subject_modules.get(("symbol", identity)) != module
                or raw_class is not None and row[3] != raw_class):
            raise ValueError("Aggregate layout has a foreign or inappropriate symbol")
        return identity

    def touch(identity):
        if identity not in first_seen[module]:
            first_seen[module][identity] = len(first_seen[module]) + 1

    def context(statement, source, compound):
        statement, source, compound = integer(statement), integer(source), integer(compound)
        if statement:
            row = model.statements.get(statement)
            if row is None or int(row[7]) != module or row[5] != str(source):
                raise ValueError("Aggregate layout has another statement context")
        if source:
            row = model.source_contexts.get(source)
            if row is None or int(row[4]) != module:
                raise ValueError("Aggregate layout has another source occurrence")
        if compound:
            row = model.constructs.get(compound)
            opening = None if row is None else model.statements.get(int(row[3]))
            if opening is None or int(opening[7]) != module:
                raise ValueError("Aggregate layout has another construct context")

    def selected_packing(passed):
        target = model.contexts[module - 1]
        return 1 if passed == 0 and target[4] == "qb" and target[2].split("-", 1)[0] != "js" else passed

    def type_alignment(dtype, nominal):
        primitives = model.primitives[module - 1]
        if dtype & 0x1e0:
            primitive = next(row for row in primitives.values() if row[2] == "pointer")
        else:
            primitive = primitives.get(dtype & 31)
        if primitive is None:
            raise ValueError("Layout uses an unknown native datatype")
        if not dtype & 0x1e0 and primitive[2] == "type":
            return natural.get((module, nominal))
        return integer(primitive[5], 1, 16) if primitive[5] else None

    def decision(values, *, descriptor=False):
        if len(values) != 8:
            raise ValueError("Native alignment decision has another shape")
        offset, pack, dtype, nominal, nat, effective = (integer(value) for value in values[:6])
        reason, pad = values[6], integer(values[7])
        if reason == "none":
            if any((offset, pack, dtype, nominal, nat, effective, pad)):
                raise ValueError("Uncalculated alignment invents a native decision")
            return offset, pack, dtype, nominal, nat, effective, reason, pad
        if reason not in ("natural", "packing", "managed-descriptor"):
            raise ValueError("Unknown native alignment reason")
        symbol(str(nominal), nullable=True)
        integer(str(dtype), 0, 4294967295)
        integer(str(pack), 0, 255)
        integer(str(nat), 1, 16)
        integer(str(effective), 1, 16)
        known = type_alignment(dtype, nominal)
        if known is not None and nat != known:
            raise ValueError("Native alignment contradicts the captured type policy")
        primitive = model.primitives[module - 1][dtype & 31][2]
        managed = descriptor or (not dtype & 0x1e0 and primitive in ("string", "ustring"))
        expected_reason = "natural" if pack == 0 else "managed-descriptor" if managed else "packing"
        chosen = nat if pack == 0 or managed else min(pack, nat)
        # FIELD accepts some non-power-of-two requests. Retain the native
        # mask calculation, including that language behavior, without making
        # a stronger claim that every selected alignment is a power of two.
        computed = (chosen - (offset & (chosen - 1))) & (chosen - 1)
        if reason != expected_reason or effective != chosen or pad != computed:
            raise ValueError("Native alignment choice or padding is inconsistent")
        return offset, pack, dtype, nominal, nat, effective, reason, pad

    for row in model.rows:
        tag = row[0]
        if tag == "M":
            if active_insert is not None or pending_layouts:
                raise ValueError("Aggregate layout remains open across modules")
            module += 1
            continue
        available = model.capabilities.get(module, {}).get(FEATURE) == "available"
        if available and module not in descriptor_masks:
            raise ValueError("Native aggregate layout lacks its descriptor flag policy")
        if available and tag == "T":
            latest_types[int(row[1])] = row
        elif available and tag == "U" and row[2] in ("type", "union"):
            identity = int(row[1])
            if identity in pending_layouts:
                raise ValueError("Aggregate U lacks its preceding layout companion")
            pending_layouts[identity] = row
        elif available and tag == "K" and row[3] in ("natural-alignment", "packing-alignment", "layout-finalized"):
            native_properties[int(row[2])][row[3]] = row[4]
        if tag != "K":
            continue
        key = row[3]
        if not key.startswith(("aggregate-layout-", "aggregate-packing-input:")):
            continue
        if not available:
            raise ValueError("Native aggregate layout has unavailable coverage")
        owner = symbol(row[2], "8")
        if row[1] != "symbol" or model.symbols[owner][11] != "0":
            raise ValueError("Aggregate layout observations lack their global owner")
        values = row[4].split("\t")
        prefix, separator, suffix = key.rpartition(":")
        if not separator:
            raise ValueError("Malformed aggregate layout property")
        prefix += ":"
        ordinal = integer(suffix, 0, LIMIT)
        if prefix == COUNTS:
            if ordinal != module or module in closures or len(values) != 8:
                raise ValueError("Repeated or foreign aggregate layout closure")
            closures[module] = [integer(value, 0, LIMIT) for value in values]
            continue
        if module in closures:
            raise ValueError("Aggregate layout observation follows module closure")
        if prefix == SUBJECT:
            if len(values) != 10 or ordinal != len(subjects[module]) + 1:
                raise ValueError("Malformed or reordered aggregate layout subject")
            identity = symbol(values[0], "10")
            if (identity in subjects[module] or first_seen[module].get(identity) != ordinal
                    or values[1] not in ("created", "preexisting")):
                raise ValueError("Repeated or unobserved aggregate layout subject")
            finalized = flag(values[2])
            counts = [integer(value, 0, LIMIT) for value in values[3:]]
            if counts != [expected[module][identity][family] for family in FAMILIES]:
                raise ValueError("Aggregate layout subject has incomplete native observations")
            if (values[1] == "created") != ((module, identity) in begins):
                raise ValueError("Aggregate layout invents an earlier creation")
            if (module, identity) in last_finalized and finalized != last_finalized[module, identity]:
                raise ValueError("Aggregate layout subject contradicts its final observed state")
            subjects[module][identity] = tuple(values)
            continue
        if prefix == INSERT_RESULT:
            if active_insert is None or active_insert[:2] != (module, ordinal) or len(values) != 4:
                raise ValueError("Anonymous insertion closes another native operation")
            parent, initial, start_count = active_insert[2:]
            offset, length, nat, promoted = (integer(value) for value in values)
            insertion = insertions[module, ordinal]
            old_length, old_nat, base, child_length, child_nat, union = map(int, insertion[11:17])
            count = len(observations[module]["promote"]) - start_count
            if promoted != count:
                raise ValueError("Anonymous insertion has an incomplete relocation set")
            selected_fields = insertion_fields[module, ordinal]
            wanted_fields = expected_insert_fields[module, ordinal]
            if wanted_fields is not None and selected_fields != wanted_fields:
                raise ValueError("Anonymous insertion omits native fields from its created inner type")
            if promoted:
                wanted_offset = 0 if union else base + child_length
                wanted_length = max(old_length, child_length) if union else wanted_offset
                wanted_nat = max(old_nat, child_nat)
            else:
                wanted_offset, wanted_length, wanted_nat = base, old_length, old_nat
            if (offset, length, nat) != (wanted_offset, wanted_length, wanted_nat):
                raise ValueError("Anonymous insertion changes another native layout")
            progress[module, parent] = offset, length, nat
            natural[module, parent] = nat
            member_fields[module, parent].update(selected_fields)
            member_fields[module, int(insertion[1])].clear()
            observations[module]["insert-result"][ordinal] = tuple(values)
            active_insert = None
            continue
        family = PREFIXES.get(prefix)
        if family is None:
            raise ValueError("Unknown aggregate layout property")
        bucket = observations[module][family]
        if ordinal != len(bucket) + 1:
            raise ValueError("Aggregate layout ordinals are missing or repeated")
        if family == "promote":
            if len(values) != 6 or active_insert is None or integer(values[0], 1) != active_insert[1]:
                raise ValueError("Anonymous relocation has no active insertion")
            parent = active_insert[2]
            member = symbol(values[1])
            insertion_key = module, active_insert[1]
            if member in insertion_members[insertion_key]:
                raise ValueError("Anonymous insertion relocates a native member twice")
            insertion_members[insertion_key].add(member)
            old, new, actual_owner = integer(values[2]), integer(values[3]), symbol(values[4], "10")
            is_field = flag(values[5])
            insertion = insertions[module, active_insert[1]]
            if actual_owner != parent or is_field != (model.symbols[member][3] == "12"):
                raise ValueError("Anonymous relocation has another native owner or class")
            if is_field:
                insertion_fields[insertion_key].add(member)
            if is_field and (module, member) in field_offsets and old != field_offsets[module, member]:
                raise ValueError("Anonymous field relocation loses its previous offset")
            wanted = old if int(insertion[16]) else old + int(insertion[13])
            if new != wanted:
                raise ValueError("Anonymous relocation has another offset adjustment")
            field_offsets[module, member] = new
            identity = parent
        else:
            if not values:
                raise ValueError("Aggregate layout observation has no subject")
            identity = symbol(values[0], "10")
            touch(identity)
            native_key = module, identity
            if family == "packing":
                if model.capabilities[module].get(PACKING_FEATURE) != "available" or len(values) != 11:
                    raise ValueError("Source packing request has unavailable or malformed coverage")
                context(*values[1:4])
                has_field = flag(values[4])
                expression = integer(values[5])
                selected, passed, actual = (integer(value, 0, 255) for value in values[7:10])
                parent = symbol(values[10], "10", nullable=True)
                if has_field:
                    requested = integer(values[6], None)
                    wanted = 0 if requested < 0 or requested > int(model.contexts[module - 1][6]) else 2 if requested == 3 else requested
                    if selected != wanted:
                        raise ValueError("Source packing loses the actual request coercion")
                    if expression and subject_modules.get(("expression", expression)) != module:
                        raise ValueError("Packing expression belongs to another module")
                elif expression or values[6] or selected:
                    raise ValueError("Absent FIELD invents a request or expression")
                inherited = packing.get((module, parent)) if parent else 0
                if inherited is not None and passed != (selected or inherited):
                    raise ValueError("Anonymous packing loses its actual inherited policy")
                if actual != selected_packing(passed):
                    raise ValueError("Packing request contradicts the selected language policy")
                if (native_key in begins
                        and tuple(values[8:11]) != begins[native_key][1:3] + begins[native_key][4:5]):
                    raise ValueError("Packing request disagrees with native layout creation")
            elif family == "begin":
                if len(values) != 8 or native_key in begins:
                    raise ValueError("Repeated or malformed native aggregate creation")
                passed, actual = (integer(value, 0, 255) for value in values[1:3])
                flag(values[3])
                symbol(values[4], "10", nullable=True)
                context(*values[5:8])
                if actual != selected_packing(passed):
                    raise ValueError("Native aggregate creation changes packing policy")
                begins[native_key] = tuple(values)
                packing[native_key], natural[native_key] = actual, 1
                progress[native_key] = 0, 0, 1
                last_finalized[native_key] = 0
            elif family == "field":
                if len(values) != 23 or values[2] not in ("allocated", "shared-bitfield", "dynamic-alias"):
                    raise ValueError("Native field has an invalid storage disposition")
                field = symbol(values[1], "12")
                if (module, field) in fields:
                    raise ValueError("Native field layout was created twice")
                native = model.symbols[field]
                descriptor = bool(int(native[7]) & descriptor_masks.get(module, 0))
                step = decision(values[3:11], descriptor=descriptor)
                applied = integer(values[11])
                dtype, nominal, element, offset, size, bitpos, bits = (integer(value) for value in values[12:19])
                integer(str(dtype), 0, 4294967295)
                symbol(str(nominal), nullable=True)
                symbol(values[19], "12", nullable=True)
                after = tuple(integer(value) for value in values[20:23])
                if native[4:6] != values[12:14] or native[9:11] != values[14:16] or native[11] != str(identity):
                    raise ValueError("Native field layout contradicts its first symbol snapshot")
                if bits and bitpos + bits > element * 8:
                    raise ValueError("Bitfield extends beyond its selected native container")
                previous = progress.get(native_key)
                union = native_key in begins and begins[native_key][3] == "1"
                if values[2] == "allocated":
                    if step[6] == "none" or applied > step[7] or offset != step[0] + applied:
                        raise ValueError("Allocated field loses its native padding")
                    if previous is not None and step[0] != previous[0]:
                        raise ValueError("Field alignment starts at another native offset")
                    wanted_offset = 0 if union else offset + size
                    wanted_length = max(previous[1], size) if union and previous else size if union else wanted_offset
                    chosen_natural = type_alignment(dtype, nominal)
                    if after[:2] != (wanted_offset, wanted_length):
                        raise ValueError("Allocated field changes another aggregate extent")
                    if previous and chosen_natural is not None and after[2] != max(previous[2], chosen_natural):
                        raise ValueError("Field packing overwrites natural aggregate alignment")
                else:
                    if step[6] != "none" or applied or previous and after != previous:
                        raise ValueError("Shared field invents another allocation or alignment")
                    if values[2] == "shared-bitfield" and not bits:
                        raise ValueError("Nonbitfield claims a shared allocation unit")
                    if values[2] == "dynamic-alias" and values[19] == "0":
                        raise ValueError("Dynamic logical field lacks its actual descriptor")
                integer(str(after[2]), 1, 16)
                progress[native_key], natural[native_key] = after, after[2]
                fields[module, field] = tuple(values)
                field_offsets[module, field] = offset
                member_fields[native_key].add(field)
            elif family == "insert":
                if len(values) != 17 or active_insert is not None:
                    raise ValueError("Malformed or overlapping anonymous insertion")
                child = symbol(values[1], "10")
                touch(child)
                union = flag(values[16])
                step = decision(values[2:10])
                before = tuple(integer(value) for value in values[10:13])
                base, length, nat = (integer(value) for value in values[13:16])
                if native_key in progress and before != progress[native_key]:
                    raise ValueError("Anonymous insertion starts at another aggregate extent")
                if union:
                    if step[6] != "none" or base != before[0]:
                        raise ValueError("Union overlay invents an alignment calculation")
                elif step[6] == "none" or step[0] != before[0] or base != before[0] + step[7]:
                    raise ValueError("Anonymous insertion loses its native padding")
                integer(str(nat), 1, 16)
                insertions[module, ordinal] = tuple(values)
                insertion_members[module, ordinal] = set()
                insertion_fields[module, ordinal] = set()
                expected_insert_fields[module, ordinal] = (set(member_fields[module, child])
                                                           if (module, child) in begins else None)
                active_insert = module, ordinal, identity, before, len(observations[module]["promote"])
            elif family == "end":
                if len(values) != 13:
                    raise ValueError("Malformed native aggregate completion")
                step = decision(values[1:9])
                unpadded, length, nat, pack = (integer(value) for value in values[9:13])
                if step[6] == "none" or step[0] != unpadded or length != unpadded + step[7] or step[4] != nat or step[1] != pack:
                    raise ValueError("Aggregate completion loses its tail padding or natural alignment")
                previous = progress.get(native_key)
                if previous and (previous[1], previous[2]) != (unpadded, nat):
                    raise ValueError("Aggregate completion describes another native extent")
                if native_key in packing and packing[native_key] != pack:
                    raise ValueError("Aggregate completion changes selected packing")
                natural[native_key], packing[native_key] = nat, pack
                if previous:
                    progress[native_key] = previous[0], length, nat
                last_finalized[native_key] = 1
            else:
                if len(values) != 7:
                    raise ValueError("Malformed initialized aggregate layout snapshot")
                native = pending_layouts.pop(identity, None)
                typed = latest_types.get(identity)
                nat, pack, length, unpadded = (integer(value) for value in values[1:5])
                finalized, base = flag(values[5]), symbol(values[6], "12", nullable=True)
                if (native is None or typed is None or typed[13] != str(length)
                        or native[4] != str(pack or nat) or native[5] != str(unpadded)
                        or finalized != (int(native[7]) != 2147483648) or unpadded > length):
                    raise ValueError("Aggregate layout companion contradicts its live T/U snapshots")
                properties = native_properties[identity]
                if [properties.get(key) for key in ("natural-alignment", "packing-alignment", "layout-finalized")] != values[1:3] + [values[5]]:
                    raise ValueError("Aggregate layout loses its initialized native properties")
                if bool(base) != bool(int(native[3])) or base and model.symbols[base][5] != native[3]:
                    raise ValueError("Aggregate layout selects another hidden base storage field")
                integer(str(nat), 1, 16)
                integer(str(pack), 0, 255)
                natural[native_key], packing[native_key] = nat, pack
                last_finalized[native_key] = finalized
        expected[module][identity][family] += 1
        bucket[ordinal] = tuple(values)

    if active_insert is not None or pending_layouts:
        raise ValueError("Aggregate layout observation coverage is incomplete")
    model.aggregate_layouts = {}
    model.aggregate_layout_subjects = {}
    for module, capabilities in model.capabilities.items():
        if capabilities.get(FEATURE) == "available":
            counts = [len(observations[module][family]) for family in FAMILIES] + [len(subjects[module])]
            if module not in closures or counts != closures[module] or set(subjects[module]) != set(first_seen[module]):
                raise ValueError("Aggregate layout module closure is incomplete")
            if set(observations[module]["insert"]) != set(observations[module]["insert-result"]):
                raise ValueError("Anonymous insertion result coverage is incomplete")
            created = {identity for identity, state in subjects[module].items() if state[1] == "created"}
            historical_members = set()
            for moved in observations[module]["promote"].values():
                insertion = observations[module]["insert"][int(moved[0])]
                child = int(insertion[1])
                if moved[5] == "1" and subjects[module][child][1] == "preexisting":
                    historical_members.add(int(moved[1]))
            for identity, native in native_fields[module]:
                if (int(native[11]) in created and (module, identity) not in fields
                        and identity not in historical_members):
                    raise ValueError("Created aggregate omits a native field layout observation")
            model.aggregate_layouts[module] = observations[module]
            model.aggregate_layout_subjects[module] = subjects[module]

# end of semantic_layout.py
