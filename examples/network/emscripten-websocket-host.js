/*
    Project: FreeBASIC JavaScript target
    -----------------------------------

    File: emscripten-websocket-host.js

    Purpose:

        Provide a small local HTTP and WebSocket echo host for the
        emscripten-websocket FreeBASIC example.

    Responsibilities:

        * serve the compiled example files from one selected directory
        * perform the RFC 6455 version 13 WebSocket handshake
        * echo complete text and binary WebSocket messages
        * reject malformed, unmasked, fragmented, or oversized frames

    This file intentionally does NOT contain:

        * TLS certificate handling
        * a production authentication or authorization policy
        * a general-purpose WebSocket framework
*/

'use strict';

const crypto = require('crypto');
const fs = require('fs');
const http = require('http');
const path = require('path');

const DEFAULT_HOST = '127.0.0.1';
const DEFAULT_PORT = 18087;
const MAX_FRAME_BYTES = 1024 * 1024;
const WEBSOCKET_GUID = '258EAFA5-E914-47DA-95CA-C5AB0DC85B11';

/* ------------------------------------------------------------------------- */
/* Command-line parsing                                                       */
/* ------------------------------------------------------------------------- */

function fail(message) {
    process.stderr.write(`emscripten-websocket-host: ${message}\n`);
    process.exit(1);
}

function usage() {
    process.stdout.write(
        'Usage: node emscripten-websocket-host.js [--host ADDRESS] [--port PORT] [--root DIRECTORY]\n'
    );
}

function parseArguments(arguments_) {
    const options = {
        host: DEFAULT_HOST,
        port: DEFAULT_PORT,
        root: process.cwd()
    };

    for (let index = 0; index < arguments_.length; index++) {
        const argument = arguments_[index];
        let value;

        switch (argument) {
        case '--host':
            value = arguments_[++index];
            if (!value) {
                fail('--host requires an address');
            }
            options.host = value;
            break;

        case '--port':
            value = arguments_[++index];
            if (!value || !/^\d+$/.test(value)) {
                fail('--port requires an integer from 1 through 65535');
            }
            options.port = Number(value);
            if (options.port < 1 || options.port > 65535) {
                fail('--port requires an integer from 1 through 65535');
            }
            break;

        case '--root':
            value = arguments_[++index];
            if (!value) {
                fail('--root requires a directory');
            }
            options.root = path.resolve(value);
            break;

        case '--help':
        case '-h':
            usage();
            process.exit(0);
            break;

        default:
            fail(`unknown option: ${argument}`);
        }
    }

    if (!fs.statSync(options.root, { throwIfNoEntry: false })?.isDirectory()) {
        fail(`root directory does not exist: ${options.root}`);
    }

    return options;
}

/* ------------------------------------------------------------------------- */
/* HTTP file serving                                                          */
/* ------------------------------------------------------------------------- */

function contentType(filename) {
    switch (path.extname(filename).toLowerCase()) {
    case '.html': return 'text/html; charset=utf-8';
    case '.js': return 'text/javascript; charset=utf-8';
    case '.wasm': return 'application/wasm';
    case '.css': return 'text/css; charset=utf-8';
    case '.json': return 'application/json; charset=utf-8';
    default: return 'application/octet-stream';
    }
}

function resolveRequestPath(root, requestUrl) {
    const requestPath = new URL(requestUrl, 'http://localhost').pathname;
    const decodedPath = decodeURIComponent(requestPath);
    const relativePath = decodedPath === '/' ? 'emscripten-websocket.html' : decodedPath.slice(1);
    const resolvedPath = path.resolve(root, relativePath);
    const rootPrefix = root.endsWith(path.sep) ? root : `${root}${path.sep}`;

    if (resolvedPath !== root && !resolvedPath.startsWith(rootPrefix)) {
        return null;
    }

    return resolvedPath;
}

function serveFile(root, request, response) {
    if (request.method !== 'GET' && request.method !== 'HEAD') {
        response.writeHead(405, { Allow: 'GET, HEAD' });
        response.end();
        return;
    }

    let filename;
    try {
        filename = resolveRequestPath(root, request.url);
    } catch {
        response.writeHead(400);
        response.end();
        return;
    }

    if (!filename) {
        response.writeHead(403);
        response.end();
        return;
    }

    fs.stat(filename, (statError, stat) => {
        if (statError || !stat.isFile()) {
            response.writeHead(404);
            response.end();
            return;
        }

        response.writeHead(200, {
            'Content-Length': stat.size,
            'Content-Type': contentType(filename),
            'Cache-Control': 'no-store'
        });

        if (request.method === 'HEAD') {
            response.end();
            return;
        }

        const input = fs.createReadStream(filename);
        input.on('error', () => response.destroy());
        input.pipe(response);
    });
}

/* ------------------------------------------------------------------------- */
/* WebSocket framing                                                          */
/* ------------------------------------------------------------------------- */

function makeFrame(opcode, payload) {
    const payloadLength = payload.length;
    let headerLength;
    let lengthOffset;
    let frame;

    if (payloadLength < 126) {
        headerLength = 2;
        frame = Buffer.allocUnsafe(headerLength + payloadLength);
        frame[1] = payloadLength;
    } else if (payloadLength <= 65535) {
        headerLength = 4;
        frame = Buffer.allocUnsafe(headerLength + payloadLength);
        frame[1] = 126;
        frame.writeUInt16BE(payloadLength, 2);
    } else {
        headerLength = 10;
        frame = Buffer.allocUnsafe(headerLength + payloadLength);
        frame[1] = 127;
        frame.writeBigUInt64BE(BigInt(payloadLength), 2);
    }

    frame[0] = 0x80 | opcode;
    lengthOffset = headerLength;
    payload.copy(frame, lengthOffset);
    return frame;
}

function closeSocket(socket, code, reason) {
    const reasonBytes = Buffer.from(reason, 'utf8');
    const payload = Buffer.allocUnsafe(2 + reasonBytes.length);

    payload.writeUInt16BE(code, 0);
    reasonBytes.copy(payload, 2);

    if (!socket.destroyed) {
        socket.write(makeFrame(0x8, payload));
        socket.end();
    }
}

function attachWebSocket(socket, initialData) {
    let pending = initialData;
    let closing = false;

    function protocolError(reason) {
        if (!closing) {
            closing = true;
            closeSocket(socket, 1002, reason);
        }
    }

    function processFrames() {
        while (pending.length >= 2 && !closing) {
            const firstByte = pending[0];
            const secondByte = pending[1];
            const finalFrame = (firstByte & 0x80) !== 0;
            const reservedBits = firstByte & 0x70;
            const opcode = firstByte & 0x0f;
            const masked = (secondByte & 0x80) !== 0;
            let payloadLength = secondByte & 0x7f;
            let headerLength = 2;
            let maskOffset;
            let payloadOffset;
            let payload;

            if (reservedBits !== 0 || !finalFrame) {
                protocolError('fragmented frames are not supported');
                return;
            }

            if (!masked) {
                protocolError('client frames must be masked');
                return;
            }

            if (payloadLength === 126) {
                if (pending.length < 4) {
                    return;
                }
                payloadLength = pending.readUInt16BE(2);
                headerLength = 4;
            } else if (payloadLength === 127) {
                if (pending.length < 10) {
                    return;
                }
                const longLength = pending.readBigUInt64BE(2);
                if (longLength > BigInt(MAX_FRAME_BYTES)) {
                    protocolError('frame is too large');
                    return;
                }
                payloadLength = Number(longLength);
                headerLength = 10;
            }

            if (payloadLength > MAX_FRAME_BYTES) {
                protocolError('frame is too large');
                return;
            }

            if (opcode >= 0x8 && payloadLength > 125) {
                protocolError('control frame is too large');
                return;
            }

            maskOffset = headerLength;
            payloadOffset = maskOffset + 4;
            if (pending.length < payloadOffset + payloadLength) {
                return;
            }

            payload = Buffer.from(pending.subarray(payloadOffset, payloadOffset + payloadLength));
            for (let index = 0; index < payload.length; index++) {
                payload[index] ^= pending[maskOffset + (index % 4)];
            }
            pending = pending.subarray(payloadOffset + payloadLength);

            switch (opcode) {
            case 0x1:
            case 0x2:
                socket.write(makeFrame(opcode, payload));
                break;

            case 0x8:
                closing = true;
                if (!socket.destroyed) {
                    socket.write(makeFrame(0x8, payload));
                    socket.end();
                }
                break;

            case 0x9:
                socket.write(makeFrame(0xA, payload));
                break;

            case 0xA:
                break;

            default:
                protocolError('unsupported WebSocket opcode');
                return;
            }
        }
    }

    socket.on('data', (data) => {
        pending = Buffer.concat([pending, data]);
        if (pending.length > MAX_FRAME_BYTES + 14) {
            protocolError('pending input is too large');
            return;
        }
        processFrames();
    });

    socket.on('error', () => socket.destroy());
    processFrames();
}

/* ------------------------------------------------------------------------- */
/* HTTP upgrade handling                                                      */
/* ------------------------------------------------------------------------- */

function isWebSocketUpgrade(request) {
    const connection = request.headers.connection || '';
    const upgrade = request.headers.upgrade || '';

    return request.method === 'GET' &&
        /(^|,)\s*upgrade\s*(,|$)/i.test(connection) &&
        upgrade.toLowerCase() === 'websocket';
}

function validWebSocketKey(key) {
    if (typeof key !== 'string' || !/^[A-Za-z0-9+/]{22}==$/.test(key)) {
        return false;
    }

    return Buffer.from(key, 'base64').length === 16;
}

function acceptUpgrade(request, socket, head) {
    const key = request.headers['sec-websocket-key'];

    if (!isWebSocketUpgrade(request) || request.headers['sec-websocket-version'] !== '13' || !validWebSocketKey(key)) {
        socket.write(
            'HTTP/1.1 400 Bad Request\r\n' +
            'Connection: close\r\n' +
            'Content-Length: 0\r\n\r\n'
        );
        socket.destroy();
        return;
    }

    const accept = crypto.createHash('sha1').update(key + WEBSOCKET_GUID).digest('base64');
    socket.write(
        'HTTP/1.1 101 Switching Protocols\r\n' +
        'Upgrade: websocket\r\n' +
        'Connection: Upgrade\r\n' +
        `Sec-WebSocket-Accept: ${accept}\r\n\r\n`
    );
    socket.setNoDelay(true);
    attachWebSocket(socket, head);
}

/* ------------------------------------------------------------------------- */
/* Program entry                                                              */
/* ------------------------------------------------------------------------- */

const options = parseArguments(process.argv.slice(2));
const server = http.createServer((request, response) => serveFile(options.root, request, response));

server.on('upgrade', acceptUpgrade);
server.on('error', (error) => fail(error.message));
server.listen(options.port, options.host, () => {
    process.stdout.write(`WebSocket echo host listening at http://${options.host}:${options.port}/\n`);
});

/* end of emscripten-websocket-host.js */
