#include "winbind_client.h"
#include <ctype.h>
#include <dirent.h>

#include "wb_gp.h"


/* Define signatures for the entry points that are used from libgpapi.so */
typedef int (*PFN_GP_GET_LOGON_RIGHTS)(
                  char** pszValue
                  );
typedef int (*PFN_GP_PROCESS_LOGIN)(
                  void* context,
                  const char* Username,
                  int cached,
                  gp_pam_msg_cb_t log_cb,
                  gp_pam_msg_cb_t user_msg_cb
                  );
typedef int (*PFN_GP_PROCESS_LOGOUT)(
                  void* context,
                  const char* Username,
                  int cached,
                  gp_pam_msg_cb_t log_cb,
                  gp_pam_msg_cb_t user_msg_cb
                  );
typedef void (*PFN_GP_FREE_BUFFER)(
                   char* buf
                   );

/* Local function pointer wrappers to libgpapi.so exports */
static PFN_GP_GET_LOGON_RIGHTS gpfnGPGetLogonRights = NULL;
static PFN_GP_PROCESS_LOGIN    gpfnGPProcessLogin = NULL;
static PFN_GP_PROCESS_LOGOUT   gpfnGPProcessLogout = NULL;
static PFN_GP_FREE_BUFFER      gpfnGPFreeBuffer = NULL;

static void *                  gpGPLibHandle = (void*)NULL;
static bool                    gfGPLibInitialized = false;

int
gp_init_api()
{
    int rcode = 0;
    char szGPLibPath[256];

    /* Test to see if we are already setup */
    if (gfGPLibInitialized == true) {
        rcode = 1;
        goto exit;
    }

    gfGPLibInitialized = true;

    sprintf(szGPLibPath, "%s/%s", LIBDIR, GPAPI_DLL_NAME);

    dlerror();

    gpGPLibHandle = dlopen(szGPLibPath, RTLD_LAZY);
    if (gpGPLibHandle == NULL) {
       goto exit;
    }

    gpfnGPGetLogonRights = (PFN_GP_GET_LOGON_RIGHTS)dlsym(
                                gpGPLibHandle,
                                "gp_pam_get_interactive_logon_rights"
                                );
    if (gpfnGPGetLogonRights == NULL) {
        goto exit;
    }

    gpfnGPProcessLogin = (PFN_GP_PROCESS_LOGIN)dlsym(
                              gpGPLibHandle,
                              "gp_pam_process_login"
                              );
    if (gpfnGPProcessLogin == NULL) {
        goto exit;
    }

    gpfnGPProcessLogout = (PFN_GP_PROCESS_LOGOUT)dlsym(
                               gpGPLibHandle,
                               "gp_pam_process_logout"
                               );
    if (gpfnGPProcessLogout == NULL) {
        goto exit;
    }

    gpfnGPFreeBuffer = (PFN_GP_FREE_BUFFER)dlsym(
                            gpGPLibHandle,
                            "gp_pam_free_buffer"
                            );
    if (gpfnGPFreeBuffer == NULL) {
        goto exit;
    }

    rcode = 1;

exit:

    if (rcode != 1) {
        gp_close_api();
    }

    return rcode;
}

void
gp_close_api()
{
    if (gpGPLibHandle) {
        if (gpfnGPGetLogonRights) {
            gpfnGPGetLogonRights = NULL; 
        } 
        if (gpfnGPProcessLogin) {
            gpfnGPProcessLogin = NULL; 
        } 
        if (gpfnGPProcessLogout) {
            gpfnGPProcessLogout = NULL; 
        } 
        if (gpfnGPFreeBuffer) {
            gpfnGPFreeBuffer = NULL; 
        } 

        dlclose(gpGPLibHandle);
        gpGPLibHandle = (void*)NULL;
    }
}

/* return non-zero on success, 0 on failure, returned string can be null if nothing there */
int
gp_get_interactive_logon_rights(
    char** pszValue
    )
{
    int rcode = 0;

    rcode = gp_init_api();
    if (rcode != 1) {
        goto exit;
    }

    if (gfGPLibInitialized && gpGPLibHandle && gpfnGPGetLogonRights) {
        rcode = gpfnGPGetLogonRights(pszValue);
    } else {
        *pszValue = NULL;
        rcode = 1;
    }

exit:

    return rcode;
}

int
gp_process_login(
    void* context,
    const char* Username,
    int cached,
    gp_pam_msg_cb_t log_cb,
    gp_pam_msg_cb_t user_msg_cb
    )
{
    int rcode = 0;

    rcode = gp_init_api();
    if (rcode != 1) {
        goto exit;
    }

    if (gpGPLibHandle && gpfnGPProcessLogin) {
        rcode = gpfnGPProcessLogin(context,
                                   Username,
                                   cached,
                                   log_cb,
                                   user_msg_cb);
    } else {
        rcode = 1;
    }

exit:

    return rcode;
}

int
gp_process_logout(
    void* context,
    const char* Username,
    int cached,
    gp_pam_msg_cb_t log_cb,
    gp_pam_msg_cb_t user_msg_cb
    )
{
    int rcode = 0;

    rcode = gp_init_api();
    if (rcode != 1) {
        goto exit;
    }

    if (gpGPLibHandle && gpfnGPProcessLogout) {
        rcode = gpfnGPProcessLogout(context,
                                    Username,
                                    cached,
                                    log_cb,
                                    user_msg_cb);
    } else {
        rcode = 1;
    }

exit:

    return rcode;
}

void
gp_free_buffer(
    char* buf
    )
{
    int rcode = gp_init_api();
    if (rcode != 1) {
        return;
    }

    if (gpGPLibHandle && gpfnGPFreeBuffer) {
        gpfnGPFreeBuffer(buf);
    }
}

