/*
 * Copyright (C) VMware. All rights reserved.
 */

typedef struct _SAMPLE_AUTH_PROVIDER_CONTEXT
{
	pthread_mutex_t  mutex;
	pthread_mutex_t* pMutex;

	uid_t peer_uid;
	gid_t peer_gid;
	pid_t peer_pid;

} SAMPLE_AUTH_PROVIDER_CONTEXT, *PSAMPLE_AUTH_PROVIDER_CONTEXT;

typedef struct _SAMPLE_GROUP_INFO
{
	PSTR  pszName;
	gid_t gid;
	PSTR  pszSID;
	PSTR ppszMembers[];

} SAMPLE_GROUP_INFO, *PSAMPLE_GROUP_INFO;

typedef struct _SAMPLE_USER_INFO
{
	PSTR    pszName;
	uid_t   uid;
	gid_t   gid;
	PSTR    pszShell;
	PSTR    pszHomedir;
	PSTR    pszUPN;
	PSTR    pszSID;
	PSTR    pszGecos;
	BOOLEAN bPasswordExpired;
	BOOLEAN bAccountLocked;
	BOOLEAN bAccountExpired;
	BOOLEAN bAccountDisabled;
	BOOLEAN bUserCanChangePassword;

} SAMPLE_USER_INFO, *PSAMPLE_USER_INFO;

typedef struct _SAMPLE_ENUM_HANDLE
{
	BOOLEAN bEnumMembers;

	LSA_OBJECT_TYPE type;
	LONG64 llTotalCount;
	LONG64 llNextIndex;

	PSTR   pszSID;

} SAMPLE_ENUM_HANDLE, *PSAMPLE_ENUM_HANDLE;

typedef struct _SAMPLE_AUTH_PROVIDER_GLOBALS
{
	pthread_rwlock_t   mutex_rw;
	pthread_rwlock_t*  pMutex_rw;

	PSTR               pszDomain;

	PSAMPLE_GROUP_INFO* ppGroups;

	PSAMPLE_USER_INFO*  ppUsers;

	PLW_HASH_TABLE      pPasswordTable;

} SAMPLE_AUTH_PROVIDER_GLOBALS, *PSAMPLE_AUTH_PROVIDER_GLOBALS;
