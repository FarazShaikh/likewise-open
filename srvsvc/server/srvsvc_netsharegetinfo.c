/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software
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
 *        srvsvc_netsharegetinfo.c
 *
 * Abstract:
 *
 *        Likewise Server Service (srvsvc) RPC client and server
 *
 *        NetShareGetInfo server API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"

/* Make memory allocation easier */
typedef union __srvsvc_NetShareInfo
{
    SHARE_INFO_0 info0;
    SHARE_INFO_1 info1;
    SHARE_INFO_2 info2;
    SHARE_INFO_501 info501;
    SHARE_INFO_502 info502;
    SHARE_INFO_1005 info1005;

} SHARE_INFO, *PSHARE_INFO;


NET_API_STATUS
SrvSvcNetShareGetInfo(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ wchar16_t *netname,
    /* [in] */ UINT32 level,
    /* [out,ref] */ srvsvc_NetShareInfo *info
    )
{
    NTSTATUS ntStatus = 0;
    DWORD dwError = 0;
    PBYTE pInBuffer = NULL;
    DWORD dwInLength = 0;
    PBYTE pOutBuffer = NULL;
    DWORD dwOutLength = 4096;
    IO_FILE_HANDLE hFile = NULL;
    IO_STATUS_BLOCK IoStatusBlock = {0};
    ACCESS_MASK DesiredAccess = 0;
    LONG64 AllocationSize = 0;
    FILE_ATTRIBUTES FileAttributes = 0;
    FILE_SHARE_FLAGS ShareAccess = 0;
    FILE_CREATE_DISPOSITION CreateDisposition = 0;
    FILE_CREATE_OPTIONS CreateOptions = 0;
    ULONG IoControlCode = SRV_DEVCTL_GET_SHARE_INFO;
    WCHAR wszDriverName[] = SRV_DRIVER_NAME_W;
    IO_FILE_NAME filename =
                        {
                              .RootFileHandle = NULL,
                              .Name = RTL_CONSTANT_STRING(wszDriverName),
                              .IoNameOptions = 0
                        };
    SHARE_INFO_GETINFO_PARAMS GetParamsIn = {0};
    PSHARE_INFO_GETINFO_PARAMS pGetParamsOut = NULL;
    PSHARE_INFO pShareInfo = NULL;

    /* Validate info level */

    switch (level){
    case 0:
    case 1:
    case 2:
    case 501:
    case 502:
    case 1005:
        break;

    default:
        ntStatus = STATUS_INVALID_LEVEL;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    memset(&GetParamsIn, 0, sizeof(GetParamsIn));
    memset(info, 0x0, sizeof(*info));

    GetParamsIn.pwszNetname = netname;
    GetParamsIn.dwInfoLevel = level;

    ntStatus = LwShareInfoMarshalGetParameters(
                        &GetParamsIn,
                        &pInBuffer,
                        &dwInLength
                        );
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NtCreateFile(
                        &hFile,
                        NULL,
                        &IoStatusBlock,
                        &filename,
                        NULL,
                        NULL,
                        DesiredAccess,
                        AllocationSize,
                        FileAttributes,
                        ShareAccess,
                        CreateDisposition,
                        CreateOptions,
                        NULL,
                        0,
                        NULL,
                        NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    dwError = LwAllocateMemory(
                    dwOutLength,
                    (void**)&pOutBuffer
                    );
    BAIL_ON_SRVSVC_ERROR(dwError);

    ntStatus = NtDeviceIoControlFile(
                    hFile,
                    NULL,
                    &IoStatusBlock,
                    IoControlCode,
                    pInBuffer,
                    dwInLength,
                    pOutBuffer,
                    dwOutLength
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    while (ntStatus == STATUS_BUFFER_TOO_SMALL) {
        /* We need more space in output buffer to make this call */

        LW_SAFE_FREE_MEMORY(pOutBuffer);
        dwOutLength *= 2;

        dwError = LwAllocateMemory(
                        dwOutLength,
                        (void**)&pOutBuffer
                        );
        BAIL_ON_SRVSVC_ERROR(dwError);

        ntStatus = NtDeviceIoControlFile(
                        hFile,
                        NULL,
                        &IoStatusBlock,
                        IoControlCode,
                        pInBuffer,
                        dwInLength,
                        pOutBuffer,
                        dwOutLength
                        );
    }

    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwShareInfoUnmarshalGetParameters(
                        pOutBuffer,
                        IoStatusBlock.BytesTransferred,
                        &pGetParamsOut
                        );
    BAIL_ON_NT_STATUS(ntStatus);

    dwError = SrvSvcSrvAllocateMemory(
                  sizeof(*pShareInfo),
                  (PVOID*)&pShareInfo);
    BAIL_ON_SRVSVC_ERROR(dwError);

    switch (pGetParamsOut->dwInfoLevel) {
    case 0:
        info->info0   = &pShareInfo->info0;

        dwError = SrvSvcSrvCopyShareInfo0(
                      info->info0,
                      pGetParamsOut->Info.p0);
        BAIL_ON_SRVSVC_ERROR(dwError);

        break;

    case 1:
        info->info1   = &pShareInfo->info1;

        dwError = SrvSvcSrvCopyShareInfo1(
                      info->info1,
                      pGetParamsOut->Info.p1);
        BAIL_ON_SRVSVC_ERROR(dwError);

        break;

    case 2:
        info->info2   = &pShareInfo->info2;

        dwError = SrvSvcSrvCopyShareInfo2(
                      info->info2,
                      pGetParamsOut->Info.p2);
        BAIL_ON_SRVSVC_ERROR(dwError);

        break;

    case 501:
        info->info501 = &pShareInfo->info501;

        dwError = SrvSvcSrvCopyShareInfo501(
                      info->info501,
                      pGetParamsOut->Info.p501);
        BAIL_ON_SRVSVC_ERROR(dwError);

        break;

    case 502:
        info->info502 = (PSHARE_INFO_502_I) &pShareInfo->info502;

        dwError = SrvSvcSrvCopyShareInfo502(
                      (PSHARE_INFO_502)info->info502,
                      pGetParamsOut->Info.p502);
        BAIL_ON_SRVSVC_ERROR(dwError);

        break;

    case 1005:
        info->info1005 = &pShareInfo->info1005;

        dwError = SrvSvcSrvCopyShareInfo1005(
                      info->info1005,
                      pGetParamsOut->Info.p1005);
        BAIL_ON_SRVSVC_ERROR(dwError);

        break;

    default:
        ntStatus = STATUS_INVALID_INFO_CLASS;
        BAIL_ON_NT_STATUS(ntStatus);

        break;
    }

cleanup:
    if (hFile)
    {
        NtCloseFile(hFile);
    }

    LW_SAFE_FREE_MEMORY(pInBuffer);
    LW_SAFE_FREE_MEMORY(pOutBuffer);

    // SrvSvcFreeShareInfoOutParams(pGetParamsOut->dwInfoLevel, &pGetParamsOut);

    return dwError;

error:

    memset(info, 0x0, sizeof(*info));

    if (pShareInfo)
    {
        SrvSvcSrvFreeMemory(pShareInfo);
    }

    if (ntStatus != STATUS_SUCCESS)
    {
        dwError = LwNtStatusToWin32Error(ntStatus);
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
