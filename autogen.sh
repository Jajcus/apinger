#!/bin/sh
aclocal
autoheader
touch ChangeLog
automake -a -c
[ -s ChangeLog ] || rm -f ChangeLog
autoconf
./configure CFLAGS="-ggdb -Wall" $*

