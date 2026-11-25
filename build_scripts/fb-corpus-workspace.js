/*
    Project: FreeBASIC browser corpus
    File: fb-corpus-workspace.js
    Purpose: Restore and save the virtual workspace used by a corpus program.
    Responsibilities: Mount IDBFS before BASIC starts, own the writable tab,
    and offer a browser download for the optional editable sample.
    The program still owns its file formats, menus and save commands.
*/

{
    const { root, aliases, sample } = /* workspace configuration */ null;
    const samplePath = sample && root + "/" + sample;
    if (samplePath) {
        Module.arguments = [samplePath];
        const initialized = Module.onRuntimeInitialized;
        Module.onRuntimeInitialized = function() {
            // Embedded files are installed by Wasm constructors after
            // preRun. Both the assets and the workspace exist at this point.
            prepareSample();
            initialized?.call(this);
        };
    }

    Module.preRun ??= [];
    Module.preRun.push(restoreWorkspace);

    function restoreWorkspace() {
        const dependency = "fb-corpus-workspace";

        function launch(message) {
            FS.chdir(root);
            if (message) {
                const warning = document.createElement("p");
                warning.setAttribute("role", "status");
                warning.textContent = message + " Files in this tab will be lost on reload.";
                Module.canvas.before(warning);
            }
            removeRunDependency(dependency);
        }

        FS.mkdir(root);
        for (const alias of aliases) {
            // Some desktop apps build absolute paths from ExePath, which is /
            // under fbc-js. Keep those paths inside this program's workspace.
            const directory = root + alias;
            FS.symlink(directory, alias);
        }
        addRunDependency(dependency);

        function complete(message) {
            for (const alias of aliases) FS.mkdirTree(root + alias);
            launch(message);
        }

        if (!navigator.locks) {
            complete("Persistent storage is unavailable in this browser.");
            return;
        }

        // IDBFS synchronizes a directory tree. Two tabs writing independent
        // snapshots could otherwise overwrite each other's work. Web Locks
        // releases this request when the document is unloaded.
        navigator.locks.request(root, { ifAvailable: true }, lock => {
            if (!lock) {
                complete("This workspace is already open in another tab.");
                return;
            }
            FS.mount(IDBFS, { autoPersist: true }, root);
            return new Promise(resolve => {
                // A run dependency keeps main from creating demo files before
                // IndexedDB has restored the previously saved ones.
                FS.syncfs(true, error => {
                    if (error) {
                        FS.unmount(root);
                        complete("The browser could not restore this workspace.");
                        resolve();
                        return;
                    }
                    complete();
                });
            });
        });
    }

    function prepareSample() {
        if (!FS.analyzePath(samplePath).exists) {
            FS.writeFile(samplePath, FS.readFile("/browser-sample/" + sample));
        }
        const button = document.createElement("button");
        button.type = "button";
        button.textContent = "Download saved " + sample;
        button.addEventListener("click", () => {
            const bytes = FS.readFile(samplePath);
            const url = URL.createObjectURL(new Blob([bytes], { type: "application/octet-stream" }));
            const link = document.createElement("a");
            link.href = url;
            link.download = sample;
            link.click();
            // Keep the object URL until the browser starts the download.
            setTimeout(() => URL.revokeObjectURL(url), 1000);
        });
        Module.canvas.before(button);
    }
}

/* end of fb-corpus-workspace.js */
