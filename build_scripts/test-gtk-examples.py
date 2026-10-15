#!/usr/bin/env python3
"""FreeBASIC GTK example qualification
File: test-gtk-examples.py
Purpose: Exercise original GTK2, GTK3 and GooCanvas windows on native Linux.
Responsibilities: Stage assets, inspect owned windows, close normally and retain receipts.
This file contains no widget replacement, global desktop input or physical display claim.
"""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import time
from PIL import Image, ImageChops
from sdl3_xvfb import private_display


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fbc", type=Path, required=True)
    parser.add_argument("--sdk-prefix", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--select", action="append", help="Path relative to examples/GUI/GTK+")
    args = parser.parse_args()
    if not os.sys.platform.startswith("linux"):
        parser.error("Native Linux is required")
    root = Path(__file__).resolve().parents[1]
    base = root / "examples/GUI/GTK+"
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    sources = [p for p in sorted(base.rglob("*.bas")) if "tobac" not in p.parts]
    if args.select:
        sources = [p for p in sources if p.relative_to(base).as_posix() in args.select]
    if not sources:
        parser.error("No examples selected")
    fingerprint = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
    pins = [Path(__file__).resolve(), root / "build_scripts/sdl3_xvfb.py",
            root / "build_scripts/test-sdl12-examples.py", *sources]
    records = []
    receipt = {"status": "pending", "records": records,
               "compiler_sha256": fingerprint(args.fbc.resolve()),
               "source_sha256": {p.relative_to(root).as_posix(): fingerprint(p) for p in pins},
               "scope": "GTK windows on private Xvfb; no physical display claim"}
    def save():
        (output / "results.json").write_text(json.dumps(receipt, indent=2) + "\n")
    save()
    spec = importlib.util.spec_from_file_location("gtk_gui", root / "build_scripts/test-sdl12-examples.py")
    gui = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(gui)
    environment = os.environ.copy()
    environment.pop("LD_PRELOAD", None)
    environment["LIBGL_ALWAYS_SOFTWARE"] = "1"
    # The fixture has no accessibility service. Do not start desktop services
    # merely to inspect the example's own rendering and WM_DELETE_WINDOW path.
    environment["NO_AT_BRIDGE"] = "1"
    library_flags = []
    if args.sdk_prefix:
        lib = args.sdk_prefix.resolve() / "lib"
        library_flags = ["-p", str(lib)]
        environment["LD_LIBRARY_PATH"] = str(lib) + ":" + environment.get("LD_LIBRARY_PATH", "")
    for backend in ("gcc", "gas64"):
        for source in sources:
            relative = source.relative_to(base)
            directory = output / backend / relative.parent
            directory.mkdir(parents=True, exist_ok=True)
            for asset in source.parent.iterdir():
                if asset.is_file() and asset.suffix.lower() not in (".bas", ".bi"):
                    shutil.copy2(asset, directory / asset.name)
            binary = directory / source.stem
            result = subprocess.run([str(args.fbc.resolve()), "-prefix", str(root), "-i", str(root / "inc"),
                                     *library_flags, "-gen", backend, str(source), "-x", str(binary)],
                                    capture_output=True, text=True, timeout=45)
            (directory / (source.stem + "-compile.log")).write_text(result.stdout + result.stderr)
            if result.returncode:
                records.append({"source": source.relative_to(root).as_posix(), "backend": backend,
                                "status": "fail", "error": "compile exit " + str(result.returncode)})
                save()
                continue
            cases = [("default", [])]
            if source.name == "packbox.bas":
                cases = [("packing-" + str(n), [str(n)]) for n in (1, 2, 3)]
            if source.name == "gtk_applicationE110.bas":
                fixture = directory / "bloatpad-input.txt"
                fixture.write_text("FreeBASIC GTK application file-open fixture\n")
                cases = [("file-open", [str(fixture)])]
            for case, arguments in cases:
                tag = source.stem + "-" + case
                logpath = directory / (tag + ".log")
                record = {"source": source.relative_to(root).as_posix(), "backend": backend,
                          "case": case, "status": "pending"}
                records.append(record)
                save()
                try:
                    with private_display(directory, environment) as (session, display):
                        record["display"] = display
                        with logpath.open("w") as log:
                            child = subprocess.Popen([str(binary), *arguments], cwd=directory, env=session,
                                                     stdout=log, stderr=subprocess.STDOUT, start_new_session=True)
                            try:
                                deadline = time.monotonic() + 8
                                window = None
                                while child.poll() is None and time.monotonic() < deadline:
                                    found = gui.command(["xdotool", "search", "--onlyvisible", "--pid", str(child.pid)], session)
                                    if found.returncode == 0 and found.stdout.strip():
                                        window = int(found.stdout.splitlines()[0])
                                        break
                                    time.sleep(.05)
                                if window is None:
                                    raise RuntimeError("No visible window; exit=" + str(child.poll()))
                                time.sleep(.3)
                                capture = gui.capture(window, directory / (tag + ".png"), session)
                                assert capture["width"] > 10 and capture["height"] > 10, capture
                                # These two examples intentionally contain only a blank
                                # window or a uniform drawing area before user input.
                                if source.name not in ("base.bas", "colorsel.bas"):
                                    assert capture["colors"] != 1, capture
                                if source.name == "demo-focus.bas":
                                    gui.input_command(["windowfocus", str(window)], session)
                                    gui.input_command(["key", "Tab", "Tab"], session)
                                    time.sleep(.2)
                                    record["keyboard_focus"] = True
                                if source.name == "demo-arrowhead.bas":
                                    # Coordinates are inside the fixed 640x600 example,
                                    # on the line-width handle in its 500x350 canvas.
                                    gui.input_command(["mousemove", "--window", str(window), "120", "292"], session)
                                    gui.input_command(["mousedown", "1"], session)
                                    gui.input_command(["mousemove", "--window", str(window), "120", "282"], session)
                                    gui.input_command(["mouseup", "1"], session)
                                    time.sleep(.2)
                                    changed = directory / (tag + "-drag.png")
                                    gui.capture(window, changed, session)
                                    with Image.open(directory / (tag + ".png")) as before, Image.open(changed) as after:
                                        assert ImageChops.difference(before.convert("RGB"), after.convert("RGB")).getbbox(), "Arrow drag did not change the rendering"
                                    record["width_drag"] = True
                                gui.request_close(window, session)
                                code = child.wait(timeout=4)
                                if code:
                                    raise RuntimeError("Normal close exit " + str(code))
                                record.update(status="pass", capture=capture, normal_close=True)
                            finally:
                                gui.stop(child)
                    assert display["stopped"]
                    text = logpath.read_text()
                    if source.name == "demo-focus.bas":
                        assert "received focus-in event" in text, "Canvas item did not receive keyboard focus"
                    if any(marker in text for marker in ("CRITICAL", "Segmentation fault", "Unable to create", "undefined symbol")):
                        raise RuntimeError("Native SDK diagnostic: " + text[-1500:])
                except Exception as error:
                    record.update(status="fail", error=str(error),
                                  log=str(logpath))
                save()
                print(backend + " " + str(relative) + " " + case + " " + record["status"], flush=True)
    receipt["status"] = "pass" if all(r["status"] == "pass" for r in records) else "failed"
    save()
    print("GTK_EXAMPLES=" + receipt["status"].upper() + " checks=" + str(len(records)), flush=True)
    return 0 if receipt["status"] == "pass" else 1


if __name__ == "__main__":
    raise SystemExit(main())
# end of test-gtk-examples.py
