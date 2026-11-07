<!--
Project: FreeBASIC GObject bindings
File: c-emitter-glib-type-callbacks.md
Purpose: Record exact GObject callback signatures and macro expansion repairs.
Responsibilities: Separate native behavior, source-profile diagnostics and build inputs.
This file intentionally does NOT claim a complete new corpus census.
-->

# GObject type callbacks, November 5, 2026

The shared inc/glib-object.bi type-definition macros now register callbacks
with the exact GClassInitFunc, GInstanceInitFunc and GClassFinalizeFunc
signatures. Each receives two native arguments. Private wrappers forward the
typed class or instance pointer to the existing one-argument user routine.
Interface default initialization has the same explicit adapter. Existing
application examples, user callback declarations and native record layouts
remain unchanged. The declarations in the installed native gobject/gtype.h
confirm these callback contracts.

Two separate macro expansion defects were reproduced with the original header.
G_DEFINE_TYPE_WITH_PRIVATE and its abstract counterpart now expand the private
data block inside the type body, after parsing the enclosing arguments. The
dynamic GTypeInfo initializer retains the continuation before its closing
parenthesis. These fixes allow ordinary uses of the supplied macros to compile.

All sixteen affected original profiles across the main and experimental
compiler cuts pass aggressive, unsuppressed GCC and Clang diagnostics. Each cut
removes sixteen GCC function-pointer-cast and sixteen Clang strict-cast
observations, with no warning increase in any tested profile. A scan of all
10,899 frozen source files finds four literal consumers of the affected type
macros. Indirect aliases are not inferred by that lexical scan, and a full new
whole-body census is not claimed. The earlier table remains its dated snapshot.

The permanent tests/compiler-support/glib-type-callbacks.bas fixture runs
against native GObject, with G_DEBUG=fatal-warnings. Thirty-two C executions
cover both C producers, both tools, GNU11/GNU23, O0/O2 and checked/ordinary modes.
Two GAS64 controls pass, and four Win32 fixture objects pass both C tools.
Win32 programs were not executed. The test checks static private storage,
abstract private type registration, interface defaults, dynamic type-module
registration and instances, stable type identities and ordinary object cleanup.
It dispatches the dynamic finalizer signature explicitly because GObject may
retain the class in its cache; native cache eviction is not claimed. Registered
type modules remain available for process lifetime, as required for later reloads.
No GTK windows or image loading run.

The unchanged installed compiler also runs the fixture with the matching
audited runtime prefix. Its older installed runtime lacks long-double helpers
referenced by the current SDK. That older compiler/runtime pair is not newly
qualified and neither installed file is replaced. Native replay requires a
compiler/runtime matching the current SDK and GObject development libraries;
compile this ordinary BAS fixture with that compiler and the repository inc
directory. No experimental compiler source overlays are needed by the binding
repair. The experimental compiler's separate portable-build boundary remains open.

Strict source-only lint reports no new findings. Five pre-existing macro
parenthesis advisories match the original header. The new fixture is clean.
Installed binaries, valid examples and unrelated dirty work remain preserved.

Evidence is retained under build/c-emitter-forward-views-20261105:
gtype-checks.json, gtype-lint.json, gtype-consumers.json, the original-header
failure controls and gtype-verification.json. The E-drive scratch and compressed
database are reused. Earlier component receipts and source preimages remain
separate from the new shared-header revision.

<!-- end of c-emitter-glib-type-callbacks.md -->
