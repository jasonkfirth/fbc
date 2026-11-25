/*
    Project: FreeBASIC browser corpus
    File: fb-corpus-console.js
    Purpose: Connect a text-only Wasm program to the catalog's browser console.
    Responsibilities: Retain output, collect a line of text, and report exit.
    This file intentionally contains no game logic or graphics emulation.
*/

"use strict";

const output = document.getElementById("console-output");
const status = document.getElementById("program-status");
const form = document.getElementById("console-form");
const input = document.getElementById("console-answer");
const send = form.querySelector('button[type="submit"]');
const continueButton = document.getElementById("console-continue");
let nextLine;
let programEnded = false;
let followLatest = document.body.dataset.followOutput === "true";

// The runtime consumes touch events at window level to emulate a mouse.
// Keep touches that start in the console local so the browser can scroll the
// transcript, focus the input and generate clicks for the native controls.
const consolePanel = output.closest("main");
for (const event of ["touchstart", "touchmove", "touchend", "touchcancel"]) {
    consolePanel.addEventListener(event, touch => touch.stopPropagation(), { passive: true });
}

function atLatest() {
    return output.scrollHeight - output.scrollTop - output.clientHeight < 24;
}

if (followLatest) {
    output.addEventListener("scroll", () => { followLatest = atLatest(); });
    const resize = new ResizeObserver(() => {
        // Opening the touch keyboard shrinks the transcript. Keep the prompt
        // in view unless the visitor has scrolled back to read earlier turns.
        if (followLatest) output.scrollTop = output.scrollHeight;
    });
    resize.observe(output);
}

function append(text) {
    const atEnd = followLatest && atLatest();
    output.append(document.createTextNode(text));
    if (atEnd) output.scrollTop = output.scrollHeight;
}

function setStatus(text) {
    status.textContent = text;
    status.hidden = !text;
}

form.addEventListener("submit", event => {
    event.preventDefault();
    if (!nextLine) return;
    const answer = input.value;
    const resolve = nextLine;
    nextLine = undefined;
    input.value = "";
    // Keep the field focused across turns. Hiding it dismisses the touch
    // keyboard while the program processes the previous answer.
    input.focus({ preventScroll: true });
    send.disabled = true;
    append(answer + "\n");
    resolve(answer);
});

continueButton?.addEventListener("click", () => {
    // ToyBASIC uses SLEEP after showing its result. This lets touch visitors
    // send the same Enter key that a desktop visitor can press.
    window.dispatchEvent(new KeyboardEvent("keydown", { key: "Enter", code: "Enter", keyCode: 13 }));
    window.dispatchEvent(new KeyboardEvent("keyup", { key: "Enter", code: "Enter", keyCode: 13 }));
});

var Module = {
    arguments: JSON.parse(document.body.dataset.args),
    fbLineInput: input,
    fbReadLine() {
        return new Promise(resolve => {
            nextLine = resolve;
            const firstPrompt = form.hidden;
            form.hidden = false;
            send.disabled = false;
            if (firstPrompt && document.activeElement === document.body) input.focus({ preventScroll: true });
        });
    },
    setStatus(text) {
        // Emscripten may clear its loading message after a synchronous
        // program has already returned from main.
        if (!programEnded) setStatus(text);
    },
    print(text) {
        append(text + "\n");
    },
    printErr(text) {
        console.error(text);
    },
    preRun: [function() {
        // BASIC PRINT and the startup shell's stdout share the transcript.
        // Assigning text nodes also keeps program output from becoming HTML.
        __fb_rtlib.console.write = append;
    }],
    onExit(code) {
        programEnded = true;
        form.hidden = true;
        if (continueButton) continueButton.hidden = true;
        setStatus(code === 0 ? "Program finished." : "Program stopped with status " + code + ".");
    },
    onAbort() {
        programEnded = true;
        form.hidden = true;
        if (continueButton) continueButton.hidden = true;
        setStatus("The program stopped unexpectedly.");
    },
};

/* end of fb-corpus-console.js */
