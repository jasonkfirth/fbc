/*
    Project: FreeBASIC browser corpus
    File: fb-corpus-openmidisesh-files.js
    Purpose: Transfer local files into and out of OpenMIDISeSh's workspace.
    Responsibilities: Bridge browser file controls and the Emscripten filesystem.
    The editor owns file formats, project operations, and playback.
*/

"use strict";

{
    const workspace = "/fb-corpus-openmidisesh";
    const importFile = document.getElementById("midi-import");
    const files = document.getElementById("midi-files");
    const refresh = document.getElementById("midi-refresh");
    const download = document.getElementById("midi-download");
    const status = document.getElementById("midi-file-status");
    const initialized = Module.onRuntimeInitialized;

    Module.onRuntimeInitialized = function () {
        initialized?.call(this);
        const fs = Module.FS;

        function listFiles() {
            const previous = files.value;
            const names = fs.readdir(workspace)
                .filter(name => fs.isFile(fs.stat(workspace + "/" + name).mode))
                .sort();
            files.replaceChildren();
            for (const name of names) files.add(new Option(name, name));
            if (names.includes(previous)) files.value = previous;
            files.disabled = download.disabled = names.length === 0;
        }

        importFile.addEventListener("change", async () => {
            const file = importFile.files[0];
            if (!file) return;
            importFile.disabled = true;
            try {
                if (!file.name || /[/\\\u0000-\u001f]/.test(file.name) || [".", ".."].includes(file.name)) {
                    throw new Error("Choose a file with a valid name.");
                }
                const path = workspace + "/" + file.name;
                const buffer = new Uint8Array(await file.arrayBuffer());
                // Check after reading. The editor could save a file while the browser reads one.
                if (fs.analyzePath(path).exists) throw new Error("A workspace file already has that name.");
                fs.writeFile(path, buffer);
                listFiles();
                files.value = file.name;
                status.textContent = "Imported " + file.name + ". Open it from the editor.";
            } catch (error) {
                status.textContent = error.message;
            } finally {
                importFile.value = "";
                importFile.disabled = false;
            }
        });

        refresh.addEventListener("click", listFiles);
        download.addEventListener("click", () => {
            try {
                const name = files.value;
                const bytes = fs.readFile(workspace + "/" + name);
                const url = URL.createObjectURL(new Blob([bytes]));
                const link = document.createElement("a");
                link.href = url;
                link.download = name;
                link.click();
                // Let the navigation consume the object URL before releasing it.
                setTimeout(() => URL.revokeObjectURL(url), 1000);
            } catch (error) {
                listFiles();
                status.textContent = error.message;
            }
        });
        listFiles();
        importFile.disabled = refresh.disabled = false;
    };
}

/* end of fb-corpus-openmidisesh-files.js */
