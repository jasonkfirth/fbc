#!/usr/bin/env python3
"""
FreeBASIC classic AmigaOS thread provider
---------------------------------------

File: amiga-build-pthread.py

Purpose:
    Build the AmigaPorts/AROS pthread implementation with stable task records.

Responsibilities:
    - verify immutable upstream sources and preserve their license notices
    - prevent TLS and thread records from moving while workers reference them
    - repair detached-thread reaping and failed-join semaphore ownership
    - publish an SDK-compatible native archive

This file intentionally does NOT contain:
    - a new scheduler or FreeBASIC thread-handle policy

The SDK's growing thread table can invalidate StarterFunc's cached pointer
while another task creates a thread. Reserving its documented maximum keeps
records stable and costs about 900 KiB, without imposing a new thread limit.
"""

import argparse
import hashlib
from pathlib import Path
import subprocess
import urllib.request


REVISION = '86711c70569ffc6b70ee1fb99895866c1d9f30e4'
SOURCE_ROOT = f'https://raw.githubusercontent.com/AmigaPorts/aros-stuff/{REVISION}/pthreads/'
SOURCE_HASHES = {
    'pthread.c': '5700209576773324c6fadd317c57f5e7a47d31e8c6a2e867323ee2299fa6bedf',
    'pthread.h': 'fb1c7422c3dd667d7a92eaf89c78a7d7c65ec1ae49d58c3252da9abdf1103855',
    'debug.h': 'cf5122c6803955a38ec5f3bb3e8d4d0b2908bf44160678978b9d4c380e269df5',
    'sched.c': 'a80a9301c026415384b58574cdbf9e4d531933f7b44af9321374d4d141abe7fc',
    'semaphore.c': '16db56066d267da3eba802dddb22c8b7ed84d09c7dd9a9ffefa435a8a9278320',
    'semaphore.h': '6972e686c1bf9844042500e852bd288e789580964b0f45ed4abb48eadede3a1c',
}


def replace_once(source, old, new):
    if source.count(old) != 1:
        raise SystemExit('Unsupported pthread source at patch: ' + old[:80])
    return source.replace(old, new, 1)


def patch_source(source):
    source = replace_once(source, '#define INITIALSIZE 1', '#define INITIALSIZE PTHREAD_THREADS_MAX')
    source = replace_once(source, '\t_threads = (ThreadInfo*) malloc(sizeof(ThreadInfo) * INITIALSIZE);',
                          '\t_tlskeys = calloc(PTHREAD_KEYS_MAX, sizeof(TLSKey));\n'
                          '\tif (!_tlskeys) exit(10);\n'
                          '\tnumTlskeys = PTHREAD_KEYS_MAX;\n'
                          '\t_threads = (ThreadInfo*) malloc(sizeof(ThreadInfo) * INITIALSIZE);')
    source = replace_once(source, '\tinf->task = (struct Task *)-1;',
                          '\tinf->task = inf->attr.detachstate ? NULL : (struct Task *)-1;')
    source = replace_once(source, '\tSignal(inf->parent, SIGF_PARENT);',
                          '\tif (inf->parent != NULL) Signal(inf->parent, SIGF_PARENT);')
    source = replace_once(source, '''int pthread_detach(pthread_t thread) {
	D(bug("%s(%ld) not implemented\\n", __FUNCTION__, thread));

	return ESRCH;
}''', '''int pthread_detach(pthread_t thread) {
	ThreadInfo *inf;
	int result = 0;
	ObtainSemaphore(&thread_sem);
	inf = GetThreadInfo(thread);
	if (inf == NULL || inf->task == NULL) result = ESRCH;
	else if (inf->attr.detachstate) result = EINVAL;
	else {
		inf->attr.detachstate = PTHREAD_CREATE_DETACHED;
		inf->parent = NULL;
		if (inf->finished) inf->task = NULL;
	}
	ReleaseSemaphore(&thread_sem);
	return result;
}''')
    source = replace_once(source, '''	if (inf == NULL || !inf->task)
		return ESRCH;

	if (inf->attr.detachstate)
		return EINVAL;''', '''	if (inf == NULL || !inf->task) {
		ReleaseSemaphore(&thread_sem);
		return ESRCH;
	}

	if (inf->attr.detachstate) {
		ReleaseSemaphore(&thread_sem);
		return EINVAL;
	}''')
    source = replace_once(source, '\ttls->used = TRUE;\n\ttls->destructor = destructor;',
                          '\tfor (unsigned j = 0; j < numThreads; ++j) _threads[j].tlsvalues[i] = NULL;\n'
                          '\ttls->used = TRUE;\n\ttls->destructor = destructor;')
    return ('/* FreeBASIC Amiga provider: altered stable tables, detach and join cleanup.\n'
            '   Original AmigaPorts/AROS notices and implementation follow. */\n' + source +
            '\n/* end of pthread-patched.c */\n')


def build(prefix, work, output):
    source_directory = work / 'source'
    source_directory.mkdir(parents=True, exist_ok=True)
    output.parent.mkdir(parents=True, exist_ok=True)
    for name, expected in SOURCE_HASHES.items():
        path = source_directory / name
        if not path.exists():
            with urllib.request.urlopen(SOURCE_ROOT + name, timeout=30) as response:
                data = response.read()
            if hashlib.sha256(data).hexdigest() != expected:
                raise SystemExit('Upstream pthread hash mismatch: ' + name)
            path.write_bytes(data)
        if hashlib.sha256(path.read_bytes()).hexdigest() != expected:
            raise SystemExit('Cached pthread hash mismatch: ' + name)

    patched = work / 'pthread-patched.c'
    patched.write_text(patch_source((source_directory / 'pthread.c').read_text()))
    object_file = work / 'pthread.o'
    subprocess.run([str(prefix / 'bin/m68k-amigaos-gcc'), '-m68020', '-msoft-float',
                    '-O2', '-I' + str(source_directory), '-c', str(patched),
                    '-o', str(object_file)], check=True)
    objects = [object_file]
    for name in ('sched', 'semaphore'):
        destination = work / (name + '.o')
        subprocess.run([str(prefix / 'bin/m68k-amigaos-gcc'), '-m68020', '-msoft-float',
                        '-O2', '-I' + str(source_directory), '-c',
                        str(source_directory / (name + '.c')), '-o', str(destination)], check=True)
        objects.append(destination)
    temporary = output.with_suffix('.a.new')
    if temporary.exists(): temporary.unlink()
    subprocess.run([str(prefix / 'bin/m68k-amigaos-ar'), 'rcs', str(temporary),
                    *map(str, objects)], check=True)
    temporary.replace(output)
    (output.parent / 'pthread-license.txt').write_text(
        (source_directory / 'pthread.c').read_text().split('*/', 1)[0] + '*/\n' +
        '\nSource: ' + SOURCE_ROOT + 'pthread.c\n' +
        'Altered for FreeBASIC by build_scripts/amiga-build-pthread.py.\n')
    print('Built stable Amiga pthread provider:', output)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--toolchain-root', type=Path, required=True)
    parser.add_argument('--work', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    build(args.toolchain_root.resolve(), args.work.resolve(), args.output.resolve())

# end of amiga-build-pthread.py
