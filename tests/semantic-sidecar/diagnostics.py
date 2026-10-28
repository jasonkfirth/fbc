"""Project: FreeBASIC compiler observations
File: diagnostics.py
Purpose: Independently validate completed compiler diagnostic artifacts.
Responsibilities: Record closure, counts, sources and compiler-owned points.
This file intentionally does NOT accept recovered AST or symbol facts.
"""
from __future__ import annotations
from pathlib import Path
from sidecar import number, unescape
from diagnostic_context import FEATURE as CONTEXT_FEATURE, LEVEL_FEATURE, policy_fields, parameter_fields, validate_warning_level

# Native error 133 is printed by the limit branch after the counter advances.
# It is a separate observation, rather than another counted source error.
TOO_MANY_ERRORS = 133


class Diagnostics:
    SHAPES = {"FBCDIA": 3, "SRC": 3, "M": 4, "CAP": 4, "DI": 14, "DC": 3, "DP": 9, "ME": 4, "END": 7}

    def __init__(self) -> None:
        self.sources: list[str] = []
        self.modules: list[list[str]] = []
        self.records: list[list[str]] = []
        self.producer = ""
        self.signature_modules: set[int] = set()
        self.variable_modules: set[int] = set()
        self.unclosed_modules: set[int] = set()
        self.error_resume_modules: set[int] = set()
        self.suffix_as_modules: set[int] = set()
        self.succeeded = False
        self.contexts = {}
        self.parameters = {}
        self.reported_contexts = False
        self.warning_levels = False
        self.attempt_lifetimes = False
        self.attempts = {}
        self.current_records = []

    @classmethod
    def read(cls, path: Path, compiler_exit: int | None = None) -> Diagnostics:
        data = path.read_bytes()
        if not data.endswith(b"\n") or len(data) > 67108864:
            raise ValueError("Incomplete or oversized diagnostic artifact")
        result = cls()
        errors, closed = 0, False

        def integer(value: str, minimum: int, maximum: int) -> int:
            parsed = number(value, minimum)
            if parsed > maximum:
                raise ValueError("Diagnostic integer exceeds its limit")
            return parsed

        def source_path(value: str) -> str:
            if not value or any(character in value for character in "\0\r\n"):
                raise ValueError("Invalid diagnostic source path")
            return value

        for index, raw in enumerate(data.decode("ascii").split("\n")[:-1]):
            if raw.endswith("\r"):
                raw = raw[:-1]
            if closed or len(raw) > 1048576:
                raise ValueError("Unexpected trailing or oversized record")
            fields = raw.split("\t")
            if not fields or len(fields) != cls.SHAPES.get(fields[0]):
                raise ValueError("Unknown diagnostic record or invalid shape")
            if any(any(ord(character) < 32 or ord(character) >= 127 for character in field) for field in fields):
                raise ValueError("Unescaped diagnostic field")
            row = [unescape(field) for field in fields]
            if index == 0:
                if row[:2] != ["FBCDIA", "1"] or row[2] not in ("1.20.4", "1.20.5"):
                    raise ValueError("Unsupported diagnostic artifact or producer")
                result.producer = row[2]
                continue
            if row[0] == "SRC":
                integer(row[1], len(result.sources) + 1, len(result.sources) + 1)
                source = source_path(row[2])
                if len(result.sources) >= 5000 or source in result.sources:
                    raise ValueError("Duplicate source or source limit exceeded")
                result.sources.append(source)
            elif row[0] == "M":
                if result.attempt_lifetimes and len(result.modules) != len(result.attempts):
                    raise ValueError("New module precedes diagnostic attempt closure")
                integer(row[1], len(result.modules) + 1, len(result.modules) + 1)
                if len(result.modules) >= 100000 or source_path(row[2]) not in result.sources or not row[3]:
                    raise ValueError("Invalid diagnostic module")
                result.modules.append(row)
            elif row[0] == "CAP":
                if row[1:3] == ["0", LEVEL_FEATURE]:
                    if result.warning_levels or result.modules or result.records or row[3] != "available":
                        raise ValueError("Invalid native warning-level capability")
                    result.warning_levels = True
                    continue
                if row[1:3] == ["0", "diagnostic-attempt-lifetimes"]:
                    if result.attempt_lifetimes or result.modules or result.records or row[3] != "available":
                        raise ValueError("Invalid diagnostic attempt capability")
                    result.attempt_lifetimes = True
                    continue
                if row[1:3] == ["0", CONTEXT_FEATURE]:
                    if result.reported_contexts or result.modules or result.records or row[3] != "available":
                        raise ValueError("Invalid invocation diagnostic-context capability")
                    result.reported_contexts = True
                    continue
                module = integer(row[1], len(result.modules), len(result.modules))
                capabilities = {"procedure-signature-mismatches": result.signature_modules,
                                "variable-case-collisions": result.variable_modules,
                                "unclosed-procedures": result.unclosed_modules,
                                "error-resume-next-syntax": result.error_resume_modules,
                                "declaration-suffix-as-conflicts": result.suffix_as_modules}
                observed = capabilities.get(row[2])
                if not result.modules or observed is None or module in observed or row[3] != "available":
                    raise ValueError("Invalid or duplicate diagnostic capability")
                observed.add(module)
            elif row[0] == "DI":
                integer(row[1], len(result.records) + 1, len(result.records) + 1)
                reported_module = integer(row[2], 0 if result.reported_contexts else len(result.modules), len(result.modules))
                if (reported_module and reported_module != len(result.modules)) or (not reported_module and not result.reported_contexts) or len(result.records) >= 100000:
                    raise ValueError("Missing module or diagnostic limit exceeded")
                if result.attempt_lifetimes and reported_module in result.attempts:
                    raise ValueError("Diagnostic refers to a completed native attempt")
                if row[3] not in ("error", "warning"):
                    raise ValueError("Unknown diagnostic severity")
                integer(row[4], 0, 2147483647)
                if reported_module:
                    if source_path(row[6]) not in result.sources:
                        raise ValueError("Unobserved diagnostic source")
                elif row[5] or row[6] or row[11:] != ["0", "0", "0"]:
                    raise ValueError("Invocation diagnostic invents a source context")
                displayed_line = integer(row[7], -1, 2147483647)
                if row[11] not in ("0", "1"):
                    raise ValueError("Invalid diagnostic point flag")
                line = integer(row[12], 0, 2147483647)
                column = integer(row[13], 0, 2147483647)
                if (row[11] == "0" and (line or column)) or (row[11] == "1" and (not line or displayed_line < 1)):
                    raise ValueError("Contradictory diagnostic point")
                if row[3] == "warning" and row[5]:
                    raise ValueError("Parser error context on warning")
                if row[5] == "procedure-signature-mismatch" and len(result.modules) not in result.signature_modules:
                    raise ValueError("Signature mismatch without compiler capability")
                if row[5] == "variable-case-collision" and len(result.modules) not in result.variable_modules:
                    raise ValueError("Variable collision without compiler capability")
                if row[5] == "unclosed-procedure" and len(result.modules) not in result.unclosed_modules:
                    raise ValueError("Unclosed procedure without compiler capability")
                if row[5] == "error-resume-next-syntax" and len(result.modules) not in result.error_resume_modules:
                    raise ValueError("Error resume syntax without compiler capability")
                if row[5] == "declaration-suffix-as-conflict" and len(result.modules) not in result.suffix_as_modules:
                    raise ValueError("Suffix/AS conflict without compiler capability")
                errors += row[3] == "error"
                result.records.append(row)
            elif row[0] == "DC":
                identity = integer(row[1], 1, len(result.records))
                if not result.reported_contexts or identity in result.contexts:
                    raise ValueError("Repeated or unavailable diagnostic policy context")
                context = policy_fields(row[2], number, warning_levels=result.warning_levels)
                if context[6] == "1" and result.records[identity - 1][3] != "warning":
                    raise ValueError("Ordinary error claims warning promotion")
                validate_warning_level(context, result.records[identity - 1][3])
                result.contexts[identity] = context
            elif row[0] == "DP":
                identity = integer(row[1], 1, len(result.records))
                if not result.reported_contexts or identity not in result.contexts:
                    raise ValueError("Diagnostic parameter lacks its accepted context")
                parameters = result.parameters.setdefault(identity, [])
                parameters.append(parameter_fields(row[2:], len(parameters) + 1, number))
            elif row[0] == "ME":
                module = integer(row[1], len(result.modules), len(result.modules))
                if not result.attempt_lifetimes or not result.modules or module in result.attempts:
                    raise ValueError("Unowned or repeated native diagnostic attempt closure")
                error_count = integer(row[3], -1, 2147483647)
                if not ((row[2] in ("accepted", "restarted") and error_count == 0)
                        or (row[2] == "rejected" and error_count > 0)
                        or (row[2] == "aborted" and error_count == -1)):
                    raise ValueError("Contradictory native diagnostic attempt outcome")
                result.attempts[module] = (row[2], error_count)
            elif row[0] == "END":
                expected = ["END", "1", str(len(result.modules)), str(len(result.sources)),
                            str(len(result.records)), str(errors)]
                if (not result.modules and not result.reported_contexts) or row[:6] != expected or row[6] not in ("0", "1"):
                    raise ValueError("Invalid diagnostic completion totals")
                result.succeeded = row[6] == "1"
                if result.succeeded and errors:
                    raise ValueError("Successful compilation retains errors")
                if compiler_exit is not None and result.succeeded != (compiler_exit == 0):
                    raise ValueError("Diagnostic outcome disagrees with compiler exit")
                closed = True
            else:
                raise ValueError("Repeated diagnostic header")
        if not closed:
            raise ValueError("Missing diagnostic completion")
        if result.warning_levels and not result.reported_contexts:
            raise ValueError("Native warning levels lack their reporter context capability")
        if result.reported_contexts:
            if set(result.contexts) != set(range(1, len(result.records) + 1)):
                raise ValueError("Reported diagnostic context coverage is incomplete")
            if any(int(context[7]) != len(result.parameters.get(identity, ())) for identity, context in result.contexts.items()):
                raise ValueError("Reported diagnostic parameter coverage is incomplete")
        if result.attempt_lifetimes and set(result.attempts) != set(range(1, len(result.modules) + 1)):
            raise ValueError("Diagnostic attempt coverage is incomplete")
        if result.attempt_lifetimes and result.reported_contexts:
            counters = {module: 0 for module in result.attempts}
            for row in result.records:
                module = int(row[2])
                if not module:
                    continue
                context = result.contexts[int(row[1])]
                counters[module] += context[6] == "1"
                if int(context[3]) != counters[module]:
                    raise ValueError("Native diagnostic counter lost a reported event")
                counters[module] += row[3] == "error" and int(row[4]) != TOO_MANY_ERRORS
            if any(count != -1 and counters[module] != count
                   for module, (_, count) in result.attempts.items()):
                raise ValueError("Diagnostic attempt count disagrees with accepted reports")
        result.current_records = [row for row in result.records if row[2] == "0" or
                                  result.attempts.get(int(row[2]), ("accepted", 0))[0] in ("accepted", "rejected")]
        return result

# end of diagnostics.py
