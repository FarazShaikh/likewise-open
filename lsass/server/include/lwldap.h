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
 *        lsaldap.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        LDAP API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __LSALDAP_H__
#define __LSALDAP_H__

#ifndef KRB5_PRIVATE
#define KRB5_PRIVATE 1
#ifndef KRB5_DEPRECATED
#define KRB5_DEPRECATED 1
#include <krb5.h>
#endif
#endif
#include <gssapi/gssapi.h>
#include <gssapi/gssapi_generic.h>
#include <gssapi/gssapi_krb5.h>
#ifndef LDAP_DEPRECATED
#define LDAP_DEPRECATED 1
#include <ldap.h>
#endif
#include <lber.h>

//maximum length of LDAP query, in bytes.
#define MAX_LDAP_QUERY_LENGTH 4096

#define LSA_LDAP_OPT_GLOBAL_CATALOG 0x00000001
#define LSA_LDAP_OPT_SIGN_AND_SEAL  0x00000002
#define LSA_LDAP_OPT_ANNONYMOUS     0x00000004

typedef struct _AD_DIRECTORY_CONTEXT {
    LDAP *ld;
} AD_DIRECTORY_CONTEXT, *PAD_DIRECTORY_CONTEXT;

typedef void (*PFNLSA_COOKIE_FREE)(PVOID);

typedef struct __LSA_SEARCH_COOKIE
{
    BOOLEAN bSearchFinished;
    PVOID pvData;
    PFNLSA_COOKIE_FREE pfnFree;
} LSA_SEARCH_COOKIE, *PLSA_SEARCH_COOKIE;

DWORD
LwLdapPingTcp(
    PCSTR pszHostAddress,
    DWORD dwTimeoutSeconds
    );

DWORD
LwLdapOpenDirectoryDomain(
    IN PCSTR pszDnsDomainName,
    IN DWORD dwFlags,
    OUT PHANDLE phDirectory
    );

DWORD
LwLdapOpenDirectoryGc(
    IN PCSTR pszDnsForestName,
    IN DWORD dwFlags,
    OUT PHANDLE phDirectory
    );

DWORD
LwLdapOpenDirectoryServer(
    IN PCSTR pszServerAddress,
    IN PCSTR pszServerName,
    IN DWORD dwFlags,
    OUT PHANDLE phDirectory
    );

DWORD
LwLdapConvertDomainToDN(
    PCSTR pszDomainName,
    PSTR * ppszDomainDN
    );

DWORD
LwLdapConvertDNToDomain(
    PCSTR pszDN,
    PSTR* ppszDomainName
    );

void
LwLdapCloseDirectory(
    HANDLE hDirectory
    );

DWORD
LwLdapReadObject(
    HANDLE hDirectory,
    PCSTR  pszObjectDN,
    PSTR*  ppszAttributeList,
    LDAPMessage **ppMessage
    );

DWORD
LwLdapGetParentDN(
    PCSTR pszObjectDN,
    PSTR* ppszParentDN
    );

DWORD
LwLdapDirectorySearch(
    HANDLE hDirectory,
    PCSTR  pszObjectDN,
    int    scope,
    PCSTR  pszQuery,
    PSTR * ppszAttributeList,
    LDAPMessage **ppMessage
    );

DWORD
LwLdapDirectorySearchEx(
    HANDLE hDirectory,
    PCSTR pszObjectDN,
    int scope,
    PCSTR pszQuery,
    PSTR* ppszAttributeList,
    LDAPControl** ppServerControls,
    DWORD dwNumMaxEntries,
    LDAPMessage** ppMessage
    );

DWORD
LwLdapEnablePageControlOption(
    HANDLE hDirectory
    );

DWORD
LwLdapDisablePageControlOption(
    HANDLE hDirectory
    );

DWORD
LwLdapDirectoryOnePagedSearch(
    HANDLE         hDirectory,
    PCSTR          pszObjectDN,
    PCSTR          pszQuery,
    PSTR*          ppszAttributeList,
    DWORD          dwPageSize,
    PLSA_SEARCH_COOKIE pCookie,
    int            scope,
    LDAPMessage**  ppMessage
    );

LDAPMessage*
LwLdapFirstEntry(
    HANDLE hDirectory,
    LDAPMessage *pMessage
    );

LDAPMessage*
LwLdapNextEntry(
    HANDLE hDirectory,
    LDAPMessage* pMessage
    );

LDAP *
LwLdapGetSession(
    HANDLE hDirectory
    );

DWORD
LwLdapGetBytes(
        HANDLE hDirectory,
        LDAPMessage* pMessage,
        PSTR pszFieldName,
        PBYTE* ppszByteValue,
        PDWORD pszByteLen
        );

DWORD
LwLdapGetString(
    HANDLE hDirectory,
    LDAPMessage* pMessage,
    PCSTR pszFieldName,
    PSTR* ppszValue
    );

DWORD
LwLdapGetDN(
    HANDLE hDirectory,
    LDAPMessage* pMessage,
    PSTR* ppszValue
    );

DWORD
LwLdapIsValidADEntry(
    HANDLE hDirectory,
    LDAPMessage* pMessage,
    PBOOLEAN pbValidADEntry
    );

DWORD
LwLdapGetUInt32(
    HANDLE hDirectory,
    LDAPMessage* pMessage,
    PCSTR pszFieldName,
    PDWORD pdwValue
    );

DWORD
LwLdapGetUInt64(
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessage,
    IN PCSTR pszFieldName,
    OUT UINT64* pqwValue
    );

DWORD
LwLdapGetInt64(
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessage,
    IN PCSTR pszFieldName,
    OUT int64_t * pqwValue
    );

DWORD
LwLdapGetStrings(
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessage,
    IN PCSTR pszFieldName,
    OUT PSTR** pppszValues,
    OUT PDWORD pdwNumValues
    );

DWORD
LwLdapGetStringsWithExtDnResult(
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessage,
    IN PCSTR pszFieldName,
    IN BOOLEAN bDoSidParsing,
    OUT PSTR** pppszValues,
    OUT PDWORD pdwNumValues
    );

DWORD
LwLdapEscapeString(
    PSTR *ppszResult,
    PCSTR pszInput
    );

VOID
LwLdapFreeCookie(
    PVOID pCookie
    );

VOID
LwFreeCookieContents(
    IN OUT PLSA_SEARCH_COOKIE pCookie
    );

VOID
LwInitCookie(
    OUT PLSA_SEARCH_COOKIE pCookie
    );

DWORD
LwLdapParseExtendedDNResult(
    IN PCSTR pszResult,
    OUT PSTR* ppszSid);

DWORD
LwLdapDirectoryExtendedDNSearch(
    IN HANDLE hDirectory,
    IN PCSTR pszObjectDN,
    IN PCSTR pszQuery,
    IN PSTR* ppszAttributeList,
    IN int scope,
    OUT LDAPMessage** ppMessage
    );

#endif /* __LSALDAP_H__ */
