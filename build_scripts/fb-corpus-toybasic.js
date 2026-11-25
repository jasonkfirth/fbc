/*
    Project: FreeBASIC browser corpus
    File: fb-corpus-toybasic.js
    Purpose: Let visitors evaluate their own ToyBASIC arithmetic expressions.
    Responsibilities: Submit one expression and show the interpreter's reply.
    Parsing and arithmetic run in the compiled BASIC program.
*/

"use strict";

const form = document.getElementById("toy-form");
const expression = document.getElementById("toy-expression");
const submit = document.getElementById("toy-submit");
const result = document.getElementById("toy-result");
const status = document.getElementById("toy-status");

form.addEventListener("submit", event => {
    event.preventDefault();
    if (submit.disabled) return;
    result.textContent = "";
    let worker;
    try {
        worker = new Worker("toybasic-worker.js");
    } catch {
        status.textContent = "Could not start ToyBASIC.";
        return;
    }

    submit.disabled = true;
    status.textContent = "Evaluating…";
    // Each invocation has its own Wasm instance and lexer state.
    const deadline = setTimeout(() => finish("", "ToyBASIC timed out."), 15000);
    function finish(value, error) {
        clearTimeout(deadline);
        worker.terminate();
        result.textContent = value;
        status.textContent = error;
        submit.disabled = false;
    }
    worker.addEventListener("message", ({ data }) => finish(data.result, data.error), { once: true });
    worker.addEventListener("error", () => finish("", "ToyBASIC stopped unexpectedly."), { once: true });
    worker.postMessage(expression.value);
});

/* end of fb-corpus-toybasic.js */
