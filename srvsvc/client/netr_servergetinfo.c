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
            PSERVER_INFO_100 a100;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_100),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a100 = (PSERVER_INFO_100)ptr;

            *a100 = *info->info100;

            if (a100->sv100_name)
            {
                status = SrvSvcAddDepStringW(a100, a100->sv100_name);
                BAIL_ON_WIN_ERROR(status);
            }
        }
        break;
    case 101:
        if (info->info101) {
            PSERVER_INFO_101 a101;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_101),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a101 = (PSERVER_INFO_101)ptr;

            *a101 = *info->info101;

            if (a101->sv101_name)
            {
                status = SrvSvcAddDepStringW(a101, a101->sv101_name);
                BAIL_ON_WIN_ERROR(status);
            }
            if (a101->sv101_comment)
            {
                status = SrvSvcAddDepStringW(a101, a101->sv101_comment);
                BAIL_ON_WIN_ERROR(status);
            }
        }
        break;
    case 102:
        if (info->info102) {
            PSERVER_INFO_102 a102;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_102),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a102 = (PSERVER_INFO_102)ptr;

            *a102 = *info->info102;
            if (a102->sv102_name)
            {
                status = SrvSvcAddDepStringW(a102, a102->sv102_name);
                BAIL_ON_WIN_ERROR(status);
            }
            if (a102->sv102_comment)
            {
                status = SrvSvcAddDepStringW(a102, a102->sv102_comment);
                BAIL_ON_WIN_ERROR(status);
            }
            if (a102->sv102_userpath)
            {
                status = SrvSvcAddDepStringW(a102, a102->sv102_userpath);
                BAIL_ON_WIN_ERROR(status);
            }
        }
        break;
    case 402:
        if (info->info402) {
            PSERVER_INFO_402 a402;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_402),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a402 = (PSERVER_INFO_402)ptr;

            *a402 = *info->info402;

            if (a402->sv402_alerts)
            {
                status = SrvSvcAddDepStringW(a402, a402->sv402_alerts);
                BAIL_ON_WIN_ERROR(status);
            }
            if (a402->sv402_guestacct)
            {
                status = SrvSvcAddDepStringW(a402, a402->sv402_guestacct);
                BAIL_ON_WIN_ERROR(status);
            }
            if (a402->sv402_srvheuristics)
            {
                status = SrvSvcAddDepStringW(a402, a402->sv402_srvheuristics);
                BAIL_ON_WIN_ERROR(status);
            }
        }
        break;
    case 403:
        if (info->info403) {
            PSERVER_INFO_403 a403;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_403),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a403 = (PSERVER_INFO_403)ptr;

            *a403 = *info->info403;
            if (a403->sv403_alerts)
            {
                status = SrvSvcAddDepStringW(a403, a403->sv403_alerts);
                BAIL_ON_WIN_ERROR(status);
            }
            if (a403->sv403_guestacct)
            {
                status = SrvSvcAddDepStringW(a403, a403->sv403_guestacct);
                BAIL_ON_WIN_ERROR(status);
            }
            if (a403->sv403_srvheuristics)
            {
                status = SrvSvcAddDepStringW(a403, a403->sv403_srvheuristics);
                BAIL_ON_WIN_ERROR(status);
            }
            if (a403->sv403_autopath)
            {
                status = SrvSvcAddDepStringW(a403, a403->sv403_autopath);
                BAIL_ON_WIN_ERROR(status);
            }
        }
        break;
    case 502:
        if (info->info502) {
            PSERVER_INFO_502 a502;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_502),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a502 = (PSERVER_INFO_502)ptr;

            *a502 = *info->info502;
        }
        break;
    case 503:
        if (info->info503) {
            PSERVER_INFO_503 a503;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_503),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a503 = (PSERVER_INFO_503)ptr;

            *a503 = *info->info503;

            if (a503->sv503_domain)
            {
                status = SrvSvcAddDepStringW(a503, a503->sv503_domain);
                BAIL_ON_WIN_ERROR(status);
            }
        }
        break;
    case 599:
        if (info->info599) {
            PSERVER_INFO_599 a599;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_599),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a599 = (PSERVER_INFO_599)ptr;

            *a599 = *info->info599;

            if (a599->sv599_domain)
            {
                status = SrvSvcAddDepStringW(a599, a599->sv599_domain);
                BAIL_ON_WIN_ERROR(status);
            }
        }
        break;
    case 1005:
        if (info->info1005) {
            PSERVER_INFO_1005 a1005;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1005),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1005 = (PSERVER_INFO_1005)ptr;

            *a1005 = *info->info1005;

            if (a1005->sv1005_comment)
            {
                status = SrvSvcAddDepStringW(a1005, a1005->sv1005_comment);
                BAIL_ON_WIN_ERROR(status);
            }
        }
        break;
    case 1010:
        if (info->info1010) {
            PSERVER_INFO_1010 a1010;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1010),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1010 = (PSERVER_INFO_1010)ptr;

            *a1010 = *info->info1010;
        }
        break;
    case 1016:
        if (info->info1016) {
            PSERVER_INFO_1016 a1016;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1016),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1016 = (PSERVER_INFO_1016)ptr;

            *a1016 = *info->info1016;
        }
        break;
    case 1017:
        if (info->info1017) {
            PSERVER_INFO_1017 a1017;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1017),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1017 = (PSERVER_INFO_1017)ptr;

            *a1017 = *info->info1017;
        }
        break;
    case 1018:
        if (info->info1018) {
            PSERVER_INFO_1018 a1018;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1018),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1018 = (PSERVER_INFO_1018)ptr;

            *a1018 = *info->info1018;
        }
        break;
    case 1107:
        if (info->info1107) {
            PSERVER_INFO_1107 a1107;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1107),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1107 = (PSERVER_INFO_1107)ptr;

            *a1107 = *info->info1107;
        }
        break;
    case 1501:
        if (info->info1501) {
            PSERVER_INFO_1501 a1501;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1501),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1501 = (PSERVER_INFO_1501)ptr;

            *a1501 = *info->info1501;
        }
        break;
    case 1502:
        if (info->info1502) {
            PSERVER_INFO_1502 a1502;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1502),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1502 = (PSERVER_INFO_1502)ptr;

            *a1502 = *info->info1502;
        }
        break;
    case 1503:
        if (info->info1503) {
            PSERVER_INFO_1503 a1503;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1503),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1503 = (PSERVER_INFO_1503)ptr;

            *a1503 = *info->info1503;
        }
        break;
    case 1506:
        if (info->info1506) {
            PSERVER_INFO_1506 a1506;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1506),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1506 = (PSERVER_INFO_1506)ptr;

            *a1506 = *info->info1506;
        }
        break;
    case 1509:
        if (info->info1509) {
            PSERVER_INFO_1509 a1509;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1509),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1509 = (PSERVER_INFO_1509)ptr;

            *a1509 = *info->info1509;
        }
        break;
    case 1510:
        if (info->info1510) {
            PSERVER_INFO_1510 a1510;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1510),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1510 = (PSERVER_INFO_1510)ptr;

            *a1510 = *info->info1510;
        }
        break;
    case 1511:
        if (info->info1511) {
            PSERVER_INFO_1511 a1511;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1511),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1511 = (PSERVER_INFO_1511)ptr;

            *a1511 = *info->info1511;
        }
        break;
    case 1512:
        if (info->info1512) {
            PSERVER_INFO_1512 a1512;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1512),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1512 = (PSERVER_INFO_1512)ptr;

            *a1512 = *info->info1512;
        }
        break;
    case 1513:
        if (info->info1513) {
            PSERVER_INFO_1513 a1513;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1513),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1513 = (PSERVER_INFO_1513)ptr;

            *a1513 = *info->info1513;
        }
        break;
    case 1514:
        if (info->info1514) {
            PSERVER_INFO_1514 a1514;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1514),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1514 = (PSERVER_INFO_1514)ptr;

            *a1514 = *info->info1514;
        }
        break;
    case 1515:
        if (info->info1515) {
            PSERVER_INFO_1515 a1515;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1515),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1515 = (PSERVER_INFO_1515)ptr;

            *a1515 = *info->info1515;
        }
        break;
    case 1516:
        if (info->info1516) {
            PSERVER_INFO_1516 a1516;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1516),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1516 = (PSERVER_INFO_1516)ptr;

            *a1516 = *info->info1516;
        }
        break;
    case 1518:
        if (info->info1518) {
            PSERVER_INFO_1518 a1518;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1518),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1518 = (PSERVER_INFO_1518)ptr;

            *a1518 = *info->info1518;
        }
        break;
    case 1520:
        if (info->info1520) {
            PSERVER_INFO_1520 a1520;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1520),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1520 = (PSERVER_INFO_1520)ptr;

            *a1520 = *info->info1520;
        }
        break;
    case 1521:
        if (info->info1521) {
            PSERVER_INFO_1521 a1521;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1521),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1521 = (PSERVER_INFO_1521)ptr;

            *a1521 = *info->info1521;
        }
        break;
    case 1522:
        if (info->info1522) {
            PSERVER_INFO_1522 a1522;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1522),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1522 = (PSERVER_INFO_1522)ptr;

            *a1522 = *info->info1522;
        }
        break;
    case 1523:
        if (info->info1523) {
            PSERVER_INFO_1523 a1523;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1523),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1523 = (PSERVER_INFO_1523)ptr;

            *a1523 = *info->info1523;
        }
        break;
    case 1524:
        if (info->info1524) {
            PSERVER_INFO_1524 a1524;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1524),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1524 = (PSERVER_INFO_1524)ptr;

            *a1524 = *info->info1524;
        }
        break;
    case 1525:
        if (info->info1525) {
            PSERVER_INFO_1525 a1525;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1525),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1525 = (PSERVER_INFO_1525)ptr;

            *a1525 = *info->info1525;
        }
        break;
    case 1528:
        if (info->info1528) {
            PSERVER_INFO_1528 a1528;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1528),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1528 = (PSERVER_INFO_1528)ptr;

            *a1528 = *info->info1528;
        }
        break;
    case 1529:
        if (info->info1529) {
            PSERVER_INFO_1529 a1529;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1529),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1529 = (PSERVER_INFO_1529)ptr;

            *a1529 = *info->info1529;
        }
        break;
    case 1530:
        if (info->info1530) {
            PSERVER_INFO_1530 a1530;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1530),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1530 = (PSERVER_INFO_1530)ptr;

            *a1530 = *info->info1530;
        }
        break;
    case 1533:
        if (info->info1533) {
            PSERVER_INFO_1533 a1533;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1533),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1533 = (PSERVER_INFO_1533)ptr;

            *a1533 = *info->info1533;
        }
        break;
    case 1534:
        if (info->info1534) {
            PSERVER_INFO_1534 a1534;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1534),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1534 = (PSERVER_INFO_1534)ptr;

            *a1534 = *info->info1534;
        }
        break;
    case 1535:
        if (info->info1535) {
            PSERVER_INFO_1535 a1535;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1535),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1535 = (PSERVER_INFO_1535)ptr;

            *a1535 = *info->info1535;
        }
        break;
    case 1536:
        if (info->info1536) {
            PSERVER_INFO_1536 a1536;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1536),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1536 = (PSERVER_INFO_1536)ptr;

            *a1536 = *info->info1536;
        }
        break;
    case 1537:
        if (info->info1537) {
            PSERVER_INFO_1537 a1537;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1537),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1537 = (PSERVER_INFO_1537)ptr;

            *a1537 = *info->info1537;
        }
        break;
    case 1538:
        if (info->info1538) {
            PSERVER_INFO_1538 a1538;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1538),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1538 = (PSERVER_INFO_1538)ptr;

            *a1538 = *info->info1538;
        }
        break;
    case 1539:
        if (info->info1539) {
            PSERVER_INFO_1539 a1539;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1539),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1539 = (PSERVER_INFO_1539)ptr;

            *a1539 = *info->info1539;
        }
        break;
    case 1540:
        if (info->info1540) {
            PSERVER_INFO_1540 a1540;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1540),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1540 = (PSERVER_INFO_1540)ptr;

            *a1540 = *info->info1540;
        }
        break;
    case 1541:
        if (info->info1541) {
            PSERVER_INFO_1541 a1541;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1541),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1541 = (PSERVER_INFO_1541)ptr;

            *a1541 = *info->info1541;
        }
        break;
    case 1542:
        if (info->info1542) {
            PSERVER_INFO_1542 a1542;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1542),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1542 = (PSERVER_INFO_1542)ptr;

            *a1542 = *info->info1542;
        }
        break;
    case 1543:
        if (info->info1543) {
            PSERVER_INFO_1543 a1543;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1543),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1543 = (PSERVER_INFO_1543)ptr;

            *a1543 = *info->info1543;
        }
        break;
    case 1544:
        if (info->info1544) {
            PSERVER_INFO_1544 a1544;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1544),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1544 = (PSERVER_INFO_1544)ptr;

            *a1544 = *info->info1544;
        }
        break;
    case 1545:
        if (info->info1545) {
            PSERVER_INFO_1545 a1545;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1545),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1545 = (PSERVER_INFO_1545)ptr;

            *a1545 = *info->info1545;
        }
        break;
    case 1546:
        if (info->info1546) {
            PSERVER_INFO_1546 a1546;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1546),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1546 = (PSERVER_INFO_1546)ptr;

            *a1546 = *info->info1546;
        }
        break;
    case 1547:
        if (info->info1547) {
            PSERVER_INFO_1547 a1547;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1547),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1547 = (PSERVER_INFO_1547)ptr;

            *a1547 = *info->info1547;
        }
        break;
    case 1548:
        if (info->info1548) {
            PSERVER_INFO_1548 a1548;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1548),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1548 = (PSERVER_INFO_1548)ptr;

            *a1548 = *info->info1548;
        }
        break;
    case 1549:
        if (info->info1549) {
            PSERVER_INFO_1549 a1549;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1549),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1549 = (PSERVER_INFO_1549)ptr;

            *a1549 = *info->info1549;
        }
        break;
    case 1550:
        if (info->info1550) {
            PSERVER_INFO_1550 a1550;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1550),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1550 = (PSERVER_INFO_1550)ptr;

            *a1550 = *info->info1550;
        }
        break;
    case 1552:
        if (info->info1552) {
            PSERVER_INFO_1552 a1552;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1552),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1552 = (PSERVER_INFO_1552)ptr;

            *a1552 = *info->info1552;
        }
        break;
    case 1553:
        if (info->info1553) {
            PSERVER_INFO_1553 a1553;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1553),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1553 = (PSERVER_INFO_1553)ptr;

            *a1553 = *info->info1553;
        }
        break;
    case 1554:
        if (info->info1554) {
            PSERVER_INFO_1554 a1554;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1554),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1554 = (PSERVER_INFO_1554)ptr;

            *a1554 = *info->info1554;
        }
        break;
    case 1555:
        if (info->info1555) {
            PSERVER_INFO_1555 a1555;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1555),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1555 = (PSERVER_INFO_1555)ptr;

            *a1555 = *info->info1555;
        }
        break;
    case 1556:
        if (info->info1556) {
            PSERVER_INFO_1556 a1556;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1556),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1556 = (PSERVER_INFO_1556)ptr;

            *a1556 = *info->info1556;
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
