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
 *        users.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        User Lookup and Management (Server)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "api.h"

DWORD
LsaSrvFindUserByName(
    HANDLE hServer,
    PCSTR  pszLoginId,
    DWORD  dwUserInfoLevel,
    PVOID* ppUserInfo
    )
{
    DWORD dwError = 0;
    PSTR pszDomain = NULL;
    PSTR pszUserName = NULL;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    BOOLEAN bInLock = FALSE;
    HANDLE hProvider = (HANDLE)NULL;
    
    dwError = LsaValidateUserName(pszLoginId);
    BAIL_ON_LSA_ERROR(dwError);
    
    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);
    
    for (pProvider = gpAuthProviderList;
         pProvider;
         pProvider = pProvider->pNext)
    {
        dwError = LsaSrvOpenProvider(hServer, pProvider, &hProvider);
        BAIL_ON_LSA_ERROR(dwError);       
        
        dwError = pProvider->pFnTable->pfnLookupUserByName(
                                        hProvider,
                                        pszLoginId,
                                        dwUserInfoLevel,
                                        ppUserInfo);
        if (!dwError) {
            
            break;
            
        } else if ((dwError == LSA_ERROR_NOT_HANDLED) ||
                   (dwError == LSA_ERROR_NO_SUCH_USER)) {
            
            LsaSrvCloseProvider(pProvider, hProvider);
            hProvider = (HANDLE)NULL;
            
            continue;
            
        } else {
            
            BAIL_ON_LSA_ERROR(dwError);
            
        }
    }
    
    if (pProvider == NULL)
    {
       dwError = LSA_ERROR_NO_SUCH_USER;
    }
    BAIL_ON_LSA_ERROR(dwError);
    
cleanup:
   
    LSA_SAFE_FREE_STRING(pszDomain);
    LSA_SAFE_FREE_STRING(pszUserName);
    
    if (hProvider != (HANDLE)NULL) {
        LsaSrvCloseProvider(pProvider, hProvider);
    }
    
    LEAVE_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);
    
    if (!dwError)
    {
        LsaSrvIncrementMetricValue(LsaMetricSuccessfulUserLookupsByName);
    }
    else
    {
        LsaSrvIncrementMetricValue(LsaMetricFailedUserLookupsByName);
    }
    
    return(dwError);

error:

    if (dwError == LSA_ERROR_NOT_HANDLED ||
                   dwError == LSA_ERROR_NO_SUCH_USER) {
        LSA_LOG_VERBOSE("Find user by name: [%s] is unknown", IsNullOrEmptyString(pszLoginId) ? "" : pszLoginId);
    }
    else {
        LSA_LOG_ERROR("Failed to find user by name [%s] [code %d]", IsNullOrEmptyString(pszLoginId) ? "" : pszLoginId, dwError);
    }

    *ppUserInfo = NULL;
    
    goto cleanup;
}

DWORD
LsaSrvFindUserById(
    HANDLE hServer,
    uid_t  uid,
    DWORD  dwUserInfoLevel,
    PVOID* ppUserInfo
    )
{
    DWORD dwError = 0;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    BOOLEAN bInLock = FALSE;
    HANDLE hProvider = (HANDLE)NULL;
    
    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);
    
    for (pProvider = gpAuthProviderList;
         pProvider;
         pProvider = pProvider->pNext)
    {
        dwError = LsaSrvOpenProvider(hServer, pProvider, &hProvider);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = pProvider->pFnTable->pfnLookupUserById(
                                        hProvider,
                                        uid,
                                        dwUserInfoLevel,
                                        ppUserInfo);
        if (!dwError) {
            
            break;
            
        } else if ((dwError == LSA_ERROR_NOT_HANDLED) ||
                   (dwError == LSA_ERROR_NO_SUCH_USER)) {
            
            LsaSrvCloseProvider(pProvider, hProvider);
            hProvider = (HANDLE)NULL;
            
            continue;
            
        } else {
            
            BAIL_ON_LSA_ERROR(dwError);
            
        }
    }

    if (pProvider == NULL)
    {
        dwError = LSA_ERROR_NO_SUCH_USER;
    }
    BAIL_ON_LSA_ERROR(dwError);
    
cleanup:
    
    if (hProvider != (HANDLE)NULL) {
        LsaSrvCloseProvider(pProvider, hProvider);
    }
    
    LEAVE_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);
    
    if (!dwError)
    {
        LsaSrvIncrementMetricValue(LsaMetricSuccessfulUserLookupsById);
    }
    else
    {
        LsaSrvIncrementMetricValue(LsaMetricFailedUserLookupsById);
    }
    
    return(dwError);

error:

    if (dwError == LSA_ERROR_NOT_HANDLED ||
                   dwError == LSA_ERROR_NO_SUCH_USER) {
        LSA_LOG_VERBOSE("Find user by id: [%ld] is unknown", (long)uid);
    }
    else {
        LSA_LOG_ERROR("Failed to find user by id [%ld] [code %d]", (long)uid, dwError);
    }

    *ppUserInfo = NULL;
    
    goto cleanup;
}

DWORD
LsaSrvAddUser(
    HANDLE hServer,
    DWORD  dwUserInfoLevel,
    PVOID  pUserInfo
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PLSA_SRV_API_STATE pServerState = (PLSA_SRV_API_STATE)hServer;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    HANDLE hProvider = (HANDLE)NULL;
    
    if (pServerState->peerUID)
    {
        dwError = EACCES;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    if (dwUserInfoLevel != 0) {
        dwError = LSA_ERROR_UNSUPPORTED_USER_LEVEL;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwError = LsaValidateUserInfo(
                    pUserInfo,
                    dwUserInfoLevel);
    BAIL_ON_LSA_ERROR(dwError);
    
    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);
    
    dwError = LSA_ERROR_NOT_HANDLED;
    
    for (pProvider = gpAuthProviderList;
         pProvider;
         pProvider = pProvider->pNext)
    {
        dwError = LsaSrvOpenProvider(hServer, pProvider, &hProvider);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = pProvider->pFnTable->pfnAddUser(
                                        hProvider,
                                        dwUserInfoLevel,
                                        pUserInfo);
        if (!dwError) {
            
            break;
            
        }
        else if (dwError == LSA_ERROR_NOT_HANDLED) {
            
            LsaSrvCloseProvider(pProvider, hProvider);
            hProvider = (HANDLE)NULL;
            
            continue;
            
        } else {
            
            BAIL_ON_LSA_ERROR(dwError);
            
        }
    }
    
cleanup:
    
    if (hProvider != (HANDLE)NULL) {
        LsaSrvCloseProvider(pProvider, hProvider);
    }
    
    LEAVE_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);
    
    return(dwError);

error:
    
    goto cleanup;
}

DWORD
LsaSrvModifyUser(
    HANDLE hServer,
    PLSA_USER_MOD_INFO pUserModInfo
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PLSA_SRV_API_STATE pServerState = (PLSA_SRV_API_STATE)hServer;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    HANDLE hProvider = (HANDLE)NULL;
    
    if (pServerState->peerUID)
    {
        dwError = EACCES;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);
    
    dwError = LSA_ERROR_NOT_HANDLED;
    
    for (pProvider = gpAuthProviderList;
         pProvider;
         pProvider = pProvider->pNext)
    {
        dwError = LsaSrvOpenProvider(hServer, pProvider, &hProvider);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = pProvider->pFnTable->pfnModifyUser(
                                        hProvider,
                                        pUserModInfo);
        if (!dwError) {
            
            break;
            
        }
        else if (dwError == LSA_ERROR_NOT_HANDLED) {
            
            LsaSrvCloseProvider(pProvider, hProvider);
            hProvider = (HANDLE)NULL;
            
            continue;
            
        } else {
            
            BAIL_ON_LSA_ERROR(dwError);
            
        }
    }
    
cleanup:
    
    if (hProvider != (HANDLE)NULL) {
        LsaSrvCloseProvider(pProvider, hProvider);
    }
    
    LEAVE_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);
    
    return(dwError);

error:
    
    goto cleanup;
}

DWORD
LsaSrvDeleteUser(
    HANDLE hServer,
    uid_t  uid
    )
{
    DWORD dwError = 0;
    PLSA_SRV_API_STATE pServerState = (PLSA_SRV_API_STATE)hServer;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    BOOLEAN bInLock = FALSE;
    HANDLE hProvider = (HANDLE)NULL;
    
    if (pServerState->peerUID)
    {
        dwError = EACCES;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);
    
    dwError = LSA_ERROR_NOT_HANDLED;
    
    for (pProvider = gpAuthProviderList;
         pProvider;
         pProvider = pProvider->pNext)
    {
        dwError = LsaSrvOpenProvider(hServer, pProvider, &hProvider);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = pProvider->pFnTable->pfnDeleteUser(
                                        hProvider,
                                        uid);
        if (!dwError) {
            break;
        } if ((dwError == LSA_ERROR_NOT_HANDLED) ||
              (dwError == LSA_ERROR_NO_SUCH_USER)) {
            LsaSrvCloseProvider(pProvider, hProvider);
            hProvider = (HANDLE)NULL;
            
            continue;
        } else {
            BAIL_ON_LSA_ERROR(dwError);
        }
    }
    
cleanup:

    if (hProvider != (HANDLE)NULL) {
        LsaSrvCloseProvider(pProvider, hProvider);
    }

    LEAVE_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);
    
    return(dwError);

error:
    
    goto cleanup;
}

DWORD
LsaSrvBeginEnumUsers(
    HANDLE hServer,
    DWORD  dwUserInfoLevel,
    DWORD  dwMaxNumUsers,
    PSTR*  ppszGUID
    )
{
    DWORD dwError = 0;
    PLSA_SRV_RECORD_ENUM_STATE pEnumState = NULL;
    PSTR pszGUID = NULL;
    
    dwError = LsaSrvAddUserEnumState(
                    hServer,
                    dwUserInfoLevel,
                    dwMaxNumUsers,
                    &pEnumState);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaAllocateString(pEnumState->pszGUID, &pszGUID);
    BAIL_ON_LSA_ERROR(dwError);
    
    *ppszGUID = pszGUID;
    
cleanup:

    return dwError;
    
error:

    *ppszGUID = NULL;

    goto cleanup;
}

DWORD
LsaSrvEnumUsers(
    HANDLE  hServer,
    PCSTR   pszGUID,
    PDWORD  pdwUserInfoLevel,
    PVOID** pppUserInfoList,
    PDWORD  pdwNumUsersFound
    )
{
    DWORD dwError = 0;
    PLSA_SRV_RECORD_ENUM_STATE pEnumState = NULL;
    PVOID* ppUserInfoList_accumulate = NULL;
    DWORD  dwTotalNumUsersFound = 0;
    PVOID* ppUserInfoList = NULL;
    DWORD  dwNumUsersFound = 0;
    DWORD  dwNumUsersRemaining = 0;
    DWORD  dwUserInfoLevel = 0;
    
    pEnumState = LsaSrvFindUserEnumState(hServer, pszGUID);
    if (!pEnumState) {
        dwError = LSA_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwUserInfoLevel = pEnumState->dwInfoLevel;
    dwNumUsersRemaining = pEnumState->dwNumMaxRecords;
    
    while (dwNumUsersRemaining &&
           pEnumState->pCurProviderState)
    {
        PLSA_SRV_PROVIDER_STATE pProviderState = pEnumState->pCurProviderState;
        PLSA_AUTH_PROVIDER pProvider = pProviderState->pProvider;
        HANDLE hProvider = pProviderState->hProvider;
        HANDLE hResume = pProviderState->hResume;

        dwNumUsersFound = 0;
        
        dwError = pProvider->pFnTable->pfnEnumUsers(
                        hProvider,
                        hResume,
                        dwNumUsersRemaining,
                        &dwNumUsersFound,
                        &ppUserInfoList);
        
        if (dwError) {
           if (dwError != LSA_ERROR_NO_MORE_USERS) {
              BAIL_ON_LSA_ERROR(dwError);
           }
        }
        
        dwNumUsersRemaining -= dwNumUsersFound;
        
        if (dwNumUsersRemaining) {
           pEnumState->pCurProviderState = pEnumState->pCurProviderState->pNext;
           if (dwError == LSA_ERROR_NO_MORE_USERS){
               dwError = 0;
            }
        }
                
        dwError = LsaCoalesceUserInfoList(
                        &ppUserInfoList,
                        &dwNumUsersFound,
                        &ppUserInfoList_accumulate,
                        &dwTotalNumUsersFound);
        BAIL_ON_LSA_ERROR(dwError);
    }
   
    *pdwUserInfoLevel = dwUserInfoLevel;
    *pppUserInfoList = ppUserInfoList_accumulate;
    *pdwNumUsersFound = dwTotalNumUsersFound;
    
cleanup:
    
    return(dwError);

error:
    
    *pdwUserInfoLevel = 0;
    *pppUserInfoList = NULL;
    *pdwNumUsersFound = 0;
    
    if (ppUserInfoList) {
        LsaFreeUserInfoList(dwUserInfoLevel, ppUserInfoList, dwNumUsersFound);
    }
    
    if (ppUserInfoList_accumulate) {
        LsaFreeUserInfoList(dwUserInfoLevel, ppUserInfoList_accumulate, dwTotalNumUsersFound);
    }

    goto cleanup;
}

DWORD
LsaSrvEndEnumUsers(
    HANDLE hServer,
    PCSTR  pszGUID
    )
{
    DWORD dwError = 0;
    PLSA_SRV_RECORD_ENUM_STATE pEnumState = NULL;
    PLSA_SRV_PROVIDER_STATE pProviderState = NULL;
    
    pEnumState = LsaSrvFindUserEnumState(hServer, pszGUID);
    if (!pEnumState) {
        dwError = LSA_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    for (pProviderState = pEnumState->pProviderStateList;
         pProviderState;
         pProviderState = pProviderState->pNext)
    {
        PLSA_AUTH_PROVIDER pProvider = pProviderState->pProvider;
        if (pProvider) {
           HANDLE hProvider = pProviderState->hProvider;
           pProvider->pFnTable->pfnEndEnumUsers(
                                       hProvider,
                                       pszGUID);
        }
    }
        
    LsaSrvFreeUserEnumState(
                        hServer,
                        pszGUID);

cleanup:

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LsaSrvGetNamesBySidList(
    HANDLE hServer,
    size_t sCount,
    PSTR* ppszSidList,
    PSTR** pppszDomainNames,
    PSTR** pppszSamAccounts,
    ADAccountType **ppTypes)
{
    DWORD dwError = 0;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    BOOLEAN bInLock = FALSE;
    HANDLE hProvider = (HANDLE)NULL;
    PSTR* ppszTempDomainNames = NULL;
    PSTR* ppszTempSamAccounts = NULL;
    ADAccountType *pTempTypes = NULL;
    PSTR* ppszTotalDomainNames = NULL;
    PSTR* ppszTotalSamAccounts = NULL;
    ADAccountType *pTotalTypes = NULL;
    size_t sIndex = 0;
 
    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    dwError = LsaAllocateMemory(
        sizeof(*ppszTotalDomainNames) * sCount,
        (PVOID*)&ppszTotalDomainNames);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateMemory(
        sizeof(*ppszTotalSamAccounts) * sCount,
        (PVOID*)&ppszTotalSamAccounts);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateMemory(
        sizeof(*pTotalTypes) * sCount,
        (PVOID*)&pTotalTypes);
    BAIL_ON_LSA_ERROR(dwError);
 
    for (pProvider = gpAuthProviderList;
         pProvider;
         pProvider = pProvider->pNext)
    {
        dwError = LsaSrvOpenProvider(hServer, pProvider, &hProvider);
        BAIL_ON_LSA_ERROR(dwError);       
 
        dwError = pProvider->pFnTable->pfnGetNamesBySidList(
            hProvider,
            sCount,
            ppszSidList,
            &ppszTempDomainNames,
            &ppszTempSamAccounts,
            &pTempTypes);

        if (dwError != LSA_ERROR_NOT_HANDLED)
        {
            BAIL_ON_LSA_ERROR(dwError);

            for (sIndex = 0; sIndex < sCount; sIndex++)
            {
                if (pTotalTypes[sIndex] == AccountType_NotFound)
                {
                    ppszTotalDomainNames[sIndex] = ppszTempDomainNames[sIndex];
                    ppszTempDomainNames[sIndex] = NULL;
                    ppszTotalSamAccounts[sIndex] = ppszTempSamAccounts[sIndex];
                    ppszTempSamAccounts[sIndex] = NULL;
                    pTotalTypes[sIndex] = pTempTypes[sIndex];
                }
            }
        }

        LsaSrvCloseProvider(pProvider, hProvider);
        hProvider = (HANDLE)NULL;

        LsaFreeStringArray(ppszTempSamAccounts, sCount);
        LsaFreeStringArray(ppszTempDomainNames, sCount);
        ppszTempDomainNames = NULL;
        ppszTempSamAccounts = NULL;
        LSA_SAFE_FREE_MEMORY(pTempTypes);
    }
 
    *pppszDomainNames = ppszTotalDomainNames;
    *pppszSamAccounts = ppszTotalSamAccounts;
    *ppTypes = pTotalTypes;
 
cleanup:
 
    if (hProvider != (HANDLE)NULL) {
        LsaSrvCloseProvider(pProvider, hProvider);
    }

    LsaFreeStringArray(ppszTempDomainNames, sCount);
    LsaFreeStringArray(ppszTempSamAccounts, sCount);
    LSA_SAFE_FREE_MEMORY(pTempTypes);
 
    LEAVE_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);
 
    return(dwError);

error:
    *pppszDomainNames = NULL;
    *pppszSamAccounts = NULL;
    *ppTypes = NULL;

    LsaFreeStringArray(ppszTotalDomainNames, sCount);
    LsaFreeStringArray(ppszTotalSamAccounts, sCount);
    LSA_SAFE_FREE_MEMORY(pTotalTypes);
    
    goto cleanup;
}
