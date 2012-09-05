/*
 * Copyright (C) VMware. All rights reserved.
 */

#include "includes.h"

PCSTR gpszSampleProviderName = LSA_PROVIDER_TAG_SAMPLE;

LSA_PROVIDER_FUNCTION_TABLE gSampleProviderAPITable =
{
    .pfnFindObjects             = &SampleFindObjects,
    .pfnOpenEnumObjects         = &SampleOpenEnumObjects,
    .pfnEnumObjects             = &SampleEnumObjects,
    .pfnOpenEnumGroupMembers    = &SampleOpenEnumMembers,
    .pfnEnumGroupMembers        = &SampleEnumMembers,
    .pfnCloseEnum               = &SampleCloseEnum,
    .pfnQueryMemberOf           = &SampleQueryMemberOf,
    .pfnGetSmartCardUserObject  = &SampleGetSmartCardUserObject,
    .pfnGetMachineAccountInfoA  = &SampleGetMachineAccountInfoA,
    .pfnGetMachineAccountInfoW  = &SampleGetMachineAccountInfoW,
    .pfnGetMachinePasswordInfoA = &SampleGetMachinePasswordInfoA,
    .pfnGetMachinePasswordInfoW = &SampleGetMachinePasswordInfoW,
    .pfnShutdownProvider        = &SampleShutdownProvider,
    .pfnOpenHandle              = &SampleOpenHandle,
    .pfnCloseHandle             = &SampleCloseHandle,
    .pfnServicesDomain          = &SampleServicesDomain,
    .pfnAuthenticateUserPam     = &SampleAuthenticateUserPam,
    .pfnAuthenticateUserEx      = &SampleAuthenticateUserEx,
    .pfnValidateUser            = &SampleValidateUser,
    .pfnCheckUserInList         = &SampleCheckUserInList,
    .pfnChangePassword          = &SampleChangePassword,
    .pfnSetPassword             = &SampleSetPassword,
    .pfnAddUser                 = &SampleAddUser,
    .pfnModifyUser              = &SampleModifyUser,
    .pfnAddGroup                = &SampleAddGroup,
    .pfnModifyGroup             = &SampleModifyGroup,
    .pfnDeleteObject            = &SampleDeleteObject,
    .pfnOpenSession             = &SampleOpenSession,
    .pfnCloseSession            = &SampleCloseSession,
    .pfnLookupNSSArtefactByKey  = &SampleFindNSSArtefactByKey,
    .pfnBeginEnumNSSArtefacts   = &SampleBeginEnumNSSArtefacts,
    .pfnEnumNSSArtefacts        = &SampleEnumNSSArtefacts,
    .pfnEndEnumNSSArtefacts     = &SampleEndEnumNSSArtefacts,
    .pfnGetStatus               = &SampleGetStatus,
    .pfnFreeStatus              = &SampleFreeStatus,
    .pfnRefreshConfiguration    = &SampleRefreshConfiguration,
    .pfnProviderIoControl       = &SampleProviderIoControl
};

static SAMPLE_GROUP_INFO gSampleGroup_1 =
{
		.pszName = "SampleGroup_1",
		.gid     = 3001,
		.pszSID  = "S-1-5-21-3623811015-3361044348-30300820-3001",
		.ppszMembers = { "SampleUser_1", NULL }
};
static SAMPLE_GROUP_INFO gSampleGroup_2 =
{
		.pszName = "SampleGroup_2",
		.gid     = 3002,
		.pszSID  = "S-1-5-21-3623811015-3361044348-30300820-3002",
		.ppszMembers = { "SampleUser_1", "SampleUser_2", NULL }
};
static SAMPLE_GROUP_INFO gSampleGroup_3 =
{
		.pszName = "SampleGroup_3",
		.gid     = 3003,
		.pszSID  = "S-1-5-21-3623811015-3361044348-30300820-3003",
		.ppszMembers = { "SampleUser_1", "SampleUser_2", "SampleUser_3", NULL }
};
static SAMPLE_GROUP_INFO gSampleGroup_4 =
{
		.pszName = "SampleGroup_4",
		.gid     = 3004,
		.pszSID  = "S-1-5-21-3623811015-3361044348-30300820-3004",
		.ppszMembers = 	{	"SampleUser_1",
							"SampleUser_2",
							"SampleUser_3",
							"SampleUser_4",
							NULL
						}
};
static SAMPLE_GROUP_INFO gSampleGroup_5 =
{
		.pszName = "SampleGroup_5",
		.gid     = 3005,
		.pszSID  = "S-1-5-21-3623811015-3361044348-30300820-3005",
		.ppszMembers = 	{ 	"SampleUser_1",
							"SampleUser_2",
							"SampleUser_3",
							"SampleUser_4",
							"SampleUser_5",
							NULL
						}
};

static SAMPLE_USER_INFO gSampleUser_1 =
{
		.pszName          = "SampleUser_1",
		.uid              = 2001,
		.gid              = 3001,
		.pszShell         = SAMPLE_USER_SHELL,
		.pszHomedir       = "/home/SampleUser_1",
		.pszUPN           = NULL,
		.pszSID           = "S-1-5-21-3623811015-3361044348-30300820-2001",
		.pszGecos         = "SampleUser 1",
		.bPasswordExpired = FALSE,
		.bAccountLocked   = FALSE,
		.bAccountExpired  = FALSE,
		.bAccountDisabled = FALSE,
		.bUserCanChangePassword = TRUE
};

static SAMPLE_USER_INFO gSampleUser_2 =
{
		.pszName          = "SampleUser_2",
		.uid              = 2002,
		.gid              = 3001,
		.pszShell         = SAMPLE_USER_SHELL,
		.pszHomedir       = "/home/SampleUser_2",
		.pszUPN           = NULL,
		.pszSID           = "S-1-5-21-3623811015-3361044348-30300820-2002",
		.pszGecos         = "SampleUser 2",
		.bPasswordExpired = FALSE,
		.bAccountLocked   = FALSE,
		.bAccountExpired  = FALSE,
		.bAccountDisabled = FALSE,
		.bUserCanChangePassword = TRUE
};

static SAMPLE_USER_INFO gSampleUser_3 =
{
		.pszName          = "SampleUser_3",
		.uid              = 2003,
		.gid              = 3001,
		.pszShell         = SAMPLE_USER_SHELL,
		.pszHomedir       = "/home/SampleUser_3",
		.pszUPN           = NULL,
		.pszSID           = "S-1-5-21-3623811015-3361044348-30300820-2003",
		.pszGecos         = "SampleUser 3",
		.bPasswordExpired = FALSE,
		.bAccountLocked   = FALSE,
		.bAccountExpired  = FALSE,
		.bAccountDisabled = TRUE,
		.bUserCanChangePassword = TRUE
};

static SAMPLE_USER_INFO gSampleUser_4 =
{
		.pszName          = "SampleUser_4",
		.uid              = 2004,
		.gid              = 3001,
		.pszShell         = SAMPLE_USER_SHELL,
		.pszHomedir       = "/home/SampleUser_4",
		.pszUPN           = NULL,
		.pszSID           = "S-1-5-21-3623811015-3361044348-30300820-2004",
		.pszGecos         = "SampleUser 4",
		.bPasswordExpired = FALSE,
		.bAccountLocked   = TRUE,
		.bAccountExpired  = FALSE,
		.bAccountDisabled = FALSE,
		.bUserCanChangePassword = TRUE
};

static SAMPLE_USER_INFO gSampleUser_5 =
{
		.pszName          = "SampleUser_5",
		.uid              = 2005,
		.gid              = 3001,
		.pszShell         = SAMPLE_USER_SHELL,
		.pszHomedir       = "/home/SampleUser_1",
		.pszUPN           = NULL,
		.pszSID           = "S-1-5-21-3623811015-3361044348-30300820-2005",
		.pszGecos         = "SampleUser 5",
		.bPasswordExpired = TRUE,
		.bAccountLocked   = FALSE,
		.bAccountExpired  = FALSE,
		.bAccountDisabled = FALSE,
		.bUserCanChangePassword = TRUE
};

static PSAMPLE_GROUP_INFO gSampleGroupList[] =
{
	&gSampleGroup_1,
	&gSampleGroup_2,
	&gSampleGroup_3,
	&gSampleGroup_4,
	&gSampleGroup_5,
	NULL
};

static PSAMPLE_USER_INFO gSampleUserList[] =
{
	&gSampleUser_1,
	&gSampleUser_2,
	&gSampleUser_3,
	&gSampleUser_4,
	&gSampleUser_5,
	NULL
};

SAMPLE_AUTH_PROVIDER_GLOBALS gSampleAuthProviderGlobals =
{
	.pMutex_rw      = NULL,
	.pszDomain      = SAMPLE_DOMAIN,
	.ppGroups       = &gSampleGroupList[0],
	.ppUsers        = &gSampleUserList[0],
	.pPasswordTable = NULL
};

