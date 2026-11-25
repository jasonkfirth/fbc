/*
    Project: FreeBASIC browser corpus
    File: fb-corpus-opensesh.js
    Purpose: Open a MIDI file and let the visitor download the converted MOD.
    Responsibilities: Own the form, worker lifetime, and temporary download.
    The worker and OpenSesh own all MIDI and MOD processing.
*/

"use strict";

const form = document.getElementById("opensesh-form");
const input = document.getElementById("opensesh-file");
const convert = document.getElementById("opensesh-convert");
const sample = document.getElementById("opensesh-sample");
const status = document.getElementById("opensesh-status");
const result = document.getElementById("opensesh-result");
const download = document.getElementById("opensesh-download");
const maxBytes = 64 * 1024 * 1024; // The OpenSesh MIDI reader uses the same limit.
let downloadUrl = "";

function clearResult() {
    if (downloadUrl) URL.revokeObjectURL(downloadUrl);
    downloadUrl = "";
    download.removeAttribute("href");
    download.hidden = true;
    result.textContent = "";
    result.hidden = true;
}

async function run(midiFile, filename) {
    clearResult();
    if (!midiFile.size || midiFile.size > maxBytes) {
        status.textContent = "Choose a MIDI file up to 64 MiB.";
        return;
    }
    input.disabled = sample.disabled = convert.disabled = true;
    status.textContent = "Converting...";

    let worker;
    let deadline;
    let finished = false;
    const finish = reply => {
        if (finished) return;
        finished = true;
        clearTimeout(deadline);
        if (worker) worker.terminate();
        result.textContent = reply.report || "";
        result.hidden = !reply.report;
        status.textContent = reply.error || "MOD ready to download.";
        if (reply.mod) {
            downloadUrl = URL.createObjectURL(new Blob([reply.mod], { type: "audio/mod" }));
            download.href = downloadUrl;
            download.download = filename;
            download.hidden = false;
        }
        input.disabled = sample.disabled = false;
        convert.disabled = input.files.length === 0;
    };
    try {
        const buffer = await midiFile.arrayBuffer();
        worker = new Worker("opensesh-worker.js");
        deadline = setTimeout(() => finish({ error: "The conversion timed out." }), 60000);
        worker.addEventListener("message", ({ data }) => finish(data), { once: true });
        worker.addEventListener("error", () => finish({ error: "OpenSesh stopped unexpectedly." }), { once: true });
        worker.postMessage(buffer, [buffer]);
    } catch {
        finish({ error: "Could not read the MIDI file or start OpenSesh." });
    }
}

input.addEventListener("change", () => {
    clearResult();
    status.textContent = "";
    convert.disabled = input.files.length === 0;
});
form.addEventListener("submit", event => {
    event.preventDefault();
    if (convert.disabled) return;
    const file = input.files[0];
    run(file, file.name.replace(/\.[^.]+$/, "") + ".mod");
});
sample.addEventListener("click", () => run(new Blob([sampleMidi()]), "OpenSesh-Sample.mod"));
sample.disabled = false;
convert.disabled = input.files.length === 0;

/* end of fb-corpus-opensesh.js */
