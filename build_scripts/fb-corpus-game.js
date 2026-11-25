/*
    Project: FreeBASIC browser corpus
    File: fb-corpus-game.js
    Purpose: Give the OMA-style player keyboard focus on entry.
    The program and its runtime own keyboard and pointer events.
*/

"use strict";

{
    const canvas = document.getElementById("canvas");
    function focusGame() {
        canvas.focus({ preventScroll: true });
    }

    canvas.tabIndex = 0;
    canvas.addEventListener("pointerdown", focusGame);
    canvas.addEventListener("contextmenu", event => event.preventDefault());
    focusGame();
    window.addEventListener("load", () => {
        if (document.activeElement === document.body) focusGame();
    });
    window.addEventListener("resize", () => {
        if (document.activeElement === document.body) focusGame();
    });
}

/* end of fb-corpus-game.js */
