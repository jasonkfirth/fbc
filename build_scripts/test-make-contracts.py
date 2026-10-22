#!/usr/bin/env python3
"""Project: FreeBASIC build system
File: test-make-contracts.py
Purpose: Reproduce Make graph, packaging, and ownership regressions cheaply.
Responsibilities: Exercise production fragments in disposable miniature trees.
This file intentionally does NOT build or clean the working compiler or SDK.
"""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import re
import shutil
import shlex
import subprocess
import sys
import tarfile
import tempfile
import zipfile

# The parent may be gmake on hosts whose system make uses another grammar.
# One runner process selects one executable for every disposable fixture.
MAKE_COMMAND = "make"

# These fixtures build and execute host programs. Windows ARM64 ships Clang
# without GCC, so choose an available native driver independently of CI's
# exported cross-compilation commands.
FIXTURE_CC = next((name for name in ("gcc", "clang", "cc") if shutil.which(name)), "cc")
FIXTURE_CXX = next((name for name in ("g++", "clang++", "c++") if shutil.which(name)), "c++")


# -------------------------------------------------------------------------
# Disposable fixture helpers
# -------------------------------------------------------------------------

def write(path: Path, content: str = "fixture\n") -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8", newline="\n")


def include(root: Path, name: str) -> str:
    return "include " + (root / "mk" / name).as_posix().replace(" ", "\\ ") + "\n"


def make(working: Path, *arguments: str, timeout: int = 30) -> subprocess.CompletedProcess[str]:
    # Each fixture owns its target and tool selection. Parent make overrides
    # and CI's exported cross tools must not redefine that miniature build.
    environment = os.environ.copy()
    for name in ("MAKEFLAGS", "MFLAGS", "MAKEOVERRIDES", "GNUMAKEFLAGS",
                 "CC", "CXX", "AR", "AS", "LD", "RANLIB", "STRIP", "DLLTOOL", "WINDRES",
                 "HOST_CC", "TARGET", "TARGET_TRIPLET", "TARGET_OS", "TARGET_ARCH",
                 "CROSS_BUILD", "BUILD_PREFIX", "BUILD_FBC", "BOOT_FBC",
                 "BUILD_FBC_TARGET", "BUILD_FBC_BUILDPREFIX"):
        environment.pop(name, None)
    return subprocess.run([MAKE_COMMAND, "--no-print-directory", "-s", *arguments],
                          cwd=working, env=environment, capture_output=True, text=True,
                          timeout=timeout, check=False)


def require(condition: bool, message: str, result: subprocess.CompletedProcess[str] | None = None) -> None:
    if not condition:
        details = "" if result is None else "\n" + result.stdout + result.stderr
        raise AssertionError(message + details)


def archive_members(listing: str) -> list[str]:
    # Darwin ar exposes its symbol table in the member listing. Ignore only
    # those reserved names; extra object members and their order still matter.
    indexes = {"__.SYMDEF", "__.SYMDEF SORTED", "__.SYMDEF_64", "__.SYMDEF_64 SORTED"}
    return [name for name in listing.splitlines() if name not in indexes]


# -------------------------------------------------------------------------
# Compiler links, distribution staging, and installation ownership
# -------------------------------------------------------------------------

def test_relink(root: Path, working: Path, pie: bool = False) -> None:
    startup = "runtime/fbrt0pic.o" if pie else "runtime/fbrt0.o"
    runtime = "runtime/libfbpic.a" if pie else "runtime/libfb.a"
    write(working / "link.sh", "#!/bin/sh\nset -eu\n"
          "while [ \"$#\" -gt 0 ]; do\n"
          "  if [ \"$1\" = -o ]; then shift; output=$1; fi\n"
          "  shift\ndone\nprintf '%s\\n' linked >> links\ntouch \"$output\"\n")
    write(working / "GNUmakefile",
          ".DEFAULT_GOAL := compiler\n"
          "libdir := runtime\nTARGET_OS := win32\nRUN_CC := sh link.sh\n"
          "FBC_EXE := native\nFBC_JS_EXE := js\nFBC_ANDROID_EXE := android\nFBC_WII_EXE := wii\n"
          "FBC_OBJS := unit.o\nFBC_JS_OBJS := unit.o\nFBC_ANDROID_OBJS := unit.o\nFBC_WII_OBJS := unit.o\n"
          "ENABLE_PIE := " + ("YesPlease" if pie else "") + "\n"
          "RTL_LIBS := " + startup + " " + runtime + "\n"
          + include(root, "build/build-targets.mk"))
    for name in ("unit.o", startup, runtime):
        write(working / name)
    targets = ("compiler", "compiler-js", "compiler-android", "compiler-wii")
    result = make(working, *targets)
    require(result.returncode == 0, "initial compiler links failed", result)
    result = make(working, *targets)
    require(result.returncode == 0 and len((working / "links").read_text().splitlines()) == 4,
            "unchanged compilers relinked", result)
    for name, expected in ((runtime, 8), (startup, 12)):
        # Use explicit timestamps instead of sleeps; Windows Make can round
        # file times to whole seconds when comparing prerequisites.
        stamp = max((working / target).stat().st_mtime for target in ("native", "js", "android", "wii")) + 2
        os.utime(working / name, (stamp, stamp))
        result = make(working, *targets)
        require(result.returncode == 0 and len((working / "links").read_text().splitlines()) == expected,
                name + " did not relink all four compiler variants", result)
        # Restore the input time so the next assertion tests only its input.
        stamp = (working / "unit.o").stat().st_mtime - 2
        os.utime(working / name, (stamp, stamp))


def test_relink_pic(root: Path, working: Path) -> None:
    test_relink(root, working, pie=True)


def distribution(root: Path, working: Path) -> None:
    write(working / "compiler")
    write(working / "inc/top.bi")
    write(working / "inc/nested/contract.bi")
    write(working / "runtime/libfb.a")
    write(working / "GNUmakefile",
          ".DEFAULT_GOAL := dist\nrootdir := .\nTARGET_OS := linux\n"
          "prefixbindir := /usr/bin\nprefixincdir := /usr/include/freebasic\n"
          "FBINSTALL_RUNTIME_DIR := /usr/lib/freebasic/unit\n"
          "FBC_EXE := compiler\nlibdir := runtime\nEXEEXT :=\n"
          + include(root, "dist.mk") + include(root, "inst_uninst.mk")
          + "\n.PHONY: runtime-libs\nruntime-libs:\n")


def test_dist_headers(root: Path, working: Path) -> None:
    distribution(root, working)
    result = make(working, "dist")
    require(result.returncode == 0, "distribution failed", result)
    with tarfile.open(working / "freebasic-dist.tar") as archive:
        require("freebasic/usr/include/freebasic/nested/contract.bi" in archive.getnames(),
                "distribution omitted nested includes")


def test_dist_order(root: Path, working: Path) -> None:
    distribution(root, working)
    # Delaying cleanup makes the sibling-prerequisite race deterministic.
    with (working / "GNUmakefile").open("a", encoding="utf-8") as stream:
        stream.write("\ndist-clean: wait-before-clean\n.PHONY: wait-before-clean\n"
                     "wait-before-clean:\n\t@sleep 1\n")
    result = make(working, "-j4", "dist")
    require(result.returncode == 0, "parallel distribution failed", result)
    require((working / "dist/freebasic/usr/bin/fbc").exists(),
            "parallel cleanup removed the staged compiler", result)
    with tarfile.open(working / "freebasic-dist.tar") as archive:
        require("freebasic/usr/bin/fbc" in archive.getnames(), "archive preceded staging")


def test_dist_zip(root: Path, working: Path) -> None:
    distribution(root, working)
    result = make(working, "dist-zip")
    require(result.returncode == 0, "standalone dist-zip did not stage inputs", result)
    with zipfile.ZipFile(working / "freebasic-dist.zip") as archive:
        require("freebasic/usr/include/freebasic/nested/contract.bi" in archive.namelist(),
                "zip omitted nested includes")


def test_dist_runtime_payload(root: Path, working: Path) -> None:
    distribution(root, working)
    write(working / "runtime/include/provider.h")
    write(working / "runtime/.archive-members/libfb.a", "build metadata\n")
    result = make(working, "dist", "TARGET_OS=amiga")
    require(result.returncode == 0, "distribution runtime staging failed", result)
    with tarfile.open(working / "freebasic-dist.tar") as archive:
        names = archive.getnames()
        require("freebasic/usr/lib/freebasic/unit/include/provider.h" in names,
                "distribution dropped the platform runtime's public headers")
        require(not any(".archive-members" in name for name in names),
                "distribution leaked archive membership metadata")


def installation(root: Path, working: Path) -> None:
    write(working / "GNUmakefile",
          ".DEFAULT_GOAL := uninstall\nrootdir := .\nTARGET_OS := linux\n"
          "prefix := /usr\nprefixbindir := /usr/bin\nprefixincdir := /usr/include/freebasic\n"
          "FBINSTALL_RUNTIME_DIR := /usr/lib/freebasic/unit\n"
          "libdir := runtime\nlibdirname := lib\nDESTDIR := stage\n"
          + include(root, "inst_uninst.mk"))


def test_uninstall_absent(root: Path, working: Path) -> None:
    installation(root, working)
    result = make(working, "uninstall")
    require(result.returncode == 0, "uninstall without a manifest failed", result)


def test_uninstall_failure(root: Path, working: Path) -> None:
    installation(root, working)
    # rm -f must fail for a directory; no permissions or privileged account
    # assumptions are needed to exercise preservation of the retry manifest.
    (working / "stage/owned").mkdir(parents=True)
    write(working / "stage.install_manifest", "/owned\n")
    result = make(working, "uninstall")
    require(result.returncode != 0 and (working / "stage.install_manifest").exists(),
            "failed uninstall reported success or discarded its manifest", result)


def test_install_ownership(root: Path, working: Path) -> None:
    installation(root, working)
    write(working / "inc/owned.bi")
    write(working / "stage/usr/include/freebasic/foreign.bi", "user header\n")
    result = make(working, "install-includes")
    require(result.returncode == 0, "include installation failed", result)
    result = make(working, "uninstall")
    require(result.returncode == 0, "include uninstall failed", result)
    require((working / "stage/usr/include/freebasic/foreign.bi").read_text() == "user header\n",
            "uninstall removed a header that install did not copy")
    require(not (working / "stage/usr/include/freebasic/owned.bi").exists(),
            "uninstall retained its own header")


# -------------------------------------------------------------------------
# Archive publication and bootstrap producers
# -------------------------------------------------------------------------

def test_archive_failure(root: Path, working: Path) -> None:
    write(working / "ar.sh", "#!/bin/sh\nprintf '%s\\n' replacement > \"$1\"\n")
    write(working / "unit.o")
    write(working / "runtime/libfb.a", "original\n")
    stamp = (working / "unit.o").stat().st_mtime - 2
    os.utime(working / "runtime/libfb.a", (stamp, stamp))
    write(working / "GNUmakefile",
          ".DEFAULT_GOAL := runtime/libfb.a\nlibdir := runtime\n"
          "RTLIB_OBJ := unit.o\nAR := sh ar.sh\nARFLAGS :=\nRANLIB := false\n"
          + include(root, "build/archive-rules.mk"))
    result = make(working)
    require(result.returncode != 0, "archive indexing failure was ignored", result)
    require((working / "runtime/libfb.a").read_text() == "original\n",
            "failed archive replacement destroyed the previous archive")
    require(not list((working / "runtime").glob("*.rsp")), "failed archive left a response file")
    result = make(working, "AR=false", "RANLIB=true")
    require(result.returncode != 0 and (working / "runtime/libfb.a").read_text() == "original\n",
            "archiver failure destroyed the previous archive", result)


def test_archive_dryrun(root: Path, working: Path) -> None:
    write(working / "unit.o")
    write(working / "GNUmakefile",
          ".DEFAULT_GOAL := runtime/libfb.a\nlibdir := runtime\n"
          "RTLIB_OBJ := unit.o\nAR := ar\nARFLAGS := rcs\nRANLIB := ranlib\n"
          + include(root, "build/archive-rules.mk") + "\nruntime:\n\t@mkdir -p runtime\n")
    result = make(working, "-ns")
    require(result.returncode == 0 and not (working / "runtime").exists(),
            "archive dry run wrote a response file or required the output directory", result)
    result = make(working)
    require(result.returncode == 0 and (working / "runtime/libfb.a").read_bytes().startswith(b"!<arch>\n"),
            "real archive publication failed", result)
    require(not list((working / "runtime").glob("*.tmp"))
            and not list((working / "runtime").glob("*.rsp")),
            "successful archive left temporary files")


def test_archive_portable_arguments(root: Path, working: Path) -> None:
    # Model Apple's rejection of GNU response files and Windows' command
    # limit while letting the native archiver verify the resulting members.
    names = ["unit-" + str(index) + "-" + "x" * 60 + ".o" for index in range(600)]
    for name in names:
        write(working / name)
    write(working / "ar.sh", "#!/bin/sh\nset -eu\nlength=0\n"
          "for argument in \"$@\"; do\n"
          "  case \"$argument\" in @*) exit 1 ;; esac\n"
          "  length=$((length + ${#argument} + 1))\n"
          "done\n[ \"$length\" -lt 32768 ] || exit 1\n"
          "if [ -f fail-after-first ] && [ -e \"$2\" ]; then exit 1; fi\n"
          "exec ar \"$@\"\n")
    write(working / "GNUmakefile",
          ".DEFAULT_GOAL := runtime/libfb.a\nlibdir := runtime\n"
          "RTLIB_OBJ := " + " ".join(names) + "\n"
          "AR := sh ar.sh\nARFLAGS := rcs\nRANLIB := ranlib\n"
          + include(root, "build/archive-rules.mk") + "\nruntime:\n\t@mkdir -p runtime\n")
    result = make(working)
    require(result.returncode == 0, "archive exceeded portable tool arguments", result)
    result = subprocess.run(["ar", "t", "runtime/libfb.a"], cwd=working,
                            capture_output=True, text=True, timeout=10, check=False)
    require(result.returncode == 0 and archive_members(result.stdout) == names,
            "batched archive lost or reordered members", result)
    published = (working / "runtime/libfb.a").read_bytes()
    write(working / "fail-after-first")
    result = make(working, "-B")
    require(result.returncode != 0 and (working / "runtime/libfb.a").read_bytes() == published,
            "a later archiver batch failure published a partial archive", result)
    require(not list((working / "runtime").glob("*.tmp"))
            and not list((working / "runtime").glob("*.rsp")),
            "failed archive batch left temporary files")


def archive_probe(root: Path, working: Path, producer_exit: int) -> None:
    with tarfile.open(working / "prepared.tar.xz", "w:xz"):
        pass
    write(working / "producer.sh", "#!/bin/sh\ncp prepared.tar.xz FreeBASIC-probe.tar.xz\n"
          "exit " + str(producer_exit) + "\n")
    write(working / "GNUmakefile",
          ".DEFAULT_GOAL := probe\nCC := gcc\nBOOTSTRAP_ARCHIVE := FreeBASIC-probe.tar.xz\n"
          + include(root, "tests/harness.mk")
          + "\nprobe:\n\t@archive=$$( $(call _mt_find_new_archives,sh producer.sh) ) || exit $$?; \\\n"
          "\tprintf '%s\\n' \"$$archive\" > receipt\n")


def test_archive_producer_failure(root: Path, working: Path) -> None:
    archive_probe(root, working, 7)
    result = make(working)
    require(result.returncode != 0 and not (working / "receipt").exists(),
            "archive helper hid its producer's failure", result)


def test_archive_replacement(root: Path, working: Path) -> None:
    archive_probe(root, working, 0)
    write(working / "FreeBASIC-probe.tar.xz", "old archive\n")
    result = make(working)
    require(result.returncode == 0 and (working / "receipt").read_text().strip() == "FreeBASIC-probe.tar.xz",
            "archive helper rejected replacement of the expected archive", result)


def test_bootstrap_no_output(root: Path, working: Path) -> None:
    write(working / "input.bas", "")
    write(working / "bootstrap/linux-x86_64/stale.o")
    write(working / "GNUmakefile",
          ".DEFAULT_GOAL := bootstrap-emit\nrootdir := .\nsrcdir := src\n"
          "FBTARGET := linux-x86_64\nTARGET_ARCH := x86_64\nAVAILABLE_FBC := true\n"
          "FBC_SRC := input.bas\n"
          + include(root, "bootstrap/bootstrap-emit.mk")
          + "\n.PHONY: bootstrap-check\nbootstrap-check:\n"
          "$(BOOTSTRAP_OUT):\n\t@mkdir -p \"$@\"\n")
    result = make(working)
    require(result.returncode != 0 and "bootstrap emission produced no sources" in result.stdout,
            "stale bootstrap objects masked a compiler that emitted nothing", result)


# -------------------------------------------------------------------------
# Documentation and target-specific Make contracts
# -------------------------------------------------------------------------

def test_doc_parallel(root: Path, working: Path) -> None:
    shutil.copyfile(root / "doc/GNUmakefile", working / "GNUmakefile")
    if (root / "doc/common.mk").exists():
        shutil.copyfile(root / "doc/common.mk", working / "common.mk")
    products = {
        "libfbdoc": "libfbdoc.a",
        "fbdoc": "fbdoc",
        "fbchkdoc": "getindex getpage putpage chkdocs replace rebuild mkprntoc mkerrlst delextra "
                    "mkimglst getimage samps pretty insert spellit spell",
        "makefbhelp": "makefbhelp",
    }
    for directory, binaries in products.items():
        write(working / directory / "GNUmakefile",
              ".PHONY: all\nall:\n\t@mkdir active\n\t@sleep 0.2\n"
              "\t@printf '%s\\n' " + directory + " >> ../calls\n"
              "\t@touch " + binaries + "\n\t@rmdir active\n")
    result = make(working, "-j4", "all", "EXEEXT=")
    require(result.returncode == 0, "documentation launched duplicate concurrent sub-makes", result)
    calls = (working / "calls").read_text().splitlines()
    require(sorted(calls) == sorted(products), "documentation invoked a sub-make more than once", result)


def test_archive_cleanup_scope(root: Path, working: Path) -> None:
    archive_probe(root, working, 0)
    foreign = working / "FreeBASIC-unrelated-source-bootstrap-host.tar.xz"
    write(foreign, "retained release\n")
    result = make(working)
    require(result.returncode == 0, "archive production failed", result)
    result = make(working, "clean-maketests-success")
    require(result.returncode == 0 and foreign.read_text() == "retained release\n"
            and not (working / "FreeBASIC-probe.tar.xz").exists(),
            "test cleanup crossed archive ownership", result)


def manual_fixture(root: Path, working: Path) -> Path:
    manual = working / "doc/manual"
    manual.mkdir(parents=True)
    shutil.copyfile(root / "doc/manual/GNUmakefile", manual / "GNUmakefile")
    shutil.copyfile(root / "doc/common.mk", working / "doc/common.mk")
    write(working / "mk/version.mk", "FBVERSION := test\n")
    return manual


def test_manual_parallel(root: Path, working: Path) -> None:
    manual = manual_fixture(root, working)
    write(manual / "html.sh", "#!/bin/sh\nset -e\nmkdir active\nsleep 0.2\n"
          "mkdir -p html\nprintf '%s\\n' generated >> calls\n"
          "touch html/fbdoc.hhp html/DocToc.html\nrmdir active\n")
    result = make(manual, "-j2", "html/fbdoc.hhp", "html/DocToc.html", "FBDOC=sh html.sh")
    require(result.returncode == 0 and (manual / "calls").read_text().splitlines() == ["generated"],
            "manual HTML outputs launched competing generators", result)


def test_manual_chm_failure(root: Path, working: Path) -> None:
    manual = manual_fixture(root, working)
    write(manual / "html/fbdoc.hhp")
    write(manual / "html/fbdoc.chm", "stale manual\n")
    stamp = (manual / "html/fbdoc.hhp").stat().st_mtime - 2
    os.utime(manual / "html/fbdoc.chm", (stamp, stamp))
    result = make(manual, "html/fbdoc.chm", "HHC=false")
    require(result.returncode != 0 and not (manual / "html/fbdoc.chm").exists(),
            "failed help compilation reused a stale manual", result)
    write(manual / "hhc.sh", "#!/bin/sh\nprintf '%s\\n' fresh > html/fbdoc.chm\nexit 1\n")
    result = make(manual, "html/fbdoc.chm", "HHC=sh hhc.sh")
    require(result.returncode == 0 and (manual / "html/fbdoc.chm").read_text() == "fresh\n",
            "fresh HHC output was rejected for its nonzero status", result)


def test_dos_timer_profile(root: Path, working: Path) -> None:
    directories = ("fbcobjdir", "fbcjsobjdir", "fbcandroidobjdir", "fbcwiiobjdir",
                   "libfbobjdir", "libfbpicobjdir", "libfbmtobjdir", "libfbmtpicobjdir",
                   "libfbrtobjdir", "libfbrtpicobjdir", "libfbrtmtobjdir", "libfbrtmtpicobjdir",
                   "libfbgfxobjdir", "libfbgfxpicobjdir", "libfbgfxmtobjdir", "libfbgfxmtpicobjdir",
                   "libfbgfx3objdir", "libfbgfx3picobjdir", "libfbgfx3mtobjdir", "libfbgfx3mtpicobjdir",
                   "libsfxobjdir", "libsfxpicobjdir", "libsfxmtobjdir", "libsfxmtpicobjdir")
    assignments = "".join(name + " := obj/" + name + "\n" for name in directories)
    target = "obj/libfbgfxobjdir/drivers/dos/gfx_dos.o"
    write(working / "source/gfxlib2/drivers/dos/gfx_dos.c", "")
    stamp = (working / "source/gfxlib2/drivers/dos/gfx_dos.c").stat().st_mtime - 10
    os.utime(working / "source/gfxlib2/drivers/dos/gfx_dos.c", (stamp, stamp))
    write(working / "cc.sh", "#!/bin/sh\nset -e\n"
          "while [ \"$#\" -gt 0 ]; do\n"
          "  if [ \"$1\" = -o ]; then shift; output=$1; fi\n"
          "  shift\ndone\nprintf '%s\\n' compiled >> calls\ntouch \"$output\"\n")
    write(working / "GNUmakefile", ".DEFAULT_GOAL := " + target + "\n"
          "TARGET_OS := dos\nTARGET_ARCH := x86\nrootdir := .\nsrcdir := source\nCC := sh cc.sh\n"
          + assignments + include(root, "build/compile-rules.mk")
          + "\n$(libfbgfxobjdir):\n\t@mkdir -p \"$@\"\n")
    result = make(working)
    require(result.returncode == 0, "initial DOS graphics fixture failed", result)
    # Make's filesystem time granularity can hide a newly rewritten stamp.
    # Make the object unambiguously older before toggling the profile.
    stamp = (working / target).stat().st_mtime - 2
    os.utime(working / target, (stamp, stamp))
    result = make(working, "ENABLE_DOS_GFX_LOW_POWER=YesPlease")
    require(result.returncode == 0 and (working / "calls").read_text().splitlines() == ["compiled", "compiled"],
            "DOS timer-profile change did not rebuild its relocated object", result)


def test_dos_thread_selection(root: Path, working: Path) -> None:
    for name in ("unit-tests.mk", "common.mk"):
        shutil.copyfile(root / "tests" / name, working / name)
    write(working / "dirlist.mk", "DIRLIST_FB := boolean threads\n")
    write(working / "unit-tests.inc", "SRCLIST += ./boolean/check.bas\n"
          "SRCLIST += ./threads/provider.bas\n")
    for provider, expected in (("pdmlwp", True), ("", False)):
        manifest = working / "unit-tests-obj.lst"
        manifest.unlink(missing_ok=True)
        result = make(working, "-f", "unit-tests.mk", "unit-tests-obj.lst",
                      "TARGET_OS=dos", "DOS_THREAD_PROVIDER=" + provider)
        require(result.returncode == 0, "DOS unit selection failed", result)
        present = "./threads/provider.o" in manifest.read_text().splitlines()
        require(present == expected, "DOS thread objects disagree with the selected scheduler", result)


def test_sanity_failure(root: Path, working: Path) -> None:
    for name in ("GNUmakefile", "common.mk"):
        shutil.copyfile(root / "tests" / name, working / name)
    result = make(working, "check", "FBC=false")
    require(result.returncode != 0, "compiler sanity check ignored a missing compiler", result)


def test_auxiliary_configuration(root: Path, working: Path) -> None:
    write(working / "GNUmakefile", include(root, "../doc/common.mk")
          + "\nprobe:\n\t@printf '%s\\n' 'suffix=$(EXEEXT)'\n")
    for host, expected in (("Linux", ""), ("Darwin", ""), ("FreeBSD", ""), ("Haiku", ""),
                           ("MINGW64_NT", ".exe"), ("MSYS_NT", ".exe"), ("CYGWIN_NT", ".exe")):
        result = make(working, "probe", "UNAME_S=" + host, "OS=")
        require(result.returncode == 0 and result.stdout.strip() == "suffix=" + expected,
                "documentation host suffix mismatch for " + host, result)
    result = make(root / "contrib/nsis-installer", "-n", "clean")
    require(result.returncode == 0, "NSIS installer could not read the release version", result)
    write(working / "clean", "sentinel\n")
    result = make(working, "-n", "-f", (root / "src/rtlib/drivers/dos/libfbexp.mk").as_posix(), "clean")
    require(result.returncode == 0 and "maksymbr.exe" in result.stdout,
            "DOS clean alias was hidden by a file with the same name", result)
    manual = manual_fixture(root, working / "manual-default")
    with (manual / "GNUmakefile").open("a", encoding="utf-8") as stream:
        stream.write("\nprobe-default:\n\t@printf '%s\\n' '$(.DEFAULT_GOAL)'\n")
    result = make(manual, "probe-default")
    require(result.returncode == 0 and result.stdout.strip() == "all",
            "manual prerequisites displaced its default all target", result)
    imports = working / "import-libraries"
    imports.mkdir()
    shutil.copyfile(root / "lib/win32/GNUmakefile", imports / "GNUmakefile")
    write(imports / "sample.def", "LIBRARY sample\n")
    write(imports / "clean", "sentinel\n")
    result = make(imports, "-n", "clean")
    require(result.returncode == 0 and "libsample.dll.a" in result.stdout,
            "import-library cleanup was hidden by a same-named file", result)


def test_cppflags_forwarding(root: Path, working: Path) -> None:
    directories = ["fbcobjdir", "fbcjsobjdir", "fbcandroidobjdir", "fbcwiiobjdir"]
    directories += ["lib" + family + variant + "objdir"
                    for family in ("fb", "fbrt", "fbgfx", "fbgfx3", "sfx")
                    for variant in ("", "pic", "mt", "mtpic")]
    assignments = "".join(name + " := obj/" + name + "\n" for name in directories)
    write(working / "cc.sh", "#!/bin/sh\nset -e\nprintf '%s\\n' \"$*\" >> calls\n"
          "while [ \"$#\" -gt 0 ]; do\n"
          "  if [ \"$1\" = -o ]; then shift; output=$1; fi\n"
          "  shift\ndone\ntouch \"$output\"\n")
    targets = []
    for library, family in (("gfxlib2", "fbgfx"), ("sfxlib", "sfx")):
        write(working / "source" / library / "fixture.cpp", "")
        for variant in ("", "pic", "mt", "mtpic"):
            targets.append("obj/lib" + family + variant + "objdir/fixture.o")
    write(working / "source/gfxlib2/assembly.s", "")
    for variant in ("", "pic", "mt", "mtpic"):
        targets.append("obj/libfbgfx" + variant + "objdir/assembly.o")
    write(working / "GNUmakefile", ".DEFAULT_GOAL := objects\nrootdir := .\nsrcdir := source\n"
          "TARGET_OS := haiku\nUSE_RUNTIME_CXX := yes\nCC := sh cc.sh\nCXX := sh cc.sh\n"
          + assignments + include(root, "build/compile-rules.mk")
          + "\n.PHONY: objects\nobjects: " + " ".join(targets) + "\n"
          + " ".join("obj/" + name for name in directories) + ":\n\t@mkdir -p \"$@\"\n")
    result = make(working, "CPPFLAGS=-DEXPECT_CPPFLAGS=1")
    require(result.returncode == 0, "preprocessor-flag fixture failed to compile", result)
    calls = (working / "calls").read_text().splitlines()
    require(len(calls) == 12 and all("-DEXPECT_CPPFLAGS=1" in call.split() for call in calls),
            "C++ or preprocessed graphics assembly lost CPPFLAGS", result)


def test_semantic_footer_boundary(root: Path, working: Path) -> None:
    source = (root / "mk/tests/compiler/smoke.mk").read_text(encoding="utf-8")
    programs = re.findall(
        r"@awk -F '\\t' -v minimum_schema=[^\n]* '\\\n(.*?)' \\\n\s*\"\$\(TEST_TMP\)/([^\"]+)\"",
        source, re.S,
    )
    selected = [body for body, name in programs if name == "semantic-recovery.tsv"]
    require(len(selected) == 1, "could not locate the production recovery-model validator")
    program = selected[0].replace("\\\n", "").replace("$$", "$")
    write(working / "validate.awk", program)
    rows = ["FBCSEM\t27\texpression", "M\tfixture", "D\tfixture",
            "E\t1\t1\tfixture.bas\t3\t0\t3\t1\t0\t0\tnone\tnone\t0\t0\t0\tdouble",
            "R\t27\t1", "RECOVERY\t27\t1\t1\t1\t0\t1\t1"]
    command = ["awk", "-F", "\t", "-v", "minimum_schema=15", "-f", "validate.awk"]
    for trailing, expected in (("", True), ("FILE\t1\t2\t3\t4\t5\t6\n", False)):
        result = subprocess.run(command, cwd=working, input="\n".join(rows) + "\n" + trailing,
                                capture_output=True, text=True, timeout=10, check=False)
        require((result.returncode == 0) == expected,
                "semantic recovery validator accepted records after its footer", result)


def test_prerequisite_order(root: Path, working: Path) -> None:
    write(working / "GNUmakefile", "include "
          + (root / "GNUmakefile").as_posix().replace(" ", "\\ ") + "\n"
          ".PHONY: delayed-guard\nmaybe-prereqs: delayed-guard\n"
          "delayed-guard:\n\t@sleep 1\n\t@touch ready\n"
          "probe.a: work.o\n\t@touch \"$@\"\n"
          "work.o:\n\t@test -f ready || { echo 'compile preceded prerequisite checks'; exit 1; }\n"
          "\t@touch \"$@\"\n")
    result = make(working, "-j2", "rtlib", "HAVE_PREREQS_MK=", "RTL_LIBS=probe.a",
                  "ALL_RUNTIME_OBJS=work.o", "ENABLE_REPRODUCIBLE=", "ENABLE_CET=",
                  "ENABLE_NO_PLT=", "ENABLE_AUTO_VAR_INIT=")
    require(result.returncode == 0 and (working / "work.o").exists(),
            "parallel object compilation preceded prerequisite checks", result)


def test_archive_membership(root: Path, working: Path) -> None:
    write(working / "first.o")
    write(working / "second.o")
    write(working / "GNUmakefile",
          ".DEFAULT_GOAL := runtime/libfb.a\nlibdir := runtime\n"
          "RTLIB_OBJ := first.o second.o\nAR := ar\nARFLAGS := rcs\nRANLIB := ranlib\n"
          + include(root, "build/archive-rules.mk") + "\nruntime:\n\t@mkdir -p runtime\n")
    result = make(working)
    require(result.returncode == 0, "initial archive failed", result)
    result = make(working, "RTLIB_OBJ=first.o", "AR=false")
    require(result.returncode != 0
            and (working / "runtime/.archive-members/libfb.a").read_text().split() == ["first.o", "second.o"],
            "failed archive committed a member list that was never published", result)
    result = make(working, "RTLIB_OBJ=first.o")
    require(result.returncode == 0, "reduced archive selection failed", result)
    result = subprocess.run(["ar", "t", "runtime/libfb.a"], cwd=working,
                            capture_output=True, text=True, timeout=10, check=False)
    require(result.returncode == 0 and archive_members(result.stdout) == ["first.o"],
            "removed source retained a stale archive member", result)
    stamp = (working / "runtime/libfb.a").stat().st_mtime_ns
    result = make(working, "RTLIB_OBJ=first.o")
    require(result.returncode == 0 and (working / "runtime/libfb.a").stat().st_mtime_ns == stamp,
            "unchanged member list rebuilt the archive", result)


def test_bootstrap_clean_alias(root: Path, working: Path) -> None:
    for suffix in ("c", "asm", "ll", "o"):
        write(working / ("bootstrap/fixture/generated." + suffix))
    write(working / "GNUmakefile", "include "
          + (root / "GNUmakefile").as_posix().replace(" ", "\\ ") + "\n")
    result = make(working, "clean-bootstrap-sources", "HAVE_PREREQS_MK=",
                  "ENABLE_REPRODUCIBLE=", "ENABLE_CET=", "ENABLE_NO_PLT=", "ENABLE_AUTO_VAR_INIT=")
    require(result.returncode == 0 and (working / "bootstrap/fixture/generated.o").exists()
            and all(not (working / ("bootstrap/fixture/generated." + suffix)).exists()
                    for suffix in ("c", "asm", "ll")),
            "packaging's bootstrap-source cleanup alias did not remove emitted sources", result)


def test_missing_cross_toolchain(root: Path, working: Path) -> None:
    # The fixture must remain a cross build when the runner itself is ARM64.
    write(working / "host.sh", "#!/bin/sh\necho x86_64-fixture-linux-gnu\n")
    write(working / "GNUmakefile",
          "rootdir := .\nHOST_CC := sh host.sh\nTARGET_OS := linux\nTARGET_ARCH := aarch64\n"
          "TARGET_TRIPLET := aarch64-fixture-linux-gnu\n"
          "TARGET_TRIPLET_LC := $(TARGET_TRIPLET)\nFBTARGET := linux-aarch64\n"
          "FBC_TARGET := linux-aarch64\nEXEEXT :=\n"
          + include(root, "compiler-config.mk") + include(root, "prereqs.mk")
          + "\nPR_COMMON_TOOLS :=\n")
    result = make(working, "prereqs-tools")
    require(result.returncode != 0 and "aarch64-fixture-linux-gnu-gcc" in result.stdout,
            "missing cross compiler silently fell back to host compilation", result)
    write(working / "cross.sh", "#!/bin/sh\nprintf '%s\\n' aarch64-fixture-linux-gnu\n")
    result = make(working, "prereqs-tools", "CC=sh cross.sh")
    require(result.returncode == 0, "explicit cross compiler override was rejected", result)
    # Source bootstrap archives are emitted by the host compiler. The target
    # C driver is installed later, inside the native package-build container.
    emission_goals = ("bootstrap-dist-target", "bootstrap-dist", "bootstrap-dist-arm",
                      "bootstrap-dist-all", "bootstrap-emit", "bootstrap-emit-matrix",
                      "bootstrap-emit-source-response")
    with (working / "GNUmakefile").open("a", encoding="utf-8") as stream:
        stream.write("\n" + " ".join(emission_goals) + ": prereqs-tools\n")
    for goal in emission_goals:
        result = make(working, goal)
        require(result.returncode == 0 and "Requested cross toolchain" not in result.stderr,
                goal + " required a target compiler for source-only emission", result)
    result = make(working, "bootstrap-dist-target", "prereqs-tools")
    require(result.returncode != 0 and "Missing requested cross compiler" in result.stdout,
            "source emission disabled SDK checks for an additional build goal", result)
    # Keep the top-level prerequisite wiring, replacing only source emission
    # and archiving so this regression never copies or builds the working tree.
    # Its policy probes need more time on native Windows than small fragments.
    write(working / "GNUmakefile", "include "
          + (root / "GNUmakefile").as_posix().replace(" ", "\\ ") + "\n"
          "bootstrap-check:\n\t@true\n"
          "bootstrap-emit:\n\t@printf source > emitted-source\n"
          "bootstrap-dist-target:\n\t@test -s emitted-source\n")
    result = make(working, "bootstrap-dist-target", "HOST_CC=sh host.sh",
                  "TARGET_TRIPLET=aarch64-fixture-linux-gnu", "FBC_TARGET=linux-aarch64",
                  "PR_COMMON_TOOLS=make tar", "ENABLE_REPRODUCIBLE=", "ENABLE_CET=",
                  "ENABLE_NO_PLT=", "ENABLE_AUTO_VAR_INIT=", timeout=120)
    require(result.returncode == 0 and (working / "emitted-source").is_file(),
            "package bootstrap target still required the target SDK", result)


def test_multilib_identity(root: Path, working: Path) -> None:
    write(working / "native.sh", "#!/bin/sh\necho x86_64-fixture-linux-gnu\n")
    write(working / "GNUmakefile", "rootdir := " + root.as_posix() + "\nHOST_CC := sh native.sh\nCC := gcc\n"
          + include(root, "platform.mk") + include(root, "cpu.mk")
          + include(root, "compiler-config.mk")
          + include(root, "naming.mk") + include(root, "layout.mk")
          + include(root, "build-layout.mk") + include(root, "multilib.mk")
          + "\nprobe:\n\t@printf '%s\\n' 'arch=$(TARGET_ARCH)' 'target=$(FBC_TARGET)' "
          "'runtime=$(libdir)' 'c=$(ALLCFLAGS)' 'cxx=$(ALLCXXFLAGS)' 'ld=$(ALLLDFLAGS)' "
          "'host_tools=$(MULTILIB_HOST_TOOLS)' 'prefix=$(BUILD_PREFIX)'\n")
    for triplet, width, arch, target in (
        ("x86_64-linux-gnu", "32", "x86", "linux-x86"),
        ("i686-linux-gnu", "64", "x86_64", "linux-x86_64"),
        ("x86_64-w64-mingw32", "32", "x86", "win32"),
    ):
        result = make(working, "probe", "TARGET_TRIPLET=" + triplet, "MULTILIB=" + width)
        rows = dict(line.split("=", 1) for line in result.stdout.splitlines() if "=" in line)
        require(result.returncode == 0 and rows.get("arch") == arch
                and rows.get("target") == target and rows.get("runtime", "").endswith("/" + target)
                and all("-m" + width in rows.get(key, "").split() for key in ("c", "cxx", "ld")),
                "multilib width disagrees with target identity or compile/link flags", result)
        if target.startswith("linux-"):
            require(rows.get("host_tools") == "yes" and rows.get("prefix") == "",
                    "same-OS multilib unnecessarily selected another compiler prefix", result)
    result = make(working, "probe", "TARGET_TRIPLET=aarch64-linux-gnu", "MULTILIB=32")
    require(result.returncode != 0, "x86 multilib width was accepted for ARM", result)


def test_manual_source_dependencies(root: Path, working: Path) -> None:
    manual = manual_fixture(root, working)
    write(manual / "cache/one.wakka")
    write(manual / "cache/removed.wakka")
    write(manual / "html.sh", "#!/bin/sh\nmkdir -p html\n"
          "printf '%s\\n' generated >> calls\ntouch html/fbdoc.hhp html/DocToc.html\n")
    result = make(manual, "html/DocToc.html", "FBDOC=sh html.sh")
    require(result.returncode == 0, "initial manual generation failed", result)
    for deleting, expected in ((False, 2), (True, 3)):
        if deleting:
            (manual / "cache/removed.wakka").unlink()
            for path in (manual / "cache").glob("*.wakka"):
                stamp = path.stat().st_mtime - 10
                os.utime(path, (stamp, stamp))
        else:
            write(manual / "cache/one.wakka", "updated page\n")
        stamp = (manual / "cache").stat().st_mtime - 2
        for name in ("html/fbdoc.hhp", "html/DocToc.html"):
            os.utime(manual / name, (stamp, stamp))
        result = make(manual, "html/DocToc.html", "FBDOC=sh html.sh")
        require(result.returncode == 0 and len((manual / "calls").read_text().splitlines()) == expected,
                "manual did not rebuild after a page edit or removal", result)


def test_manual_removed_zip_member(root: Path, working: Path) -> None:
    manual = manual_fixture(root, working)
    write(manual / "cache/one.wakka")
    write(manual / "cache/removed.wakka")
    target = "FB-manual-test-wakka.zip"
    result = make(manual, target)
    require(result.returncode == 0, "initial manual ZIP failed", result)
    (manual / "cache/removed.wakka").unlink()
    stamp = (manual / "cache").stat().st_mtime - 2
    os.utime(manual / target, (stamp, stamp))
    result = make(manual, target)
    require(result.returncode == 0, "manual ZIP refresh failed", result)
    with zipfile.ZipFile(manual / target) as archive:
        require(archive.namelist() == ["one.wakka"], "deleted page remained in manual ZIP")


def test_example_helper_headers(root: Path, working: Path) -> None:
    shutil.copyfile(root / "examples/GNUmakefile", working / "GNUmakefile")
    helper = working / "win32/COM/MoviePlayer/movctrl"
    helper.mkdir(parents=True)
    shutil.copyfile(root / "examples/win32/COM/MoviePlayer/movctrl/GNUmakefile", helper / "GNUmakefile")
    for name in ("movctrl.bas", "CMovie.bas", "contract.bi"):
        write(helper / name, "")
        stamp = (helper / name).stat().st_mtime - 10
        os.utime(helper / name, (stamp, stamp))
    write(working / "fbc.sh", "#!/bin/sh\nset -eu\n"
          "if [ \"$1\" = -print ]; then echo win32; exit 0; fi\n"
          "directory=$(dirname \"$0\")\nprintf '%s\\n' built >> \"$directory/calls\"\n"
          "while [ \"$#\" -gt 0 ]; do\n"
          "  case \"$1\" in -o|-x) shift; output=$1;; esac\n"
          "  shift\ndone\ntouch \"$output\"\n")
    arguments = ("win32/COM/MoviePlayer/libmovctrl.a",
                 "FBC=" + shlex.join(["sh", (working / "fbc.sh").as_posix()]))
    result = make(working, *arguments)
    require(result.returncode == 0 and len((working / "calls").read_text().splitlines()) == 3,
            "initial example helper build failed", result)
    result = make(working, *arguments)
    require(result.returncode == 0 and len((working / "calls").read_text().splitlines()) == 3,
            "unchanged example helper recompiled", result)
    write(helper / "contract.bi", "updated contract\n")
    stamp = (helper / "contract.bi").stat().st_mtime - 2
    for path in (helper / "obj").glob("*.o"):
        os.utime(path, (stamp, stamp))
    os.utime(helper.parent / "libmovctrl.a", (stamp - 2, stamp - 2))
    result = make(working, *arguments)
    require(result.returncode == 0 and len((working / "calls").read_text().splitlines()) == 6,
            "example parent or child ignored a helper header change", result)
    write(working / "win32/COM/MoviePlayer/test.bas", "")
    write(working / "win32/COM/MoviePlayer/test.exe")
    result = make(working, "clean", arguments[1], "EXEEXT=.exe")
    require(result.returncode == 0 and not list((helper / "obj").glob("*.o"))
            and not (working / "win32/COM/MoviePlayer/test.exe").exists(),
            "example cleanup retained helper objects or Windows executables", result)


def test_example_cleanup_arguments(root: Path, working: Path) -> None:
    shutil.copyfile(root / "examples/GNUmakefile", working / "GNUmakefile")
    # Large argument coverage needs only filenames, rather than hundreds of
    # files. The fixture stays small while exceeding Windows process limits.
    names = ["missing-owned-fixture-" + str(index) + "-" + "x" * 40 for index in range(900)]
    with (working / "GNUmakefile").open("a", encoding="utf-8") as stream:
        stream.write("\nALL_FILES := " + " ".join(names) + "\n")
    for arguments in (("-n", "clean", "FBC=false"), ("clean", "FBC=false")):
        result = make(working, *arguments)
        require(result.returncode == 0 and not (working / ".example-clean-files.lst").exists(),
                "example cleanup exceeded process arguments or left its manifest", result)


def test_cross_host_identity(root: Path, working: Path) -> None:
    write(working / "host.sh", "#!/bin/sh\necho x86_64-fixture-linux-gnu\n")
    write(working / "cross.sh", "#!/bin/sh\necho aarch64-fixture-linux-gnu\n")
    write(working / "bin/fbc", "target compiler placeholder\n")
    write(working / "GNUmakefile", "rootdir := .\nTARGET_OS := linux\n"
          "TARGET_ARCH := aarch64\nTARGET_TRIPLET := aarch64-fixture-linux-gnu\n"
          "TARGET_TRIPLET_LC := $(TARGET_TRIPLET)\nFBC_TARGET := linux-aarch64\n"
          "EXEEXT :=\n" + include(root, "compiler-config.mk")
          + "\nprobe:\n\t@printf '%s\\n' 'host=$(HOST_ARCH)' 'cross=$(CROSS_BUILD)' 'build=$(BUILD_FBC)'\n")
    result = make(working, "probe", "HOST_CC=sh host.sh", "CC=sh cross.sh", "BOOT_FBC=host-fbc")
    rows = dict(line.split("=", 1) for line in result.stdout.splitlines() if "=" in line)
    require(result.returncode == 0 and rows.get("host") == "x86_64"
            and rows.get("cross") == "yes" and rows.get("build") == "host-fbc",
            "target compiler replaced host identity or selected a target-only bootstrap compiler", result)


def test_sound_member_collision(root: Path, working: Path) -> None:
    write(working / "source/sfxlib/shared.c", "")
    write(working / "source/sfxlib/shared.cpp", "")
    write(working / "GNUmakefile", "rootdir := .\nsrcdir := source\n"
          "mkpath := " + (root / "mk").as_posix() + "\nTARGET_OS := haiku\nTARGET_ARCH := x86_64\n"
          + include(root, "source-graph.mk")
          + "\nreport:\n\t@printf '%s\\n' 'sources=$(words $(SFX_SRC))'\n")
    result = make(working, "report")
    require(result.returncode != 0 and "Duplicate sfxlib source basenames" in result.stderr,
            "sound graph accepted competing C/C++ archive members", result)
    (working / "source/sfxlib/shared.cpp").unlink()
    result = make(working, "report")
    require(result.returncode == 0 and result.stdout.strip() == "sources=1",
            "sound graph rejected an unambiguous source", result)


def test_multi_module_fixtures(root: Path, working: Path) -> None:
    fixtures = sorted((root / "tests").rglob("*.bmk"))
    require(bool(fixtures), "multi-module fixture inventory is empty")
    for fixture in fixtures:
        write(working / "GNUmakefile", "SRCDIR := " + fixture.parent.as_posix() + "/\n"
              "TARGET_OS := unix\nFBC := false\nCC := false\nCXX := false\n"
              "include " + fixture.as_posix().replace(" ", "\\ ") + "\n"
              "\nreport:\n\t@printf '%s\\n' 'main=$(MAIN)' 'sources=$(SRCS)'\n")
        result = make(working, "report")
        rows = dict(line.split("=", 1) for line in result.stdout.splitlines() if "=" in line)
        sources = rows.get("main", "").split() + rows.get("sources", "").split()
        require(result.returncode == 0 and len(rows.get("main", "").split()) == 1
                and all((fixture.parent / source).is_file() for source in sources),
                "invalid multi-module fixture: " + fixture.relative_to(root).as_posix(), result)
    print(f"MULTI_MODULE_FIXTURES_PASS files={len(fixtures)}", flush=True)


def test_doc_target_guard(root: Path, working: Path) -> None:
    shutil.copyfile(root / "doc/fbhelp/GNUmakefile", working / "GNUmakefile")
    shutil.copyfile(root / "doc/fbhelp/version.mk", working / "version.mk")
    result = make(working, "-n", "clean", "TARGET=win")
    require(result.returncode != 0, "help viewer accepted a partial target name", result)
    result = make(working, "-n", "clean", "TARGET=linux")
    require(result.returncode == 0, "help viewer rejected a supported target", result)


# -------------------------------------------------------------------------
# Regression runner
# -------------------------------------------------------------------------

def test_compiler_link_failure(root: Path, working: Path) -> None:
    test_relink(root, working)
    write(working / "fail-link.sh", "#!/bin/sh\nset -eu\n"
          "while [ \"$#\" -gt 0 ]; do\n"
          "  if [ \"$1\" = -o ]; then shift; output=$1; fi\n"
          "  shift\ndone\nprintf partial >\"$output\"\nexit 1\n")
    for target, output in (("compiler", "native"), ("compiler-js", "js"),
                           ("compiler-android", "android"), ("compiler-wii", "wii")):
        image = working / output
        image.write_bytes(b"previous working compiler\n")
        result = make(working, "-B", target, "RUN_CC=sh fail-link.sh")
        require(result.returncode != 0 and image.read_bytes() == b"previous working compiler\n",
                target + " lost its previous image after a failed link", result)
        require(not (working / (output + ".tmp")).exists(), "failed compiler link retained a temporary")


def configuration_fixture(root: Path, working: Path) -> None:
    write(working / "source/rtlib/fb.h", "")
    write(working / "source/gfxlib2/fb_gfx.h", "")
    write(working / "source/rtlib/value.c", "#ifndef BASE_VALUE\n#define BASE_VALUE 0\n#endif\n"
          "#ifndef PIC_ONLY\n#define PIC_ONLY 0\n#endif\n"
          "#ifndef FIXTURE_TEXT\n#define FIXTURE_TEXT \"\"\n#endif\n"
          "int fixture_value(void) { return BASE_VALUE + PIC_ONLY; }\n"
          "int fixture_text_length(void) { return sizeof(FIXTURE_TEXT) - 1; }\n")
    write(working / "source/gfxlib2/value.cpp", "#ifndef BASE_VALUE\n#define BASE_VALUE 0\n#endif\n"
          "#ifndef CXX_ONLY\n#define CXX_ONLY 0\n#endif\n"
          "extern \"C\" int fixture_cpp_value(void) { return BASE_VALUE + CXX_ONLY; }\n")
    for name, tool in (("cc", FIXTURE_CC), ("cxx", FIXTURE_CXX)):
        write(working / (name + ".sh"), "#!/bin/sh\nset -eu\n"
              "printf '%s\\n' \"$*\" >>" + name + "-calls\nexec " + tool + " \"$@\"\n")
    write(working / "GNUmakefile", ".DEFAULT_GOAL := all\nrootdir := .\nsrcdir := source\n"
          "mkpath := " + (root / "mk").as_posix() + "\n"
          "CC := sh cc.sh\nCXX := sh cxx.sh\nAR := ar\nARFLAGS := rcs\nRANLIB := ranlib\n"
          "TARGET_OS := fixture\nUSE_RUNTIME_CXX := yes\nlibdir := lib\n"
          "libfbobjdir := obj\nlibfbpicobjdir := obj/pic\nlibfbgfxobjdir := obj/gfx\n"
          "RTLIB_OBJ := obj/value.o\nRTLIB_PIC_OBJ := obj/pic/value.o\nGFX_OBJ := obj/gfx/value.o\n"
          "ALLCFLAGS := $(CFLAGS)\nALLCXXFLAGS := $(CXXFLAGS)\n"
          + include(root, "build/compile-rules.mk")
          + include(root, "build/archive-rules.mk")
          + "\nlib obj obj/pic obj/gfx:\n\t@mkdir -p $@\n"
          "all: lib/libfb.a lib/libfbpic.a lib/libfbgfx.a\n"
          + include(root, "build/config-rules.mk"))


def configuration_value(working: Path, archive: str, symbol: str, expected: int) -> None:
    write(working / "read-value.c", "int " + symbol + "(void);\n"
          "int main(void) { return " + symbol + "(); }\n")
    executable = working / ("read-value.exe" if os.name == "nt" else "read-value")
    result = subprocess.run([FIXTURE_CC, "read-value.c", archive, "-o", str(executable)],
                            cwd=working, capture_output=True, text=True, timeout=30, check=False)
    require(result.returncode == 0, "could not link the configured archive", result)
    result = subprocess.run([str(executable)], cwd=working, capture_output=True,
                            text=True, timeout=30, check=False)
    require(result.returncode == expected, "archive contains code from another configuration", result)


def test_build_configuration(root: Path, working: Path) -> None:
    configuration_fixture(root, working)
    flags = ["CPPFLAGS=-DBASE_VALUE=1 -DFIXTURE_TEXT='\"a  b\"'", "PIC_CFLAGS=-DPIC_ONLY=1"]
    result = make(working, *flags)
    require(result.returncode == 0, "initial configured archive build failed", result)
    configuration_value(working, "lib/libfb.a", "fixture_value", 1)
    configuration_value(working, "lib/libfbpic.a", "fixture_value", 2)
    configuration_value(working, "lib/libfb.a", "fixture_text_length", 4)
    # Put all old outputs at one future tick. Configuration changes must force
    # the object and its archive without depending on timestamp granularity.
    future = (working / "source/rtlib/value.c").stat().st_mtime + 120
    for path in list((working / "obj").rglob("*.o")) + list((working / "lib").glob("*.a")):
        os.utime(path, (future, future))
    flags[0] = "CPPFLAGS=-DBASE_VALUE=7 -DFIXTURE_TEXT='\"a  b\"'"
    result = make(working, *flags)
    require(result.returncode == 0, "changed preprocessor flags did not rebuild", result)
    configuration_value(working, "lib/libfb.a", "fixture_value", 7)
    configuration_value(working, "lib/libfbpic.a", "fixture_value", 8)
    flags[0] = "CPPFLAGS=-DBASE_VALUE=7 -DFIXTURE_TEXT='\"a b\"'"
    result = make(working, *flags)
    require(result.returncode == 0, "quoted flag whitespace could not be tracked", result)
    configuration_value(working, "lib/libfb.a", "fixture_text_length", 3)
    normal = working / "lib/libfb.a"
    normal_time = normal.stat().st_mtime_ns
    calls = (working / "cc-calls").read_text().splitlines()
    flags[1] = "PIC_CFLAGS=-DPIC_ONLY=3"
    result = make(working, *flags)
    require(result.returncode == 0 and normal.stat().st_mtime_ns == normal_time
            and len((working / "cc-calls").read_text().splitlines()) == len(calls) + 1,
            "PIC configuration rebuilt an unrelated variant or retained old PIC code", result)
    configuration_value(working, "lib/libfbpic.a", "fixture_value", 10)
    calls = (working / "cc-calls").read_text()
    result = make(working, *flags, "CXXFLAGS=-DCXX_ONLY=4")
    require(result.returncode == 0 and (working / "cc-calls").read_text() == calls,
            "C++ flags rebuilt unrelated C objects", result)
    configuration_value(working, "lib/libfbgfx.a", "fixture_cpp_value", 11)
    flags.append("CXXFLAGS=-DCXX_ONLY=4")
    saved = {path: path.read_bytes() for path in (normal, working / "lib/.build-config/libfb.a")}
    result = make(working, *flags, "CPPFLAGS=-DBASE_VALUE=11 -DFIXTURE_TEXT='\"a b\"'", "RANLIB=false")
    require(result.returncode != 0 and all(path.read_bytes() == data for path, data in saved.items())
            and (working / "lib/.build-config/libfb.a.pending").is_file(),
            "failed compile configuration lost its previous archive or attempt marker", result)
    result = make(working, *flags)
    require(result.returncode == 0 and not (working / "lib/.build-config/libfb.a.pending").exists(),
            "reverted archive configuration did not clear its completed attempt", result)
    configuration_value(working, "lib/libfb.a", "fixture_value", 7)
    # Recovery legitimately publishes a new archive. Native archivers may
    # record different timestamps, so subsequent failures must preserve this
    # successful image rather than the one from before the recovery build.
    saved = {path: path.read_bytes() for path in saved}
    for _ in range(2):
        result = make(working, *flags, "ARFLAGS=rc", "RANLIB=false")
        require(result.returncode != 0 and all(path.read_bytes() == data for path, data in saved.items()),
                "failed archive configuration was committed or lost its previous image", result)
    result = make(working, *flags, "ARFLAGS=rc")
    require(result.returncode == 0, "configured archive could not recover after failure", result)
    stable = {path: path.stat().st_mtime_ns for path in working.glob("lib/*") if path.is_file()}
    calls = [(working / name).read_text() for name in ("cc-calls", "cxx-calls")]
    result = make(working, *flags, "ARFLAGS=rc")
    require(result.returncode == 0 and all(path.stat().st_mtime_ns == stamp for path, stamp in stable.items())
            and calls == [(working / name).read_text() for name in ("cc-calls", "cxx-calls")],
            "unchanged configuration rebuilt artifacts", result)
    result = make(working, "-n", *flags[1:], "CPPFLAGS=-DBASE_VALUE=9", "ARFLAGS=rc")
    require(result.returncode == 0 and all(path.stat().st_mtime_ns == stamp for path, stamp in stable.items())
            and calls == [(working / name).read_text() for name in ("cc-calls", "cxx-calls")]
            and not list(working.rglob("*.tmp")), "dry-run changed configured artifacts", result)


def test_build_rule_dependencies(root: Path, working: Path) -> None:
    configuration_fixture(root, working)
    dependency = working / ("source-dependencies-" + "x" * 96 + ".d")
    makefile = working / "GNUmakefile"
    content = makefile.read_text()
    # A populated runtime tree includes thousands of dependency files. Reuse
    # one long path to exceed shell limits without creating thousands of files.
    includes = "-include " + " ".join([dependency.as_posix()] * 1600) + "\n"
    content = content.replace(include(root, "build/config-rules.mk"),
                              includes + include(root, "build/config-rules.mk"))
    makefile.write_text(content, encoding="utf-8", newline="\n")
    result = make(working)
    require(result.returncode == 0, "build with absent dependency files failed", result)
    calls = (working / "cc-calls").read_bytes()
    for content in ("# generated dependency\n", "# updated dependency\n"):
        write(dependency, content)
        result = make(working)
        require(result.returncode == 0 and (working / "cc-calls").read_bytes() == calls,
                "generated dependencies changed build commands or exceeded shell limits", result)


def test_compiler_configuration(root: Path, working: Path) -> None:
    write(working / "unit.bas", "end 0\n")
    write(working / "basic.sh", "#!/bin/sh\nset -eu\narguments=$*\n"
          "while [ \"$#\" -gt 0 ]; do\n"
          "  if [ \"$1\" = -o ]; then shift; output=$1; fi\n"
          "  shift\ndone\nprintf '%s\\n' \"$arguments\" >\"$output\"\n"
          "printf '%s\\n' \"$arguments\" >>basic-calls\n")
    write(working / "link.sh", "#!/bin/sh\nset -eu\nprintf '%s\\n' \"$*\" >>link-calls\n"
          "while [ \"$#\" -gt 0 ]; do\n"
          "  case \"$1\" in\n"
          "    -o) shift; output=$1; printf linked >\"$output\" ;;\n"
          "    *.o) cat \"$1\" >>\"$output\" ;;\n"
          "  esac\n  shift\ndone\n[ \"${FAIL_LINK:-0}\" != 1 ]\n")
    for name in ("lib/fbrt0.o", "lib/libfb.a"):
        write(working / name)
    write(working / "GNUmakefile", ".DEFAULT_GOAL := compiler\nrootdir := .\n"
          "mkpath := " + (root / "mk").as_posix() + "\nTARGET_OS := fixture\n"
          "libdir := lib\nRTL_LIBS := lib/fbrt0.o lib/libfb.a\n"
          "FBC_EXE := native.exe\nFBC_JS_EXE := js.exe\n"
          "FBC_ANDROID_EXE := android.exe\nFBC_WII_EXE := wii.exe\n"
          "fbcobjdir := obj/native\nfbcjsobjdir := obj/js\n"
          "fbcandroidobjdir := obj/android\nfbcwiiobjdir := obj/wii\n"
          "FBC_OBJS := obj/native/unit.o\nFBC_JS_OBJS := obj/js/unit.o\n"
          "FBC_ANDROID_OBJS := obj/android/unit.o\nFBC_WII_OBJS := obj/wii/unit.o\n"
          "BUILD_FBC := sh basic.sh\nRUN_CC := FAIL_LINK=$(FAIL_LINK) sh link.sh\n"
          "ALLFBCFLAGS := $(FBFLAGS)\nALLLDFLAGS := $(LDFLAGS)\n"
          + include(root, "build/build-targets.mk")
          + "\n$(FBC_OBJS) $(FBC_JS_OBJS) $(FBC_ANDROID_OBJS) $(FBC_WII_OBJS): unit.bas\n"
          "\t@mkdir -p $(dir $@)\n\t@$(BUILD_FBC) $(ALLFBCFLAGS) -c $< -o $@\n"
          "lib/fbrt0.o lib/libfb.a:\n\t$(call publish_build_config)\n"
          + include(root, "build/config-rules.mk"))
    result = make(working, "compiler", "compiler-js", "FBFLAGS=-d BUILD_FIXTURE=1")
    require(result.returncode == 0, "initial compiler configuration failed", result)
    result = make(working, "compiler", "FBFLAGS=-d BUILD_FIXTURE=2")
    require(result.returncode == 0 and "BUILD_FIXTURE=2" in (working / "obj/native/unit.o").read_text()
            and "BUILD_FIXTURE=1" in (working / "obj/js/unit.o").read_text(),
            "native configuration committed an unbuilt compiler variant", result)
    result = make(working, "compiler-js", "FBFLAGS=-d BUILD_FIXTURE=2")
    require(result.returncode == 0 and "BUILD_FIXTURE=2" in (working / "js.exe").read_text(),
            "unbuilt compiler variant retained its old configuration", result)
    calls = (working / "basic-calls").read_text()
    result = make(working, "compiler", "FBFLAGS=-d BUILD_FIXTURE=2", "LDFLAGS=-s")
    require(result.returncode == 0 and (working / "basic-calls").read_text() == calls
            and "-s" in (working / "link-calls").read_text().splitlines()[-1],
            "link flags recompiled BASIC objects or failed to relink", result)
    image = (working / "native.exe").read_bytes()
    record = (working / "obj/native/.build-config").read_bytes()
    result = make(working, "compiler", "FBFLAGS=-d BUILD_FIXTURE=3", "LDFLAGS=-s", "FAIL_LINK=1")
    require(result.returncode != 0 and (working / "native.exe").read_bytes() == image
            and (working / "obj/native/.build-config").read_bytes() == record
            and (working / "obj/native/.build-config.pending").is_file()
            and not (working / "native.tmp.exe").exists(),
            "failed compiler link committed a configuration or destroyed its previous executable", result)
    result = make(working, "compiler", "FBFLAGS=-d BUILD_FIXTURE=2", "LDFLAGS=-s")
    require(result.returncode == 0 and "BUILD_FIXTURE=2" in (working / "native.exe").read_text()
            and "BUILD_FIXTURE=3" not in (working / "native.exe").read_text()
            and not (working / "obj/native/.build-config.pending").exists(),
            "reverting flags reused objects from the failed compiler configuration", result)
    result = make(working, "compiler", "FBFLAGS=-d BUILD_FIXTURE=3", "LDFLAGS=-s", "FAIL_LINK=1")
    require(result.returncode != 0, "second pending compiler configuration did not fail", result)
    calls = (working / "basic-calls").read_text().splitlines()
    result = make(working, "compiler", "FBFLAGS=-d BUILD_FIXTURE=3", "LDFLAGS=-s")
    require(result.returncode == 0 and len((working / "basic-calls").read_text().splitlines()) == len(calls) + 1
            and "BUILD_FIXTURE=3" in (working / "native.exe").read_text(),
            "compiler retry did not complete the failed configuration", result)
    calls = [(working / name).read_bytes() for name in ("basic-calls", "link-calls")]
    result = make(working, "compiler", "FBFLAGS=-d BUILD_FIXTURE=3", "LDFLAGS=-s")
    require(result.returncode == 0 and calls == [(working / name).read_bytes()
                                               for name in ("basic-calls", "link-calls")],
            "unchanged compiler configuration rebuilt", result)


def test_warning_normalization(root: Path, working: Path) -> None:
    # Exercise the real runner, including its reference oracle. Checking must
    # leave source/reference bytes intact, and a changed warning must fail.
    write(working / "probe.bas", "end 0\n")
    write(working / "fake-compiler.c", '#include <stdio.h>\n#include <stdlib.h>\nint main(void) {\n'
          'puts("probe.bas(7) warning 25(1): expected source warning");\n'
          'puts("include.bi(12) warning 3(2): expected include warning");\n'
          'if (getenv("WARNING_FIXTURE_CHANGED")) puts("unexpected warning");\n'
          'return 0; }\n')
    compiler = working / "fake-fbc.exe"
    result = subprocess.run([FIXTURE_CC, "fake-compiler.c", "-o", str(compiler)],
                            cwd=working, capture_output=True, text=True, timeout=30, check=False)
    require(result.returncode == 0, "warning runner fixture could not compile", result)
    environment = dict(os.environ, FBC=compiler.as_posix(), FBC_FLAGS="", TEST_FILTER="probe.bas")
    command = [sys.executable, str(root / "build_scripts/check-warning-reports.py"), "--source", str(working)]
    result = subprocess.run(command + ["--update"],
                            cwd=working, env=environment, capture_output=True, text=True, timeout=30, check=False)
    require(result.returncode == 0, "warning runner failed", result)
    for target in ("dos", "linux-x86", "linux-x86_64", "win32", "win64"):
        require((working / "r" / target / "probe.txt").read_text() ==
                "\texpected source warning\n\texpected include warning\n",
                "warning normalization retained locations or changed diagnostics for " + target, result)
    references = {path: path.read_bytes() for path in (working / "r").rglob("*.txt")}
    write(working / "probe.c", "preexisting intermediate\n")
    result = subprocess.run(command, cwd=working, env=environment,
                            capture_output=True, text=True, timeout=30, check=False)
    require(result.returncode == 0 and "WARNING_REPORTS_PASS reports=5" in result.stdout,
            "matching warning reports did not pass", result)
    environment["WARNING_FIXTURE_CHANGED"] = "1"
    result = subprocess.run(command, cwd=working, env=environment,
                            capture_output=True, text=True, timeout=30, check=False)
    require(result.returncode != 0 and "WARNING_REPORT_DIFF" in result.stdout
            and all(path.read_bytes() == data for path, data in references.items())
            and (working / "probe.c").read_text() == "preexisting intermediate\n",
            "changed warnings passed or checking modified maintained inputs", result)
    result = subprocess.run(command + ["--update"], cwd=working, env=environment,
                            capture_output=True, text=True, timeout=30, check=False)
    require(result.returncode == 0 and all("unexpected warning" in path.read_text() for path in references),
            "explicit warning update failed", result)


def test_source_artifact_guard(root: Path, working: Path) -> None:
    artifact = working / "src/gfxlib2/input/probe.c.123.dump"
    write(artifact, "generated analyzer state\n")
    command = [sys.executable, str(root / "build_scripts/check-source-artifacts.py"),
               "--root", str(working)]
    result = subprocess.run(command, capture_output=True, text=True, timeout=30, check=False)
    require(result.returncode != 0 and artifact.name in result.stdout,
            "source archive accepted a generated analysis dump", result)
    artifact.unlink()
    write(working / "src/gfxlib2/obj/target/probe.c.dump", "owned build output\n")
    result = subprocess.run(command, capture_output=True, text=True, timeout=30, check=False)
    require(result.returncode == 0 and "SOURCE_ARTIFACTS_PASS" in result.stdout,
            "source guard rejected the separate object tree", result)
    # Inject a real scandir error rather than chmod: administrators/root can
    # still traverse unreadable fixtures, making a permission test unreliable.
    write(working / "scan-error.py", "import os, runpy, sys\n"
          "original = os.scandir\n"
          "def scandir(path):\n"
          "    if str(path).endswith('input'):\n"
          "        raise PermissionError(13, 'fixture scan denied', str(path))\n"
          "    return original(path)\n"
          "os.scandir = scandir\n"
          "sys.argv = sys.argv[1:]\n"
          "runpy.run_path(sys.argv[0], run_name='__main__')\n")
    result = subprocess.run([sys.executable, str(working / "scan-error.py"), *command[1:]],
                            capture_output=True, text=True, timeout=30, check=False)
    require(result.returncode != 0 and "scan was incomplete" in result.stdout
            and "SOURCE_ARTIFACTS_PASS" not in result.stdout,
            "an incomplete directory scan reported success", result)


def test_tool_replacement(root: Path, working: Path) -> None:
    configuration_fixture(root, working)
    result = make(working)
    require(result.returncode == 0, "initial tool fixture build failed", result)
    configuration_value(working, "lib/libfb.a", "fixture_value", 0)
    tool = working / "cc.sh"
    tool.write_text(tool.read_text().replace('exec ' + FIXTURE_CC + ' "$@"',
                                            'exec ' + FIXTURE_CC + ' -DBASE_VALUE=7 "$@"'),
                    encoding="utf-8", newline="\n")
    result = make(working)
    require(result.returncode == 0, "same-path compiler replacement failed", result)
    configuration_value(working, "lib/libfb.a", "fixture_value", 7)
    before = (working / "cc-calls").read_bytes()
    result = make(working)
    require(result.returncode == 0 and (working / "cc-calls").read_bytes() == before,
            "unchanged tool fingerprint rebuilt objects", result)


def test_bootstrap_configuration(root: Path, working: Path) -> None:
    write(working / "bootstrap/native/main.c", "#ifndef VALUE\n#define VALUE 0\n#endif\nint main(void) { return VALUE; }\n")
    write(working / "GNUmakefile", ".DEFAULT_GOAL := bootstrap/fbc.exe\nrootdir := .\n"
          "mkpath := " + (root / "mk").as_posix() + "\n"
          "TARGET_OS := win32\nTARGET_ARCH := fixture\nFBTARGET := native\nEXEEXT := .exe\n"
          "CC := " + FIXTURE_CC + "\nRUN_CC := " + FIXTURE_CC + "\n"
          "ALLCFLAGS := $(CFLAGS)\nlibdir := lib\nAR := ar\nARFLAGS := rcs\n"
          + include(root, "bootstrap/bootstrap-core.mk")
          + "BOOTSTRAP_RT0 :=\nBOOTSTRAP_RTL :=\n"
          "lib/fbrt0.o lib/libfb.a:\n\t@mkdir -p lib\n\t@touch $@\n\t$(publish_build_config)\n"
          + include(root, "build/config-rules.mk"))
    result = make(working)
    require(result.returncode == 0, "initial bootstrap fixture failed", result)
    executable = working / "bootstrap/fbc.exe"
    stamp = executable.stat().st_mtime_ns
    result = make(working)
    require(result.returncode == 0 and executable.stat().st_mtime_ns == stamp,
            "unchanged bootstrap compiler relinked", result)
    result = make(working, "CFLAGS=-DVALUE=7")
    require(result.returncode == 0, "bootstrap flag change did not rebuild", result)
    require(subprocess.run([str(executable)], check=False).returncode == 7,
            "bootstrap compiler retained code from old flags")
    image = executable.read_bytes()
    record = (working / "bootstrap/obj/native/.build-config").read_bytes()
    result = make(working, "CFLAGS=-DVALUE=7", "ALLLDFLAGS=-Wl,--invalid-bootstrap-link-option")
    require(result.returncode != 0 and executable.read_bytes() == image
            and (working / "bootstrap/obj/native/.build-config").read_bytes() == record
            and not (working / "bootstrap/fbc.tmp.exe").exists(),
            "failed bootstrap publication lost the executable or committed its record", result)


def test_bootstrap_emission_configuration(root: Path, working: Path) -> None:
    write(working / "source/compiler/input.bas", "fixture\n")
    write(working / "source/compiler/nested/header.bi", "fixture\n")
    original = working / "source/compiler/input.c"
    write(original, "unrelated intermediate\n")
    write(working / "emitter.sh", "#!/bin/sh\nset -eu\n"
          "for argument do\n case $argument in @*) response=${argument#@} ;; esac\ndone\n"
          "while IFS=' ' read -r flag source; do\n"
          " source=$(printf '%s' \"$source\" | tr -d '\\r')\n"
          " test -f \"$(dirname \"$source\")/nested/header.bi\"\n"
          " printf 'emitted\\n' >\"${source%.bas}.c\"\ndone <\"$response\"\n")
    write(working / "compile-probe.sh", "#!/bin/sh\nprintf 'called\\n' >> compiler-calls\nexit 1\n")
    write(working / "GNUmakefile", ".DEFAULT_GOAL := bootstrap-emit\nrootdir := $(CURDIR)\nsrcdir := $(CURDIR)/source\n"
          "mkpath := " + (root / "mk").as_posix() + "\n"
          "FBTARGET := native\nTARGET_ARCH := fixture\nTARGET_OS := win32\nCC := gcc\nRUN_CC := sh compile-probe.sh\n"
          "AVAILABLE_FBC := sh emitter.sh\n"
          "FBC_SRC := $(srcdir)/compiler/input.bas\nFBC_BI := $(srcdir)/compiler/nested/header.bi\n"
          + include(root, "build-layout.mk") + include(root, "bootstrap/bootstrap-core.mk")
          + include(root, "bootstrap/bootstrap-emit.mk") + include(root, "build/config-rules.mk"))
    result = make(working, "-n", "BUILD_ROOT=build/emitter")
    require(result.returncode == 0 and not (working / "build/emitter").exists(),
            "bootstrap emission dry run changed its output tree", result)
    result = make(working, "BUILD_ROOT=build/emitter")
    output = working / "build/emitter/bootstrap/native"
    require(result.returncode == 0 and (output / "input.c").is_file()
            and not (output / ".emit-work").exists()
            and original.read_text() == "unrelated intermediate\n",
            "bootstrap emission touched shared intermediates or failed to collect output", result)
    record = (output / ".emit-config").read_bytes()
    result = make(working, "bootstrap-emission-ready", "BUILD_ROOT=build/emitter",
                  "BOOT_FBC=unavailable-producer", "BOOTFBCFLAGS=-changed", "CC=another-host-cc")
    require(result.returncode == 0 and (output / ".emit-config").read_bytes() == record,
            "bootstrap consumer required the producing host configuration", result)
    result = make(working, "BUILD_ROOT=build/emitter", "BOOTFBCFLAGS=-changed")
    require(result.returncode == 0 and (output / ".emit-config").read_bytes() != record,
            "explicit emission did not record changed options", result)
    result = make(working, "BUILD_ROOT=build/emitter", "BOOT_FBC=false")
    require(result.returncode != 0 and (output / ".emit-config.pending").exists(),
            "failed bootstrap emission did not retain its incomplete marker", result)
    result = make(working, "bootstrap-emission-ready", "BUILD_ROOT=build/emitter")
    require(result.returncode != 0 and "unfinished bootstrap emission" in result.stdout,
            "partial bootstrap sources were accepted", result)
    write(output / "input.c", "int main(void) { return 0; }\n")
    result = make(working, "-j2", "build/emitter/bootstrap/obj/native/input.o", "BUILD_ROOT=build/emitter")
    require(result.returncode != 0 and "unfinished bootstrap emission" in result.stdout
            and not (working / "compiler-calls").exists(),
            "object compilation started before the emission guard", result)
    result = make(working, "BUILD_ROOT=build/emitter")
    require(result.returncode == 0 and not (output / ".emit-config.pending").exists(),
            "bootstrap emission retry did not recover", result)


def test_isolated_builds(root: Path, working: Path) -> None:
    write(working / "source/rtlib/value.c", "int fixture_value(void) { return VALUE; }\n")
    write(working / "source/rtlib/fb.h", "")
    write(working / "GNUmakefile", ".DEFAULT_GOAL := all\nrootdir := $(CURDIR)\nsrcdir := $(CURDIR)/source\n"
          "mkpath := " + (root / "mk").as_posix() + "\n"
          "FBTARGET := native\nFBNAME := freebasic\nlibdirname := lib\n"
          "CC := " + FIXTURE_CC + "\nRUN_CC := " + FIXTURE_CC + "\n"
          "AR := ar\nARFLAGS := rcs\nRANLIB := ranlib\nALLCFLAGS := $(CFLAGS)\n"
          + include(root, "build-layout.mk")
          + "RTLIB_OBJ := $(libfbobjdir)/value.o\n"
          + include(root, "build/build-dirs.mk") + include(root, "build/compile-rules.mk")
          + include(root, "build/archive-rules.mk") + include(root, "build/clean-rules.mk")
          + "all: $(libdir)/libfb.a\n" + include(root, "build/config-rules.mk"))
    processes = [subprocess.Popen([MAKE_COMMAND, "-s", "BUILD_ROOT=build/" + name, "CFLAGS=-DVALUE=" + str(value)],
                                 cwd=working, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
                 for name, value in (("one", 1), ("two", 2))]
    try:
        for process in processes:
            output, error = process.communicate(timeout=30)
            require(process.returncode == 0, "isolated native build failed\n" + output + error)
    finally:
        for process in processes:
            if process.poll() is None:
                process.kill()
                process.wait()
    for name, value in (("one", 1), ("two", 2)):
        configuration_value(working, "build/" + name + "/lib/freebasic/native/libfb.a", "fixture_value", value)
    write(working / "src/rtlib/obj/unrelated", "preserve\n")
    result = make(working, "clean-all-objects", "BUILD_ROOT=build/one")
    require(result.returncode == 0 and not (working / "build/one/src/rtlib/obj").exists()
            and (working / "build/two/src/rtlib/obj/native/value.o").exists()
            and (working / "src/rtlib/obj/unrelated").is_file(),
            "isolated cleanup crossed output-tree ownership", result)
    result = make(working, "BUILD_ROOT=../unsafe")
    require(result.returncode != 0 and "BUILD_ROOT must" in result.stderr,
            "an unsafe output-root spelling was accepted", result)


def test_source_docs(root: Path, working: Path) -> None:
    import runpy
    checker = runpy.run_path(str(root / "build_scripts/check-source-docs.py"))
    page = working / "guide.md"
    write(page, "[source](source.c)\n")
    write(working / "source.c")
    require(checker["check_markdown"](page) == 1, "valid source link was rejected")
    (working / "source.c").unlink()
    try:
        checker["check_markdown"](page)
    except ValueError as error:
        require("missing link" in str(error), "wrong Markdown failure")
    else:
        raise AssertionError("broken source link was accepted")
    cache = working / "doc/manual/cache"
    rendered = working / "rendered"
    names = checker["PAGES"]
    for name in names:
        write(cache / (name + ".wakka"), '{{fbdoc item="title" value="' + name + '"}}\n')
        token = checker["TREES"].get(name, ".pending" if name == "DevBuildConfig" else "source-artifacts-test")
        html = '<html><body><pre>' + token + '</pre>' + name + ' .pending cksum<a href="DevToc.html">contents</a>'
        html += '<a href="DevSourceCode.html">guide</a></body></html>'
        write(rendered / "html" / (name + ".html"), html)
        for mode in ("fbhelp", "txt"):
            write(rendered / mode / (name + ".txt"), token + "rendered prose " * 50)
    write(rendered / "html/fbdoc.hhp", "\n".join(name + ".html" for name in names))
    for name in ("DocToc", "DevToc"):
        write(rendered / "html" / (name + ".html"), " ".join(guide + ".html" for guide in checker["GUIDES"]))
    checker["check_rendered"](working, rendered)
    (rendered / "txt/DevSourceRuntime.txt").unlink()
    try:
        checker["check_rendered"](working, rendered)
    except OSError:
        pass
    else:
        raise AssertionError("missing rendered format was accepted")


def main() -> int:
    global MAKE_COMMAND
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--case", help="Run one named regression")
    parser.add_argument("--make", default=shutil.which("gmake") or shutil.which("make") or "make",
                        help="GNU Make executable used by the parent build")
    options = parser.parse_args()
    MAKE_COMMAND = options.make
    tests = (test_relink, test_relink_pic, test_dist_headers, test_dist_order, test_dist_zip,
             test_dist_runtime_payload,
             test_uninstall_absent, test_uninstall_failure, test_install_ownership,
             test_archive_failure, test_archive_dryrun, test_archive_portable_arguments,
             test_archive_producer_failure,
             test_archive_replacement, test_bootstrap_no_output, test_doc_parallel,
             test_archive_cleanup_scope, test_manual_parallel, test_manual_chm_failure,
             test_dos_timer_profile, test_dos_thread_selection, test_sanity_failure,
             test_auxiliary_configuration, test_cppflags_forwarding, test_semantic_footer_boundary,
             test_prerequisite_order, test_archive_membership, test_bootstrap_clean_alias,
             test_missing_cross_toolchain, test_multilib_identity, test_manual_source_dependencies,
             test_manual_removed_zip_member, test_example_helper_headers, test_example_cleanup_arguments,
             test_cross_host_identity, test_sound_member_collision, test_multi_module_fixtures,
             test_doc_target_guard, test_source_artifact_guard, test_compiler_link_failure,
             test_build_configuration, test_build_rule_dependencies, test_compiler_configuration,
             test_warning_normalization,
             test_tool_replacement, test_bootstrap_configuration, test_bootstrap_emission_configuration,
             test_isolated_builds, test_source_docs)
    if options.case and options.case not in {test.__name__ for test in tests}:
        parser.error("unknown regression: " + options.case)
    failures = 0
    executed = 0
    with tempfile.TemporaryDirectory(prefix="fbc-make-contracts-") as temporary:
        for test in tests:
            if options.case and options.case != test.__name__:
                continue
            working = Path(temporary) / test.__name__
            working.mkdir()
            executed += 1
            try:
                test(options.root.resolve(), working)
            except (AssertionError, OSError, subprocess.TimeoutExpired) as error:
                failures += 1
                print(test.__name__ + ": FAILED\n" + str(error), flush=True)
            else:
                print(test.__name__ + ": passed", flush=True)
    print(f"MAKE_CONTRACTS_{'FAIL' if failures else 'PASS'} cases={executed} failures={failures}")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())

# end of test-make-contracts.py
