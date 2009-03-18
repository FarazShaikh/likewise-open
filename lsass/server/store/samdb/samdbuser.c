/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 *        samdbuser.c
 *
 * Abstract:
 *
 *
 *      Likewise SAM Database Provider
 *
 *      SamDbInitUserTable
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
 */
#include "includes.h"

#define DB_QUERY_CREATE_USERS_TABLE  \
    "create table samdbusers (       \
                UserRecordId     integer PRIMARY KEY, \
                DomainRecordId   integer,             \
                ObjectSID        text unique,         \
                Uid              integer,             \
                Name             text,                \
                Passwd           text,                \
                Gid              integer,             \
                UserInfoFlags    integer,             \
                Gecos            text,                \
                HomeDir          text,                \
                Shell            text,                \
                PasswdChangeTime integer,             \
                FullName         text,                \
                AccountExpiry    integer,             \
                LMOwf_1          integer,             \
                LMOwf_2          integer,             \
                LMOwf_3          integer,             \
                LMOwf_4          integer,             \
                NTOwf_1          integer,             \
                NTOwf_2          integer,             \
                NTOwf_3          integer,             \
                NTOwf_4          integer,             \
                CreatedTime      date,                \
                unique(DomainRecordId, Uid),          \
                unique(DomainRecordId, Name),         \
               )"


#define DB_QUERY_CREATE_USERS_INSERT_TRIGGER                   \
    "create trigger samdbusers_createdtime                     \
     after insert on samdbusers                                \
     begin                                                     \
          update samdbusers                                    \
          set CreatedTime = DATETIME('NOW')                    \
          where rowid = new.rowid;                             \
                                                               \
          insert into samdbgroupmembers (UserRecordId, GroupRecordId) \
          values (new.UserRecordId, new.GroupRecordId);        \
     end"

#define DB_QUERY_CREATE_USERS_DELETE_TRIGGER                   \
    "create trigger samdbusers_delete_record                   \
     after delete on samdbusers                                \
     begin                                                     \
          delete from samdbgroupmembers where UserRecordId = old.UserRecordId;   \
     end"


DWORD
SamDbInitUserTable(
    PSAM_DB_CONTEXT pDbContext
    )
{
    DWORD dwError = 0;
    PSTR pszError = NULL;

    dwError = sqlite3_exec(
                    pDbContext->pDbHandle,
                    DB_QUERY_CREATE_USERS_TABLE,
                    NULL,
                    NULL,
                    &pszError);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = sqlite3_exec(
                    pDbContext->pDbHandle,
                    DB_QUERY_CREATE_USERS_INSERT_TRIGGER,
                    NULL,
                    NULL,
                    &pszError);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = sqlite3_exec(
                    pDbContext->pDbHandle,
                    DB_QUERY_CREATE_USERS_DELETE_TRIGGER,
                    NULL,
                    NULL,
                    &pszError);
    BAIL_ON_SAMDB_ERROR(dwError);

cleanup:

    return dwError;

error:

    if (pszError)
    {
        sqlite3_free(pszError);
    }

    goto cleanup;
}

DWORD
SamDbAddUserAttrLookups(
    PSAMDB_ATTRIBUTE_LOOKUP pAttrLookup
    )
{
    DWORD dwError = 0;
    struct {
        PSTR pszAttrName;
        DIRECTORY_ATTR_TYPE attrType;
        BOOL bIsMandatory;
    } userAttrs[] =
    {
        {
            DIRECTORY_ATTR_TAG_USER_NAME,
            DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            TRUE
        },
        {
            DIRECTORY_ATTR_TAG_USER_FULLNAME,
            DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            TRUE
        },
        {
            DIRECTORY_ATTR_TAG_UID,
            DIRECTORY_ATTR_TYPE_INTEGER,
            TRUE
        },
        {
            DIRECTORY_ATTR_TAG_USER_SID,
            DIRECTORY_ATTR_TYPE_NT_SECURITY_DESCRIPTOR,
            TRUE
        },
        {
            DIRECTORY_ATTR_TAG_USER_PRIMARY_GROUP,
            DIRECTORY_ATTR_TYPE_INTEGER,
            TRUE
        },
        {
            DIRECTORY_ATTR_TAG_USER_PASSWORD,
            DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            FALSE
        },
        {
            DIRECTORY_ATTR_TAG_GECOS,
            DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            FALSE
        },
        {
            DIRECTORY_ATTR_TAG_HOMEDIR,
            DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            TRUE
        },
        {
            DIRECTORY_ATTR_TAG_PASSWORD_CHANGE_TIME,
            DIRECTORY_ATTR_TYPE_INTEGER,
            FALSE
        },
        {
            DIRECTORY_ATTR_TAG_ACCOUNT_EXPIRY,
            DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
            FALSE
        },
        {
            DIRECTORY_ATTR_TAG_USER_INFO_FLAGS,
            DIRECTORY_ATTR_TYPE_INTEGER,
            FALSE
        },
        {
            DIRECTORY_ATTR_TAG_USER_LM_HASH,
            DIRECTORY_ATTR_TYPE_OCTET_STREAM,
            FALSE
        },
        {
            DIRECTORY_ATTR_TAG_USER_NT_HASH,
            DIRECTORY_ATTR_TYPE_OCTET_STREAM,
            FALSE
        }
    };
    DWORD dwNumAttrs = sizeof(userAttrs)/sizeof(userAttrs[0]);
    DWORD iAttr = 0;
    PSAMDB_ATTRIBUTE_LOOKUP_ENTRY pAttrEntry = NULL;

    for(; iAttr < dwNumAttrs; iAttr++)
    {
        dwError = DirectoryAllocateMemory(
                        sizeof(SAMDB_ATTRIBUTE_LOOKUP_ENTRY),
                        (PVOID*)&pAttrEntry);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = LsaMbsToWc16s(
                        userAttrs[iAttr].pszAttrName,
                        &pAttrEntry->pwszAttributeName);
        BAIL_ON_SAMDB_ERROR(dwError);

        pAttrEntry->bIsMandatory = userAttrs[iAttr].bIsMandatory;
        pAttrEntry->attrType = userAttrs[iAttr].attrType;

        dwError = LwRtlRBTreeAdd(
                        pAttrLookup->pAttrTree,
                        pAttrEntry->pwszAttributeName,
                        pAttrEntry);
        BAIL_ON_SAMDB_ERROR(dwError);

        pAttrEntry = NULL;
    }

cleanup:

    return dwError;

error:

    if (pAttrEntry)
    {
        SamDbFreeAttributeLookupEntry(pAttrEntry);
    }

    goto cleanup;
}


DWORD
SamDbAddUser(
    HANDLE        hDirectory,
    PWSTR         pwszObjectName,
    DIRECTORY_MOD Modifications[]
    )
{
    DWORD dwError = 0;
    PSAM_DIRECTORY_CONTEXT pDirContext = NULL;

    pDirContext = (PSAM_DIRECTORY_CONTEXT)hDirectory;

    return dwError;
}

