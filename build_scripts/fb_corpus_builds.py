"""Project: FreeBASIC compiler tests
File: fb_corpus_builds.py
Purpose: Capture GCC build controls for maintained external BASIC projects.
Responsibilities: Freeze selected inputs, run project recipes, and collect compiler options.
This file intentionally does NOT contain: semantic validation or project source repairs.
"""

from __future__ import annotations

from concurrent.futures import ThreadPoolExecutor, as_completed
import importlib.util
import json
import os
from pathlib import Path
import shutil
import time

from compiler_semantic_audit import digest, freeze_semantic_reader, invoke


OMIT_DIRECTORIES = {".git", "toolchains", "node_modules", "__pycache__", "bin", "obj",
                    "out", "dist", "Temp", ".cache", "build", "release", "debug"}
OMIT_SUFFIXES = {".o", ".obj", ".exe", ".dll", ".log", ".bak", ".zip", ".7z", ".pyc"}
PATH_OPTIONS = {"-i", "-p", "-include"}
VALUE_OPTIONS = {"-arch", "-asm", "-d", "-entry", "-fpu", "-lang", "-m", "-maxerr",
                 "-s", "-t", "-target", "-w", "-Wc", "-Wl", "-Wf", "-O", "-nolib",
                 "-libpath", "-rpp"}


def write_json(path: Path, data) -> None:
    pending = path.with_suffix(path.suffix + ".tmp")
    pending.write_text(json.dumps(data, indent=2) + "\n")
    pending.replace(path)


def expand(argument: str, substitutions: dict[str, str]) -> str:
    for key, value in substitutions.items():
        argument = argument.replace("{" + key + "}", value)
    return argument


def freeze_projects(corpus: Path, target: Path, projects: set[str]) -> dict:
    omitted_links = []

    def omit(folder: str, names: list[str]) -> list[str]:
        excluded = []
        for name in names:
            path = Path(folder) / name
            if path.is_symlink() and (not path.exists() or not path.resolve().is_relative_to(corpus)):
                omitted_links.append(str(path.relative_to(corpus)))
                excluded.append(name)
            elif path.is_dir():
                if name in OMIT_DIRECTORIES or name.startswith(("build-", "build_", ".build-")):
                    excluded.append(name)
            elif path.suffix.lower() in OMIT_SUFFIXES or name.endswith("~"):
                excluded.append(name)
        return excluded

    for name in sorted(projects):
        if Path(name).is_absolute() or ".." in Path(name).parts:
            raise ValueError("Project name must stay inside the corpus: " + name)
        source = corpus / name
        if not source.is_dir():
            raise ValueError("Missing corpus project: " + str(source))
        shutil.copytree(source, target / name, ignore=omit)

    preparation = {"omitted_links": omitted_links, "prepared_directories": []}
    if "fbfrog" in projects:
        # The source archive omits its empty object directory, which its normal
        # Make recipe expects before the first source compilation.
        (target / "fbfrog/src/obj").mkdir(parents=True, exist_ok=True)
        preparation["prepared_directories"].append("fbfrog/src/obj")
    if "ohrrpgce" in projects and not (target / "ohrrpgce/revision.txt").exists():
        # Use OHR's own revision calculation before discarding Git metadata.
        # revision.txt is its supported source-archive input to the SCons build.
        script = corpus / "ohrrpgce/ohrbuild.py"
        spec = importlib.util.spec_from_file_location("corpus_ohrbuild", script)
        if spec is None or spec.loader is None:
            raise ValueError("Cannot load OHR's source-archive revision helper")
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        revision = module.query_revision(str(corpus / "ohrrpgce"))
        (target / "ohrrpgce/revision.txt").write_text(f"Revision: {revision}\n")
        preparation["ohr_revision"] = revision
    return preparation


def capture_wrapper(folder: Path, toolchain: Path, defines: list[str]) -> Path:
    folder.mkdir(parents=True)
    write_json(folder / "config.json", {"compiler": str(toolchain / "fbc"),
                                       "prefix": str(toolchain), "defines": defines})
    wrapper = folder / "fbc"
    wrapper.write_text('''#!/usr/bin/env python3
"""FreeBASIC corpus compiler wrapper. Preserve and capture real GCC source options."""
import json, os, subprocess, sys
from pathlib import Path
folder = Path(__file__).resolve().parent
config = json.loads((folder / "config.json").read_text())
arguments = []; index = 0
while index < len(sys.argv) - 1:
    arg = sys.argv[index + 1]
    if arg in ("-gen", "-prefix"):
        index += 2
        continue
    arguments.append(arg); index += 1
command = [config["compiler"], "-prefix", config["prefix"], "-i", config["prefix"] + "/inc", "-gen", "gcc"]
for define in config["defines"]:
    command += ["-d", define]
command += arguments
status = subprocess.run(command).returncode
if any(arg.lower().endswith((".bas", ".rbas")) for arg in arguments):
    record = {"arguments": arguments, "command": command, "cwd": str(Path.cwd()), "status": status}
    (folder / (str(os.getpid()) + ".json")).write_text(json.dumps(record, indent=2) + "\\n")
raise SystemExit(status)
# end of fbc
''')
    wrapper.chmod(0o755)
    return wrapper


def gcc_program(snapshot: Path, index: int, job: dict, timeout: int) -> dict:
    corpus = snapshot / "inputs/corpus"
    toolchain = snapshot / "inputs/toolchain"
    work = snapshot / "baseline-programs" / f"{index:03d}"
    work.mkdir(parents=True)
    project = corpus / job["project"]
    sources = [project / path for path in job["sources"]]
    flags = [expand(arg, {"corpus": str(corpus)}) for arg in job["flags"]]
    command = [str(toolchain / "fbc"), "-prefix", str(toolchain), "-i", str(toolchain / "inc"),
               "-i", str(project), "-i", str(sources[0].parent), "-gen", "gcc", *flags,
               *map(str, sources), "-x", str(work / "program")]
    start = time.monotonic()
    status = invoke(command, project, work / "build.log", timeout)
    return {**job, "index": index, "baseline_status": status, "command": command,
            "cwd": str(project), "log": str(work / "build.log"),
            "executable": str(work / "program"), "seconds": round(time.monotonic() - start, 2)}


def gcc_project(snapshot: Path, job: dict, timeout: int) -> dict:
    name = job["project"]
    project = snapshot / "inputs/corpus" / name
    toolchain = snapshot / "inputs/toolchain"
    wrapper = capture_wrapper(snapshot / "wrappers" / name, toolchain, job.get("defines", []))
    substitutions = {"project": str(project), "wrapper": str(wrapper), "toolchain": str(toolchain)}
    command = [expand(arg, substitutions) for arg in job["command"]]
    environment = [key + "=" + expand(value, substitutions)
                   for key, value in job.get("environment", {}).items()]
    command = ["env", *environment, "PATH=" + str(wrapper.parent) + ":" + os.environ["PATH"], *command]
    log = snapshot / "baseline-projects" / (name + ".log")
    log.parent.mkdir(exist_ok=True)
    status = invoke(command, project, log, timeout)
    return {"project": name, "status": status, "command": command, "cwd": str(project), "log": str(log)}


def source_options(arguments: list[str], cwd: Path) -> tuple[list[str], list[str]]:
    flags, sources = [], []
    index = 0
    while index < len(arguments):
        arg = arguments[index]
        if arg in ("-o", "-x", "-gen", "-prefix"):
            index += 2
            continue
        if arg == "-b":
            sources.append(str((cwd / arguments[index + 1]).resolve()))
            index += 2
            continue
        if arg in PATH_OPTIONS:
            flags += [arg, str((cwd / arguments[index + 1]).resolve())]
            index += 2
            continue
        if arg in VALUE_OPTIONS:
            flags += [arg, arguments[index + 1]]
            index += 2
            continue
        if arg.lower().endswith((".bas", ".rbas")):
            sources.append(str((cwd / arg).resolve()))
        elif arg.startswith("-"):
            flags.append(arg)
        index += 1
    return flags, sources


def collect_plan(snapshot: Path, registry: dict, programs: list[dict], projects: list[dict]) -> list[dict]:
    corpus = snapshot / "inputs/corpus"
    plan = []
    for record in programs:
        if record["baseline_status"]:
            continue
        project = corpus / record["project"]
        sources = [str(project / name) for name in record["sources"]]
        flags = ["-i", str(project), "-i", str(Path(sources[0]).parent)]
        flags += [expand(arg, {"corpus": str(corpus)}) for arg in record["flags"]]
        plan.append({"project": record["project"], "name": record["name"], "sources": sources,
                     "flags": flags, "cwd": str(project), "baseline": "linked program",
                     "control_log": record["log"]})
    seen = set()
    definitions = {job["project"]: job.get("defines", []) for job in registry["project_builds"]}
    for project in projects:
        if project["status"]:
            continue
        for path in sorted((snapshot / "wrappers" / project["project"]).glob("[0-9]*.json")):
            record = json.loads(path.read_text())
            if record["status"]:
                continue
            flags, sources = source_options(record["arguments"], Path(record["cwd"]))
            if not sources:
                continue
            for define in definitions[project["project"]]:
                flags += ["-d", define]
            key = (project["project"], tuple(sources), tuple(flags))
            if key in seen:
                continue
            seen.add(key)
            name = str(Path(sources[0]).relative_to(corpus / project["project"]))
            plan.append({"project": project["project"], "name": project["project"] + "/" + name,
                         "sources": sources, "flags": flags, "cwd": record["cwd"],
                         "baseline": "normal project build", "control_log": project["log"]})
    for index, item in enumerate(plan):
        item["id"] = f"{index:04d}"
        # -r makes the driver select object output. Restore the linked build's
        # implicit main role explicitly; otherwise command-line globals vanish.
        if not any(flag in item["flags"] for flag in ("-m", "-c", "-lib")):
            item["flags"] += ["-m", Path(item["sources"][0]).stem]
    return plan


def prepare_snapshot(root: Path, compiler: Path, corpus: Path, snapshot: Path, registry_path: Path,
                     selected: list[str] | None, jobs: int, timeout: int, build_timeout: int) -> None:
    registry = json.loads(registry_path.read_text())
    if selected:
        registry["programs"] = [job for job in registry["programs"] if job["project"] in selected]
        registry["project_builds"] = [job for job in registry["project_builds"] if job["project"] in selected]
    project_names = {job["project"] for family in ("programs", "project_builds") for job in registry[family]}
    if not project_names:
        raise ValueError("No registered corpus projects were selected")
    for project, dependencies in registry.get("project_dependencies", {}).items():
        if project in project_names:
            project_names.update(dependencies)
    snapshot.mkdir(parents=True, exist_ok=False)
    toolchain = snapshot / "inputs/toolchain"
    toolchain.mkdir(parents=True)
    shutil.copy2(compiler, toolchain / "fbc")
    shutil.copytree(root / "inc", toolchain / "inc")
    shutil.copytree(root / "lib/freebasic/linux-x86_64", toolchain / "lib/freebasic/linux-x86_64")
    validation = snapshot / "inputs/validation"
    validation.mkdir()
    freeze_semantic_reader(root, validation)
    for name in ("test-compiler-semantic-projects.py", "fb_corpus_builds.py", "compiler_semantic_audit.py"):
        shutil.copy2(Path(__file__).with_name(name), validation / name)
    preparation = freeze_projects(corpus, snapshot / "inputs/corpus", project_names)
    write_json(snapshot / "snapshot-preparation.json", preparation)
    write_json(snapshot / "build-registry.json", registry)
    write_json(snapshot / "inputs-sha256.json", {str(path.relative_to(snapshot)): digest(path)
               for path in sorted((snapshot / "inputs").rglob("*")) if path.is_file()})

    programs = []
    with ThreadPoolExecutor(max_workers=jobs) as workers:
        futures = [workers.submit(gcc_program, snapshot, index, job, timeout)
                   for index, job in enumerate(registry["programs"])]
        for future in as_completed(futures):
            record = future.result()
            programs.append(record)
            programs.sort(key=lambda item: item["index"])
            write_json(snapshot / "baseline-programs.json", programs)
            print("GCC", record["name"], record["baseline_status"], flush=True)
    projects = []
    with ThreadPoolExecutor(max_workers=min(jobs, 2)) as workers:
        futures = [workers.submit(gcc_project, snapshot, job, build_timeout) for job in registry["project_builds"]]
        for future in as_completed(futures):
            record = future.result()
            projects.append(record)
            projects.sort(key=lambda item: item["project"])
            write_json(snapshot / "baseline-projects.json", projects)
            print("GCC project", record["project"], record["status"], flush=True)
    plan = collect_plan(snapshot, registry, programs, projects)
    if not plan:
        raise ValueError("No GCC builds passed; see baseline logs")
    write_json(snapshot / "audit-plan.json", plan)
    sources = {Path(name) for item in plan for name in item["sources"]}
    write_json(snapshot / "audited-sources-sha256.json", {str(path): digest(path) for path in sorted(sources)})
    generated = snapshot / "inputs/corpus/ohrrpgce/audit-build"
    write_json(snapshot / "generated-inputs-sha256.json", {str(path): digest(path)
               for path in sorted(generated.glob("*")) if path.suffix.lower() in (".bas", ".bi", ".rbas")})
    print(f"Captured {len(plan)} GCC-controlled invocations, {len(sources)} root units", flush=True)


# end of fb_corpus_builds.py
