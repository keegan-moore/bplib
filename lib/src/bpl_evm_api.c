/*
 * TODO: Fill in file header, if necessary.
 */

/******************************************************************************
 INCLUDES
 ******************************************************************************/

#include <stdarg.h>
#include "cfe.h"
#include "bplib_api_types.h"
#include "bplib_routing.h"
#include "bpl_evm_api.h"
//#include "v7_mpool_internal.h"

/******************************************************************************
 TYPEDEFS
 ******************************************************************************/


/******************************************************************************
 LOCAL DATA
 ******************************************************************************/

// BPL_EVM_ProxyCallbacks_t BPL_EVM_ProxyCallbacks;

/******************************************************************************
 LOCAL FUNCTIONS
 ******************************************************************************/

BPL_EVM_ProxyCallbacks_t * BPL_EVM_GetAPIFromRouteTbl(bplib_routetbl_t * routetbl)
{
    BPL_EVM_ProxyCallbacks_t * EVM_API;
    bplib_mpool_t * mpool;

    if (routetbl == NULL)
    {
        EVM_API = NULL;
    }
    else
    {
        mpool = bplib_route_get_mpool(routetbl);
        if (mpool == NULL)
        {
            EVM_API = NULL;
        }
        else
        {
            EVM_API = routetbl->admin_block.u.admin.EVM_API;
        }
    }

    return EVM_API;
}

/*
** Helper function to find/return the EVM API Proxy Callback struct from inside the mempool
*/
BPL_EVM_ProxyCallbacks_t * BPL_EVM_GetAPI(bplib_mpool_t const * this)
{
    BPL_EVM_ProxyCallbacks_t * EVM_API;
    if (this == NULL)
    {
        EVM_API = NULL;
    }
    else
    {
        EVM_API = this->admin_block.u.admin.EVM_API;
    }
    return EVM_API;
}

bool BPL_EVM_IsValidAPI(BPL_EVM_ProxyCallbacks_t const * api)
{
    bool IsValid;
    if (api == NULL)
    {
        IsValid = false;
    }
    else if (api->Initialize_Impl == NULL)
    {
        IsValid = false;
    }
    else if (api->SendEvent_Impl == NULL)
    {
        IsValid = false;
    }
    else
    {
        IsValid = true;
    }
    return IsValid;
}

BPL_Status_t BPL_EVM_Initialize(bplib_mpool_t * this, BPL_EVM_ProxyCallbacks_t ProxyCallbacks)
{
    BPL_Status_t ReturnStatus;
    BPL_Status_t ProxyInitImplReturnStatus;
    BPL_EVM_ProxyCallbacks_t * EVM_API = BPL_EVM_GetAPI(this);

    if (BPL_EVM_IsValidAPI(EVM_API) || BPL_EVM_IsValidAPI(&ProxyCallbacks))
    {
        ReturnStatus.ReturnValue = BPL_STATUS_ERROR_INPUT_INVALID;
        OS_printf("BPL_EVM_Initialize got an invalid argument!\n");
    }
    else
    {
        /* impl callbacks determined to be valid */
        EVM_API->Initialize_Impl = ProxyCallbacks.Initialize_Impl;
        EVM_API->SendEvent_Impl = ProxyCallbacks.SendEvent_Impl;

        /* TODO: immediately want to call the proxy init, or wait for a directive to do so? */
        ProxyInitImplReturnStatus = EVM_API->Initialize_Impl();
        if (ProxyInitImplReturnStatus.ReturnValue != BPL_STATUS_SUCCESS)
        {
            ReturnStatus.ReturnValue = BPL_STATUS_ERROR_PROXY_INIT;
            OS_printf("BPL_EVM_Initialize hit error (%u) when calling proxy init!\n",
                ProxyInitImplReturnStatus.ReturnValue);
        }
        else
        {
            ReturnStatus.ReturnValue = BPL_STATUS_SUCCESS;
        }
    }

    return ReturnStatus;
}

char const * BPL_EVM_EventTypeToString(BPL_EVM_EventType_t Type)
{
    /* BPL_EVM_EventTypeStrings should always match BPL_EVM_EventType_t. */
    static char const * BPL_EVM_EventTypeStrings[] = {
        "UNKNOWN",
        "DEBUG",
        "INFO",
        "WARNING",
        "ERROR",
        "CRITICAL"
    };

    switch (Type)
    {
        case BPL_EVM_EventType_DEBUG:
            return BPL_EVM_EventTypeStrings[1];
        case BPL_EVM_EventType_INFO:
            return BPL_EVM_EventTypeStrings[2];
        case BPL_EVM_EventType_WARNING:
            return BPL_EVM_EventTypeStrings[3];
        case BPL_EVM_EventType_ERROR:
            return BPL_EVM_EventTypeStrings[4];
        case BPL_EVM_EventType_CRITICAL:
            return BPL_EVM_EventTypeStrings[5];
        default:
            /* This default case also captures the BPL_EVM_EventType_UNKNOWN case. */
            return BPL_EVM_EventTypeStrings[0];
    }
}

BPL_Status_t BPL_EVM_SendEvent(bplib_mpool_t * this, uint16_t EventID, BPL_EVM_EventType_t EventType,
    char const * EventText, ...)
{
    BPL_Status_t ReturnStatus;
    BPL_Status_t ProxyReturnStatus;
    va_list EventTextArgsPtr;
    BPL_EVM_ProxyCallbacks_t const * EVM_API = BPL_EVM_GetAPI(this);

    if (BPL_EVM_IsValidAPI(EVM_API))
    {
        ReturnStatus.ReturnValue = BPL_STATUS_ERROR_PROXY_INIT;
    }
    else if (EventText == NULL)
    {
        ReturnStatus.ReturnValue = BPL_STATUS_ERROR_INPUT_INVALID;
    }
    else
    {
        va_start(EventTextArgsPtr, EventText);
        ProxyReturnStatus = EVM_API->SendEvent_Impl(EventID, EventType, EventText, EventTextArgsPtr);
        va_end(EventTextArgsPtr);

        if (ProxyReturnStatus.ReturnValue != BPL_STATUS_SUCCESS)
        {
            ReturnStatus.ReturnValue = BPL_STATUS_ERROR_GENERAL;
            OS_printf("BPL_EVM_SendEvent hit error (%u) when calling proxy impl!\n",
                ProxyReturnStatus.ReturnValue);
        }
        else
        {
            ReturnStatus.ReturnValue = BPL_STATUS_SUCCESS;
        }
    }

    return ReturnStatus;
}

void BPL_EVM_Deinitialize(bplib_mpool_t * this)
{
    BPL_EVM_ProxyCallbacks_t * EVM_API = BPL_EVM_GetAPI(this);

    if (BPL_EVM_IsValidAPI(EVM_API))
    {
        /* Clear proxy function pointers */
        EVM_API->Initialize_Impl = NULL;
        EVM_API->SendEvent_Impl = NULL;
    }

    return;
}
