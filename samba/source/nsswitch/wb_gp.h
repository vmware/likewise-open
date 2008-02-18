#ifndef __WB_GP_H__
#define __WB_GP_H__

typedef void (*gp_pam_msg_cb_t)(void *context, int is_err, char *format, ...);

int
gp_init_api();

void
gp_close_api();

int
gp_get_interactive_logon_rights(
    char** pszValue
    );

int
gp_process_login(
    void* context,
    const char* Username,
    int cached,
    gp_pam_msg_cb_t log_cb,
    gp_pam_msg_cb_t user_msg_cb
    );

int
gp_process_logout(
    void* context,
    const char* Username,
    int cached,
    gp_pam_msg_cb_t log_cb,
    gp_pam_msg_cb_t user_msg_cb
    );

void
gp_free_buffer(
    char* buf
    );

#endif /* __WB_GP_H__ */
