#!/bin/sh

# Run this script to build samba from SVN.

autoheader
aclocal
libtoolize
automake --add-missing
autoconf
