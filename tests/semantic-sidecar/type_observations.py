"""Project: FreeBASIC semantic sidecar tests
File: type_observations.py
Purpose: Preserve compiler-owned type structure and native byte observations.
Responsibilities: Independent trees, counted closure, ABI variants and emission.
This file intentionally does NOT implement a complexity score or lint predicate.
"""
from collections import Counter
from sidecar import Model


def stable_observations(test, path):
    # Use the complete consumer for stable models too. Slot assertions below
    # additionally protect the published encoding without bypassing payload,
    # ownership, source revision or capability checks.
    model = Model.read(path)
    model.validate_physical_locations()
    records, capabilities = model.records, model.capabilities
    primitives = {int(row[1]):row for row in records['Y']}
    test.assertEqual(set(primitives),set(range(29)))
    test.assertEqual(primitives[8][2],'integer')
    test.assertEqual(primitives[9][2],'uinteger')
    for identity,name,signed in ((27,'integer128','1'),(28,'uinteger128','0')):
        test.assertEqual(primitives[identity][2:5],[name,'integer','16'])
        test.assertEqual(primitives[identity][6],signed)
    return records, capabilities


def check_observations(test):
    source = test.fixture('type_observations_probe.bas')
    for backend in test.backends:
        with test.subTest(backend=backend):
            test.invoke([source],mode='off',backend=backend)
            native = test.emission_path(source.stem,backend).read_bytes()
            for abi in ('native','stable-v1'):
                with test.subTest(abi=abi):
                    _, path = test.invoke([source],backend=backend,extra=('-semantic-type-abi',abi,
                        '-semantic-extra-facts','type-shapes,native-bytes'))
                    if abi == 'native':
                        model = Model.read(path)
                        model.validate_physical_locations()
                        records, capabilities = model.records, model.capabilities
                    else:
                        records, capabilities = stable_observations(test,path)
                    test.assertEqual(test.emission_path(source.stem,backend).read_bytes(),native)
                    for feature in ('written-type-shapes','native-byte-conversion-inputs','written-type-root-origins'):
                        test.assertEqual(capabilities[1].get(feature),'available')
                    properties = records['K']
                    openings = {row[3].split(':')[1]:row[4].split('\t') for row in properties
                                if row[3].startswith('type-shape-open:')}
                    nodes = {row[3].split(':')[1]:row[4].split('\t') for row in properties
                             if row[3].startswith('type-shape-input:')}
                    # Two inline callbacks each have a parameter and result.
                    # The named alias and the two scalar declarations are leaves.
                    # The alias definition itself does not contribute a subtree.
                    test.assertEqual(len(nodes),9)
                    test.assertEqual(set(openings),set(nodes))
                    test.assertEqual(sorted(int(node[11]) for node in nodes.values()),list(range(1,10)))
                    roots = {identity:node for identity,node in nodes.items() if node[1] == '0'}
                    test.assertEqual(len(roots),5)
                    test.assertEqual(Counter(node[4] for node in roots.values()),
                                     Counter(callback=2,named=1,primitive=2))
                    for identity,node in nodes.items():
                        test.assertEqual(openings[identity],node[:4])
                        test.assertIn(node[0],roots)
                        if node[1] != '0':
                            test.assertEqual(nodes[node[1]][4],'callback')
                            test.assertEqual(node[0],nodes[node[1]][0])
                        children = [item for item in nodes.values() if item[1] == identity]
                        test.assertEqual(len(children),int(node[9]))
                    test.assertEqual([row[4] for row in properties if row[3] == 'accepted-type-shape-count:1'],['9'])
                    test.assertEqual([row[4] for row in properties if row[3] == 'accepted-type-shape-array-count:1'],['1'])
                    arrays = [row[4].split('\t') for row in properties if row[3].startswith('type-shape-array:')]
                    test.assertEqual(len(arrays),1)
                    test.assertEqual(roots[arrays[0][0]][4],'callback')
                    test.assertEqual(arrays[0][3:],['1','1'])
                    edges = [row for row in records['H'] if row[5] == 'native-byte-input']
                    test.assertEqual(len(edges),2)
                    values = {(row[2],row[3]):row[4] for row in properties if row[1] == 'expression'}
                    test.assertEqual(Counter(values[row[2],'native-byte-kind'] for row in edges),Counter(pack=1,unpack=1))
                    expressions = {row[1]:row for row in records['E']}
                    for edge in edges:
                        test.assertEqual(values[edge[2],'native-byte-width'],'4')
                        test.assertEqual(values[edge[2],'native-byte-operand'],edge[4])
                        test.assertEqual(values[edge[2],'native-byte-token'],edge[6])
                        test.assertLess(int(edge[4]),int(edge[2]))
                        if values[edge[2],'native-byte-kind'] == 'unpack':
                            test.assertEqual(expressions[edge[2]][8],'16')
                            test.assertEqual(values[edge[2],'native-byte-callee'],'0')
                    test.assertEqual([row[4] for row in properties if row[3] == 'native-byte-count:1'],['2'])
            _, path = test.invoke([source],backend=backend,mode='bindings',
                                  extra=('-semantic-extra-facts','all'))
            reduced = Model.read(path,bindings_only=True)
            for feature in ('written-type-shapes','native-byte-conversion-inputs','written-type-root-origins'):
                test.assertEqual(reduced.capabilities[1].get(feature),'unavailable')
            test.assertFalse(any(row[3].startswith(('type-shape-','accepted-type-shape-','native-byte-'))
                                 for row in reduced.records.get('K',[])))
            test.assertEqual(test.emission_path(source.stem,backend).read_bytes(),native)
            check_macro_roots(test,backend)


def check_macro_roots(test,backend):
    source = test.fixture('type_macro_roots_probe.bas')
    test.invoke([source],mode='off',backend=backend)
    ordinary = test.emission_path(source.stem,backend).read_bytes()
    for abi in ('native','stable-v1'):
        with test.subTest(root_origins=True,abi=abi):
            _, path = test.invoke([source],backend=backend,extra=('-semantic-type-abi',abi,
                        '-semantic-extra-facts','type-shapes,native-bytes'))
            if abi == 'native':
                model = Model.read(path)
                model.validate_physical_locations()
                records, capabilities = model.records, model.capabilities
            else:
                records, capabilities = stable_observations(test,path)
            test.assertEqual(capabilities[1].get('written-type-root-origins'),'available')
            test.assertEqual(test.emission_path(source.stem,backend).read_bytes(),ordinary)
            nodes = {row[3].split(':')[1]:row[4].split('\t') for row in records['K']
                     if row[3].startswith('type-shape-input:')}
            test.assertEqual(len(nodes),30)
            roots = sorted(((identity,node) for identity,node in nodes.items() if node[1] == '0'),
                           key=lambda item:int(item[1][11]))
            test.assertEqual(len(roots),4)
            test.assertEqual([node[10] for _,node in roots],['0','1','1','1'])
            for index,(identity,node) in enumerate(roots):
                role = 'type-shape-input:'+identity
                locations = [row for row in records['LOC'] if row[1] == 'source-context' and row[3] == role]
                macros = [row for row in records['MR'] if row[1] == 'symbol' and row[4] == role]
                if index == 2:
                    # The whole written root came from a macro, so an invented
                    # contiguous physical range would misrepresent the source.
                    test.assertEqual(len(macros),1)
                    test.assertFalse(locations)
                else:
                    test.assertFalse(macros)
                    test.assertEqual(len(locations),1)
                    location = locations[0]
                    expected = ((9,19),(10,18),(0,0),(12,17))[index]
                    test.assertEqual(tuple(map(int,location[5:7])),expected)
                    test.assertEqual(location[11],'mapped')
                    if index == 3:
                        # A generated result supplies the final token. The
                        # retained root range identifies its physical keyword,
                        # not a fictional span through the macro expansion.
                        test.assertEqual(source.read_bytes()[int(location[9]):int(location[10])],b'function')

# end of type_observations.py
