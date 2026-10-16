#!/usr/bin/env python3
"""
FreeBASIC classic AmigaOS toolchain support
------------------------------------------

File: amiga-build-softfloat.py

Purpose:
    Build the CPU software floating-point provider omitted by the Amiga SDK.

Responsibilities:
    - verify the GCC 6.5 assembly source and SDK conversion objects
    - repair arithmetic classification, signs and guard/sticky-bit handling
    - assemble native Hunk helpers with the port's 68020 soft-float contract
    - publish one complete archive before Amiga programs are linked

This file intentionally does NOT contain:
    - FreeBASIC arithmetic implementations or disk-based Amiga math libraries

The pinned SDK supplies arithmetic through mathieee*.library and omits those
operations from libgcc. GCC's original lb1sf68.S supplies CPU arithmetic; the
SDK's verified xfpgnulib objects supply bit-based format and integer conversion.
Both have the GCC Runtime Library Exception and are kept separate from libfb.
"""

import argparse
import hashlib
from pathlib import Path
import shutil
import subprocess
import urllib.request


SOURCE_URL = "https://raw.githubusercontent.com/gcc-mirror/gcc/releases/gcc-6.5.0/libgcc/config/m68k/lb1sf68.S"
SOURCE_SHA256 = "a6455a8b299bb903d6fc6b7654409f3fc539b2aad5ce4058e9f79b01b378e3d9"
LICENSES = {
    'COPYING3': '8ceb4b9ee5adedde47b31e975c1d90c73ad27b6b165a1dcd80c7c545eb65b903',
    'COPYING.RUNTIME': '9d6b43ce4d8de0c878bf16b54d8e7a10d9bd42b75178153e3af6a815bdc90f74',
}
PARTS = ("floatex", "double", "float", "eqdf2", "nedf2", "gtdf2", "gedf2",
         "ledf2", "ltdf2", "eqsf2", "nesf2", "gtsf2", "gesf2", "lesf2", "ltsf2")
CONVERSIONS = {
    "fixdfsi": "336ae5f2acd998a9a9169e1fd7d698f3ed23df18ed215b0cf6cc2eab1e5c59d9",
    "fixsfsi": "078bb79bf23d7e62999db1e4a6b125a316ba0f9c20f77b53df10cd512c2778aa",
    "floatsidf": "1c8a6c0a8382377df8ae01b69bde05c93f18cba4c588c2dd2d0351fd8c6ebcbe",
    "floatsisf": "a54e460d63aa3a9cfe3bf082fcbdae1336ec5338daa487d9a92835c234d781e4",
    "floatunsidf": "6fb692d744663da14fe5c5d288302a7cec5e4f048035838807b4201598cc26e2",
    "floatunsisf": "e4a9dd890abea9487a361449a9153661c62c96bb3886133add167634bdabe199",
}


def patch_source(source: str) -> str:
    # A product with two leading mantissa bits needs one right shift. GCC's
    # 68020 double helper shifts d0-d1 but drops the bit carried out of d1.
    # Propagate it through d2-d3 so the shared rounder can distinguish an
    # exact halfway value from one just above it. The input remains pinned;
    # only verified helper sequences are changed in the build copy.
    old = '''\tbtst\tIMM (DBL_MANT_DIG+1-32),d0
\tbeq\tLround$exit
#ifndef __mcoldfire__
\tlsrl\tIMM (1),d0
\troxrl\tIMM (1),d1
\taddw\tIMM (1),d4'''
    new = old.replace('\taddw\tIMM (1),d4',
                      '\troxrl\tIMM (1),d2\n\troxrl\tIMM (1),d3\n\taddw\tIMM (1),d4')
    if source.count(old) != 1:
        raise SystemExit('Unsupported GCC double multiplication normalization')
    source = source.replace(old, new, 1)
    # The single helper rotates ten high product bits into the low end of
    # d1, copies them to d0, then clears that temporary copy. Its original
    # mask leaves bit 8 set, turning some exact halfway products into an
    # apparent inexact result. Clear all ten transferred bits in both forms.
    for old, new in (('andw\tIMM (0xfd00),d1', 'andw\tIMM (0xfc00),d1'),
                     ('andl\tIMM (0xfffffd00),d1', 'andl\tIMM (0xfffffc00),d1')):
        if source.count(old) != 1:
            raise SystemExit('Unsupported GCC single multiplication normalization')
        source = source.replace(old, new, 1)
    # An infinite first operand still requires checking whether the second
    # operand is NaN, including a payload confined to its low word.
    old = '''Lmuldf$a$nf:
\tmoveq\tIMM (MULTIPLY),d5
\tmovel\ta0,d7\t\t| get sign bit back into d7
\ttstl\td1\t\t| we know d0 == 0x7ff00000, so check d1
\tbne\tLd$inop\t\t| if d1 <> 0 a is NaN
\tbra\tLd$overflow\t| else signal overflow'''
    new = old.replace('\tbra\tLd$overflow\t| else signal overflow',
                      '\tcmpl\tIMM (0x7ff00000),d2\n'
                      '\tbhi\tLd$inop\n\tbne\tLd$overflow\n'
                      '\ttstl\td3\n\tbne\tLd$inop\n\tbra\tLd$overflow')
    if source.count(old) != 1:
        raise SystemExit('Unsupported GCC double multiplication nonfinite check')
    source = source.replace(old, new, 1)
    # Underflow keeps the product/quotient sign even when every fraction bit
    # is lost. The double division infinity path must restore that sign from
    # a0 before entering the shared return routine.
    replacements = (
        ('Ld$underflow:\n| Return 0 and set the exception flags \n'
         '\tmovel\tIMM (0),d0\n\tmovel\td0,d1',
         'Ld$underflow:\n| Return a signed zero and set the exception flags\n'
         '\tmovel\td7,d0\n\tandl\tIMM (0x80000000),d0\n\tmoveq\tIMM (0),d1'),
        ('Lf$underflow:\n| Return 0 and set the exception flags \n\tmoveq\tIMM (0),d0',
         'Lf$underflow:\n| Return a signed zero and set the exception flags\n'
         '\tmovel\td7,d0\n\tandl\tIMM (0x80000000),d0'),
        ('\tbra\tLd$underflow\t| else b is +/-INFINITY, so signal underflow',
         '\tmovel\ta0,d7\n\tbra\tLd$underflow\t| b is +/-INFINITY')
    )
    for old, new in replacements:
        if source.count(old) != 1:
            raise SystemExit('Unsupported GCC signed underflow return')
        source = source.replace(old, new, 1)
    # Addition reaches infinity as soon as the exponent equals its reserved
    # all-ones value. Treating only larger exponents as overflow leaves the
    # accumulated fraction in place and turns a finite sum into NaN.
    old = '''Laddsf$4:
| Put back the exponent, but check for overflow.
#ifndef __mcoldfire__
\tcmpw\tIMM (0xff),d2
#else
\tcmpl\tIMM (0xff),d2
#endif
\tbhi\t1f'''
    if source.count(old) != 1:
        raise SystemExit('Unsupported GCC single addition overflow check')
    source = source.replace(old, old.replace('\tbhi\t1f', '\tbhs\t1f'), 1)
    # A double can have a zero high fraction word and a nonzero low word.
    # Test both before discarding a negative sign on the zero-operand path.
    old = '''\tmovel\td0,d0           \t| check for zero, since we don't  '
\tbne\tLadddf$ret\t\t| want to return -0 by mistake'''
    new = '''\tmovel\td0,d6
\torl\td1,d6\t\t| both words must be zero before clearing sign
\tbne\tLadddf$ret'''
    if source.count(old) != 1:
        raise SystemExit('Unsupported GCC double addition zero check')
    source = source.replace(old, new, 1)
    # Fraction bits in a finite dividend are not NaN payloads. Inspect its
    # low word only when the high word is exactly the infinity encoding.
    old = '''\tcmpl\tIMM (0x7ff00000),d0 | compare d0 with INFINITY
\tbhi\tLd$inop\t\t| if larger it is NaN
\ttstl\td1\t\t|'''
    new = old.replace('\ttstl\td1\t\t|', '\tbne\tLd$div$0\n\ttstl\td1\t\t|')
    if source.count(old) != 1:
        raise SystemExit('Unsupported GCC double division zero check')
    source = source.replace(old, new, 1)
    # Infinity divided by any finite nonzero value is infinity. The right
    # low word is ordinary fraction data here; restore the quotient sign
    # saved at entry before using the common infinity return.
    old = '''\tcmpl\td7,d2\t\t| compare b with INFINITY\x20
\tbge\tLd$inop\t\t| if b is NaN or INFINITY return NaN
\ttstl\td3\t\t|
\tbne\tLd$inop\t\t|\x20
\tbra\tLd$overflow\t| else return overflow'''
    new = '''\tcmpl\td7,d2\t\t| compare b with INFINITY
\tbge\tLd$inop\t\t| if b is NaN or INFINITY return NaN
\tmovel\ta0,d7
\tbra\tLd$overflow'''
    if source.count(old) != 1:
        raise SystemExit('Unsupported GCC double division infinity check')
    source = source.replace(old, new, 1)
    # The nonfinite single-addition path cleared d0's sign while classifying
    # it. Recover the original sign from the argument, not that magnitude.
    start = source.index('Laddsf$nf:')
    end = source.index('SYM (__mulsf3):', start)
    section = source[start:end]
    if section.count('\tmovel\td0,d7') != 2:
        raise SystemExit('Unsupported GCC single addition infinity sign')
    source = source[:start] + section.replace('\tmovel\td0,d7', '\tmovel\ta6@(8),d7') + source[end:]
    # An exactly exhausted remainder has no sticky bits. The original early
    # exit leaves the divisor in d1, making exact halfway subnormal quotients
    # look inexact. Normalizing a two-bit quotient must also retain its guard.
    old = '''\tbset\td3,d6\t\t| set a bit in d6
\tsubl\td1,d0\t\t| if a >= b  a <-- a-b
\tbeq\t3f\t\t| if a is zero, exit'''
    new = old.replace('\tbeq\t3f\t\t| if a is zero, exit',
                      '\tbne\t2f\n\tmoveq\tIMM (0),d1\n\tbra\t3f')
    if source.count(old) != 1:
        raise SystemExit('Unsupported GCC single division exact remainder')
    source = source.replace(old, new, 1)
    old = '''\tbtst\tIMM (FLT_MANT_DIG+1),d0\t\t
\tbeq\t1f              | if it is not set, then bit 24 is set
\tlsrl\tIMM (1),d0\t|
#ifndef __mcoldfire__
\taddw\tIMM (1),d2\t|
#else
\taddl\tIMM (1),d2\t|
#endif'''
    new = '''\tbtst\tIMM (FLT_MANT_DIG+1),d0
\tbeq\t1f
#ifndef __mcoldfire__
\tlsrl\tIMM (1),d0
\troxrl\tIMM (1),d1
\taddw\tIMM (1),d2
#else
\tlsrl\tIMM (1),d1
\tbtst\tIMM (0),d0
\tbeq\t10f
\tbset\tIMM (31),d1
10:\tlsrl\tIMM (1),d0
\taddl\tIMM (1),d2
#endif'''
    if source.count(old) != 1:
        raise SystemExit('Unsupported GCC single division normalization')
    source = source.replace(old, new, 1)
    # The double divider can place a remaining quotient bit in either d2
    # or d3. Both words belong to the rounder; clearing d3 here loses the
    # distinction between halfway and just above halfway near underflow.
    old = '''\tmovel\td6,d0
\tmovel\td7,d1
\tmovel\tIMM (0),d3

| Here we have finished the division, with the result in d0-d1-d2-d3'''
    new = old.replace('\tmovel\tIMM (0),d3\n', '')
    if source.count(old) != 1:
        raise SystemExit('Unsupported GCC double division sticky word')
    source = source.replace(old, new, 1)
    # Equal opposite operands cancel to positive zero in the default
    # nearest/even mode. Neither the left sign nor a fraction temporary
    # belongs in that result. Preserve condition codes on nonzero paths.
    for precision, target in (('sf', 'Laddsf$ret'), ('df', 'Ladddf$ret$1')):
        old = '\tbeq\t' + target + '\t| if zero just exit'
        label = 'Lsub' + precision + '$nonzero'
        new = ('\tbne\t' + label + '\n\tmoveq\tIMM (0),d7\n'
               '\tbra\t' + target + '\n' + label + ':')
        if source.count(old) != 1:
            raise SystemExit('Unsupported GCC exact cancellation return')
        source = source.replace(old, new, 1)
    return source


def build(prefix: Path, work: Path, output: Path) -> None:
    work.mkdir(parents=True, exist_ok=True)
    output.parent.mkdir(parents=True, exist_ok=True)
    source = work / "lb1sf68.S"
    if not source.exists():
        with urllib.request.urlopen(SOURCE_URL, timeout=60) as response:
            data = response.read()
        if hashlib.sha256(data).hexdigest() != SOURCE_SHA256:
            raise SystemExit("GCC m68k assembly source hash mismatch")
        source.write_bytes(data)
    if hashlib.sha256(source.read_bytes()).hexdigest() != SOURCE_SHA256:
        raise SystemExit("Cached GCC m68k assembly source hash mismatch")
    patched_source = work / "lb1sf68-patched.S"
    patched_source.write_text(
        '/* FreeBASIC Amiga provider: repaired rounding, classification and zero signs.\n'
        '   Original GCC notices and implementation follow. */\n' +
        patch_source(source.read_text()))
    for name, expected in LICENSES.items():
        license_file = work / name
        if not license_file.exists():
            url = 'https://raw.githubusercontent.com/gcc-mirror/gcc/releases/gcc-6.5.0/' + name
            with urllib.request.urlopen(url, timeout=60) as response:
                data = response.read()
            if hashlib.sha256(data).hexdigest() != expected:
                raise SystemExit('GCC license hash mismatch: ' + name)
            license_file.write_bytes(data)
        if hashlib.sha256(license_file.read_bytes()).hexdigest() != expected:
            raise SystemExit('Cached GCC license hash mismatch: ' + name)
        shutil.copy2(license_file, output.parent / ('fbsoftfloat-' + name + '.txt'))

    compiler = prefix / "bin/m68k-amigaos-gcc"
    archiver = prefix / "bin/m68k-amigaos-ar"
    sdk_archive = prefix / "lib/gcc/m68k-amigaos/6.5.0b/libm020/libgcc.a"
    objects = []
    for part in PARTS:
        destination = work / (part + ".o")
        subprocess.run([str(compiler), "-m68020", "-msoft-float",
                        "-x", "assembler-with-cpp", "-DL_" + part,
                        "-c", str(patched_source), "-o", str(destination)], check=True)
        objects.append(destination)

    for name, expected_hash in CONVERSIONS.items():
        member = "xfpgnulib__" + name + ".o"
        data = subprocess.check_output([str(archiver), "p", str(sdk_archive), member])
        if hashlib.sha256(data).hexdigest() != expected_hash:
            raise SystemExit("Unsupported SDK software conversion object: " + member)
        destination = work / member
        destination.write_bytes(data)
        objects.append(destination)

    conversion_source = Path(__file__).resolve().parent / "amiga/softfloat-convert.c"
    destination = work / "softfloat-convert.o"
    subprocess.run([str(compiler), "-m68020", "-msoft-float", "-O2",
                    "-ffreestanding", "-fno-builtin", "-Wall", "-Wextra",
                    "-c", str(conversion_source), "-o", str(destination)], check=True)
    objects.append(destination)

    temporary = output.with_suffix(".a.new")
    # ar replaces named members but retains unnamed old members. Always create
    # this temporary archive afresh so an interrupted older build cannot add
    # unrelated objects to a successful new archive.
    if temporary.exists():
        temporary.unlink()
    subprocess.run([str(archiver), "rcs", str(temporary),
                    *map(str, objects)], check=True)
    temporary.replace(output)
    print("Built Amiga CPU floating-point archive:", output)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--toolchain-root", type=Path, required=True)
    parser.add_argument("--work", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    arguments = parser.parse_args()
    build(arguments.toolchain_root.resolve(), arguments.work.resolve(), arguments.output.resolve())

# end of amiga-build-softfloat.py
