"""Project: FreeBASIC semantic sidecar
File: semantic_contexts.py
Purpose: Validate complete effective configuration snapshots.
Responsibilities: Required option vocabulary, occurrence keys and context ownership.
This file intentionally does NOT interpret options or supply missing defaults.
"""

# These are wire-contract keys, independent of the compiler structure layout.
# Source occurrence receipts are additional keys with native statement IDs.
REQUIRED = {
    'compiler': frozenset(('arrayboundchk', 'arraydimschk', 'asmsyntax', 'assertions', 'backend', 'cputype', 'debug', 'debuginfo', 'errlocation', 'errorcheck', 'export', 'extraerrchk', 'fbgfx', 'fbrt', 'fbsfx', 'forcelang', 'fpmode', 'fputype', 'gosubsetjmp', 'lang', 'maxerrors', 'modeview', 'msbitfields', 'multithreaded', 'nobuiltins', 'nocmdline', 'nofastcall', 'nothiscall', 'nullptrchk', 'objinfo', 'optabstract', 'optlevel', 'outtype', 'pdcheckopt', 'pic', 'pponly', 'profile', 'resumeerr', 'returninflts', 'showerror', 'showincludes', 'stacksize', 'target', 'unwindinfo', 'valistasptr', 'vectorize', 'warninglevel')),
    'language-default': frozenset(('base', 'dynamic', 'escapestr', 'explicit', 'gosub', 'parammode', 'procprofile', 'procpublic')),
    'language-policy': frozenset(('floatliteraldtype', 'int15literaldtype', 'int16literaldtype', 'int31literaldtype', 'int32literaldtype', 'int63literaldtype', 'int64literaldtype', 'integerkeyworddtype', 'opt', 'remkeyword-active')),
}
OCCURRENCES = ("option-statement-", "option-route-", "base-statement-")
EXTERN_OPTION = ("language-policy", "parser-mangling")


def validate_contexts(model, number):
    for identity, module in model.configurations.items():
        if model.expressions_only or model.bindings_only:
            raise ValueError("Reduced model invents an unavailable configuration graph")
        options = model.options.get(identity, {})
        if model.capabilities.get(module, {}).get("extern-mangling-contexts") == "available":
            if EXTERN_OPTION not in options:
                raise ValueError("Effective configuration lacks native EXTERN mangling policy")
            if options[EXTERN_OPTION] not in range(7):
                raise ValueError("Effective configuration has an unknown native mangling mode")
        if model.capabilities.get(module, {}).get("complete-effective-contexts") == "available":
            for domain, keys in REQUIRED.items():
                if not all((domain, key) in options for key in keys):
                    raise ValueError("Effective configuration snapshot is incomplete")
        for domain, key in options:
            if (domain, key) == EXTERN_OPTION:
                if options[EXTERN_OPTION] not in range(7):
                    raise ValueError("Effective configuration has an unknown native mangling mode")
                continue
            if key in REQUIRED.get(domain, ()):
                continue
            if domain != "language-default":
                raise ValueError("Unknown effective configuration option")
            prefix = next((prefix for prefix in OCCURRENCES if key.startswith(prefix)), None)
            if prefix is None:
                raise ValueError("Unknown language-default configuration option")
            spelling = key[len(prefix):]
            statement = number(spelling, 1)
            if str(statement) != spelling or statement not in model.statements or int(model.statements[statement][7]) != module:
                raise ValueError("Effective option occurrence has a missing or foreign statement")
    model.configuration_contract = REQUIRED

# end of semantic_contexts.py
