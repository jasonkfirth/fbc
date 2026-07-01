#!/usr/bin/env python3
#
# Project: FreeBASIC NuttX/RP2350 SRAM profile budget
# ---------------------------------------------------
#
# File: nuttx-rp2350-sram-profile-check.py
#
# Purpose:
#
#     Classify a built NuttX image against the RP2350-PiZero SRAM budget.
#
# Responsibilities:
#
#     - read the loadable text, data, and bss sizes from a NuttX ELF image
#     - report whether the image can fit in a 512 KB all-RAM layout
#     - report the controller budget when code executes from flash/XIP
#     - reserve explicit heap, stack, graphics, audio, USB, and network slack
#       for the profiles we expect to test on hardware
#
# This file intentionally does NOT contain:
#
#     - a linker script parser
#     - a claim that QEMU virt devices match RP2350 electrical timing
#     - automatic NuttX configuration changes
#     - RP2350 image generation or controller writes
#

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path


RP2350_SRAM_BYTES = 512 * 1024
SCREEN13_FRAMEBUFFER_BYTES = 320 * 200
DVI_SCANOUT_BYTES = 11148
GRAPHICS_RESERVED_BYTES = SCREEN13_FRAMEBUFFER_BYTES + DVI_SCANOUT_BYTES
AUDIO_RESERVED_BYTES = 2 * 8192
USB_AND_NET_RESERVED_BYTES = 64 * 1024
CONSOLE_APP_STACK_BYTES = 16 * 1024
GFX_APP_STACK_BYTES = 32 * 1024
MIN_HEAP_BYTES = 64 * 1024


@dataclass(frozen=True)
class ImageSize:
    text: int
    data: int
    bss: int

    @property
    def all_ram_bytes(self) -> int:
        return self.text + self.data + self.bss

    @property
    def xip_static_ram_bytes(self) -> int:
        return self.data + self.bss


@dataclass(frozen=True)
class Profile:
    name: str
    stack_bytes: int
    graphics_bytes: int = 0
    audio_bytes: int = 0
    usb_net_bytes: int = 0
    heap_bytes: int = MIN_HEAP_BYTES

    def reserve_bytes(self) -> int:
        return (
            self.stack_bytes +
            self.graphics_bytes +
            self.audio_bytes +
            self.usb_net_bytes +
            self.heap_bytes
        )


PROFILES: tuple[Profile, ...] = (
    Profile(
        "console-xip",
        stack_bytes=CONSOLE_APP_STACK_BYTES,
    ),
    Profile(
        "gfx-xip",
        stack_bytes=GFX_APP_STACK_BYTES,
        graphics_bytes=GRAPHICS_RESERVED_BYTES,
    ),
    Profile(
        "sfx-xip",
        stack_bytes=GFX_APP_STACK_BYTES,
        audio_bytes=AUDIO_RESERVED_BYTES,
    ),
    Profile(
        "everything-xip",
        stack_bytes=GFX_APP_STACK_BYTES,
        graphics_bytes=GRAPHICS_RESERVED_BYTES,
        audio_bytes=AUDIO_RESERVED_BYTES,
        usb_net_bytes=USB_AND_NET_RESERVED_BYTES,
    ),
)


def fail(message: str) -> None:
    print(f"nuttx-sram-budget: FAIL {message}", file=sys.stderr)
    raise SystemExit(1)


def parse_size_output(text: str) -> ImageSize:
    lines = [line.strip() for line in text.splitlines() if line.strip()]

    for line in lines:
        fields = line.split()

        if len(fields) < 4:
            continue

        if fields[0] == "text":
            continue

        if not all(re.fullmatch(r"[0-9]+", field) for field in fields[:3]):
            continue

        return ImageSize(
            text=int(fields[0], 10),
            data=int(fields[1], 10),
            bss=int(fields[2], 10),
        )

    fail("could not parse size output")


def run_size_tool(elf_path: Path, size_tool: str) -> ImageSize:
    try:
        completed = subprocess.run(
            [size_tool, str(elf_path)],
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
    except OSError as ex:
        fail(f"could not run {size_tool}: {ex}")
    except subprocess.CalledProcessError as ex:
        stderr = ex.stderr.strip()

        if stderr:
            fail(f"{size_tool} failed: {stderr}")

        fail(f"{size_tool} failed with status {ex.returncode}")

    return parse_size_output(completed.stdout)


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Check a NuttX ELF against RP2350 SRAM profiles."
    )
    parser.add_argument(
        "elf",
        type=Path,
        help="Built NuttX ELF image, normally <nuttx-workdir>/nuttx/nuttx.",
    )
    parser.add_argument(
        "--size-tool",
        default="size",
        help="Size tool to use. Default: size.",
    )
    parser.add_argument(
        "--sram-bytes",
        type=int,
        default=RP2350_SRAM_BYTES,
        help="Target SRAM budget in bytes. Default: 512 KiB.",
    )

    return parser.parse_args(argv)


def report_profile(size: ImageSize, profile: Profile, sram_bytes: int) -> None:
    used = size.xip_static_ram_bytes + profile.reserve_bytes()
    remaining = sram_bytes - used

    print(
        "nuttx-sram-budget: profile %s xip-used bytes=%d remaining=%d"
        % (profile.name, used, remaining)
    )

    if remaining < 0:
        fail(f"profile {profile.name} exceeds SRAM by {-remaining} bytes")


def main(argv: list[str]) -> int:
    args = parse_args(argv)

    if not args.elf.is_file():
        fail(f"missing ELF image: {args.elf}")

    if args.sram_bytes <= 0:
        fail("SRAM budget must be positive")

    size = run_size_tool(args.elf, args.size_tool)
    all_ram_remaining = args.sram_bytes - size.all_ram_bytes
    xip_static_remaining = args.sram_bytes - size.xip_static_ram_bytes

    print(
        "nuttx-sram-budget: image text=%d data=%d bss=%d all-ram=%d"
        % (size.text, size.data, size.bss, size.all_ram_bytes)
    )
    print(
        "nuttx-sram-budget: target sram bytes=%d"
        % args.sram_bytes
    )
    print(
        "nuttx-sram-budget: all-ram remaining=%d"
        % all_ram_remaining
    )

    if all_ram_remaining >= 0:
        print("nuttx-sram-budget: all-ram image fits before runtime reserves")
    else:
        print(
            "nuttx-sram-budget: qemu all-device all-ram exceeds 512K as expected"
        )

    print(
        "nuttx-sram-budget: xip static bytes=%d remaining-before-reserves=%d"
        % (size.xip_static_ram_bytes, xip_static_remaining)
    )
    print(
        "nuttx-sram-budget: reserve graphics=%d audio=%d usb-net=%d app-stack=%d heap=%d"
        % (
            GRAPHICS_RESERVED_BYTES,
            AUDIO_RESERVED_BYTES,
            USB_AND_NET_RESERVED_BYTES,
            GFX_APP_STACK_BYTES,
            MIN_HEAP_BYTES,
        )
    )

    for profile in PROFILES:
        report_profile(size, profile, args.sram_bytes)

    print("nuttx-sram-budget: RP2350 SRAM profile budget passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))

# end of nuttx-rp2350-sram-profile-check.py
