#!/usr/bin/env python3
"""FreeBASIC browser corpus. Adapt L-BASIC's LLVM emitter to the Wasm host.

fb-corpus-lbasic-llvm-source.py keeps the original AST lowering and bitcode
writer. It translates the QB64
dynamic imports to Wasm C ABI calls and select the Wasm data layout. The
frontend staging driver supplies the pinned sources and generated tables.
"""

from pathlib import Path
import re


TYPES = {"%&": "UInteger", "~%&": "UInteger", "&": "Long", "~&": "ULong", "~%%": "UByte", "%%": "Byte",
         "&&": "LongInt", "~&&": "ULongInt", "$": "String", "#": "Double"}
TYPE_SUFFIX = re.compile(r"(%&|~%&|~%%|%%|~&&|&&|~&)(?![&%])")
SPECIAL_ABI = {"LLVMGetBufferSize": "UInteger", "LLVMOffsetOfElement": "ULongInt"}
WASM_LAYOUT = "e-m:e-p:32:32-p10:8:8-p20:8:8-i64:64-f128:64-n32:64-S128-ni:1:10:20"


def bindings(directory):
    # LLVM opaque handles and size_t both occupy one Wasm machine word.
    # LLVMOffsetOfElement returns i64 independently of the host word size.
    declarations = []
    constant = None
    signature = re.compile(r"(sub|function)\s+(\w+)([~%&#$!]*)"
                           r"(?:\s+alias\s+(\w+))?(?:\((.*)\))?", re.I)
    parameter = re.compile(r"(byval\s+)?(\w+)([~%&#$!]*)", re.I)
    for line in (directory / "llvm.ffi").read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if not line or line.startswith("'"):
            continue
        head, *tail = line.split(maxsplit=1)
        if head == "enum":
            constant = int(tail[0]) if tail else 0
        elif head == "endenum":
            constant = None
        elif constant is not None:
            declarations.append(f"const {head} = {constant}")
            constant += 1
        elif match := signature.fullmatch(line):
            kind, name, suffix, alias, arguments = match.groups()
            symbol = alias or name
            params = []
            for number, argument in enumerate(arguments.split(",") if arguments else [], 1):
                match = parameter.fullmatch(argument.strip())
                if not match:
                    raise ValueError(f"Invalid LLVM interface parameter: {argument}")
                byval, _, data_type = match.groups()
                value_type = "ZString Ptr" if data_type == "$" else TYPES[data_type]
                mode = "byval" if byval or data_type == "$" else "byref"
                params.append(f"{mode} arg{number} as {value_type}")
            return_type = "" if kind == "sub" else " as " + SPECIAL_ABI.get(symbol, TYPES[suffix])
            declarations.append(f'declare {kind} {name} cdecl alias "{symbol}"({", ".join(params)}){return_type}')
        elif head not in {"rootname", "lib", "endlib"}:
            raise ValueError(line)
    declarations.extend([
        'declare function create_target_data cdecl alias "LLVMCreateTargetData"(byval layout as zstring ptr) as uinteger',
        'declare sub dispose_target_data cdecl alias "LLVMDisposeTargetData"(byval layout as uinteger)',
    ])
    return "\n".join(declarations) + "\n"


def remove_proc(text, name):
    result, matches = re.subn(r"(?ims)^(?:sub|function) " + name + r"\b.*?^end (?:sub|function)\n", "", text)
    if matches != 1:
        raise ValueError(f"Expected one LLVM procedure named {name}")
    return result


def emitter(text):
    text = text.replace("    dynlib_llvm_init\n", "")
    text = text.replace("LLVMAbortProcessAction, message$)", "LLVMReturnStatusAction, message$)\n    if e then ll_error message$")
    text = re.sub(r"(?ms)^    if options.build_stages and BUILD_LINK then\n.*?^    end if\n", "    ll_emit_ir\n", text, count=1)
    for proc in ("ll_emit_obj", "ll_emit_asm", "ll_do_link", "ll_set_target"):
        text = remove_proc(text, proc)
    text = text.replace("    Error_message$ = msg$\n    error 101\n", '    fatalerror "LLVM: " + msg$\n')
    # The layout is from LLVM 14's WebAssemblyTargetMachine. IR emission
    # needs the type layout, and never instantiates a native target machine.
    return text + f'''\nsub ll_set_target
    dim as uinteger layout = create_target_data("{WASM_LAYOUT}")
    llvm_module_set_data_layout ll_cg_state.module, layout
    llvm_set_target ll_cg_state.module, "wasm32-unknown-emscripten"
    dispose_target_data layout
end sub
'''


def suffixes(text, preserved_text):
    def typed_param(match):
        return f"{match[1]}{match[3] or ''} as {TYPES[match[2]]}"

    def procedure(match):
        arguments = match[4] or ""
        arguments = re.sub(r"\b(\w+)(~%&|%&|~%%|%%|~&&|&&|~&)(\(\))?", typed_param, arguments)
        return f"{match[1]} {match[2]}{arguments}" + (" as " + TYPES[match[3]] if match[3] else "")

    text = re.sub(r"(?im)^(sub|function)\s+(\w+)(~%&|%&|~&&|&&|~&)?[ \t]*(\([^\n]*\))?\s*$", procedure, text)
    text = re.sub(r"(?i)\b_offset\((\w+)%&\(\)\)", r"cuint(@\1(lbound(\1)))", text)
    text = re.sub(r"(?im)^\s*\$checking:[^\n]*\n", "", text)
    fragments = preserved_text.split(text)
    for i in range(0, len(fragments), 2):
        fragments[i] = TYPE_SUFFIX.sub("", fragments[i])
        fragments[i] = re.sub(r"(?i)\bname\$", "symbol_name$", fragments[i])
        fragments[i] = re.sub(r"(?i)\bpointer\b", "pointer_value", fragments[i])
    return "".join(fragments)


def merge(leaf, compiler, include_pattern, preserved_text):
    data = leaf.read_text(encoding="utf-8")
    llvm_dir = compiler / "emitters/llvm"
    if leaf.name == "llvm.bm":
        data = emitter(data)
    elif leaf.name == "llvm_bindings.bm":
        data = remove_proc(data, "llvm_recv_string") + '''\nfunction llvm_recv_string$(msgp as uinteger)
    if msgp = 0 then exit function
    llvm_recv_string$ = *cptr(zstring ptr, msgp)
    LLVMDisposeMessage msgp
end function
'''
    elif leaf.name == "vars.bm" and leaf.parent == llvm_dir:
        data = re.sub(r"(?i)\bptr\b", "parameter_slot", data)
    if leaf.parent == llvm_dir:
        data = suffixes(data, preserved_text)
    for line in data.splitlines():
        include = include_pattern.fullmatch(line)
        if not include:
            yield line
        elif include[1] == "llvm_bindings.bi":
            yield bindings(llvm_dir)
        elif Path(include[1]).name not in {"spawn.bm", "tempfile.bm"}:
            child = (leaf.parent / include[1]).resolve()
            if not child.is_relative_to(compiler):
                raise ValueError(f"Compiler include escapes the staged tree: {child}")
            yield from merge(child, compiler, include_pattern, preserved_text)


# end of fb-corpus-lbasic-llvm-source.py
