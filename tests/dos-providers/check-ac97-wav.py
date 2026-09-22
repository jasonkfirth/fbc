#!/usr/bin/env python3
"""FreeBASIC DOS audio tests: check-ac97-wav.py.

Validate the emulator's hardware capture of ac97-playback's direct mode:
440 Hz left, 880 Hz right, and the intervening duplicated mono cycle.
Does not read sfxlib's software capture or repair unfinished WAV headers.
"""

import argparse
from array import array
import math
import sys
import wave


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('file')
    args = parser.parse_args()
    with wave.open(args.file, 'rb') as wav:
        rate = wav.getframerate()
        if (rate, wav.getnchannels(), wav.getsampwidth(), wav.getcomptype()) != (48000, 2, 2, 'NONE'):
            raise ValueError('Expected 48 kHz, stereo signed 16-bit PCM')
        pcm = array('h', wav.readframes(wav.getnframes()))
    if sys.byteorder != 'little':
        pcm.byteswap()
    left, right = pcm[0::2], pcm[1::2]
    first = next(i for i, value in enumerate(left) if abs(value) > 100)
    if len(left) < rate * 3:
        raise ValueError('Missing one or more playback cycles')
    frequencies = []
    for channel, target in [(left, 440), (right, 880)]:
        # The blocking driver can have silent gaps between descriptors.
        # Count crossings over nonzero PCM time, retaining the previous sign
        # across gaps. The first half second is wholly in the stereo cycle.
        signal = [value for value in channel[first:first + rate // 2] if value]
        rms = math.sqrt(sum(value * value for value in signal) / max(1, len(signal)))
        crossings = sum(a < 0 < b for a, b in zip(signal, signal[1:]))
        frequency = crossings * rate / max(1, len(signal))
        if len(signal) < rate // 5 or rms < 1000 or abs(frequency - target) > target * 0.05:
            raise ValueError(f'Wrong or missing {target} Hz channel: {frequency:.1f} Hz, RMS={rms:.1f}')
        frequencies.append(round(frequency, 1))
    mono_run = longest = 0
    for a, b in zip(left, right):
        if a == b:
            if abs(a) > 100:
                mono_run += 1
            longest = max(longest, mono_run)
        else:
            mono_run = 0
    if longest < rate * 0.8:
        raise ValueError('Missing audible mono-to-stereo duplication cycle')
    print(f'PASS hardware stereo: {frequencies} Hz; duplicated mono: {longest} samples')


if __name__ == '__main__':
    main()

# end of check-ac97-wav.py
