#!/bin/sh
aclocal
autoheader
automake -a -c
autoconf
./configure CFLAGS="-ggdb -Wall" $*

