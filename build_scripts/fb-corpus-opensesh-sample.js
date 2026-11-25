/*
    Project: FreeBASIC browser corpus
    File: fb-corpus-opensesh-sample.js
    Purpose: Construct an original Standard MIDI File for the player and tests.
    Responsibilities: Write the single-track header and note events.
    Playback, MIDI reading, and MOD export live elsewhere.
*/

"use strict";

function sampleMidi(notes = [48, 52, 55, 60]) {
    const ticksPerQuarter = 96;
    const track = [];
    for (const note of notes) {
        // Status 0x90 starts a channel-one note. Status 0x80 stops it one beat later.
        track.push(0, 0x90, note, 96, ticksPerQuarter, 0x80, note, 0);
    }
    track.push(0, 0xff, 0x2f, 0); // End of track.

    const bytes = new Uint8Array(22 + track.length);
    const header = new DataView(bytes.buffer);
    bytes.set([0x4d, 0x54, 0x68, 0x64], 0); // MThd, format 0, one track.
    header.setUint32(4, 6);
    header.setUint16(10, 1);
    header.setUint16(12, ticksPerQuarter);
    bytes.set([0x4d, 0x54, 0x72, 0x6b], 14); // MTrk, with a big-endian byte count.
    header.setUint32(18, track.length);
    bytes.set(track, 22);
    return bytes;
}

if (typeof module !== "undefined") module.exports = sampleMidi;

/* end of fb-corpus-opensesh-sample.js */
