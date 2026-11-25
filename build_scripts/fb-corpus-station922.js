/*
    Project: FreeBASIC browser corpus
    File: fb-corpus-station922.js
    Purpose: Preview a static site through the compiled Station922 handler.
    Responsibilities: Read optional local files and display the raw HTTP response.
    The worker owns the private document root and runs the request.
*/
"use strict";

const form = document.getElementById("station-form");
const folder = document.getElementById("station-folder");
const method = document.getElementById("station-method");
const host = document.getElementById("station-host");
const target = document.getElementById("station-target");
const submit = document.getElementById("station-submit");
const statusMessage = document.getElementById("station-status");
const headers = document.getElementById("station-headers");
const body = document.getElementById("station-body");
const download = document.getElementById("station-download");
const readLimit = 8 * 1024 * 1024;
let responseBody;

form.addEventListener("submit", async event => {
    event.preventDefault();
    if (submit.disabled) return;
    const path = target.value;
    const authority = host.value;
    if (!path.startsWith("/") || /[\u0000-\u0020\u007f]/.test(path) || /[\u0000-\u0020\u007f]/.test(authority)) {
        statusMessage.textContent = "Enter a host and an absolute path. Encode spaces as %20.";
        return;
    }
    const selected = Array.from(folder.files);
    if (selected.length > 128 || selected.reduce((total, file) => total + file.size, 0) > readLimit) {
        statusMessage.textContent = "Choose a site folder with up to 128 files and 8 MiB total.";
        return;
    }

    const requestMethod = method.value;
    submit.disabled = true;
    download.disabled = true;
    headers.textContent = "";
    body.textContent = "";
    statusMessage.textContent = "Running Station922…";
    responseBody = undefined;
    let worker;
    let deadline;
    try {
        const files = await Promise.all(selected.map(async file => [file.webkitRelativePath.split("/").slice(1).join("/"),
            new Uint8Array(await file.arrayBuffer())]));
        worker = new Worker("station922-worker.js");
        deadline = setTimeout(() => finish({ error: "The request timed out." }), 15000);
        worker.addEventListener("message", ({ data }) => finish(data), { once: true });
        worker.addEventListener("error", () => finish({ error: "Station922 stopped unexpectedly." }), { once: true });
        worker.postMessage({ files, method: requestMethod, host: authority, target: path },
            files.map(([, bytes]) => bytes.buffer));
    } catch {
        finish({ error: "Could not read the selected site or start Station922." });
    }

    function finish(result) {
        clearTimeout(deadline);
        worker?.terminate();
        submit.disabled = false;
        if (result.error) { statusMessage.textContent = result.error; return; }
        headers.textContent = result.headers;
        responseBody = result.body;
        download.disabled = responseBody.byteLength === 0;
        if (/^Content-Type: (text\/|application\/(json|xml))/im.test(result.headers)) {
            body.textContent = new TextDecoder().decode(responseBody.subarray(0, 64000));
        }
        statusMessage.textContent = `${responseBody.byteLength} response body bytes.`;
    }
});

download.addEventListener("click", () => {
    if (!responseBody?.byteLength) return;
    const url = URL.createObjectURL(new Blob([responseBody], { type: "application/octet-stream" }));
    const link = document.createElement("a");
    link.href = url;
    link.download = "response-body";
    link.click();
    setTimeout(() => URL.revokeObjectURL(url), 1000);
});

/* end of fb-corpus-station922.js */
