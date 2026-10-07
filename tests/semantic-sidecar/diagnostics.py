"""Project: FreeBASIC compiler observations
File: diagnostics.py
Purpose: Independently validate completed compiler diagnostic artifacts.
Responsibilities: Record closure, counts, sources and compiler-owned points.
This file intentionally does NOT accept recovered AST or symbol facts.
"""
from __future__ import annotations
from pathlib import Path
from sidecar import number, unescape


class Diagnostics:
    SHAPES = {"FBCDIA": 3, "SRC": 3, "M": 4, "DI": 14, "END": 7}

    def __init__(self) -> None:
        self.sources: list[str] = []
        self.modules: list[list[str]] = []
        self.records: list[list[str]] = []
        self.producer = ""
        self.succeeded = False

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
            row = [unescape(field) for field in fields]
            if index == 0:
                if row[:2] != ["FBCDIA", "1"] or row[2] != "1.20.4":
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
                integer(row[1], len(result.modules) + 1, len(result.modules) + 1)
                if len(result.modules) >= 100000 or source_path(row[2]) not in result.sources or not row[3]:
                    raise ValueError("Invalid diagnostic module")
                result.modules.append(row)
            elif row[0] == "DI":
                integer(row[1], len(result.records) + 1, len(result.records) + 1)
                integer(row[2], len(result.modules), len(result.modules))
                if not result.modules or len(result.records) >= 100000:
                    raise ValueError("Missing module or diagnostic limit exceeded")
                if row[3] not in ("error", "warning"):
                    raise ValueError("Unknown diagnostic severity")
                integer(row[4], 0, 2147483647)
                if source_path(row[6]) not in result.sources:
                    raise ValueError("Unobserved diagnostic source")
                displayed_line = integer(row[7], -1, 2147483647)
                if row[11] not in ("0", "1"):
                    raise ValueError("Invalid diagnostic point flag")
                line = integer(row[12], 0, 2147483647)
                column = integer(row[13], 0, 2147483647)
                if (row[11] == "0" and (line or column)) or (row[11] == "1" and (not line or displayed_line < 1)):
                    raise ValueError("Contradictory diagnostic point")
                if row[3] == "warning" and row[5]:
                    raise ValueError("Parser error context on warning")
                errors += row[3] == "error"
                result.records.append(row)
            elif row[0] == "END":
                expected = ["END", "1", str(len(result.modules)), str(len(result.sources)),
                            str(len(result.records)), str(errors)]
                if not result.modules or row[:6] != expected or row[6] not in ("0", "1"):
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
        return result

# end of diagnostics.py
