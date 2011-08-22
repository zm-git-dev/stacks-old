#!/usr/bin/perl
#
# Copyright 2011, Julian Catchen <jcatchen@uoregon.edu>
#
# This file is part of Stacks.
#
# Stacks is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Stacks is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Stacks.  If not, see <http://www.gnu.org/licenses/>.
#

#
# Sort paired-end sequences according to the stacks the non-paired-end 
# was found in. 
#
# By Julian Catchen <jcatchen@uoregon.edu>
#

use strict;
use constant stacks_version => "_VERSION_";

use constant true  => 1;
use constant false => 0;

my $debug          = 0;
my $read_lim       = 8;
my $white_list     = "";
my $cat_white_list = "";
my $in_path        = "";
my $out_path       = "";
my $samp_path      = "";

parse_command_line();

my (@files, %matches, %stacks, %reads, %marker_wl);

build_file_list(\@files);

my ($file, $num_files, $i);

if (length($cat_white_list) > 0) {
    load_white_list($cat_white_list, \%marker_wl);
    print STDERR "Loaded ", scalar(keys %marker_wl), " catalog IDs from '$cat_white_list'\n";
}

$num_files = scalar(@files);
$i         = 1;
foreach $file (@files) {
    printf(STDERR "Processing tag file % 2s of % 2s [%s]\n", $i, $num_files, $file);
    #
    # Load the sstacks file, listing matches between the stacks and the catalog
    #
    load_matches($in_path, $file, \%matches, \%marker_wl);

    #
    # Load the ustacks tag file for each sample, containing the read-to-stack mappings
    #
    $stacks{$file} = {};

    load_stacks($in_path, $file, $stacks{$file});

    $i++;
}

$i = 1;
foreach $file (@files) {
    printf(STDERR "Processing sample file % 2s of % 2s [%s]\n", $i, $num_files, $file);

    $reads{$file} = {};
    #
    # Map the read-pairs to the stack/catalog match they correspond to.
    #
    process_read_pairs($samp_path, $file, \%stacks, $reads{$file});

    $i++;
}

print STDERR "Printing results...\n";
print_results($out_path, \%matches, \%stacks, \%reads);


sub load_matches {
    my ($in_path, $in_file, $matches, $marker_wl) = @_;

    my ($file, $in_fh, $line, @parts, $key);

    $file = $in_path . $in_file . ".matches.tsv";
    open($in_fh, "<$file") or die("Unable to open '$file', $!\n");

    while ($line = <$in_fh>) {
        chomp $line;
        @parts = split(/\t/, $line);

        if (length($cat_white_list) > 0) {
	    next if (!defined($marker_wl->{$parts[2]}));
	}

        if (!defined($matches->{$parts[2]})) {
            $matches->{$parts[2]} = {};
        }

        #
        # Index by catalog ID -> sample/stack ID
        #
        $key = $in_file . "|" . $parts[4];
        $matches->{$parts[2]}->{$key}++;
    }

    close($in_fh);
}

sub load_stacks {
    my ($in_path, $in_file, $stacks) = @_;

    my ($file, $in_fh, $line, @parts);

    $file = $in_path . $in_file . ".tags.tsv";
    open($in_fh, "<$file") or die("Unable to open '$file', $!\n");

    while ($line = <$in_fh>) {
        chomp $line;
        @parts = split(/\t/, $line);

        next if ($parts[5] eq "consensus");

        #
        # Index by sequence ID -> stack ID
        #
        $stacks->{substr($parts[7], 0, -2)} = $parts[2];
    }

    close($in_fh);
}

sub process_read_pairs {
    my ($in_path, $in_file, $stacks, $reads) = @_;

    my ($file, $in_fh, $line, $seq, $key, $read_id);

    $file = $in_path . $in_file . ".fq_2";
    open($in_fh, "<$file") or die("Unable to open paired-end input file '$file'\n");

    while ($line = <$in_fh>) {
	next if (substr($line, 0, 1) ne "@");
	chomp $line;

        $read_id = substr($line, 1, -2);
	$seq     = <$in_fh>;
	chomp $seq;

	#
	# Read the repeated ID and the quality scores.
	#
	<$in_fh>;
	<$in_fh>;

        $key = $stacks->{$in_file}->{$read_id};
        next if (!defined($key));

        if (!defined($reads->{$key})) {
            $reads->{$key} = [];
        }

        push(@{$reads->{$key}}, {'id' => $read_id, 'seq' => $seq});
    }
}

sub print_results {
    my ($out_path, $matches, $stacks, $reads) = @_;

    my ($path, $cat_id, $sample, $stack_id, $read, $out_fh, $i, @keys, $count, $key, $mult_hits);

    # 
    # If a catalog ID matches stacks from multiple samples, print them out together.
    #
    foreach $cat_id (keys %{$matches}) {
        #
        # Check that this catalog ID only has a single match from each sample.
        #
        next if (check_mult_catalog_matches($matches->{$cat_id}) == true);

        #
        # Check that we have reads for this catalog ID to avoid opening mostly empty files.
        #
        next if (count_reads($matches->{$cat_id}, $reads) < $read_lim);

        $path = $out_path . $cat_id . ".fa";
        open($out_fh, ">$path") or die("Unable to open $path; '$!'\n");

        foreach $key (keys %{$matches->{$cat_id}}) {
            ($sample, $stack_id) = split(/\|/, $key);

            foreach $read (@{$reads->{$sample}->{$stack_id}}) {
                print $out_fh
                    ">", $cat_id, "|", $stack_id, "|", $read->{'id'}, "\n",
                    $read->{'seq'}, "\n";
            }
        }

        close($out_fh);
    }
}

sub check_mult_catalog_matches {
    my ($catalog) = @_;

    my (%samples, @keys, $key, $sample, $stack_id);

    foreach $key (keys %{$catalog}) {
        ($sample, $stack_id) = split(/\|/, $key);
        $samples{$sample}++;
    }

    my $mult_hits = 0;
       
    foreach $key (keys %samples) {
        $mult_hits++ if ($samples{$key} > 1);
    }
    
    if ($mult_hits > 0) {
        return true;
    } else {
        return false;
    }
}

sub count_reads {
    my ($catalog, $reads) = @_;

    my ($count, $key, $sample, $stack_id);

    $count = 0;

    foreach $key (keys %{$catalog}) {
        ($sample, $stack_id) = split(/\|/, $key);
        $count += scalar(@{$reads->{$sample}->{$stack_id}}) if (defined($reads->{$sample}->{$stack_id}));
    }

    return $count;
}

sub build_file_list {
    my ($files) = @_;

    my (@ls, $line, $file);

    # Load a white list of files to process if it is supplied.
    my %wl;
    if (length($white_list) > 0) {
	load_white_list($white_list, \%wl);
	print STDERR "Loaded ", scalar(keys %wl), " filenames from '$white_list'\n";
    }

    @ls = `ls -1 $in_path/*.tags.tsv`;

    foreach $line (@ls) {
	chomp $line;

	next if (length($line) == 0);	
	next if ($line =~ /batch_/);

	($file) = ($line =~ /$in_path\/(.+)\.tags\.tsv$/); 

        if (length($white_list) > 0) {
	    next if (!defined($wl{$file}));
	}

	push(@{$files}, $file);
    }
}

sub load_white_list {
    my ($list, $wl) = @_;

    open(WHITE, "<" . $list) 
	or die("Unable to open white list file '$white_list': $!\n");

    my $line   = "";

    while ($line = <WHITE>) {
	chomp $line;

	next if (length($line) == 0);

	$wl->{$line}++;
    }

    close(WHITE);
}

sub parse_command_line {
    while (@ARGV) {
	$_ = shift @ARGV;
	if    ($_ =~ /^-p$/) { $in_path    = shift @ARGV; }
	elsif ($_ =~ /^-o$/) { $out_path   = shift @ARGV; }
	elsif ($_ =~ /^-s$/) { $samp_path  = shift @ARGV; }
        elsif ($_ =~ /^-r$/) { $read_lim   = shift @ARGV; }
	elsif ($_ =~ /^-W$/) { $white_list = shift @ARGV; }
	elsif ($_ =~ /^-w$/) { $cat_white_list = shift @ARGV; }
	elsif ($_ =~ /^-d$/) { $debug++; }
	elsif ($_ =~ /^-v$/) { version(); exit(); }
	elsif ($_ =~ /^-h$/) { usage(); }
	else {
	    print STDERR "Unknown command line option: '$_'\n";
	    usage();
	}
    }

    $in_path   = $in_path   . "/" if (substr($in_path, -1)   ne "/");
    $out_path  = $out_path  . "/" if (substr($out_path, -1)  ne "/");
    $samp_path = $samp_path . "/" if (substr($samp_path, -1) ne "/");
}

sub version {
    print STDERR "sort_read_pairs.pl ", stacks_version, "\n";
}

sub usage {
    version();

    print STDERR <<EOQ; 
sort_read_pairs.pl -p path -s path -o path [-r reads] [-W white_list] [-w white_list] [-d] [-h]
    p: path to the stacks output files.
    s: path to paired-end sample files.
    o: path to output the collated FASTA files.
    r: number of reads required to output data for a particular stack.
    W: a white list of files to process in the input path.
    w: a white list of catalog IDs to include.
    h: display this help message.
    d: turn on debug output.

EOQ

exit(0);
}