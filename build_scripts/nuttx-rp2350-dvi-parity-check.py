#!/usr/bin/env python3
#
# Project: FreeBASIC NuttX/RP2350 DVI support
# -------------------------------------------
#
# File: nuttx-rp2350-dvi-parity-check.py
#
# Purpose:
#
#     Compare the hardware-critical DVI constants in the gfxlib2 RP2350
#     scanout driver against the standalone DVI smoke app.
#
# Responsibilities:
#
#     - verify that the gfxlib driver still uses the known-good pin mapping
#     - verify that the PIO serializer program and DVI timing table match
#     - verify that DVI control symbols match the standalone board smoke
#     - verify that DMA-fed DVI source words live in writable static storage
#     - verify that the gfxlib DVI scanout uses the same doubled-pixel TMDS
#       table basis as the standalone board smoke
#     - model the gfxlib framebuffer-to-TMDS scanline path for the same solid
#       red field that the standalone smoke app drives on hardware
#
# This file intentionally does NOT contain:
#
#     - a C parser
#     - hardware register access
#     - a full framebuffer renderer
#

import argparse
import contextlib
import io
import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DRIVER = ROOT / "src/gfxlib2/nuttx/gfx_rp2350_dvi.c"
SMOKE = ROOT / "examples/nuttx/fbdvi_solid.c"
KNOWN_GOOD_DVI = (
    ROOT / "build/nuttx-rp2350-pizero/waveshare/RP2350-PiZero/C/01-DVI"
)

DEFINES = [
    "FBDVI_PIO",
    "FBDVI_PIO_GPIOBASE",
    "FBDVI_LANE_COUNT",
    "FBDVI_SYNC_LANE",
    "FBDVI_SYMBOLS_PER_WORD",
    "FBDVI_SYNC_LANE_CHUNKS",
    "FBDVI_NOSYNC_LANE_CHUNKS",
    "FBDVI_GPIO_BLUE",
    "FBDVI_GPIO_GREEN",
    "FBDVI_GPIO_RED",
    "FBDVI_GPIO_CLOCK",
    "FBDVI_PWM_CLOCK_SLICE",
    "FBDVI_DMA_IRQ_PRIORITY",
    "FBDVI_DMA_WAIT_GUARD",
    "FBDVI_HAZARD3_MEIPRA_CSR",
]

TABLES = [
    "g_fbdvi_serialiser_program",
    "g_fbdvi_timing_640x480",
    "g_fbdvi_ctrl_symbols",
]

ACTIVE_SOLID_TABLE = "g_fbdvi_active_solid_tmds"
ACTIVE_SOLID_RGB = (255, 0, 0)
DRIVER_TMDS_PAIR_TABLE = "g_fbdvi_tmds_pair_table"

DRIVER_ENCODER_MARKERS = [
    "static uint32_t fbdvi_tmds_word(uint8_t value)",
    "return g_fbdvi_tmds_pair_table[value >> 2];",
    "g_fbdvi_palette_words[index][0] = fbdvi_tmds_word(blue);",
    "g_fbdvi_palette_words[index][1] = fbdvi_tmds_word(green);",
    "g_fbdvi_palette_words[index][2] = fbdvi_tmds_word(red);",
]

DRIVER_SCANOUT_MARKERS = [
    "active_line -= FBDVI_OUTPUT_TOP_BORDER;",
    "return active_line / FBDVI_OUTPUT_SCALE_Y;",
    "src = __fb_gfx->framebuffer + ((size_t)source_y * (size_t)__fb_gfx->pitch);",
    "g_fbdvi_line_buffer[buffer][0][x] = g_fbdvi_palette_words[index][0];",
    "g_fbdvi_line_buffer[buffer][1][x] = g_fbdvi_palette_words[index][1];",
    "g_fbdvi_line_buffer[buffer][2][x] = g_fbdvi_palette_words[index][2];",
]

DRIVER_PIO_MARKERS = [
    "virtual_pin = gpio - FBDVI_PIO_GPIOBASE;",
    "pin_mask = 3u << virtual_pin;",
    "pin_values = 2u << virtual_pin;",
    "putreg32(FBDVI_PIO_GPIOBASE, RP23XX_PIO_GPIOBASE(FBDVI_PIO));",
    "rp23xx_sm_config_set_sideset_pins(&config, virtual_pin);",
]

VENDOR_MARKERS = {
    "apps/hello_dvi/main.c": [
        "set_sys_clock_khz(DVI_TIMING.bit_clk_khz, true);",
        "pio_set_gpio_base(DVI_DEFAULT_SERIAL_CONFIG.pio,16);",
        "dvi0.ser_cfg = DVI_DEFAULT_SERIAL_CONFIG;",
    ],
    "libdvi/dvi_serialiser.pio": [
        "pio_sm_set_pins_with_mask64(pio, sm, 2ULL << data_pins, 3ULL << data_pins);",
        "pio_sm_set_pindirs_with_mask64(pio, sm, ~0ULL, 3ULL << data_pins);",
        "sm_config_set_sideset_pins(&c, data_pins);",
        "sm_config_set_out_shift(&c, true, !debug, 10 * DVI_SYMBOLS_PER_WORD);",
        "sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);",
    ],
    "libdvi/dvi_serialiser.c": [
        "uint slice = pwm_gpio_to_slice_num(cfg->pins_clk);",
        "pwm_config_set_output_polarity(&pwm_cfg, true, false);",
        "pwm_config_set_wrap(&pwm_cfg, 9);",
        "pwm_set_both_levels(slice, 5, 5);",
    ],
}

VENDOR_TIMING_640X480_MARKERS = [
    ".h_sync_polarity   = false,",
    ".h_front_porch     = 16,",
    ".h_sync_width      = 96,",
    ".h_back_porch      = 48,",
    ".h_active_pixels   = 640,",
    ".v_sync_polarity   = false,",
    ".v_front_porch     = 10,",
    ".v_sync_width      = 2,",
    ".v_back_porch      = 33,",
    ".v_active_lines    = 480,",
    ".bit_clk_khz       = 252000",
]


def read_text(path):
    try:
        return path.read_text(encoding="utf-8")
    except OSError as ex:
        raise SystemExit(f"cannot read {path}: {ex}") from ex


def normalize_value(value):
    return re.sub(r"\s+", " ", value.strip())


def define_value(text, name):
    pattern = re.compile(r"^[ \t]*#define[ \t]+" + re.escape(name) +
        r"[ \t]+(.+?)$", re.MULTILINE)
    match = pattern.search(text)

    if match is None:
        raise SystemExit(f"missing #define {name}")

    return normalize_value(match.group(1))


def define_int(text, name):
    value = define_value(text, name)
    value = re.sub(r"[uUlL]+$", "", value)

    try:
        return int(value, 0)
    except ValueError as ex:
        raise SystemExit(f"#define {name} is not an integer: {value}") from ex


def table_values(text, name):
    pattern = re.compile(
        re.escape(name) + r"\s*\[[^\]]*\]\s*=\s*\{(?P<body>.*?)\};",
        re.DOTALL,
    )
    match = pattern.search(text)

    if match is None:
        raise SystemExit(f"missing table {name}")

    body = re.sub(r"/\*.*?\*/", "", match.group("body"), flags=re.DOTALL)
    body = re.sub(r"//.*", "", body)
    return [normalize_value(item) for item in body.split(",") if item.strip()]


def struct_values(text, name):
    pattern = re.compile(
        re.escape(name) + r"\s*=\s*\{(?P<body>.*?)\};",
        re.DOTALL,
    )
    match = pattern.search(text)

    if match is None:
        raise SystemExit(f"missing struct initializer {name}")

    body = re.sub(r"/\*.*?\*/", "", match.group("body"), flags=re.DOTALL)
    body = re.sub(r"//.*", "", body)
    return [normalize_value(item) for item in body.split(",") if item.strip()]


def active_solid_words(driver_text):
    tmds_pair_table = table_values(driver_text, DRIVER_TMDS_PAIR_TABLE)
    red, green, blue = ACTIVE_SOLID_RGB

    return [
        tmds_pair_table[blue >> 2],
        tmds_pair_table[green >> 2],
        tmds_pair_table[red >> 2],
    ]


def rp2350_pwm_slice_for_gpio(gpio):
    """
    Mirror the RP2350 branch of Raspberry Pi's Pico SDK
    pwm_gpio_to_slice_num() helper. GPIO32 and above use the four extra PWM
    slices, while lower pins keep the eight-slice RP2040-style mapping.
    """
    if gpio >= 32:
        return 8 + ((gpio >> 1) & 0x03)

    return (gpio >> 1) & 0x07


def source_y_from_active_line(driver_text, active_line):
    top_border = define_int(driver_text, "FBDVI_OUTPUT_TOP_BORDER")
    scale_y = define_int(driver_text, "FBDVI_OUTPUT_SCALE_Y")
    fb_height = define_int(driver_text, "FBDVI_FRAMEBUFFER_HEIGHT")

    active_line -= top_border

    if active_line < 0:
        return None

    if active_line >= fb_height * scale_y:
        return None

    return active_line // scale_y


def check_scanout_model(driver_text, smoke_text):
    ok = True
    width = define_int(driver_text, "FBDVI_FRAMEBUFFER_WIDTH")
    active_words = define_int(driver_text, "FBDVI_ACTIVE_WORDS")
    top_border = define_int(driver_text, "FBDVI_OUTPUT_TOP_BORDER")
    scale_y = define_int(driver_text, "FBDVI_OUTPUT_SCALE_Y")
    fb_height = define_int(driver_text, "FBDVI_FRAMEBUFFER_HEIGHT")
    timing = struct_values(driver_text, "g_fbdvi_timing_640x480")
    active_lines = int(timing[9])

    if active_words != width:
        print("nuttx-dvi-parity: active DVI words do not match framebuffer width",
            file=sys.stderr)
        ok = False

    if top_border + (fb_height * scale_y) > active_lines:
        print("nuttx-dvi-parity: scaled framebuffer does not fit active field",
            file=sys.stderr)
        ok = False

    for marker in DRIVER_SCANOUT_MARKERS:
        if marker not in driver_text:
            print(f"nuttx-dvi-parity: missing scanout marker: {marker}",
                file=sys.stderr)
            ok = False

    smoke_active_solid = table_values(smoke_text, ACTIVE_SOLID_TABLE)
    driver_active_solid = active_solid_words(driver_text)
    black_words = [table_values(driver_text, DRIVER_TMDS_PAIR_TABLE)[0]] * 3

    if driver_active_solid != smoke_active_solid:
        return False

    model_points = [
        (top_border - 1, None, black_words),
        (top_border, 0, smoke_active_solid),
        (top_border + 1, 0, smoke_active_solid),
        (top_border + ((fb_height - 1) * scale_y), fb_height - 1,
            smoke_active_solid),
        (top_border + (fb_height * scale_y), None, black_words),
    ]

    for active_line, expected_y, expected_words in model_points:
        source_y = source_y_from_active_line(driver_text, active_line)

        if source_y != expected_y:
            print(
                "nuttx-dvi-parity: active line %d maps to %s, expected %s" %
                (active_line, source_y, expected_y),
                file=sys.stderr,
            )
            ok = False

        if expected_y is not None and expected_words != smoke_active_solid:
            print("nuttx-dvi-parity: active model does not match smoke TMDS",
                file=sys.stderr)
            ok = False

    return ok


def compare(label, driver_value, smoke_value):
    if driver_value != smoke_value:
        print(f"nuttx-dvi-parity: mismatch {label}", file=sys.stderr)
        print(f"  driver: {driver_value}", file=sys.stderr)
        print(f"  smoke:  {smoke_value}", file=sys.stderr)
        return False

    return True


def check_driver_markers(driver_text):
    ok = True

    for marker in DRIVER_ENCODER_MARKERS:
        if marker not in driver_text:
            print(f"nuttx-dvi-parity: missing driver encoder marker: {marker}",
                file=sys.stderr)
            ok = False

    for marker in DRIVER_PIO_MARKERS:
        if marker not in driver_text:
            print(f"nuttx-dvi-parity: missing driver PIO marker: {marker}",
                file=sys.stderr)
            ok = False

    return ok


def uint32_table_is_writable_static(text, name):
    pattern = re.compile(
        r"static\s+(?P<const>const\s+)?uint32_t\s+" +
        re.escape(name) + r"\s*\["
    )
    matches = list(pattern.finditer(text))

    if not matches:
        raise SystemExit(f"missing uint32_t table declaration {name}")

    return matches[-1].group("const") is None


def check_dma_source_storage(driver_text, smoke_text):
    ok = True
    checks = [
        ("driver control symbols", driver_text, "g_fbdvi_ctrl_symbols"),
        ("smoke control symbols", smoke_text, "g_fbdvi_ctrl_symbols"),
        ("smoke active solid words", smoke_text, ACTIVE_SOLID_TABLE),
    ]

    for label, text, name in checks:
        if not uint32_table_is_writable_static(text, name):
            print(
                "nuttx-dvi-parity: DMA source table is const/rodata: "
                f"{label}",
                file=sys.stderr,
            )
            ok = False

    return ok


def read_vendor_text(relative_path):
    return read_text(KNOWN_GOOD_DVI / relative_path)


def vendor_pico_sock_config(text):
    pattern = re.compile(
        r"pico_sock_cfg\s*=\s*\{(?P<body>.*?)\};",
        re.DOTALL,
    )
    match = pattern.search(text)

    if match is None:
        raise SystemExit("known-good DVI source is missing pico_sock_cfg")

    body = match.group("body")

    pins_match = re.search(r"\.pins_tmds\s*=\s*\{(?P<pins>[^}]+)\}", body)
    clk_match = re.search(r"\.pins_clk\s*=\s*(?P<clk>\d+)", body)
    invert_match = re.search(
        r"\.invert_diffpairs\s*=\s*(?P<invert>true|false)", body)

    if pins_match is None or clk_match is None or invert_match is None:
        raise SystemExit("known-good DVI source has incomplete pico_sock_cfg")

    pins = [
        int(item.strip(), 0)
        for item in pins_match.group("pins").split(",")
        if item.strip()
    ]

    return pins, int(clk_match.group("clk"), 0), invert_match.group("invert")


def vendor_timing_block(text):
    pattern = re.compile(
        r"dvi_timing_640x480p_60hz\)\s*=\s*\{(?P<body>.*?)\};",
        re.DOTALL,
    )
    match = pattern.search(text)

    if match is None:
        raise SystemExit("known-good DVI source is missing 640x480 timing")

    return match.group("body")


def check_known_good_vendor(driver_text):
    ok = True

    if not KNOWN_GOOD_DVI.is_dir():
        return True

    pin_text = read_vendor_text("include/common_dvi_pin_configs.h")
    pins, clock_pin, invert = vendor_pico_sock_config(pin_text)
    driver_pins = [
        define_int(driver_text, "FBDVI_GPIO_BLUE"),
        define_int(driver_text, "FBDVI_GPIO_GREEN"),
        define_int(driver_text, "FBDVI_GPIO_RED"),
    ]
    driver_clock = define_int(driver_text, "FBDVI_GPIO_CLOCK")

    ok = compare("known-good TMDS pins", driver_pins, pins) and ok
    ok = compare("known-good clock pin", driver_clock, clock_pin) and ok
    ok = compare("known-good differential inversion", "false", invert) and ok

    expected_slice = rp2350_pwm_slice_for_gpio(driver_clock)
    ok = compare("RP2350 PWM clock slice",
        define_int(driver_text, "FBDVI_PWM_CLOCK_SLICE"),
        expected_slice) and ok

    for relative_path, markers in VENDOR_MARKERS.items():
        text = read_vendor_text(relative_path)
        for marker in markers:
            if marker not in text:
                print(
                    "nuttx-dvi-parity: known-good source missing marker "
                    f"{relative_path}: {marker}",
                    file=sys.stderr,
                )
                ok = False

    timing_text = vendor_timing_block(read_vendor_text("libdvi/dvi_timing.c"))
    for marker in VENDOR_TIMING_640X480_MARKERS:
        if marker not in timing_text:
            print(
                "nuttx-dvi-parity: known-good 640x480 timing missing marker: "
                f"{marker}",
                file=sys.stderr,
            )
            ok = False

    return ok


def run_checks(driver_text, smoke_text):
    ok = True

    for name in DEFINES:
        ok = compare(name, define_value(driver_text, name),
            define_value(smoke_text, name)) and ok

    for name in TABLES:
        if name == "g_fbdvi_timing_640x480":
            driver_value = struct_values(driver_text, name)
            smoke_value = struct_values(smoke_text, name)
        else:
            driver_value = table_values(driver_text, name)
            smoke_value = table_values(smoke_text, name)

        ok = compare(name, driver_value, smoke_value) and ok

    smoke_active_solid = table_values(smoke_text, ACTIVE_SOLID_TABLE)
    ok = compare(ACTIVE_SOLID_TABLE, active_solid_words(driver_text),
        smoke_active_solid) and ok

    ok = check_driver_markers(driver_text) and ok
    ok = check_dma_source_storage(driver_text, smoke_text) and ok
    ok = check_scanout_model(driver_text, smoke_text) and ok
    vendor_source_present = KNOWN_GOOD_DVI.is_dir()
    ok = check_known_good_vendor(driver_text) and ok

    if not ok:
        return 1

    print("nuttx-dvi-parity: gfx driver matches standalone DVI smoke constants")
    print("nuttx-dvi-parity: standalone solid field matches reference TMDS words")
    print("nuttx-dvi-parity: gfx framebuffer scanout model matches solid DVI smoke")
    if vendor_source_present:
        print("nuttx-dvi-parity: known-good Waveshare/PicoDVI source comparison passed")
    else:
        print("nuttx-dvi-parity: known-good Waveshare/PicoDVI source comparison skipped")
    return 0


def quiet_run_checks(driver_text, smoke_text):
    stdout = io.StringIO()
    stderr = io.StringIO()

    with contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
        result = run_checks(driver_text, smoke_text)

    return result


def replace_once(text, old, new):
    changed = text.replace(old, new, 1)

    if changed == text:
        raise SystemExit(f"self-test could not find mutation source: {old}")

    return changed


def run_self_test(driver_text, smoke_text):
    mutations = [
        (
            "pin mapping",
            replace_once(driver_text, "#define FBDVI_GPIO_RED 32",
                "#define FBDVI_GPIO_RED 33"),
            smoke_text,
        ),
        (
            "scanout scaling",
            replace_once(driver_text, "#define FBDVI_OUTPUT_SCALE_Y 2",
                "#define FBDVI_OUTPUT_SCALE_Y 3"),
            smoke_text,
        ),
        (
            "solid-field TMDS word",
            driver_text,
            replace_once(smoke_text, "0x000bfa01", "0x000bfa00"),
        ),
        (
            "DMA source storage",
            replace_once(driver_text, "static uint32_t g_fbdvi_ctrl_symbols",
                "static const uint32_t g_fbdvi_ctrl_symbols"),
            smoke_text,
        ),
    ]

    for label, mutated_driver, mutated_smoke in mutations:
        if quiet_run_checks(mutated_driver, mutated_smoke) == 0:
            print(f"nuttx-dvi-parity: self-test failed to catch {label}",
                file=sys.stderr)
            return 1

    print("nuttx-dvi-parity: self-test mutation checks passed")
    return 0


def parse_args(argv):
    parser = argparse.ArgumentParser(
        description="Check RP2350 DVI smoke and gfxlib scanout parity."
    )
    parser.add_argument(
        "--self-test",
        action="store_true",
        help="also prove the checker catches deliberate in-memory drift",
    )

    return parser.parse_args(argv)


def main(argv):
    args = parse_args(argv)
    driver_text = read_text(DRIVER)
    smoke_text = read_text(SMOKE)
    result = run_checks(driver_text, smoke_text)

    if result != 0:
        return result

    if args.self_test:
        return run_self_test(driver_text, smoke_text)

    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))

# end of nuttx-rp2350-dvi-parity-check.py
