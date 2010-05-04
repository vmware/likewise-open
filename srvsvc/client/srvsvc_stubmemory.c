/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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

#include "includes.h"


static void SrvSvcClearCONNECTION_INFO_1(PCONNECTION_INFO_1 info)
{
    if (!info) {
        return;
    }

    SRVSVC_SAFE_FREE(info->coni1_username);
    SRVSVC_SAFE_FREE(info->coni1_netname);
}

void SrvSvcClearNetConnCtr(UINT32 level, srvsvc_NetConnCtr *ctr)
{
    int i;

    if (!ctr) {
        return;
    }

    switch (level) {
    case 0:
        if (!ctr->ctr0) {
            break;
        }

        SRVSVC_SAFE_FREE(ctr->ctr0->array);
        SRVSVC_SAFE_FREE(ctr->ctr0);
        break;
    case 1:
        if (!ctr->ctr1) {
            break;
        }

        for (i=0; i < ctr->ctr1->count; i++) {
             SrvSvcClearCONNECTION_INFO_1(&ctr->ctr1->array[i]);
        }
        SRVSVC_SAFE_FREE(ctr->ctr1->array);
        SRVSVC_SAFE_FREE(ctr->ctr1);
        break;
    }
}

static void SrvSvcClearFILE_INFO_3(PFILE_INFO_3 info)
{
    if (!info) {
        return;
    }

    SRVSVC_SAFE_FREE(info->fi3_path_name);
    SRVSVC_SAFE_FREE(info->fi3_username);
}

void SrvSvcClearNetFileCtr(UINT32 level, srvsvc_NetFileCtr *ctr)
{
    int i;

    if (!ctr) {
        return;
    }

    switch (level) {
    case 2:
        if (!ctr->ctr2) {
            break;
        }

        SRVSVC_SAFE_FREE(ctr->ctr2->array);
        SRVSVC_SAFE_FREE(ctr->ctr2);
        break;
    case 3:
        if (!ctr->ctr3) {
            break;
        }

        for (i=0; i < ctr->ctr3->count; i++) {
             SrvSvcClearFILE_INFO_3(&ctr->ctr3->array[i]);
        }
        SRVSVC_SAFE_FREE(ctr->ctr3->array);
        SRVSVC_SAFE_FREE(ctr->ctr3);
        break;
    }
}

void SrvSvcClearNetFileInfo(UINT32 level, srvsvc_NetFileInfo *info)
{
    if (!info) {
        return;
    }

    switch (level) {
    case 2:
        if (!info->info2) {
            break;
        }

        SRVSVC_SAFE_FREE(info->info2);
        break;
    case 3:
        if (!info->info3) {
            break;
        }

        SrvSvcClearFILE_INFO_3(info->info3);
        SRVSVC_SAFE_FREE(info->info3);
        break;
    }
}

static void SrvSvcClearSESSION_INFO_0(PSESSION_INFO_0 info)
{
    if (!info) {
        return;
    }

    SRVSVC_SAFE_FREE(info->sesi0_cname);
}

static void SrvSvcClearSESSION_INFO_1(PSESSION_INFO_1 info)
{
    if (!info) {
        return;
    }

    SRVSVC_SAFE_FREE(info->sesi1_cname);
    SRVSVC_SAFE_FREE(info->sesi1_username);
}

static void SrvSvcClearSESSION_INFO_2(PSESSION_INFO_2 info)
{
    if (!info) {
        return;
    }

    SRVSVC_SAFE_FREE(info->sesi2_cname);
    SRVSVC_SAFE_FREE(info->sesi2_username);
    SRVSVC_SAFE_FREE(info->sesi2_cltype_name);
}

static void SrvSvcClearSESSION_INFO_10(PSESSION_INFO_10 info)
{
    if (!info) {
        return;
    }

    SRVSVC_SAFE_FREE(info->sesi10_cname);
    SRVSVC_SAFE_FREE(info->sesi10_username);
}

static void SrvSvcClearSESSION_INFO_502(PSESSION_INFO_502 info)
{
    if (!info) {
        return;
    }

    SRVSVC_SAFE_FREE(info->sesi502_cname);
    SRVSVC_SAFE_FREE(info->sesi502_username);
    SRVSVC_SAFE_FREE(info->sesi502_cltype_name);
    SRVSVC_SAFE_FREE(info->sesi502_transport);
}

void SrvSvcClearNetSessCtr(UINT32 level, srvsvc_NetSessCtr *ctr)
{
    int i;

    if (!ctr) {
        return;
    }

    switch (level) {
    case 0:
        if (!ctr->ctr0) {
            break;
        }

        for (i=0; i < ctr->ctr0->count; i++) {
             SrvSvcClearSESSION_INFO_0(&ctr->ctr0->array[i]);
        }
        SRVSVC_SAFE_FREE(ctr->ctr0->array);
        SRVSVC_SAFE_FREE(ctr->ctr0);
        break;
    case 1:
        if (!ctr->ctr1) {
            break;
        }

        for (i=0; i < ctr->ctr1->count; i++) {
             SrvSvcClearSESSION_INFO_1(&ctr->ctr1->array[i]);
        }
        SRVSVC_SAFE_FREE(ctr->ctr1->array);
        SRVSVC_SAFE_FREE(ctr->ctr1);
        break;
    case 2:
        if (!ctr->ctr2) {
            break;
        }

        for (i=0; i < ctr->ctr2->count; i++) {
             SrvSvcClearSESSION_INFO_2(&ctr->ctr2->array[i]);
        }
        SRVSVC_SAFE_FREE(ctr->ctr2->array);
        SRVSVC_SAFE_FREE(ctr->ctr2);
        break;
    case 10:
        if (!ctr->ctr10) {
            break;
        }

        for (i=0; i < ctr->ctr10->count; i++) {
             SrvSvcClearSESSION_INFO_10(&ctr->ctr10->array[i]);
        }
        SRVSVC_SAFE_FREE(ctr->ctr10->array);
        SRVSVC_SAFE_FREE(ctr->ctr10);
        break;
    case 502:
        if (!ctr->ctr502) {
            break;
        }

        for (i=0; i < ctr->ctr502->count; i++) {
             SrvSvcClearSESSION_INFO_502(&ctr->ctr502->array[i]);
        }
        SRVSVC_SAFE_FREE(ctr->ctr502->array);
        SRVSVC_SAFE_FREE(ctr->ctr502);
        break;
    }
}

static void SrvSvcClearSHARE_INFO_0(PSHARE_INFO_0 info)
{
    if (!info) {
        return;
    }

    SRVSVC_SAFE_FREE(info->shi0_netname);
}

static void SrvSvcClearSHARE_INFO_1(PSHARE_INFO_1 info)
{
    if (!info) {
        return;
    }

    SRVSVC_SAFE_FREE(info->shi1_netname);
    SRVSVC_SAFE_FREE(info->shi1_remark);
}

static void SrvSvcClearSHARE_INFO_2(PSHARE_INFO_2 info)
{
    if (!info) {
        return;
    }

    SRVSVC_SAFE_FREE(info->shi2_netname);
    SRVSVC_SAFE_FREE(info->shi2_remark);
    SRVSVC_SAFE_FREE(info->shi2_path);
    SRVSVC_SAFE_FREE(info->shi2_password);
}

static void SrvSvcClearSHARE_INFO_501(PSHARE_INFO_501 info)
{
    if (!info) {
        return;
    }

    SRVSVC_SAFE_FREE(info->shi501_netname);
    SRVSVC_SAFE_FREE(info->shi501_remark);
}

static void SrvSvcClearSHARE_INFO_502_I(PSHARE_INFO_502_I info)
{
    if (!info) {
        return;
    }

    SRVSVC_SAFE_FREE(info->shi502_netname);
    SRVSVC_SAFE_FREE(info->shi502_remark);
    SRVSVC_SAFE_FREE(info->shi502_path);
    SRVSVC_SAFE_FREE(info->shi502_password);
    SRVSVC_SAFE_FREE(info->shi502_security_descriptor);
}

void SrvSvcClearNetShareCtr(UINT32 level, srvsvc_NetShareCtr *ctr)
{
    int i;

    if (!ctr) {
        return;
    }

    switch (level) {
    case 0:
        if (!ctr->ctr0) {
            break;
        }

        for (i=0; i < ctr->ctr0->count; i++) {
             SrvSvcClearSHARE_INFO_0(&ctr->ctr0->array[i]);
        }
        SRVSVC_SAFE_FREE(ctr->ctr0->array);
        break;
    case 1:
        if (!ctr->ctr1) {
            break;
        }

        for (i=0; i < ctr->ctr1->count; i++) {
             SrvSvcClearSHARE_INFO_1(&ctr->ctr1->array[i]);
        }
        SRVSVC_SAFE_FREE(ctr->ctr1->array);
        break;
    case 2:
        if (!ctr->ctr2) {
            break;
        }

        for (i=0; i < ctr->ctr2->count; i++) {
             SrvSvcClearSHARE_INFO_2(&ctr->ctr2->array[i]);
        }
        SRVSVC_SAFE_FREE(ctr->ctr2->array);
        break;
    case 501:
        if (!ctr->ctr501) {
            break;
        }

        for (i=0; i < ctr->ctr501->count; i++) {
             SrvSvcClearSHARE_INFO_501(&ctr->ctr501->array[i]);
        }
        SRVSVC_SAFE_FREE(ctr->ctr501->array);
        break;
    case 502:
        if (!ctr->ctr502) {
            break;
        }

        for (i=0; i < ctr->ctr502->count; i++) {
             SrvSvcClearSHARE_INFO_502_I(&ctr->ctr502->array[i]);
        }
        SRVSVC_SAFE_FREE(ctr->ctr502->array);
        break;
    }
}

void SrvSvcClearNetShareInfo(UINT32 level, srvsvc_NetShareInfo *info)
{
    if (!info) {
        return;
    }

    switch (level) {
    case 0:
        if (!info->info0) {
            break;
        }

        SrvSvcClearSHARE_INFO_0(info->info0);
        SRVSVC_SAFE_FREE(info->info0);
        break;
    case 1:
        if (!info->info1) {
            break;
        }

        SrvSvcClearSHARE_INFO_1(info->info1);
        SRVSVC_SAFE_FREE(info->info1);
        break;
    case 2:
        if (!info->info2) {
            break;
        }

        SrvSvcClearSHARE_INFO_2(info->info2);
        SRVSVC_SAFE_FREE(info->info2);
        break;
    case 501:
        if (!info->info501) {
            break;
        }

        SrvSvcClearSHARE_INFO_501(info->info501);
        SRVSVC_SAFE_FREE(info->info501);
        break;
    case 502:
        if (!info->info502) {
            break;
        }

        SrvSvcClearSHARE_INFO_502_I(info->info502);
        SRVSVC_SAFE_FREE(info->info502);
        break;

    case 1005:
        if (!info->info1005) {
            break;
        }

        SRVSVC_SAFE_FREE(info->info1005);
        break;
    }
}

static void SrvSvcClearSERVER_INFO_100(PSERVER_INFO_100 info)
{
    if (!info) {
        return;
    }

    SRVSVC_SAFE_FREE(info->sv100_name);
}

static void SrvSvcClearSERVER_INFO_101(PSERVER_INFO_101 info)
{
    if (!info) {
        return;
    }

    SRVSVC_SAFE_FREE(info->sv101_name);
    SRVSVC_SAFE_FREE(info->sv101_comment);
}

static void SrvSvcClearSERVER_INFO_102(PSERVER_INFO_102 info)
{
    if (!info) {
        return;
    }

    SRVSVC_SAFE_FREE(info->sv102_name);
    SRVSVC_SAFE_FREE(info->sv102_comment);
    SRVSVC_SAFE_FREE(info->sv102_userpath);
}

static void SrvSvcClearSERVER_INFO_402(PSERVER_INFO_402 info)
{
    if (!info) {
        return;
    }

    SRVSVC_SAFE_FREE(info->sv402_alerts);
    SRVSVC_SAFE_FREE(info->sv402_guestacct);
    SRVSVC_SAFE_FREE(info->sv402_srvheuristics);
}

static void SrvSvcClearSERVER_INFO_403(PSERVER_INFO_403 info)
{
    if (!info) {
        return;
    }

    SRVSVC_SAFE_FREE(info->sv403_alerts);
    SRVSVC_SAFE_FREE(info->sv403_guestacct);
    SRVSVC_SAFE_FREE(info->sv403_srvheuristics);
    SRVSVC_SAFE_FREE(info->sv403_autopath);
}

static void SrvSvcClearSERVER_INFO_503(PSERVER_INFO_503 info)
{
    if (!info) {
        return;
    }

    SRVSVC_SAFE_FREE(info->sv503_domain);
}

static void SrvSvcClearSERVER_INFO_599(PSERVER_INFO_599 info)
{
    if (!info) {
        return;
    }

    SRVSVC_SAFE_FREE(info->sv599_domain);
}

static void SrvSvcClearSERVER_INFO_1005(PSERVER_INFO_1005 info)
{
    if (!info) {
        return;
    }

    SRVSVC_SAFE_FREE(info->sv1005_comment);
}

void SrvSvcClearNetSrvInfo(UINT32 level, srvsvc_NetSrvInfo *info)
{
    if (!info) {
        return;
    }

    switch (level) {
    case 100:
        if (!info->info100) {
            break;
        }

        SrvSvcClearSERVER_INFO_100(info->info100);
        SRVSVC_SAFE_FREE(info->info100);
        break;
    case 101:
        if (!info->info101) {
            break;
        }

        SrvSvcClearSERVER_INFO_101(info->info101);
        SRVSVC_SAFE_FREE(info->info101);
        break;
    case 102:
        if (!info->info102) {
            break;
        }

        SrvSvcClearSERVER_INFO_102(info->info102);
        SRVSVC_SAFE_FREE(info->info102);
        break;
    case 402:
        if (!info->info402) {
            break;
        }

        SrvSvcClearSERVER_INFO_402(info->info402);
        SRVSVC_SAFE_FREE(info->info402);
        break;
    case 403:
        if (!info->info403) {
            break;
        }

        SrvSvcClearSERVER_INFO_403(info->info403);
        SRVSVC_SAFE_FREE(info->info403);
        break;
    case 502:
        if (!info->info502) {
            break;
        }

        SRVSVC_SAFE_FREE(info->info502);
        break;
    case 503:
        if (!info->info503) {
            break;
        }

        SrvSvcClearSERVER_INFO_503(info->info503);
        SRVSVC_SAFE_FREE(info->info503);
        break;
    case 599:
        if (!info->info599) {
            break;
        }

        SrvSvcClearSERVER_INFO_599(info->info599);
        SRVSVC_SAFE_FREE(info->info599);
        break;
    case 1005:
        if (!info->info1005) {
            break;
        }

        SrvSvcClearSERVER_INFO_1005(info->info1005);
        SRVSVC_SAFE_FREE(info->info1005);
        break;
    case 1010:
        if (!info->info1010) {
            break;
        }

        SRVSVC_SAFE_FREE(info->info1010);
        break;
    case 1016:
        if (info->info1016) {
            break;
        }

        SRVSVC_SAFE_FREE(info->info1016);
        break;
    case 1017:
        if (info->info1017) {
            break;
        }

        SRVSVC_SAFE_FREE(info->info1017);
        break;
    case 1018:
        if (info->info1018) {
            break;
        }

        SRVSVC_SAFE_FREE(info->info1018);
        break;
    case 1107:
        if (info->info1107) {
            break;
        }

        SRVSVC_SAFE_FREE(info->info1107);
        break;
    case 1501:
        if (info->info1501) {
            break;
        }

        SRVSVC_SAFE_FREE(info->info1501);
        break;
    case 1502:
        if (info->info1502) {
            break;
        }

        SRVSVC_SAFE_FREE(info->info1502);
        break;
    case 1503:
        if (info->info1503) {
            break;
        }

        SRVSVC_SAFE_FREE(info->info1503);
        break;
    case 1506:
        if (info->info1506) {
            break;
        }

        SRVSVC_SAFE_FREE(info->info1506);
        break;
    case 1509:
        if (info->info1509) {
            break;
        }

        SRVSVC_SAFE_FREE(info->info1509);
        break;
    case 1510:
        if (info->info1510) {
            break;
        }

        SRVSVC_SAFE_FREE(info->info1510);
        break;
    case 1511:
        if (info->info1511) {
            break;
        }

        SRVSVC_SAFE_FREE(info->info1511);
        break;
    case 1512:
        if (info->info1512) {
            break;
        }

        SRVSVC_SAFE_FREE(info->info1512);
        break;
    case 1513:
        if (info->info1513) {
            break;
        }

        SRVSVC_SAFE_FREE(info->info1513);
        break;
    case 1514:
        if (info->info1514) {
            break;
        }

        SRVSVC_SAFE_FREE(info->info1514);
        break;
    case 1515:
        if (info->info1515) {
            break;
        }

        SRVSVC_SAFE_FREE(info->info1515);
        break;
    case 1516:
        if (info->info1516) {
            break;
        }

        SRVSVC_SAFE_FREE(info->info1516);
        break;
    case 1518:
        if (info->info1518) {
            break;
        }

        SRVSVC_SAFE_FREE(info->info1518);
        break;
    case 1520:
        if (info->info1520) {
            break;
        }

        SRVSVC_SAFE_FREE(info->info1520);
        break;
    case 1521:
        if (info->info1521) {
            break;
        }

        SRVSVC_SAFE_FREE(info->info1521);
        break;
    case 1522:
        if (info->info1522) {
            break;
        }

        SRVSVC_SAFE_FREE(info->info1522);
        break;
    case 1523:
        if (info->info1523) {
            break;
        }

        SRVSVC_SAFE_FREE(info->info1523);
        break;
    case 1524:
        if (info->info1524) {
            break;
        }

        SRVSVC_SAFE_FREE(info->info1524);
        break;
    case 1525:
        if (info->info1525) {
            break;
        }

        SRVSVC_SAFE_FREE(info->info1525);
        break;
    case 1528:
        if (info->info1528) {
            break;
        }

        SRVSVC_SAFE_FREE(info->info1528);
        break;
    case 1529:
        if (info->info1529) {
            break;
        }

        SRVSVC_SAFE_FREE(info->info1529);
        break;
    case 1530:
        if (info->info1530) {
            break;
        }

        SRVSVC_SAFE_FREE(info->info1530);
        break;
    case 1533:
        if (info->info1533) {
            break;
        }

        SRVSVC_SAFE_FREE(info->info1533);
        break;
    case 1534:
        if (info->info1534) {
            break;
        }

        SRVSVC_SAFE_FREE(info->info1534);
        break;
    case 1535:
        if (info->info1535) {
            break;
        }

        SRVSVC_SAFE_FREE(info->info1535);
        break;
    case 1536:
        if (info->info1536) {
            break;
        }

        SRVSVC_SAFE_FREE(info->info1536);
        break;
    case 1537:
        if (info->info1537) {
            break;
        }

        SRVSVC_SAFE_FREE(info->info1537);
        break;
    case 1538:
        if (info->info1538) {
            break;
        }

        SRVSVC_SAFE_FREE(info->info1538);
        break;
    case 1539:
        if (info->info1539) {
            break;
        }

        SRVSVC_SAFE_FREE(info->info1539);
        break;
    case 1540:
        if (info->info1540) {
            break;
        }

        SRVSVC_SAFE_FREE(info->info1540);
        break;
    case 1541:
        if (info->info1541) {
            break;
        }

        SRVSVC_SAFE_FREE(info->info1541);
        break;
    case 1542:
        if (info->info1542) {
            break;
        }

        SRVSVC_SAFE_FREE(info->info1542);
        break;
    case 1543:
        if (info->info1543) {
            break;
        }

        SRVSVC_SAFE_FREE(info->info1543);
        break;
    case 1544:
        if (info->info1544) {
            break;
        }

        SRVSVC_SAFE_FREE(info->info1544);
        break;
    case 1545:
        if (info->info1545) {
            break;
        }

        SRVSVC_SAFE_FREE(info->info1545);
        break;
    case 1546:
        if (info->info1546) {
            break;
        }

        SRVSVC_SAFE_FREE(info->info1546);
        break;
    case 1547:
        if (info->info1547) {
            break;
        }

        SRVSVC_SAFE_FREE(info->info1547);
        break;
    case 1548:
        if (info->info1548) {
            break;
        }

        SRVSVC_SAFE_FREE(info->info1548);
        break;
    case 1549:
        if (info->info1549) {
            break;
        }

        SRVSVC_SAFE_FREE(info->info1549);
        break;
    case 1550:
        if (info->info1550) {
            break;
        }

        SRVSVC_SAFE_FREE(info->info1550);
        break;
    case 1552:
        if (info->info1552) {
            break;
        }

        SRVSVC_SAFE_FREE(info->info1552);
        break;
    case 1553:
        if (info->info1553) {
            break;
        }

        SRVSVC_SAFE_FREE(info->info1553);
        break;
    case 1554:
        if (info->info1554) {
            break;
        }

        SRVSVC_SAFE_FREE(info->info1554);
        break;
    case 1555:
        if (info->info1555) {
            break;
        }

        SRVSVC_SAFE_FREE(info->info1555);
        break;
    case 1556:
        if (info->info1556) {
            break;
        }

        SRVSVC_SAFE_FREE(info->info1556);
        break;
    }
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
