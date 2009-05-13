/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        moduser.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)

 *        Driver for program to modify an exiting user
 *
 * Authors:
 *
 *        Krishna Ganugapati (krishnag@likewisesoftware.com)
 *        Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "includes.h"

DWORD
MapErrorCode(
    DWORD dwError
    );

DWORD
LsaModUserMain(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    PDLINKEDLIST pTaskList = NULL;
    PSTR pszLoginId = NULL;
    size_t dwErrorBufferSize = 0;
    BOOLEAN bPrintOrigError = TRUE;

     if (geteuid() != 0) {
         fprintf(stderr, "This program requires super-user privileges.\n");
         dwError = EACCES;
         BAIL_ON_LSA_ERROR(dwError);
     }

     dwError = ParseArgs(argc, argv, &pszLoginId, &pTaskList);
     BAIL_ON_LSA_ERROR(dwError);

     dwError = ModifyUser(
                     pszLoginId,
                     pTaskList);
     BAIL_ON_LSA_ERROR(dwError);

 cleanup:

     if (pTaskList) {
         LsaDLinkedListForEach(pTaskList, &FreeTasksInList, NULL);
         LsaDLinkedListFree(pTaskList);
     }

     LSA_SAFE_FREE_STRING(pszLoginId);

     return dwError;

 error:

     dwError = MapErrorCode(dwError);

     dwErrorBufferSize = LsaGetErrorString(dwError, NULL, 0);

     if (dwErrorBufferSize > 0)
     {
         DWORD dwError2 = 0;
         PSTR   pszErrorBuffer = NULL;

         dwError2 = LsaAllocateMemory(
                     dwErrorBufferSize,
                     (PVOID*)&pszErrorBuffer);

         if (!dwError2)
         {
             DWORD dwLen = LsaGetErrorString(dwError, pszErrorBuffer, dwErrorBufferSize);

             if ((dwLen == dwErrorBufferSize) && !IsNullOrEmptyString(pszErrorBuffer))
             {
                 fprintf(stderr, "Failed to modify user.  %s\n", pszErrorBuffer);
                 bPrintOrigError = FALSE;
             }
         }

         LSA_SAFE_FREE_STRING(pszErrorBuffer);
     }

     if (bPrintOrigError)
     {
         fprintf(stderr, "Failed to modify user. Error code [%d]\n", dwError);
     }

     goto cleanup;
}

DWORD
ParseArgs(
    int   argc,
    char* argv[],
    PSTR* ppszLoginId,
    PDLINKEDLIST* ppTaskList
    )
{
    typedef enum {
        PARSE_MODE_OPEN = 0,
        PARSE_MODE_ADD_TO_GROUPS,
        PARSE_MODE_REMOVE_FROM_GROUPS,
        PARSE_MODE_SET_ACCOUNT_EXPIRY,
        PARSE_MODE_DONE
    } ParseMode;

    DWORD dwError = 0;
    ParseMode parseMode = PARSE_MODE_OPEN;
    int iArg = 1;
    PSTR pArg = NULL;
    PDLINKEDLIST pTaskList = NULL;
    PUSER_MOD_TASK pTask = NULL;
    PSTR    pszLoginId = NULL;

    do {
        pArg = argv[iArg++];
        if (pArg == NULL || *pArg == '\0') {
          break;
        }

        switch(parseMode) {
            case PARSE_MODE_OPEN:
            {
                if (!strcmp(pArg, "--enable-user"))
                {
                   dwError = LsaAllocateMemory(sizeof(USER_MOD_TASK), (PVOID*)&pTask);
                   BAIL_ON_LSA_ERROR(dwError);
                   pTask->taskType = UserModTask_EnableUser;

                   dwError = LsaDLinkedListAppend(&pTaskList, pTask);
                   BAIL_ON_LSA_ERROR(dwError);

                   pTask = NULL;
                }
                else if (!strcmp(pArg, "--disable-user"))
                {

                    dwError = LsaAllocateMemory(sizeof(USER_MOD_TASK), (PVOID*)&pTask);
                    BAIL_ON_LSA_ERROR(dwError);
                    pTask->taskType = UserModTask_DisableUser;

                    dwError = LsaDLinkedListAppend(&pTaskList, pTask);
                    BAIL_ON_LSA_ERROR(dwError);

                    pTask = NULL;
                }
                else if ((strcmp(pArg, "--help") == 0) ||
                         (strcmp(pArg, "-h") == 0))
                {
                  ShowUsage(GetProgramName(argv[0]));
                  exit(0);
                }
                else if (!strcmp(pArg, "--change-password-at-next-logon"))
                {

                    dwError = LsaAllocateMemory(sizeof(USER_MOD_TASK), (PVOID*)&pTask);
                    BAIL_ON_LSA_ERROR(dwError);
                    pTask->taskType = UserModTask_ChangePasswordAtNextLogon;

                    dwError = LsaDLinkedListAppend(&pTaskList, pTask);
                    BAIL_ON_LSA_ERROR(dwError);

                    pTask = NULL;
                }
                else if (!strcmp(pArg, "--unlock")) {

                    dwError = LsaAllocateMemory(sizeof(USER_MOD_TASK), (PVOID*)&pTask);
                    BAIL_ON_LSA_ERROR(dwError);
                    pTask->taskType = UserModTask_UnlockUser;

                    dwError = LsaDLinkedListAppend(&pTaskList, pTask);
                    BAIL_ON_LSA_ERROR(dwError);

                    pTask = NULL;
                }
                else if (!strcmp(pArg, "--password-never-expires"))
                {

                    dwError = LsaAllocateMemory(sizeof(USER_MOD_TASK), (PVOID*)&pTask);
                    BAIL_ON_LSA_ERROR(dwError);
                    pTask->taskType = UserModTask_SetPasswordNeverExpires;

                    dwError = LsaDLinkedListAppend(&pTaskList, pTask);
                    BAIL_ON_LSA_ERROR(dwError);

                    pTask = NULL;
                }
                else if (!strcmp(pArg, "--password-must-expire"))
                {

                    dwError = LsaAllocateMemory(sizeof(USER_MOD_TASK), (PVOID*)&pTask);
                    BAIL_ON_LSA_ERROR(dwError);
                    pTask->taskType = UserModTask_SetPasswordMustExpire;

                    dwError = LsaDLinkedListAppend(&pTaskList, pTask);
                    BAIL_ON_LSA_ERROR(dwError);

                    pTask = NULL;
                }
                else if (!strcmp(pArg, "--add-to-groups"))
                {
                    parseMode = PARSE_MODE_ADD_TO_GROUPS;
                }
                else if (!strcmp(pArg, "--remove-from-groups"))
                {
                    parseMode = PARSE_MODE_REMOVE_FROM_GROUPS;
                }
                else if (!strcmp(pArg, "--set-account-expiry"))
                {
                    parseMode = PARSE_MODE_SET_ACCOUNT_EXPIRY;
                }
                else
                {
                    dwError = LsaAllocateString(pArg, &pszLoginId);
                    BAIL_ON_LSA_ERROR(dwError);
                    parseMode = PARSE_MODE_DONE;
                }
                break;
            }

            case PARSE_MODE_SET_ACCOUNT_EXPIRY:
            {
                  dwError = LsaAllocateMemory(sizeof(USER_MOD_TASK), (PVOID*)&pTask);
                  BAIL_ON_LSA_ERROR(dwError);

                  pTask->taskType = UserModTask_SetAccountExpiryDate;

                  dwError = LsaAllocateString(pArg, &pTask->pszData);
                  BAIL_ON_LSA_ERROR(dwError);

                  dwError = LsaDLinkedListAppend(&pTaskList, pTask);
                  BAIL_ON_LSA_ERROR(dwError);

                  pTask = NULL;

                  parseMode = PARSE_MODE_OPEN;

                  break;
            }

            case PARSE_MODE_REMOVE_FROM_GROUPS:
            {
                 dwError = LsaAllocateMemory(sizeof(USER_MOD_TASK), (PVOID*)&pTask);
                 BAIL_ON_LSA_ERROR(dwError);

                 pTask->taskType = UserModTask_RemoveFromGroups;

                 dwError = LsaAllocateString(pArg, &pTask->pszData);
                 BAIL_ON_LSA_ERROR(dwError);

                 dwError = LsaDLinkedListAppend(&pTaskList, pTask);
                 BAIL_ON_LSA_ERROR(dwError);

                 parseMode = PARSE_MODE_OPEN;

                 break;
            }

            case PARSE_MODE_ADD_TO_GROUPS:
            {
                  dwError = LsaAllocateMemory(sizeof(USER_MOD_TASK), (PVOID*)&pTask);
                  BAIL_ON_LSA_ERROR(dwError);

                  pTask->taskType = UserModTask_AddToGroups;

                  dwError = LsaAllocateString(pArg, &pTask->pszData);
                  BAIL_ON_LSA_ERROR(dwError);

                  dwError = LsaDLinkedListAppend(&pTaskList, pTask);
                  BAIL_ON_LSA_ERROR(dwError);

                  parseMode = PARSE_MODE_OPEN;

                 break;
            }

            case PARSE_MODE_DONE:
            {

                ShowUsage(GetProgramName(argv[0]));
                exit(1);
            }
        }

    } while (iArg < argc);

    if (parseMode != PARSE_MODE_OPEN && parseMode != PARSE_MODE_DONE)
    {
        ShowUsage(GetProgramName(argv[0]));
        exit(1);
    }

    if (!ValidateArgs(pszLoginId, pTaskList)) {
       dwError = LSA_ERROR_INVALID_PARAMETER;
       BAIL_ON_LSA_ERROR(dwError);
    }

    *ppszLoginId = pszLoginId;
    *ppTaskList = pTaskList;

cleanup:

    return dwError;

error:

    *ppTaskList = NULL;

    if (pTaskList) {
        LsaDLinkedListForEach(pTaskList, FreeTasksInList, NULL);
        LsaDLinkedListFree(pTaskList);
    }

    if (pTask) {
        FreeTask(pTask);
    }

    LSA_SAFE_FREE_STRING(pszLoginId);

    ShowUsage(GetProgramName(argv[0]));

    goto cleanup;
}

BOOLEAN
ValidateArgs(
    PCSTR        pszLoginId,
    PDLINKEDLIST pTaskList
    )
{
    BOOLEAN bValid = FALSE;

    PDLINKEDLIST pListMember = NULL;
    BOOLEAN bEnableUser = FALSE;
    BOOLEAN bDisableUser = FALSE;
    BOOLEAN bSetChangePasswordAtNextLogon = FALSE;
    BOOLEAN bSetPasswordNeverExpires = FALSE;

    for (pListMember = pTaskList; pListMember; pListMember = pListMember->pNext)
    {
        PUSER_MOD_TASK pTask = (PUSER_MOD_TASK)pListMember->pItem;
        if (pTask) {
           switch(pTask->taskType)
           {
               case UserModTask_EnableUser:
               {
                   bEnableUser = TRUE;
                   break;
               }
               case UserModTask_DisableUser:
               {
                   bDisableUser = TRUE;
                   break;
               }
               case UserModTask_SetPasswordNeverExpires:
               {
                   bSetPasswordNeverExpires = TRUE;
                   break;
               }
               case UserModTask_ChangePasswordAtNextLogon:
               {
                   bSetChangePasswordAtNextLogon = TRUE;
                   break;
               }
               default:
                   break;
           }
        }
    }

    if (bEnableUser && bDisableUser)
    {
        fprintf(stderr, "Error: Both --enable-user and --disable-user cannot be specified.\n");
        goto cleanup;
    }

    if (bSetPasswordNeverExpires && bSetChangePasswordAtNextLogon)
    {
        fprintf(stderr, "Error: The options --password-never-expires and\n");
        fprintf(stderr, "       --change-password-at-next-logon cannot be specified together.\n");
        goto cleanup;
    }

    if (IsNullOrEmptyString(pszLoginId)) {
        fprintf(stderr, "Error: A valid user id or user login id must be specified.\n");
        goto cleanup;
    }

    bValid = TRUE;

cleanup:

    return bValid;
}

VOID
FreeTasksInList(
    PVOID pTask,
    PVOID pUserData
    )
{
    if (pTask) {
       FreeTask((PUSER_MOD_TASK)pTask);
    }
}

VOID
FreeTask(
    PUSER_MOD_TASK pTask
    )
{
    LSA_SAFE_FREE_STRING(pTask->pszData);
    LsaFreeMemory(pTask);
}

PSTR
GetProgramName(
    PSTR pszFullProgramPath
    )
{
    if (pszFullProgramPath == NULL || *pszFullProgramPath == '\0') {
        return NULL;
    }

    // start from end of the string
    PSTR pszNameStart = pszFullProgramPath + strlen(pszFullProgramPath);
    do {
        if (*(pszNameStart - 1) == '/') {
            break;
        }

        pszNameStart--;

    } while (pszNameStart != pszFullProgramPath);

    return pszNameStart;
}

VOID
ShowUsage(
    PCSTR pszProgramName
    )
{
    fprintf(stdout, "Usage: %s {modification options} ( user login id | uid )\n\n", pszProgramName);

    fprintf(stdout, "\nModification options:\n");
    fprintf(stdout, "{ --help }\n");
    fprintf(stdout, "{ --disable-user | --enable-user }\n");
    fprintf(stdout, "{ --unlock }\n");
    fprintf(stdout, "{ --set-account-expiry expiry-date (YYYY-MM-DD format) }\n");
    fprintf(stdout, "{ --change-password-at-next-logon }\n");
    fprintf(stdout, "{ --password-never-expires }\n");
    fprintf(stdout, "{ --password-must-expire }\n");
    fprintf(stdout, "{ --add-to-groups nt4-style-group-name }\n");
    fprintf(stdout, "{ --remove-from-groups nt4-style-group-name }\n");

    fprintf(stdout, "\nNotes:\n");
    fprintf(stdout, "a) Set the expiry-date to 0 for an account that must never expire.\n");
    fprintf(stdout, "b) If both --remove-from-group and --add-to-group are specified,\n");
    fprintf(stdout, "   the user is removed from the specified group first.\n");
    fprintf(stdout, "c) The options ""--change-password-at-next-logon"" and \n");
    fprintf(stdout, "   ""--password-never-expires"" cannot be set simultaneously.\n");

}

DWORD
ModifyUser(
    PSTR pszLoginId,
    PDLINKEDLIST pTaskList
    )
{
    DWORD dwError = 0;
    PLSA_USER_MOD_INFO pUserModInfo = NULL;
    uid_t uid = 0;
    int   nRead = 0;
    PVOID pUserInfo = NULL;
    DWORD dwUserInfoLevel = 0;
    HANDLE hLsaConnection = (HANDLE)NULL;

    BAIL_ON_INVALID_STRING(pszLoginId);

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LSA_ERROR(dwError);

    nRead = sscanf(pszLoginId, "%u", (unsigned int*)&uid);
    if ((nRead == EOF) || (nRead == 0)) {

       dwError = LsaFindUserByName(
                       hLsaConnection,
                       pszLoginId,
                       dwUserInfoLevel,
                       &pUserInfo);
       BAIL_ON_LSA_ERROR(dwError);

       uid = ((PLSA_USER_INFO_0)pUserInfo)->uid;

       LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
       pUserInfo = NULL;
    }

    dwError = BuildUserModInfo(
                    uid,
                    pTaskList,
                    &pUserModInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaModifyUser(
                    hLsaConnection,
                    pUserModInfo);
    BAIL_ON_LSA_ERROR(dwError);

    fprintf(stdout, "Successfully modified user %s\n", pszLoginId);

cleanup:

    if (pUserModInfo) {
        LsaFreeUserModInfo(pUserModInfo);
    }

    if (pUserInfo) {
       LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }

    if (hLsaConnection != (HANDLE)NULL) {
       LsaCloseServer(hLsaConnection);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
BuildUserModInfo(
    uid_t        uid,
    PDLINKEDLIST pTaskList,
    PLSA_USER_MOD_INFO* ppUserModInfo
    )
{
    DWORD dwError = 0;
    PDLINKEDLIST pListMember = pTaskList;
    PLSA_USER_MOD_INFO pUserModInfo = NULL;

    dwError = LsaBuildUserModInfo(uid, &pUserModInfo);
    BAIL_ON_LSA_ERROR(dwError);

    for (; pListMember; pListMember = pListMember->pNext)
    {
        PUSER_MOD_TASK pTask = (PUSER_MOD_TASK)pListMember->pItem;
        switch(pTask->taskType)
        {
            case UserModTask_EnableUser:
            {
                 dwError = LsaModifyUser_EnableUser(pUserModInfo, TRUE);
                 BAIL_ON_LSA_ERROR(dwError);

                 break;
            }
            case UserModTask_DisableUser:
            {
                 dwError = LsaModifyUser_DisableUser(pUserModInfo, TRUE);
                 BAIL_ON_LSA_ERROR(dwError);

                 break;
            }
            case UserModTask_UnlockUser:
            {
                 dwError = LsaModifyUser_Unlock(pUserModInfo, TRUE);
                 BAIL_ON_LSA_ERROR(dwError);

                 break;
            }
            case UserModTask_ChangePasswordAtNextLogon:
            {
                 dwError = LsaModifyUser_ChangePasswordAtNextLogon(pUserModInfo, TRUE);
                 BAIL_ON_LSA_ERROR(dwError);

                 break;
            }
            case UserModTask_SetAccountExpiryDate:
            {
                 dwError = LsaModifyUser_SetAccountExpiryDate(pUserModInfo, pTask->pszData);
                 BAIL_ON_LSA_ERROR(dwError);

                 break;
            }
            case UserModTask_AddToGroups:
            {
                 dwError = LsaModifyUser_AddToGroups(pUserModInfo, pTask->pszData);
                 BAIL_ON_LSA_ERROR(dwError);

                 break;
            }
            case UserModTask_RemoveFromGroups:
            {
                 dwError = LsaModifyUser_RemoveFromGroups(pUserModInfo, pTask->pszData);
                 BAIL_ON_LSA_ERROR(dwError);

                 break;
            }
            case UserModTask_SetPasswordNeverExpires:
            {
                dwError = LsaModifyUser_SetPasswordNeverExpires(pUserModInfo, TRUE);
                BAIL_ON_LSA_ERROR(dwError);

                break;
            }
            case UserModTask_SetPasswordMustExpire:
            {
                dwError = LsaModifyUser_SetPasswordMustExpire(pUserModInfo, TRUE);
                BAIL_ON_LSA_ERROR(dwError);

                break;
            }
        }
    }

    *ppUserModInfo = pUserModInfo;

cleanup:

    return dwError;

error:

    *ppUserModInfo = NULL;

    if (pUserModInfo) {
       LsaFreeUserModInfo(pUserModInfo);
    }

    goto cleanup;
}

DWORD
MapErrorCode(
    DWORD dwError
    )
{
    DWORD dwError2 = dwError;

    switch (dwError)
    {
        case ECONNREFUSED:
        case ENETUNREACH:
        case ETIMEDOUT:

            dwError2 = LSA_ERROR_LSA_SERVER_UNREACHABLE;

            break;

        default:

            break;
    }

    return dwError2;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
