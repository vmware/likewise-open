#!/bin/sh
##
## Copyright (C) Centeris Corporation 2004-2007
## Copyright (C) Likewise Software 2007.  
## All rights reserved.
## 
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 3 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program.  If not, see <http:##www.gnu.org/licenses/>.
##

#!/bin/sh

## insert all possible names (only works with autoconf 2.x)
TESTAUTOCONF="autoconf autoconf-2.53 autoconf2.50 autoconf259 autoconf253"
AUTOCONFFOUND="0"

## 
## Look for autoconf
##

for i in $TESTAUTOCONF; do
	if which $i > /dev/null 2>&1; then
		if test `$i --version | head -n 1 | cut -d.  -f 2 | sed "s/[^0-9]//g"` -ge 53; then
			AUTOCONF=$i
			AUTOCONFFOUND="1"
			break
		fi
	fi
done


## 
## do we have it?
##
if test "$AUTOCONFFOUND" = "0" -o "$AUTOHEADERFOUND" = "0"; then
	echo "$0: need autoconf 2.53 or later" >&2
	exit 1
fi

rm -rf autom4te*.cache
rm -f configure include/config.h*

echo "$0: running $AUTOCONF"
$AUTOCONF || exit 1

rm -rf autom4te*.cache

echo "Now run ./configure and then make."
exit 0
