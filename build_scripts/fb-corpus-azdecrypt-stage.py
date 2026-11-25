#!/usr/bin/env python3
"""Stage AZdecrypt's original four-gram substitution search for a worker.

Extract the solver and four-gram branches from the selected corpus checkout.
The BASIC adapter owns input, execution bounds and file output. The desktop
UI and its native thread dispatcher are outside this build.
"""

import argparse
import gzip
import hashlib
from pathlib import Path
import re
import shutil
import subprocess
import textwrap


def read_source(path):
    # The solver fragments are ASCII inside legacy Windows source files.
    return path.read_text(encoding="latin1")


def section(source, first, after):
    start = source.index(first) + len(first)
    finish = source.index(after, start)
    return source[start:finish]


def fourgram(source):
    return textwrap.dedent(section(source, "\t\t\tcase 4\n", "\t\t\tcase 5\n")).strip()


def substitute(source, old, new):
    if source.count(old) != 1:
        raise ValueError(f"Unexpected AZdecrypt solver source near {old}")
    return source.replace(old, new, 1)


def write_basic(path, purpose, body):
    cleaned = "\n".join(line.rstrip() for line in body.strip().splitlines())
    path.write_text(f"' Project: AZdecrypt browser corpus\n' File: {path.name}\n"
                    f"' Purpose: {purpose}\n' The browser adapter supplies the inputs and job lifecycle.\n\n"
                    + cleaned + f"\n\n' end of {path.name}\n", encoding="ascii", newline="\n")


def stage(project, output):
    app = project / "AZdecrypt"
    upstream = app / "Source/AZdecrypt"
    main = read_source(upstream / "AZdecrypt.bas")
    solver = section(main, "\nsub azdecrypt_234567810g(byval tn_ptr as any ptr)\n", "\nend sub")
    # The native thread waits for multiple jobs. One call here runs the inner
    # search once, with the normal restart schedule and no desktop callbacks.
    search = section(solver, "\n\t\t\t\tenttable_ngs=ngram_size\n", "\n\t\t\t\tnext lv") + "\n\t\t\t\tnext lv"
    search = substitute(search, "nba(i)=thread(tn).cip(i)", "nba(i)=cipherSymbols(i)")
    search = substitute(search, '#include "solver_accuracy_shortcircuit.bi" \'inflates 100% accuracy', "")
    begin = search.index("if solution_improved=1 andalso timer-solution_timer>")
    finish = search.index("#include \"ext_hc3.bi\"", begin)
    finish = search.index("end if", finish) + len("end if")
    search = search[:begin] + search[finish:]
    begin = search.index("thread(tn).iterations_completed+=1")
    finish = search.index("\n", search.index("if pausetask=1", begin))
    search = search[:begin] + "tested += 1" + search[finish:]
    search = substitute(search, "thread(tn).sectime=timer-sectimer", "")
    search = substitute(search, "thread(tn).homophones=hps", "")
    search = substitute(search, "thread(tn).ent=entropy", "")
    search = substitute(search, "thread(tn).multiplicity=s/l", "")
    search = substitute(search, "solution_timer=timer", "")
    search = substitute(search, "solution_improved=1", "")
    search = textwrap.dedent(search)
    # The local job has no cribs or explicit homophone penalty. Preserve the
    # original moves and cooling schedule for that configuration.
    search = re.sub(r"(?ms)^(\t+)if use_cribs=0 then\n\1\t(rng[^\n]+)\n.*?^\1end if\n", r"\1\2\n", search, count=1)
    search = re.sub(r"(?ms)^(\t+)if ngram_system=3 then\n.*?^\1end if\n", r"\1rng(d,4,state)\n", search, count=1)
    search = re.sub(r"(?ms)^(\t+)if solvesub_homophoneweight>0 then\n.*?^\1end if\n", "", search, count=1)
    search = re.sub(r"(?ms)^(\t+)hps=0 'homophone weight\n.*?^\1next i\n", "", search, count=1)
    search = "\n".join(line for line in search.splitlines()
                       if not line.lstrip().startswith(("'", "hp(", "memset(@hp", "ngf/=")))
    search = substitute(search, "if accept=0 then old_score-=temp*map1(curr_symbol,0)*onesixl*old_score/new_score",
                        "if accept=0 then\n\t\t\t\t\t' An unseen four-gram can produce a zero candidate score.\n"
                        "\t\t\t\t\tif new_score<=0 then\n\t\t\t\t\t\told_score=0\n\t\t\t\t\telse\n"
                        "\t\t\t\t\t\told_score-=temp*map1(curr_symbol,0)*onesixl*old_score/new_score\n"
                        "\t\t\t\t\tend if\n\t\t\t\tend if")
    if re.search(r"\b(?:thread|hp|hps|ngram_system|use_cribs|cribkey|solvesub_homophoneweight)\b", search):
        raise ValueError("Desktop-only state remained in the search fragment")

    output.mkdir(parents=True, exist_ok=False)
    for name in ("solver_ngram_init.bi", "solver_ngram_main.bi", "solver_ngram_tail.bi", "thread_load_best_ngram.bi"):
        write_basic(output / name, "Use the four-gram branch of the upstream solver.", fourgram(read_source(upstream / name)))
    picker = read_source(upstream / "solver_pickletter.bi")
    pick_letter = section(picker, "bls=0\n", "\tselect case ngram_system\n") + fourgram(picker) + "\n"
    pick_letter += picker[picker.rindex("\tif new_letter=old_letter or new_letter=abc_size then"):]
    write_basic(output / "solver_pickletter.bi", "Choose a replacement letter with AZdecrypt's four-gram candidate table.", pick_letter)
    write_basic(output / "solver_picksymbol.bi", "Use the upstream symbol selection cycle for a solve without cribs.",
                section(read_source(upstream / "solver_picksymbol.bi"), "if use_cribs=0 then", "\nelse"))
    write_basic(output / "solver_randomnewletter.bi", "Draw a different letter in the native alphabet.",
                read_source(upstream / "solver_randomnewletter.bi"))
    score = re.search(r"^\tcase 2:(.+)$", read_source(upstream / "solver_fastent.bi"), re.M)[1]
    write_basic(output / "solver_fastent.bi", "Use AZdecrypt's square-root entropy weighting.", score)
    write_basic(output / "solver_search.bi", "Run the original substitution search for one finite browser job.",
                re.sub(r"\n(?:[ \t]*\n){2,}", "\n\n", search))
    helpers = []
    for name in ("logbx", "m_ioc2"):
        helpers.append("function " + name + section(main, "\nfunction " + name, "\nend function") + "\nend function")
    write_basic(output / "solver_statistics.bi", "Use the original log and index of coincidence helpers.", "\n\n".join(helpers))
    macro = "#macro rng(a,b,state)\n" + section(main, "#macro rng(a,b,state) 'https://en.wikipedia.org/wiki/Lehmer_random_number_generator\n", "#endmacro")
    write_basic(output / "solver_random.bi", "Use AZdecrypt's Lehmer random-number step.", macro + "#endmacro")

    model_name = "4-grams_english_jarlve_reddit_v1912.gz"
    source_model = app / "N-grams" / model_name
    with gzip.open(source_model, "rb") as stream:
        model = stream.read(26 ** 4 + 1)
    if len(model) != 26 ** 4:
        raise ValueError("The English model must contain one byte for each four-letter sequence")
    (output / "english-fourgrams.bin").write_bytes(model)
    revision = subprocess.check_output(["git", "-C", str(project), "rev-parse", "HEAD"], text=True).strip()
    (output / "MODEL.txt").write_text(
        f"AZdecrypt N-grams/{model_name}\n"
        f"Corpus revision {revision}\n"
        f"Original AZdecrypt.bas SHA-256 {hashlib.sha256((upstream / 'AZdecrypt.bas').read_bytes()).hexdigest()}\n"
        f"Original compressed SHA-256 {hashlib.sha256(source_model.read_bytes()).hexdigest()}\n"
        "The build decompresses the original byte table. Its indexes run from AAAA through ZZZZ.\n",
        encoding="ascii", newline="\n")
    shutil.copyfile(project / "LICENSE", output / "LICENSE")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("project", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    stage(args.project, args.output)


if __name__ == "__main__":
    main()

# end of fb-corpus-azdecrypt-stage.py
