/*
 * Copyright (C) 2013 VMware, Inc. All rights reserved.
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
 * This is a wrapper library for PKCS11 module.
 *
 * The Cryptoki API states that C_Initialize and C_Finalize should be
 * called only once by the application, however Likewise needs cryptoki
 * services in several places: first in authentication providers to
 * identify the user trying to log in, and then in Kerberos stack (pkinit)
 * during actual authentication phase.
 *
 * To work around the need of calling C_Initialize and C_Finalize several
 * times we override them with reference-counted versions here.
 *
 * NOTE: this relies on users of Cryptoki in the project using
 * C_GetFunctionList and then using pointers from the list.
 */

#include <dlfcn.h>
#include <pkcs11.h>
#include <pthread.h>

#ifndef PKCS11_LIBRARY_NAME
#error Missing PKCS11_LIBRARY_NAME definition
#endif

static pthread_mutex_t loadMutex = PTHREAD_MUTEX_INITIALIZER;
static void *pkcs11LibraryHandle;
static int useCount;
static CK_FUNCTION_LIST ckFunctionList;
static CK_FUNCTION_LIST_PTR ckOriginalFnList;

static void PKCS11WrapperUnloadModule(void);

static
CK_RV
Local_C_Initialize(
    CK_VOID_PTR pInitArgs
    )
{
    CK_C_INITIALIZE_ARGS_PTR pArgs = (CK_C_INITIALIZE_ARGS_PTR) pInitArgs;
    CK_C_INITIALIZE_ARGS localArgs =
    {
        .flags = CKF_OS_LOCKING_OK,
    };
    CK_RV retval = CKR_OK;

    if (!pArgs)
    {
        /* Force multithreading */
        pArgs = &localArgs;
    }

    if (pArgs->CreateMutex || pArgs->DestroyMutex ||
        pArgs->LockMutex || pArgs->UnlockMutex ||
        !(pArgs->flags & CKF_OS_LOCKING_OK))
    {
        /* We have to use standard OS locking */
        return CKR_CANT_LOCK;
    }

    pthread_mutex_lock(&loadMutex);

    if (useCount++ == 0)
    {
        retval = ckOriginalFnList->C_Initialize(pInitArgs);
        if (retval != CKR_OK)
        {
            PKCS11WrapperUnloadModule();
            useCount--;
        }
    }

    pthread_mutex_unlock(&loadMutex);

    return retval;
}

static
CK_RV
Local_C_Finalize(
    CK_VOID_PTR pReserved
    )
{
    CK_RV retval;

    pthread_mutex_lock(&loadMutex);

    if (useCount == 0)
    {
        retval = CKR_CRYPTOKI_NOT_INITIALIZED;
    }
    else if (--useCount == 0)
    {
        retval = ckOriginalFnList->C_Finalize(pReserved);
        if (retval == CKR_OK)
        {
            PKCS11WrapperUnloadModule();
        }
        else
        {
            useCount++;
        }
    }

    pthread_mutex_unlock(&loadMutex);

    return retval;
}

static
CK_BBOOL
PKCS11WrapperLoadModule(
    void
    )
{
    void *handle;
    void *ptr;
    CK_FUNCTION_LIST_PTR fnList;
    CK_RV (*getFnList)(CK_FUNCTION_LIST_PTR_PTR);

    if (pkcs11LibraryHandle)
    {
        /* Already loaded */
        return TRUE;
    }

    handle = dlopen(PKCS11_LIBRARY_NAME, RTLD_NOW);
    if (!handle)
    {
        return FALSE;
    }

    ptr = dlsym(handle, "C_GetFunctionList");
    if (!ptr)
    {
        dlclose(handle);
        return FALSE;
    }

    getFnList = ptr;
    if (getFnList(&fnList) != CKR_OK)
    {
        dlclose(handle);
        return FALSE;
    }

    ckOriginalFnList = fnList;

    ckFunctionList = *fnList;
    ckFunctionList.C_GetFunctionList = C_GetFunctionList;
    ckFunctionList.C_Initialize = Local_C_Initialize;
    ckFunctionList.C_Finalize = Local_C_Finalize;

    pkcs11LibraryHandle = handle;

    return TRUE;
}

static
void
PKCS11WrapperUnloadModule(
    void
    )
{
    ckOriginalFnList = NULL;

    dlclose(pkcs11LibraryHandle);
    pkcs11LibraryHandle = NULL;
}


CK_RV
C_GetFunctionList(
    CK_FUNCTION_LIST_PTR_PTR pPtr
)
{
    CK_FUNCTION_LIST_PTR fnListPtr;
    CK_RV retval;

    pthread_mutex_lock(&loadMutex);

    if (!PKCS11WrapperLoadModule())
    {
       retval = CKR_FUNCTION_FAILED;
    }
    else
    {
       *pPtr = &ckFunctionList;
       retval = CKR_OK;
    }

    pthread_mutex_unlock(&loadMutex);

    return retval;
}
