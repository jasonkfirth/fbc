/*
    Project: FreeBASIC browser corpus
    File: fb-corpus-compiler.js
    Purpose: Connect the browser compiler page to the FreeBASIC worker.
    Responsibilities: Read user files, run or cancel one job, and offer
    the compiler's output as a download. Compilation stays in the worker.
*/

"use strict";

const source = document.getElementById("compiler-source");
const input = document.getElementById("compiler-file");
const includes = document.getElementById("compiler-includes");
const modeInput = document.getElementById("compiler-mode");
const form = document.getElementById("compiler-form");
const status = document.getElementById("compiler-status");
const diagnostics = document.getElementById("compiler-diagnostics");
const output = document.getElementById("compiler-output");
const cancel = document.getElementById("compiler-cancel");
const download = document.getElementById("compiler-download");
const maxInputBytes = 256 * 1024;
let job;
let downloadUrl;

function finishJob() {
    job?.worker.terminate();
    clearTimeout(job?.timer);
    job = null;
    cancel.disabled = true;
}

function clearOutput() {
    diagnostics.textContent = "";
    output.value = "";
    if (downloadUrl) URL.revokeObjectURL(downloadUrl);
    downloadUrl = null;
    download.hidden = true;
}

modeInput.addEventListener("change", () => {
    if (!form.dataset.compilerWorker) return;
    finishJob();
    clearOutput();
    status.textContent = "";
    const bitcode = modeInput.value === "lbasic-ir";
    form.querySelector("button[type=submit]").textContent = bitcode ? "Generate bitcode" : "Parse";
    document.getElementById("compiler-text-output").hidden = bitcode;
});

input.addEventListener("change", async () => {
    const picked = input.files[0];
    if (!picked) return;
    finishJob();
    clearOutput();
    try {
        if (picked.size > maxInputBytes) throw new Error("The source file exceeds 256 KiB.");
        const text = new TextDecoder("utf-8", { fatal: true }).decode(await picked.arrayBuffer());
        if (input.files[0] !== picked) return;
        source.value = text;
        status.textContent = "";
    } catch (error) {
        status.textContent = error.message;
    }
});

cancel.addEventListener("click", () => {
    finishJob();
    status.textContent = "Compilation cancelled.";
});

form.addEventListener("submit", async event => {
    event.preventDefault();
    finishJob();
    clearOutput();
    const mode = modeInput.value;
    const parse = mode === "lbasic-parse";
    const bitcode = mode === "lbasic-ir";
    const code = source.value;
    const files = includes ? [...includes.files] : [];
    const totalBytes = new TextEncoder().encode(code).length + files.reduce((sum, file) => sum + file.size, 0);
    if (!code.trim() || totalBytes > maxInputBytes) {
        status.textContent = includes ? "Enter source and headers totaling up to 256 KiB." : "Enter source up to 256 KiB.";
        return;
    }
    const names = new Set();
    for (const file of files) {
        const key = file.name.toLowerCase();
        if (!/^[a-z0-9_.-]+\.bi$/.test(key) || names.has(key)) {
            status.textContent = "Choose .bi headers with distinct filenames.";
            return;
        }
        names.add(key);
    }
    const outputName = parse ? "main.parse" : bitcode ? "main.bc" : mode === "preprocess" ? "main.pp.bas" : "main.c";
    const worker = new Worker(form.dataset.compilerWorker || "compiler-worker.js");
    const current = { worker };
    job = current;
    cancel.disabled = false;
    status.textContent = parse ? "Parsing source…" : "Compiling source…";
    current.timer = setTimeout(() => {
        if (job !== current) return;
        finishJob();
        status.textContent = "The compiler exceeded 20 seconds.";
    }, 20000);
    worker.onmessage = ({ data }) => {
        if (job !== current) return;
        finishJob();
        diagnostics.textContent = data.diagnostics;
        if (data.error) {
            status.textContent = data.error;
            return;
        }
        if (!bitcode) output.value = data.output;
        status.textContent = "Generated " + outputName + ".";
        const type = bitcode ? "application/octet-stream" : "text/plain;charset=utf-8";
        downloadUrl = URL.createObjectURL(new Blob([data.output], { type }));
        download.href = downloadUrl;
        download.download = outputName;
        download.hidden = false;
    };
    worker.onerror = () => {
        if (job !== current) return;
        finishJob();
        status.textContent = "Could not run the compiler.";
    };
    try {
        const headers = await Promise.all(files.map(async file => ({ name: file.name, bytes: await file.arrayBuffer() })));
        if (job !== current) return;
        worker.postMessage({ mode, source: code, headers }, headers.map(header => header.bytes));
    } catch {
        if (job !== current) return;
        finishJob();
        status.textContent = "Could not open the selected headers.";
    }
});

/* end of fb-corpus-compiler.js */
