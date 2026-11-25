/*
    Project: FreeBASIC browser corpus
    File: fb-corpus-fblint.js
    Purpose: Submit a single file to the compiled FreeBASIC linter.
    Responsibilities: Manage worker lifetime and render the returned diagnostics.
    Rule evaluation and report formats are owned by fblint.
*/
"use strict";

const form = document.getElementById("lint-form");
const fileInput = document.getElementById("lint-file");
const source = document.getElementById("lint-source");
const target = document.getElementById("lint-target");
const profile = document.getElementById("lint-profile");
const suppressions = document.getElementById("lint-suppressions");
const submit = document.getElementById("lint-submit");
const message = document.getElementById("lint-status");
const table = document.getElementById("lint-findings");
const rows = table.tBodies[0];
const maxSourceBytes = 256 * 1024;
let header = false;

fileInput.addEventListener("change", async () => {
    const file = fileInput.files[0];
    if (!file) return;
    if (file.size > maxSourceBytes) {
        message.textContent = "Choose a BASIC file up to 256 KiB.";
        fileInput.value = "";
        return;
    }
    try {
        const text = await file.text();
        if (fileInput.files[0] !== file) return;
        source.value = text;
        header = /\.bi$/i.test(file.name);
        message.textContent = "Loaded " + file.name + ".";
        rows.replaceChildren();
        table.hidden = true;
    } catch {
        message.textContent = "Could not read this file.";
    }
});

form.addEventListener("submit", event => {
    event.preventDefault();
    if (submit.disabled) return;
    const data = new TextEncoder().encode(source.value);
    if (data.byteLength > maxSourceBytes) {
        message.textContent = "Enter up to 256 KiB of BASIC source.";
        return;
    }
    rows.replaceChildren();
    table.hidden = true;
    let worker;
    try { worker = new Worker("lint-worker.js"); }
    catch { message.textContent = "Could not start fblint."; return; }

    submit.disabled = true;
    fileInput.disabled = true;
    source.readOnly = true;
    message.textContent = "Checking source…";
    const deadline = setTimeout(() => finished({ error: "The check timed out." }), 20000);

    function finished({ findings, summary, error }) {
        clearTimeout(deadline);
        worker.terminate();
        submit.disabled = false;
        fileInput.disabled = false;
        source.readOnly = false;
        if (error) { message.textContent = error; return; }

        for (const finding of findings) {
            const row = rows.insertRow();
            const line = finding.line ? String(finding.line) : "File";
            const location = finding.column ? line + ":" + finding.column : line;
            for (const value of [location, finding.severity, finding.rule, finding.message]) {
                row.insertCell().textContent = value;
            }
        }
        table.hidden = findings.length === 0;
        const { errors, warnings, info } = summary;
        message.textContent = `${errors} errors, ${warnings} warnings, ${info} notes.`;
        if (errors + warnings + info > findings.length) message.textContent += ` Showing ${findings.length} findings.`;
    }

    worker.addEventListener("message", ({ data: report }) => finished(report), { once: true });
    worker.addEventListener("error", () => finished({ error: "The linter stopped unexpectedly." }), { once: true });
    worker.postMessage({ source: data, header, target: target.value, profile: profile.value,
        honorSuppressions: suppressions.checked }, [data.buffer]);
});

/* end of fb-corpus-fblint.js */
