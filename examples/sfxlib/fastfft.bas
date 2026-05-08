'' fastfft.bas
''
'' Analyze a plain-text sample dump with a radix-2 FFT.
''
'' Expected input: one mono floating-point sample per line.  This matches the
'' optional sfxlib mixer dump format:
''   SFXLIB_MIXER_DUMP=/tmp/sfx-mix.txt SFXLIB_MIXER_DUMP_FRAMES=44100 ./program
''   fbc examples/sfxlib/fastfft.bas -x /tmp/fastfft
''   /tmp/fastfft 44100 /tmp/sfx-mix.txt
''
'' Optional expected frequencies may be listed after the filename.  Each one
'' is checked against the FFT bins and causes a non-zero exit if it is not
'' present above the local magnitude threshold:
''   /tmp/fastfft 48000 /tmp/sfx-mix.txt 261.63 329.63

const MAX_FFT_SAMPLES as integer = 65536
const MAX_EXPECTED_PEAKS as integer = 16

declare function IsNumericArg( byref text as string ) as integer
declare function PreviousPowerOfTwo( byval value as integer ) as integer
declare sub ReadSamples( byref filename as string, samples() as double, byref count as integer )
declare sub RunFft( real_part() as double, imag_part() as double, byval n as integer )

dim as integer sample_rate = 44100
dim as string filename = ""
dim as integer expected_start_arg = 2
dim expected_hz( 0 to MAX_EXPECTED_PEAKS - 1 ) as double
dim as integer expected_count = 0

if( command( 1 ) <> "" ) then
	if( IsNumericArg( command( 1 ) ) ) then
		sample_rate = valint( command( 1 ) )
		filename = command( 2 )
		expected_start_arg = 3
	else
		filename = command( 1 )
		expected_start_arg = 2
	end if
end if

if( sample_rate <= 0 ) then sample_rate = 44100

for arg_index as integer = expected_start_arg to 64
	dim as string text = command( arg_index )

	if( text = "" ) then exit for
	if( expected_count >= MAX_EXPECTED_PEAKS ) then exit for

	expected_hz( expected_count ) = val( text )
	if( expected_hz( expected_count ) > 0.0 ) then
		expected_count += 1
	end if
next

dim samples( 0 to MAX_FFT_SAMPLES - 1 ) as double
dim as integer sample_count = 0

ReadSamples( filename, samples(), sample_count )

if( sample_count < 16 ) then
	print "Need at least 16 samples."
	end 1
end if

dim as integer fft_size = PreviousPowerOfTwo( sample_count )
if( fft_size > MAX_FFT_SAMPLES ) then fft_size = MAX_FFT_SAMPLES

dim real_part( 0 to fft_size - 1 ) as double
dim imag_part( 0 to fft_size - 1 ) as double

dim as double peak = 0.0
dim as double sum_squares = 0.0
dim as double dc = 0.0
dim as double pi = 4.0 * atn( 1.0 )

for i as integer = 0 to fft_size - 1
	dim as double value = samples( i )
	dim as double magnitude = abs( value )
	dim as double window_value = 0.5 - 0.5 * cos( (2.0 * pi * i) / (fft_size - 1) )

	if( magnitude > peak ) then peak = magnitude
	sum_squares += value * value
	dc += value

	real_part( i ) = value * window_value
	imag_part( i ) = 0.0
next

RunFft( real_part(), imag_part(), fft_size )

dim as integer best_bin = 0
dim as double best_mag = 0.0
dim bin_mag( 0 to (fft_size \ 2) - 1 ) as double
dim top_bin( 0 to 7 ) as integer
dim top_mag( 0 to 7 ) as double

for bin_index as integer = 1 to (fft_size \ 2) - 1
	dim as double mag = sqr( real_part( bin_index ) * real_part( bin_index ) + imag_part( bin_index ) * imag_part( bin_index ) )
	mag = mag / (fft_size * 0.25)
	bin_mag( bin_index ) = mag

	if( mag > best_mag ) then
		best_mag = mag
		best_bin = bin_index
	end if

	for slot as integer = 0 to 7
		if( mag > top_mag( slot ) ) then
			for move_slot as integer = 7 to slot + 1 step -1
				top_mag( move_slot ) = top_mag( move_slot - 1 )
				top_bin( move_slot ) = top_bin( move_slot - 1 )
			next
			top_mag( slot ) = mag
			top_bin( slot ) = bin_index
			exit for
		end if
	next
next

print "samples read:"; sample_count
print "fft size:"; fft_size
print "sample rate:"; sample_rate
print "bin width hz:"; csng( sample_rate / fft_size )
print "peak:"; csng( peak )
print "rms:"; csng( sqr( sum_squares / fft_size ) )
print "dc:"; csng( dc / fft_size )
print "dominant bin:"; best_bin
print "dominant hz:"; csng( (best_bin * sample_rate) / fft_size )
print "dominant magnitude:"; csng( best_mag )
print
print "top bins:"

for slot as integer = 0 to 7
	if( top_bin( slot ) > 0 ) then
		print using "#######  ########.###  #.######"; _
			top_bin( slot ); _
			(top_bin( slot ) * sample_rate) / fft_size; _
			top_mag( slot )
	end if
next

if( expected_count > 0 ) then
	dim as integer matched_count = 0
	dim as double bin_width = sample_rate / fft_size
	dim as double magnitude_floor = best_mag * 0.15

	if( magnitude_floor < 0.000001 ) then
		magnitude_floor = 0.000001
	end if

	print
	print "expected peaks:"

	for expected_index as integer = 0 to expected_count - 1
		dim as double target_hz = expected_hz( expected_index )
		dim as double tolerance_hz = bin_width * 3.0
		dim as integer first_bin
		dim as integer last_bin
		dim as integer matched_bin = 0
		dim as double matched_mag = 0.0
		dim as double matched_hz = 0.0
		dim as string result_text = "miss"

		if( tolerance_hz < target_hz * 0.015 ) then
			tolerance_hz = target_hz * 0.015
		end if

		first_bin = cint( (target_hz - tolerance_hz) / bin_width )
		last_bin = cint( (target_hz + tolerance_hz) / bin_width )

		if( first_bin < 1 ) then first_bin = 1
		if( last_bin >= (fft_size \ 2) ) then last_bin = (fft_size \ 2) - 1

		for bin_index as integer = first_bin to last_bin
			if( bin_mag( bin_index ) > matched_mag ) then
				matched_mag = bin_mag( bin_index )
				matched_bin = bin_index
			end if
		next

		if( matched_bin > 0 ) then
			matched_hz = (matched_bin * sample_rate) / fft_size
		end if

		if( matched_mag >= magnitude_floor ) then
			result_text = "ok"
			matched_count += 1
		end if

		print using "target ########.###  matched ########.###  bin #######  mag #.######  &"; _
			target_hz; _
			matched_hz; _
			matched_bin; _
			matched_mag; _
			result_text
	next

	print "expected peaks matched:"; matched_count; "/"; expected_count

	if( matched_count <> expected_count ) then
		end 2
	end if
end if

function IsNumericArg( byref text as string ) as integer
	if( len( text ) = 0 ) then return 0

	for i as integer = 1 to len( text )
		dim as integer ch = asc( mid( text, i, 1 ) )

		if( ch < asc( "0" ) or ch > asc( "9" ) ) then
			return 0
		end if
	next

	return 1
end function

function PreviousPowerOfTwo( byval value as integer ) as integer
	dim as integer result = 1

	while( result <= value \ 2 )
		result *= 2
	wend

	return result
end function

sub ReadSamples( byref filename as string, samples() as double, byref count as integer )
	dim as integer file_num = freefile()
	dim as string line_text

	count = 0

	if( filename <> "" ) then
		if( open( filename for input as #file_num ) <> 0 ) then
			print "Unable to open "; filename
			end 1
		end if

		while( not eof( file_num ) and count < MAX_FFT_SAMPLES )
			line input #file_num, line_text
			line_text = trim( line_text )

			if( line_text <> "" and left( line_text, 1 ) <> "#" ) then
				samples( count ) = val( line_text )
				count += 1
			end if
		wend

		close #file_num
	else
		while( not eof( 0 ) and count < MAX_FFT_SAMPLES )
			line input line_text
			line_text = trim( line_text )

			if( line_text <> "" and left( line_text, 1 ) <> "#" ) then
				samples( count ) = val( line_text )
				count += 1
			end if
		wend
	end if
end sub

sub RunFft( real_part() as double, imag_part() as double, byval n as integer )
	dim as double pi = 4.0 * atn( 1.0 )
	dim as integer j = 0

	for i as integer = 1 to n - 1
		dim as integer bit_value = n \ 2

		while( (j and bit_value) <> 0 )
			j xor= bit_value
			bit_value \= 2
		wend

		j xor= bit_value

		if( i < j ) then
			swap real_part( i ), real_part( j )
			swap imag_part( i ), imag_part( j )
		end if
	next

	dim as integer length = 2
	while( length <= n )
		dim as double angle = -2.0 * pi / length
		dim as double wlen_real = cos( angle )
		dim as double wlen_imag = sin( angle )

		for block_start as integer = 0 to n - 1 step length
			dim as double w_real = 1.0
			dim as double w_imag = 0.0

			for offset as integer = 0 to (length \ 2) - 1
				dim as integer even_index = block_start + offset
				dim as integer odd_index = even_index + (length \ 2)
				dim as double odd_real = real_part( odd_index ) * w_real - imag_part( odd_index ) * w_imag
				dim as double odd_imag = real_part( odd_index ) * w_imag + imag_part( odd_index ) * w_real
				dim as double even_real = real_part( even_index )
				dim as double even_imag = imag_part( even_index )

				real_part( even_index ) = even_real + odd_real
				imag_part( even_index ) = even_imag + odd_imag
				real_part( odd_index ) = even_real - odd_real
				imag_part( odd_index ) = even_imag - odd_imag

				dim as double next_w_real = w_real * wlen_real - w_imag * wlen_imag
				dim as double next_w_imag = w_real * wlen_imag + w_imag * wlen_real

				w_real = next_w_real
				w_imag = next_w_imag
			next
		next

		length *= 2
	wend
end sub
