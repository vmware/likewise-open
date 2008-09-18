/*
 *  LWIAuthAdapter.cpp
 *  LWIDSPlugIn
 *
 *  Created by Sriram Nambakam on 5/23/07.
 *  Copyright 2007 Centeris Corporation. All rights reserved.
 *
 */

#include "LWIAuthAdapter.h"
#include "wbl.h"

#define DECL_LWI_FUNC(PTR, func, retType, args) \
    typedef retType (* PTR(func)) args

#define DECL_LWI_VAR(PTR, VAR, func)  \
    PTR(func) VAR(func) = 0

#define NSS_FUNC_PTR_T(func) \
    PF_NSS_LWIDENTITY_ ## func ## _t
#define NSS_FUNC_NAME(func) \
    "_nss_lwidentity_" #func

#define DECL_NSS(func, retType, args) \
    DECL_LWI_FUNC(NSS_FUNC_PTR_T, func, retType, args)

#define MAC_FUNC_PTR_T(func) \
    PF_MAC_LWIDENTITY_ ## func ## _t
#define MAC_FUNC_NAME(func) \
    "mac_ds_" #func

#define DECL_MAC(func, retType, args) \
    DECL_LWI_FUNC(MAC_FUNC_PTR_T, func, retType, args)

/* user functions */

DECL_NSS(setpwent, void, (void));
DECL_NSS(endpwent, void, (void));
DECL_NSS(getpwent_r, long, (struct passwd *result, char *buffer, size_t buflen, int *errnop));
DECL_NSS(getpwuid_r, long, (uid_t uid, struct passwd *result, char *buffer, size_t buflen, int *errnop));
DECL_NSS(getpwnam_r, long, (const char *name, struct passwd *result, char *buffer, size_t buflen, int *errnop));

/* group functions */

DECL_NSS(setgrent, void, (void));
DECL_NSS(endgrent, void, (void));
DECL_NSS(getgrent_r, long, (struct group *result, char *buffer, size_t buflen, int *errnop));
DECL_NSS(getgrnam_r, long, (const char *name, struct group *result, char *buffer, size_t buflen, int *errnop));
DECL_NSS(getgrgid_r, long, (gid_t gid, struct group *result, char *buffer, size_t buflen, int *errnop));

/* additional functions */

DECL_MAC(authenticate, uint32_t, (const char *username,
                                   const char *password,
                                   bool is_auth_only,
                                   WBL_LOG_CALLBACK log_function,
                                   WBL_LOG_CALLBACK message_function,
                                   void* context));
DECL_MAC(change_password, uint32_t, (const char *username,
                                      const char *old_password,
                                      const char *password,
                                      WBL_LOG_CALLBACK log_function,
                                      WBL_LOG_CALLBACK message_function,
                                      void* context));
DECL_MAC(get_principal, uint32_t, (const char* username,
                                     char** principal_name,
                                     WBL_LOG_CALLBACK log_function,
                                     WBL_LOG_CALLBACK message_function,
                                     void* context));
DECL_MAC(free_principal, uint32_t, (char* principal_name));
DECL_MAC(get_user_groups, int, (const char *user, gid_t **groups, int *num_groups));

typedef struct _NSS_MODULE_FUNCTIONS {
    NSS_FUNC_PTR_T(setpwent) setpwent;
    NSS_FUNC_PTR_T(endpwent) endpwent;
    NSS_FUNC_PTR_T(getpwent_r) getpwent_r;
    NSS_FUNC_PTR_T(getpwuid_r) getpwuid_r;
    NSS_FUNC_PTR_T(getpwnam_r) getpwnam_r;

    NSS_FUNC_PTR_T(setgrent) setgrent;
    NSS_FUNC_PTR_T(endgrent) endgrent;
    NSS_FUNC_PTR_T(getgrent_r) getgrent_r;
    NSS_FUNC_PTR_T(getgrgid_r) getgrgid_r;
    NSS_FUNC_PTR_T(getgrnam_r) getgrnam_r;

    MAC_FUNC_PTR_T(authenticate) authenticate;
    MAC_FUNC_PTR_T(change_password) change_password;
    MAC_FUNC_PTR_T(get_principal) get_principal;
    MAC_FUNC_PTR_T(free_principal) free_principal;
    MAC_FUNC_PTR_T(get_user_groups) get_user_groups;
} NSS_MODULE_FUNCTIONS;

typedef struct _NSS_MODULE {
    void *LibHandle;
    NSS_MODULE_FUNCTIONS Functions;
} NSS_MODULE;

static NSS_MODULE NssModuleState;

#define NSS_LIB_PATH "/usr/lib/nss_lwidentity.1.dylib"

#define NSS_MODULE_FUNC(function) \
    (NssModuleState.Functions.function)

#define LOAD_NSS_FUNCTION(function) \
    LoadFunction(NssModuleState.LibHandle, NSS_FUNC_NAME(function), (void**)&NSS_MODULE_FUNC(function))

#define LOAD_MAC_FUNCTION(function) \
    LoadFunction(NssModuleState.LibHandle, MAC_FUNC_NAME(function), (void**)&NSS_MODULE_FUNC(function))


MACERROR LWIAuthAdapter::LoadFunction(void* LibHandle, const char* FunctionName, void** FunctionPointer)
{
    MACERROR macError = eDSNoErr;
    void* function;

    function = dlsym(LibHandle, FunctionName);
    if (!function)
    {
        LOG_ERROR("Failed to load symbol \"%s\" from library \"" NSS_LIB_PATH "\" with error: %s", FunctionName, dlerror());
        macError = ePlugInInitError;
    }

    *FunctionPointer = function;

    return macError;
}

LWIAuthAdapter::LWIAuthAdapter()
{
}

LWIAuthAdapter::~LWIAuthAdapter()
{
}

MACERROR LWIAuthAdapter::Initialize()
{
    MACERROR macError = eDSNoErr;

    if (!NssModuleState.LibHandle)
    {
        NssModuleState.LibHandle = dlopen(NSS_LIB_PATH, RTLD_LAZY);
        if (!NssModuleState.LibHandle)
        {
            int libcError = errno;
            LOG_ERROR("Failed to load " NSS_LIB_PATH ": %s (%d)", strerror(libcError), libcError);
            macError = ePlugInInitError;
            GOTO_CLEANUP();
        }
    }

    /* clear any existing error */
    dlerror();

    /* user functions */

    macError = LOAD_NSS_FUNCTION(setpwent);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LOAD_NSS_FUNCTION(endpwent);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LOAD_NSS_FUNCTION(getpwent_r);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LOAD_NSS_FUNCTION(getpwuid_r);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LOAD_NSS_FUNCTION(getpwnam_r);
    GOTO_CLEANUP_ON_MACERROR(macError);

    /* group functions */

    macError = LOAD_NSS_FUNCTION(setgrent);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LOAD_NSS_FUNCTION(endgrent);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LOAD_NSS_FUNCTION(getgrent_r);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LOAD_NSS_FUNCTION(getgrgid_r);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LOAD_NSS_FUNCTION(getgrnam_r);
    GOTO_CLEANUP_ON_MACERROR(macError);

    /* additional functions */

    macError = LOAD_MAC_FUNCTION(authenticate);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LOAD_MAC_FUNCTION(change_password);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LOAD_MAC_FUNCTION(get_principal);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LOAD_MAC_FUNCTION(free_principal);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LOAD_MAC_FUNCTION(get_user_groups);
    GOTO_CLEANUP_ON_MACERROR(macError);

cleanup:
    if (macError)
    {
        Cleanup();
    }

    return macError;
}

void LWIAuthAdapter::Cleanup()
{
    if (NssModuleState.LibHandle)
    {
        memset(&NssModuleState.Functions, 0, sizeof(NssModuleState.Functions));
        dlclose(NssModuleState.LibHandle);
        NssModuleState.LibHandle = NULL;
    }
}

void
LWIAuthAdapter::setpwent(void)
{
    NSS_MODULE_FUNC(setpwent)();
}

void
LWIAuthAdapter::endpwent(void)
{
    NSS_MODULE_FUNC(endpwent)();
}

long
LWIAuthAdapter::getpwent(struct passwd *result,
                         char *buffer,
                         size_t buflen,
                         int *errnop)
{
    return NSS_MODULE_FUNC(getpwent_r)(result, buffer, buflen, errnop);
}

long
LWIAuthAdapter::getpwuid(
    uid_t uid,
    struct passwd *result,
    char *buffer,
    size_t buflen,
    int *errnop
    )
{
    return NSS_MODULE_FUNC(getpwuid_r)(uid, result, buffer, buflen, errnop);
}

long
LWIAuthAdapter::getpwnam(
    const char *name,
    struct passwd *result,
    char *buffer,
    size_t buflen,
    int *errnop
    )
{
    return NSS_MODULE_FUNC(getpwnam_r)(name, result, buffer, buflen, errnop);
}

void
LWIAuthAdapter::setgrent(void)
{
    NSS_MODULE_FUNC(setgrent)();
}

void
LWIAuthAdapter::endgrent(void)
{
    NSS_MODULE_FUNC(endgrent)();
}

long
LWIAuthAdapter::getgrent(
    struct group *result,
    char *buffer,
    size_t buflen,
    int *errnop
    )
{
    return NSS_MODULE_FUNC(getgrent_r)(result, buffer, buflen, errnop);
}

long
LWIAuthAdapter::getgrgid(
    gid_t gid,
    struct group *result,
    char *buffer,
    size_t buflen,
    int *errnop
    )
{
    return NSS_MODULE_FUNC(getgrgid_r)(gid, result, buffer, buflen, errnop);
}

long
LWIAuthAdapter::getgrnam(
    const char *name,
    struct group *result,
    char *buffer,
    size_t buflen,
    int *errnop
    )
{
    return NSS_MODULE_FUNC(getgrnam_r)(name, result, buffer, buflen, errnop);
}

static
void
LogCallback(
    void* Context,
    WBL_LOG_LEVEL Level,
    const char* Format,
    va_list Args
    )
{
    char* alloc_output = NULL;
    char* fail_output = "*** Failed to allocate memory while formatting log message ***";
    char* output;

    vasprintf(&alloc_output, Format, Args);
    output = alloc_output ? alloc_output : fail_output;
    LogMessage("[LWI/WBL](%d): %s", Level, output);
    if (alloc_output)
    {
        free(alloc_output);
    }
}

uint32_t
LWIAuthAdapter::authenticate(
    const char *username,
    const char *password,
    bool is_auth_only
    )
{
    return NSS_MODULE_FUNC(authenticate)(username, password, is_auth_only, LogCallback, LogCallback, NULL);
}

uint32_t
LWIAuthAdapter::change_password(
    const char *username,
    const char *old_password,
    const char *password
    )
{
    return NSS_MODULE_FUNC(change_password)(username, old_password, password, LogCallback, LogCallback, NULL);
}

uint32_t
LWIAuthAdapter::get_principal(
    const char *username,
    char** principal_name
    )
{
    return NSS_MODULE_FUNC(get_principal)(username, principal_name, LogCallback, LogCallback, NULL);
}

void
LWIAuthAdapter::free_principal(
    char* principal_name
    )
{
    NSS_MODULE_FUNC(free_principal)(principal_name);
}

uint32_t
LWIAuthAdapter::get_user_groups(
    const char *user,
    gid_t **groups,
    int *num_groups
    )
{
    return NSS_MODULE_FUNC(get_user_groups)(user, groups, num_groups);
}

void
LWIAuthAdapter::free_user_groups(
    gid_t * groups
    )
{
    if (groups)
        free(groups);
}

