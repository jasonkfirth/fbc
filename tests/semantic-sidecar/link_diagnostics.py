"""Project: FreeBASIC native link observations
File: link_diagnostics.py
Purpose: Independently validate complete native link diagnostic artifacts.
Responsibilities: Source closure, native bindings, callbacks and exit agreement.
This file intentionally does NOT resolve symbols or parse linker console output.
"""
from pathlib import Path
from sidecar import number, unescape


class LinkDiagnostics:
    SHAPES = {'FBCLNK': 3, 'M': 5, 'FILE': 6, 'SRE': 3, 'P': 12,
              'ARGS': 2, 'U': 4, 'TOTAL': 4, 'LINK': 6, 'END': 4}
    MAX_RECORDS = 100000
    MAX_SOURCES = 5000
    MAX_BYTES = 67108864

    def __init__(self):
        self.version = None
        self.modules = []
        self.sources = []
        self.procedures = {}
        self.callbacks = []
        self.arguments = None
        self.link = None
        self.succeeded = False

    @classmethod
    def read(cls, path: Path, compiler_exit: int | None = None, root: str | None = None):
        return cls.parse(path.read_bytes(), compiler_exit, root)

    @classmethod
    def parse(cls, data: bytes, compiler_exit: int | None = None, root: str | None = None):
        if len(data) > cls.MAX_BYTES or not data.endswith(b'\n'):
            raise ValueError('Incomplete or oversized link diagnostics')
        result = cls()
        source_closed, source_modules = set(), set()
        totals_seen = closed = False

        def integer(value, minimum, maximum):
            parsed = number(value, minimum)
            if parsed > maximum:
                raise ValueError('Link diagnostic integer exceeds its bound')
            return parsed

        def path_text(value):
            if not value or any(character in value for character in '\0\r\n'):
                raise ValueError('Invalid link diagnostic source path')
            return value

        def object_symbol(value):
            if not value or '\0' in value or len(value.encode('utf-8', errors='surrogateescape')) > 4096:
                raise ValueError('Invalid link diagnostic object symbol')
            return value

        lines = data.decode('ascii').split('\n')[:-1]
        for ordinal, raw in enumerate(lines):
            if raw.endswith('\r'):
                raw = raw[:-1]
            if closed or len(raw) > 1048576:
                raise ValueError('Trailing or oversized link record')
            fields = raw.split('\t')
            shape = 13 if fields[0] == 'P' and result.version == '2' else cls.SHAPES.get(fields[0])
            if len(fields) != shape:
                raise ValueError('Unknown link record or invalid shape')
            # Percent escapes carry native byte strings. Literal control bytes
            # cannot masquerade as field separators or additional records.
            if any(any(ord(character) < 32 or ord(character) >= 127 for character in field) for field in fields):
                raise ValueError('Unescaped link diagnostic field')
            row = [unescape(field) for field in fields]
            tag = row[0]
            if ordinal == 0:
                if row[0] != 'FBCLNK' or row[1] not in ('1', '2') or row[2] != '1.20.4':
                    raise ValueError('Unsupported link diagnostics header')
                result.version = row[1]
                continue
            if tag == 'M':
                if result.arguments is not None or totals_seen or len(result.modules) >= cls.MAX_RECORDS or len(source_closed) != len(result.sources):
                    raise ValueError('Module outside compilation phase')
                integer(row[1], len(result.modules) + 1, len(result.modules) + 1)
                path_text(row[2])
                if not row[3] or '\0' in row[3]:
                    raise ValueError('Missing native target')
                integer(row[4], 0, 4)
                result.modules.append(row)
            elif tag == 'FILE':
                if result.arguments is not None or totals_seen or not result.modules or len(result.sources) >= cls.MAX_SOURCES:
                    raise ValueError('Source outside compilation phase')
                integer(row[1], len(result.sources) + 1, len(result.sources) + 1)
                module = integer(row[2], len(result.modules), len(result.modules))
                filename = path_text(row[3])
                if len(row[4]) != 64 or any(character not in '0123456789abcdef' for character in row[4]):
                    raise ValueError('Invalid observed source digest')
                integer(row[5], 0, 18446744073709551615)
                source_modules.add((module, filename))
                result.sources.append(row)
            elif tag == 'SRE':
                if result.arguments is not None or totals_seen:
                    raise ValueError('Source close outside compilation phase')
                identity = integer(row[1], 1, len(result.sources))
                if identity in source_closed or row[2] != 'verified' or int(result.sources[identity - 1][2]) != len(result.modules):
                    raise ValueError('Duplicate or unverified source close')
                source_closed.add(identity)
            elif tag == 'P':
                if result.arguments is not None or totals_seen or not result.modules or len(result.procedures) >= cls.MAX_RECORDS:
                    raise ValueError('Procedure outside compilation phase')
                identity = integer(row[1], 1, cls.MAX_RECORDS)
                module = integer(row[2], len(result.modules), len(result.modules))
                if identity in result.procedures or row[3] not in ('sub', 'function', 'other') or row[4] not in ('prototype', 'definition'):
                    raise ValueError('Invalid or duplicate native procedure')
                object_symbol(row[5])
                if (module, path_text(row[8])) not in source_modules:
                    raise ValueError('Procedure lacks its native source observation: ' + repr((row, source_modules)))
                if row[6] not in ('observed', 'unobserved', 'unsupported-backend'):
                    raise ValueError('Unknown native object name state')
                if row[6] == 'observed':
                    object_symbol(row[7])
                elif row[7]:
                    raise ValueError('Unknown object name retains a value')
                if row[9] not in ('0', '1'):
                    raise ValueError('Invalid procedure point flag')
                line = integer(row[10], 0, 2147483647)
                column = integer(row[11], 0, 2147483647)
                if (row[9] == '0' and (line or column)) or (row[9] == '1' and not line):
                    raise ValueError('Contradictory procedure source point')
                if result.version == '2' and row[12] not in ('0', '1'):
                    raise ValueError('Invalid native procedure access flag')
                result.procedures[identity] = row
            elif tag == 'ARGS':
                if result.arguments is not None or totals_seen or len(source_closed) != len(result.sources):
                    raise ValueError('Invalid native link phase transition')
                if not row[1] or '\0' in row[1]:
                    raise ValueError('Missing native link arguments')
                result.arguments = row[1]
            elif tag == 'U':
                if result.arguments is None or totals_seen or len(result.callbacks) >= cls.MAX_RECORDS:
                    raise ValueError('Callback outside native link phase')
                integer(row[1], len(result.callbacks) + 1, len(result.callbacks) + 1)
                if row[2] not in ('undefined-symbol', 'missing-lib'):
                    raise ValueError('Unknown native link failure kind')
                object_symbol(row[3])
                result.callbacks.append(row)
            elif tag == 'TOTAL':
                if totals_seen or len(source_closed) != len(result.sources):
                    raise ValueError('Incomplete compilation observations')
                integer(row[1], len(result.modules), len(result.modules))
                integer(row[2], len(result.sources), len(result.sources))
                integer(row[3], len(result.procedures), len(result.procedures))
                if set(result.procedures) != set(range(1, len(result.procedures) + 1)):
                    raise ValueError('Incomplete native procedure identities')
                totals_seen = True
            elif tag == 'LINK':
                if not totals_seen or result.link is not None or row[1] not in ('0', '1') or row[2] not in ('0', '1'):
                    raise ValueError('Invalid native link completion')
                native_exit = integer(row[3], -2147483648, 2147483647)
                if row[4] not in ('available', 'unavailable'):
                    raise ValueError('Unknown native callback coverage')
                if row[1] == '0':
                    if row[2:] != ['0', '0', 'unavailable', ''] or result.arguments is not None or result.callbacks:
                        raise ValueError('Unattempted link claims observations')
                else:
                    if row[2] != '1' or result.arguments is None or not row[5] or '\0' in row[5]:
                        raise ValueError('Attempted link lacks its native outcome')
                    if result.callbacks and (row[4] != 'available' or native_exit <= 0):
                        raise ValueError('Callbacks contradict native link outcome')
                result.link = row
            elif tag == 'END':
                if result.link is None or row[1] != result.version or row[3] not in ('0', '1'):
                    raise ValueError('Missing native link completion')
                integer(row[2], len(result.callbacks), len(result.callbacks))
                result.succeeded = row[3] == '1'
                if result.succeeded and (result.callbacks or int(result.link[3]) != 0):
                    raise ValueError('Successful invocation retains native failures')
                if compiler_exit is not None and result.succeeded != (compiler_exit == 0):
                    raise ValueError('Link diagnostics disagree with compiler exit')
                if result.succeeded and any((int(module[1]), module[2]) not in source_modules for module in result.modules):
                    raise ValueError('Successful module lacks its source observation')
                closed = True
            else:
                raise ValueError('Repeated link header')
        if not closed:
            raise ValueError('Missing link diagnostic footer')
        if root is not None and root and (not result.modules or result.modules[0][2] != root):
            raise ValueError('Link diagnostics belong to another source root')
        return result

# end of link_diagnostics.py
