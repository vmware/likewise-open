#!/bin/sh

# Run this script to build samba from SVN.

autoheader
aclocal
libtoolize -c
automake --add-missing --copy
autoconf
