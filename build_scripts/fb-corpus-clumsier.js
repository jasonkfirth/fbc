/*
    Project: FreeBASIC browser corpus
    File: fb-corpus-clumsier.js
    Purpose: Connect Clumsier's native terminal editor to browser file controls.
    Responsibilities: Import and export files, forward editor commands, and
    collect lines of input while BASIC awaits a filename or a confirmation.
    The BASIC program owns editing, prompts, and its on-disk file format.
*/

"use strict";

const controls = document.getElementById("editor-controls");
const status = document.getElementById("editor-status");
const terminal = document.getElementById("termDiv");
const form = document.getElementById("editor-prompt");
const answer = document.getElementById("editor-answer");
const upload = document.getElementById("editor-upload");
const files = document.getElementById("editor-files");
const download = document.getElementById("editor-download");
const projectDir = "/prj/";
const baseName = /^[A-Za-z0-9_-]{1,48}$/;
const sourceName = /^[A-Za-z0-9_-]{1,48}\.bas$/;
const commands = { F1: 112, F2: 113, F3: 114 };
let nextLine;
let refreshTimer;

function refreshFiles() {
    const names = Module.FS.readdir(projectDir).filter(name => sourceName.test(name)).sort();
    if (names.join("\n") !== Array.from(files.options, option => option.value).join("\n")) {
        const selected = files.value;
        files.replaceChildren(...names.map(name => new Option(name, name)));
        if (names.includes(selected)) files.value = selected;
    }
    download.disabled = !files.value;
}

// The fbc-js console sees the same legacy key codes from buttons and from
// the physical function keys. The desktop editor still handles each action.
for (const button of controls.querySelectorAll("[data-editor-key]")) {
    button.addEventListener("click", () => {
        if (nextLine) {
            answer.focus();
            return;
        }
        terminal.focus({ preventScroll: true });
        const key = button.dataset.editorKey;
        const event = { key, code: key, keyCode: commands[key] };
        window.dispatchEvent(new KeyboardEvent("keydown", event));
        window.dispatchEvent(new KeyboardEvent("keyup", event));
    });
}

// Preserve keyboard navigation through the browser controls. In the
// terminal, Tab indents and Shift+Tab returns to the browser controls.
document.addEventListener("focusin", event => {
    Module.fbLineInput = terminal.contains(event.target) ? null : event.target;
});
window.addEventListener("keydown", event => {
    if (event.key === "Tab" && event.shiftKey && document.activeElement === terminal) {
        event.stopImmediatePropagation();
    }
}, true);

form.addEventListener("submit", event => {
    event.preventDefault();
    if (!nextLine) return;
    const line = answer.value.trim();
    if (!baseName.test(line)) {
        status.textContent = "Use letters, digits, hyphens or underscores.";
        return;
    }
    const resolve = nextLine;
    nextLine = undefined;
    answer.value = "";
    form.hidden = true;
    status.textContent = "";
    terminal.focus({ preventScroll: true });
    resolve(line);
});

upload.addEventListener("change", async () => {
    const file = upload.files[0];
    if (!file) return;
    upload.value = "";
    const name = file.name.replace(/\.bas$/i, ".bas");
    // The original editor stores single-byte text in a fixed 80-column
    // grid. Its native reader rewrites long lines as BASIC continuations.
    if (!sourceName.test(name) || file.size === 0 || file.size > 64 * 1024) {
        status.textContent = "Choose a nonempty .bas file under 64 KiB with a simple filename.";
        return;
    }
    try {
        const text = await file.text();
        if (!/^[\x20-\x7e\r\n]+$/.test(text) ||
            text.split(/\r\n|\n|\r/).some(line => line.length > 79)) {
            status.textContent = "Import ASCII source with up to 79 characters per line. Use spaces for indentation.";
            return;
        }
        const path = projectDir + name;
        if (Module.FS.analyzePath(path).exists) {
            status.textContent = name + " already exists. Rename the file before importing it.";
            return;
        }
        Module.FS.writeFile(path, text.replace(/\r\n?/g, "\n"));
        refreshFiles();
        files.value = name;
        status.textContent = "Imported " + name + ". Choose Open and enter " + name.slice(0, -4) + ".";
    } catch (error) {
        console.error(error);
        status.textContent = "The source file could not be imported.";
    }
});

download.addEventListener("click", () => {
    const name = files.value;
    if (!sourceName.test(name)) return;
    try {
        const bytes = Module.FS.readFile(projectDir + name);
        const url = URL.createObjectURL(new Blob([bytes], { type: "application/octet-stream" }));
        const link = document.createElement("a");
        link.href = url;
        link.download = name;
        link.click();
        setTimeout(() => URL.revokeObjectURL(url), 1000);
    } catch (error) {
        console.error(error);
        status.textContent = "The saved source file could not be downloaded.";
    }
});

function stopEditor(message) {
    clearInterval(refreshTimer);
    controls.disabled = true;
    form.hidden = true;
    status.textContent = message;
}

var Module = {
    canvas: document.getElementById("canvas"),
    preRun: [function() {
        __fb_rtlib.console.open(80, 25);
    }],
    fbReadLine() {
        return new Promise(resolve => {
            nextLine = resolve;
            form.hidden = false;
            answer.focus();
        });
    },
    onRuntimeInitialized() {
        controls.disabled = false;
        status.textContent = "";
        refreshFiles();
        // BASIC uses its own F3 handler and writes directly to IDBFS.
        // Refresh the file picker without moving the current selection.
        refreshTimer = setInterval(() => {
            if (!document.hidden) refreshFiles();
        }, 500);
    },
    onExit() {
        stopEditor("Editor closed. Reload this page to reopen it.");
    },
    onAbort() {
        stopEditor("The editor stopped unexpectedly.");
    },
};

/* end of fb-corpus-clumsier.js */
