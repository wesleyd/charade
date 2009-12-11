#!/usr/bin/env perl
#
# Generate a .c file with a print_copyright() function in it to print
# out the contents of all the files on the command line...
#
# Copyright (c) 2009, Wesley Darlington. All Rights Reserved.

use warnings;
use strict;

die "usage: $0 LICENCE ...\n" unless @ARGV;

sub c_stringify {
    my $line = shift;
    chomp($line);

    $line =~ s/\\/\\\\/;
    $line =~ s/"/\\"/;
    $line =~ s/\n/"\n"/g;

    return $line;
}

print qq{
/* AUTO GENERATED FILE - DO NOT MODIFY - SEE $0 FOR DETAILS */

#include <stdlib.h>

#include "copyright.h"
#include "eprintf.h"

void
print_copyright(void)
\{
    EPRINTF_RAW(0, "\\n");
};

while (<>) {
    my $safe = c_stringify($_);
    print "    EPRINTF_RAW(0, \"" . c_stringify($safe) . "\\n\");\n";
}

print qq{
    EPRINTF_RAW(0, "\\n");

    exit(0);
\}
};
