#!/bin/sh

PREFIX_DIR=/usr/centeris
ache/centeris:/var/lib/likewise:g
LSASS_DB_FILENAME=lwi_lsass.db

if [ ! -f $PREFIX_DIR/bin/sqlite3 ]; then
   echo "Error: Cannot find $PREFIX_DIR/bin/sqlite3"
   exit 1
fi

if [ ! -d $LSASS_DB_DIR ]; then
   mkdir -p $LSASS_DB_DIR
   if [ $? -ne 0 ]; then
      echo "Failed to create folder at $LSASS_DB_DIR"
      exit 1
   fi
fi

LSASS_DB=$LSASS_DB_DIR/$LSASS_DB_FILENAME

if [ -f $LSASS_DB ]; then
   echo "Removing current local security authority database at $LSASS_DB"
   /bin/rm -f $LSASS_DB
   if [ $? -ne 0 ]; then
      echo "Failed to remove current local security authority database at $LSASS_DB"
      exit 1
   fi
fi

$PREFIX_DIR/bin/sqlite3 $LSASS_DB << EOF

create table lwigroupmembers (Gid integer,
                              Uid integer
                              );

create table lwiusers (UserRecordId integer PRIMARY KEY,
ache/centeris:/var/lib/likewise:g
ache/centeris:/var/lib/likewise:g
                        Uid           integer,
                        Gid           integer,
ache/centeris:/var/lib/likewise:g
ache/centeris:/var/lib/likewise:g
ache/centeris:/var/lib/likewise:g
                        CreatedTime   date 
                       );
create trigger lwiusers_createdtime after insert on lwiusers
    begin
       update lwiusers set CreatedTime = DATETIME('NOW') where rowid = new.rowid;
       insert into lwigroupmembers (Uid, Gid) values (new.Uid, new.Gid);
    end;
create trigger lwiusers_delete_record after delete on lwiusers
    begin
       delete from lwigroupmembers where Uid = old.Uid;
    end;

create table lwigroups (GroupRecordId integer PRIMARY KEY,
ache/centeris:/var/lib/likewise:g
ache/centeris:/var/lib/likewise:g
                       Gid           integer,
                       CreatedTime   date
                       );
create trigger lwigroups_createdtime after insert on lwigroups
    begin
       update lwigroups set CreatedTime = DATETIME('NOW') where rowid = new.rowid;
    end;
create trigger lwigroups_delete_record after delete on lwigroups
    begin
       delete from lwigroupmembers where Gid = old.Gid;
    end;

EOF

echo "Successfully created local security authority tables at $LSASS_DB"

