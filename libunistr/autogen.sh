#!/bin/sh
autoheader
aclocal
libtoolize --automake
automake --add-missing
autoconf
