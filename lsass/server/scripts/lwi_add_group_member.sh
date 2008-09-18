#!/bin/bash

PREFIX_DIR=/usr/centeris
ache/centeris:/var/lib/likewise:g
LSASS_DB_FILENAME=lwi_lsass.db

if [ $# -lt 2 ]; then
   echo "Usage: lwi_add_group_member.sh <group id> <group members:comma separated>"
   exit 1
fi

if [ ! -f $PREFIX_DIR/bin/sqlite3 ]; then
   echo "Error: Cannot find $PREFIX_DIR/bin/sqlite3"
   exit 1
fi

LSASS_DB=$LSASS_DB_DIR/$LSASS_DB_FILENAME

if [ ! -f $LSASS_DB ]; then
   echo "Failed to find lsass database at $LSASS_DB"
   exit 1
fi

grp_id=$1
grpExists=`$PREFIX_DIR/bin/sqlite3 $LSASS_DB "select count(*) from lwigroups where Gid=$grp_id"`
if [ $grpExists -eq 0 ]; then
   echo "Group [Gid:$grp_id] does not exist in database"
   exit 1
fi

IFS=","
for grp_member in $2
do
      echo "Group Member=$grp_member"
      user_id=`$PREFIX_DIR/bin/sqlite3 $LSASS_DB "select Uid from lwiusers where Name=\"$grp_member\""`

      let grpMembershipExists=0
      # The user does exist
      if [ ! -z "$user_id" ]; then
         grpMembershipExists=`$PREFIX_DIR/bin/sqlite3 $LSASS_DB "select count(*) from lwigroupmembers where Uid=$user_id and Gid=$grp_id"`
         if [ $grpMembershipExists -eq 0 ]; then
            echo "Adding group member: User [uid:$user_id] [gid:$grp_id]"
            $PREFIX_DIR/bin/sqlite3 $LSASS_DB "insert into lwigroupmembers (Uid, Gid)values ( $user_id, $grp_id)";
         fi
       else
         echo "Failed to find user [Id:$user] in database"
       fi
done 
