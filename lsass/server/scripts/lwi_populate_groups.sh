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
#Gid=$3
let iGrpRec=1
awk 'BEGIN {FS=":"} {print $1, $2, $3}' /etc/group |
    while read grp_name grp_passwd grp_id; do
        echo "Adding group record [$iGrpRec]"
        echo "Group Name: [$grp_name]; Passwd: [$grp_passwd]; Id: [$grp_id]"
        $PREFIX_DIR/bin/sqlite3 $LSASS_DB "insert into lwigroups (Name,
                                  Passwd,
                                  Gid)
                       values ( \"$grp_name\",
                                \"$grp_passwd\",
                                $grp_id)";
        let iGrpRec=$iGrpRec+1
    done
