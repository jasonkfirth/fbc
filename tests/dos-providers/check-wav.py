#!/usr/bin/env python3
"""FreeBASIC DOS tests: check-wav.py.

Check captured PCM has duration and audible signal, reporting peak and RMS.
Does not infer hardware IRQs or background progress; the guest tests do that.
"""

import argparse
from array import array
import math
import sys
import wave


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('files', nargs='+')
    args = parser.parse_args()
    for filename in args.files:
        with wave.open(filename, 'rb') as wav:
            if wav.getsampwidth() != 2 or wav.getcomptype() != 'NONE':
                raise ValueError('Expected uncompressed 16-bit PCM: ' + filename)
            duration = wav.getnframes() / wav.getframerate()
            pcm = array('h', wav.readframes(wav.getnframes()))
        if sys.byteorder != 'little':
            pcm.byteswap()
        peak = max((abs(sample) for sample in pcm), default=0)
        rms = math.sqrt(sum(sample * sample for sample in pcm) / max(1, len(pcm)))
        if duration < 0.2 or peak < 100 or rms < 10:
            raise ValueError('Empty, too short or silent capture: ' + filename)
        print(f'PASS {filename}: {duration:.3f}s, peak={peak}, RMS={rms:.1f}')


if __name__ == '__main__':
    main()

# end of check-wav.py
