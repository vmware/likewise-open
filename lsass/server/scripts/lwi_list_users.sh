#!/bin/sh

PREFIX_DIR=/usr/centeris
ache/centeris:/var/lib/likewise:g
LSASS_DB_FILENAME=lwi_lsass.db

if [ ! -f $PREFIX_DIR/bin/sqlite3 ]; then
   echo "Error: Cannot find $PREFIX_DIR/bin/sqlite3"
   exit 1
fi

LSASS_DB=$LSASS_DB_DIR/$LSASS_DB_FILENAME

if [ ! -f $LSASS_DB ]; then
   echo "Failed to find lsass database at $LSASS_DB"
   exit 1
fi

$PREFIX_DIR/bin/sqlite3 $LSASS_DB << EOF

select * from lwiusers;

EOF

