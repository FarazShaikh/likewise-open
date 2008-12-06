/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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
 * Module Name:
 *
 *        protocol.c
 *
 * Abstract:
 *
 *        Protocol specification and construction API
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#include <lwmsg/marshal.h>
#include <lwmsg/unmarshal.h>

#include "protocol-private.h"
#include "util-private.h"
#include "type-private.h"

LWMsgStatus
lwmsg_protocol_new(
    LWMsgContext* context,
    LWMsgProtocol** prot
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    LWMsgProtocol* my_prot = calloc(1, sizeof(*my_prot));

    if (!my_prot)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
    }

    lwmsg_context_setup(&my_prot->context, context);

    *prot = my_prot;

done:

    return status;

error:

    if (my_prot)
    {
        free(my_prot);
    }

    goto done;
}

void
lwmsg_protocol_delete(LWMsgProtocol* prot)
{
    lwmsg_context_cleanup(&prot->context);
    free(prot->types);
    free(prot);
}

LWMsgStatus
lwmsg_protocol_get_message_type(LWMsgProtocol* prot, unsigned int type, LWMsgTypeSpec** out_type)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    if (type >= prot->num_types)
    {
        RAISE_ERROR(&prot->context, status = LWMSG_STATUS_NOT_FOUND,
                    "Unknown message type");
    }
    else
    {
        *out_type = prot->types[type];
    }

error:

    return status;
}

LWMsgStatus
lwmsg_protocol_add_protocol_spec(LWMsgProtocol* prot, LWMsgProtocolSpec* spec)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    size_t num_types = 0;
    size_t i;


    for (i = 0; spec[i].tag != -1; i++)
    {
        if (spec[i].tag >= num_types)
        {
            num_types = spec[i].tag + 1;
        }
    }

    prot->num_types = num_types;
    prot->types = calloc(num_types, sizeof(*prot->types));
    
    if (!prot->types)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
    }

    for (i = 0; spec[i].tag != -1; i++)
    {
        if (spec[i].type)
        {
            prot->types[spec[i].tag] = spec[i].type;
        }
        else
        {
            prot->types[spec[i].tag] = NULL;
        }
    }

error:

    return status;
}


const char*
lwmsg_protocol_get_error_message(
    LWMsgProtocol* prot,
    LWMsgStatus status
    )
{
    return lwmsg_context_get_error_message(&prot->context, status);
}

LWMsgStatus
lwmsg_protocol_marshal(
    LWMsgProtocol* prot,
    unsigned int tag,
    void* object,
    LWMsgBuffer* buffer
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTypeSpec* mtype = NULL;

    BAIL_ON_ERROR(status = lwmsg_protocol_get_message_type(prot, tag, &mtype));

    BAIL_ON_ERROR(status = lwmsg_marshal(&prot->context, mtype, object, buffer));

error:

    return status;
}

LWMsgStatus
lwmsg_protocol_unmarshal(
    LWMsgProtocol* prot,
    unsigned int tag,
    LWMsgBuffer* buffer,
    void** out_object
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTypeSpec* mtype = NULL;

    BAIL_ON_ERROR(status = lwmsg_protocol_get_message_type(prot, tag, &mtype));

    BAIL_ON_ERROR(status = lwmsg_unmarshal(&prot->context, mtype, buffer, out_object));

error:

    return status;
}
