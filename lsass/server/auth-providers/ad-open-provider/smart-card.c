/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright 2013 VMware, Inc. All rights reserved.
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
 */

/*
 * Copyright (C) VMware, Inc.  All rights reserved.
 *
 * Module Name:
 *
 *        smart_card.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Smart card handling functions (implementation)
 *
 * Author: Dmitry Torokhov <dtor@vmware.com>
 */

#include <pkcs11.h>
#include <openssl/err.h>
#include <openssl/safestack.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

#include "adprovider.h"
#include "smart-card.h"

struct _AD_SMART_CARD_DATA {
    CK_FUNCTION_LIST_PTR pkcs11;
};

static
DWORD
AD_GetUPNFromCertificate(
    IN X509 *pCertificate,
    OUT PSTR *ppszUPN
    )
{
    STACK_OF(GENERAL_NAME) *altNames;
    int nAltNames;
    OTHERNAME *upnName = NULL;
    int i;
    DWORD dwError;

    *ppszUPN = NULL;

    altNames = X509_get_ext_d2i(pCertificate, NID_subject_alt_name, 0, 0);
    if (!altNames)
    {
        LSA_LOG_DEBUG("No subjectAltName entries in the certificate\n");
        return LW_ERROR_NO_SUCH_OBJECT;
    }

    nAltNames = sk_GENERAL_NAME_num(altNames);
    for (i = 0; i < nAltNames; i++)
    {
        GENERAL_NAME *altName = sk_GENERAL_NAME_value(altNames, i);
        OTHERNAME *otherName = altName->d.otherName;

        if (altName->type == GEN_OTHERNAME &&
            OBJ_obj2nid(otherName->type_id) == NID_ms_upn &&
            otherName->value->type == V_ASN1_UTF8STRING)
        {
            upnName = otherName;
            break;
        }
    }

    if (upnName)
    {
        void *data = upnName->value->value.utf8string->data;

        LSA_LOG_DEBUG("UPN: %s\n", data);
        dwError = LwAllocateStringPrintf(ppszUPN, "%s", data);
    }
    else
    {
        LSA_LOG_DEBUG("No UPN in certificate\n");
        dwError = LW_ERROR_NO_SUCH_OBJECT;
    }

    sk_GENERAL_NAME_pop_free(altNames, GENERAL_NAME_free);
    return dwError;
}

static
DWORD
AD_ScanCertificates(
    IN PAD_PROVIDER_CONTEXT pContext,
    IN CK_SESSION_HANDLE session,
    OUT PLSA_SECURITY_OBJECT* ppUserInfo
)
{
    PLSA_AD_PROVIDER_STATE pState = pContext->pState;
    PAD_SMART_CARD_DATA pSCard = pState->pScData;
    CK_OBJECT_CLASS certClass = CKO_CERTIFICATE;
    CK_CERTIFICATE_TYPE certType = CKC_X_509;
    CK_BYTE *certValue = NULL;
    CK_BYTE *idValue = NULL;
    CK_OBJECT_HANDLE object;
    CK_ULONG nObjects;
    CK_ATTRIBUTE certTemplate[] =
    {
        { CKA_CLASS, &certClass, sizeof(CK_OBJECT_CLASS) },
        { CKA_CERTIFICATE_TYPE, &certType, sizeof(CK_CERTIFICATE_TYPE) },
        { CKA_ID, NULL, 0 },
        { CKA_VALUE, NULL, 0 }
    };
    CK_RV rc;
    DWORD dwError;
    X509 *x509;
    PSTR pUPN;

    *ppUserInfo = NULL;

    rc = pSCard->pkcs11->C_FindObjectsInit(session, certTemplate, 2);
    if (rc != CKR_OK)
    {
        LSA_LOG_ERROR("C_Initialize() failed, error: 0x%08X\n", rc);
        return LW_ERROR_NOT_HANDLED;
    }

    while (1)
    {
        /* look for certificates */
        rc = pSCard->pkcs11->C_FindObjects(session, &object, 1, &nObjects);
        if (rc != CKR_OK)
        {
            LSA_LOG_ERROR("C_FindObjects() failed: 0x%08lX", rc);
            BAIL_WITH_LSA_ERROR(LW_ERROR_NOT_HANDLED);
        }

        if (nObjects == 0)
        {
            LSA_LOG_DEBUG("No more certificates\n");
            break;
        }

        /* Retrieve cert object id length */
        certTemplate[2].pValue = NULL;
        certTemplate[2].ulValueLen = 0;
        rc = pSCard->pkcs11->C_GetAttributeValue(session, object,
                                                 certTemplate, 3);
        if (rc != CKR_OK)
        {
            LSA_LOG_ERROR("C_GetAttributeValue(ID length) failed: 0x%08lX", rc);
            continue;
        }

        LwFreeMemory(idValue);
        idValue = NULL;
        dwError = LwAllocateMemory(certTemplate[2].ulValueLen,
                                   (PVOID *)&idValue);
        BAIL_ON_LSA_ERROR(dwError);

        /* read cert id into allocated space */
        certTemplate[2].pValue = idValue;
        rc = pSCard->pkcs11->C_GetAttributeValue(session, object,
                                                 certTemplate, 3);
        if (rc != CKR_OK)
        {
            LSA_LOG_ERROR("C_GetAttributeValue(ID) failed: 0x%08lX", rc);
            continue;
        }

        /* Retrieve certificate length */
        certTemplate[3].pValue = NULL;
        rc = pSCard->pkcs11->C_GetAttributeValue(session, object,
                                                 certTemplate, 4);
        if (rc != CKR_OK)
        {
            LSA_LOG_ERROR("C_GetAttributeValue(Cert Length) failed: 0x%08lX",
                          rc);
            continue;
        }

        LwFreeMemory(certValue);
        certValue = NULL;
        dwError = LwAllocateMemory(certTemplate[3].ulValueLen,
                                   (PVOID *)&certValue);
        BAIL_ON_LSA_ERROR(dwError);

        /* Now actually read the certificate */
        certTemplate[3].pValue = certValue;
        rc = pSCard->pkcs11->C_GetAttributeValue(session, object,
                                                certTemplate, 4);
        if (rc != CKR_OK)
        {
            LSA_LOG_ERROR("C_GetAttributeValue(Cert) failed: 0x%08lX", rc);
            continue;
        }

        x509 = d2i_X509(NULL, (const unsigned char **)&certTemplate[3].pValue,
                        certTemplate[3].ulValueLen);
        if (!x509)
        {
            LSA_LOG_ERROR("Failed to parse certificate: %d, skipping\n",
                          ERR_get_error());
            continue;
        }

        dwError = AD_GetUPNFromCertificate(x509, &pUPN);
        X509_free(x509);
        if (dwError == LW_ERROR_NO_SUCH_OBJECT)
        {
            continue;
        }
        BAIL_ON_LSA_ERROR(dwError);

        dwError = AD_FindUserObjectByName(pContext, pUPN, ppUserInfo);
        if (dwError == LW_ERROR_SUCCESS)
        {
            goto cleanup;
        }
    }

    LSA_LOG_DEBUG("No usable certificates found\n");
    BAIL_WITH_LSA_ERROR(LW_ERROR_NO_SUCH_OBJECT);

cleanup:
    LwFreeMemory(idValue);
    LwFreeMemory(certValue);

    rc = pSCard->pkcs11->C_FindObjectsFinal(session);
    if (rc != CKR_OK)
    {
        LSA_LOG_WARNING("C_FindObjectsFinal() failed: 0x%08lX", rc);
    }

    return dwError;

error:
    goto cleanup;
}

static
DWORD
AD_EstablishCardSession(
    IN PAD_SMART_CARD_DATA pSCard,
    IN CK_SLOT_ID slotId,
    IN BOOLEAN readOnly,
    OUT CK_SESSION_HANDLE *pSession
)
{
    CK_RV rc;

    rc = pSCard->pkcs11->C_OpenSession(slotId, CKF_SERIAL_SESSION,
                                        NULL, NULL, pSession);
    if (rc != CKR_OK)
    {
        LSA_LOG_ERROR("C_OpenSession() failed, error: 0x%08X\n", rc);
        return LW_ERROR_NOT_HANDLED;
    }

    return LW_ERROR_SUCCESS;
}

static
DWORD
AD_CloseCardSession(
    IN PAD_SMART_CARD_DATA pSCard,
    IN CK_SESSION_HANDLE session
)
{
    CK_RV rc;

    rc = pSCard->pkcs11->C_CloseSession(session);
    if (rc != CKR_OK)
    {
        LSA_LOG_ERROR("C_CloseSession() failed, error: 0x%08X\n", rc);
        return LW_ERROR_NOT_HANDLED;
    }

    return LW_ERROR_SUCCESS;
}

DWORD
AD_InspectSmartCard(
    IN PAD_PROVIDER_CONTEXT pContext,
    OUT PLSA_SECURITY_OBJECT* ppUserInfo,
    OUT PSTR* ppszSmartCardReader
    )
{
    PLSA_AD_PROVIDER_STATE pState = pContext->pState;
    PAD_SMART_CARD_DATA pSCard = pState->pScData;
    PLSA_SECURITY_OBJECT pUserInfo;
    CK_SLOT_ID *pSlotList = NULL;
    unsigned long nSlots;
    DWORD dwError;
    int i;
    CK_RV rc;

    *ppUserInfo = NULL;
    *ppszSmartCardReader = NULL;

    if (!pSCard)
    {
        LSA_LOG_DEBUG("Cryptoki is not initialized");
        BAIL_WITH_LSA_ERROR(LW_ERROR_NOT_HANDLED);
    }

    rc = pSCard->pkcs11->C_GetSlotList(CK_TRUE, NULL, &nSlots);
    if (rc != CKR_OK)
    {
        LSA_LOG_ERROR("C_GetInfo() failed: 0x%08X", rc);
        BAIL_WITH_LSA_ERROR(LW_ERROR_NOT_HANDLED);
    }

    do
    {
        dwError = LwReallocMemory(pSlotList, (PVOID *)&pSlotList,
                                  sizeof(CK_SLOT_ID) * nSlots);
        BAIL_ON_LSA_ERROR(dwError);

        rc = pSCard->pkcs11->C_GetSlotList(CK_TRUE, pSlotList, &nSlots);
        if (rc != CKR_OK && rc != CKR_BUFFER_TOO_SMALL)
        {
            LSA_LOG_ERROR("C_GetInfo() failed: 0x%08X", rc);
            BAIL_WITH_LSA_ERROR(LW_ERROR_NOT_HANDLED);
        }
    } while (rc == CKR_BUFFER_TOO_SMALL);

    LSA_LOG_DEBUG("Number of slots with tokens: %d\n", nSlots);

    for (i = 0; i < nSlots; i++)
    {
        CK_SLOT_INFO slotInfo;
        CK_TOKEN_INFO tokenInfo;
        CK_SESSION_HANDLE session;

        rc = pSCard->pkcs11->C_GetSlotInfo(pSlotList[i], &slotInfo);
        if (rc != CKR_OK)
        {
            LSA_LOG_ERROR("C_GetSlotInfo() failed for slot %d: 0x%08X",
                          pSlotList[i], rc);
            continue;
        }

        LSA_LOG_DEBUG("Slot info of slot %d (#%d):\n"
                      "\tDescription: %.*s\n"
                      "\tManufacturer: %.*s\n"
                      "\tFlags: 0x%08X\n",
                      pSlotList[i], i,
                      sizeof(slotInfo.slotDescription), slotInfo.slotDescription,
                      sizeof(slotInfo.manufacturerID), slotInfo.manufacturerID,
                      tokenInfo.flags);

        rc = pSCard->pkcs11->C_GetTokenInfo(pSlotList[i], &tokenInfo);
        if (rc != CKR_OK)
        {
            LSA_LOG_ERROR("C_GetTokenInfo() failed for slot %d: 0x%08X",
                          pSlotList[i], rc);
            continue;
        }

        LSA_LOG_DEBUG("Token info of token #%d:\n"
                    "\tLabel: %.*s\n"
                    "\tManufacturer: %.*s\n"
                    "\tModel: %.*s\n"
                    "\tSerial Number: %.*s\n"
                    "\tFlags: 0x%08X\n",
                    i,
                    sizeof(tokenInfo.label), tokenInfo.label,
                    sizeof(tokenInfo.manufacturerID), tokenInfo.manufacturerID,
                    sizeof(tokenInfo.model), tokenInfo.model,
                    sizeof(tokenInfo.serialNumber), tokenInfo.serialNumber,
                    tokenInfo.flags);

        dwError = AD_EstablishCardSession(pSCard, pSlotList[i], TRUE, &session);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = AD_ScanCertificates(pContext, session, &pUserInfo);
        AD_CloseCardSession(pSCard, session);

        if (dwError == LW_ERROR_SUCCESS)
        {
            if (*ppUserInfo)
            {
                LSA_LOG_INFO("More than one potential user found, aborting\n");
                BAIL_WITH_LSA_ERROR(LW_ERROR_NO_SUCH_OBJECT);
            }

            *ppUserInfo = pUserInfo;
            dwError = LwAllocateStringPrintf(ppszSmartCardReader,
                                             "%.*s",
                                             sizeof(slotInfo.slotDescription),
                                             slotInfo.slotDescription);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    if (!*ppUserInfo)
    {
        /* No useful certificates found */
        BAIL_WITH_LSA_ERROR(LW_ERROR_NO_SUCH_OBJECT);
    }

    dwError = LW_ERROR_SUCCESS;

cleanup:
    LwFreeMemory(pSlotList);
    return dwError;

error:
    ADCacheSafeFreeObject(ppUserInfo);
    LwFreeMemory(*ppszSmartCardReader);
    *ppszSmartCardReader = NULL;
    goto cleanup;
}

static
DWORD
AD_InitializeCryptoki(
    PAD_SMART_CARD_DATA pSCard
    )
{
    CK_C_INITIALIZE_ARGS initArgs = {
        .flags = CKF_OS_LOCKING_OK,
    };
    CK_INFO info;
    CK_RV rc;
    DWORD dwError;

    rc = C_GetFunctionList(&pSCard->pkcs11);
    if (rc != CKR_OK)
    {
        LSA_LOG_ERROR("C_GetFunctionList() failed, error: 0x%08X\n", rc);
        return LW_ERROR_NOT_HANDLED;
    }

    rc = pSCard->pkcs11->C_Initialize((CK_VOID_PTR) &initArgs);
    if (rc != CKR_OK)
    {
        LSA_LOG_ERROR("C_Initialize() failed, error: 0x%08X\n", rc);
        return LW_ERROR_NOT_HANDLED;
    }

    rc = pSCard->pkcs11->C_GetInfo(&info);
    if (rc != CKR_OK)
    {
        LSA_LOG_ERROR("C_GetInfo() failed: 0x%08X", rc);
        BAIL_WITH_LSA_ERROR(LW_ERROR_NOT_HANDLED);
    }

    LSA_LOG_DEBUG("PKCS#11 module info:\n"
                  "\tVersion: %d.%d\n"
                  "\tManufacturer: %.*s\n"
                  "\tFlags: 0x%08X\n"
                  "\tLibrary Name: %.*s, version: %d.%d\n",
                  info.cryptokiVersion.major, info.cryptokiVersion.minor,
                  sizeof(info.manufacturerID), info.manufacturerID,
                  info.flags,
                  sizeof(info.libraryDescription), info.libraryDescription,
                  info.libraryVersion.major, info.libraryVersion.minor);

    return LW_ERROR_SUCCESS;

error:
    pSCard->pkcs11->C_Finalize(NULL);
    return dwError;
}

DWORD
AD_InitializeSmartCard(
    IN PLSA_AD_PROVIDER_STATE pState
    )
{
    PAD_SMART_CARD_DATA pSCard;
    DWORD dwError;

    dwError = LwAllocateMemory(sizeof(*pSCard), (PVOID *)&pSCard);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_InitializeCryptoki(pSCard);
    if (!dwError)
    {
        pState->pScData = pSCard;
    }
    else
    {
        /* This is not a fatal error */
        LSA_LOG_INFO("Failed to initialize Cryptoki (PKCS#11) module, "
                     "Smart Card services will be disabled.\n");
        LwFreeMemory(pSCard);
    }

    return LW_ERROR_SUCCESS;

error:
    return dwError;
}

VOID
AD_FinishSmartCard(
    IN PLSA_AD_PROVIDER_STATE pState
    )
{
    PAD_SMART_CARD_DATA pSCard = pState->pScData;
    if (pSCard)
    {
        pSCard->pkcs11->C_Finalize(NULL);
        LwFreeMemory(pSCard);
        pState->pScData = NULL;
    }
}


