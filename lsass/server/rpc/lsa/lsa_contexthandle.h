#ifndef _LSA_CONTEXT_HANDLE_H_
#define _LSA_CONTEXT_HANDLE_H_


typedef struct lsa_policy_context {
    PACCESS_TOKEN pUserToken;
} POLICY_CONTEXT, *PPOLICY_CONTEXT;


#endif /* _LSA_CONTEXT_HANDLE_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
