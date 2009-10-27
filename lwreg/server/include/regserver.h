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
 *        regserver.h
 *
 * Abstract:
 *
 *        Registry
 *
 *        Inter-process communication (Server) API for Users
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 */

#ifndef __REGSERVER_H_
#define __REGSERVER_H_

typedef struct __REG_SRV_API_STATE
{
    uid_t  peerUID;
    gid_t  peerGID;
    HANDLE hEventLog;
} REG_SRV_API_STATE, *PREG_SRV_API_STATE;

typedef struct __REG_KEY_CONTEXT
{
    LONG refCount;

    pthread_rwlock_t mutex;
    pthread_rwlock_t* pMutex;

    PSTR pszKeyName;
    PSTR pszParentKeyName;

    DWORD dwNumSubKeys;
    DWORD dwNumCacheSubKeys;
    size_t sMaxSubKeyLen;
    PSTR* ppszSubKeyNames;
    BOOLEAN bHasSubKeyInfo;

    DWORD dwNumValues;
    DWORD dwNumCacheValues;
    size_t sMaxValueNameLen;
    size_t sMaxValueLen;
    PREG_DATA_TYPE pTypes;
    PSTR* ppszValueNames;
    PSTR* ppszValues;
    BOOLEAN bHasValueInfo;

} REG_KEY_CONTEXT, *PREG_KEY_CONTEXT;

#define LWREG_LOCK_MUTEX(bInLock, mutex) \
    if (!bInLock) { \
       int thr_err = pthread_mutex_lock(mutex); \
       if (thr_err) { \
           LW_LOG_ERROR("Failed to lock mutex. Aborting program"); \
           abort(); \
       } \
       bInLock = TRUE; \
    }

#define LWREG_UNLOCK_MUTEX(bInLock, mutex) \
    if (bInLock) { \
       int thr_err = pthread_mutex_unlock(mutex); \
       if (thr_err) { \
           LW_LOG_ERROR("Failed to unlock mutex. Aborting program"); \
           abort(); \
       } \
       bInLock = FALSE; \
    }

//reader
#define LWREG_LOCK_RWMUTEX_SHARED(bInLock, mutex) \
    if (!bInLock) { \
       int thr_err = pthread_rwlock_rdlock(mutex); \
       if (thr_err) { \
           LW_LOG_ERROR("Failed to acquire shared lock on rw mutex. Aborting program"); \
           abort(); \
       } \
       bInLock = TRUE; \
    }

 //writer
#define LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, mutex) \
    if (!bInLock) { \
       int thr_err = pthread_rwlock_wrlock(mutex); \
       if (thr_err) { \
           LW_LOG_ERROR("Failed to acquire exclusive lock on rw mutex. Aborting program"); \
           abort(); \
       } \
       bInLock = TRUE; \
    }

#define LWREG_UNLOCK_RWMUTEX(bInLock, mutex) \
    if (bInLock) { \
       int thr_err = pthread_rwlock_unlock(mutex); \
       if (thr_err) { \
           LW_LOG_ERROR("Failed to unlock rw mutex. Aborting program"); \
           abort(); \
       } \
       bInLock = FALSE; \
    }

void
RegSrvSafeFreeKeyContext(
    IN OUT PREG_KEY_CONTEXT* ppKeyResult
    );

void
RegSrvIpcDestructSession(
    LWMsgSecurityToken* pToken,
    void* pSessionData
    );

LWMsgStatus
RegSrvIpcConstructSession(
    LWMsgSecurityToken* pToken,
    void* pData,
    void** ppSessionData
    );

DWORD
RegSrvApiInit(
    VOID
    );

DWORD
RegSrvApiShutdown(
    VOID
    );

DWORD
RegSrvOpenServer(
    uid_t peerUID,
    gid_t peerGID,
    PHANDLE phServer
    );

DWORD
RegSrvOpenServerEnum(
    PHANDLE phServer
    );

VOID
RegSrvGetUid(
    HANDLE hServer,
    uid_t* pUid
    );

void
RegSrvCloseServerEnum(
    HANDLE hServer
    );

void
RegSrvCloseServer(
    HANDLE hServer
    );

LWMsgDispatchSpec*
RegSrvGetDispatchSpec(
    void
    );

BOOLEAN
RegSrvIsValidKeyName(
    PSTR pszKeyName
    );

DWORD
RegSrvEnumRootKeysW(
    IN HANDLE Handle,
    OUT PWSTR** pppszRootKeys,
    OUT PDWORD pdwNumRootKeys
    );

DWORD
RegSrvCreateKeyEx(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN PCWSTR pSubKey,
    IN DWORD Reserved,
    IN OPTIONAL PWSTR pClass,
    IN DWORD dwOptions,
    IN REGSAM samDesired,
    IN OPTIONAL PSECURITY_ATTRIBUTES pSecurityAttributes,
    OUT PHKEY phkResult,
    OUT OPTIONAL PDWORD pdwDisposition
    );

DWORD
RegSrvOpenKeyExA(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pszSubKey,
    IN DWORD ulOptions,
    IN REGSAM samDesired,
    OUT PHKEY phkResult
    );

DWORD
RegSrvOpenKeyExW(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pwszSubKey,
    IN DWORD ulOptions,
    IN REGSAM samDesired,
    OUT PHKEY phkResult
    );

VOID
RegSrvCloseKey(
    HKEY hKey
    );

DWORD
RegSrvDeleteKey(
    HANDLE Handle,
    HKEY hKey,
    PCWSTR pSubKey
    );

DWORD
RegSrvDeleteKeyValue(
    HANDLE Handle,
    HKEY hKey,
    PCWSTR pSubKey,
    PCWSTR pValueName
    );

DWORD
RegSrvDeleteTree(
    HANDLE Handle,
    HKEY hKey,
    PCWSTR lpSubKey
    );

DWORD
RegSrvDeleteValue(
    HANDLE Handle,
    HKEY hKey,
    PCWSTR lpValueName
    );

DWORD
RegSrvEnumKeyEx(
    HANDLE Handle,
    HKEY hKey,
    DWORD dwIndex,
    PWSTR pName,
    PDWORD pcName,
    PDWORD pReserved,
    PWSTR pClass,
    PDWORD pcClass,
    PFILETIME pftLastWriteTime
    );

DWORD
RegSrvEnumValueA(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN DWORD dwIndex,
    OUT PSTR pszValueName, /*buffer hold valueName*/
    IN OUT PDWORD pcchValueName, /*input - buffer pValueName length*/
    IN PDWORD pReserved,
    OUT OPTIONAL PDWORD pType,
    OUT OPTIONAL PBYTE pData,/*buffer hold value content*/
    IN OUT OPTIONAL PDWORD pcbData /*input - buffer pData length*/
    );

DWORD
RegSrvEnumValueW(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN DWORD dwIndex,
    OUT PWSTR pValueName, /*buffer hold valueName*/
    IN OUT PDWORD pcchValueName, /*input - buffer pValueName length*/
    IN PDWORD pReserved,
    OUT OPTIONAL PDWORD pType,
    OUT OPTIONAL PBYTE pData,/*buffer hold value content*/
    IN OUT OPTIONAL PDWORD pcbData /*input - buffer pData length*/
    );

DWORD
RegSrvGetValueA(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pszSubKey,
    IN OPTIONAL PCSTR pszValue,
    IN OPTIONAL REG_DATA_TYPE_FLAGS Flags,
    OUT PDWORD pdwType,
    OUT PBYTE pData,
    IN OUT PDWORD pcbData
    );

DWORD
RegSrvGetValueW(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN OPTIONAL PCWSTR pValue,
    IN OPTIONAL REG_DATA_TYPE_FLAGS Flags,
    OUT PDWORD pdwType,
    OUT PBYTE pData,
    IN OUT PDWORD pcbData
    );

LWMsgStatus
RegSrvQueryInfoKey(
    HANDLE Handle,
    HKEY hKey,
    PWSTR pClass,
    PDWORD pcClass,
    PDWORD pReserved,
    PDWORD pcSubKeys,
    PDWORD pcMaxSubKeyLen,
    PDWORD pcMaxClassLen,
    PDWORD pcValues,
    PDWORD pcMaxValueNameLen,
    PDWORD pcMaxValueLen,
    PDWORD pcbSecurityDescriptor,
    PFILETIME pftLastWriteTime
    );

DWORD
RegSrvQueryMultipleValues(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OUT PVALENT pVal_list,
    IN DWORD num_vals,
    OUT OPTIONAL PWSTR pValue,
    OUT OPTIONAL PDWORD pdwTotalsize
    );

DWORD
RegSrvQueryValueExA(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN PCSTR pszValueName,
    IN PDWORD pReserved,
    OUT PDWORD pType,
    OUT PBYTE pData,
    IN OUT PDWORD pcbData
    );

DWORD
RegSrvQueryValueExW(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN PCWSTR pValueName,
    IN PDWORD pReserved,
    OUT PDWORD pType,
    OUT PBYTE pData,
    IN OUT PDWORD pcbData
    );

DWORD
RegSrvSetValueExA(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pszValueName,
    IN DWORD Reserved,
    IN DWORD dwType,
    IN const BYTE *pData,
    DWORD cbData
    );

DWORD
RegSrvSetValueExW(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pValueName,
    IN DWORD Reserved,
    IN DWORD dwType,
    IN const BYTE *pData,
    DWORD cbData
    );


//Server side helper functions
#if 0
PREG_KEY_CONTEXT
RegSrvLocateActiveKey(
    IN PCSTR pszKeyName
    );

PREG_KEY_CONTEXT
RegSrvLocateActiveKey_inlock(
    IN PCSTR pszKeyName
    );

DWORD
RegSrvInsertActiveKey(
    IN PREG_KEY_CONTEXT pKeyResult
    );

DWORD
RegSrvInsertActiveKey_inlock(
    IN PREG_KEY_CONTEXT pKeyResult
    );

VOID
RegSrvDeleteActiveKey(
    IN PSTR pszKeyName
    );

VOID
RegSrvDeleteActiveKey_inlock(
    IN PSTR pszKeyName
    );

void
RegSrvResetParentKeySubKeyInfo(
    IN PSTR pszParentKeyName
    );

void
RegSrvResetKeyValueInfo(
    IN PSTR pszKeyName
    );

VOID
RegSrvReleaseKey(
    PREG_KEY_CONTEXT pKeyResult
    );
#endif

DWORD
RegSrvGetKeyRefCount(
    IN PREG_KEY_CONTEXT pKeyResult
    );

void
RegSrvSetHasSubKeyInfo(
    IN BOOLEAN bHasSubKeyInfo,
    IN OUT PREG_KEY_CONTEXT pKeyResult
    );

BOOLEAN
RegSrvHasSubKeyInfo(
    IN PREG_KEY_CONTEXT pKeyResult
    );

DWORD
RegSrvSubKeyNum(
    IN PREG_KEY_CONTEXT pKeyResult
    );

size_t
RegSrvSubKeyNameMaxLen(
    IN PREG_KEY_CONTEXT pKeyResult
    );

PCSTR
RegSrvSubKeyName(
    IN PREG_KEY_CONTEXT pKeyResult,
    IN DWORD dwIndex
    );

void
RegSrvSetHasValueInfo(
    IN BOOLEAN bHasValueInfo,
    IN OUT PREG_KEY_CONTEXT pKeyResult
    );

BOOLEAN
RegSrvHasValueInfo(
    IN PREG_KEY_CONTEXT pKeyResult
    );

DWORD
RegSrvValueNum(
    IN PREG_KEY_CONTEXT pKeyResult
    );

size_t
RegSrvMaxValueNameLen(
    IN PREG_KEY_CONTEXT pKeyResult
    );

size_t
RegSrvMaxValueLen(
    IN PREG_KEY_CONTEXT pKeyResult
    );

PCSTR
RegSrvValueName(
    IN PREG_KEY_CONTEXT pKeyResult,
    DWORD dwIndex
    );

PCSTR
RegSrvValueContent(
    IN PREG_KEY_CONTEXT pKeyResult,
    DWORD dwIndex
    );

REG_DATA_TYPE
RegSrvValueType(
    IN PREG_KEY_CONTEXT pKeyResult,
    DWORD dwIndex
    );

#endif // __REGSERVER_H_
