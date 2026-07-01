#!/usr/bin/env python3
#
# Project: FreeBASIC NuttX/RP2350 memory budget
# ---------------------------------------------
#
# File: nuttx-rp2350-memory-budget-check.py
#
# Purpose:
#
#     Check the static graphics and DVI memory budget before a scarce
#     RP2350-PiZero hardware write.
#
# Responsibilities:
#
#     - verify that the NuttX gfxlib driver exposes only low-memory
#       paletted 320x200 modes
#     - verify that each currently exposed gfxlib page remains capped at the
#       SCREEN 13 64,000 byte budget
#     - model which QB-style packed framebuffer modes can fit that same limit
#     - calculate the DVI scanout buffers from the source constants
#     - fail when the framebuffer plus scanout bookkeeping grows past
#       the budget reserved for graphics
#
# This file intentionally does NOT contain:
#
#     - a C parser
#     - live NuttX heap inspection
#     - RP2350 image generation
#     - mounted-drive or controller writes
#

from __future__ import annotations

import re
import sys
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
GFX_DRIVER = ROOT / "src/gfxlib2/nuttx/gfx_driver.c"
DVI_DRIVER = ROOT / "src/gfxlib2/nuttx/gfx_rp2350_dvi.c"

FRAMEBUFFER_LIMIT = 320 * 200
GRAPHICS_RESERVED_LIMIT = 96 * 1024
UINT32_SIZE = 4
RISCV32_UINTPTR_SIZE = 4
FBDVI_DMA_CB_SIZE = UINT32_SIZE + RISCV32_UINTPTR_SIZE + RISCV32_UINTPTR_SIZE + UINT32_SIZE

PACKED_MODE_LIMIT = 64 * 1024
PACKED_QB_MODES = (
    (1, 320, 200, 2),
    (2, 640, 200, 1),
    (7, 320, 200, 4),
    (8, 640, 200, 4),
    (10, 640, 350, 2),
    (11, 640, 480, 1),
    (13, 320, 200, 8),
)
OVER_BUDGET_QB_MODES = (
    (9, 640, 350, 4),
    (12, 640, 480, 4),
)


@dataclass(frozen=True)
class Mode:
    width: int
    height: int
    depth: int


def read_text(path: Path) -> str:
    try:
        return path.read_text(encoding="utf-8")
    except OSError as ex:
        raise SystemExit(f"cannot read {path}: {ex}") from ex


def define_int(text: str, name: str) -> int:
    pattern = re.compile(
        r"^[ \t]*#define[ \t]+" + re.escape(name) +
        r"[ \t]+(?P<value>[0-9]+)(?:u|U)?[ \t]*$",
        re.MULTILINE,
    )
    match = pattern.search(text)

    if match is None:
        raise SystemExit(f"missing integer #define {name}")

    return int(match.group("value"), 10)


def parse_modes(text: str) -> list[Mode]:
    pattern = re.compile(
        r"static[ \t]+const[ \t]+NUTTX_MODE[ \t]+nuttx_modes\[\][ \t]*="
        r"[ \t\r\n]*\{(?P<body>.*?)\};",
        re.DOTALL,
    )
    match = pattern.search(text)

    if match is None:
        raise SystemExit("missing nuttx_modes table")

    modes = []

    for width, height, depth in re.findall(
        r"\{\s*([0-9]+)\s*,\s*([0-9]+)\s*,\s*([0-9]+)\s*\}",
        match.group("body"),
    ):
        modes.append(Mode(int(width), int(height), int(depth)))

    if not modes:
        raise SystemExit("nuttx_modes table is empty")

    return modes


def fail(message: str) -> None:
    print(f"nuttx-memory-budget: FAIL {message}", file=sys.stderr)
    raise SystemExit(1)


def packed_framebuffer_bytes(width: int, height: int, depth: int) -> int:
    pixels = width * height
    bits = pixels * depth

    return (bits + 7) // 8


def format_packed_mode(mode: tuple[int, int, int, int]) -> str:
    screen, width, height, depth = mode
    size = packed_framebuffer_bytes(width, height, depth)

    return f"SCREEN {screen}={width}x{height}x{depth}:{size}"


def main() -> int:
    gfx_text = read_text(GFX_DRIVER)
    dvi_text = read_text(DVI_DRIVER)

    screen_width = define_int(gfx_text, "NUTTX_SCREEN_WIDTH")
    screen_height = define_int(gfx_text, "NUTTX_SCREEN_HEIGHT")
    framebuffer_limit = screen_width * screen_height

    if framebuffer_limit != FRAMEBUFFER_LIMIT:
        fail(f"unexpected framebuffer limit {framebuffer_limit}")

    modes = parse_modes(gfx_text)

    for mode in modes:
        if (mode.width, mode.height) != (screen_width, screen_height):
            fail(f"mode {mode.width}x{mode.height} is outside the budget")

        if mode.depth not in (1, 2, 4, 8):
            fail(f"mode depth {mode.depth} is not paletted")

        mode_bytes = mode.width * mode.height

        if mode_bytes > framebuffer_limit:
            fail(f"mode {mode.width}x{mode.height} needs {mode_bytes} bytes")

    lane_count = define_int(dvi_text, "FBDVI_LANE_COUNT")
    sync_chunks = define_int(dvi_text, "FBDVI_SYNC_LANE_CHUNKS")
    nosync_chunks = define_int(dvi_text, "FBDVI_NOSYNC_LANE_CHUNKS")
    active_words = define_int(dvi_text, "FBDVI_ACTIVE_WORDS")
    fb_width = define_int(dvi_text, "FBDVI_FRAMEBUFFER_WIDTH")
    fb_height = define_int(dvi_text, "FBDVI_FRAMEBUFFER_HEIGHT")

    if (fb_width, fb_height) != (screen_width, screen_height):
        fail(f"DVI source is {fb_width}x{fb_height}, expected {screen_width}x{screen_height}")

    if lane_count != 3:
        fail(f"DVI lane count is {lane_count}, expected 3")

    line_buffer_bytes = 2 * lane_count * active_words * UINT32_SIZE
    palette_bytes = 256 * lane_count * UINT32_SIZE
    black_words_bytes = lane_count * UINT32_SIZE
    scanline_list_bytes = (sync_chunks + (2 * nosync_chunks)) * FBDVI_DMA_CB_SIZE
    scanline_lists_bytes = 3 * scanline_list_bytes
    scanout_bytes = (
        line_buffer_bytes +
        palette_bytes +
        black_words_bytes +
        scanline_lists_bytes
    )
    graphics_total = framebuffer_limit + scanout_bytes

    if scanout_bytes >= framebuffer_limit:
        fail(f"DVI scanout uses {scanout_bytes} bytes, not a small scanout buffer")

    if graphics_total > GRAPHICS_RESERVED_LIMIT:
        fail(f"graphics path uses {graphics_total} bytes, limit is {GRAPHICS_RESERVED_LIMIT}")

    packed_modes = []

    for mode in PACKED_QB_MODES:
        size = packed_framebuffer_bytes(mode[1], mode[2], mode[3])

        if size > PACKED_MODE_LIMIT:
            fail(f"packed {format_packed_mode(mode)} exceeds {PACKED_MODE_LIMIT} bytes")

        packed_modes.append(format_packed_mode(mode))

    over_budget_modes = []

    for mode in OVER_BUDGET_QB_MODES:
        size = packed_framebuffer_bytes(mode[1], mode[2], mode[3])

        if size <= PACKED_MODE_LIMIT:
            fail(f"packed {format_packed_mode(mode)} unexpectedly fits {PACKED_MODE_LIMIT} bytes")

        over_budget_modes.append(format_packed_mode(mode))

    print("nuttx-memory-budget: framebuffer limit bytes=%d" % framebuffer_limit)
    print("nuttx-memory-budget: modes " + ", ".join(
        "%dx%dx%d" % (mode.width, mode.height, mode.depth) for mode in modes
    ))
    print("nuttx-memory-budget: dvi line buffers bytes=%d" % line_buffer_bytes)
    print("nuttx-memory-budget: dvi palette bytes=%d" % palette_bytes)
    print("nuttx-memory-budget: dvi scanline lists bytes=%d" % scanline_lists_bytes)
    print("nuttx-memory-budget: scanout bytes=%d" % scanout_bytes)
    print("nuttx-memory-budget: graphics reserved bytes=%d limit=%d" % (
        graphics_total,
        GRAPHICS_RESERVED_LIMIT,
    ))
    print("nuttx-memory-budget: packed QB modes within 64K " + ", ".join(packed_modes))
    print("nuttx-memory-budget: packed QB modes over 64K " + ", ".join(over_budget_modes))
    print("nuttx-memory-budget: RP2350 graphics memory budget passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

# end of nuttx-rp2350-memory-budget-check.py
