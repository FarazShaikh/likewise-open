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
 *        externs.h
 *
 * Abstract:
 *
 *        Likewise Registry
 *
 *        External Variables
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __EXTERNS_H__
#define __EXTERNS_H__
#include "memstore_p.h"


extern REGPROV_PROVIDER_FUNCTION_TABLE gRegMemProviderAPITable;

extern REG_DB_HANDLE ghCacheConnection;

PREGMEM_NODE ghMemRegRoot;

extern pthread_mutex_t gMemRegDbMutex;
extern BOOLEAN gbInLockDbMutex;

extern pthread_mutex_t gExportMutex;
extern BOOLEAN gbInLockExportMutex;
extern pthread_cond_t gExportCond;

extern BOOLEAN gbValueChanged;
extern PMEMDB_FILE_EXPORT_CTX gExportCtx;


#if 0
extern REG_SRV_MEMORY_KEYLOOKUP gActiveKeyList;

extern REG_SRV_MEMORY_KEYLOOKUP gRegDbKeyList;

extern const DWORD dwDefaultCacheSize;

extern GENERIC_MAPPING gRegKeyGenericMapping;
#endif

#endif /* __EXTERNS_H__ */

