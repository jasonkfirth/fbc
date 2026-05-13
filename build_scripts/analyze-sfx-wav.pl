#!/usr/bin/env perl

#
# FreeBASIC DOS sfxlib audio validation
# -------------------------------------
#
# File: analyze-sfx-wav.pl
#
# Purpose:
#
#     Analyze a DOSBox-X WAV capture from sfxlib runtime tests.
#
# Responsibilities:
#
#     - read PCM WAV files produced by DOSBox-X
#     - downmix multi-channel captures to mono
#     - trim leading and trailing silence
#     - run a radix-2 FFT over the active audio
#     - verify expected tone frequencies
#     - detect low-energy windows inside the active audio
#
# This file intentionally does NOT contain:
#
#     - DOSBox-X launch logic
#     - FreeBASIC package staging
#     - platform audio driver code
#

use strict;
use warnings;
use Getopt::Long;

my @expected_hz;
my $label = 'audio';
my $tolerance_hz = 8.0;
my $minimum_active_seconds = 0.65;
my $window_ms = 10.0;
my $gap_ratio = 0.18;
my $maximum_gap_ms = 0.0;
my $minimum_peak_ratio = 0.08;
my $maximum_transient_ms = 0.0;
my $maximum_unexpected_ratio = -1.0;
my $allowed_harmonics = 1;
my $unexpected_min_hz = 20.0;
my $unexpected_max_hz = 12000.0;

GetOptions(
	'expect=f@' => \@expected_hz,
	'label=s' => \$label,
	'tolerance-hz=f' => \$tolerance_hz,
	'min-active-seconds=f' => \$minimum_active_seconds,
	'window-ms=f' => \$window_ms,
	'gap-ratio=f' => \$gap_ratio,
	'max-gap-ms=f' => \$maximum_gap_ms,
	'min-peak-ratio=f' => \$minimum_peak_ratio,
	'allow-edge-transient-ms=f' => \$maximum_transient_ms,
	'max-unexpected-ratio=f' => \$maximum_unexpected_ratio,
	'allowed-harmonics=i' => \$allowed_harmonics,
	'unexpected-min-hz=f' => \$unexpected_min_hz,
	'unexpected-max-hz=f' => \$unexpected_max_hz,
) or usage();

my $filename = shift @ARGV;
usage() unless defined $filename;

my ($sample_rate, $channels, $bits_per_sample, $samples) = read_wav($filename);
my $frame_count = scalar(@$samples);
die "no samples found in $filename\n" if $frame_count <= 0;

my $peak = 0.0;
for my $sample (@$samples) {
	my $abs = abs($sample);
	$peak = $abs if $abs > $peak;
}
die "$label: capture is silent\n" if $peak < 0.000001;

my $window_frames = int(($sample_rate * $window_ms) / 1000.0);
$window_frames = 1 if $window_frames < 1;

my @window_rms;
for (my $first = 0; $first + $window_frames <= $frame_count; $first += $window_frames) {
	push @window_rms, rms($samples, $first, $window_frames);
}

die "$label: not enough audio windows for analysis\n" if @window_rms < 4;

my $active_threshold = $peak * 0.03;
$active_threshold = 0.00001 if $active_threshold < 0.00001;

my @active_spans = find_active_spans(\@window_rms, $active_threshold);
die "$label: no active audio found\n" unless @active_spans;

my $transient_windows = int($maximum_transient_ms / $window_ms);
my $ignored_transient_spans = 0;

$ignored_transient_spans = trim_edge_transient_spans(\@active_spans, $transient_windows)
	if $transient_windows > 0;
die "$label: no sustained active audio found\n" unless @active_spans;

my $first_active_window = $active_spans[0]->[0];
my $last_active_window = $active_spans[-1]->[1];

my $active_start = $first_active_window * $window_frames;
my $active_end = ($last_active_window + 1) * $window_frames;
$active_end = $frame_count if $active_end > $frame_count;

my $active_frames = $active_end - $active_start;
my $active_seconds = $active_frames / $sample_rate;
die sprintf("%s: active audio too short: %.3fs\n", $label, $active_seconds)
	if $active_seconds < $minimum_active_seconds;

my $gap_first_window = $first_active_window;
my $gap_last_window = $last_active_window;
my $edge_gap_windows = 0;

if ($transient_windows > 0) {
	my $active_window_count = $gap_last_window - $gap_first_window + 1;
	if ($active_window_count > ($transient_windows * 2) + 4) {
		$gap_first_window += $transient_windows;
		$gap_last_window -= $transient_windows;
		$edge_gap_windows = $transient_windows * 2;
	}
}

my @core_windows = core_windows(\@window_rms, $gap_first_window, $gap_last_window);
my $median_rms = median(@core_windows);
my $gap_threshold = $median_rms * $gap_ratio;
$gap_threshold = 0.00001 if $gap_threshold < 0.00001;

my ($low_windows, $max_low_run) = count_low_windows(\@core_windows, $gap_threshold);
my $max_gap_ms = $max_low_run * $window_ms;
die sprintf("%s: low-energy gap detected: %.1fms (%d windows below %.6f)\n",
	$label, $max_gap_ms, $low_windows, $gap_threshold)
	if $max_gap_ms > $maximum_gap_ms;

my $fft_size = previous_power_of_two($active_frames);
$fft_size = 65536 if $fft_size > 65536;
die "$label: active audio too short for FFT\n" if $fft_size < 2048;

my $fft_start = $active_start + int(($active_frames - $fft_size) / 2);
my ($magnitudes, $dominant_bin, $dominant_mag) = fft_magnitudes($samples, $fft_start, $fft_size);
my $bin_width = $sample_rate / $fft_size;
my $dominant_hz = $dominant_bin * $bin_width;
my ($allowed_power,
	$unexpected_power,
	$unexpected_ratio,
	$top_unexpected) = spectral_purity($magnitudes,
	                                   $sample_rate,
	                                   $fft_size,
	                                   \@expected_hz,
	                                   $allowed_harmonics,
	                                   $tolerance_hz,
	                                   $unexpected_min_hz,
	                                   $unexpected_max_hz);

print "label: $label\n";
print "wav: $filename\n";
print "sample rate: $sample_rate\n";
print "channels: $channels\n";
print "bits per sample: $bits_per_sample\n";
print "frames: $frame_count\n";
printf "active seconds: %.3f\n", $active_seconds;
print "active spans: " . scalar(@active_spans) . "\n";
print "ignored transient spans: $ignored_transient_spans\n";
print "ignored edge gap windows: $edge_gap_windows\n";
print "fft size: $fft_size\n";
printf "bin width hz: %.6f\n", $bin_width;
printf "peak: %.6f\n", $peak;
printf "median rms: %.6f\n", $median_rms;
printf "gap threshold rms: %.6f\n", $gap_threshold;
print "low-energy windows: $low_windows\n";
printf "max low-energy run ms: %.1f\n", $max_gap_ms;
printf "dominant hz: %.3f\n", $dominant_hz;
printf "dominant magnitude: %.6f\n", $dominant_mag;
printf "allowed spectral power: %.6f\n", $allowed_power;
printf "unexpected spectral power: %.6f\n", $unexpected_power;
printf "unexpected spectral ratio: %.6f\n", $unexpected_ratio;
print "top unexpected hz: " . join(", ", @$top_unexpected) . "\n" if @$top_unexpected;

my $failures = 0;
for my $expected (@expected_hz) {
	my $matched = match_expected_frequency($magnitudes, $sample_rate, $fft_size, $expected, $tolerance_hz);
	my ($matched_bin, $matched_hz, $matched_mag) = @$matched;
	my $ratio = ($dominant_mag > 0.0) ? ($matched_mag / $dominant_mag) : 0.0;

	printf "expected hz: %.3f matched hz: %.3f bin: %d magnitude: %.6f ratio: %.6f\n",
		$expected, $matched_hz, $matched_bin, $matched_mag, $ratio;

	if ($matched_bin <= 0 || $ratio < $minimum_peak_ratio) {
		++$failures;
	}
}

die "$label: expected FFT peak was not strong enough\n" if $failures;
die sprintf("%s: unexpected spectral energy too high: %.6f > %.6f\n",
	$label, $unexpected_ratio, $maximum_unexpected_ratio)
	if $maximum_unexpected_ratio >= 0.0 &&
	   $unexpected_ratio > $maximum_unexpected_ratio;
print "result: ok\n";
exit 0;

sub usage {
	die "usage: analyze-sfx-wav.pl [--expect HZ] [--label NAME] FILE.wav\n";
}

sub read_wav {
	my ($path) = @_;

	open my $fh, '<:raw', $path or die "unable to open $path: $!\n";
	local $/;
	my $data = <$fh>;
	close $fh;

	die "$path is too small to be a WAV file\n" if length($data) < 44;

	my ($riff, undef, $wave) = unpack('a4Va4', substr($data, 0, 12));
	die "$path is not a RIFF/WAVE file\n" unless $riff eq 'RIFF' && $wave eq 'WAVE';

	my $offset = 12;
	my ($format_tag, $channels, $sample_rate, $bits_per_sample);
	my $sample_data;

	while ($offset + 8 <= length($data)) {
		my ($chunk_id, $chunk_size) = unpack('a4V', substr($data, $offset, 8));
		$offset += 8;
		die "$path has a truncated chunk\n" if $offset + $chunk_size > length($data);

		my $chunk = substr($data, $offset, $chunk_size);
		if ($chunk_id eq 'fmt ') {
			die "$path has a truncated fmt chunk\n" if $chunk_size < 16;
			($format_tag, $channels, $sample_rate, undef, undef, $bits_per_sample) =
				unpack('vvVVvv', substr($chunk, 0, 16));
		} elsif ($chunk_id eq 'data') {
			$sample_data = $chunk;
		}

		$offset += $chunk_size;
		++$offset if $chunk_size & 1;
	}

	die "$path is missing a fmt chunk\n" unless defined $format_tag;
	die "$path is missing a data chunk\n" unless defined $sample_data;
	die "$path has unsupported channel count: $channels\n" if $channels <= 0;

	my $bytes_per_sample = int($bits_per_sample / 8);
	die "$path has unsupported sample size: $bits_per_sample\n" if $bytes_per_sample <= 0;

	my $bytes_per_frame = $bytes_per_sample * $channels;
	die "$path has invalid frame size\n" if $bytes_per_frame <= 0;

	my $frames = int(length($sample_data) / $bytes_per_frame);
	my @mono;
	for my $frame (0 .. $frames - 1) {
		my $sum = 0.0;
		my $frame_offset = $frame * $bytes_per_frame;

		for my $channel (0 .. $channels - 1) {
			my $sample_offset = $frame_offset + ($channel * $bytes_per_sample);
			$sum += decode_sample($sample_data, $sample_offset, $format_tag, $bits_per_sample);
		}

		push @mono, $sum / $channels;
	}

	return ($sample_rate, $channels, $bits_per_sample, \@mono);
}

sub decode_sample {
	my ($data, $offset, $format_tag, $bits) = @_;

	if ($format_tag == 1) {
		if ($bits == 8) {
			return (unpack('C', substr($data, $offset, 1)) - 128) / 128.0;
		}
		if ($bits == 16) {
			return unpack('s<', substr($data, $offset, 2)) / 32768.0;
		}
		if ($bits == 24) {
			my @b = unpack('C3', substr($data, $offset, 3));
			my $value = $b[0] | ($b[1] << 8) | ($b[2] << 16);
			$value -= 0x1000000 if $value & 0x800000;
			return $value / 8388608.0;
		}
		if ($bits == 32) {
			return unpack('l<', substr($data, $offset, 4)) / 2147483648.0;
		}
	} elsif ($format_tag == 3 && $bits == 32) {
		return unpack('f<', substr($data, $offset, 4));
	}

	die "unsupported WAV format tag=$format_tag bits=$bits\n";
}

sub rms {
	my ($samples, $first, $count) = @_;
	my $sum = 0.0;

	for my $i (0 .. $count - 1) {
		my $sample = $samples->[$first + $i] // 0.0;
		$sum += $sample * $sample;
	}

	return sqrt($sum / $count);
}

sub find_active_spans {
	my ($rms_values, $threshold) = @_;
	my @spans;
	my $first;

	for my $i (0 .. $#$rms_values) {
		if ($rms_values->[$i] >= $threshold) {
			$first = $i unless defined $first;
		} elsif (defined $first) {
			push @spans, [$first, $i - 1];
			undef $first;
		}
	}

	push @spans, [$first, $#$rms_values] if defined $first;
	return @spans;
}

sub trim_edge_transient_spans {
	my ($spans, $maximum_windows) = @_;
	my $ignored = 0;

	while (@$spans && span_windows($spans->[0]) <= $maximum_windows) {
		shift @$spans;
		++$ignored;
	}

	while (@$spans && span_windows($spans->[-1]) <= $maximum_windows) {
		pop @$spans;
		++$ignored;
	}

	return $ignored;
}

sub span_windows {
	my ($span) = @_;
	return ($span->[1] - $span->[0]) + 1;
}

sub core_windows {
	my ($rms_values, $first, $last) = @_;

	$first += 2 if $last - $first >= 6;
	$last -= 2 if $last - $first >= 6;

	my @result;
	for my $i ($first .. $last) {
		push @result, $rms_values->[$i];
	}

	return @result;
}

sub median {
	my @values = sort { $a <=> $b } @_;
	return 0.0 unless @values;
	return $values[int(@values / 2)] if @values & 1;
	return ($values[(@values / 2) - 1] + $values[@values / 2]) * 0.5;
}

sub count_low_windows {
	my ($values, $threshold) = @_;
	my $low = 0;
	my $run = 0;
	my $max_run = 0;

	for my $value (@$values) {
		if ($value < $threshold) {
			++$low;
			++$run;
			$max_run = $run if $run > $max_run;
		} else {
			$run = 0;
		}
	}

	return ($low, $max_run);
}

sub previous_power_of_two {
	my ($value) = @_;
	my $result = 1;
	$result <<= 1 while ($result << 1) <= $value;
	return $result;
}

sub fft_magnitudes {
	my ($samples, $first, $size) = @_;
	my @real;
	my @imag = (0.0) x $size;
	my $sum = 0.0;

	for my $i (0 .. $size - 1) {
		$sum += $samples->[$first + $i];
	}
	my $mean = $sum / $size;

	for my $i (0 .. $size - 1) {
		my $window = 0.5 - (0.5 * cos((2.0 * 3.14159265358979323846 * $i) / ($size - 1)));
		$real[$i] = ($samples->[$first + $i] - $mean) * $window;
	}

	run_fft(\@real, \@imag, $size);

	my @magnitudes = (0.0) x int($size / 2);
	my $dominant_bin = 0;
	my $dominant_mag = 0.0;
	my $min_audio_bin = int((20.0 * $size) / $sample_rate);
	my $max_low_bin = int((2000.0 * $size) / $sample_rate);
	$min_audio_bin = 1 if $min_audio_bin < 1;
	$max_low_bin = int($size / 2) - 1 if $max_low_bin >= int($size / 2);

	for my $bin (1 .. int($size / 2) - 1) {
		my $mag = sqrt(($real[$bin] * $real[$bin]) + ($imag[$bin] * $imag[$bin]));
		$mag /= ($size * 0.25);
		$magnitudes[$bin] = $mag;

		if ($bin >= $min_audio_bin && $bin <= $max_low_bin && $mag > $dominant_mag) {
			$dominant_mag = $mag;
			$dominant_bin = $bin;
		}
	}

	return (\@magnitudes, $dominant_bin, $dominant_mag);
}

sub run_fft {
	my ($real, $imag, $n) = @_;
	my $j = 0;

	for my $i (1 .. $n - 1) {
		my $bit = $n >> 1;
		while ($j & $bit) {
			$j ^= $bit;
			$bit >>= 1;
		}
		$j ^= $bit;

		if ($i < $j) {
			($real->[$i], $real->[$j]) = ($real->[$j], $real->[$i]);
			($imag->[$i], $imag->[$j]) = ($imag->[$j], $imag->[$i]);
		}
	}

	for (my $length = 2; $length <= $n; $length <<= 1) {
		my $angle = -2.0 * 3.14159265358979323846 / $length;
		my $wlen_real = cos($angle);
		my $wlen_imag = sin($angle);

		for (my $block = 0; $block < $n; $block += $length) {
			my $w_real = 1.0;
			my $w_imag = 0.0;

			for my $offset (0 .. int($length / 2) - 1) {
				my $even = $block + $offset;
				my $odd = $even + int($length / 2);
				my $odd_real = ($real->[$odd] * $w_real) - ($imag->[$odd] * $w_imag);
				my $odd_imag = ($real->[$odd] * $w_imag) + ($imag->[$odd] * $w_real);
				my $even_real = $real->[$even];
				my $even_imag = $imag->[$even];

				$real->[$even] = $even_real + $odd_real;
				$imag->[$even] = $even_imag + $odd_imag;
				$real->[$odd] = $even_real - $odd_real;
				$imag->[$odd] = $even_imag - $odd_imag;

				my $next_w_real = ($w_real * $wlen_real) - ($w_imag * $wlen_imag);
				my $next_w_imag = ($w_real * $wlen_imag) + ($w_imag * $wlen_real);
				$w_real = $next_w_real;
				$w_imag = $next_w_imag;
			}
		}
	}
}

sub match_expected_frequency {
	my ($magnitudes, $rate, $fft_size, $expected, $minimum_tolerance) = @_;
	my $bin_width = $rate / $fft_size;
	my $tolerance = $minimum_tolerance;
	my $scaled_tolerance = abs($expected) * 0.015;
	my $bin_tolerance = $bin_width * 3.0;

	$tolerance = $scaled_tolerance if $scaled_tolerance > $tolerance;
	$tolerance = $bin_tolerance if $bin_tolerance > $tolerance;

	my $first_bin = int(($expected - $tolerance) / $bin_width);
	my $last_bin = int(($expected + $tolerance) / $bin_width);
	$first_bin = 1 if $first_bin < 1;
	$last_bin = $#$magnitudes if $last_bin > $#$magnitudes;

	my $best_bin = 0;
	my $best_mag = 0.0;
	for my $bin ($first_bin .. $last_bin) {
		if ($magnitudes->[$bin] > $best_mag) {
			$best_mag = $magnitudes->[$bin];
			$best_bin = $bin;
		}
	}

	return [$best_bin, $best_bin * $bin_width, $best_mag];
}

sub spectral_purity {
	my ($magnitudes,
	    $rate,
	    $fft_size,
	    $expected_values,
	    $harmonics,
	    $minimum_tolerance,
	    $min_hz,
	    $max_hz) = @_;

	my $bin_width = $rate / $fft_size;
	my $nyquist = $rate / 2.0;
	my $allowed_power = 0.0;
	my $unexpected_power = 0.0;
	my @top;

	$harmonics = 1 if $harmonics < 1;
	$min_hz = 0.0 if $min_hz < 0.0;
	$max_hz = $nyquist if $max_hz <= 0.0 || $max_hz > $nyquist;

	for my $bin (1 .. $#$magnitudes) {
		my $hz = $bin * $bin_width;
		my $power;

		next if $hz < $min_hz || $hz > $max_hz;

		$power = $magnitudes->[$bin] * $magnitudes->[$bin];

		if (frequency_is_allowed($hz,
		                         $bin_width,
		                         $expected_values,
		                         $harmonics,
		                         $minimum_tolerance,
		                         $max_hz))
		{
			$allowed_power += $power;
		}
		else
		{
			$unexpected_power += $power;
			remember_top_unexpected(\@top, $hz, $magnitudes->[$bin]);
		}
	}

	my $ratio = ($allowed_power > 0.0)
		? ($unexpected_power / $allowed_power)
		: 0.0;

	return ($allowed_power,
	        $unexpected_power,
	        $ratio,
	        [map { sprintf("%.1f", $_->[1]) } @top]);
}

sub frequency_is_allowed {
	my ($hz,
	    $bin_width,
	    $expected_values,
	    $harmonics,
	    $minimum_tolerance,
	    $max_hz) = @_;

	for my $expected (@$expected_values) {
		for my $harmonic (1 .. $harmonics) {
			my $target = $expected * $harmonic;
			my $tolerance;

			last if $target > $max_hz;

			$tolerance = $minimum_tolerance;
			$tolerance = $target * 0.015 if $target * 0.015 > $tolerance;
			$tolerance = $bin_width * 3.0 if $bin_width * 3.0 > $tolerance;

			return 1 if abs($hz - $target) <= $tolerance;
		}
	}

	return 0;
}

sub remember_top_unexpected {
	my ($top, $hz, $magnitude) = @_;

	push @$top, [$magnitude, $hz];
	@$top = sort { $b->[0] <=> $a->[0] } @$top;
	pop @$top while @$top > 8;
}

# end of analyze-sfx-wav.pl
