/*
    Project: FreeBASIC browser corpus
    File: fb-corpus-darkcastle.js
    Purpose: Download map files written by the original Dark Castle editor.
    Responsibilities: List saved files and pass their exact bytes to the browser.
    The BASIC editor owns map validation, editing, and serialization.
*/

"use strict";

const menu = document.getElementById("darkcastle-maps");
const list = document.getElementById("darkcastle-map-list");
const download = document.getElementById("darkcastle-download");
const message = document.getElementById("darkcastle-map-status");

menu.addEventListener("toggle", () => {
    if (!menu.open) return;
    list.replaceChildren();
    download.disabled = true;
    if (!Module.FS?.analyzePath("/maps").exists) {
        message.textContent = "The editor is still loading.";
        return;
    }
    for (const name of Module.FS.readdir("/maps").filter(file => /\.map$/i.test(file)).sort()) {
        list.add(new Option(name, name));
    }
    download.disabled = list.options.length === 0;
    message.textContent = download.disabled ? "Save a map in the editor first." : "";
});

download.addEventListener("click", () => {
    const name = list.value;
    if (!name) return;
    try {
        const bytes = Module.FS.readFile("/maps/" + name);
        const url = URL.createObjectURL(new Blob([bytes], { type: "application/octet-stream" }));
        const link = document.createElement("a");
        link.href = url;
        link.download = name;
        link.click();
        setTimeout(() => URL.revokeObjectURL(url), 1000);
    } catch {
        message.textContent = "The saved map could not be opened.";
    }
});

/* end of fb-corpus-darkcastle.js */
