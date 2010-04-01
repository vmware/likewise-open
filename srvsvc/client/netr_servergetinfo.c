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

#include "includes.h"

static
NET_API_STATUS
SrvSvcCopyNetSrvInfo(
    UINT32             level,
    srvsvc_NetSrvInfo* info,
    UINT8**            bufptr
    );

NET_API_STATUS
NetrServerGetInfo(
    PSRVSVC_CONTEXT pContext,
    const wchar16_t *servername,
    UINT32 level,
    UINT8 **bufptr
    )
{
    NET_API_STATUS status = ERROR_SUCCESS;
    dcethread_exc* pDceException = NULL;
    srvsvc_NetSrvInfo info;

    BAIL_ON_INVALID_PTR(pContext, status);
    BAIL_ON_INVALID_PTR(bufptr, status);

    memset(&info, 0, sizeof(info));
    *bufptr = NULL;

    TRY
    {
        status = _NetrServerGetInfo(
                    pContext->hBinding,
                    (wchar16_t *)servername,
                    level,
                    &info);
    }
    CATCH_ALL(pDceException)
    {
        NTSTATUS ntStatus = LwRpcStatusToNtStatus(pDceException->match.value);
        status = LwNtStatusToWin32Error(ntStatus);
    }
    ENDTRY;
    BAIL_ON_WIN_ERROR(status);

    status = SrvSvcCopyNetSrvInfo(level, &info, bufptr);
    BAIL_ON_WIN_ERROR(status);

cleanup:
    SrvSvcClearNetSrvInfo(level, &info);
    return status;

error:
    goto cleanup;
}

static
NET_API_STATUS
SrvSvcCopyNetSrvInfo(
    UINT32             level,
    srvsvc_NetSrvInfo* info,
    UINT8**            bufptr
    )
{
    NET_API_STATUS status = ERROR_SUCCESS;
    void *ptr = NULL;

    BAIL_ON_INVALID_PTR(bufptr, status);
    *bufptr = NULL;

    BAIL_ON_INVALID_PTR(info, status);

    switch (level) {
    case 100:
        if (info->info100) {
            PSERVER_INFO_100 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_100),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_100)ptr;

            *pServerInfo = *info->info100;

            pServerInfo->sv100_name = NULL;

            if (info->info100->sv100_name)
            {
                status = SrvSvcAddDepStringW(
                            pServerInfo,
                            info->info100->sv100_name,
                            &pServerInfo->sv100_name);
                BAIL_ON_WIN_ERROR(status);
            }
        }
        break;
    case 101:
        if (info->info101) {
            PSERVER_INFO_101 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_101),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_101)ptr;

            *pServerInfo = *info->info101;

            pServerInfo->sv101_name    = NULL;
            pServerInfo->sv101_comment = NULL;

            if (info->info101->sv101_name)
            {
                status = SrvSvcAddDepStringW(
                            pServerInfo,
                            info->info101->sv101_name,
                            &pServerInfo->sv101_name);
                BAIL_ON_WIN_ERROR(status);
            }
            if (info->info101->sv101_comment)
            {
                status = SrvSvcAddDepStringW(
                            pServerInfo,
                            info->info101->sv101_comment,
                            &pServerInfo->sv101_comment);
                BAIL_ON_WIN_ERROR(status);
            }
        }
        break;
    case 102:
        if (info->info102) {
            PSERVER_INFO_102 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_102),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_102)ptr;

            *pServerInfo = *info->info102;

            pServerInfo->sv102_name     = NULL;
            pServerInfo->sv102_comment  = NULL;
            pServerInfo->sv102_userpath = NULL;

            if (info->info102->sv102_name)
            {
                status = SrvSvcAddDepStringW(
                            pServerInfo,
                            info->info102->sv102_name,
                            &pServerInfo->sv102_name);
                BAIL_ON_WIN_ERROR(status);
            }
            if (info->info102->sv102_comment)
            {
                status = SrvSvcAddDepStringW(
                            pServerInfo,
                            info->info102->sv102_comment,
                            &pServerInfo->sv102_comment);
                BAIL_ON_WIN_ERROR(status);
            }
            if (info->info102->sv102_userpath)
            {
                status = SrvSvcAddDepStringW(
                            pServerInfo,
                            info->info102->sv102_userpath,
                            &pServerInfo->sv102_userpath);
                BAIL_ON_WIN_ERROR(status);
            }
        }
        break;
    case 402:
        if (info->info402) {
            PSERVER_INFO_402 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_402),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_402)ptr;

            *pServerInfo = *info->info402;

            pServerInfo->sv402_alerts        = NULL;
            pServerInfo->sv402_guestacct     = NULL;
            pServerInfo->sv402_srvheuristics = NULL;

            if (info->info402->sv402_alerts)
            {
                status = SrvSvcAddDepStringW(
                            pServerInfo,
                            info->info402->sv402_alerts,
                            &pServerInfo->sv402_alerts);
                BAIL_ON_WIN_ERROR(status);
            }
            if (info->info402->sv402_guestacct)
            {
                status = SrvSvcAddDepStringW(
                            pServerInfo,
                            info->info402->sv402_guestacct,
                            &pServerInfo->sv402_guestacct);
                BAIL_ON_WIN_ERROR(status);
            }
            if (info->info402->sv402_srvheuristics)
            {
                status = SrvSvcAddDepStringW(
                            pServerInfo,
                            info->info402->sv402_srvheuristics,
                            &pServerInfo->sv402_srvheuristics);
                BAIL_ON_WIN_ERROR(status);
            }
        }
        break;
    case 403:
        if (info->info403) {
            PSERVER_INFO_403 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_403),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_403)ptr;

            *pServerInfo = *info->info403;

            pServerInfo->sv403_alerts        = NULL;
            pServerInfo->sv403_guestacct     = NULL;
            pServerInfo->sv403_srvheuristics = NULL;
            pServerInfo->sv403_autopath      = NULL;

            if (info->info403->sv403_alerts)
            {
                status = SrvSvcAddDepStringW(
                            pServerInfo,
                            info->info403->sv403_alerts,
                            &pServerInfo->sv403_alerts);
                BAIL_ON_WIN_ERROR(status);
            }
            if (info->info403->sv403_guestacct)
            {
                status = SrvSvcAddDepStringW(
                            pServerInfo,
                            info->info403->sv403_guestacct,
                            &pServerInfo->sv403_guestacct);
                BAIL_ON_WIN_ERROR(status);
            }
            if (info->info403->sv403_srvheuristics)
            {
                status = SrvSvcAddDepStringW(
                            pServerInfo,
                            info->info403->sv403_srvheuristics,
                            &pServerInfo->sv403_srvheuristics);
                BAIL_ON_WIN_ERROR(status);
            }
            if (info->info403->sv403_autopath)
            {
                status = SrvSvcAddDepStringW(
                            pServerInfo,
                            info->info403->sv403_autopath,
                            &pServerInfo->sv403_autopath);
                BAIL_ON_WIN_ERROR(status);
            }
        }
        break;
    case 502:
        if (info->info502) {
            PSERVER_INFO_502 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_502),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_502)ptr;

            *pServerInfo = *info->info502;
        }
        break;
    case 503:
        if (info->info503) {
            PSERVER_INFO_503 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_503),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_503)ptr;

            *pServerInfo = *info->info503;

            pServerInfo->sv503_domain = NULL;

            if (info->info503->sv503_domain)
            {
                status = SrvSvcAddDepStringW(
                            pServerInfo,
                            info->info503->sv503_domain,
                            &pServerInfo->sv503_domain);
                BAIL_ON_WIN_ERROR(status);
            }
        }
        break;
    case 599:
        if (info->info599) {
            PSERVER_INFO_599 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_599),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_599)ptr;

            *pServerInfo = *info->info599;

            pServerInfo->sv599_domain = NULL;

            if (info->info599->sv599_domain)
            {
                status = SrvSvcAddDepStringW(
                            pServerInfo,
                            info->info599->sv599_domain,
                            &pServerInfo->sv599_domain);
                BAIL_ON_WIN_ERROR(status);
            }
        }
        break;
    case 1005:
        if (info->info1005) {
            PSERVER_INFO_1005 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1005),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_1005)ptr;

            *pServerInfo = *info->info1005;

            pServerInfo->sv1005_comment = NULL;

            if (info->info1005->sv1005_comment)
            {
                status = SrvSvcAddDepStringW(
                            pServerInfo,
                            info->info1005->sv1005_comment,
                            &pServerInfo->sv1005_comment);
                BAIL_ON_WIN_ERROR(status);
            }
        }
        break;
    case 1010:
        if (info->info1010) {
            PSERVER_INFO_1010 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1010),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_1010)ptr;

            *pServerInfo = *info->info1010;
        }
        break;
    case 1016:
        if (info->info1016) {
            PSERVER_INFO_1016 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1016),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_1016)ptr;

            *pServerInfo = *info->info1016;
        }
        break;
    case 1017:
        if (info->info1017) {
            PSERVER_INFO_1017 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1017),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_1017)ptr;

            *pServerInfo = *info->info1017;
        }
        break;
    case 1018:
        if (info->info1018) {
            PSERVER_INFO_1018 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1018),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_1018)ptr;

            *pServerInfo = *info->info1018;
        }
        break;
    case 1107:
        if (info->info1107) {
            PSERVER_INFO_1107 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1107),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_1107)ptr;

            *pServerInfo = *info->info1107;
        }
        break;
    case 1501:
        if (info->info1501) {
            PSERVER_INFO_1501 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1501),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_1501)ptr;

            *pServerInfo = *info->info1501;
        }
        break;
    case 1502:
        if (info->info1502) {
            PSERVER_INFO_1502 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1502),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_1502)ptr;

            *pServerInfo = *info->info1502;
        }
        break;
    case 1503:
        if (info->info1503) {
            PSERVER_INFO_1503 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1503),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_1503)ptr;

            *pServerInfo = *info->info1503;
        }
        break;
    case 1506:
        if (info->info1506) {
            PSERVER_INFO_1506 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1506),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_1506)ptr;

            *pServerInfo = *info->info1506;
        }
        break;
    case 1509:
        if (info->info1509) {
            PSERVER_INFO_1509 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1509),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_1509)ptr;

            *pServerInfo = *info->info1509;
        }
        break;
    case 1510:
        if (info->info1510) {
            PSERVER_INFO_1510 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1510),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_1510)ptr;

            *pServerInfo = *info->info1510;
        }
        break;
    case 1511:
        if (info->info1511) {
            PSERVER_INFO_1511 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1511),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_1511)ptr;

            *pServerInfo = *info->info1511;
        }
        break;
    case 1512:
        if (info->info1512) {
            PSERVER_INFO_1512 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1512),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_1512)ptr;

            *pServerInfo = *info->info1512;
        }
        break;
    case 1513:
        if (info->info1513) {
            PSERVER_INFO_1513 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1513),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_1513)ptr;

            *pServerInfo = *info->info1513;
        }
        break;
    case 1514:
        if (info->info1514) {
            PSERVER_INFO_1514 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1514),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_1514)ptr;

            *pServerInfo = *info->info1514;
        }
        break;
    case 1515:
        if (info->info1515) {
            PSERVER_INFO_1515 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1515),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_1515)ptr;

            *pServerInfo = *info->info1515;
        }
        break;
    case 1516:
        if (info->info1516) {
            PSERVER_INFO_1516 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1516),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_1516)ptr;

            *pServerInfo = *info->info1516;
        }
        break;
    case 1518:
        if (info->info1518) {
            PSERVER_INFO_1518 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1518),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_1518)ptr;

            *pServerInfo = *info->info1518;
        }
        break;
    case 1520:
        if (info->info1520) {
            PSERVER_INFO_1520 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1520),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_1520)ptr;

            *pServerInfo = *info->info1520;
        }
        break;
    case 1521:
        if (info->info1521) {
            PSERVER_INFO_1521 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1521),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_1521)ptr;

            *pServerInfo = *info->info1521;
        }
        break;
    case 1522:
        if (info->info1522) {
            PSERVER_INFO_1522 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1522),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_1522)ptr;

            *pServerInfo = *info->info1522;
        }
        break;
    case 1523:
        if (info->info1523) {
            PSERVER_INFO_1523 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1523),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_1523)ptr;

            *pServerInfo = *info->info1523;
        }
        break;
    case 1524:
        if (info->info1524) {
            PSERVER_INFO_1524 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1524),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_1524)ptr;

            *pServerInfo = *info->info1524;
        }
        break;
    case 1525:
        if (info->info1525) {
            PSERVER_INFO_1525 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1525),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_1525)ptr;

            *pServerInfo = *info->info1525;
        }
        break;
    case 1528:
        if (info->info1528) {
            PSERVER_INFO_1528 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1528),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_1528)ptr;

            *pServerInfo = *info->info1528;
        }
        break;
    case 1529:
        if (info->info1529) {
            PSERVER_INFO_1529 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1529),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_1529)ptr;

            *pServerInfo = *info->info1529;
        }
        break;
    case 1530:
        if (info->info1530) {
            PSERVER_INFO_1530 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1530),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_1530)ptr;

            *pServerInfo = *info->info1530;
        }
        break;
    case 1533:
        if (info->info1533) {
            PSERVER_INFO_1533 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1533),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_1533)ptr;

            *pServerInfo = *info->info1533;
        }
        break;
    case 1534:
        if (info->info1534) {
            PSERVER_INFO_1534 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1534),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_1534)ptr;

            *pServerInfo = *info->info1534;
        }
        break;
    case 1535:
        if (info->info1535) {
            PSERVER_INFO_1535 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1535),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_1535)ptr;

            *pServerInfo = *info->info1535;
        }
        break;
    case 1536:
        if (info->info1536) {
            PSERVER_INFO_1536 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1536),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_1536)ptr;

            *pServerInfo = *info->info1536;
        }
        break;
    case 1537:
        if (info->info1537) {
            PSERVER_INFO_1537 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1537),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_1537)ptr;

            *pServerInfo = *info->info1537;
        }
        break;
    case 1538:
        if (info->info1538) {
            PSERVER_INFO_1538 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1538),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_1538)ptr;

            *pServerInfo = *info->info1538;
        }
        break;
    case 1539:
        if (info->info1539) {
            PSERVER_INFO_1539 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1539),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_1539)ptr;

            *pServerInfo = *info->info1539;
        }
        break;
    case 1540:
        if (info->info1540) {
            PSERVER_INFO_1540 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1540),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_1540)ptr;

            *pServerInfo = *info->info1540;
        }
        break;
    case 1541:
        if (info->info1541) {
            PSERVER_INFO_1541 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1541),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_1541)ptr;

            *pServerInfo = *info->info1541;
        }
        break;
    case 1542:
        if (info->info1542) {
            PSERVER_INFO_1542 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1542),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_1542)ptr;

            *pServerInfo = *info->info1542;
        }
        break;
    case 1543:
        if (info->info1543) {
            PSERVER_INFO_1543 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1543),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_1543)ptr;

            *pServerInfo = *info->info1543;
        }
        break;
    case 1544:
        if (info->info1544) {
            PSERVER_INFO_1544 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1544),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_1544)ptr;

            *pServerInfo = *info->info1544;
        }
        break;
    case 1545:
        if (info->info1545) {
            PSERVER_INFO_1545 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1545),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_1545)ptr;

            *pServerInfo = *info->info1545;
        }
        break;
    case 1546:
        if (info->info1546) {
            PSERVER_INFO_1546 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1546),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_1546)ptr;

            *pServerInfo = *info->info1546;
        }
        break;
    case 1547:
        if (info->info1547) {
            PSERVER_INFO_1547 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1547),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_1547)ptr;

            *pServerInfo = *info->info1547;
        }
        break;
    case 1548:
        if (info->info1548) {
            PSERVER_INFO_1548 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1548),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_1548)ptr;

            *pServerInfo = *info->info1548;
        }
        break;
    case 1549:
        if (info->info1549) {
            PSERVER_INFO_1549 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1549),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_1549)ptr;

            *pServerInfo = *info->info1549;
        }
        break;
    case 1550:
        if (info->info1550) {
            PSERVER_INFO_1550 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1550),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_1550)ptr;

            *pServerInfo = *info->info1550;
        }
        break;
    case 1552:
        if (info->info1552) {
            PSERVER_INFO_1552 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1552),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_1552)ptr;

            *pServerInfo = *info->info1552;
        }
        break;
    case 1553:
        if (info->info1553) {
            PSERVER_INFO_1553 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1553),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_1553)ptr;

            *pServerInfo = *info->info1553;
        }
        break;
    case 1554:
        if (info->info1554) {
            PSERVER_INFO_1554 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1554),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_1554)ptr;

            *pServerInfo = *info->info1554;
        }
        break;
    case 1555:
        if (info->info1555) {
            PSERVER_INFO_1555 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1555),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_1555)ptr;

            *pServerInfo = *info->info1555;
        }
        break;
    case 1556:
        if (info->info1556) {
            PSERVER_INFO_1556 pServerInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1556),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pServerInfo = (PSERVER_INFO_1556)ptr;

            *pServerInfo = *info->info1556;
        }
        break;
    }

    *bufptr = (UINT8 *)ptr;

cleanup:
    return status;

error:
    if (ptr) {
        SrvSvcFreeMemory(ptr);
    }
    goto cleanup;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
