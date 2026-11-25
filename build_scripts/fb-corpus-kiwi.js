/*
    Project: FreeBASIC browser corpus
    File: fb-corpus-kiwi.js
    Purpose: Display the URL parts returned by the compiled Kiwi sample.
    Responsibilities: Submit an address and present the output or error.
    The worker owns the Wasm instance. This page makes no URL requests.
*/

"use strict";

const form = document.getElementById("kiwi-form");
const address = document.getElementById("kiwi-address");
const submit = form.querySelector("button[type=submit]");
const result = document.getElementById("kiwi-result");
const status = document.getElementById("kiwi-status");

form.addEventListener("submit", event => {
    event.preventDefault();
    if (submit.disabled) return;
    result.hidden = true;

    let worker;
    try {
        worker = new Worker("kiwi-worker.js");
    } catch {
        status.textContent = "Could not start Kiwi.";
        return;
    }

    submit.disabled = true;
    status.textContent = "Inspecting...";
    const deadline = setTimeout(() => finish({ error: "Kiwi timed out." }), 15000);

    function finish(reply) {
        clearTimeout(deadline);
        worker.terminate();
        status.textContent = reply.error || "";
        if (!reply.error) {
            document.getElementById("kiwi-protocol").textContent = reply.protocol;
            document.getElementById("kiwi-host").textContent = reply.host;
            document.getElementById("kiwi-resource").textContent = reply.resource || "(empty)";
            result.hidden = false;
        }
        submit.disabled = false;
    }

    worker.addEventListener("message", ({ data }) => finish(data), { once: true });
    worker.addEventListener("error", () => finish({ error: "Kiwi stopped unexpectedly." }), { once: true });
    worker.postMessage(address.value);
});
submit.disabled = false;

/* end of fb-corpus-kiwi.js */
