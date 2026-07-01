#!/usr/bin/env python3
#
# Project: FreeBASIC NuttX/RP2350-PiZero device audit
# ---------------------------------------------------
#
# File: nuttx-rp2350-device-source-audit.py
#
# Purpose:
#
#     Check that the hardware image path, RP23xx USB-host patch, and QEMU
#     device lab still describe the same controller-facing device plan.
#
# Responsibilities:
#
#     - require the native RP23xx USB host-controller patch markers
#     - require RP2350 image Kconfig choices for hub, HID, storage, and USB net
#     - require QEMU topology markers that exercise the same USB class stack
#     - produce stable PASS lines consumed by the QEMU device-lab audit
#
# This file intentionally does NOT contain:
#
#     - QEMU launch logic
#     - NuttX patch application
#     - RP2350 UF2 generation
#     - claims that QEMU emulates the RP2350 USB controller hardware
#

from __future__ import annotations

import argparse
import sys
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class SourceRequirement:
    name: str
    path: str
    markers: tuple[str, ...]
    note: str


SOURCE_REQUIREMENTS: tuple[SourceRequirement, ...] = (
    SourceRequirement(
        "RP23xx native USB host controller patch",
        "build_scripts/nuttx-patches/rp23xx-rv-usbhost-controller.patch",
        (
            "arch/risc-v/src/rp23xx-rv/rp23xx_usbhost.c",
            "FAR struct usbhost_connection_s *rp23xx_usbhost_initialize(void)",
            "config RP23XX_RV_USBHOST",
            "depends on USBHOST && !USBDEV",
            "select USBHOST_HAVE_ASYNCH",
            "RP23XX_USBCTRL_REGS_MAIN_CTRL_HOST_NDEVICE",
            "RP23XX_USBCTRL_REGS_USB_MUXING_TO_PHY",
            "RP23XX_USBCTRL_REGS_USB_PWR_VBUS_DETECT_OVERRIDE_EN",
            "RP23XX_USBCTRL_REGS_LINESTATE_TUNING_MULTI_HUB_FIX",
        ),
        "The patch adds a real RP23xx host-mode controller, not only class toggles.",
    ),
    SourceRequirement(
        "RP23xx hub and low-speed support",
        "build_scripts/nuttx-patches/rp23xx-rv-usbhost-controller.patch",
        (
            "#ifdef CONFIG_USBHOST_HUB",
            "FAR struct usbhost_hubport_s *hport",
            "static bool rp23xx_usbhost_need_preamble",
            "RP23XX_USBCTRL_REGS_ADDR_ENDPN_INTEP_PREAMBLE",
            "static int rp23xx_usbhost_connect",
            "priv->drvr.connect      = rp23xx_usbhost_connect;",
        ),
        "External hubs and low-speed devices behind a full-speed hub stay represented.",
    ),
    SourceRequirement(
        "RP23xx interrupt and bulk endpoint paths",
        "build_scripts/nuttx-patches/rp23xx-rv-usbhost-controller.patch",
        (
            "RP23XX_USBHOST_INT_ENDPOINTS",
            "USB_EP_ATTR_XFER_INT",
            "RP23XX_USBHOST_INT_DATA(intnum)",
            "static ssize_t rp23xx_usbhost_transfer",
            "rp23xx_usbhost_start_transfer",
            "priv->drvr.transfer     = rp23xx_usbhost_transfer;",
            "priv->drvr.asynch       = rp23xx_usbhost_asynch;",
        ),
        "HID interrupt endpoints and storage/network bulk transfers have driver hooks.",
    ),
    SourceRequirement(
        "RP2350 image USB host configuration",
        "build_scripts/nuttx-rp2350-pizero-image.sh",
        (
            "rp23xx-rv-usbhost-controller.patch",
            "nuttx_has_rp23xx_usbhost_controller",
            "kconfig_disable CONFIG_USBDEV",
            "kconfig_enable CONFIG_USBHOST",
            "kconfig_enable CONFIG_RP23XX_RV_USBHOST",
            "kconfig_enable CONFIG_USBHOST_ASYNCH",
            "kconfig_enable CONFIG_USBHOST_WAITER",
            "kconfig_enable CONFIG_USBHOST_HUB",
            "kconfig_enable CONFIG_USBHOST_HID",
            "kconfig_enable CONFIG_USBHOST_HIDKBD",
            "kconfig_enable CONFIG_USBHOST_HIDMOUSE",
            "kconfig_enable CONFIG_USBHOST_MSC",
        ),
        "The hardware image enables the native host driver and expected USB classes.",
    ),
    SourceRequirement(
        "RP2350 image USB Ethernet configuration",
        "build_scripts/nuttx-rp2350-pizero-image.sh",
        (
            "--with-usb-ethernet",
            "kconfig_enable CONFIG_NET",
            "kconfig_enable CONFIG_NET_IPv4",
            "kconfig_enable CONFIG_NET_TCP",
            "kconfig_enable CONFIG_NET_UDP",
            "kconfig_enable CONFIG_NETUTILS_DHCPC",
            "kconfig_enable CONFIG_USBHOST_CDCECM",
            "require_config_enabled CONFIG_USBHOST_CDCECM",
        ),
        "USB Ethernet remains optional but configured through the standard CDC-ECM path.",
    ),
    SourceRequirement(
        "QEMU USB hub topology",
        "build_scripts/nuttx-riscv32-qemu-smoke.sh",
        (
            '-device "usb-hub,bus=fbxlxhci.0,port=1,id=fbxlhub,ports=4,port-power=on"',
            '-device "usb-storage,bus=fbxlxhci.0,port=1.1,drive=fbxlusb"',
            '-device "usb-kbd,bus=fbxlxhci.0,port=1.2"',
            '-device "usb-mouse,bus=fbxlxhci.0,port=1.3"',
            '-device "usb-net,bus=fbxlxhci.0,port=1.4,netdev=fbxlusbnet"',
            "CONFIG_USBHOST_XHCI_PCI",
            "CONFIG_USBHOST_HUB",
            "CONFIG_USBHOST_MSC",
            "CONFIG_USBHOST_HIDKBD",
            "CONFIG_USBHOST_HIDMOUSE",
            "CONFIG_USBHOST_CDCECM",
        ),
        "The emulator boots the same hub, storage, HID, and USB-net class stack.",
    ),
    SourceRequirement(
        "QEMU device lab coverage",
        "build_scripts/nuttx-riscv32-qemu-device-lab.sh",
        (
            "usbcombo",
            "allcombo",
            "usbhub",
            "usbnet",
            "--qemu-usb-hub-devices",
            "--expect-qemu-usb-hub-supported",
            "--expect-qemu-usb-net-supported",
            "--qemu-inject-hid-events",
        ),
        "The saved-log lab keeps combined and focused USB hub/device cases.",
    ),
)


def find_project_root(start: Path) -> Path:
    search = start.resolve()

    while True:
        if (search / "build_scripts").is_dir() and (
            (search / "GNUmakefile").is_file()
            or (search / "makefile").is_file()
            or (search / "Makefile").is_file()
        ):
            return search

        if search.parent == search:
            raise RuntimeError("could not locate FreeBASIC project root")

        search = search.parent


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace")


def audit_requirement(root: Path, requirement: SourceRequirement) -> list[str]:
    source_path = root / requirement.path

    if not source_path.is_file():
        return [f"missing source: {requirement.path}"]

    text = read_text(source_path)
    failures = []

    for marker in requirement.markers:
        if marker not in text:
            failures.append(f"{requirement.path}: missing marker: {marker}")

    return failures


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Audit RP2350 device source coverage for the NuttX lab."
    )
    parser.add_argument(
        "--root",
        type=Path,
        default=None,
        help="FreeBASIC project root. Default: auto-detect from cwd.",
    )

    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    root = args.root.resolve() if args.root is not None else find_project_root(Path.cwd())
    failures = []

    print(f"nuttx-device-source-audit: root {root}")

    for requirement in SOURCE_REQUIREMENTS:
        requirement_failures = audit_requirement(root, requirement)
        if requirement_failures:
            failures.extend(requirement_failures)
            print(f"nuttx-device-source-audit: FAIL {requirement.name}")
        else:
            print(f"nuttx-device-source-audit: PASS {requirement.name}")
            print(f"  {requirement.note}")

    if failures:
        for failure in failures:
            print(f"nuttx-device-source-audit: {failure}", file=sys.stderr)
        return 1

    print("nuttx-device-source-audit: all device source checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))

# end of nuttx-rp2350-device-source-audit.py
