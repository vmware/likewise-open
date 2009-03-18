#ifndef _SAMR_CONTEXT_HANDLE_H_
#define _SAMR_CONTEXT_HANDLE_H_


enum SamrContextType {
    SamrContextConnect = 0,
    SamrContextDomain,
    SamrContextAccount
};


typedef struct samr_generic_context {
    enum SamrContextType    Type;
} GENERIC_CONTEXT, *PGENERIC_CONTEXT;


typedef struct samr_connect_context {
    enum SamrContextType    Type;
    PACCESS_TOKEN           pUserToken;
} CONNECT_CONTEXT, *PCONNECT_CONTEXT;


void
CONNECT_HANDLE_rundown(
    void *hContext
    );


#endif /* _SAMR_CONTEXT_HANDLE_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
