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
#GroupMembers=$4
let iGrpRec=1
IFS=":"
while read grp_name grp_passwd grp_id grp_members
do
        if [ ! -z "$grp_members" ]; then
           ./lwi_add_group_member.sh $grp_id $grp_members
        fi
done < /etc/group;

