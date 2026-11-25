/*
    Project: FreeBASIC browser corpus
    File: fb-corpus-visualfb-align.js
    Purpose: Supply browser input and output for the alignment command.
    Responsibilities: Read source, pass the selected line range and offer
    the result as a download. The compiled FreeBASIC worker formats it.
*/

"use strict";

const source = document.getElementById("visualfb-source");
const result = document.getElementById("visualfb-result");
const file = document.getElementById("visualfb-file");
const sample = document.getElementById("visualfb-sample");
const align = document.getElementById("visualfb-align");
const status = document.getElementById("visualfb-status");
const download = document.getElementById("visualfb-download");
const maxBytes = 256 * 1024;
let worker;
let url;
let loaded;
let loadId = 0;

function clearResult() {
    worker?.terminate();
    worker = null;
    result.value = "";
    if (url) URL.revokeObjectURL(url);
    url = null;
    download.hidden = true;
}

async function loadSource(read, name, run) {
    const id = ++loadId;
    clearResult();
    status.textContent = "Opening source…";
    try {
        const bytes = new Uint8Array(await read());
        if (id !== loadId) return;
        if (!bytes.length || bytes.length > maxBytes) throw new Error("Choose source between 1 byte and 256 KiB.");
        const bom = bytes[0] === 239 && bytes[1] === 187 && bytes[2] === 191;
        const original = new TextDecoder("utf-8", { fatal: true }).decode(bytes);
        source.value = original;
        loaded = { original, display: source.value, bom, name };
        source.setSelectionRange(0, 0);
        status.textContent = "";
        if (run) alignSource();
    } catch (error) {
        if (id === loadId) status.textContent = error.message;
    }
}

function alignSource() {
    ++loadId;
    clearResult();
    const editor = source.value;
    if (!editor.trim() || new TextEncoder().encode(editor).length > maxBytes) {
        status.textContent = "Enter up to 256 KiB of source.";
        return;
    }
    const lineNumber = offset => editor.slice(0, offset).split("\n").length;
    const hasSelection = source.selectionStart !== source.selectionEnd;
    const first = hasSelection ? lineNumber(source.selectionStart) : 1;
    // A selection ending at the start of a line excludes that line.
    const end = hasSelection ? source.selectionEnd : editor.trimEnd().length;
    const last = lineNumber(end - (hasSelection ? 1 : 0));
    const original = loaded && editor === loaded.display ? loaded.original : editor;
    const bom = loaded?.bom ? "\ufeff" : "";
    const name = loaded?.name || "source.bas";
    const extension = /\.[^.]+$/.exec(name)?.[0] || ".bas";
    const stem = name.replace(/\.[^.]+$/, "");
    const active = new Worker("visualfb-align-worker.js");
    worker = active;
    status.textContent = "Aligning source…";
    active.onmessage = ({ data }) => {
        if (worker !== active) return;
        active.terminate();
        worker = null;
        if (data.error) {
            status.textContent = data.error;
            return;
        }
        result.value = data.output;
        status.textContent = data.message;
        url = URL.createObjectURL(new Blob([bom, data.output], { type: "text/plain;charset=utf-8" }));
        download.href = url;
        download.download = (stem || "source") + "-aligned" + extension;
        download.hidden = false;
    };
    active.onerror = () => {
        if (worker !== active) return;
        active.terminate();
        worker = null;
        status.textContent = "Could not run the formatter.";
    };
    active.postMessage({ first: String(first), last: String(last), source: original });
}

function loadSample() {
    file.value = "";
    return loadSource(async () => {
        const response = await fetch("sample.bas");
        if (!response.ok) throw new Error("Could not open the sample.");
        return response.arrayBuffer();
    }, "sample.bas", true);
}

sample.addEventListener("click", loadSample);
file.addEventListener("change", () => {
    const selected = file.files[0];
    if (selected) loadSource(() => {
        if (selected.size > maxBytes) throw new Error("The source exceeds 256 KiB.");
        return selected.arrayBuffer();
    }, selected.name, false);
});
align.addEventListener("click", alignSource);
source.addEventListener("input", () => {
    ++loadId;
    clearResult();
    status.textContent = "";
});
loadSample();

/* end of fb-corpus-visualfb-align.js */
