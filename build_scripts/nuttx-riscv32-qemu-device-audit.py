#!/usr/bin/env python3
#
# Project: FreeBASIC NuttX/RISC-V device lab
# ------------------------------------------
#
# File: nuttx-riscv32-qemu-device-audit.py
#
# Purpose:
#
#     Audit a saved QEMU device-lab log directory against the controller
#     features we are trying to prove before writing an RP2350 image.
#
# Responsibilities:
#
#     - verify that every expected QEMU case log exists
#     - require the log markers that prove each emulated device surface
#     - reject unexpected compiler, Kconfig, or build warnings in proof logs
#     - separate proven emulator behavior from hardware-only gaps
#     - produce stable text that can be used by scripts or by a developer
#
# This file intentionally does NOT contain:
#
#     - QEMU launch logic
#     - NuttX configuration changes
#     - RP2350 image generation or mounted-drive writes
#     - claims that QEMU emulates RP2350 HSTX, PIO, DMA, or USB PHY timing
#

from __future__ import annotations

import argparse
import hashlib
import re
import sys
from dataclasses import dataclass
from pathlib import Path


ANSI_ESCAPE_RE = re.compile(r"\x1b\[[0-9;?]*[A-Za-z]")


@dataclass(frozen=True)
class Requirement:
    name: str
    log_name: str
    markers: tuple[str, ...]
    note: str
    patterns: tuple[str, ...] = ()


REQUIREMENTS: tuple[Requirement, ...] = (
    Requirement(
        "generic rtlib staging audit",
        "rtlibaudit.log",
        (
            "nuttx-rtlib-audit: PASS generic memory clear/copy helper",
            "nuttx-rtlib-audit: PASS generic GOSUB helper",
            "nuttx-rtlib-audit: PASS generic object base helper",
            "nuttx-rtlib-audit: PASS generic object type check helper",
            "nuttx-rtlib-audit: PASS generic FIX helper",
            "nuttx-rtlib-audit: PASS generic MK/CV numeric helpers",
            "nuttx-rtlib-audit: PASS generic FRAC helper",
            "nuttx-rtlib-audit: PASS generic integer LOG10 helpers",
            "nuttx-rtlib-audit: PASS generic RND/RANDOMIZE helpers",
            "nuttx-rtlib-audit: PASS generic SGN helpers",
            "nuttx-rtlib-audit: PASS generic date/time clock helpers",
            "nuttx-rtlib-audit: PASS generic environment variable helpers",
            "nuttx-rtlib-audit: PASS generic directory command helpers",
            "nuttx-rtlib-audit: PASS generic file copy helper",
            "nuttx-rtlib-audit: PASS generic base conversion string helpers",
            "nuttx-rtlib-audit: PASS generic string fill helpers",
            "nuttx-rtlib-audit: PASS generic string extra helpers",
            "nuttx-rtlib-audit: PASS generic WSTRING operator helpers",
            "nuttx-rtlib-audit: PASS generic assertion helpers",
            "nuttx-rtlib-audit: all runtime staging checks passed",
        ),
        "The NuttX QEMU smoke path keeps approved generic rtlib helpers enabled.",
    ),
    Requirement(
        "RP2350 device source alignment",
        "sourceaudit.log",
        (
            "nuttx-device-source-audit: PASS RP23xx native USB host controller patch",
            "nuttx-device-source-audit: PASS RP23xx hub and low-speed support",
            "nuttx-device-source-audit: PASS RP23xx interrupt and bulk endpoint paths",
            "nuttx-device-source-audit: PASS RP2350 image USB host configuration",
            "nuttx-device-source-audit: PASS RP2350 image USB Ethernet configuration",
            "nuttx-device-source-audit: PASS QEMU USB hub topology",
            "nuttx-device-source-audit: PASS QEMU device lab coverage",
            "nuttx-device-source-audit: all device source checks passed",
        ),
        "The RP2350 image path, native USB-host patch, and QEMU topology stay aligned.",
    ),
    Requirement(
        "core FreeBASIC app",
        "core.log",
        ("NUTTX_RISCV32_FB_SMOKE_OK",),
        "Generated-C BASIC app runs under NuttX/QEMU.",
    ),
    Requirement(
        "generic GOSUB runtime",
        "gosub.log",
        ("FB_NUTTX_GOSUB_SMOKE_OK", "NUTTX_RISCV32_FB_SMOKE_OK"),
        "The lab uses the normal rtlib GOSUB helper instead of the NuttX shim.",
    ),
    Requirement(
        "heap availability",
        "fre.log",
        (
            "FB_NUTTX_FRE_SMOKE_OK",
            "fre initial nonzero =1",
            "fre after alloc lower =1",
            "fre after free sane =1",
        ),
        "FRE() sees usable NuttX heap space and tracks allocator movement.",
    ),
    Requirement(
        "generic file copy helper",
        "filecopy.log",
        ("FB_NUTTX_FILECOPY_SMOKE_OK", "NUTTX_RISCV32_FB_SMOKE_OK"),
        "The normal C stdio FILECOPY helper is linked and copies file data.",
    ),
    Requirement(
        "generic CV numeric helpers",
        "mathcvn.log",
        ("FB_NUTTX_MATH_CVN_SMOKE_OK", "NUTTX_RISCV32_FB_SMOKE_OK"),
        "Target-neutral MK*/CV* bit conversion helpers are linked and run.",
    ),
    Requirement(
        "generic FRAC helper",
        "frac.log",
        ("FB_NUTTX_FRAC_SMOKE_OK", "NUTTX_RISCV32_FB_SMOKE_OK"),
        "Target-neutral FRAC() helper is linked and run.",
    ),
    Requirement(
        "generic integer LOG10 helpers",
        "log10.log",
        ("FB_NUTTX_LOG10_SMOKE_OK", "NUTTX_RISCV32_FB_SMOKE_OK"),
        "Target-neutral integer base-10 helpers are linked and run.",
    ),
    Requirement(
        "generic FIX helper",
        "core.log",
        ("fb_nuttx_math_fix.c", "fix sample =1", "NUTTX_RISCV32_FB_SMOKE_OK"),
        "Target-neutral FIX() helper is compiled and exercised by the core app.",
    ),
    Requirement(
        "generic RND/RANDOMIZE helpers",
        "core.log",
        ("fb_nuttx_math_rnd.c", "rnd range =1", "NUTTX_RISCV32_FB_SMOKE_OK"),
        "Target-neutral RND/RANDOMIZE helpers are compiled and exercised by the core app.",
    ),
    Requirement(
        "generic SGN helpers",
        "sign.log",
        ("FB_NUTTX_SIGN_SMOKE_OK", "NUTTX_RISCV32_FB_SMOKE_OK"),
        "Target-neutral SGN() helpers are linked and run.",
    ),
    Requirement(
        "generic assertion helpers",
        "assert.log",
        (
            "fb_nuttx_error_assert.c",
            "fb_nuttx_error_assert_wstr.c",
            "generic assert warn",
            "generic wide assert warn",
            "generic assert hard",
            "fb-nuttx-status=1",
            "NUTTX_RISCV32_FB_SMOKE_OK",
        ),
        "The normal ASSERT and ASSERTWARN helpers run under the NuttX smoke path.",
    ),
    Requirement(
        "RP2350 graphics memory budget",
        "membudget.log",
        (
            "nuttx-memory-budget: framebuffer limit bytes=64000",
            "nuttx-memory-budget: packed QB modes within 64K",
            "nuttx-memory-budget: packed QB modes over 64K",
            "nuttx-memory-budget: RP2350 graphics memory budget passed",
        ),
        "The exposed gfxlib modes, packed-mode plan, and DVI scanout buffers stay inside the SRAM budget.",
        (
            r"packed QB modes within 64K .*SCREEN 13=320x200x8:64000",
            r"packed QB modes over 64K .*SCREEN 12=640x480x4:153600",
        ),
    ),
    Requirement(
        "RP2350 SRAM profile budget",
        "srambudget.log",
        (
            "nuttx-sram-budget: image text=",
            "nuttx-sram-budget: qemu all-device all-ram exceeds 512K as expected",
            "nuttx-sram-budget: profile console-xip xip-used",
            "nuttx-sram-budget: profile gfx-xip xip-used",
            "nuttx-sram-budget: profile sfx-xip xip-used",
            "nuttx-sram-budget: profile everything-xip xip-used",
            "nuttx-sram-budget: RP2350 SRAM profile budget passed",
        ),
        "The current built image is classified as QEMU-only for all-RAM, while the XIP controller profiles fit 512 KB SRAM.",
        (
            r"profile everything-xip xip-used bytes=[0-9]+ remaining=[1-9][0-9]*",
        ),
    ),
    Requirement(
        "RP2350 DVI parity",
        "dviparity.log",
        (
            "nuttx-dvi-parity: gfx driver matches standalone DVI smoke constants",
            "nuttx-dvi-parity: standalone solid field matches reference TMDS words",
            "nuttx-dvi-parity: gfx framebuffer scanout model matches solid DVI smoke",
            "nuttx-dvi-parity: self-test mutation checks passed",
        ),
        "The gfxlib scanout path matches the standalone known-good DVI smoke basis.",
    ),
    Requirement(
        "standalone DVI demo model",
        "dvisolid.log",
        (
            "FB_NUTTX_QEMU_DVI_SOLID_MODEL_OK",
            "fbdvi: QEMU solid DVI model ok htotal=800 vtotal=525 active=640x480",
            "fbdvi: QEMU solid DVI model tmds=0007fd00,0007fd00,000bfa01",
            "NUTTX_RISCV32_FB_SMOKE_OK",
        ),
        "The known-good standalone DVI demo source boots in QEMU model mode and validates its timing/TMDS basis.",
    ),
    Requirement(
        "gfxlib framebuffer modes",
        "gfx.log",
        (
            "NUTTX_GFX_SMOKE_OK",
            "FB_NUTTX_QEMU_GFX_PRESENT",
            "FB_NUTTX_QEMU_GFX_VISUAL",
            "FB_NUTTX_QEMU_DVI_SCANOUT",
            "FB_NUTTX_QEMU_HDMI_CONSOLE",
            'text="gfx print ok"',
        ),
        "SCREEN 1/7/13, HDMI console text, and QEMU-visible framebuffer-to-DVI presentation work.",
        (
            r"FB_NUTTX_QEMU_GFX_VISUAL .*p10_10=4",
            r"FB_NUTTX_QEMU_DVI_SCANOUT .*sample_x=10 index=4 .*nonblack=[1-9][0-9]*",
        ),
    ),
    Requirement(
        "gfxlib USB HID input devices",
        "gfxhid.log",
        (
            "FB_NUTTX_GFX_HID_SMOKE_OK",
            "FB_NUTTX_QEMU_HID_KEYBOARD_OPEN /dev/kbda",
            "FB_NUTTX_QEMU_HID_MOUSE_OPEN /dev/mouse0",
            "NuttX USB HID: QEMU monitor injected keyboard and mouse events reached gfxlib",
        ),
        "gfxlib opens the NuttX USB HID nodes and consumes injected keyboard and mouse events.",
    ),
    Requirement(
        "combined USB hub device workload",
        "usbcombo.log",
        (
            "FB_NUTTX_DEVICE_COMBO_SMOKE_OK",
            "FB_NUTTX_QEMU_GFX_VISUAL",
            "FB_NUTTX_QEMU_DVI_SCANOUT",
            "fbxl QEMU usb-storage ready: /mnt/sd0",
            "FB_NUTTX_QEMU_HID_KEYBOARD_OPEN /dev/kbda",
            "FB_NUTTX_QEMU_HID_MOUSE_OPEN /dev/mouse0",
            "NuttX USB HID: QEMU monitor injected keyboard and mouse events reached gfxlib",
            "NuttX USB hub: QEMU hub-attached storage and HID device nodes bound",
            "NuttX USB Ethernet: QEMU usb-net bound as a CDC Ethernet device",
        ),
        "One QEMU boot runs gfxlib, USB HID, USB mass storage, and USB Ethernet behind the hub.",
    ),
    Requirement(
        "combined all-device workload",
        "allcombo.log",
        (
            "FB_NUTTX_DEVICE_COMBO_SMOKE_OK",
            "FB_NUTTX_QEMU_GFX_VISUAL",
            "FB_NUTTX_QEMU_DVI_SCANOUT",
            "fbxl QEMU usb-storage ready: /mnt/sd0",
            "FB_NUTTX_QEMU_HID_KEYBOARD_OPEN /dev/kbda",
            "FB_NUTTX_QEMU_HID_MOUSE_OPEN /dev/mouse0",
            "NuttX USB HID: QEMU monitor injected keyboard and mouse events reached gfxlib",
            "NuttX USB hub: QEMU hub-attached storage and HID device nodes bound",
            "NuttX USB Ethernet: QEMU usb-net bound as a CDC Ethernet device",
            "fbxl QEMU virtio sound device directory follows",
            "pcm0p",
            "NuttX audio: QEMU virtio sound registered /dev/audio/pcm0p",
        ),
        "One QEMU boot presents the gfx, USB hub/HID/storage/net, and audio device surfaces together.",
    ),
    Requirement(
        "virtio storage",
        "storage.log",
        ("FB_NUTTX_SD_SMOKE_OK", "NUTTX_RISCV32_FB_SMOKE_OK"),
        "NuttX block, FAT, mount, and FreeBASIC file I/O work on a block disk.",
    ),
    Requirement(
        "USB mass storage",
        "usbstore.log",
        ("class:8 subclass:6 protocol:80", "FB_NUTTX_SD_SMOKE_OK"),
        "QEMU xHCI usb-storage backs the board-style /mnt/sd0 smoke path.",
    ),
    Requirement(
        "USB hub topology",
        "usbhub.log",
        (
            "class:9 subclass:0 protocol:0",
            "class:8 subclass:6 protocol:80",
            "class:3 subclass:1 protocol:1",
            "usbhost_kbdpoll: Started",
            "usbhost_kbdpoll: Entering poll loop",
            "class:3 subclass:1 protocol:2",
            "usbhost_mouse_poll: Started",
            "usbhost_mouse_poll: Entering poll loop",
            "fbxl QEMU usb device directory follows",
            "kbda",
            "mouse0",
            "NuttX USB hub: QEMU hub-attached storage and HID device nodes bound",
        ),
        "QEMU hub, storage, keyboard, and mouse expose usable NuttX device nodes through xHCI.",
    ),
    Requirement(
        "USB Ethernet",
        "usbnet.log",
        (
            "class:2 subclass:0 protocol:0",
            "NuttX USB Ethernet: QEMU usb-net bound as a CDC Ethernet device",
        ),
        "QEMU usb-net binds as a NuttX CDC Ethernet device.",
    ),
    Requirement(
        "sfxlib audio path",
        "sfx.log",
        (
            "FB_NUTTX_SFX_SMOKE_OK",
            "FB_NUTTX_QEMU_SFX_AUDIO_OPEN",
            "FB_NUTTX_QEMU_SFX_AUDIO_ENQUEUE",
            "NuttX audio: QEMU virtio sound registered /dev/audio/pcm0p",
        ),
        "sfxlib opens the NuttX audio PCM node and enqueues mixer output buffers.",
        (
            r"FB_NUTTX_QEMU_SFX_AUDIO_OPEN .*device=/dev/audio/pcm0p .*buffers=[1-9][0-9]*",
            r"FB_NUTTX_QEMU_SFX_AUDIO_ENQUEUE .*frames=[1-9][0-9]* .*bytes=[1-9][0-9]*",
        ),
    ),
    Requirement(
        "NuttX virtio sound device",
        "audio.log",
        (
            "fbxl QEMU virtio sound device directory follows",
            "pcm0p",
            "NuttX audio: QEMU virtio sound registered /dev/audio/pcm0p",
        ),
        "QEMU virtio-sound reaches the NuttX audio framework and exposes the PCM playback node.",
    ),
    Requirement(
        "network shell",
        "network.log",
        ("eth0", "telnetd", "FTP daemon", "fbxl NuttX network shell ready"),
        "NuttX TCP/IP, telnet, and FTP setup works under QEMU virtio-net.",
    ),
)


HARDWARE_GAPS: tuple[str, ...] = (
    "RP2350 DVI/HSTX/PIO/DMA timing still needs hardware or a dedicated model.",
    "QEMU USB HID proves keyboard and mouse events through gfxlib; hardware USB timing still needs the board.",
    "QEMU audio checks prove sfxlib can enqueue PCM through NuttX virtio-sound, not HDMI audio transport.",
    "QEMU virtio storage is a block-driver proof; USB storage is the closer removable-device proof.",
)


FBCTESTS_REQUIREMENT = Requirement(
    "fresh staged fbctests runtime suite",
    "fbctests.log",
    (
        "nuttx-suite: run fb_pretest_compile_and_run_ok_40b7ec31",
        "nuttx-suite: ok  fb_pretest_multi_module_fail_51dda47f",
        "nuttx-suite: all tests passed",
    ),
    "The current staged NuttX fbctests runtime manifest passes under QEMU.",
)


@dataclass(frozen=True)
class LogEvidence:
    log_name: str
    size: int
    sha256: str


def read_log(log_dir: Path, log_name: str) -> str:
    log_path = log_dir / log_name

    if not log_path.is_file():
        raise FileNotFoundError(str(log_path))

    return log_path.read_text(encoding="utf-8", errors="replace")


def normalize_log_text(text: str) -> str:
    text = ANSI_ESCAPE_RE.sub("\n", text)
    return text.replace("\r", "\n")


def warning_lines(text: str) -> list[str]:
    normalized = normalize_log_text(text)
    return [
        line.strip()
        for line in normalized.splitlines()
        if "warning:" in line.lower()
    ]


def audit_warning_clean(text: str, log_name: str) -> list[str]:
    warnings = warning_lines(text)

    if not warnings:
        return []

    failures = [f"{log_name}: unexpected warning count: {len(warnings)}"]

    for line in warnings[:8]:
        failures.append(f"{log_name}: unexpected warning: {line}")

    if len(warnings) > 8:
        failures.append(f"{log_name}: unexpected warning: ... {len(warnings) - 8} more")

    return failures


def log_evidence(log_dir: Path, log_name: str) -> LogEvidence:
    log_path = log_dir / log_name

    return log_evidence_path(log_path, log_name)


def log_evidence_path(log_path: Path, log_name: str | None = None) -> LogEvidence:
    data = log_path.read_bytes()

    return LogEvidence(
        log_name=log_name if log_name is not None else str(log_path),
        size=len(data),
        sha256=hashlib.sha256(data).hexdigest(),
    )


def audit_requirement(log_dir: Path, requirement: Requirement) -> list[str]:
    try:
        text = read_log(log_dir, requirement.log_name)
    except FileNotFoundError:
        return [f"missing log: {requirement.log_name}"]

    missing = audit_warning_clean(text, requirement.log_name)

    for marker in requirement.markers:
        if marker not in text:
            missing.append(f"{requirement.log_name}: missing marker: {marker}")

    for pattern in requirement.patterns:
        if re.search(pattern, text) is None:
            missing.append(f"{requirement.log_name}: missing pattern: {pattern}")

    return missing


def audit_text_requirement(text: str, requirement: Requirement) -> list[str]:
    missing = audit_warning_clean(text, requirement.log_name)

    for marker in requirement.markers:
        if marker not in text:
            missing.append(f"{requirement.log_name}: missing marker: {marker}")

    for pattern in requirement.patterns:
        if re.search(pattern, text) is None:
            missing.append(f"{requirement.log_name}: missing pattern: {pattern}")

    return missing


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Audit NuttX/RISC-V QEMU device-lab logs."
    )
    parser.add_argument(
        "log_dir",
        type=Path,
        help="Directory containing per-case device-lab logs.",
    )
    parser.add_argument(
        "--strict-gaps",
        action="store_true",
        help="Treat documented hardware-only gaps as a failure.",
    )
    parser.add_argument(
        "--write-evidence",
        type=Path,
        default=None,
        help="Write a TSV file containing the audited log size and SHA-256 digest.",
    )
    parser.add_argument(
        "--fbctests-log",
        type=Path,
        default=None,
        help="Also audit a full staged NuttX fbctests QEMU suite log.",
    )

    return parser.parse_args(argv)


def write_evidence(path: Path, evidence: list[LogEvidence]) -> None:
    lines = ["log\tbytes\tsha256"]

    for item in evidence:
        lines.append(f"{item.log_name}\t{item.size}\t{item.sha256}")

    path.write_text("\n".join(lines) + "\n", encoding="utf-8", newline="\n")


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    log_dir = args.log_dir
    failures: list[str] = []
    evidence: list[LogEvidence] = []
    seen_logs: set[str] = set()

    if not log_dir.is_dir():
        print(f"nuttx-device-audit: missing log directory: {log_dir}", file=sys.stderr)
        return 2

    print(f"nuttx-device-audit: log dir {log_dir}")

    for requirement in REQUIREMENTS:
        missing = audit_requirement(log_dir, requirement)

        if missing:
            failures.extend(missing)
            print(f"nuttx-device-audit: FAIL {requirement.name}")
        else:
            if requirement.log_name not in seen_logs:
                evidence.append(log_evidence(log_dir, requirement.log_name))
                seen_logs.add(requirement.log_name)

            print(f"nuttx-device-audit: PASS {requirement.name}")
            print(f"  {requirement.note}")

    if args.fbctests_log is not None:
        fbctests_log = args.fbctests_log

        if not fbctests_log.is_file():
            failures.append(f"missing fbctests log: {fbctests_log}")
            print(f"nuttx-device-audit: FAIL {FBCTESTS_REQUIREMENT.name}")
        else:
            text = fbctests_log.read_text(encoding="utf-8", errors="replace")
            missing = audit_text_requirement(text, FBCTESTS_REQUIREMENT)

            if missing:
                failures.extend(missing)
                print(f"nuttx-device-audit: FAIL {FBCTESTS_REQUIREMENT.name}")
            else:
                evidence.append(log_evidence_path(fbctests_log))
                print(f"nuttx-device-audit: PASS {FBCTESTS_REQUIREMENT.name}")
                print(f"  {FBCTESTS_REQUIREMENT.note}")

    print("nuttx-device-audit: hardware gaps")

    for gap in HARDWARE_GAPS:
        print(f"  - {gap}")

    if args.strict_gaps:
        failures.extend(f"hardware gap: {gap}" for gap in HARDWARE_GAPS)

    if failures:
        print("nuttx-device-audit: failures", file=sys.stderr)

        for failure in failures:
            print(f"  - {failure}", file=sys.stderr)

        return 1

    print("nuttx-device-audit: evidence")

    for item in evidence:
        print(
            "  - %s bytes=%d sha256=%s"
            % (item.log_name, item.size, item.sha256[:16])
        )

    if args.write_evidence is not None:
        write_evidence(args.write_evidence, evidence)
        print(f"nuttx-device-audit: evidence file {args.write_evidence}")

    print("nuttx-device-audit: all emulated requirements passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
