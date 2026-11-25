/*
    Project: FreeBASIC browser corpus
    File: fb-corpus-converter.js
    Purpose: Present the FlightGear calculator's seven operations in a form.
    Responsibilities: Collect values and show the result of each Wasm request.
    The calculation and rounding rules live in the original BASIC units.
*/

"use strict";

const form = document.getElementById("converter");
const operation = document.getElementById("converter-operation");
const fields = document.getElementById("converter-fields");
const calculate = document.getElementById("converter-calculate");
const result = document.getElementById("converter-result");

const inputs = {
    1: [{ label: "Altitude (meters)" }],
    2: [{ label: "Altitude (feet)" }],
    3: [{ label: "Distance (kilometers)" }],
    4: [{ label: "Distance (miles)" }],
    5: [
        { label: "Degrees", max: 180, whole: true },
        { label: "Minutes", max: 59, whole: true },
        { label: "Seconds" },
    ],
    6: [{ label: "Angle (decimal degrees)", max: 180 }],
    7: [
        { label: "Descent angle (degrees)", min: 3, max: 15 },
        { label: "Altitude (feet)", max: 50000 },
        { label: "Destination elevation (feet)", max: 50000 },
    ],
};

function showFields() {
    fields.replaceChildren();
    for (const [index, field] of inputs[operation.value].entries()) {
        const label = document.createElement("label");
        const input = document.createElement("input");
        input.id = "converter-input-" + index;
        input.type = "number";
        input.inputMode = field.whole ? "numeric" : "decimal";
        input.required = true;
        input.min = field.min ?? 0;
        input.step = field.whole ? "1" : "any";
        if (field.max !== undefined) input.max = field.max;
        label.htmlFor = input.id;
        label.textContent = field.label;
        fields.append(label, input);
    }
    result.textContent = "";
}

operation.addEventListener("change", showFields);
showFields();

form.addEventListener("submit", event => {
    event.preventDefault();
    if (calculate.disabled) return;
    const values = Array.from(fields.querySelectorAll("input"), input => input.valueAsNumber);
    if (!values.every(Number.isFinite)) {
        result.textContent = "Enter a number in every field.";
        return;
    }
    if (operation.value === "5" && (values[2] >= 60 || (values[0] === 180 && (values[1] || values[2])))) {
        result.textContent = "Seconds must be below 60, and the total angle must be at most 180 degrees.";
        return;
    }
    if (operation.value === "7" && values[2] > values[1]) {
        result.textContent = "Destination elevation must be at or below the starting altitude.";
        return;
    }

    let worker;
    try {
        worker = new Worker("converter-worker.js");
    } catch {
        result.textContent = "Could not start the calculator.";
        return;
    }

    // Use a fresh instance so a failed request cannot leave a BASIC input
    // prompt pending when the visitor starts another calculation.
    calculate.disabled = true;
    operation.disabled = true;
    result.textContent = "Calculating…";
    const deadline = setTimeout(() => finish("The calculation timed out."), 15000);
    function finish(text) {
        clearTimeout(deadline);
        worker.terminate();
        calculate.disabled = false;
        operation.disabled = false;
        result.textContent = text;
    }
    worker.addEventListener("message", ({ data }) => finish(data.text), { once: true });
    worker.addEventListener("error", () => finish("The calculator stopped unexpectedly."), { once: true });
    worker.postMessage({ operation: operation.value, values });
});

/* end of fb-corpus-converter.js */
