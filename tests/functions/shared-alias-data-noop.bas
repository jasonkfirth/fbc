'' Project: FreeBASIC compiler tests
'' File: shared-alias-data-noop.bas
'' Purpose: Provide a passing test entry on non-Linux targets.
'' Responsibilities: Keep the Linux-specific C linker test scoped to Linux.
'' This file intentionally does NOT test the C variable alias.

Print "SHARED_ALIAS_DATA=SKIP"

'' end of shared-alias-data-noop.bas
