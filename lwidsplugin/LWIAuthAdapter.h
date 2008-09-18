/*
 *  LWIAuthAdapter.h
 *  LWIDSPlugIn
 *
 *  Created by Sriram Nambakam on 5/23/07.
 *  Copyright 2007 Centeris Corporation. All rights reserved.
 *
 */

#ifndef __LWIAUTHADAPTER_H__
#define __LWIAUTHADAPTER_H__

#include "LWAuthAdapterImpl.h"
#include "LWIPlugIn.h"
#include "wbl.h"


class LWIAuthAdapter : public LWAuthAdapterImpl
{
public:

    LWIAuthAdapter();
    ~LWIAuthAdapter();

public:
    
    MACERROR Initialize();
    void Cleanup();

    void setpwent(void);

    void endpwent(void);

    long getpwent(struct passwd *result,
                  char *buffer,
                  size_t buflen,
                  int *errnop);

    long getpwuid(uid_t uid,
                  struct passwd *result,
                  char *buffer,
                  size_t buflen,
                  int *errnop);

    long getpwnam(const char *name,
                  struct passwd *result,
                  char *buffer,
                  size_t buflen,
                  int *errnop);

    void setgrent(void);

    void endgrent(void);

    long getgrent(struct group *result,
                  char *buffer,
                  size_t buflen,
                  int *errnop);

    long getgrgid(gid_t gid,
                  struct group *result,
                  char *buffer,
                  size_t buflen,
                  int *errnop);

    long getgrnam(const char *name,
                  struct group *result,
                  char *buffer,
                  size_t buflen,
                  int *errnop);

    uint32_t authenticate(const char *username,
                          const char *password,
                          bool is_auth_only);

    uint32_t change_password(const char *username,
                             const char *old_password,
                             const char *password);

    uint32_t get_principal(const char *username,
                           char** principal_name);

    void free_principal(char* principal_name);

    uint32_t get_user_groups(const char *user,
                             gid_t **groups,
                             int *num_groups);

    void free_user_groups(gid_t * groups);

private:

    static MACERROR LoadFunction(void* libHandle, const char* functionName, void** functionPointer);
};

#endif /* __LWIAUTHADAPTER_H__ */
