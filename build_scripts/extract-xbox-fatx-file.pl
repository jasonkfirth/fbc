#!/usr/bin/env perl
#
# Project: FreeBASIC Xbox package tests
# ------------------------------------
#
# File: extract-xbox-fatx-file.pl
#
# Purpose:
#
#   Extract a small file from the Xbox E: FATX partition inside the xemu
#   qcow2 hard disk image used by the package tests.
#
# Responsibilities:
#
#   * translate qcow2 virtual disk reads to physical host-file reads
#   * read the fixed Xbox E: partition used by the test harness
#   * walk FATX directory entries and file cluster chains
#   * write the requested guest file to standard output
#
# This file intentionally does NOT contain:
#
#   * generic qcow2 image editing
#   * support for compressed qcow2 clusters
#   * Xbox dashboard or save-data knowledge
#

use strict;
use warnings;

my $image_path = shift @ARGV or die "usage: $0 image.qcow2 E:/path/file\n";
my $guest_path = shift @ARGV or die "usage: $0 image.qcow2 E:/path/file\n";

open my $image_fh, '<:raw', $image_path or die "$image_path: $!\n";

read($image_fh, my $qcow_header, 104) == 104 or die "short qcow2 header\n";

my $qcow_cluster_bits = unpack('N', substr($qcow_header, 20, 4));
my $qcow_cluster_size = 1 << $qcow_cluster_bits;
my $l1_size = unpack('N', substr($qcow_header, 36, 4));
my $l1_offset = raw_u64(substr($qcow_header, 40, 8));
my $l2_bits = $qcow_cluster_bits - 3;
my $l2_entries = 1 << $l2_bits;

my $QCOW_OFLAG_COPIED = 2 ** 63;
my $QCOW_OFLAG_COMPRESSED = 2 ** 62;

my $XBOX_E_OFFSET = 0xabe8 * 65536;
my $XBOX_E_LENGTH = 0x131f * 1048576;

sub raw_u64
{
	my ($bytes) = @_;
	my ($hi, $lo) = unpack('NN', $bytes);

	return ($hi * 4294967296) + $lo;
}

sub strip_qcow_flags
{
	my ($value) = @_;

	$value -= $QCOW_OFLAG_COPIED if $value >= $QCOW_OFLAG_COPIED;

	if ($value >= $QCOW_OFLAG_COMPRESSED) {
		die "compressed qcow2 clusters are not supported\n";
	}

	return int($value / $qcow_cluster_size) * $qcow_cluster_size;
}

seek($image_fh, $l1_offset, 0) or die "could not seek to qcow2 L1 table\n";
read($image_fh, my $l1_buffer, 8 * $l1_size) == 8 * $l1_size or die "short qcow2 L1 table\n";

my @l1_table = map {
	my $value = raw_u64($_);
	$value ? strip_qcow_flags($value) : 0;
} unpack('(a8)*', $l1_buffer);

my %l2_cache;

sub read_l2_table
{
	my ($offset) = @_;

	return $l2_cache{$offset} if exists $l2_cache{$offset};

	seek($image_fh, $offset, 0) or die "could not seek to qcow2 L2 table\n";
	read($image_fh, my $buffer, 8 * $l2_entries) == 8 * $l2_entries or die "short qcow2 L2 table\n";

	my @entries = map { raw_u64($_) } unpack('(a8)*', $buffer);
	$l2_cache{$offset} = \@entries;

	return \@entries;
}

sub qcow_physical_offset
{
	my ($virtual_offset) = @_;

	my $l1_index = int($virtual_offset / (2 ** ($qcow_cluster_bits + $l2_bits)));
	return undef if $l1_index > $#l1_table;
	return undef if !$l1_table[$l1_index];

	my $l2_table = read_l2_table($l1_table[$l1_index]);
	my $l2_index = int($virtual_offset / $qcow_cluster_size) & ($l2_entries - 1);
	my $entry = $l2_table->[$l2_index];

	return undef if !$entry;

	return strip_qcow_flags($entry) + ($virtual_offset & ($qcow_cluster_size - 1));
}

sub qcow_read
{
	my ($virtual_offset, $count) = @_;
	my $output = '';

	while ($count > 0) {
		my $chunk = $qcow_cluster_size - ($virtual_offset & ($qcow_cluster_size - 1));
		$chunk = $count if $chunk > $count;

		my $physical_offset = qcow_physical_offset($virtual_offset);

		if (!defined $physical_offset) {
			$output .= "\0" x $chunk;
		} else {
			seek($image_fh, $physical_offset, 0) or die "could not seek to qcow2 data cluster\n";
			read($image_fh, my $buffer, $chunk) == $chunk or die "short qcow2 data read\n";
			$output .= $buffer;
		}

		$virtual_offset += $chunk;
		$count -= $chunk;
	}

	return $output;
}

sub round_up
{
	my ($value, $alignment) = @_;

	return int(($value + $alignment - 1) / $alignment) * $alignment;
}

my $fatx_superblock = qcow_read($XBOX_E_OFFSET, 512);
my $sectors_per_cluster = unpack('V', substr($fatx_superblock, 8, 4));
die "invalid FATX cluster size\n" if $sectors_per_cluster == 0;

my $fatx_cluster_size = $sectors_per_cluster * 512;
my $fatx_cluster_count = int($XBOX_E_LENGTH / $fatx_cluster_size) + 1;
my $fat_entry_size = ($fatx_cluster_count < 65520) ? 2 : 4;
my $fat_size = round_up($fatx_cluster_count * $fat_entry_size, 4096);
my $fat_offset = $XBOX_E_OFFSET + 4096;
my $data_offset = $XBOX_E_OFFSET + 4096 + $fat_size;

sub read_fat_entry
{
	my ($cluster) = @_;
	my $entry_offset = $fat_offset + ($cluster * $fat_entry_size);
	my $entry = qcow_read($entry_offset, $fat_entry_size);

	return $fat_entry_size == 2 ? unpack('v', $entry) : unpack('V', $entry);
}

sub is_fat_end_marker
{
	my ($value) = @_;

	return $fat_entry_size == 2 ? ($value >= 0xfff8) : ($value >= 0xfffffff8);
}

sub read_cluster_chain
{
	my ($cluster, $size) = @_;
	my $output = '';
	my $seen = 0;

	while ($cluster >= 1 && $seen < $fatx_cluster_count) {
		$output .= qcow_read($data_offset + (($cluster - 1) * $fatx_cluster_size), $fatx_cluster_size);
		last if defined($size) && length($output) >= $size;

		my $next_cluster = read_fat_entry($cluster);
		last if is_fat_end_marker($next_cluster);
		last if $next_cluster == 0;
		last if $next_cluster == $cluster;

		$cluster = $next_cluster;
		$seen++;
	}

	return defined($size) ? substr($output, 0, $size) : $output;
}

sub find_directory_entry
{
	my ($directory_data, $name) = @_;

	for (my $offset = 0; $offset + 64 <= length($directory_data); $offset += 64) {
		my $name_length = ord(substr($directory_data, $offset, 1));
		last if $name_length == 0;
		next if $name_length == 0xe5 || $name_length == 0xff;

		my $entry_name = substr($directory_data, $offset + 2, $name_length);
		next if uc($entry_name) ne uc($name);

		return {
			attr => ord(substr($directory_data, $offset + 1, 1)),
			cluster => unpack('V', substr($directory_data, $offset + 44, 4)),
			size => unpack('V', substr($directory_data, $offset + 48, 4)),
		};
	}

	return undef;
}

$guest_path =~ s#\\#/#g;
$guest_path =~ s#^[A-Za-z]:/+##;
$guest_path =~ s#^/+##;

my @path_parts = grep { $_ ne '' } split('/', $guest_path);
die "empty guest path\n" if !@path_parts;

my $directory_data = qcow_read($data_offset, $fatx_cluster_size);
my $entry;

while (@path_parts) {
	my $part = shift @path_parts;
	$entry = find_directory_entry($directory_data, $part);
	die "$part not found\n" if !defined $entry;

	if (@path_parts) {
		die "$part is not a directory\n" if (($entry->{attr} & 0x10) == 0);
		$directory_data = read_cluster_chain($entry->{cluster}, undef);
	}
}

die "final path is a directory\n" if (($entry->{attr} & 0x10) != 0);

binmode STDOUT;
print read_cluster_chain($entry->{cluster}, $entry->{size});

# end of extract-xbox-fatx-file.pl
