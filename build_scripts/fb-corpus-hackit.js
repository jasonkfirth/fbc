/*
    Project: FreeBASIC browser corpus
    File: fb-corpus-hackit.js
    Purpose: Open a browser-supplied document in HACKIT and export saved bytes.
    Responsibilities: Validate one local file and seed the Wasm filesystem.
    HACKIT owns editing, keyboard commands, and saving the document.
*/

"use strict";

const form = document.getElementById("hackit-open");
const upload = document.getElementById("hackit-upload");
const terminal = document.getElementById("termDiv");
const status = document.getElementById("hackit-status");
const download = document.getElementById("hackit-download");
// Bound the document before reading it into both JavaScript and Wasm memory.
const maxBytes = 128 * 1024;
const validName = /^[A-Za-z0-9_-]{1,48}\.(?:bas|bi|txt)$/i;
let documentName;
let documentPath;

form.addEventListener("submit", async event => {
    event.preventDefault();
    if (documentPath) return;
    form.querySelector("fieldset").disabled = true;
    try {
        const file = upload.files[0];
        if (file && (!validName.test(file.name) || file.size > maxBytes)) {
            throw new Error("Choose a .bas, .bi, or .txt file up to 128 KiB with a simple filename.");
        }
        const source = file ? await file.text() : 'print "Hello from HACKIT"\n';
        if (/[^\x20-\x7e\t\r\n]/.test(source)) {
            throw new Error("Choose a file containing plain ASCII text.");
        }
        documentName = file?.name || "sample.bas";
        documentPath = "/" + documentName;
        // Pass the document as a startup argument so the editor opens it
        // through the same loader it uses on Linux.
        Module.arguments = [documentPath];
        Module.preRun.push(() => Module.FS.writeFile(documentPath, source));
        status.textContent = "Opening " + documentName + "…";
        const script = document.createElement("script");
        script.src = "index.js";
        script.onerror = () => stopEditor("The editor could not load. Reload the page to retry.");
        document.head.append(script);
    } catch (error) {
        status.textContent = error.message;
        form.querySelector("fieldset").disabled = false;
    }
});

download.addEventListener("click", () => {
    try {
        const bytes = Module.FS.readFile(documentPath);
        const url = URL.createObjectURL(new Blob([bytes], { type: "application/octet-stream" }));
        const link = document.createElement("a");
        link.href = url;
        link.download = documentName;
        link.click();
        setTimeout(() => URL.revokeObjectURL(url), 1000);
    } catch (error) {
        console.error(error);
        status.textContent = "The saved file could not be downloaded.";
    }
});

// BASIC handles keystrokes at window scope. Give the surrounding page its
// keyboard controls when the editor div does not have focus.
document.addEventListener("focusin", event => {
    Module.fbLineInput = terminal.contains(event.target) ? null : event.target;
});
window.addEventListener("keydown", event => {
    if (event.key === "Tab" && event.shiftKey && document.activeElement === terminal) {
        event.stopImmediatePropagation();
    }
}, true);

function stopEditor(message) {
    status.textContent = message;
    if (!terminal.hidden) download.disabled = false;
}

var Module = {
    canvas: document.getElementById("canvas"),
    preRun: [function() {
        terminal.hidden = false;
        __fb_rtlib.console.open(80, 25);
    }],
    onRuntimeInitialized() {
        form.hidden = true;
        download.disabled = false;
        status.textContent = documentName + " is open. Press Ctrl+S in the editor before downloading.";
        terminal.focus({ preventScroll: true });
    },
    onExit() {
        stopEditor("Editor closed. Download your saved file or reload the page to open another.");
    },
    onAbort() {
        stopEditor("The editor stopped unexpectedly.");
    },
};

/* end of fb-corpus-hackit.js */
