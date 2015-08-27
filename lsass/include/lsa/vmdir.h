/*
 * Copyright (C) 2015 VMware, Inc. All rights reserved.
 */
#ifndef __LSACLIENT_VMDIR_H__
#define __LSACLIENT_VMDIR_H__

#include <lsa/lsa.h>
#include <sys/types.h>
#include <lsa/vmdir-types.h>
#include <lsa/lsapstore-types.h>

DWORD
LsaVmdirSignal(
    IN HANDLE hLsaConnection,
    IN DWORD dwFlags
    );

#endif /* __LSACLIENT_VMDIR_H__ */
