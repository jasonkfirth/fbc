#!/usr/bin/env python3
"""Build declared corpus programs with a staged fbc-js SDK.

Compile isolated sources and retain HTML, JS, Wasm, and build receipts.
Runtime qualification and publication are separate from this driver.
"""
import argparse
import concurrent.futures
import hashlib
import json
import os
import re
import shlex
import shutil
import subprocess
import tempfile
from pathlib import Path


def inside(base, relative):
    path = (base / relative).resolve()
    if not path.is_relative_to(base) or path == base:
        raise ValueError(f"path escapes {base}: {relative}")
    return path


def configure_tools(toolchain):
    toolchain = toolchain.resolve()
    env = os.environ.copy()
    env["EM_CONFIG"] = str(toolchain / "toolchain/ucrt64/lib/emscripten/.emscripten")
    for var, tool in (("EMCC", "emcc"), ("EMAS", "emcc"), ("EMLD", "emcc"), ("EMAR", "emar")):
        cmd = toolchain / "toolchain/cmd-shims" / (tool + ".cmd")
        if not cmd.is_file():
            raise FileNotFoundError(cmd)
        env[var] = str(cmd)
    cache = Path(env["LOCALAPPDATA"]) / "FreeBASIC/fbc-js"
    for var, directory in (("EM_CACHE", "em-cache"), ("EMCC_TEMP_DIR", "emcc-temp")):
        env.setdefault(var, str(cache / directory))
        Path(env[var]).mkdir(parents=True, exist_ok=True)
    env.setdefault("BINARYEN_CORES", "1")
    return env


def digest(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def graphics_keyboard_flags(toolchain, work):
    settings = toolchain / "toolchain/ucrt64/lib/emscripten/src/settings.js"
    defaults = re.search(r"var INCOMING_MODULE_JS_API\s*=\s*\[(.*?)\];",
                         settings.read_text(encoding="utf-8"), re.DOTALL)
    names = re.findall(r"'(\w+)'", defaults.group(1)) if defaults else []
    if not {"preRun", "canvas", "onExit"}.issubset(names):
        raise ValueError("could not read Emscripten's default incoming Module API")
    if "doNotCaptureKeyboard" not in names:
        names.append("doNotCaptureKeyboard")
    # gfxlib2 owns BASIC key events. SDL's JS keydown listener suppresses
    # the browser's printable keypress unless the linker retains this hook.
    # A response file carries the list past fbc's comma-separated -Wl parser.
    flags = work / "gfx-keyboard.rsp"
    flags.write_text("-sINCOMING_MODULE_JS_API=" + ",".join(names) + "\n", encoding="ascii")
    return ["-Wl", '@"' + str(flags) + '"']


def stage_asset(project, temp, relative, sources):
    origin = inside(project, relative)
    dest = inside(temp, relative)
    dest.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(origin, dest)
    sources[relative] = digest(dest)
    return dest


def compile_program(command, directory, env, log, timeout):
    with subprocess.Popen(command, cwd=directory, env=env, stdout=log, stderr=subprocess.STDOUT) as compiler:
        try:
            return compiler.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            # On Windows, killing only fbc leaves emcc and its children
            # holding files open in the staging directory.
            if os.name == "nt":
                try:
                    subprocess.run(["taskkill.exe", "/pid", str(compiler.pid), "/t", "/f"],
                                   stdout=log, stderr=subprocess.STDOUT, timeout=10)
                except (OSError, subprocess.TimeoutExpired) as error:
                    log.write(f"Could not stop the compiler tree: {error}\n")
            if compiler.poll() is None:
                compiler.kill()
            compiler.wait()
            return None


def run_build(case, args, env):
    slug = case["slug"]
    if not re.fullmatch(r"[a-z0-9]+(?:-[a-z0-9]+)*", slug):
        raise ValueError(f"invalid slug: {slug}")
    timeout = args.timeout if args.timeout is not None else case.get("timeout", 150)
    if type(timeout) is not int or timeout < 1:
        raise ValueError(f"invalid compile timeout for {slug}")
    project = inside(args.corpus, case["project"])
    output = args.output
    row = {key: case[key] for key in ("slug", "project", "entry")}
    git = ["git", "-C", str(project)]
    top = subprocess.run([*git, "rev-parse", "--show-toplevel"], capture_output=True, text=True)
    if top.returncode == 0 and Path(top.stdout.strip()).resolve() == project:
        for key, command in (("revision", ["rev-parse", "HEAD"]), ("origin", ["remote", "get-url", "origin"])):
            info = subprocess.run([*git, *command], capture_output=True, text=True)
            if info.returncode == 0:
                row[key] = info.stdout.strip()
    if source_origin := case.get("source_origin"):
        row.setdefault("origin", source_origin)
        row.setdefault("revision", "Unversioned corpus snapshot")
    with tempfile.TemporaryDirectory(prefix=slug + "-", dir=output / "work") as work:
        temp = Path(work).resolve()
        row["sources"] = {}
        units = set()
        excluded = {inside(project, relative) for relative in case.get("exclude_files", [])}
        patterns = set(case.get("files", [])) | set(case.get("units", [])) | set(case.get("notices", []))
        if case.get("adapter"):
            # A browser entry point can live beside the build driver. Keep it
            # in the build receipt and leave the upstream checkout alone.
            if not re.fullmatch(r"fb-corpus-[a-z0-9-]+\.bas", case["entry"]):
                raise ValueError("invalid browser adapter name")
            adapter = inside(Path(__file__).resolve().parent, case["entry"])
            target = inside(temp, case["entry"])
            shutil.copyfile(adapter, target)
            row["sources"][case["entry"]] = digest(target)
        else:
            patterns.add(case["entry"])
        for pattern in patterns:
            files = [file for file in project.glob(pattern) if file.is_file() and file.resolve() not in excluded]
            if not files:
                raise ValueError(f"no source matched {project.name}/{pattern}")
            for file in files:
                relative = file.relative_to(project)
                target = inside(temp, relative)
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(inside(project, relative), target)
                row["sources"][relative.as_posix()] = digest(target)
                if pattern in case.get("units", []) and relative.as_posix() != case["entry"]:
                    units.add(target)
        if prefix := case.get("preamble"):
            entry = inside(temp, case["entry"])
            entry.write_bytes(prefix.encode("ascii") + entry.read_bytes())
        entry = inside(temp, case["entry"])
        include_flags = []
        for relative in case.get("include_dirs", []):
            include = inside(temp, relative)
            if not include.is_dir():
                raise ValueError(f"include directory was not staged: {relative}")
            include_flags += ["-i", str(include)]
        cmd = [str(args.compiler), str(entry), *(str(unit) for unit in sorted(units)), "-x", str(temp / "index.html"),
               "-Wl", "-sEXIT_RUNTIME=1", *include_flags]
        archive = None
        archive_command = None
        if spec := case.get("static_archive"):
            name, sources = spec
            if not re.fullmatch(r"[a-z0-9_]+", name) or not sources or any(source not in row["sources"] for source in sources):
                raise ValueError("the static archive requires a name and staged BASIC sources")
            archive = temp / f"lib{name}.a"
            archive_command = [str(args.compiler), "-lib", "-x", str(archive), *include_flags,
                               *(str(inside(temp, source)) for source in sources)]
            cmd += ["-p", str(temp)]
        if case.get("graphics_keyboard"):
            cmd += graphics_keyboard_flags(args.toolchain, temp)
        cmd += case.get("flags", [])
        if "workspace_aliases" in case:
            aliases = case["workspace_aliases"]
            if not isinstance(aliases, list) or any(not isinstance(path, str) or not re.fullmatch(r"/[a-z]+", path)
                                                    for path in aliases) or len(set(aliases)) != len(aliases):
                raise ValueError(f"invalid workspace aliases: {aliases}")
            sample = case.get("workspace_sample")
            if sample is not None and (not isinstance(sample, str) or not re.fullmatch(r"[\w-]+\.[\w-]+", sample)
                                       or "/browser-sample/" + sample not in case.get("assets", {}).values()):
                raise ValueError(f"invalid workspace sample: {sample}")
            config = json.dumps({"root": f"/fb-corpus-{slug}", "aliases": aliases, "sample": sample})
            template = Path(__file__).with_name("fb-corpus-workspace.js").read_text(encoding="utf-8")
            script = temp / "workspace.js"
            script.write_text(template.replace("/* workspace configuration */ null", config), encoding="utf-8")
            response = temp / "workspace.rsp"
            response.write_text(shlex.join(["--pre-js", script.as_posix(), "-lidbfs.js"]) + "\n", encoding="utf-8")
            cmd += ["-Wl", '@"' + str(response) + '"']
        # Emscripten packs generated and tracked assets into the same mode.
        # Explicit asset lists can use a .data file without requiring Git.
        asset_mode = case.get("asset_mode", "preload" if case.get("preload") else "embed")
        if asset_mode not in ("embed", "preload") or (asset_mode == "embed" and case.get("preload")):
            raise ValueError(f"incompatible asset mode for {slug}: {asset_mode}")
        asset_flag = "--preload-file" if asset_mode == "preload" else "--embed-file"
        asset_args = []
        for asset, virtual in case.get("assets", {}).items():
            path = inside(project, asset)
            if not path.is_file() or not virtual.startswith("/") or ".." in virtual.split("/"):
                raise ValueError(f"invalid asset: {asset}")
            embedded = stage_asset(project, temp, asset, row["sources"])
            asset_args += [asset_flag, embedded.as_posix() + "@" + virtual]
        for asset, virtual in case.get("preload", {}).items():
            directory = inside(project, asset)
            if not directory.is_dir() or not virtual.startswith("/") or ".." in virtual.split("/"):
                raise ValueError(f"invalid preloaded directory: {asset}")
            # Only tracked data from a declared directory belongs in a pack.
            # A separate .data file keeps large graphics and maps out of Wasm.
            tracked = subprocess.check_output(["git", "-C", str(project), "ls-files", "-z", "--", asset + "/"])
            members = tracked.decode("utf-8").split("\0")[:-1]
            if not members:
                raise ValueError(f"preloaded directory has no tracked files: {asset}")
            for member in members:
                if not inside(project, member).is_relative_to(directory):
                    raise ValueError(f"preloaded file escapes {asset}: {member}")
                stage_asset(project, temp, member, row["sources"])
            asset_args += ["--preload-file", inside(temp, asset).as_posix() + "@" + virtual]
        if asset_args:
            # fbc's -Wl parser splits tokens before Emscripten sees them.
            # Emscripten reads its own response files with shlex.split().
            response = temp / "assets.rsp"
            response.write_text(shlex.join(asset_args) + "\n", encoding="utf-8")
            cmd += ["-Wl", '@"' + str(response) + '"']
        for relative, before, after, *count in case.get("replacements", []):
            # Match the expected number of occurrences so a changed upstream
            # file cannot silently produce a different browser adaptation.
            if relative not in row["sources"]:
                raise ValueError(f"replacement target was not staged: {relative}")
            target = inside(temp, relative)
            source = target.read_bytes().replace(b"\r\n", b"\n")
            old = before.encode("utf-8")
            expected = count[0] if count else 1
            if not old or expected < 1 or source.count(old) != expected:
                raise ValueError(f"replacement in {relative} requires {expected} match(es): {before}")
            target.write_bytes(source.replace(old, after.encode("utf-8"), expected))
        with (output / "logs" / (slug + ".build.log")).open("w", encoding="utf-8") as log:
            if archive_command:
                log.write(json.dumps(archive_command) + "\n")
                log.flush()
                result = compile_program(archive_command, temp, env, log, timeout)
                if result != 0 or not archive.is_file():
                    row.update(status="archive-failed", exit_code=result)
                    return row
            log.write(json.dumps(cmd) + "\n")
            log.flush()
            result = compile_program(cmd, entry.parent, env, log, timeout)
            if result is None:
                row["status"] = "compile-timeout"
            else:
                row["status"] = "linked" if result == 0 else "compile-failed"
                row["exit_code"] = result
        suffixes = ("html", "js", "wasm", "data") if asset_args and asset_mode == "preload" else ("html", "js", "wasm")
        files = [temp / f"index.{suffix}" for suffix in suffixes]
        if archive:
            files.append(archive)
        if row["status"] == "linked":
            if not all(file.is_file() and file.stat().st_size for file in files):
                row["status"] = "missing-output"
            else:
                app = output / "apps" / slug
                app.mkdir(parents=True, exist_ok=True)
                row["artifacts"] = {}
                for file in files:
                    shutil.copyfile(file, app / file.name)
                    row["artifacts"][file.name] = digest(file)
    return row


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for feature in ("corpus", "profile", "output", "compiler", "toolchain"):
        parser.add_argument("--" + feature, type=Path, required=True)
    parser.add_argument("--jobs", type=int, default=3)
    parser.add_argument("--timeout", type=int, help="override the compile timeout in seconds for every selected case")
    parser.add_argument("--only", action="append", default=[])
    args = parser.parse_args()
    for key in ("corpus", "output", "compiler"):
        setattr(args, key, getattr(args, key).resolve())
    if (not args.corpus.is_dir() or not args.compiler.is_file() or args.jobs < 1 or
            (args.timeout is not None and args.timeout < 1)):
        parser.error("invalid corpus directory, compiler or build limits")
    env = configure_tools(args.toolchain)
    profile = json.loads(args.profile.read_text(encoding="utf-8"))
    required = set(args.only) if args.only else {row["slug"] for row in profile}
    selected = [row for row in profile if row["slug"] in required]
    if len(selected) != len(required):
        parser.error("unknown or duplicate app slug")
    for name in ("apps", "logs", "work"):
        (args.output / name).mkdir(parents=True, exist_ok=True)
    receipt = args.output / "build-results.json"
    previous = json.loads(receipt.read_text(encoding="utf-8")) if args.only and receipt.is_file() else []
    rows = {row["slug"]: row for row in previous if row["slug"] not in required}
    passed = True
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        jobs = {pool.submit(run_build, case, args, env): case for case in selected}
        for job in concurrent.futures.as_completed(jobs):
            case = jobs[job]
            try:
                row = job.result()
            except (OSError, ValueError, subprocess.CalledProcessError) as error:
                row = {key: case[key] for key in ("slug", "project", "entry")}
                row.update(status="build-setup-failed", error=str(error))
            rows[row["slug"]] = row
            passed &= row["status"] == "linked"
            print(row["slug"], row["status"], flush=True)
    with tempfile.NamedTemporaryFile(mode="w", encoding="utf-8", dir=args.output, delete=False) as pending:
        json.dump([rows[slug] for slug in sorted(rows)], pending, indent=2)
        pending.write("\n")
    os.replace(pending.name, receipt)
    return 0 if passed else 1


if __name__ == "__main__":
    raise SystemExit(main())

# end of fb-corpus-js.py
