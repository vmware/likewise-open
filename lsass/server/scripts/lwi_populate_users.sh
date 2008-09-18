#!/bin/bash

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

#Name=$1
#Passwd=$2
#Uid=$3
#Gid=$4
#Gecos=$5
#HomeDir=$6
#Shell=$7
IFS=":"
while read name passwd uid gid gecos homedir shell
do

$PREFIX_DIR/bin/sqlite3 $LSASS_DB "insert into lwiusers (
                                  Name,
                                  Passwd,
                                  Uid,
                                  Gid,
                                  Gecos,
                                  HomeDir,
                                  Shell)
                       values ( \"$name\",
                                \"$passwd\",
                                $uid,
                                $gid,
                                \"$gecos\",
                                \"$homedir\",
                                \"$shell\")";

done < /etc/passwd
