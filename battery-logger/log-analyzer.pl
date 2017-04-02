#!/usr/bin/perl

#
# DESCRIPTION
#
#   A quick script which generates battery_percent_mV table for
#   BatteryLogger::calculatePercentage from given input logs.
#
# USAGE
#
#   perl log-analyzer.pl STEP LOGFILE...
#
#   STEP is the wanted resolution (in percents) for calculatePercentage.
#        Possible values are 1, 2, 5, 10, 20 and 25.
#
# EXAMPLE
#
#   perl log-analyzer.pl 5 logs/*
#
# INPUT LOGS
#
#   - each input log must be from full battery until Mono shuts down because of
#     empty battery
#   - each log is considered to start at 00:00:00 with full battery
#   - power consumption of Mono must remain constant during each log
#
# CALCULATION
#
#   1) for each log
#     1.1) elapsed time is divided into 100 equal parts, to get timepoints
#          at which there was 100%, 99%, ..., 0% power remaining
#     1.2) for each timepoint, mV at that point is calculated by
#          linear interpolation/extrapolation of nearest two datapoints
#   2) for each timepoint, average of values from each log is calculated
#      - average is forced to be non-increasing by using previous value
#        if average would increase
#
# COPYRIGHT
# 
#   Copyright (c) 2017 Markus Laire
# 
# LICENSE
# 
#   Permission is hereby granted, free of charge, to any person obtaining a copy
#   of this software and associated documentation files (the "Software"), to
#   deal in the Software without restriction, including without limitation the
#   rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
#   sell copies of the Software, and to permit persons to whom the Software is
#   furnished to do so, subject to the following conditions:
#
#   The above copyright notice and this permission notice shall be included in
#   all copies or substantial portions of the Software.
#
#   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
#   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
#   IN THE SOFTWARE.
#

use v5.20;
use warnings FATAL => 'all';

# ======================================================================
# CONSTANTS

my %ALLOWED_STEP_SIZES = map {$_ => 1} 1, 2, 5, 10, 20, 25;

# ======================================================================
# MAIN

if (@ARGV < 2) {
  say STDERR "USAGE: perl log-analyzer.pl STEP LOGFILE...";
  exit 1;
}

my $STEP_SIZE = shift;
if (not exists $ALLOWED_STEP_SIZES{$STEP_SIZE}) {
  say "Invalid STEP, allowed values are: ",
    join(',', sort {$a <=> $b} keys %ALLOWED_STEP_SIZES);
  exit 1;
}

my @percent_mV;    # [$percent] = [ $mV... ]
@percent_mV[$_] = [] for 0..100;

for my $logfile (@ARGV) {
  say STDERR "Reading $logfile ...";

  my @logdata; # [..] = [ $time, $mV ]

  # READ FILE

  my $prevTime = -1;  
  open my $fh, '<', $logfile or die "Can't open $logfile";
  while (<$fh>) {
    # ignore empty lines and lines not starting with number
    next if /^\s*$/ or /^[^0-9]/;

    # data lines must be like '01:02:03 1234 mV'
    # - anything after 'mV' is ignored
    die "Invalid line: '$_'" unless /^(\d+):(\d\d):(\d\d)\s+(\d+)\s+mV/;
    my $time = $1 * 3600 + $2 * 60 + $3;
    my $mV   = $4;

    # time must increase    
    die "Time values are not increasing @ '$_'" if $time <= $prevTime;
    $prevTime = $time;

    push @logdata, [ $time, $mV ];
  }  

  # CONVERT TIMES TO PERCENTS

  my $startTime   = 0;
  my $endTime     = $logdata[-1][0];
  my $elapsedTime = $endTime - $startTime;
  printf STDERR "- %4d measurements / %5d seconds elapsed\n",
    scalar @logdata, $elapsedTime;

  my $index    = 0;
  my $maxIndex = $#logdata;
  for (my $percent = 100; $percent >= 0; $percent--) {
    my $percentTime = $startTime + $elapsedTime * (100 - $percent) / 100;

    while ($index < $maxIndex && $logdata[$index + 1][0] < $percentTime) {
      $index++;
    }

    # linear interpolation/extrapolation with nearest two datapoints
    #   y = y0 + (x - x0) * (y1 - y0) / (x1 - x0)

    my $time_0 = $logdata[$index    ][0];
    my $time_1 = $logdata[$index + 1][0];
    my $mV_0   = $logdata[$index    ][1];
    my $mV_1   = $logdata[$index + 1][1];

    my $mV = $mV_0 +
      ($percentTime - $time_0) * ($mV_1 - $mV_0) / ($time_1 - $time_0);

    push @{$percent_mV[$percent]}, $mV;

    # DEBUG
    if (0) {
      printf STDERR "%3d%% @ %7.1f (%5d ~ %5d) -- %4d ~ %4d -> %4d\n",
        $percent, $percentTime,
        $logdata[$index    ][0], $logdata[$index + 1][0],
        $logdata[$index    ][1], $logdata[$index + 1][1],
        $mV
    }
  }
}

# CALCULATE AVERAGES

my @averages;
{
  my $prev_avg = 1e9;
  for (my $percent = 100; $percent >= 0; $percent--) {
    my @values = @{$percent_mV[$percent]};

    my $sum = 0;
    $sum += $_ for @values;
    my $avg = $sum / @values;

    # force non-increasing average (TODO: something better)
    $avg = $prev_avg if $avg > $prev_avg;
    $prev_avg = $avg;

    $averages[$percent] = $avg;
  }
}

# SHOW FINAL DATA
# - percent,average,inputs...

for (my $percent = 100; $percent >= 0; $percent--) {
  printf "%d,%d,%s\n", $percent, int($averages[$percent] + 0.5),
    join(',', map {sprintf "%.1f", $_} @{$percent_mV[$percent]});
}

# SHOW CODE

say "";
say "  /* BEGIN AUTO-GENERATED CODE */";
say "  static const uint16_t PERCENT_STEP_SIZE = $STEP_SIZE;";
say "  static const uint16_t battery_percent_mV[];";
say "  /* END AUTO-GENERATED CODE */";
say "";
say "/* BEGIN AUTO-GENERATED CODE */";
printf "// mV at %d%%, %d%%, ..., %d%%\n",
  100 - $STEP_SIZE, 100 - 2 * $STEP_SIZE, $STEP_SIZE;
say "const uint16_t BatteryLogger::battery_percent_mV[] = {";
print "  ";
my $chars = 2;
for (my $percent = 100 - $STEP_SIZE; $percent >= $STEP_SIZE; $percent -= $STEP_SIZE) {
  print int($averages[$percent] + 0.5), ",";
  $chars += 5;
  if ($chars > 75) {
    print "\n  ";
    $chars = 2;
  }
}
say "0\n};";
say "/* END AUTO-GENERATED CODE */";
say "";
