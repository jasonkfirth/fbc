/*
    Project: FreeBASIC browser corpus
    File: fb-corpus-bindings.js
    Purpose: Let visitors convert a C header and download the resulting .bi.
    Responsibilities: Own the form, report diagnostics, and manage the worker.
    The worker runs fbfrog and owns its virtual filesystem.
*/

"use strict";

const form = document.getElementById("binding-form");
const source = document.getElementById("binding-source");
const output = document.getElementById("binding-output");
const diagnostics = document.getElementById("binding-diagnostics");
const status = document.getElementById("binding-status");
const convert = document.getElementById("binding-convert");
const download = document.getElementById("binding-download");
const file = document.getElementById("binding-file");

// One header in the browser works well without a full SDK include tree.
const maxHeaderBytes = 512 * 1024;
let downloadUrl;

function clearResult() {
    if (downloadUrl) URL.revokeObjectURL(downloadUrl);
    downloadUrl = undefined;
    download.removeAttribute("href");
    download.hidden = true;
    output.value = "";
    diagnostics.textContent = "";
    diagnostics.hidden = true;
}

file.addEventListener("change", async () => {
    const selected = file.files[0];
    if (!selected) return;
    if (selected.size > maxHeaderBytes) {
        status.textContent = "Use a header of 512 KiB or less.";
        return;
    }
    try {
        source.value = await selected.text();
        clearResult();
        status.textContent = "Loaded " + selected.name + ".";
    } catch (error) {
        status.textContent = "The browser could not read the selected header.";
    }
});

form.addEventListener("submit", event => {
    event.preventDefault();
    const header = source.value;
    clearResult();
    if (!header.trim()) {
        status.textContent = "Enter a C declaration to convert.";
        return;
    }
    if (header.length > maxHeaderBytes || new TextEncoder().encode(header).length > maxHeaderBytes) {
        status.textContent = "Use a header of 512 KiB or less.";
        return;
    }

    const worker = new Worker("binding-worker.js");
    convert.disabled = true;
    file.disabled = true;
    source.readOnly = true;
    status.textContent = "Converting…";
    const timeout = setTimeout(() => finish({ error: "Conversion took longer than 20 seconds." }), 20000);

    function finish({ result, error, log }) {
        clearTimeout(timeout);
        worker.terminate();
        convert.disabled = false;
        file.disabled = false;
        source.readOnly = false;
        diagnostics.textContent = log || "";
        diagnostics.hidden = !log;
        if (error) {
            status.textContent = error;
            return;
        }
        output.value = result;
        downloadUrl = URL.createObjectURL(new Blob([result], { type: "text/plain;charset=utf-8" }));
        download.href = downloadUrl;
        download.hidden = false;
        status.textContent = "Bindings ready.";
    }

    worker.addEventListener("message", event => finish(event.data), { once: true });
    worker.addEventListener("error", event => {
        event.preventDefault();
        finish({ error: "The browser could not run fbfrog." });
    }, { once: true });
    worker.postMessage(header);
});

/* end of fb-corpus-bindings.js */
