#!/usr/bin/env python3
"""
Project: FreeBASIC gfxlib3
--------------------------

File: regenerate_vulkan_shaders.py

Purpose:

    Rebuild gfx3_vulkan_shader.h from the editable Vulkan compute shaders.

Responsibilities:

    - compile every checked shader for the Vulkan 1.0 environment
    - run the SPIR-V optimizer with deterministic inputs and ordering
    - emit aligned uint32_t arrays without depending on xxd or bin2c

This file intentionally does NOT contain:

    - run-time shader compilation
    - Vulkan device or pipeline creation
    - automatic toolchain installation
"""

from __future__ import annotations

import argparse
import shutil
import struct
import subprocess
import tempfile
from pathlib import Path


SHADERS = (
    ("vulkan_compute_smoke.comp", "fb_gfx3_vulkan_compute_smoke_spirv"),
    ("vulkan_points.comp", "fb_gfx3_vulkan_points_spirv"),
    ("vulkan_line.comp", "fb_gfx3_vulkan_line_spirv"),
    ("vulkan_rectangle.comp", "fb_gfx3_vulkan_rectangle_spirv"),
    ("vulkan_rectangle_tile.comp", "fb_gfx3_vulkan_rectangle_tile_spirv"),
    ("vulkan_blit.comp", "fb_gfx3_vulkan_blit_spirv"),
    ("vulkan_transform_blit.comp", "fb_gfx3_vulkan_transform_blit_spirv"),
    ("vulkan_blit_winner.comp", "fb_gfx3_vulkan_blit_winner_spirv"),
    ("vulkan_blit_resolve.comp", "fb_gfx3_vulkan_blit_resolve_spirv"),
    ("vulkan_blit_tile.comp", "fb_gfx3_vulkan_blit_tile_spirv"),
    ("vulkan_glyph_tile.comp", "fb_gfx3_vulkan_glyph_tile_spirv"),
    ("vulkan_ellipse.comp", "fb_gfx3_vulkan_ellipse_spirv"),
    ("vulkan_ellipse_winner.comp", "fb_gfx3_vulkan_ellipse_winner_spirv"),
    ("vulkan_ellipse_resolve.comp", "fb_gfx3_vulkan_ellipse_resolve_spirv"),
    ("vulkan_primitive_winner.comp", "fb_gfx3_vulkan_primitive_winner_spirv"),
    ("vulkan_primitive_resolve.comp", "fb_gfx3_vulkan_primitive_resolve_spirv"),
    ("vulkan_primitive_tile.comp", "fb_gfx3_vulkan_primitive_tile_spirv"),
    ("vulkan_paint.comp", "fb_gfx3_vulkan_paint_spirv"),
    ("vulkan_present.comp", "fb_gfx3_vulkan_present_spirv"),
)

SPECIALIZED_SHADERS = (
    (
        "vulkan_blit_tile_trans.comp",
        "fb_gfx3_vulkan_blit_tile_trans8_spirv",
        ("FB_GFX3_VULKAN_BLIT_TRANS_DEPTH=8",),
    ),
    (
        "vulkan_blit_tile_trans.comp",
        "fb_gfx3_vulkan_blit_tile_trans16_spirv",
        ("FB_GFX3_VULKAN_BLIT_TRANS_DEPTH=16",),
    ),
    (
        "vulkan_blit_tile_trans.comp",
        "fb_gfx3_vulkan_blit_tile_trans32_spirv",
        ("FB_GFX3_VULKAN_BLIT_TRANS_DEPTH=32",),
    ),
    (
        "vulkan_blit_tile.comp",
        "fb_gfx3_vulkan_blit_tile_nvidia_spirv",
        ("FB_GFX3_VULKAN_BLIT_BIN_SHIFT=3",),
    ),
)


def find_tool(requested: str, fallback_name: str) -> str:
    candidate = shutil.which(requested)
    if candidate is None and requested != fallback_name:
        candidate = shutil.which(fallback_name)
    if candidate is None:
        raise SystemExit(f"required tool not found: {requested}")
    return candidate


def compile_shader(
    glslang: str,
    optimizer: str,
    source: Path,
    temporary: Path,
    defines: tuple[str, ...] = (),
) -> bytes:
    unoptimized = temporary / f"{source.stem}.spv"
    optimized = temporary / f"{source.stem}.optimized.spv"
    command = [glslang, "-V", "--target-env", "vulkan1.0"]
    command.extend(f"-D{define}" for define in defines)
    command.extend(["-o", str(unoptimized), str(source)])
    subprocess.run(command, check=True)
    subprocess.run(
        [optimizer, "-O", str(unoptimized), "-o", str(optimized)],
        check=True,
    )
    data = optimized.read_bytes()
    if len(data) == 0 or (len(data) % 4) != 0:
        raise SystemExit(f"invalid SPIR-V byte count for {source.name}: {len(data)}")
    return data


def format_array(name: str, data: bytes) -> list[str]:
    words = struct.unpack(f"<{len(data) // 4}I", data)
    lines = [f"static const uint32_t {name}[] = {{"]
    for offset in range(0, len(words), 8):
        row = ", ".join(f"0x{word:08X}u" for word in words[offset : offset + 8])
        lines.append(f"\t{row},")
    lines.append("};")
    lines.append("")
    return lines


def build_header(compiled: list[tuple[str, bytes]]) -> str:
    lines = [
        "/*",
        "    Project: FreeBASIC gfxlib3",
        "    --------------------------",
        "",
        "    File: gfx3_vulkan_shader.h",
        "",
        "    Purpose:",
        "",
        "        Embed the checked SPIR-V modules used by the Vulkan compute backend.",
        "",
        "    Responsibilities:",
        "",
        "        - keep the renderer independent of a run-time shader compiler",
        "        - preserve a word-aligned representation accepted by Vulkan 1.0",
        "",
        "    This file intentionally does NOT contain:",
        "",
        "        - Vulkan object lifecycle",
        "        - editable shader source",
        "",
        "    Source:",
        "",
        "        The editable modules in shaders/vulkan_*.comp, compiled for Vulkan",
        "        1.0 with glslangValidator and optimized with spirv-opt.",
        "*/",
        "",
        "#ifndef __FB_GFX3_VULKAN_SHADER_H__",
        "#define __FB_GFX3_VULKAN_SHADER_H__",
        "",
    ]
    for name, data in compiled:
        lines.extend(format_array(name, data))
    lines.extend(
        [
            "#endif",
            "",
            "/* end of gfx3_vulkan_shader.h */",
            "",
        ]
    )
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--glslang", default="glslangValidator")
    parser.add_argument("--spirv-opt", default="spirv-opt")
    parser.add_argument("--output", type=Path)
    arguments = parser.parse_args()

    shader_directory = Path(__file__).resolve().parent
    output = arguments.output
    if output is None:
        output = shader_directory.parent / "gfx3_vulkan_shader.h"
    glslang = find_tool(arguments.glslang, "glslangValidator")
    optimizer = find_tool(arguments.spirv_opt, "spirv-opt")

    compiled: list[tuple[str, bytes]] = []
    with tempfile.TemporaryDirectory(prefix="gfxlib3-spv-") as directory:
        temporary = Path(directory)
        for source_name, array_name in SHADERS:
            source = shader_directory / source_name
            if not source.is_file():
                raise SystemExit(f"shader source not found: {source}")
            compiled.append(
                (array_name, compile_shader(glslang, optimizer, source, temporary))
            )
        for source_name, array_name, defines in SPECIALIZED_SHADERS:
            source = shader_directory / source_name
            if not source.is_file():
                raise SystemExit(f"shader source not found: {source}")
            compiled.append(
                (
                    array_name,
                    compile_shader(
                        glslang, optimizer, source, temporary, defines
                    ),
                )
            )
    output.write_text(build_header(compiled), encoding="ascii", newline="\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

# end of regenerate_vulkan_shaders.py
