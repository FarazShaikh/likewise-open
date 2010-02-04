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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        context.c
 *
 * Abstract:
 *
 *        NTLM Security Context for negotiate/challenge/authenticate
 *
 * Authors: Marc Guy (mguy@likewisesoftware.com)
 *
 */

#include "ntlmsrvapi.h"

/******************************************************************************/
VOID
NtlmReleaseContext(
    IN PNTLM_CONTEXT_HANDLE phContext
    )
{
    PNTLM_CONTEXT pContext = *phContext;

    pContext->nRefCount--;

    LW_ASSERT(pContext->nRefCount >= 0);

    if (!(pContext->nRefCount))
    {
        //LsaListRemove(&pContext->ListEntry);
        NtlmFreeContext(&pContext);
    }

    *phContext = NULL;
}

/******************************************************************************/
VOID
NtlmGetContextInfo(
    IN NTLM_CONTEXT_HANDLE ContextHandle,
    OUT OPTIONAL PNTLM_STATE pNtlmState,
    OUT OPTIONAL PDWORD pNegotiatedFlags,
    OUT OPTIONAL PBYTE* ppSessionKey,
    OUT OPTIONAL PNTLM_CRED_HANDLE pCredHandle
    )
{
    PNTLM_CONTEXT pContext = ContextHandle;

    if (pNtlmState)
    {
        *pNtlmState = pContext->NtlmState;
    }

    if (pNegotiatedFlags)
    {
        *pNegotiatedFlags = pContext->NegotiatedFlags;
    }

    if (ppSessionKey)
    {
        *ppSessionKey = pContext->SessionKey;
    }

    if (pCredHandle)
    {
        *pCredHandle = pContext->CredHandle;
    }
}

/******************************************************************************/
DWORD
NtlmCreateContext(
    IN NTLM_CRED_HANDLE hCred,
    OUT PNTLM_CONTEXT* ppNtlmContext
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PNTLM_CONTEXT pContext = NULL;

    if (!ppNtlmContext)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppNtlmContext = NULL;

    dwError = LwAllocateMemory(
        sizeof(NTLM_CONTEXT),
        OUT_PPVOID(&pContext)
        );

    BAIL_ON_LSA_ERROR(dwError);

    pContext->NtlmState = NtlmStateBlank;
    pContext->nRefCount = 1;

    pContext->CredHandle = hCred;
    NtlmReferenceCredential(pContext->CredHandle);

cleanup:
    *ppNtlmContext = pContext;
    return dwError;
error:
    LW_SAFE_FREE_MEMORY(pContext);
    goto cleanup;
}

/******************************************************************************/
VOID
NtlmFreeContext(
    IN PNTLM_CONTEXT* ppContext
    )
{
    PNTLM_CONTEXT pContext = *ppContext;

    NtlmReleaseCredential(&pContext->CredHandle);

    if (pContext->pUnsealKey != pContext->pSealKey)
    {
        LW_SAFE_FREE_MEMORY(pContext->pUnsealKey);
    }
    LW_SAFE_FREE_MEMORY(pContext->pSealKey);
    LW_SAFE_FREE_STRING(pContext->pszClientUsername);

    if (pContext->pdwSendMsgSeq != pContext->pdwRecvMsgSeq)
    {
        LW_SAFE_FREE_MEMORY(pContext->pdwSendMsgSeq);
    }
    LW_SAFE_FREE_MEMORY(pContext->pdwRecvMsgSeq);

    if (pContext->pUserInfo)
    {
        LsaFreeAuthUserInfo(&pContext->pUserInfo);
    }

    LW_SAFE_FREE_MEMORY(pContext);
    *ppContext = NULL;
}

/******************************************************************************/
DWORD
NtlmGetMessageFromSecBufferDesc(
    IN const SecBufferDesc* pSecBufferDesc,
    OUT PDWORD pdwMessageSize,
    OUT const VOID** ppMessage
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PSecBuffer pSecBuffer = NULL;
    DWORD dwMessageSize = 0;
    PBYTE pMessage = NULL;

    *pdwMessageSize = 0;
    *ppMessage = NULL;

    if (!pSecBufferDesc)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    // Transfered context tokens only contain one SecBuffer and are tagged as
    // tokens... verify
    if (pSecBufferDesc->cBuffers != 1 || !(pSecBufferDesc->pBuffers))
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pSecBuffer = pSecBufferDesc->pBuffers;

    if (pSecBuffer->BufferType != SECBUFFER_TOKEN ||
       pSecBuffer->cbBuffer == 0)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pMessage = pSecBuffer->pvBuffer;
    dwMessageSize = pSecBuffer->cbBuffer;

cleanup:
    *pdwMessageSize = dwMessageSize;
    *ppMessage = pMessage;

    return dwError;

error:
    dwMessageSize = 0;
    pMessage = NULL;
    goto cleanup;
}

/******************************************************************************/
DWORD
NtlmGetRandomBuffer(
    OUT PBYTE pBuffer,
    IN DWORD dwLen
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    INT nFileDesc;
    DWORD dwBytesRead = 0;

    if (!pBuffer || dwLen <= 0)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    nFileDesc = open(NTLM_URANDOM_DEV, O_RDONLY);
    if (-1 == nFileDesc)
    {
        nFileDesc = open(NTLM_RANDOM_DEV, O_RDONLY);
        if (-1 == nFileDesc)
        {
            dwError = LW_ERROR_INTERNAL; //LwMapErrnoToLwError(errno);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    dwBytesRead = read(nFileDesc, pBuffer, dwLen);
    close(nFileDesc);

    if (dwBytesRead < dwLen)
    {
        dwError = LW_ERROR_INTERNAL;
    }

error:
    return dwError;
}


/******************************************************************************/
DWORD
NtlmCreateNegotiateMessage(
    IN DWORD dwOptions,
    IN PCSTR pDomain,
    IN PCSTR pWorkstation,
    IN PBYTE pOsVersion,
    OUT PDWORD pdwSize,
    OUT PNTLM_NEGOTIATE_MESSAGE_V1* ppNegMsg
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PNTLM_NEGOTIATE_MESSAGE_V1 pMessage = NULL;
    DWORD dwSize = 0;
    // The following pointers point into pMessage and will not be freed on error
    PNTLM_SEC_BUFFER pDomainSecBuffer = NULL;
    PNTLM_SEC_BUFFER pWorkstationSecBuffer = NULL;
    PBYTE pBuffer = NULL;

    // sanity checks
    if (!ppNegMsg)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppNegMsg = NULL;
    *pdwSize = 0;

    if (dwOptions & NTLM_FLAG_DOMAIN)
    {
        if (!pDomain)
        {
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
        }
        else
        {
            dwSize += strlen(pDomain);
        }
    }

    if (dwOptions & NTLM_FLAG_WORKSTATION)
    {
        if (!pWorkstation)
        {
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
        }
        else
        {
            dwSize += strlen(pWorkstation);
        }
    }

    dwSize += sizeof(NTLM_NEGOTIATE_MESSAGE_V1);

    // There is no flag to indicate if there is OS version information added
    // to the packet... if we have OS information, we will need to allocate
    // Domain and Workstation information as well.
    if (pOsVersion)
    {
        dwSize +=
            sizeof(NTLM_SEC_BUFFER) +
            sizeof(NTLM_SEC_BUFFER) +
            NTLM_WIN_SPOOF_SIZE;
    }
    else if (dwOptions & NTLM_FLAG_WORKSTATION)
    {
        dwSize +=
            sizeof(NTLM_SEC_BUFFER) +
            sizeof(NTLM_SEC_BUFFER);
    }
    else if (dwOptions & NTLM_FLAG_DOMAIN)
    {
        dwSize += sizeof(NTLM_SEC_BUFFER);
    }

    dwError = LwAllocateMemory(dwSize, OUT_PPVOID(&pMessage));
    BAIL_ON_LSA_ERROR(dwError);

    // Data is checked and memory is allocated; fill in the structure
    //
    memcpy(
        &pMessage->NtlmSignature,
        NTLM_NETWORK_SIGNATURE,
        NTLM_NETWORK_SIGNATURE_SIZE);
    pMessage->MessageType = NTLM_NEGOTIATE_MSG;
    pMessage->NtlmFlags = dwOptions;

    // Start writing optional information (if there is any) after the structure
    pBuffer = (PBYTE)pMessage + sizeof(NTLM_NEGOTIATE_MESSAGE_V1);

    // If you have OS info, you HAVE to at least adjust the pointers past the
    // domain secbuffer (even if you don't fill it in with data).
    if (dwOptions & NTLM_FLAG_DOMAIN ||
        pOsVersion)
    {
        if (pDomain && (dwOptions & NTLM_FLAG_DOMAIN))
        {
            pDomainSecBuffer = (PNTLM_SEC_BUFFER)pBuffer;

            // The Domain name is ALWAYS given as an OEM (i.e. ASCII) string
            pDomainSecBuffer->usLength = strlen(pDomain);
            pDomainSecBuffer->usMaxLength = pDomainSecBuffer->usLength;
        }

        pBuffer += sizeof(NTLM_SEC_BUFFER);
    }

    // If you have a domain or OS info, you HAVE to at least adjust the pointers
    // past the workstation secbuffer (even if you don't fill it in with data)
    if (dwOptions & NTLM_FLAG_WORKSTATION ||
        dwOptions & NTLM_FLAG_DOMAIN ||
        pOsVersion)
    {
        if (pWorkstation && (dwOptions & NTLM_FLAG_WORKSTATION))
        {
            pWorkstationSecBuffer = (PNTLM_SEC_BUFFER)pBuffer;

            // The Workstation name is also ALWAYS given as an OEM string
            pWorkstationSecBuffer->usLength = strlen(pWorkstation);
            pWorkstationSecBuffer->usMaxLength = pWorkstationSecBuffer->usLength;
        }

        pBuffer += sizeof(NTLM_SEC_BUFFER);
    }

    if (pOsVersion)
    {
        memcpy(pBuffer, pOsVersion, NTLM_WIN_SPOOF_SIZE);

        pBuffer += NTLM_WIN_SPOOF_SIZE;
    }

    // So now we're at the very end of the buffer; copy the optional data
    if (pWorkstationSecBuffer && pWorkstationSecBuffer->usLength)
    {
        memcpy(pBuffer, pWorkstation, pWorkstationSecBuffer->usLength);
        pWorkstationSecBuffer->dwOffset = pBuffer - (PBYTE)pMessage;
        pBuffer += pWorkstationSecBuffer->usLength;
    }

    if (pDomainSecBuffer && pDomainSecBuffer->usLength)
    {
        memcpy(pBuffer, pDomain, pDomainSecBuffer->usLength);
        pDomainSecBuffer->dwOffset = pBuffer - (PBYTE)pMessage;
        pBuffer += pDomainSecBuffer->usLength;
    }

    LW_ASSERT(pBuffer == (PBYTE)pMessage + dwSize);

cleanup:
    *ppNegMsg = pMessage;
    *pdwSize = dwSize;
    return dwError;
error:
    LW_SAFE_FREE_MEMORY(pMessage);
    dwSize = 0;
    goto cleanup;
}

/******************************************************************************/
DWORD
NtlmCreateChallengeMessage(
    IN const NTLM_NEGOTIATE_MESSAGE_V1* pNegMsg,
    IN PCSTR pServerName,
    IN PCSTR pDomainName,
    IN PCSTR pDnsServerName,
    IN PCSTR pDnsDomainName,
    IN PBYTE pOsVersion,
    IN BYTE Challenge[NTLM_CHALLENGE_SIZE],
    OUT PDWORD pdwSize,
    OUT PNTLM_CHALLENGE_MESSAGE* ppChlngMsg
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwSize = 0;
    PNTLM_CHALLENGE_MESSAGE pMessage = NULL;
    DWORD dwTargetInfoSize = 0;
    DWORD dwTargetNameSize = 0;
    DWORD dwOptions = 0;
    // The following pointers point into pMessage and will not be freed on error
    PBYTE pTrav = NULL;
    PBYTE pBuffer = NULL;
    PNTLM_SEC_BUFFER pTargetInfoSecBuffer= NULL;
    PNTLM_TARGET_INFO_BLOCK pTargetInfoBlock = NULL;
    NTLM_CONFIG config;

    // sanity checks
    if (!pNegMsg || !ppChlngMsg)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppChlngMsg = NULL;
    *pdwSize = 0;

    dwError = NtlmReadRegistry(&config);
    BAIL_ON_LSA_ERROR(dwError);

    dwSize = sizeof(NTLM_CHALLENGE_MESSAGE);

    // sanity check... we need to have at least NTLM or NTLM2 requested
    if (!(pNegMsg->NtlmFlags & NTLM_FLAG_NTLM) &&
       !(pNegMsg->NtlmFlags & NTLM_FLAG_NTLM2))
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    // We need to build up the challenge options based on the negotiate options.
    // We *must* have either unicode or ansi string set
    //
    if ((pNegMsg->NtlmFlags & NTLM_FLAG_UNICODE) &&
            config.bSupportUnicode)
    {
        dwOptions |= NTLM_FLAG_UNICODE;
    }
    else if (pNegMsg->NtlmFlags & NTLM_FLAG_OEM)
    {
        dwOptions |= NTLM_FLAG_OEM;
    }
    else
    {
        // bit of a sanity check, the negotiation message should have had at
        // least one of those flags set... if it didin't...
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }
    if ((pNegMsg->NtlmFlags & NTLM_FLAG_NTLM2) &&
            config.bSupportNTLM2SessionSecurity)
    {
        dwOptions |= NTLM_FLAG_NTLM2;
    }
    if ((pNegMsg->NtlmFlags & NTLM_FLAG_KEY_EXCH) &&
            config.bSupportKeyExchange)
    {
        dwOptions |= NTLM_FLAG_KEY_EXCH;
    }
    if ((pNegMsg->NtlmFlags & NTLM_FLAG_56) &&
            config.bSupport56bit)
    {
        dwOptions |= NTLM_FLAG_56;
    }
    if ((pNegMsg->NtlmFlags & NTLM_FLAG_128) &&
            config.bSupport128bit)
    {
        dwOptions |= NTLM_FLAG_128;
    }

    // calculate optional data size
    if (pOsVersion)
    {
        dwSize +=
            NTLM_LOCAL_CONTEXT_SIZE +   // NTLM context (for local auth)
            sizeof(NTLM_SEC_BUFFER) +   // For target information block
            NTLM_WIN_SPOOF_SIZE;        // Win version info

        // This is for the terminating target info block
        dwTargetInfoSize += sizeof(NTLM_TARGET_INFO_BLOCK);

        dwOptions |= NTLM_FLAG_TARGET_INFO;
    }
    else
    {
        // This is the same as the 'if' statement above, minus accounting for
        // space for the OS version spoof.
        dwSize +=
            NTLM_LOCAL_CONTEXT_SIZE +
            sizeof(NTLM_SEC_BUFFER);

        dwTargetInfoSize += sizeof(NTLM_TARGET_INFO_BLOCK);
        dwOptions |= NTLM_FLAG_TARGET_INFO;
    }

    // Allocate space in the target information block for each piece of target
    // information we have.
    // Target information block info is always in unicode format.
    if (!LW_IS_NULL_OR_EMPTY_STR(pServerName))
    {
        dwTargetInfoSize += sizeof(NTLM_TARGET_INFO_BLOCK);
        dwTargetInfoSize += strlen(pServerName) * sizeof(WCHAR);
    }
    if (!LW_IS_NULL_OR_EMPTY_STR(pDomainName))
    {
        dwTargetInfoSize += sizeof(NTLM_TARGET_INFO_BLOCK);
        dwTargetInfoSize += strlen(pDomainName) * sizeof(WCHAR);
    }
    if (!LW_IS_NULL_OR_EMPTY_STR(pDnsServerName))
    {
        dwTargetInfoSize += sizeof(NTLM_TARGET_INFO_BLOCK);
        dwTargetInfoSize += strlen(pDnsServerName) * sizeof(WCHAR);
    }
    if (!LW_IS_NULL_OR_EMPTY_STR(pDnsDomainName))
    {
        dwTargetInfoSize += sizeof(NTLM_TARGET_INFO_BLOCK);
        dwTargetInfoSize += strlen(pDnsDomainName) * sizeof(WCHAR);
    }

    dwSize += dwTargetInfoSize;

    if (pNegMsg->NtlmFlags & NTLM_FLAG_REQUEST_TARGET)
    {
        // To determine what name will be returned we check in this order:
        // 1). Domain name
        // 2). Server name
        //
        // For now, we never set the type to share (since we don't really know
        // what that means).
        if (!LW_IS_NULL_OR_EMPTY_STR(pDomainName))
        {
            dwTargetNameSize = strlen(pDomainName);
            dwOptions |= NTLM_FLAG_TYPE_DOMAIN;
        }
        else if (!LW_IS_NULL_OR_EMPTY_STR(pServerName))
        {
            dwTargetNameSize = strlen(pServerName);
            dwOptions |= NTLM_FLAG_TYPE_SERVER;
        }

        if (dwOptions & NTLM_FLAG_UNICODE)
        {
            dwTargetNameSize *= sizeof(WCHAR);
        }

        // Documentation indicates that the NTLM_FLAG_REQUEST_TARGET flag is
        // often set but doesn't have much meaning in a type 2 message (it's
        // for type 1 messages).  We'll propogate it for now when we have
        // target information to return (but we may remove it or make it
        // configurable in the future.
        dwOptions |= NTLM_FLAG_REQUEST_TARGET;
    }

    dwSize += dwTargetNameSize;

    dwError = LwAllocateMemory(dwSize, OUT_PPVOID(&pMessage));
    BAIL_ON_LSA_ERROR(dwError);

    // If the client wants to support a dummy signature, we will too... as long
    // as it doesn't support real signing.
    if (pNegMsg->NtlmFlags & NTLM_FLAG_ALWAYS_SIGN &&
        !(pNegMsg->NtlmFlags & NTLM_FLAG_SIGN))
    {
        dwOptions |= NTLM_FLAG_ALWAYS_SIGN;
    }

    // We will not set the following flags:
    // NTLM_FLAG_LOCAL_CALL - Indicates local authentication which we do not
    //                        provide yet.  We will not use the local context
    // NTLM_FLAG_TYPE_SHARE - The authentication target is a network share (?).
    //                        Odd.
    dwOptions |= NTLM_FLAG_NTLM;

    // Data is checked and memory is allocated; fill in the structure
    //
    memcpy(
        &(pMessage->NtlmSignature),
        NTLM_NETWORK_SIGNATURE,
        NTLM_NETWORK_SIGNATURE_SIZE
        );

    pMessage->MessageType = NTLM_CHALLENGE_MSG;

    pMessage->NtlmFlags = dwOptions;

    if (pMessage->NtlmFlags & NTLM_FLAG_REQUEST_TARGET)
    {
        pMessage->Target.usLength = dwTargetNameSize;
        pMessage->Target.usMaxLength = pMessage->Target.usLength;
    }
    else
    {
        pMessage->Target.usLength = 0;
        pMessage->Target.usMaxLength = 0;
        pMessage->Target.dwOffset = 0;
    }

    memcpy(pMessage->Challenge,
            Challenge,
            NTLM_CHALLENGE_SIZE);

    // Main structure has been filled, now fill in optional data
    pBuffer = (PBYTE)pMessage + sizeof(NTLM_CHALLENGE_MESSAGE);

    if (pOsVersion || pMessage->NtlmFlags & NTLM_FLAG_TARGET_INFO)
    {
        // We have to fill in a local context which we will never use
        // ... so make it all zeros
        memset(pBuffer, 0, NTLM_LOCAL_CONTEXT_SIZE);
        pBuffer += NTLM_LOCAL_CONTEXT_SIZE;

        if (pMessage->NtlmFlags & NTLM_FLAG_TARGET_INFO)
        {
            pTargetInfoSecBuffer = (PNTLM_SEC_BUFFER)pBuffer;

            pTargetInfoSecBuffer->usLength = dwTargetInfoSize;
            pTargetInfoSecBuffer->usMaxLength = pTargetInfoSecBuffer->usLength;

        }

        // Always account for the size of the TargetInfoSecBuffer, even if it's
        // not filled in.  Otherwise, OS version info won't end up in the right
        // place.
        pBuffer += sizeof(NTLM_SEC_BUFFER);

        if (pOsVersion)
        {
            memcpy(pBuffer, pOsVersion, NTLM_WIN_SPOOF_SIZE);

            pBuffer += NTLM_WIN_SPOOF_SIZE;
        }
    }

    if (pMessage->Target.usLength)
    {
        pMessage->Target.dwOffset = pBuffer - (PBYTE)pMessage;

        if (pMessage->NtlmFlags & NTLM_FLAG_TYPE_DOMAIN)
        {
            pTrav = (PBYTE)pDomainName;
        }
        else
        {
            pTrav = (PBYTE)pServerName;
        }

        while (*pTrav)
        {
            *pBuffer = *pTrav;

            if (dwOptions & NTLM_FLAG_UNICODE)
            {
                pBuffer++;
            }
            pBuffer++;

            pTrav++;
        }
    }

    if (pTargetInfoSecBuffer)
    {
        // Remember, target block information is ALWAYS in UNICODE
        pTargetInfoSecBuffer->dwOffset = pBuffer - (PBYTE)pMessage;

        if (!LW_IS_NULL_OR_EMPTY_STR(pDomainName))
        {
            pTargetInfoBlock = (PNTLM_TARGET_INFO_BLOCK)pBuffer;

            pTargetInfoBlock->sLength = strlen(pDomainName) * sizeof(WCHAR);

            pTargetInfoBlock->sType = NTLM_TIB_DOMAIN_NAME;

            pTrav = (PBYTE)pDomainName;

            pBuffer += sizeof(NTLM_TARGET_INFO_BLOCK);

            while (*pTrav)
            {
                *pBuffer = *pTrav;

                pBuffer++;
                pBuffer++;

                pTrav++;
            }
        }

        if (!LW_IS_NULL_OR_EMPTY_STR(pServerName))
        {
            pTargetInfoBlock = (PNTLM_TARGET_INFO_BLOCK)pBuffer;

            pTargetInfoBlock->sLength = strlen(pServerName) * sizeof(WCHAR);

            pTargetInfoBlock->sType = NTLM_TIB_SERVER_NAME;

            pTrav = (PBYTE)pServerName;

            pBuffer += sizeof(NTLM_TARGET_INFO_BLOCK);

            while (*pTrav)
            {
                *pBuffer = *pTrav;

                pBuffer++;
                pBuffer++;

                pTrav++;
            }
        }

        if (!LW_IS_NULL_OR_EMPTY_STR(pDnsDomainName))
        {
            pTargetInfoBlock = (PNTLM_TARGET_INFO_BLOCK)pBuffer;

            pTargetInfoBlock->sLength = strlen(pDnsDomainName) * sizeof(WCHAR);

            pTargetInfoBlock->sType = NTLM_TIB_DNS_DOMAIN_NAME;

            pTrav = (PBYTE)pDnsDomainName;

            pBuffer += sizeof(NTLM_TARGET_INFO_BLOCK);

            while (*pTrav)
            {
                *pBuffer = *pTrav;

                pBuffer++;
                pBuffer++;

                pTrav++;
            }
        }

        if (!LW_IS_NULL_OR_EMPTY_STR(pDnsServerName))
        {
            pTargetInfoBlock = (PNTLM_TARGET_INFO_BLOCK)pBuffer;

            pTargetInfoBlock->sLength = strlen(pDnsServerName) * sizeof(WCHAR);

            pTargetInfoBlock->sType = NTLM_TIB_DNS_SERVER_NAME;

            pTrav = (PBYTE)pDnsServerName;

            pBuffer += sizeof(NTLM_TARGET_INFO_BLOCK);

            while (*pTrav)
            {
                *pBuffer = *pTrav;

                pBuffer++;
                pBuffer++;

                pTrav++;
            }
        }

        pTargetInfoBlock = (PNTLM_TARGET_INFO_BLOCK)pBuffer;
        pTargetInfoBlock->sLength = 0;
        pTargetInfoBlock->sType = NTLM_TIB_TERMINATOR;

        pBuffer += sizeof(*pTargetInfoBlock);
    }

    LW_ASSERT(pBuffer == (PBYTE)pMessage + dwSize);

cleanup:
    *pdwSize = dwSize;
    *ppChlngMsg = pMessage;
    return dwError;
error:
    dwSize = 0;
    LW_SAFE_FREE_MEMORY(pMessage);
    goto cleanup;
}

VOID
NtlmCopyStringToSecBuffer(
    IN PCSTR pszInput,
    IN DWORD dwFlags,
    IN PBYTE pBufferStart,
    IN OUT PBYTE* ppBufferPos,
    OUT PNTLM_SEC_BUFFER pSec
    )
{
    DWORD dwLen = 0;

    if (dwFlags & NTLM_FLAG_UNICODE)
    {
        dwLen = mbstrlen(pszInput) * sizeof(WCHAR);
        mbstowc16s((WCHAR*)*ppBufferPos, pszInput, dwLen/sizeof(WCHAR));
    }
    else
    {
        dwLen = strlen(pszInput);
        memcpy(*ppBufferPos, pszInput, dwLen);
    }

    pSec->usLength = dwLen;
    pSec->usMaxLength = dwLen;
    pSec->dwOffset = *ppBufferPos - pBufferStart;
    *ppBufferPos += dwLen;
}

DWORD
NtlmGetStringProtocolSize(
    DWORD dwFlags,
    PCSTR pszString
    )
{
    DWORD dwLen = strlen(pszString);

    if (dwFlags & NTLM_FLAG_UNICODE)
    {
        return dwLen * sizeof(WCHAR);
    }
    else
    {
        return dwLen;
    }
}

/******************************************************************************/
DWORD
NtlmCreateResponseMessage(
    IN PNTLM_CHALLENGE_MESSAGE pChlngMsg,
    IN PCSTR pUserName,
    IN PCSTR pDomainName,
    IN PCSTR pPassword,
    IN PBYTE pOsVersion,
    IN DWORD dwNtRespType,
    IN DWORD dwLmRespType,
    OUT PDWORD pdwSize,
    OUT PNTLM_RESPONSE_MESSAGE_V1* ppRespMsg,
    OUT PBYTE pLmUserSessionKey,
    OUT PBYTE pNtlmUserSessionKey
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwSize = 0;
    PNTLM_RESPONSE_MESSAGE_V3 pMessage = NULL;
    DWORD dwNtMsgSize = 0;
    DWORD dwLmMsgSize = 0;
    PBYTE pNtMsg = NULL;
    PBYTE pLmMsg = NULL;
    CHAR pWorkstation[HOST_NAME_MAX];
    // The following pointers point into pMessage and will not be freed on error
    PBYTE pBuffer = NULL;

    // sanity checks
    if (!pChlngMsg || !ppRespMsg)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = gethostname(pWorkstation, HOST_NAME_MAX);
    if (dwError)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppRespMsg = NULL;
    *pdwSize = 0;

    dwSize += sizeof(*pMessage);

    dwSize += NtlmGetStringProtocolSize(
                pChlngMsg->NtlmFlags,
                pDomainName);
    dwSize += NtlmGetStringProtocolSize(
                pChlngMsg->NtlmFlags,
                pUserName);
    dwSize += NtlmGetStringProtocolSize(
                pChlngMsg->NtlmFlags,
                pWorkstation);

    if (dwLmRespType == NTLM_RESPONSE_TYPE_NTLM2)
    {
        // The LM session key is not used in this case
        memset(pLmUserSessionKey, 0, NTLM_SESSION_KEY_SIZE);

        dwError = NtlmBuildNtlm2Response(
            pChlngMsg->Challenge,
            pPassword,
            &dwLmMsgSize,
            &pLmMsg,
            &dwNtMsgSize,
            &pNtMsg,
            pNtlmUserSessionKey);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwError = NtlmBuildResponse(
            pChlngMsg,
            pUserName,
            pPassword,
            dwLmRespType,
            &dwLmMsgSize,
            pLmUserSessionKey,
            &pLmMsg
            );
        BAIL_ON_LSA_ERROR(dwError);

        dwError = NtlmBuildResponse(
            pChlngMsg,
            pUserName,
            pPassword,
            dwNtRespType,
            &dwNtMsgSize,
            pNtlmUserSessionKey,
            &pNtMsg
            );
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwSize += dwNtMsgSize;
    dwSize += dwLmMsgSize;

    if (pChlngMsg->NtlmFlags & NTLM_FLAG_KEY_EXCH)
    {
        dwSize += NTLM_SESSION_KEY_SIZE;
    }

    dwError = LwAllocateMemory(dwSize, OUT_PPVOID(&pMessage));
    BAIL_ON_LSA_ERROR(dwError);

    // Data is checked and memory is allocated; fill in the structure
    //
    memcpy(
        &pMessage->NtlmSignature,
        NTLM_NETWORK_SIGNATURE,
        NTLM_NETWORK_SIGNATURE_SIZE);

    pMessage->MessageType = NTLM_RESPONSE_MSG;

    pMessage->LmResponse.usLength = dwLmMsgSize;
    pMessage->LmResponse.usMaxLength = pMessage->LmResponse.usLength;

    pMessage->NtResponse.usLength = dwNtMsgSize;
    pMessage->NtResponse.usMaxLength = pMessage->NtResponse.usLength;

    pMessage->Flags = pChlngMsg->NtlmFlags;

    memcpy(&pMessage->Version, pOsVersion, NTLM_WIN_SPOOF_SIZE);

    // We've filled in the main structure, now add the data for the sec buffers
    // at the end.
    pBuffer = (PBYTE)pMessage + sizeof(*pMessage);

    pMessage->LmResponse.dwOffset = pBuffer - (PBYTE)pMessage;
    memcpy(pBuffer, pLmMsg, dwLmMsgSize);

    pBuffer += pMessage->LmResponse.usLength;

    pMessage->NtResponse.dwOffset = pBuffer - (PBYTE)pMessage;
    memcpy(pBuffer, pNtMsg, dwNtMsgSize);

    pBuffer += pMessage->NtResponse.usLength;

    NtlmCopyStringToSecBuffer(
        pDomainName,
        pChlngMsg->NtlmFlags,
        (PBYTE)pMessage,
        &pBuffer,
        &pMessage->AuthTargetName);
    NtlmCopyStringToSecBuffer(
        pUserName,
        pChlngMsg->NtlmFlags,
        (PBYTE)pMessage,
        &pBuffer,
        &pMessage->UserName);
    NtlmCopyStringToSecBuffer(
        pWorkstation,
        pChlngMsg->NtlmFlags,
        (PBYTE)pMessage,
        &pBuffer,
        &pMessage->Workstation);

    pMessage->SessionKey.usLength = 0;
    if (pChlngMsg->NtlmFlags & NTLM_FLAG_KEY_EXCH)
    {
        // we won't fill this in until after we return from this function, but
        // we'll save space to fill in later
        pMessage->SessionKey.usLength = NTLM_SESSION_KEY_SIZE;
    }
    pMessage->SessionKey.usMaxLength = pMessage->SessionKey.usLength;
    pMessage->SessionKey.dwOffset = pBuffer - (PBYTE)pMessage;

    // This is only a partial validation since we may be adding a session key
    // to this message.
    LW_ASSERT(pBuffer + pMessage->SessionKey.usLength == (PBYTE)pMessage + dwSize);

cleanup:
    LW_SAFE_FREE_MEMORY(pNtMsg);
    LW_SAFE_FREE_MEMORY(pLmMsg);
    *ppRespMsg = &pMessage->V1;
    *pdwSize = dwSize;
    return dwError;
error:
    LW_SAFE_FREE_MEMORY(pMessage);
    dwSize = 0;
    goto cleanup;
}

/******************************************************************************/
VOID
NtlmStoreSecondaryKey(
    IN PBYTE pMasterKey,
    IN PBYTE pSecondaryKey,
    IN OUT PNTLM_RESPONSE_MESSAGE_V1 pMessage
    )
{
    PNTLM_RESPONSE_MESSAGE_V2 pV2Message = (PNTLM_RESPONSE_MESSAGE_V2)pMessage;
    BYTE EncryptedKey[NTLM_SESSION_KEY_SIZE] = {0};
    PBYTE pSessionKeyData = NULL;
    RC4_KEY Rc4Key;

    // Encrypt the secondary key with the master key
    memset(&Rc4Key, 0, sizeof(Rc4Key));

    RC4_set_key(&Rc4Key, NTLM_SESSION_KEY_SIZE, pMasterKey);
    RC4(&Rc4Key, NTLM_SESSION_KEY_SIZE, pSecondaryKey, EncryptedKey);

    pSessionKeyData = pV2Message->SessionKey.dwOffset + (PBYTE)pMessage;

    memcpy(pSessionKeyData, EncryptedKey, pV2Message->SessionKey.usLength);
}

/******************************************************************************/
VOID
NtlmWeakenSessionKey(
    IN PNTLM_CHALLENGE_MESSAGE pChlngMsg,
    IN OUT PBYTE pMasterKey,
    OUT PDWORD pcbKeyLength
    )
{
    // Only weaken the key if LanManagerSessionKey was used
    if (pChlngMsg->NtlmFlags & NTLM_FLAG_LM_KEY)
    {
        if (pChlngMsg->NtlmFlags & NTLM_FLAG_56)
        {
            pMasterKey[7] = 0xa0;
            memset(&pMasterKey[8], 0, 8);

            // key strength is 56 bits
            *pcbKeyLength = 7;
        }
        else if (!(pChlngMsg->NtlmFlags & NTLM_FLAG_128))
        {
            pMasterKey[5] = 0xe5;
            pMasterKey[6] = 0x38;
            pMasterKey[7] = 0xb0;
            memset(&pMasterKey[8], 0, 8);

            // key strength is 40 bits
            *pcbKeyLength = 5;
        }
        else
        {
            // key strength is 128 bits
            *pcbKeyLength = 16;
        }
    }
    else
    {
        // key weakening is only used if the LanManagerSessionKey is used.
        // this key is full strength... 128 bits
        *pcbKeyLength = 16;
    }
}

/******************************************************************************/
DWORD
NtlmGetAuthTargetNameFromChallenge(
    IN PNTLM_CHALLENGE_MESSAGE pChlngMsg,
    OUT PCHAR* ppAuthTargetName
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PCHAR pName = NULL;
    DWORD dwNameLength = 0;
    DWORD nIndex = 0;
    // The following pointers point into pChlngMsg and will not be freed
    PBYTE pBuffer = NULL;
    PNTLM_SEC_BUFFER pTargetSecBuffer = &pChlngMsg->Target;

    *ppAuthTargetName = NULL;

    dwNameLength = pTargetSecBuffer->usLength;
    pBuffer = pTargetSecBuffer->dwOffset + (PBYTE)pChlngMsg;

    if (pChlngMsg->NtlmFlags & NTLM_FLAG_OEM)
    {
        dwError = LwAllocateMemory(dwNameLength + 1, OUT_PPVOID(&pName));
        BAIL_ON_LSA_ERROR(dwError);

        memcpy(pName, pBuffer, dwNameLength);
    }
    else
    {
        dwNameLength = dwNameLength / sizeof(WCHAR);

        dwError = LwAllocateMemory(dwNameLength + 1, OUT_PPVOID(&pName));
        BAIL_ON_LSA_ERROR(dwError);

        for (nIndex = 0; nIndex < dwNameLength; nIndex++)
        {
            pName[nIndex] = pBuffer[nIndex * sizeof(WCHAR)];
        }
    }

cleanup:
    *ppAuthTargetName = pName;
    return dwError;
error:
    LW_SAFE_FREE_STRING(pName);
    goto cleanup;
}

/******************************************************************************/
DWORD
NtlmBuildResponse(
    IN PNTLM_CHALLENGE_MESSAGE pChlngMsg,
    IN PCSTR pUserName,
    IN PCSTR pPassword,
    IN DWORD dwResponseType,
    OUT PDWORD pdwBufferSize,
    OUT PBYTE pUserSessionKey,
    OUT PBYTE* ppBuffer
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    if (!pChlngMsg)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    switch (dwResponseType)
    {
    case NTLM_RESPONSE_TYPE_LM:
        dwError = NtlmBuildLmResponse(
            pChlngMsg,
            pPassword,
            pdwBufferSize,
            pUserSessionKey,
            ppBuffer
            );
        BAIL_ON_LSA_ERROR(dwError);
        break;
    case NTLM_RESPONSE_TYPE_LMv2:
        dwError = NtlmBuildLmV2Response();
        BAIL_ON_LSA_ERROR(dwError);
        break;
    case NTLM_RESPONSE_TYPE_NTLM:
        dwError = NtlmBuildNtlmResponse(
            pChlngMsg,
            pPassword,
            pdwBufferSize,
            pUserSessionKey,
            ppBuffer
            );
        BAIL_ON_LSA_ERROR(dwError);
        break;
    case NTLM_RESPONSE_TYPE_NTLMv2:
        dwError = NtlmBuildNtlmV2Response(
            pChlngMsg,
            pUserName,
            pPassword,
            pdwBufferSize,
            pUserSessionKey,
            ppBuffer
            );
        BAIL_ON_LSA_ERROR(dwError);
        break;
    case NTLM_RESPONSE_TYPE_ANONYMOUS:
        {
            dwError = NtlmBuildAnonymousResponse();
            BAIL_ON_LSA_ERROR(dwError);
        }
        break;
    default:
        {
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

cleanup:
    return dwError;
error:
    goto cleanup;
}

/******************************************************************************/
DWORD
NtlmBuildLmResponse(
    IN PNTLM_CHALLENGE_MESSAGE pChlngMsg,
    IN PCSTR pPassword,
    OUT PDWORD pdwResponseSize,
    OUT PBYTE pUserSessionKey,
    OUT PBYTE* ppResponse
    )
{
    DWORD dwError = 0;
    DWORD dwIndex = 0;
    DWORD dwPasswordLen = 0;
    BYTE LmHash[NTLM_HASH_SIZE] = {0};
    ULONG64 ulKey1 = 0;
    ULONG64 ulKey2 = 0;
    ULONG64 ulKey3 = 0;
    DES_key_schedule DesKeySchedule;
    PBYTE pResponse = NULL;

    memset(&DesKeySchedule, 0, sizeof(DES_key_schedule));

    dwError = LwAllocateMemory(NTLM_RESPONSE_SIZE_LM, OUT_PPVOID(&pResponse));
    BAIL_ON_LSA_ERROR(dwError);

    memset(pResponse, 0, NTLM_RESPONSE_SIZE_LM);
    memset(pUserSessionKey, 0, NTLM_SESSION_KEY_SIZE);

    dwPasswordLen = strlen(pPassword);

    // if password is less than 15 characters, create LM hash, otherwise, the
    // hash is set to zero.
    if (dwPasswordLen <= NTLM_LM_MAX_PASSWORD_SIZE)
    {
        // convert the password to upper case
        for (dwIndex = 0; dwIndex < NTLM_LM_MAX_PASSWORD_SIZE; dwIndex++)
        {
            LmHash[dwIndex] = toupper((int)pPassword[dwIndex]);

            if (!pPassword[dwIndex])
            {
                break;
            }
        }

        ulKey1 = NtlmCreateKeyFromHash(&LmHash[0], 7);
        ulKey2 = NtlmCreateKeyFromHash(&LmHash[7], 7);

        DES_set_key_unchecked((const_DES_cblock*)&ulKey1, &DesKeySchedule);
        DES_ecb_encrypt(
            (const_DES_cblock *)NTLM_LM_DES_STRING,
            (DES_cblock*)&LmHash[0],
            &DesKeySchedule,
            DES_ENCRYPT
            );

        DES_set_key_unchecked((const_DES_cblock*)&ulKey2, &DesKeySchedule);
        DES_ecb_encrypt(
            (const_DES_cblock *)NTLM_LM_DES_STRING,
            (DES_cblock*)&LmHash[8],
            &DesKeySchedule,
            DES_ENCRYPT
            );
    }

    // The LM user session is... surprisingly... just the first half of the hash
    // we just generated padded out to 16 bytes.
    memcpy(pUserSessionKey, LmHash, 8);

    ulKey1 = NtlmCreateKeyFromHash(&LmHash[0],  7);
    ulKey2 = NtlmCreateKeyFromHash(&LmHash[7],  7);
    ulKey3 = NtlmCreateKeyFromHash(&LmHash[14], 2);

    DES_set_key_unchecked((const_DES_cblock*)&ulKey1, &DesKeySchedule);
    DES_ecb_encrypt(
        (const_DES_cblock *)pChlngMsg->Challenge,
        (DES_cblock*)&pResponse[0],
        &DesKeySchedule,
        DES_ENCRYPT
        );

    DES_set_key_unchecked((const_DES_cblock*)&ulKey2, &DesKeySchedule);
    DES_ecb_encrypt(
        (const_DES_cblock *)pChlngMsg->Challenge,
        (DES_cblock*)&pResponse[8],
        &DesKeySchedule,
        DES_ENCRYPT
        );

    DES_set_key_unchecked((const_DES_cblock*)&ulKey3, &DesKeySchedule);
    DES_ecb_encrypt(
        (const_DES_cblock *)pChlngMsg->Challenge,
        (DES_cblock*)&pResponse[16],
        &DesKeySchedule,
        DES_ENCRYPT
        );

    *pdwResponseSize = NTLM_RESPONSE_SIZE_LM;
    *ppResponse = pResponse;

cleanup:

    return dwError;

error:
    *pdwResponseSize = 0;
    *ppResponse = NULL;
    LW_SAFE_FREE_MEMORY(pResponse);
    goto cleanup;
}

/******************************************************************************/
DWORD
NtlmBuildNtlmResponse(
    IN PNTLM_CHALLENGE_MESSAGE pChlngMsg,
    IN PCSTR pPassword,
    OUT PDWORD pdwResponseSize,
    OUT PBYTE pUserSessionKey,
    OUT PBYTE* ppResponse
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    BYTE NtlmHash[MD4_DIGEST_LENGTH] = {0};
    ULONG64 ulKey1 = 0;
    ULONG64 ulKey2 = 0;
    ULONG64 ulKey3 = 0;
    DES_key_schedule DesKeySchedule;
    PBYTE pResponse = NULL;

    memset(&DesKeySchedule, 0, sizeof(DES_key_schedule));

    dwError = LwAllocateMemory(NTLM_RESPONSE_SIZE_LM, OUT_PPVOID(&pResponse));
    BAIL_ON_LSA_ERROR(dwError);

    memset(pResponse, 0, NTLM_RESPONSE_SIZE_NTLM);
    memset(pUserSessionKey, 0, NTLM_SESSION_KEY_SIZE);

    NtlmCreateNtlmV1Hash(pPassword, NtlmHash);

    // Generate the session key
    dwError = NtlmCreateMD4Digest(
        NtlmHash,
        MD4_DIGEST_LENGTH,
        pUserSessionKey
        );
    BAIL_ON_LSA_ERROR(dwError);

    ulKey1 = NtlmCreateKeyFromHash(&NtlmHash[0], 7);
    ulKey2 = NtlmCreateKeyFromHash(&NtlmHash[7], 7);
    ulKey3 = NtlmCreateKeyFromHash(&NtlmHash[14], 2);

    DES_set_key_unchecked((const_DES_cblock*)&ulKey1, &DesKeySchedule);
    DES_ecb_encrypt(
        (const_DES_cblock *)pChlngMsg->Challenge,
        (DES_cblock*)&pResponse[0],
        &DesKeySchedule,
        DES_ENCRYPT
        );

    DES_set_key_unchecked((const_DES_cblock*)&ulKey2, &DesKeySchedule);
    DES_ecb_encrypt(
        (const_DES_cblock *)pChlngMsg->Challenge,
        (DES_cblock*)&pResponse[8],
        &DesKeySchedule,
        DES_ENCRYPT
        );

    DES_set_key_unchecked((const_DES_cblock*)&ulKey3, &DesKeySchedule);
    DES_ecb_encrypt(
        (const_DES_cblock *)pChlngMsg->Challenge,
        (DES_cblock*)&pResponse[16],
        &DesKeySchedule,
        DES_ENCRYPT
        );

    *pdwResponseSize = NTLM_RESPONSE_SIZE_LM;
    *ppResponse = pResponse;

cleanup:
    return dwError;

error:
    memset(pUserSessionKey, 0, NTLM_SESSION_KEY_SIZE);
    *pdwResponseSize = 0;
    *ppResponse = NULL;
    LW_SAFE_FREE_MEMORY(pResponse);
    goto cleanup;
}

/******************************************************************************/
DWORD
NtlmBuildNtlmV2Response(
    IN PNTLM_CHALLENGE_MESSAGE pChlngMsg,
    IN PCSTR pUserName,
    IN PCSTR pPassword,
    OUT PDWORD pdwResponseSize,
    OUT PBYTE pUserSessionKey,
    OUT PBYTE* ppResponse
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    BYTE NtlmHashV1[MD4_DIGEST_LENGTH] = {0};
    BYTE NtlmHashV2[MD4_DIGEST_LENGTH] = {0};
    PSTR pTarget = NULL;
    DWORD dwKeyLen = NTLM_HASH_SIZE;
    PBYTE pResponse = NULL;
    DWORD dwResponseSize = 0;

    memset(pUserSessionKey, 0, NTLM_SESSION_KEY_SIZE);

    dwError = NtlmCreateNtlmV1Hash(pPassword, NtlmHashV1);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = NtlmGetAuthTargetNameFromChallenge(pChlngMsg, &pTarget);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = NtlmCreateNtlmV2Hash(pUserName, pTarget, NtlmHashV1, NtlmHashV2);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = NtlmCreateNtlmV2Blob(pChlngMsg, NtlmHashV2, &dwResponseSize, &pResponse);
    BAIL_ON_LSA_ERROR(dwError);

    // Generate the session key which is just the first 16 bytes of the response
    // HMAC md5 encrypted using the NTLMv2 hash as the key
    HMAC(
        EVP_md5(),
        NtlmHashV2,
        NTLM_HASH_SIZE,
        pResponse,
        NTLM_HASH_SIZE,
        pUserSessionKey,
        &dwKeyLen);

    *pdwResponseSize = dwResponseSize;
    *ppResponse = pResponse;

cleanup:
    LW_SAFE_FREE_MEMORY(pTarget);

    return dwError;
error:
    *pdwResponseSize = 0;
    *ppResponse = NULL;
    LW_SAFE_FREE_MEMORY(pResponse);
    memset(pUserSessionKey, 0, NTLM_SESSION_KEY_SIZE);
    goto cleanup;
}

/******************************************************************************/
DWORD
NtlmCreateNtlmV2Blob(
    IN PNTLM_CHALLENGE_MESSAGE pChlngMsg,
    IN BYTE NtlmHashV2[MD4_DIGEST_LENGTH],
    OUT PDWORD pdwSize,
    OUT PBYTE* ppBlob
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    BYTE BlobSignature[NTLM_BLOB_SIGNATURE_SIZE] = NTLM_BLOB_SIGNATURE;
    PBYTE pOriginal = NULL;
    DWORD dwBlobSize = 0;
    BYTE TempBlobHash[EVP_MAX_MD_SIZE] = {0};
    DWORD dwKeyLen = NTLM_HASH_SIZE;
    // The following pointers point into pOriginal and will not be freed
    PNTLM_BLOB pNtlmBlob = NULL;
    PBYTE pBuffer = NULL;
    // The following Pointers point into pChlngMsg and will not be freed
    PBYTE pChallenge = NULL;
    PNTLM_SEC_BUFFER pTargetInfo = NULL;
    PBYTE pTargetBuffer = NULL;

    *ppBlob = NULL;
    *pdwSize = 0;

    pChallenge = pChlngMsg->Challenge;

    pTargetInfo =
        (PNTLM_SEC_BUFFER)
            ((PBYTE)pChlngMsg +
            sizeof(NTLM_CHALLENGE_MESSAGE) +
            NTLM_LOCAL_CONTEXT_SIZE);

    pTargetBuffer = (PBYTE)pChlngMsg + pTargetInfo->dwOffset;

    // We're going to allocate the entire blob at this point and just use
    // portions of it as needed.
    dwBlobSize =
        NTLM_HASH_SIZE +
        sizeof(NTLM_BLOB) +
        pTargetInfo->usLength +
        NTLM_BLOB_TRAILER_SIZE;

    dwError = LwAllocateMemory(dwBlobSize, OUT_PPVOID(&pOriginal));
    BAIL_ON_LSA_ERROR(dwError);

    // the beginning of the blob will contain the final hash value, so push
    // the blob pointer up by that amount
    pNtlmBlob = (PNTLM_BLOB)((PBYTE)pOriginal + NTLM_HASH_SIZE);

    memcpy(
        pNtlmBlob->NtlmBlobSignature,
        &BlobSignature[0],
        NTLM_BLOB_SIGNATURE_SIZE);

    pNtlmBlob->Reserved1 = 0;

    pNtlmBlob->TimeStamp = (ULONG64)time(NULL);
    pNtlmBlob->TimeStamp += 11644473600ULL;
    pNtlmBlob->TimeStamp *= 10000000ULL;

    dwError = NtlmGetRandomBuffer(
        (PBYTE)&pNtlmBlob->ClientNonce,
        sizeof(pNtlmBlob->ClientNonce));

    pNtlmBlob->Reserved2 = 0;

    // Copy in the target info buffer
    pBuffer = (PBYTE)pNtlmBlob + sizeof(NTLM_BLOB);
    memcpy(pBuffer, pTargetBuffer, pTargetInfo->usLength);

    // The last 4 bytes should never have changed so they should still be 0

    // Now prepend the challenge and encrypt
    pBuffer = *(PBYTE*)&pNtlmBlob - NTLM_CHALLENGE_SIZE;
    memcpy(pBuffer, pChallenge, NTLM_CHALLENGE_SIZE);

    HMAC(
        EVP_md5(),
        NtlmHashV2,
        NTLM_HASH_SIZE,
        pBuffer,
        dwBlobSize - (NTLM_HASH_SIZE / 2),
        TempBlobHash,
        &dwKeyLen);

    memcpy(pOriginal, TempBlobHash, NTLM_HASH_SIZE);

cleanup:
    *ppBlob = pOriginal;
    *pdwSize = dwBlobSize;
    return dwError;
error:
    LW_SAFE_FREE_MEMORY(pOriginal);
    goto cleanup;
}

    /******************************************************************************/
DWORD
NtlmBuildLmV2Response(
    VOID
    )
{
    DWORD dwError = LW_ERROR_NOT_SUPPORTED;
    return dwError;
}

/******************************************************************************/
DWORD
NtlmBuildNtlm2Response(
    UCHAR ServerChallenge[8],
    PCSTR pPassword,
    PDWORD pdwLmRespSize,
    PBYTE* ppLmResp,
    PDWORD pdwNtRespSize,
    PBYTE* ppNtResp,
    BYTE pUserSessionKey[NTLM_SESSION_KEY_SIZE]
    )
{
    DWORD dwError = ERROR_SUCCESS;
    BYTE clientNonce[8];
    DWORD dwLmRespSize = 0;
    PBYTE pLmResp = NULL;
    DWORD dwNtRespSize = 0;
    PBYTE pNtResp = NULL;
    BYTE sessionNonce[MD5_DIGEST_LENGTH];
    // This array is 16 bytes long, but only the first 8 bytes are used for the
    // NTLM2 Session Hash
    BYTE sessionHashUntrunc[MD5_DIGEST_LENGTH];
    DES_key_schedule DesKeySchedule;
    BYTE NtlmHash[MD4_DIGEST_LENGTH] = {0};
    ULONG64 ulKey1 = 0;
    ULONG64 ulKey2 = 0;
    ULONG64 ulKey3 = 0;
    BYTE NtlmUserSessionKey[NTLM_SESSION_KEY_SIZE];

    memset(&DesKeySchedule, 0, sizeof(DES_key_schedule));

    dwError = NtlmGetRandomBuffer(
        clientNonce,
        sizeof(clientNonce));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = NtlmCreateNtlmV1Hash(pPassword, NtlmHash);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = NtlmCreateMD4Digest(
        NtlmHash,
        MD4_DIGEST_LENGTH,
        NtlmUserSessionKey);
    BAIL_ON_LSA_ERROR(dwError);

    dwLmRespSize = 24;
    dwError = LwAllocateMemory(dwLmRespSize, OUT_PPVOID(&pLmResp));
    BAIL_ON_LSA_ERROR(dwError);

    // Leave the last 16 bytes of the Lm Response as null
    memcpy(pLmResp, clientNonce, sizeof(clientNonce));

    // Calculate the session nonce first
    memcpy(sessionNonce + 0, ServerChallenge, 8);
    memcpy(sessionNonce + 8, clientNonce, 8);

    MD5(sessionNonce, 16, sessionHashUntrunc);

    dwNtRespSize = 24;
    dwError = LwAllocateMemory(dwNtRespSize, OUT_PPVOID(&pNtResp));
    BAIL_ON_LSA_ERROR(dwError);

    ulKey1 = NtlmCreateKeyFromHash(&NtlmHash[0], 7);
    ulKey2 = NtlmCreateKeyFromHash(&NtlmHash[7], 7);
    ulKey3 = NtlmCreateKeyFromHash(&NtlmHash[14], 2);

    DES_set_key_unchecked((const_DES_cblock*)&ulKey1, &DesKeySchedule);
    DES_ecb_encrypt(
        (const_DES_cblock *)sessionHashUntrunc,
        (DES_cblock*)&pNtResp[0],
        &DesKeySchedule,
        DES_ENCRYPT
        );

    DES_set_key_unchecked((const_DES_cblock*)&ulKey2, &DesKeySchedule);
    DES_ecb_encrypt(
        (const_DES_cblock *)sessionHashUntrunc,
        (DES_cblock*)&pNtResp[8],
        &DesKeySchedule,
        DES_ENCRYPT
        );

    DES_set_key_unchecked((const_DES_cblock*)&ulKey3, &DesKeySchedule);
    DES_ecb_encrypt(
        (const_DES_cblock *)sessionHashUntrunc,
        (DES_cblock*)&pNtResp[16],
        &DesKeySchedule,
        DES_ENCRYPT
        );

    HMAC(
        EVP_md5(),
        NtlmUserSessionKey,
        NTLM_SESSION_KEY_SIZE,
        sessionNonce,
        16,
        pUserSessionKey,
        NULL);

    *pdwLmRespSize = dwLmRespSize;
    *ppLmResp = pLmResp;
    *pdwNtRespSize = dwNtRespSize;
    *ppNtResp = pNtResp;

cleanup:
    return dwError;

error:
    *pdwLmRespSize = 0;
    *ppLmResp = NULL;
    *pdwNtRespSize = 0;
    *ppNtResp = NULL;
    LW_SAFE_FREE_MEMORY(pLmResp);
    LW_SAFE_FREE_MEMORY(pNtResp);
    memset(pUserSessionKey, 0, NTLM_SESSION_KEY_SIZE);

    goto cleanup;
}

/******************************************************************************/
DWORD
NtlmBuildAnonymousResponse(
    VOID
    )
{
    DWORD dwError = LW_ERROR_NOT_SUPPORTED;
    return dwError;
}

/******************************************************************************/
DWORD
NtlmGetUserNameFromResponse(
    IN PNTLM_RESPONSE_MESSAGE_V1 pRespMsg,
    IN BOOLEAN bUnicode,
    OUT PSTR* ppUserName
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PCHAR pName = NULL;
    DWORD dwNameLength = 0;
    PBYTE pBuffer = NULL;
    PNTLM_SEC_BUFFER pSecBuffer = &pRespMsg->UserName;
    DWORD nIndex = 0;

    *ppUserName = NULL;

    dwNameLength = pSecBuffer->usLength;
    pBuffer = pSecBuffer->dwOffset + (PBYTE)pRespMsg;

    if (!bUnicode)
    {
        dwError = LwAllocateMemory(dwNameLength + 1, OUT_PPVOID(&pName));
        BAIL_ON_LSA_ERROR(dwError);

        memcpy(pName, pBuffer, dwNameLength);
    }
    else
    {
        dwNameLength = dwNameLength / sizeof(WCHAR);

        dwError = LwAllocateMemory(dwNameLength + 1, OUT_PPVOID(&pName));
        BAIL_ON_LSA_ERROR(dwError);

        for (nIndex = 0; nIndex < dwNameLength; nIndex++)
        {
            pName[nIndex] = pBuffer[nIndex * sizeof(WCHAR)];
        }
    }

cleanup:
    *ppUserName = pName;
    return dwError;
error:
    LW_SAFE_FREE_STRING(pName);
    goto cleanup;
}

/******************************************************************************/
DWORD
NtlmCreateNtlmV1Hash(
    PCSTR pPassword,
    BYTE Hash[MD4_DIGEST_LENGTH]
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwTempPassSize = strlen(pPassword) * sizeof(WCHAR);
    PWCHAR pwcTempPass = NULL;

    memset(Hash, 0, MD4_DIGEST_LENGTH);

    dwError = LwAllocateMemory(dwTempPassSize, OUT_PPVOID(&pwcTempPass));
    BAIL_ON_LSA_ERROR(dwError);

    while (*pPassword)
    {
        *pwcTempPass = *pPassword;
        pwcTempPass++;
        pPassword++;
    }

    pwcTempPass = (PWCHAR)((PBYTE)pwcTempPass - dwTempPassSize);

    dwError = NtlmCreateMD4Digest(
        (PBYTE)pwcTempPass,
        dwTempPassSize,
        Hash
        );
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LW_SAFE_FREE_MEMORY(pwcTempPass);
    return dwError;
error:
    memset(Hash, 0, MD4_DIGEST_LENGTH);
    goto cleanup;
}

/******************************************************************************/
DWORD
NtlmCreateNtlmV2Hash(
    PCSTR pUserName,
    PCSTR pDomain,
    BYTE NtlmV1Hash[MD4_DIGEST_LENGTH],
    BYTE NtlmV2Hash[MD4_DIGEST_LENGTH]
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwTempBufferSize = 0;
    PWSTR pTempBuffer = NULL;
    DWORD dwKeySize = NTLM_HASH_SIZE;
    // The following pointers point into pTempBuffer and will not be freed
    PWCHAR pTrav = NULL;

    memset(NtlmV2Hash, 0, MD4_DIGEST_LENGTH);

    dwTempBufferSize = (strlen(pDomain) + strlen(pUserName)) * sizeof(WCHAR);

    dwError = LwAllocateMemory(dwTempBufferSize, OUT_PPVOID(&pTempBuffer));
    BAIL_ON_LSA_ERROR(dwError);

    pTrav = pTempBuffer;

    // remember that the user name is converted to upper case
    while (*pUserName)
    {
        *pTrav = toupper((int)*pUserName);
        pTrav++;
        pUserName++;
    }

    while (*pDomain)
    {
        *pTrav = *pDomain;
        pTrav++;
        pDomain++;
    }

    HMAC(
        EVP_md5(),
        NtlmV1Hash,
        MD4_DIGEST_LENGTH,
        (PBYTE)pTempBuffer,
        dwTempBufferSize,
        NtlmV2Hash,
        &dwKeySize);

cleanup:
    LW_SAFE_FREE_MEMORY(pTempBuffer);
    return dwError;
error:
    memset(NtlmV2Hash, 0, MD4_DIGEST_LENGTH);
    goto cleanup;
}

/******************************************************************************/
VOID
NtlmGenerateLanManagerSessionKey(
    IN PNTLM_RESPONSE_MESSAGE_V1 pMessage,
    IN PBYTE pLmUserSessionKey,
    OUT PBYTE pLanManagerSessionKey
    )
{
    ULONG64 ulKey1 = 0;
    ULONG64 ulKey2 = 0;
    BYTE KeyBuffer[NTLM_SESSION_KEY_SIZE] = {0};
    DES_key_schedule DesKeySchedule;
    // The following pointers point into pMessage and will not be freed
    PNTLM_SEC_BUFFER pLmSecBuffer = NULL;
    PBYTE pLmResponse = NULL;

    memset(&DesKeySchedule, 0, sizeof(DES_key_schedule));

    pLmSecBuffer = &pMessage->LmResponse;
    pLmResponse = (PBYTE)pMessage + pLmSecBuffer->dwOffset;

    memcpy(KeyBuffer, pLmUserSessionKey, 8);
    memset(&KeyBuffer[8], 0xbd, 6);

    ulKey1 = NtlmCreateKeyFromHash(&KeyBuffer[0], 7);
    ulKey2 = NtlmCreateKeyFromHash(&KeyBuffer[7], 7);

    DES_set_key_unchecked((const_DES_cblock*)&ulKey1, &DesKeySchedule);
    DES_ecb_encrypt(
        (const_DES_cblock *)pLmResponse,
        (DES_cblock*)&pLanManagerSessionKey[0],
        &DesKeySchedule,
        DES_ENCRYPT
        );

    DES_set_key_unchecked((const_DES_cblock*)&ulKey2, &DesKeySchedule);
    DES_ecb_encrypt(
        (const_DES_cblock *)pLmResponse,
        (DES_cblock*)&pLanManagerSessionKey[8],
        &DesKeySchedule,
        DES_ENCRYPT
        );
}

/******************************************************************************/
DWORD
NtlmCreateMD4Digest(
    IN PBYTE pBuffer,
    IN DWORD dwBufferLen,
    OUT BYTE MD4Digest[MD4_DIGEST_LENGTH]
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    MD4_CTX Md4Ctx;

    dwError = MD4_Init(&Md4Ctx);

    if (dwError != 1)
    {
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = MD4_Update(&Md4Ctx, pBuffer, dwBufferLen);

    if (dwError != 1)
    {
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = MD4_Final(MD4Digest, &Md4Ctx);

    if (dwError != 1)
    {
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LW_ERROR_SUCCESS;

cleanup:
    return dwError;
error:
    goto cleanup;
}

/******************************************************************************/
ULONG64
NtlmCreateKeyFromHash(
    IN PBYTE pBuffer,
    IN DWORD dwLength
    )
{
    ULONG64 Key = 0;
    DWORD nIndex = 0;

    LW_ASSERT(dwLength <= 7);

    for (nIndex = 0; nIndex < dwLength; nIndex++)
    {
        ((PBYTE)(&Key))[6 - nIndex] = pBuffer[nIndex];
    }

    NtlmSetParityBit(&Key);

    Key = LW_ENDIAN_SWAP64(Key);

    return Key;
}

/******************************************************************************/
VOID
NtlmSetParityBit(
    IN OUT PULONG64 pKey
    )
{
    ULONG64 NewKey = *pKey;

    NewKey = NewKey << 1;

    NewKey = (NewKey & 0x00000000000000FFULL)|
            ((NewKey & 0xFFFFFFFFFFFFFF00ULL) << 1);

    NewKey = (NewKey & 0x000000000000FFFFULL)|
            ((NewKey & 0xFFFFFFFFFFFF0000ULL) << 1);

    NewKey = (NewKey & 0x0000000000FFFFFFULL)|
            ((NewKey & 0xFFFFFFFFFF000000ULL) << 1);

    NewKey = (NewKey & 0x00000000FFFFFFFFULL)|
            ((NewKey & 0xFFFFFFFF00000000ULL) << 1);

    NewKey = (NewKey & 0x000000FFFFFFFFFFULL)|
            ((NewKey & 0xFFFFFF0000000000ULL) << 1);

    NewKey = (NewKey & 0x0000FFFFFFFFFFFFULL)|
            ((NewKey & 0xFFFF000000000000ULL) << 1);

    NewKey = (NewKey & 0x00FFFFFFFFFFFFFFULL)|
            ((NewKey & 0xFF00000000000000ULL) << 1);

    DES_set_odd_parity((DES_cblock*)&NewKey);

    *pKey = NewKey;
}
