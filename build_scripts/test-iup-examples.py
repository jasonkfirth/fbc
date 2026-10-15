#!/usr/bin/env python3
"""FreeBASIC IUP native Linux example qualification
File: test-iup-examples.py
Purpose: Exercise genuine GTK3 IUP dialogs and initialization failures.
Responsibilities: Current-source builds, isolated displays, callbacks and normal exit.
This file contains no widget replacement or global desktop input.
"""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import time
from PIL import Image
from sdl3_xvfb import private_display


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("fbc", "sdk-prefix", "output"):
        parser.add_argument("--" + name, type=Path, required=True)
    args = parser.parse_args()
    if not os.sys.platform.startswith("linux"):
        parser.error("Native Linux is required")
    root = Path(__file__).resolve().parents[1]
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    library = args.sdk_prefix.resolve() / "lib/libiup.so"
    sources = sorted((root / "examples/GUI/IUP").glob("*.bas"))
    pins = [Path(__file__).resolve(), root / "inc/IUP/iup.bi", *sources]
    records = []
    receipt = {"status": "pending", "records": records,
               "compiler_sha256": hashlib.sha256(args.fbc.resolve().read_bytes()).hexdigest(),
               "library_sha256": hashlib.sha256(library.read_bytes()).hexdigest(),
               "source_sha256": {p.relative_to(root).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest() for p in pins}}
    def save():
        (output / "results.json").write_text(json.dumps(receipt, indent=2) + "\n")
    save()
    spec = importlib.util.spec_from_file_location("iup_gui", root / "build_scripts/test-sdl12-examples.py")
    gui = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(gui)
    environment = os.environ.copy()
    environment.pop("LD_PRELOAD", None)
    environment["LD_LIBRARY_PATH"] = str(library.parent) + ":" + environment.get("LD_LIBRARY_PATH", "")
    environment["NO_AT_BRIDGE"] = "1"
    environment["GDK_BACKEND"] = "x11"
    titles = {"buttons": "IupButton", "color": "IupColor", "frame": "IupFrame",
              "message": "IupMessage Example", "textbox": "Text box example"}
    for backend in ("gcc", "gas64"):
        for source in sources:
            directory = output / backend / source.stem
            directory.mkdir(parents=True, exist_ok=True)
            binary = directory / "example"
            result = subprocess.run([str(args.fbc.resolve()), "-prefix", str(root), "-i", str(root / "inc"),
                                     "-p", str(library.parent), "-d", "__USE_GTK3__", "-gen", backend,
                                     str(source), "-x", str(binary)], capture_output=True, text=True, timeout=30)
            (directory / "compile.log").write_text(result.stdout + result.stderr)
            if result.returncode:
                raise RuntimeError(result.stdout + result.stderr)
            absent = environment.copy()
            absent.pop("DISPLAY", None)
            absent.pop("WAYLAND_DISPLAY", None)
            absent.pop("XAUTHORITY", None)
            result = subprocess.run([str(binary)], env=absent, capture_output=True, text=True, timeout=5)
            assert result.returncode == 1 and "Could not initialize IUP" in result.stdout, (source, result.returncode, result.stdout, result.stderr)
            records.append({"source": source.relative_to(root).as_posix(), "backend": backend,
                            "case": "no-display", "status": "pass", "exit_code": result.returncode})
            logpath = directory / "native.log"
            with private_display(directory, environment) as (session, display):
                with logpath.open("w") as log:
                    child = subprocess.Popen([str(binary)], cwd=directory, env=session, stdout=log,
                                             stderr=subprocess.STDOUT, start_new_session=True)
                    def window(title, seconds=8):
                        deadline = time.monotonic() + seconds
                        while child.poll() is None and time.monotonic() < deadline:
                            found = gui.command(["xdotool", "search", "--onlyvisible", "--pid", str(child.pid),
                                                 "--name", "^" + title + "$"], session)
                            if found.returncode == 0 and found.stdout.strip():
                                return int(found.stdout.splitlines()[0])
                            time.sleep(.05)
                        raise RuntimeError("No owned window: " + title)
                    try:
                        main_window = window(titles[source.stem])
                        time.sleep(.2)
                        capture = gui.capture(main_window, directory / "initial.png", session)
                        assert capture["width"] > 20 and capture["height"] > 20 and capture["colors"] != 1, capture
                        interactions = []
                        def click(x, y):
                            gui.input_command(["mousemove", "--window", str(main_window), str(x), str(y)], session)
                            gui.input_command(["click", "1"], session)
                        def enter_text(x, y, value):
                            click(x, y)
                            gui.input_command(["key", "ctrl+a"], session)
                            gui.input_command(["key", "BackSpace"], session)
                            if value:
                                gui.input_command(["type", "--clearmodifiers", "--", value], session)
                        if source.stem == "buttons":
                            gui.input_command(["windowfocus", str(main_window)], session)
                            gui.input_command(["key", "Return"], session)
                            modal = window("IupMessage")
                            gui.capture(modal, directory / "modal.png", session)
                            gui.input_command(["windowfocus", str(modal)], session)
                            gui.input_command(["key", "Return"], session)
                            window(titles[source.stem])
                            time.sleep(.1)
                            click(capture["width"] * 3 // 4, capture["height"] // 2)
                            interactions.append("modal open/dismiss and Close callback")
                        elif source.stem == "textbox":
                            for index, value in enumerate(("123.5", "", "words with spaces")):
                                enter_text(100, 18, value)
                                click(100, 49)
                                marker = "OK clicked! content of textbox: <" + value + ">"
                                gui.wait_message(child, logpath, marker, 3)
                                gui.capture(main_window, directory / ("text-" + str(index) + ".png"), session)
                                interactions.append("text callback: " + repr(value))
                            gui.request_close(main_window, session)
                        elif source.stem == "color":
                            cases = ((("255", "128", "0"), (255, 128, 0)),
                                     (("-1", "999", "-2"), (0, 0, 0)),
                                     (("0", "255", "255"), (0, 255, 255)))
                            for index, (values, expected) in enumerate(cases):
                                for y, value in zip((31, 58, 85), values):
                                    enter_text(77, y, value)
                                click(57, 140)
                                time.sleep(.15)
                                target = directory / ("rgb-" + str(index) + ".png")
                                gui.capture(main_window, target, session)
                                with Image.open(target) as image:
                                    # Inspect the interior of the fixed read-only
                                    # colour field, excluding black label glyphs.
                                    assert image.width >= 108 and image.height >= 124
                                    pixels = image.convert("RGB").crop((45, 104, 108, 124)).getdata()
                                    count = sum(all(abs(a-b) <= 2 for a, b in zip(pixel, expected)) for pixel in pixels)
                                assert count > 100, (values, expected, count)
                                interactions.append({"RGB": values, "expected": expected, "matching_pixels": count})
                            gui.request_close(main_window, session)
                        if source.stem == "message":
                            gui.input_command(["windowfocus", str(main_window)], session)
                            gui.input_command(["key", "Return"], session)
                        elif source.stem == "frame":
                            gui.request_close(main_window, session)
                        assert child.wait(timeout=5) == 0, logpath.read_text()
                    finally:
                        gui.stop(child)
            assert display["stopped"]
            text = logpath.read_text()
            assert not any(marker in text for marker in ("CRITICAL", "Segmentation fault", "undefined symbol")), text
            records.append({"source": source.relative_to(root).as_posix(), "backend": backend,
                            "case": "native", "status": "pass", "normal_close": True,
                            "capture": capture, "display": display, "interactions": interactions})
            save()
            print(backend + " " + source.stem + " native/no-display pass", flush=True)
    receipt["status"] = "pass"
    save()
    print("IUP_EXAMPLES=PASS checks=" + str(len(records)), flush=True)


if __name__ == "__main__":
    main()
# end of test-iup-examples.py
