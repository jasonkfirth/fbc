''
'' FreeBASIC Sound Library (sfxlib)
'' --------------------------------
''
'' File: sfxlib_raw.bi
''
'' Purpose:
''
''     Provide an opt-in low-level declaration for writing raw sample
''     data through the sfxlib driver path and recording final output.
''
'' Responsibilities:
''
''     - link the sfxlib runtime library
''     - expose the raw floating-point output queue writer
''     - expose output-side WAV recording helpers
''     - keep raw driver access separate from the BASIC command set
''
'' This file intentionally does NOT contain:
''
''     - BASIC command declarations
''     - decoding helpers
''     - synthesis helpers
''     - platform driver details
''

#pragma once

#inclib "sfx"

namespace sfxlib

	extern "C"

	''
	'' RawOpen()
	''
	'' Starts an exclusive caller-clocked raw stream, discards previously
	'' queued output, and returns the active device sample rate. Callers must
	'' generate frames at this rate. A negative result indicates that the
	'' output device could not be initialized.
	''
	declare function RawOpen cdecl alias "fb_sfxRawOpen" _
		( _
		) as long

	''
	'' RawClose()
	''
	'' Stops raw streaming and discards frames that have not reached the
	'' platform driver. Ordinary sfxlib mixer commands may resume afterward.
	''
	declare sub RawClose cdecl alias "fb_sfxRawClose" _
		( _
		)

	''
	'' RawUnderruns()
	''
	'' Returns the number of device-side starvation events observed by the
	'' active driver since it was selected. A submitted capture can look valid
	'' even when the hardware ran out of queued frames between writes.
	''
	declare function RawUnderruns cdecl alias "fb_sfxRawUnderruns" _
		( _
		) as ulongint

	''
	'' OutputUnderruns()
	''
	'' Returns device starvation events for either the ordinary mixer or a raw
	'' stream. Stream handoffs are excluded from the count.
	''
	declare function OutputUnderruns cdecl alias "fb_sfxOutputUnderruns" _
		( _
		) as ulongint

	''
	'' OutputSampleRate()
	''
	'' Returns the sample rate selected by the active output driver. This is
	'' useful to sample-clocked sequencers that use the ordinary sfxlib mixer.
	'' A negative result indicates that output could not be initialized.
	''
	declare function OutputSampleRate cdecl alias "fb_sfxOutputSampleRate" _
		( _
		) as long

	''
	'' RawWrite()
	''
	'' Writes interleaved floating-point samples to the runtime output queue.
	'' Samples are clamped to [-1.0, 1.0]. The function returns the number of
	'' frames accepted, 0 when the queue is full, or -1 on invalid input or
	'' initialization failure. RawOpen() should normally be called first so
	'' the producer can use the device's actual sample rate. RawWrite() opens
	'' a stream implicitly for compatibility if necessary.
	''
	declare function RawWrite cdecl alias "fb_sfxRawWrite" _
		( _
			byval samples as const single ptr, _
			byval frames as long, _
			byval channels as long _
		) as long

	''
	'' OutputCaptureStart()
	''
	'' Starts recording the final output accepted by the active sfxlib
	'' driver. This records playback output, not input-device capture.
	''
	declare function OutputCaptureStart cdecl alias "fb_sfxOutputCaptureStart" _
		( _
		) as long

	''
	'' OutputCaptureReserve()
	''
	'' Reserves storage for at least the requested number of output frames.
	'' Reserving is optional, but helps avoid allocation work during a known
	'' length export.
	''
	declare function OutputCaptureReserve cdecl alias "fb_sfxOutputCaptureReserve" _
		( _
			byval frames as long _
		) as long

	''
	'' OutputCaptureStop()
	''
	'' Stops output recording. Save after stopping so the WAV file is written
	'' from a stable buffer.
	''
	declare sub OutputCaptureStop cdecl alias "fb_sfxOutputCaptureStop" _
		( _
		)

	''
	'' OutputCaptureSave()
	''
	'' Saves the recorded output as 16-bit PCM WAV. Returns 0 on success.
	''
	declare function OutputCaptureSave cdecl alias "fb_sfxOutputCaptureSave" _
		( _
			byval filename as const zstring ptr _
		) as long

	end extern

end namespace

'' end of sfxlib_raw.bi
