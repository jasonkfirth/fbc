/*
    Project: FreeBASIC browser corpus
    File: fb-corpus-format.js
    Purpose: Try fbrtLib's Format function with a value and mask.
    Responsibilities: Validate the request and show the returned text.
    Numeric formatting is implemented in the compiled FreeBASIC library.
*/

"use strict";

const form = document.getElementById("format-form");
const value = document.getElementById("format-value");
const mask = document.getElementById("format-mask");
const submit = document.getElementById("format-submit");
const result = document.getElementById("format-result");
const status = document.getElementById("format-status");

form.addEventListener("submit", event => {
    event.preventDefault();
    if (submit.disabled) return;
    const number = value.valueAsNumber;
    if (!Number.isFinite(number)) {
        status.textContent = "Enter a finite number.";
        return;
    }
    // The library receives UTF-8 bytes. This limit includes multibyte
    // literals and covers the 5,000-place masks in its regression fixture.
    if (new TextEncoder().encode(mask.value).length > 5002) {
        status.textContent = "Use a shorter format mask.";
        return;
    }

    let worker;
    try {
        worker = new Worker("format-worker.js");
    } catch {
        status.textContent = "Could not start the formatter.";
        return;
    }
    submit.disabled = true;
    result.textContent = "";
    status.textContent = "Formatting…";
    const deadline = setTimeout(() => finish("", "The format request timed out."), 15000);

    function finish(output, message) {
        clearTimeout(deadline);
        worker.terminate();
        result.textContent = output;
        status.textContent = message || (output ? "" : "The result is empty.");
        submit.disabled = false;
    }

    worker.addEventListener("message", ({ data }) => finish(data.output, data.error), { once: true });
    worker.addEventListener("error", () => finish("", "The formatter stopped unexpectedly."), { once: true });
    const numberArgument = Object.is(number, -0) ? "-0" : String(number);
    worker.postMessage({ value: numberArgument, mask: mask.value });
});

/* end of fb-corpus-format.js */
