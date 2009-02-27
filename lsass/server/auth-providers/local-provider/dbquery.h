/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see 
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        dbquery.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        Local Authentication Provider
 * 
 *        User/Group Database Query Templates
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __DBQUERY_H__
#define __DBQUERY_H__

#define DB_QUERY_LOOKUP_UID_BY_NAME            \
    "SELECT Uid from lwiusers where Name = %Q"

#define DB_QUERY_CHECK_GROUP_MEMBERSHIP                      \
    "SELECT count(*) from lwigroupmembers                    \
     WHERE  Uid = %d AND Gid = %d"

#define DB_QUERY_COUNT_EXISTING_USERS                        \
    "SELECT count(*) from lwiusers"                    
     

#define DB_QUERY_COUNT_EXISTING_GROUPS                       \
    "SELECT count(*) from lwigroups"                    
     

#define DB_QUERY_FIND_GROUP_MEMBERS_BY_GID                     \
    "select distinct Name                                      \
       from   lwiusers, lwigroupmembers                        \
      where  lwiusers.Uid = lwigroupmembers.Uid                \
        and  lwigroupmembers.Gid = %d"

#define DB_QUERY_FIND_USER_GROUPS_0_BY_UID                     \
    "select lwigroups.Name, "                                  \
    "       lwigroups.Gid "                                    \
    "  from lwigroups, lwigroupmembers "                       \
    " where lwigroupmembers.Uid = %d "                         \
    "   and lwigroups.Gid = lwigroupmembers.Gid"

#define DB_QUERY_FIND_USER_GROUPS_1_BY_UID                     \
    "select lwigroups.Name, "                                  \
    "       lwigroups.Passwd, "                                \
    "       lwigroups.Gid "                                    \
    "  from lwigroups, lwigroupmembers "                       \
    " where lwigroupmembers.Uid = %d "                         \
    "   and lwigroups.Gid = lwigroupmembers.Gid"

#define DB_QUERY_FIND_ROWID_FOR_UID                            \
    "select RowId                                              \
       from lwiusers                                           \
      where Uid = %d"

#define DB_QUERY_FIND_MAX_UID                                  \
    "select max(Uid) from lwiusers"

#define DB_QUERY_FIND_MAX_GID                                  \
    "select max(Gid) from lwigroups"

#define DB_QUERY_FIND_LM_OWF_BY_UID                            \
    "select LMOwf                                              \
       from lwiusers                                           \
      where Uid = %d"

#define DB_QUERY_FIND_NT_OWF_BY_UID                            \
    "select NTOwf                                              \
       from lwiusers                                           \
      where Uid = %d"

#define DB_QUERY_INSERT_GROUP "INSERT INTO lwigroups         \
                                     (GroupRecordId,         \
                                      Name,                  \
                                      Passwd,                \
                                      Gid                    \
                                     )                       \
                               VALUES( NULL,                 \
                                       %Q,                   \
                                       %Q,                   \
                                       %d                    \
                                     )"

#define DB_QUERY_FIND_GROUP_0_BY_GID                             \
    "select Name,                                                \
            Gid                                                  \
    from    lwigroups                                            \
    where   Gid = %d"

#define DB_QUERY_FIND_GROUP_1_BY_GID                             \
    "select Name,                                                \
            Passwd,                                              \
            Gid                                                  \
    from    lwigroups                                            \
    where   Gid = %d"

#define DB_QUERY_FIND_GROUP_0_BY_NAME                          \
    "select Name,                                              \
            Gid                                                \
    from    lwigroups                                          \
    where   Name = %Q"

#define DB_QUERY_FIND_GROUP_1_BY_NAME                          \
    "select Name,                                              \
            Passwd,                                            \
            Gid                                                \
    from    lwigroups                                          \
    where   Name = %Q"

#define DB_QUERY_FIND_GROUPS_0_LIMIT                           \
    "select Name,                                              \
            Gid                                                \
    from    lwigroups                                          \
    order by Gid                                               \
    LIMIT %d OFFSET %d"

#define DB_QUERY_FIND_GROUPS_1_LIMIT                           \
    "select Name,                                              \
            Passwd,                                            \
            Gid                                                \
    from    lwigroups                                          \
    order by Gid                                               \
    LIMIT %d OFFSET %d"

#define DB_QUERY_FIND_USER_0_BY_UID                            \
    "select Name,                                              \
            Passwd,                                            \
            Uid,                                               \
            Gid,                                               \
            Gecos,                                             \
            HomeDir,                                           \
            Shell                                              \
    from    lwiusers                                           \
    where   Uid = %d"

#define DB_QUERY_FIND_USER_1_BY_UID                            \
    "select Name,                                              \
            Passwd,                                            \
            Uid,                                               \
            Gid,                                               \
            Gecos,                                             \
            HomeDir,                                           \
            Shell                                              \
    from    lwiusers                                           \
    where   Uid = %d"

#define DB_QUERY_FIND_USER_2_BY_UID                            \
    "select Name,                                              \
            Passwd,                                            \
            Uid,                                               \
            Gid,                                               \
            Gecos,                                             \
            HomeDir,                                           \
            Shell,                                             \
            UserInfoFlags,                                     \
            AccountExpiry,                                     \
            PasswdChangeTime                                   \
    from    lwiusers                                           \
    where   Uid = %d"

#define DB_QUERY_FIND_USER_0_BY_NAME                           \
    "select Name,                                              \
            Passwd,                                            \
            Uid,                                               \
            Gid,                                               \
            Gecos,                                             \
            HomeDir,                                           \
            Shell                                              \
    from    lwiusers                                           \
    where   Name = %Q"

#define DB_QUERY_FIND_USER_1_BY_NAME                           \
    "select Name,                                              \
            Passwd,                                            \
            Uid,                                               \
            Gid,                                               \
            Gecos,                                             \
            HomeDir,                                           \
            Shell                                              \
    from    lwiusers                                           \
    where   Name = %Q"

#define DB_QUERY_FIND_USER_2_BY_NAME                           \
    "select Name,                                              \
            Passwd,                                            \
            Uid,                                               \
            Gid,                                               \
            Gecos,                                             \
            HomeDir,                                           \
            Shell,                                             \
            UserInfoFlags,                                     \
            AccountExpiry,                                     \
            PasswdChangeTime                                   \
    from    lwiusers                                           \
    where   Name = %Q"

#define DB_QUERY_FIND_USERS_0_LIMIT                            \
    "select Name,                                              \
            Passwd,                                            \
            Uid,                                               \
            Gid,                                               \
            Gecos,                                             \
            HomeDir,                                           \
            Shell                                              \
    from    lwiusers                                           \
    order by Uid                                               \
    LIMIT %d OFFSET %d"

#define DB_QUERY_FIND_USERS_1_LIMIT                            \
    "select Name,                                              \
            Passwd,                                            \
            Uid,                                               \
            Gid,                                               \
            Gecos,                                             \
            HomeDir,                                           \
            Shell                                              \
    from    lwiusers                                           \
    order by Uid                                               \
    LIMIT %d OFFSET %d"

#define DB_QUERY_FIND_USERS_2_LIMIT                            \
    "select Name,                                              \
            Passwd,                                            \
            Uid,                                               \
            Gid,                                               \
            Gecos,                                             \
            HomeDir,                                           \
            Shell,                                             \
            UserInfoFlags,                                     \
            AccountExpiry,                                     \
            PasswdChangeTime                                   \
    from    lwiusers                                           \
    order by Uid                                               \
    LIMIT %d OFFSET %d"

#define DB_QUERY_FIND_LM_OWF_FOR_UID                           \
    "select LMOwf_1,                                           \
            LMOwf_2,                                           \
            LMOwf_3,                                           \
            LMOwf_4                                            \
       from lwiusers                                           \
      where Uid = %d"

#define DB_QUERY_UPDATE_LM_OWF_FOR_UID                         \
    "update lwiusers                                           \
        set LMOwf_1 = %d,                                      \
            LMOwf_2 = %d,                                      \
            LMOwf_3 = %d,                                      \
            LMOwf_4 = %d,                                      \
            PasswdChangeTime = %d                              \
      where Uid = %d"

#define DB_QUERY_FIND_NT_OWF_FOR_UID                           \
    "select NTOwf_1,                                           \
            NTOwf_2,                                           \
            NTOwf_3,                                           \
            NTOwf_4                                            \
       from lwiusers                                           \
      where Uid = %d"

#define DB_QUERY_UPDATE_NT_OWF_FOR_UID                         \
    "update lwiusers                                           \
        set NTOwf_1 = %d,                                      \
            NTOwf_2 = %d,                                      \
            NTOwf_3 = %d,                                      \
            NTOwf_4 = %d,                                      \
            PasswdChangeTime = %d                              \
      where Uid = %d"

#define DB_QUERY_GET_USER_INFO_FLAGS                           \
    "select UserInfoFlags                                      \
       from lwiusers                                           \
      where Uid = %d"

#define DB_QUERY_UPDATE_USER_INFO_FLAGS                        \
    "update lwiusers                                           \
        set UserInfoFlags = %d                                 \
      where Uid = %d"

#define DB_QUERY_DISABLE_USER                                  \
    "update lwiusers                                           \
        set Enabled = 0                                        \
      where Uid = %d"

#define DB_QUERY_UPDATE_ACCOUNT_EXPIRY_DATE                    \
    "update lwiusers                                           \
        set AccountExpiry = %d                                 \
      where Uid = %d"

#define DB_QUERY_UPDATE_ACCOUNT_EXPIRY_DATE_NEVER_EXPIRE       \
    "update lwiusers                                           \
        set AccountExpiry = 0                                  \
      where Uid = %d"

#define DB_QUERY_UPDATE_LAST_PASSWORD_CHANGE_TIME              \
    "update lwiusers                                           \
        set PasswdChangeTime = %d                              \
      where Uid = %d"

#define DB_QUERY_CREATE_GROUPMEMBERSHIP_TABLE                  \
    "create table lwigroupmembers (Gid integer,                \
                                   Uid integer                 \
                                  )"

#define DB_QUERY_CREATE_GROUPS_TABLE                           \
    "create table lwigroups (GroupRecordId integer PRIMARY KEY,\
                             Name          varchar(256),       \
                             Passwd        varchar(256),       \
                             Gid           integer,            \
                             CreatedTime   date                \
                             SecurityDescriptor BLOB           \
                            )"

#define DB_QUERY_CREATE_GROUPS_INSERT_TRIGGER                  \
    "create trigger lwigroups_createdtime                      \
     after insert on lwigroups                                 \
     begin                                                     \
          update lwigroups set CreatedTime = DATETIME('NOW')   \
          where rowid = new.rowid;                             \
     end"

#define DB_QUERY_CREATE_GROUPS_DELETE_TRIGGER                  \
    "create trigger lwigroups_delete_record                    \
     after delete on lwigroups                                 \
     begin                                                     \
          delete from lwigroupmembers where Gid = old.Gid;     \
     end"

#define DB_QUERY_CREATE_USERS_TABLE                               \
    "create table lwiusers (UserRecordId integer PRIMARY KEY,     \
                            Name             varchar(256),        \
                            Passwd           varchar(256),        \
                            Uid              integer,             \
                            Gid              integer,             \
                            UserInfoFlags    integer,             \
                            Gecos            varchar(256),        \
                            HomeDir          varchar(1024),       \
                            Shell            varchar(128),        \
                            PasswdChangeTime integer,             \
                            FullName         varchar(256),        \
                            AccountExpiry    integer,             \
                            LMOwf_1          integer,             \
                            LMOwf_2          integer,             \
                            LMOwf_3          integer,             \
                            LMOwf_4          integer,             \
                            NTOwf_1          integer,             \
                            NTOwf_2          integer,             \
                            NTOwf_3          integer,             \
                            NTOwf_4          integer,             \
                            CreatedTime      date                 \
                            SecurityDescriptor  BLOB              \
                            )"

#define DB_QUERY_CREATE_USERS_INSERT_TRIGGER                   \
    "create trigger lwiusers_createdtime                       \
     after insert on lwiusers                                  \
     begin                                                     \
          update lwiusers                                      \
          set CreatedTime = DATETIME('NOW')                    \
          where rowid = new.rowid;                             \
                                                               \
          insert into lwigroupmembers (Uid, Gid)               \
          values (new.Uid, new.Gid);                           \
     end"
    
#define DB_QUERY_CREATE_USERS_DELETE_TRIGGER                   \
    "create trigger lwiusers_delete_record                     \
     after delete on lwiusers                                  \
     begin                                                     \
          delete from lwigroupmembers where Uid = old.Uid;     \
     end"

#define DB_QUERY_INSERT_USER  "INSERT INTO lwiusers          \
                                     (UserRecordId,          \
                                      Name,                  \
                                      Passwd,                \
                                      Uid,                   \
                                      Gid,                   \
                                      UserInfoFlags,         \
                                      Gecos,                 \
                                      HomeDir,               \
                                      Shell,                 \
                                      PasswdChangeTime,      \
                                      FullName,              \
                                      AccountExpiry,         \
                                      LMOwf_1,               \
                                      LMOwf_2,               \
                                      LMOwf_3,               \
                                      LMOwf_4,               \
                                      NTOwf_1,               \
                                      NTOwf_2,               \
                                      NTOwf_3,               \
                                      NTOwf_4                \
                                     )                       \
                               VALUES( NULL,                 \
                                       %Q,                   \
                                       %Q,                   \
                                       %d,                   \
                                       %d,                   \
                                       0,                    \
                                       %Q,                   \
                                       %Q,                   \
                                       %Q,                   \
                                       0,                    \
                                       %Q,                   \
                                       0,                    \
                                       0,                    \
                                       0,                    \
                                       0,                    \
                                       0,                    \
                                       0,                    \
                                       0,                    \
                                       0,                    \
                                       0                     \
                                     )"

#define DB_QUERY_DELETE_USER "delete from lwiusers where Uid = %d"

#define DB_QUERY_DELETE_GROUP "delete from lwigroups where Gid = %d"

#define DB_QUERY_NUMBER_USERS_WITH_THIS_PRIMARY_GROUP        \
        "select count(*)                                     \
           from lwiusers                                     \
          where Gid = %d"

#define DB_QUERY_NUMBER_GROUP_MEMBERS "select count(*)       \
                                      from lwigroupmembers   \
                                      where Gid = %d"

#define DB_QUERY_ADD_GROUP_MEMBERSHIP                        \
        "insert                                              \
           into lwigroupmembers                              \
                (Uid,                                        \
                 Gid                                         \
                )                                            \
         values (%d,                                         \
                 %d                                          \
                )"

#define DB_QUERY_REMOVE_USER_FROM_GROUP                      \
        "delete                                              \
           from lwigroupmembers                              \
          where Uid = %d                                     \
            and Gid = %d"
                

#endif /* __DBQUERY_H__ */

