/*
    Project: FreeBASIC browser corpus
    File: fb-corpus-azdecrypt.js
    Purpose: Submit alphabetic substitution ciphertext to AZdecrypt.
    Responsibilities: Validate input, cancel workers and display each result.
    All search and scoring are implemented by the compiled solver.
*/
"use strict";

const form = document.getElementById("azdecrypt-form");
const source = document.getElementById("azdecrypt-ciphertext");
const effort = document.getElementById("azdecrypt-effort");
const seed = document.getElementById("azdecrypt-seed");
const submit = document.getElementById("azdecrypt-submit");
const cancel = document.getElementById("azdecrypt-cancel");
const message = document.getElementById("azdecrypt-status");
const candidate = document.getElementById("azdecrypt-candidate");
let cancelCurrent;

form.addEventListener("submit", event => {
    event.preventDefault();
    if (submit.disabled) return;
    candidate.textContent = "";
    const ciphertext = new TextEncoder().encode(source.value);
    const letters = source.value.match(/[a-z]/gi) ?? [];
    const distinct = new Set(letters.map(letter => letter.toUpperCase()));
    if (ciphertext.length > 32768 || letters.length < 40 || letters.length > 4000 || distinct.size < 2) {
        message.textContent = "Use 40 to 4,000 letters and at least two distinct letters.";
        return;
    }
    const options = { restarts: Number(effort.value), seed: seed.valueAsNumber };
    if (!Number.isInteger(options.seed) || options.seed < 0 || options.seed > 1000000) {
        message.textContent = "Choose a seed from 0 to 1,000,000.";
        return;
    }

    let worker;
    try { worker = new Worker("azdecrypt-worker.js"); }
    catch { message.textContent = "Could not start AZdecrypt."; return; }
    submit.disabled = true;
    cancel.disabled = false;
    message.textContent = "Searching…";
    let complete = false;
    const deadline = setTimeout(() => finish({ error: "The search timed out. Try fewer restarts." }), 45000);
    cancelCurrent = () => finish({ error: "Search canceled." });

    function finish(result) {
        if (complete) return;
        complete = true;
        clearTimeout(deadline);
        worker.terminate();
        cancelCurrent = undefined;
        submit.disabled = false;
        cancel.disabled = true;
        if (result.error) { message.textContent = result.error; return; }
        candidate.textContent = result.solution;
        message.textContent = `Score ${result.score.toFixed(2)} after ${result.iterations.toLocaleString()} iterations.`;
    }

    worker.addEventListener("message", ({ data }) => finish(data), { once: true });
    worker.addEventListener("error", () => finish({ error: "AZdecrypt stopped unexpectedly." }), { once: true });
    worker.postMessage({ ...options, ciphertext }, [ciphertext.buffer]);
});

cancel.addEventListener("click", () => cancelCurrent?.());

/* end of fb-corpus-azdecrypt.js */
