/*
    Project: FreeBASIC browser corpus
    File: fb-corpus-brpg-map.js
    Purpose: Preview and download a map produced by the BRPGEngine converter.
    Responsibilities: Pick the input, manage the worker and render signed
    tile IDs from the returned bytes. Conversion runs inside FreeBASIC.
*/

"use strict";

const file = document.getElementById("brpg-file");
const sample = document.getElementById("brpg-sample");
const status = document.getElementById("brpg-status");
const preview = document.getElementById("brpg-preview");
const cell = document.getElementById("brpg-cell");
const download = document.getElementById("brpg-download");
const drawing = preview.getContext("2d");
const tileSize = 12;
// The original RPG declares each layer with indexes from 0 through 80.
const limit = 81;
let job = 0;
let worker;
let currentMap;
let downloadUrl;

function showMap(bytes, filename) {
    const data = new DataView(bytes);
    const width = data.getUint32(0, true);
    const height = data.getUint32(4, true);
    if (width < 1 || height < 1 || width > limit || height > limit ||
        data.byteLength !== 8 + width * height * 8) {
        throw new Error("The converted map has an unexpected size.");
    }
    preview.width = width * tileSize;
    preview.height = height * tileSize;
    let blocked = 0;
    let overlays = 0;
    for (let y = 0; y < height; y++) {
        for (let x = 0; x < width; x++) {
            const offset = 8 + (y * width + x) * 8;
            const ground = data.getInt32(offset, true);
            const overlay = data.getInt32(offset + 4, true);
            const px = x * tileSize;
            const py = y * tileSize;
            // The RPG permits walking when the ground ID is positive.
            if (ground <= 0) ++blocked;
            drawing.fillStyle = ground <= 0 ? "#25485b" : "#739777";
            drawing.fillRect(px, py, tileSize - 1, tileSize - 1);
            if (overlay !== 0) {
                ++overlays;
                drawing.fillStyle = "#f1c582";
                drawing.fillRect(px + 3, py + 3, tileSize - 7, tileSize - 7);
            }
        }
    }
    currentMap = { data, width, height };
    preview.hidden = false;
    status.textContent = `${width} × ${height} · ${blocked} blocked cells · ${overlays} overlays`;
    downloadUrl = URL.createObjectURL(new Blob([bytes], { type: "application/octet-stream" }));
    download.href = downloadUrl;
    download.download = filename.replace(/\.map$/i, "") + "-32bit.map";
    download.hidden = false;
}

async function convert(input, filename) {
    const request = ++job;
    worker?.terminate();
    if (downloadUrl) URL.revokeObjectURL(downloadUrl);
    preview.hidden = true;
    currentMap = null;
    cell.textContent = "";
    download.hidden = true;
    status.textContent = "Converting map…";
    try {
        const bytes = await input();
        if (request !== job) return;
        worker = new Worker("brpg-map-worker.js");
        worker.onmessage = ({ data }) => {
            if (request !== job) return;
            worker.terminate();
            try {
                if (data.error) throw new Error(data.error);
                showMap(data.map, filename);
            } catch (error) {
                status.textContent = error.message;
            }
        };
        worker.onerror = () => {
            if (request === job) status.textContent = "Could not run the converter.";
        };
        worker.postMessage(bytes, [bytes]);
    } catch (error) {
        if (request === job) status.textContent = error.message;
    }
}

function loadSample() {
    file.value = "";
    return convert(async () => {
        const response = await fetch("sample.map");
        if (!response.ok) throw new Error("Could not load the sample map.");
        return response.arrayBuffer();
    }, "sample.map");
}

sample.addEventListener("click", loadSample);
file.addEventListener("change", () => {
    const selected = file.files[0];
    if (!selected) return;
    convert(() => {
        if (selected.size > 4 + limit * limit * 4) throw new Error("The compact map exceeds 81 × 81 cells.");
        return selected.arrayBuffer();
    }, selected.name);
});

preview.addEventListener("click", event => {
    if (!currentMap) return;
    const { data, width, height } = currentMap;
    const box = preview.getBoundingClientRect();
    const x = Math.floor((event.clientX - box.left) * width / box.width);
    const y = Math.floor((event.clientY - box.top) * height / box.height);
    if (x < 0 || x >= width || y < 0 || y >= height) return;
    const offset = 8 + (y * width + x) * 8;
    cell.textContent = `Cell ${x}, ${y} · Ground ${data.getInt32(offset, true)} · Overlay ${data.getInt32(offset + 4, true)}`;
});

loadSample();

/* end of fb-corpus-brpg-map.js */
