#!/bin/sh
autoheader
aclocal
libtoolize --automake -c
automake --add-missing --copy
autoconf
