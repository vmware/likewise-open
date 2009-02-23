/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

#ifndef _SRVSVCDEFS_H_
#define _SRVSVCDEFS_H_

#include <srvsvc/types.h>
#include <secdesc/sddef.h>


#ifndef CONNECTION_INFO_0_DEFINED
#define CONNECTION_INFO_0_DEFINED 1

typedef struct _CONNECTION_INFO_0 {
    uint32 coni0_id;
} CONNECTION_INFO_0, *PCONNECTION_INFO_0;

#endif


#ifndef FILE_INFO_2_DEFINED
#define FILE_INFO_2_DEFINED 1

typedef struct _FILE_INFO_2 {
    uint32 fi2_id;
} FILE_INFO_2, *PFILE_INFO_2;

#endif


#ifndef FILE_INFO_3_DEFINED
#define FILE_INFO_3_DEFINED 1

typedef struct _FILE_INFO_3 {
    uint32 fi3_idd;
    uint32 fi3_permissions;
    uint32 fi3_num_locks;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *fi3_path_name;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *fi3_username;
} FILE_INFO_3, *PFILE_INFO_3;

#endif


#ifndef SESSION_INFO_0_DEFINED
#define SESSION_INFO_0_DEFINED 1

typedef struct _SESSION_INFO_0 {
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sesi0_cname;
} SESSION_INFO_0, *PSESSION_INFO_0;

#endif


#ifndef SESSION_INFO_1_DEFINED
#define SESSION_INFO_1_DEFINED 1

typedef struct _SESSION_INFO_1 {
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sesi1_cname;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sesi1_username;
    uint32 sesi1_num_opens;
    uint32 sesi1_time;
    uint32 sesi1_idle_time;
    uint32 sesi1_user_flags;
} SESSION_INFO_1, *PSESSION_INFO_1;

#endif


#ifndef SESSION_INFO_2_DEFINED
#define SESSION_INFO_2_DEFINED 1


typedef struct _SESSION_INFO_2 {
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sesi2_cname;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sesi2_username;
    uint32 sesi2_num_opens;
    uint32 sesi2_time;
    uint32 sesi2_idle_time;
    uint32 sesi2_user_flags;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sesi2_cltype_name;
} SESSION_INFO_2, *PSESSION_INFO_2;

#endif


#ifndef SESSION_INFO_10_DEFINED
#define SESSION_INFO_10_DEFINED 1

typedef struct _SESSION_INFO_10 {
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sesi10_cname;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sesi10_username;
    uint32 sesi10_time;
    uint32 sesi10_idle_time;
} SESSION_INFO_10, *PSESSION_INFO_10;

#endif


#ifndef SESSION_INFO_502_DEFINED
#define SESSION_INFO_502_DEFINED 1

typedef struct _SESSION_INFO_502 {
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sesi502_cname;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sesi502_username;
    uint32 sesi502_num_opens;
    uint32 sesi502_time;
    uint32 sesi502_idle_time;
    uint32 sesi502_user_flags;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sesi502_cltype_name;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sesi502_transport;
} SESSION_INFO_502, *PSESSION_INFO_502;

#endif


#ifndef SERVER_INFO_100_DEFINED
#define SERVER_INFO_100_DEFINED 1


typedef struct _SERVER_INFO_100 {
    uint32 sv100_platform_id;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sv100_name;
} SERVER_INFO_100, *PSERVER_INFO_100;

#endif


#ifndef SERVER_INFO_101_DEFINED
#define SERVER_INFO_101_DEFINED 1

typedef struct _SERVER_INFO_101 {
    uint32 sv101_platform_id;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sv101_name;
    uint32 sv101_version_major;
    uint32 sv101_version_minor;
    uint32 sv101_type;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sv101_comment;
} SERVER_INFO_101, *PSERVER_INFO_101;

#endif


#ifndef SERVER_INFO_102_DEFINED
#define SERVER_INFO_102_DEFINED 1


typedef struct _SERVER_INFO_102 {
    uint32 sv102_platform_id;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sv102_name;
    uint32 sv102_version_major;
    uint32 sv102_version_minor;
    uint32 sv102_type;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sv102_comment;
    uint32 sv102_users;
    uint32 sv102_disc;
    uint32 sv102_hidden;
    uint32 sv102_announce;
    uint32 sv102_anndelta;
    uint32 sv102_licenses;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sv102_userpath;
} SERVER_INFO_102, *PSERVER_INFO_102;

#endif


#ifndef SERVER_INFO_402_DEFINED
#define SERVER_INFO_402_DEFINED 1


typedef struct _SERVER_INFO_402 {
    uint32 sv402_ulist_mtime;
    uint32 sv402_glist_mtime;
    uint32 sv402_alist_mtime;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sv402_alerts;
    uint32 sv402_security;
    uint32 sv402_numadmin;
    uint32 sv402_lanmask;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sv402_guestacct;
    uint32 sv402_chdevs;
    uint32 sv402_chdevq;
    uint32 sv402_chdevjobs;
    uint32 sv402_connections;
    uint32 sv402_shares;
    uint32 sv402_openfiles;
    uint32 sv402_sessopens;
    uint32 sv402_sesssvcs;
    uint32 sv402_sessreqs;
    uint32 sv402_opensearch;
    uint32 sv402_activelocks;
    uint32 sv402_numreqbuf;
    uint32 sv402_sizreqbuf;
    uint32 sv402_numbigbuf;
    uint32 sv402_numfiletasks;
    uint32 sv402_alertsched;
    uint32 sv402_erroralert;
    uint32 sv402_logonalert;
    uint32 sv402_accessalert;
    uint32 sv402_diskalert;
    uint32 sv402_netioalert;
    uint32 sv402_maxaudits;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sv402_srvheuristics;
} SERVER_INFO_402, *PSERVER_INFO_402;

#endif


#ifndef SERVER_INFO_403_DEFINED
#define SERVER_INFO_403_DEFINED 1

typedef struct _SERVER_INFO_403 {
    uint32 sv403_ulist_mtime;
    uint32 sv403_glist_mtime;
    uint32 sv403_alist_mtime;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sv403_alerts;
    uint32 sv403_security;
    uint32 sv403_numadmin;
    uint32 sv403_lanmask;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sv403_guestacct;
    uint32 sv403_chdevs;
    uint32 sv403_chdevq;
    uint32 sv403_chdevjobs;
    uint32 sv403_connections;
    uint32 sv403_shares;
    uint32 sv403_openfiles;
    uint32 sv403_sessopens;
    uint32 sv403_sesssvcs;
    uint32 sv403_sessreqs;
    uint32 sv403_opensearch;
    uint32 sv403_activelocks;
    uint32 sv403_numreqbuf;
    uint32 sv403_sizereqbuf;
    uint32 sv403_numbigbuf;
    uint32 sv403_numfiletasks;
    uint32 sv403_alertsched;
    uint32 sv403_erroralert;
    uint32 sv403_logonalert;
    uint32 sv403_accessalert;
    uint32 sv403_diskalert;
    uint32 sv403_netioalert;
    uint32 sv403_maxaudits;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sv403_srvheuristics;
    uint32 sv403_auditedevents;
    uint32 sv403_auditprofile;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sv403_autopath;
} SERVER_INFO_403, *PSERVER_INFO_403;

#endif


#ifndef SERVER_INFO_502_DEFINED
#define SERVER_INFO_502_DEFINED 1

typedef struct _SERVER_INFO_502 {
    uint32 sv502_sessopens;
    uint32 sv502_sessvcs;
    uint32 sv502_opensearch;
    uint32 sv502_sizreqbuf;
    uint32 sv502_initworkitems;
    uint32 sv502_maxworkitems;
    uint32 sv502_rawworkitems;
    uint32 sv502_irpstacksize;
    uint32 sv502_maxrawbuflen;
    uint32 sv502_sessusers;
    uint32 sv502_sessconns;
    uint32 sv502_maxpagedmemoryusage;
    uint32 sv502_maxnonpagedmemoryusage;
    uint32 sv502_enablesoftcompat;
    uint32 sv502_enableforcedlogoff;
    uint32 sv502_timesource;
    uint32 sv502_acceptdownlevelapis;
    uint32 sv502_lmannounce;
} SERVER_INFO_502, *PSERVER_INFO_502;

#endif


#ifndef SERVER_INFO_503_DEFINED
#define SERVER_INFO_503_DEFINED 1

typedef struct _SERVER_INFO_503 {
    uint32 sv503_sessopens;
    uint32 sv503_sessvcs;
    uint32 sv503_opensearch;
    uint32 sv503_sizreqbuf;
    uint32 sv503_initworkitems;
    uint32 sv503_maxworkitems;
    uint32 sv503_rawworkitems;
    uint32 sv503_irpstacksize;
    uint32 sv503_maxrawbuflen;
    uint32 sv503_sessusers;
    uint32 sv503_sessconns;
    uint32 sv503_maxpagedmemoryusage;
    uint32 sv503_maxnonpagedmemoryusage;
    uint32 sv503_enablesoftcompat;
    uint32 sv503_enableforcedlogoff;
    uint32 sv503_timesource;
    uint32 sv503_acceptdownlevelapis;
    uint32 sv503_lmannounce;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sv503_domain;
    uint32 sv503_maxcopyreadlen;
    uint32 sv503_maxcopywritelen;
    uint32 sv503_minkeepsearch;
    uint32 sv503_maxkeepsearch;
    uint32 sv503_minkeepcomplsearch;
    uint32 sv503_maxkeepcomplsearch;
    uint32 sv503_threadcountadd;
    uint32 sv503_numblockthreads;
    uint32 sv503_scavtimeout;
    uint32 sv503_minrcvqueue;
    uint32 sv503_minfreeworkitems;
    uint32 sv503_xactmemsize;
    uint32 sv503_threadpriority;
    uint32 sv503_maxmpxct;
    uint32 sv503_oplockbreakwait;
    uint32 sv503_oplockbreakresponsewait;
    uint32 sv503_enableoplocks;
    uint32 sv503_enableoplockforceclose;
    uint32 sv503_enablefcbopens;
    uint32 sv503_enableraw;
    uint32 sv503_enablesharednetdrives;
    uint32 sv503_minfreeconnections;
    uint32 sv503_maxfreeconnections;
} SERVER_INFO_503, *PSERVER_INFO_503;

#endif


#ifndef SERVER_INFO_599_DEFINED
#define SERVER_INFO_599_DEFINED 1

typedef struct _SERVER_INFO_599 {
    uint32 sv599_sessopens;
    uint32 sv599_sessvcs;
    uint32 sv599_opensearch;
    uint32 sv599_sizreqbuf;
    uint32 sv599_initworkitems;
    uint32 sv599_maxworkitems;
    uint32 sv599_rawworkitems;
    uint32 sv599_irpstacksize;
    uint32 sv599_maxrawbuflen;
    uint32 sv599_sessusers;
    uint32 sv599_sessconns;
    uint32 sv599_maxpagedmemoryusage;
    uint32 sv599_maxnonpagedmemoryusage;
    uint32 sv599_enablesoftcompat;
    uint32 sv599_enableforcedlogoff;
    uint32 sv599_timesource;
    uint32 sv599_acceptdownlevelapis;
    uint32 sv599_lmannounce;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sv599_domain;
    uint32 sv599_maxcopyreadlen;
    uint32 sv599_maxcopywritelen;
    uint32 sv599_minkeepsearch;
    uint32 sv599_maxkeepsearch;
    uint32 sv599_minkeepcomplsearch;
    uint32 sv599_maxkeepcomplsearch;
    uint32 sv599_threadcountadd;
    uint32 sv599_numblockthreads;
    uint32 sv599_scavtimeout;
    uint32 sv599_minrcvqueue;
    uint32 sv599_minfreeworkitems;
    uint32 sv599_xactmemsize;
    uint32 sv599_threadpriority;
    uint32 sv599_maxmpxct;
    uint32 sv599_oplockbreakwait;
    uint32 sv599_oplockbreakresponsewait;
    uint32 sv599_enableoplocks;
    uint32 sv599_enableoplockforceclose;
    uint32 sv599_enablefcbopens;
    uint32 sv599_enableraw;
    uint32 sv599_enablesharednetdrives;
    uint32 sv599_minfreeconnections;
    uint32 sv599_maxfreeconnections;
    uint32 sv599_initsesstable;
    uint32 sv599_initconntable;
    uint32 sv599_initfiletable;
    uint32 sv599_initsearchtable;
    uint32 sv599_alertschedule;
    uint32 sv599_errorthreshold;
    uint32 sv599_networkerrorthreshold;
    uint32 sv599_diskspacethreshold;
    uint32 sv599_reserved;
    uint32 sv599_maxlinkdelay;
    uint32 sv599_minlinkthroughput;
    uint32 sv599_linkinfovalidtime;
    uint32 sv599_scavqosinfoupdatetime;
    uint32 sv599_maxworkitemidletime;
} SERVER_INFO_599, *PSERVER_INFO_599;

#endif


#ifndef SERVER_INFO_1005_DEFINED
#define SERVER_INFO_1005_DEFINED 1

typedef struct _SERVER_INFO_1005 {
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sv1005_comment;
} SERVER_INFO_1005, *PSERVER_INFO_1005;

#endif


#ifndef SERVER_INFO_1010_DEFINED
#define SERVER_INFO_1010_DEFINED 1

typedef struct _SERVER_INFO_1010 {
    uint32 sv1010_disc;
} SERVER_INFO_1010, *PSERVER_INFO_1010;

#endif


#ifndef SERVER_INFO_1016_DEFINED
#define SERVER_INFO_1016_DEFINED 1

typedef struct _SERVER_INFO_1016 {
    uint32 sv1016_hidden;
} SERVER_INFO_1016, *PSERVER_INFO_1016;

#endif


#ifndef SERVER_INFO_1017_DEFINED
#define SERVER_INFO_1017_DEFINED 1

typedef struct _SERVER_INFO_1017 {
    uint32 sv1017_announce;
} SERVER_INFO_1017, *PSERVER_INFO_1017;

#endif


#ifndef SERVER_INFO_1018_DEFINED
#define SERVER_INFO_1018_DEFINED 1

typedef struct _SERVER_INFO_1018 {
    uint32 sv1018_anndelta;
} SERVER_INFO_1018, *PSERVER_INFO_1018;

#endif


#ifndef SERVER_INFO_1107_DEFINED
#define SERVER_INFO_1107_DEFINED 1

typedef struct _SERVER_INFO_1107 {
    uint32 sv1107_users;
} SERVER_INFO_1107, *PSERVER_INFO_1107;

#endif


#ifndef SERVER_INFO_1501_DEFINED
#define SERVER_INFO_1501_DEFINED 1

typedef struct _SERVER_INFO_1501 {
    uint32 sv1501_sessopens;
} SERVER_INFO_1501, *PSERVER_INFO_1501;

#endif


#ifndef SERVER_INFO_1502_DEFINED
#define SERVER_INFO_1502_DEFINED 1

typedef struct _SERVER_INFO_1502 {
    uint32 sv1502_sessvcs;
} SERVER_INFO_1502, *PSERVER_INFO_1502;

#endif


#ifndef SERVER_INFO_1503_DEFINED
#define SERVER_INFO_1503_DEFINED 1

typedef struct _SERVER_INFO_1503 {
    uint32 sv1503_opensearch;
} SERVER_INFO_1503, *PSERVER_INFO_1503;

#endif


#ifndef SERVER_INFO_1506_DEFINED
#define SERVER_INFO_1506_DEFINED 1

typedef struct _SERVER_INFO_1506 {
    uint32 sv1506_maxworkitems;
} SERVER_INFO_1506, *PSERVER_INFO_1506;

#endif


#ifndef SERVER_INFO_1509_DEFINED
#define SERVER_INFO_1509_DEFINED 1

typedef struct _SERVER_INFO_1509 {
    uint32 sv1509_maxrawbuflen;
} SERVER_INFO_1509, *PSERVER_INFO_1509;

#endif


#ifndef SERVER_INFO_1510_DEFINED
#define SERVER_INFO_1510_DEFINED 1

typedef struct _SERVER_INFO_1510 {
    uint32 sv1510_sessusers;
} SERVER_INFO_1510, *PSERVER_INFO_1510;

#endif


#ifndef SERVER_INFO_1511_DEFINED
#define SERVER_INFO_1511_DEFINED 1

typedef struct _SERVER_INFO_1511 {
    uint32 sv1511_sessconns;
} SERVER_INFO_1511, *PSERVER_INFO_1511;

#endif


#ifndef SERVER_INFO_1512_DEFINED
#define SERVER_INFO_1512_DEFINED 1

typedef struct _SERVER_INFO_1512 {
    uint32 sv1512_maxnonpagedmemoryusage;
} SERVER_INFO_1512, *PSERVER_INFO_1512;

#endif


#ifndef SERVER_INFO_1513_DEFINED
#define SERVER_INFO_1513_DEFINED 1

typedef struct _SERVER_INFO_1513 {
    uint32 sv1513_maxpagedmemoryusage;
} SERVER_INFO_1513, *PSERVER_INFO_1513;

#endif


#ifndef SERVER_INFO_1514_DEFINED
#define SERVER_INFO_1514_DEFINED 1

typedef struct _SERVER_INFO_1514 {
    uint32 sv1514_enablesoftcompat;
} SERVER_INFO_1514, *PSERVER_INFO_1514;

#endif


#ifndef SERVER_INFO_1515_DEFINED
#define SERVER_INFO_1515_DEFINED 1

typedef struct _SERVER_INFO_1515 {
    uint32 sv1515_enableforcedlogoff;
} SERVER_INFO_1515, *PSERVER_INFO_1515;

#endif


#ifndef SERVER_INFO_1516_DEFINED
#define SERVER_INFO_1516_DEFINED 1

typedef struct _SERVER_INFO_1516 {
    uint32 sv1516_timesource;
} SERVER_INFO_1516, *PSERVER_INFO_1516;

#endif


#ifndef SERVER_INFO_1518_DEFINED
#define SERVER_INFO_1518_DEFINED 1

typedef struct _SERVER_INFO_1518 {
    uint32 sv1518_lmannounce;
} SERVER_INFO_1518, *PSERVER_INFO_1518;

#endif


#ifndef SERVER_INFO_1520_DEFINED
#define SERVER_INFO_1520_DEFINED 1

typedef struct _SERVER_INFO_1520 {
    uint32 sv1520_maxcopyreadlen;
} SERVER_INFO_1520, *PSERVER_INFO_1520;

#endif


#ifndef SERVER_INFO_1521_DEFINED
#define SERVER_INFO_1521_DEFINED 1

typedef struct _SERVER_INFO_1521 {
    uint32 sv1521_maxcopywritelen;
} SERVER_INFO_1521, *PSERVER_INFO_1521;

#endif


#ifndef SERVER_INFO_1522_DEFINED
#define SERVER_INFO_1522_DEFINED 1

typedef struct _SERVER_INFO_1522 {
    uint32 sv1522_minkeepsearch;
} SERVER_INFO_1522, *PSERVER_INFO_1522;

#endif


#ifndef SERVER_INFO_1523_DEFINED
#define SERVER_INFO_1523_DEFINED 1

typedef struct _SERVER_INFO_1523 {
    uint32 sv1523_maxkeepsearch;
} SERVER_INFO_1523, *PSERVER_INFO_1523;

#endif


#ifndef SERVER_INFO_1524_DEFINED
#define SERVER_INFO_1524_DEFINED 1

typedef struct _SERVER_INFO_1524 {
    uint32 sv1524_minkeepcomplsearch;
} SERVER_INFO_1524, *PSERVER_INFO_1524;

#endif


#ifndef SERVER_INFO_1525_DEFINED
#define SERVER_INFO_1525_DEFINED 1

typedef struct _SERVER_INFO_1525 {
    uint32 sv1525_maxkeepcomplsearch;
} SERVER_INFO_1525, *PSERVER_INFO_1525;

#endif

#ifndef SERVER_INFO_1528_DEFINED
#define SERVER_INFO_1528_DEFINED 1

typedef struct _SERVER_INFO_1528 {
    uint32 sv1528_scavtimeout;
} SERVER_INFO_1528, *PSERVER_INFO_1528;

#endif


#ifndef SERVER_INFO_1529_DEFINED
#define SERVER_INFO_1529_DEFINED 1

typedef struct _SERVER_INFO_1529 {
    uint32 sv1529_minrcvqueue;
} SERVER_INFO_1529, *PSERVER_INFO_1529;

#endif


#ifndef SERVER_INFO_1530_DEFINED
#define SERVER_INFO_1530_DEFINED 1

typedef struct _SERVER_INFO_1530 {
    uint32 sv1530_minfreeworkitems;
} SERVER_INFO_1530, *PSERVER_INFO_1530;

#endif


#ifndef SERVER_INFO_1533_DEFINED
#define SERVER_INFO_1533_DEFINED 1

typedef struct _SERVER_INFO_1533 {
    uint32 sv1533_maxmpxct;
} SERVER_INFO_1533, *PSERVER_INFO_1533;

#endif


#ifndef SERVER_INFO_1534_DEFINED
#define SERVER_INFO_1534_DEFINED 1

typedef struct _SERVER_INFO_1534 {
    uint32 sv1534_oplockbreakwait;
} SERVER_INFO_1534, *PSERVER_INFO_1534;

#endif


#ifndef SERVER_INFO_1535_DEFINED
#define SERVER_INFO_1535_DEFINED 1

typedef struct _SERVER_INFO_1535 {
    uint32 sv1535_oplockbreakresponsewait;
} SERVER_INFO_1535, *PSERVER_INFO_1535;

#endif


#ifndef SERVER_INFO_1536_DEFINED
#define SERVER_INFO_1536_DEFINED 1

typedef struct _SERVER_INFO_1536 {
    uint32 sv1536_enableoplocks;
} SERVER_INFO_1536, *PSERVER_INFO_1536;

#endif


#ifndef SERVER_INFO_1537_DEFINED
#define SERVER_INFO_1537_DEFINED 1

typedef struct _SERVER_INFO_1537 {
    uint32 sv1537_enableoplockforceclose;
} SERVER_INFO_1537, *PSERVER_INFO_1537;

#endif

#ifndef SERVER_INFO_1538_DEFINED
#define SERVER_INFO_1538_DEFINED 1

typedef struct _SERVER_INFO_1538 {
    uint32 sv1538_enablefcbopens;
} SERVER_INFO_1538, *PSERVER_INFO_1538;

#endif


#ifndef SERVER_INFO_1539_DEFINED
#define SERVER_INFO_1539_DEFINED 1

typedef struct _SERVER_INFO_1539 {
    uint32 sv1539_enableraw;
} SERVER_INFO_1539, *PSERVER_INFO_1539;

#endif


#ifndef SERVER_INFO_1540_DEFINED
#define SERVER_INFO_1540_DEFINED 1

typedef struct _SERVER_INFO_1540 {
    uint32 sv1540_enablesharednetdrives;
} SERVER_INFO_1540, *PSERVER_INFO_1540;

#endif


#ifndef SERVER_INFO_1541_DEFINED
#define SERVER_INFO_1541_DEFINED 1

typedef struct _SERVER_INFO_1541 {
    uint32 sv1541_minfreeconnections;
} SERVER_INFO_1541, *PSERVER_INFO_1541;

#endif


#ifndef SERVER_INFO_1542_DEFINED
#define SERVER_INFO_1542_DEFINED 1

typedef struct _SERVER_INFO_1542 {
    uint32 sv1542_maxfreeconnections;
} SERVER_INFO_1542, *PSERVER_INFO_1542;

#endif


#ifndef SERVER_INFO_1543_DEFINED
#define SERVER_INFO_1543_DEFINED 1

typedef struct _SERVER_INFO_1543 {
    uint32 sv1543_initsesstable;
} SERVER_INFO_1543, *PSERVER_INFO_1543;

#endif


#ifndef SERVER_INFO_1544_DEFINED
#define SERVER_INFO_1544_DEFINED 1

typedef struct _SERVER_INFO_1544 {
    uint32 sv1544_initconntable;
} SERVER_INFO_1544, *PSERVER_INFO_1544;

#endif


#ifndef SERVER_INFO_1545_DEFINED
#define SERVER_INFO_1545_DEFINED 1

typedef struct _SERVER_INFO_1545 {
    uint32 sv1545_initfiletable;
} SERVER_INFO_1545, *PSERVER_INFO_1545;

#endif


#ifndef SERVER_INFO_1546_DEFINED
#define SERVER_INFO_1546_DEFINED 1

typedef struct _SERVER_INFO_1546 {
    uint32 sv1546_initsearchtable;
} SERVER_INFO_1546, *PSERVER_INFO_1546;

#endif


#ifndef SERVER_INFO_1547_DEFINED
#define SERVER_INFO_1547_DEFINED 1

typedef struct _SERVER_INFO_1547 {
    uint32 sv1547_alertsched;
} SERVER_INFO_1547, *PSERVER_INFO_1547;

#endif


#ifndef SERVER_INFO_1548_DEFINED
#define SERVER_INFO_1548_DEFINED 1

typedef struct _SERVER_INFO_1548 {
    uint32 sv1548_errorthreshold;
} SERVER_INFO_1548, *PSERVER_INFO_1548;

#endif


#ifndef SERVER_INFO_1549_DEFINED
#define SERVER_INFO_1549_DEFINED 1

typedef struct _SERVER_INFO_1549 {
    uint32 sv1549_networkerrorthreshold;
} SERVER_INFO_1549, *PSERVER_INFO_1549;

#endif


#ifndef SERVER_INFO_1550_DEFINED
#define SERVER_INFO_1550_DEFINED 1

typedef struct _SERVER_INFO_1550 {
    uint32 sv1550_diskspacethreshold;
} SERVER_INFO_1550, *PSERVER_INFO_1550;

#endif


#ifndef SERVER_INFO_1552_DEFINED
#define SERVER_INFO_1552_DEFINED 1

typedef struct _SERVER_INFO_1552 {
    uint32 sv1552_maxlinkdelay;
} SERVER_INFO_1552, *PSERVER_INFO_1552;

#endif


#ifndef SERVER_INFO_1553_DEFINED
#define SERVER_INFO_1553_DEFINED 1

typedef struct _SERVER_INFO_1553 {
    uint32 sv1553_minlinkthroughput;
} SERVER_INFO_1553, *PSERVER_INFO_1553;

#endif


#ifndef SERVER_INFO_1554_DEFINED
#define SERVER_INFO_1554_DEFINED 1

typedef struct _SERVER_INFO_1554 {
    uint32 sv1554_linkinfovalidtime;
} SERVER_INFO_1554, *PSERVER_INFO_1554;

#endif


#ifndef SERVER_INFO_1555_DEFINED
#define SERVER_INFO_1555_DEFINED 1

typedef struct _SERVER_INFO_1555 {
    uint32 sv1555_scavqosinfoupdatetime;
} SERVER_INFO_1555, *PSERVER_INFO_1555;

#endif


#ifndef SERVER_INFO_1556_DEFINED
#define SERVER_INFO_1556_DEFINED 1

typedef struct _SERVER_INFO_1556 {
    uint32 sv1556_maxworkitemidletime;
} SERVER_INFO_1556, *PSERVER_INFO_1556;

#endif


#ifndef TIME_OF_DAY_INFO_DEFINED
#define TIME_OF_DAY_INFO_DEFINED 1

typedef struct _TIME_OF_DAY_INFO {
    uint32 tod_elapsedt; /* time(NULL) */
    uint32 tod_msecs; /* milliseconds till system reboot (uptime) */
    uint32 tod_hours;
    uint32 tod_mins;
    uint32 tod_secs;
    uint32 tod_hunds;
    int32  tod_timezone; /* in minutes */
    uint32 tod_tinterval; /* clock tick interval in 0.0001 second units; 310 on windows */
    uint32 tod_day;
    uint32 tod_month;
    uint32 tod_year;
    uint32 tod_weekday;
} TIME_OF_DAY_INFO, *PTIME_OF_DAY_INFO;

#endif /* TIME_OF_DAY_INFO_DEFINED */

/*
 * Include the share info typedefs from lwio
 */
#include <lwio/lmshare.h>


#endif /* _SRVSVCDEFS_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
