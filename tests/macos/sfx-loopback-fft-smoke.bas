''
'' FreeBASIC macOS smoke tests
'' ---------------------------
''
'' File: sfx-loopback-fft-smoke.bas
''
'' Purpose:
''
''     Exercise the Darwin CoreAudio playback and capture paths together
''     through a macOS loopback audio device.
''
'' Responsibilities:
''
''     - play one-note and two-note sound cases through CoreAudio
''     - capture the looped-back signal through the generic capture API
''     - use an FFT to verify the expected tone frequencies
''     - check for low-energy gaps and high-frequency energy that would
''       indicate stutters, discontinuities, or buffer underruns
''
'' This file intentionally does NOT contain:
''
''     - virtual audio driver installation
''     - platform device selection outside sfxlib
''     - subjective listening checks
''

#include once "../sfx/sfx_test_common.bi"

declare function fb_sfxControlDeviceList cdecl alias "fb_sfxControlDeviceList" ( ) as integer
declare function fb_sfxControlDeviceSelect cdecl alias "fb_sfxControlDeviceSelect" ( byval device as integer ) as integer

const SKIP_NO_COREAUDIO_LOOPBACK = 77
const LOOPBACK_SAMPLE_RATE = 44100.0
const LOOPBACK_CHANNELS = 2
const CAPTURE_FRAMES = 65536
const READ_CHUNK_FRAMES = 1024
const FFT_FRAMES = 16384
const FFT_BAND_BINS = 8
const PLAY_SECONDS = 1.00

type LoopbackMetrics
	active_start as integer
	active_rms as double
	low_windows as integer
	max_step as double
	high_ratio as double
	peak_frequency as double
	target1_peak as double
	target2_peak as double
	target1_power as double
	target2_power as double
	reference_power as double
end type

dim shared as double g_fft_re(0 to FFT_FRAMES - 1)
dim shared as double g_fft_im(0 to FFT_FRAMES - 1)
dim shared as double g_capture(0 to CAPTURE_FRAMES - 1)
dim shared as single g_read_buffer(0 to (READ_CHUNK_FRAMES * LOOPBACK_CHANNELS) - 1)

sub Fail( byref message as string )
	print "sfx-loopback-fft: failed: "; message
	end 1
end sub

sub Skip( byref message as string )
	print "sfx-loopback-fft: skipped: "; message
	end SKIP_NO_COREAUDIO_LOOPBACK
end sub

function AbsDouble( byval value as double ) as double
	if( value < 0.0 ) then
		return -value
	end if

	return value
end function

function MinInt( byval a as integer, byval b as integer ) as integer
	if( a < b ) then
		return a
	end if

	return b
end function

function BinForFrequency( byval frequency as double ) as integer
	return cint( ( frequency * cdbl( FFT_FRAMES ) ) / LOOPBACK_SAMPLE_RATE )
end function

function FrequencyForBin( byval fft_bin as integer ) as double
	return ( cdbl( fft_bin ) * LOOPBACK_SAMPLE_RATE ) / cdbl( FFT_FRAMES )
end function

function WindowRms _
	( _
		samples() as double, _
		byval first_frame as integer, _
		byval frames as integer _
	) as double

	dim as integer i
	dim as double sum_squares = 0.0

	if( frames <= 0 ) then
		return 0.0
	end if

	for i = 0 to frames - 1
		dim as double sample = samples( first_frame + i )
		sum_squares += sample * sample
	next

	return sqr( sum_squares / cdbl( frames ) )
end function

function FindActiveStart _
	( _
		samples() as double, _
		byval frames as integer, _
		byref best_rms as double _
	) as integer

	dim as integer first_frame
	dim as integer best_start = -1

	best_rms = 0.0

	if( frames < FFT_FRAMES ) then
		return -1
	end if

	for first_frame = 0 to frames - FFT_FRAMES step 1024
		dim as double rms = WindowRms( samples(), first_frame, FFT_FRAMES )

		if( rms > best_rms ) then
			best_rms = rms
			best_start = first_frame
		end if
	next

	return best_start
end function

function CountLowEnergyWindows _
	( _
		samples() as double, _
		byval first_frame as integer, _
		byval frames as integer, _
		byval window_frames as integer, _
		byval minimum_rms as double _
	) as integer

	dim as integer count = 0
	dim as integer offset = 0

	while( offset + window_frames <= frames )
		if( WindowRms( samples(), first_frame + offset, window_frames ) < minimum_rms ) then
			count += 1
		end if

		offset += window_frames
	wend

	return count
end function

function MaxStep _
	( _
		samples() as double, _
		byval first_frame as integer, _
		byval frames as integer _
	) as double

	dim as integer i
	dim as double largest = 0.0

	for i = 1 to frames - 1
		dim as double delta = AbsDouble( samples( first_frame + i ) - samples( first_frame + i - 1 ) )

		if( delta > largest ) then
			largest = delta
		end if
	next

	return largest
end function

sub BuildFft _
	( _
		samples() as double, _
		byval first_frame as integer _
	)

	dim as integer i
	dim as integer j
	dim as integer bit_mask
	dim as integer span
	dim as double mean = 0.0

	for i = 0 to FFT_FRAMES - 1
		mean += samples( first_frame + i )
	next

	mean /= cdbl( FFT_FRAMES )

	for i = 0 to FFT_FRAMES - 1
		dim as double window_scale = 0.5 - 0.5 * cos( ( 2.0 * SFX_TEST_PI * cdbl( i ) ) / cdbl( FFT_FRAMES - 1 ) )

		g_fft_re( i ) = ( samples( first_frame + i ) - mean ) * window_scale
		g_fft_im( i ) = 0.0
	next

	j = 0
	for i = 1 to FFT_FRAMES - 1
		bit_mask = FFT_FRAMES \ 2

		while( ( j and bit_mask ) <> 0 )
			j xor= bit_mask
			bit_mask \= 2
		wend

		j xor= bit_mask

		if( i < j ) then
			swap g_fft_re( i ), g_fft_re( j )
			swap g_fft_im( i ), g_fft_im( j )
		end if
	next

	span = 2
	while( span <= FFT_FRAMES )
		dim as double angle = -2.0 * SFX_TEST_PI / cdbl( span )
		dim as double step_re = cos( angle )
		dim as double step_im = sin( angle )
		dim as integer first

		for first = 0 to FFT_FRAMES - 1 step span
			dim as double walk_re = 1.0
			dim as double walk_im = 0.0

			for j = 0 to ( span \ 2 ) - 1
				dim as integer even_bin = first + j
				dim as integer odd_bin = even_bin + ( span \ 2 )
				dim as double odd_re = ( g_fft_re( odd_bin ) * walk_re ) - ( g_fft_im( odd_bin ) * walk_im )
				dim as double odd_im = ( g_fft_re( odd_bin ) * walk_im ) + ( g_fft_im( odd_bin ) * walk_re )
				dim as double even_re = g_fft_re( even_bin )
				dim as double even_im = g_fft_im( even_bin )

				g_fft_re( even_bin ) = even_re + odd_re
				g_fft_im( even_bin ) = even_im + odd_im
				g_fft_re( odd_bin ) = even_re - odd_re
				g_fft_im( odd_bin ) = even_im - odd_im

				dim as double next_re = ( walk_re * step_re ) - ( walk_im * step_im )
				dim as double next_im = ( walk_re * step_im ) + ( walk_im * step_re )

				walk_re = next_re
				walk_im = next_im
			next
		next

		span *= 2
	wend
end sub

function FftPowerBin( byval fft_bin as integer ) as double
	if( fft_bin < 0 or fft_bin >= FFT_FRAMES \ 2 ) then
		return 0.0
	end if

	return ( g_fft_re( fft_bin ) * g_fft_re( fft_bin ) ) + ( g_fft_im( fft_bin ) * g_fft_im( fft_bin ) )
end function

function FftBandPower( byval frequency as double, byval bins_on_each_side as integer ) as double
	dim as integer center = BinForFrequency( frequency )
	dim as integer first_bin = center - bins_on_each_side
	dim as integer last_bin = center + bins_on_each_side
	dim as integer fft_bin
	dim as double total = 0.0

	if( first_bin < 1 ) then
		first_bin = 1
	end if

	if( last_bin >= FFT_FRAMES \ 2 ) then
		last_bin = ( FFT_FRAMES \ 2 ) - 1
	end if

	for fft_bin = first_bin to last_bin
		total += FftPowerBin( fft_bin )
	next

	return total
end function

function FftRangePower( byval low_frequency as double, byval high_frequency as double ) as double
	dim as integer first_bin = BinForFrequency( low_frequency )
	dim as integer last_bin = BinForFrequency( high_frequency )
	dim as integer fft_bin
	dim as double total = 0.0

	if( first_bin < 1 ) then
		first_bin = 1
	end if

	if( last_bin >= FFT_FRAMES \ 2 ) then
		last_bin = ( FFT_FRAMES \ 2 ) - 1
	end if

	for fft_bin = first_bin to last_bin
		total += FftPowerBin( fft_bin )
	next

	return total
end function

function FftPeakFrequency( byval low_frequency as double, byval high_frequency as double ) as double
	dim as integer first_bin = BinForFrequency( low_frequency )
	dim as integer last_bin = BinForFrequency( high_frequency )
	dim as integer fft_bin
	dim as integer peak_bin = first_bin
	dim as double peak_power = 0.0

	if( first_bin < 1 ) then
		first_bin = 1
	end if

	if( last_bin >= FFT_FRAMES \ 2 ) then
		last_bin = ( FFT_FRAMES \ 2 ) - 1
	end if

	for fft_bin = first_bin to last_bin
		dim as double power = FftPowerBin( fft_bin )

		if( power > peak_power ) then
			peak_power = power
			peak_bin = fft_bin
		end if
	next

	return FrequencyForBin( peak_bin )
end function

sub ReadCapturedFrames _
	( _
		samples() as double, _
		byref frames_read as integer _
	)

	dim as integer wanted
	dim as integer got
	dim as integer frame

	do
		wanted = MinInt( READ_CHUNK_FRAMES, CAPTURE_FRAMES - frames_read )
		if( wanted <= 0 ) then
			exit do
		end if

		got = fb_sfxCaptureReadSamples( @g_read_buffer(0), wanted )
		if( got < 0 ) then
			fb_sfxCaptureStop()
			Fail( "capture read failed" )
		end if

		if( got = 0 ) then
			exit do
		end if

		for frame = 0 to got - 1
			samples( frames_read + frame ) = _
				( cdbl( g_read_buffer( ( frame * LOOPBACK_CHANNELS ) ) ) + _
				  cdbl( g_read_buffer( ( frame * LOOPBACK_CHANNELS ) + 1 ) ) ) * 0.5
		next

		frames_read += got
	loop
end sub

sub DrainCapture()
	dim as integer frames_read = 0
	ReadCapturedFrames( g_capture(), frames_read )
end sub

function CaptureTone _
	( _
		samples() as double, _
		byval first_frequency as integer, _
		byval second_frequency as integer _
	) as integer

	dim as integer frames_read = 0
	dim as double started

	erase samples

	if( fb_sfxCaptureStart() <> 0 ) then
		return -1
	end if

	sleep 150, 1
	DrainCapture()

	sound 0, first_frequency, PLAY_SECONDS, 0.45
	if( second_frequency > 0 ) then
		sound 1, second_frequency, PLAY_SECONDS, 0.45
	end if

	started = timer
	do
		fb_sfxUpdate 1024
		sleep 10, 1
		ReadCapturedFrames( samples(), frames_read )
	loop while( ( timer - started ) < 1.20 and frames_read < CAPTURE_FRAMES )

	sleep 150, 1
	ReadCapturedFrames( samples(), frames_read )
	fb_sfxCaptureStop()

	return frames_read
end function

sub AnalyzeCapture _
	( _
		byref case_name as string, _
		samples() as double, _
		byval frames as integer, _
		byval first_frequency as double, _
		byval second_frequency as double, _
		byref metrics as LoopbackMetrics _
	)

	dim as double total_power
	dim as double high_power

	if( frames < FFT_FRAMES ) then
		Fail( case_name + " captured too few frames" )
	end if

	metrics.active_start = FindActiveStart( samples(), frames, metrics.active_rms )
	if( metrics.active_start < 0 ) then
		Fail( case_name + " did not capture a complete analysis window" )
	end if

	if( metrics.active_rms < 0.03 ) then
		Fail( case_name + " loopback signal is too quiet" )
	end if

	metrics.low_windows = CountLowEnergyWindows( samples(), metrics.active_start, FFT_FRAMES, 512, metrics.active_rms * 0.20 )
	metrics.max_step = MaxStep( samples(), metrics.active_start, FFT_FRAMES )

	BuildFft( samples(), metrics.active_start )

	metrics.target1_power = FftBandPower( first_frequency, FFT_BAND_BINS )
	metrics.target2_power = 0.0
	metrics.reference_power = FftBandPower( 550.0, FFT_BAND_BINS )
	metrics.peak_frequency = FftPeakFrequency( 200.0, 1200.0 )
	metrics.target1_peak = FftPeakFrequency( first_frequency - 25.0, first_frequency + 25.0 )
	metrics.target2_peak = 0.0

	if( second_frequency > 0.0 ) then
		metrics.target2_power = FftBandPower( second_frequency, FFT_BAND_BINS )
		metrics.target2_peak = FftPeakFrequency( second_frequency - 25.0, second_frequency + 25.0 )
	end if

	total_power = FftRangePower( 80.0, 20000.0 )
	high_power = FftRangePower( 8000.0, 20000.0 )

	if( total_power <= 0.0 ) then
		Fail( case_name + " FFT produced no usable signal power" )
	end if

	metrics.high_ratio = high_power / total_power
end sub

sub PrintMetrics _
	( _
		byref case_name as string, _
		byval frames as integer, _
		byref metrics as LoopbackMetrics _
	)

	print "sfx-loopback-fft: "; case_name; _
		" frames="; frames; _
		" active_start="; metrics.active_start; _
		" rms="; metrics.active_rms; _
		" peak_hz="; metrics.peak_frequency; _
		" p1_peak_hz="; metrics.target1_peak; _
		" p2_peak_hz="; metrics.target2_peak; _
		" p1="; metrics.target1_power; _
		" p2="; metrics.target2_power; _
		" pref="; metrics.reference_power; _
		" high_ratio="; metrics.high_ratio; _
		" low_windows="; metrics.low_windows; _
		" max_step="; metrics.max_step
end sub

sub CheckOneNote( byref metrics as LoopbackMetrics )
	if( AbsDouble( metrics.target1_peak - 440.0 ) > 20.0 ) then
		Fail( "one-note 440 Hz peak is outside the expected band" )
	end if

	if( metrics.target1_power < metrics.reference_power * 6.0 ) then
		Fail( "one-note 440 Hz band is not dominant" )
	end if

	if( metrics.high_ratio > 0.08 ) then
		Fail( "one-note high-frequency energy suggests stutter or discontinuity" )
	end if

	if( metrics.low_windows > 0 ) then
		Fail( "one-note capture contains low-energy gaps" )
	end if
end sub

sub CheckTwoNotes( byref metrics as LoopbackMetrics )
	if( AbsDouble( metrics.target1_peak - 440.0 ) > 20.0 ) then
		Fail( "two-note 440 Hz peak is outside the expected band" )
	end if

	if( AbsDouble( metrics.target2_peak - 660.0 ) > 20.0 ) then
		Fail( "two-note 660 Hz peak is outside the expected band" )
	end if

	if( metrics.target1_power < metrics.reference_power * 2.0 ) then
		Fail( "two-note 440 Hz band is not clearly present" )
	end if

	if( metrics.target2_power < metrics.reference_power * 2.0 ) then
		Fail( "two-note 660 Hz band is not clearly present" )
	end if

	if( metrics.target1_power < metrics.target2_power * 0.20 or _
	    metrics.target2_power < metrics.target1_power * 0.20 ) then
		Fail( "two-note tones are badly imbalanced" )
	end if

	if( metrics.high_ratio > 0.10 ) then
		Fail( "two-note high-frequency energy suggests stutter or discontinuity" )
	end if

	if( metrics.low_windows > 0 ) then
		Fail( "two-note capture contains low-energy gaps" )
	end if
end sub

setenviron "SFXLIB_DRIVER=CoreAudio"

dim as integer current = fb_sfxDeviceCurrent()
if( current < 0 ) then
	Skip( "CoreAudio driver is not available" )
end if

dim as zstring ptr driver_name = fb_sfxDeviceName( current )
if( driver_name = 0 ) then
	Skip( "CoreAudio driver name is not available" )
end if

if( lcase( *driver_name ) <> "coreaudio" ) then
	Skip( "CoreAudio driver is not active" )
end if

if( fb_sfxControlDeviceList() <= 0 ) then
	Skip( "no CoreAudio output device is available" )
end if

if( fb_sfxControlDeviceSelect( 0 ) <> 0 ) then
	Fail( "failed to select first CoreAudio output device" )
end if

dim as LoopbackMetrics one_note
dim as LoopbackMetrics two_notes
dim as integer one_note_frames = CaptureTone( g_capture(), 440, 0 )

if( one_note_frames < 0 ) then
	Skip( "CoreAudio capture is not available" )
end if

AnalyzeCapture( "one-note", g_capture(), one_note_frames, 440.0, 0.0, one_note )
PrintMetrics( "one-note", one_note_frames, one_note )
CheckOneNote( one_note )

dim as integer two_note_frames = CaptureTone( g_capture(), 440, 660 )

if( two_note_frames < 0 ) then
	Skip( "CoreAudio capture is not available" )
end if

AnalyzeCapture( "two-note", g_capture(), two_note_frames, 440.0, 660.0, two_notes )
PrintMetrics( "two-note", two_note_frames, two_notes )
CheckTwoNotes( two_notes )

print "sfx-loopback-fft: passed"
end 0

'' end of sfx-loopback-fft-smoke.bas
