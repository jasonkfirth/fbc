<!--
Project: FreeBASIC parser verification
File: c-emitter-parser-source-locations.md
Purpose: Record the recursive source-location storage repair and qualification.
Responsibilities: Preserve source facts, recover compiler failures and bound storage.
-->

# Recursive expression source locations

The coreutils source expansion exposes four host compiler crashes while emitting
Linux C. GDB identifies recursive expression parsing at the Windows thread's
2 MiB stack boundary. The `nl` control has 1,255 native frames. A separately
linked symbol image has an exact executable-code identity with the selected
compiler. Its register and thread-stack records preserve the failure evidence.

Binary precedence functions each reserve several `LEX_LOCATION` records,
including a complete source filename, for optional semantic export. The repair
gives each active semantic invocation heap-owned location storage. Ordinary
parsing uses a shared read-only empty record. Every normal and recovery return
releases owned storage.

The lexer adds output-form current/last-location operations. These copy into
the caller's storage and preserve identifier-hint capture and empty-context
behavior. Existing value-returning lexer interfaces retain their implementations.
The binary parser uses the output forms to avoid aggregate return buffers in
its recursive frames.

The canonical compiler image is `8ee4f7d06bc01ba188345e4c13a89d57fbfcd173bc50b661466ef151160a30bf`.
Its 224-unit self-host reaches an exact emitted-C fixed point with zero GCC
warnings. Two compiler C units change. The complete GCC/Clang backend suite
passes in 979.068 seconds. Four focused semantic-export tests pass in 32.400
seconds, covering cast aliases, written macro operands, nested evaluation
contexts and deep macro evaluation.

All four original coreutils failures emit C successfully through this compiler.
The 494 preceding successful C outputs match exactly. Native object checks of
the four recovered outputs pass with both Linux tools. Their diagnostic counts
remain in the [additional project queue](c-emitter-additional-projects.md).

The permanent `c-deep-source-locations.bas` regression contains sixty-four
written nested casts and checks three runtime values. The backend runner includes
ordinary and optimized profiles. The preceding selected compiler fails the
literal-depth control at forty-eight casts. The repaired trial passes depths
48, 64 and 96. A 128-cast control still reaches the host stack limit and remains
an open structural check.

Strict source-only lint passes the binary parser, both lexer files and the new
fixture with zero findings. The installed semantic-lint consumer still has the
capability mismatch recorded in the current status. The focused producer tests
above retain their independent results.

The `parser-source-locations-selfhost-v1`, `parser-source-locations-gates-v1`,
`parser-source-locations-semantic-v1` and `coreutils-parser-c-identities-v1`
receipts dated November 14 retain these checks. The completed 703-profile
fbctests replay preserves all preceding C identities, zero GCC warnings and
771 Clang observations. Development-path selection passes twenty-two actual
executions and preserves installed release binaries and runtime archives.
The preceding `987d6d9d` image remains available for rollback. Ongoing GUI and
JRPG checks use immutable compiler paths with their original producer identity.

<!-- end of c-emitter-parser-source-locations.md -->
