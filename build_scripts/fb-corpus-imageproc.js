/*
    Project: FreeBASIC browser corpus
    File: fb-corpus-imageproc.js
    Purpose: Compare a photo with its fb-imageproc grayscale conversion.
    Responsibilities: Decode the photo locally, render both images, and
    provide a PNG download. Grayscale conversion belongs to the Wasm worker.
*/

"use strict";

const file = document.getElementById("imageproc-file");
const sample = document.getElementById("imageproc-sample");
const method = document.getElementById("imageproc-method");
const original = document.getElementById("imageproc-original");
const result = document.getElementById("imageproc-result");
const status = document.getElementById("imageproc-status");
const images = document.getElementById("imageproc-images");
const download = document.getElementById("imageproc-download");
const maxSide = 1600;
const maxFileBytes = 12 * 1024 * 1024;
let source;
let downloadUrl;

function setBusy(busy) {
    file.disabled = busy;
    sample.disabled = busy;
    method.disabled = busy || !source;
}

function clearDownload() {
    if (downloadUrl) URL.revokeObjectURL(downloadUrl);
    downloadUrl = undefined;
    download.hidden = true;
    download.removeAttribute("href");
}

function runConversion(image) {
    return new Promise((resolve, reject) => {
        const worker = new Worker("imageproc-worker.js");
        const deadline = setTimeout(() => finish(new Error("Image conversion timed out.")), 15000);
        function finish(error, pixels) {
            clearTimeout(deadline);
            worker.terminate();
            if (error) reject(error);
            else resolve(pixels);
        }
        worker.addEventListener("message", ({ data }) => {
            finish(data.error ? new Error(data.error) : null, data.pixels);
        }, { once: true });
        worker.addEventListener("error", () => finish(new Error("The image converter stopped unexpectedly.")),
            { once: true });
        // Retain the source pixels so the next method can use the same input.
        const pixels = image.data.slice().buffer;
        worker.postMessage({ width: image.width, height: image.height, method: method.value, pixels }, [pixels]);
    });
}

async function showConversion() {
    clearDownload();
    images.hidden = true;
    status.textContent = "Converting image…";
    const pixels = new Uint8ClampedArray(await runConversion(source));
    const image = new ImageData(pixels, source.width, source.height);
    result.width = image.width;
    result.height = image.height;
    result.getContext("2d").putImageData(image, 0, 0);
    const blob = await new Promise(resolve => result.toBlob(resolve, "image/png"));
    if (!blob) throw new Error("Could not encode the result as PNG.");
    downloadUrl = URL.createObjectURL(blob);
    download.href = downloadUrl;
    download.download = method.value + "-grayscale.png";
    images.hidden = false;
    download.hidden = false;
    status.textContent = "Converted " + image.width + " × " + image.height + " pixels.";
}

async function loadImage(getFile) {
    setBusy(true);
    status.textContent = "Opening image…";
    try {
        const blob = await getFile();
        if (blob.size > maxFileBytes) throw new Error("Choose an image under 12 MiB.");
        let bitmap;
        try {
            bitmap = await createImageBitmap(blob);
        } catch {
            throw new Error("Could not open that image. Try a PNG, JPEG, WebP, or BMP.");
        }
        try {
            const scale = Math.min(1, maxSide / Math.max(bitmap.width, bitmap.height));
            original.width = Math.max(1, Math.round(bitmap.width * scale));
            original.height = Math.max(1, Math.round(bitmap.height * scale));
            const context = original.getContext("2d");
            context.drawImage(bitmap, 0, 0, original.width, original.height);
            source = context.getImageData(0, 0, original.width, original.height);
        } finally {
            bitmap.close();
        }
        await showConversion();
    } catch (error) {
        status.textContent = error.message;
    } finally {
        setBusy(false);
    }
}

function loadSample() {
    return loadImage(async () => {
        const response = await fetch("sample.bmp");
        if (!response.ok) throw new Error("Could not load the sample image.");
        return response.blob();
    });
}

file.addEventListener("change", () => {
    const selected = file.files[0];
    file.value = "";
    if (selected) loadImage(() => selected);
});
sample.addEventListener("click", loadSample);
method.addEventListener("change", async () => {
    if (!source) return;
    setBusy(true);
    try {
        await showConversion();
    } catch (error) {
        status.textContent = error.message;
    } finally {
        setBusy(false);
    }
});
loadSample();

/* end of fb-corpus-imageproc.js */
