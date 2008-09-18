/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
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
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "wbclient.h"

#define BAIL_ON_WBC_ERR(x) \
	do {		   \
		if (x != WBC_ERR_SUCCESS) {	\
			goto done;		\
		}				\
	} while (0);	

static bool split_name(const char *name, char **domain, char **account)
{
	char *pszCopy;	
	char *p;
	
	if ((pszCopy = strdup(name)) == NULL) {
		return false;
	}
	
	if ((p = strchr(pszCopy, '\\')) == NULL)  {
		free(pszCopy);		
		return false;
	}

	*p = '\0';
	
	*domain = strdup(pszCopy);
	*account = strdup(p+1);

	free(pszCopy);
	
	return true;
}
		
void PrintStructPasswd(struct passwd *pw)
{
	printf("%s:%s:%u:%u:%s:%s:%s\n",
	       pw->pw_name,
	       pw->pw_passwd,
	       (unsigned int)pw->pw_uid,
	       (unsigned int)pw->pw_gid,
	       pw->pw_gecos ? pw->pw_gecos : "",
	       pw->pw_dir,
	       pw->pw_shell);

	return;	
}

void PrintStructGroup(struct group *gr)
{
	int i = 0;
	
	printf("%s:%s:%u:",
	       gr->gr_name,
	       gr->gr_passwd,
	       (unsigned int)gr->gr_gid);	

	for (i=0; gr->gr_mem[i]; i++) {
		printf("%s,", gr->gr_mem[i]);
	}
	printf("\n");	

	return;	
}

static int LookupUser(char *name)
{
	int ret = 0;	
	struct passwd *pw;
	wbcErr wbc_status;
	uid_t uid;

	wbc_status = wbcGetpwnam(name, &pw);
	BAIL_ON_WBC_ERR(wbc_status);
	
	PrintStructPasswd(pw);
	uid = pw->pw_uid;	
	wbcFreeMemory(pw);
	pw = NULL;	

	wbc_status = wbcGetpwuid(uid, &pw);
	BAIL_ON_WBC_ERR(wbc_status);

	PrintStructPasswd(pw);
	wbcFreeMemory(pw);
	pw = NULL;	

done: 	
	if (!WBC_ERROR_IS_OK(wbc_status)) {
		printf("LookupUser: Result was %s\n", 
		       wbcErrorString(wbc_status));
		ret = 1;		
	}	
	
	if (pw) {		
		wbcFreeMemory(pw);
	}

	return ret;	
}

static int LookupGroup(char *name)
{
 	int ret = 0;	
	struct group *gr = NULL;
	wbcErr wbc_status;
	gid_t gid;
	
	wbc_status = wbcGetgrnam(name, &gr);
	BAIL_ON_WBC_ERR(wbc_status);
	
	PrintStructGroup(gr);
	gid = gr->gr_gid;	
	wbcFreeMemory(gr);
	gr = NULL;	

	wbc_status = wbcGetgrgid(gid, &gr);
	BAIL_ON_WBC_ERR(wbc_status);

	PrintStructGroup(gr);
	wbcFreeMemory(gr);
	gr = NULL;	

done: 	
	if (!WBC_ERROR_IS_OK(wbc_status)) {
		printf("LookupGroup: Result was %s\n", 
		       wbcErrorString(wbc_status));
		ret = 1;		
	}	
	
	if (gr) {		
		wbcFreeMemory(gr);		
	}

	return ret;	
}

int LookupUserGroups(const char *name)
{
	wbcErr wbc_status;
	uint32_t num_groups = 0;
	gid_t *gids = NULL;	
	int i;
	struct wbcDomainSid user_sid;
	enum wbcSidType type;
	struct wbcDomainSid *sidList = NULL;
	uint32_t num_sids = 0;	
	char *sid_string = NULL;	
	char *domain = NULL;
	char *account = NULL;
	
	wbc_status = wbcGetGroups(name, &num_groups, &gids);
	BAIL_ON_WBC_ERR(wbc_status);

	for (i=0; i<num_groups; i++) {
		struct group *gr = NULL;
		
		wbc_status = wbcGetgrgid(gids[i], &gr);
		BAIL_ON_WBC_ERR(wbc_status);

		printf("%s(%u)\n", gr->gr_name, (unsigned int)gr->gr_gid);
		wbcFreeMemory(gr);
		gr = NULL;		
	}

	if (!split_name(name, &domain, &account)) {
		printf("Failed to parse %s\n", name);
		return 1;
	}

	printf("\n");
	
	wbc_status = wbcLookupName(domain, account, &user_sid, &type);
	BAIL_ON_WBC_ERR(wbc_status);

	if (type != WBC_SID_NAME_USER) {
		wbc_status = WBC_ERR_INVALID_SID;
		BAIL_ON_WBC_ERR(wbc_status);
	}

	free(domain);
	free(account);
	domain = account = NULL;
	
	wbc_status = wbcLookupUserSids(&user_sid, true, &num_sids, &sidList);
	BAIL_ON_WBC_ERR(wbc_status);

	for (i=0; i<num_sids; i++) {
		wbc_status = wbcSidToString(&sidList[i], &sid_string);
		BAIL_ON_WBC_ERR(wbc_status);

		printf("%s", sid_string);
		wbcFreeMemory(sid_string);
		sid_string = NULL;

		wbc_status = wbcLookupSid(&sidList[i], &domain, &account, &type);
		BAIL_ON_WBC_ERR(wbc_status);

		printf(" (%s\\%s)\n", domain, account);

		wbcFreeMemory(domain);
		wbcFreeMemory(account);		
	}

done:
	if (gids) {
		wbcFreeMemory(gids);
	}
       
	return 0;	
}

static void MapSidToUid(const char *sid_string)
{
	wbcErr wbc_status;
	struct wbcDomainSid sid;	
	struct wbcDomainSid sid2;
	uid_t uid;
	char *sid_string2 = NULL;	

	memset(&sid, 0x0, sizeof(struct wbcDomainSid));
	memset(&sid2, 0x0, sizeof(struct wbcDomainSid));

	wbc_status = wbcStringToSid(sid_string, &sid);
	BAIL_ON_WBC_ERR(wbc_status);

	wbc_status = wbcSidToUid(&sid, &uid);
	BAIL_ON_WBC_ERR(wbc_status);

	wbc_status = wbcUidToSid(uid, &sid2);
	BAIL_ON_WBC_ERR(wbc_status);

	wbc_status = wbcSidToString(&sid2, &sid_string2);
	BAIL_ON_WBC_ERR(wbc_status);

	if (strcmp(sid_string, sid_string2) != 0) {
		printf("Sids do not match!\n");
		printf("  Original -> %s\n", sid_string);
		printf("  Derived  -> %s\n", sid_string2);
		goto done;
	}

	printf("%s <-> %u\n", sid_string2, (unsigned int)uid);	
	
done:
	if (wbc_status != WBC_ERR_SUCCESS) {
		printf("MapSidToUid Failed (%s)\n", wbcErrorString(wbc_status));		
	}
	
	if (sid_string2)
		wbcFreeMemory(sid_string2);
	
	return;	
}

static void MapSidToGid(const char *sid_string)
{
	wbcErr wbc_status;
	struct wbcDomainSid sid;	
	struct wbcDomainSid sid2;
	gid_t gid;
	char *sid_string2 = NULL;

	memset(&sid, 0x0, sizeof(struct wbcDomainSid));
	memset(&sid2, 0x0, sizeof(struct wbcDomainSid));

	wbc_status = wbcStringToSid(sid_string, &sid);
	BAIL_ON_WBC_ERR(wbc_status);

	wbc_status = wbcSidToGid(&sid, &gid);
	BAIL_ON_WBC_ERR(wbc_status);

	wbc_status = wbcGidToSid(gid, &sid2);
	BAIL_ON_WBC_ERR(wbc_status);

	wbc_status = wbcSidToString(&sid2, &sid_string2);
	BAIL_ON_WBC_ERR(wbc_status);

	if (strcmp(sid_string, sid_string2) != 0) {
		printf("Sids do not match!\n");
		printf("  Original -> %s\n", sid_string);
		printf("  Derived  -> %s\n", sid_string2);
		goto done;
	}

	printf("%s <-> %u\n", sid_string2, (unsigned int)gid);	
	
done:
	if (wbc_status != WBC_ERR_SUCCESS) {
		printf("MapSidToGid Failed (%s)\n", wbcErrorString(wbc_status));		
	}
	
	if (sid_string2)
		wbcFreeMemory(sid_string2);
	
	return;	
}


int LookupName(const char *name)
{
 	int ret = 0;	
	wbcErr wbc_status;
	char *domain = NULL;
	char *account = NULL;
	struct wbcDomainSid sid;
	char *sid_string;
	enum wbcSidType type;	

	if (!split_name(name, &domain, &account)) {
		return 1;
	}

	wbc_status = wbcLookupName(domain, account, &sid, &type);
	BAIL_ON_WBC_ERR(wbc_status);

	wbc_status = wbcSidToString(&sid, &sid_string);
	BAIL_ON_WBC_ERR(wbc_status);

	printf("%s (%d)\n", sid_string, type);
	
	free(domain);
	free(account);
	
	domain = account = NULL;
	
	wbc_status = wbcLookupSid(&sid, &domain, &account, &type);	
	BAIL_ON_WBC_ERR(wbc_status);

	printf("%s\\%s (%d)\n", domain, account, type);

	switch (type) {
	case WBC_SID_NAME_USER:
		MapSidToUid(sid_string);		
		break;
	case WBC_SID_NAME_DOM_GRP:
		MapSidToGid(sid_string);		
		break;
	default:
		printf("unknown SID type\n");		
	}
	
	
	wbcFreeMemory(sid_string);
	wbcFreeMemory(domain);
	wbcFreeMemory(account);	

	sid_string = domain = account = NULL;	

	ret = 0;
	
done:
	if (!WBC_ERROR_IS_OK(wbc_status)) {
		printf("LookupName: Result was %s\n", 
		       wbcErrorString(wbc_status));
		ret = 1;		
	}	

	if (domain)
		free(domain);
	if (account)
		free(account);

	return ret;	
}

static int InterfaceDetails(void)
{
	struct wbcInterfaceDetails *iface = NULL;
	wbcErr wbc_status = WBC_ERR_UNKNOWN_FAILURE;

	wbc_status = wbcInterfaceDetails(&iface);
	BAIL_ON_WBC_ERR(wbc_status);

	printf("Interface Version = %d\n", iface->interface_version);
	printf("Winbind Version   = %s\n", iface->winbind_version);
	printf("Separator         = %c\n", iface->winbind_separator);
	printf("My name           = %s\n", iface->netbios_name);
	printf("My domain         = %s\n", iface->netbios_domain);
	printf("My DNS domain     = %s\n", iface->dns_domain);

done:
	if (iface)
		wbcFreeMemory(iface);

	return 0;
}

static int ListGroups(void)
{
	wbcErr wbc_status = WBC_ERR_UNKNOWN_FAILURE;
	const char **ppszGroupList = NULL;
	uint32_t nGroups = 0;
	int i;	
	
	wbc_status = wbcListGroups(NULL, &nGroups, &ppszGroupList);	
	BAIL_ON_WBC_ERR(wbc_status);

	for (i=0; i<nGroups; i++) {
		printf("%s\n", ppszGroupList[i]);
	}

done:

	if (ppszGroupList)
		wbcFreeMemory(ppszGroupList);
	
	return 0;	

}


static int ListUsers(void)
{
	wbcErr wbc_status = WBC_ERR_UNKNOWN_FAILURE;
	const char **ppszUserList = NULL;
	uint32_t nUsers = 0;
	int i;	
	
	wbc_status = wbcListUsers(NULL, &nUsers, &ppszUserList);	
	BAIL_ON_WBC_ERR(wbc_status);

	for (i=0; i<nUsers; i++) {
		printf("%s\n", ppszUserList[i]);
	}

done:

	if (ppszUserList)
		wbcFreeMemory(ppszUserList);
	
	return 0;	

}

static int ParseArgs(int argc, char *argv[])
{
	int i = 0;

	if (argc < 1)
		return 1;
	
	while (i < argc) {
		if (*argv[i] != '-')
			return 1;

		if (strcmp(argv[i], "--getgrnam") == 0) {
			if (i+1 >= argc)
				return 1;
			i++;
			LookupGroup(argv[i++]);
			continue;			
		}

		if (strcmp(argv[i], "--getpwnam") == 0) {
			if (i+1 >= argc)
				return 1;
			i++;
			LookupUser(argv[i++]);
			continue;			
		}

		if (strcmp(argv[i], "--usergroups") == 0) {
			if (i+1 >= argc)
				return 1;
			i++;
			LookupUserGroups(argv[i++]);
			continue;			
		}

		if (strcmp(argv[i], "--lookupname") == 0) {
			if (i+1 >= argc)
				return 1;
			i++;
			LookupName(argv[i++]);
			continue;			
		}

		if (strcmp(argv[i], "--interface") == 0) {
			i++;
			InterfaceDetails();
			continue;			
		}

		if (strcmp(argv[i], "--listgroups") == 0) {
			i++;
			ListGroups();
			continue;			
		}

		if (strcmp(argv[i], "--listusers") == 0) {
			i++;
			ListUsers();
			continue;
		}

		/* Unparseable option */

		return 1;
	}

	return 0;	
}


static void PrintUsage(const char *program)
{
	printf("%s [options]\n", program);
	printf("\t--getpwnam <user>        Perform forward and reverse lookups of a user\n");
	printf("\t--getgrnam <group>       Perform forward and reverse lookups of a group\n");
	printf("\t--usergroups <user>      Print a list of the user's group membership\n");
	printf("\t--lookupname <name>      Convert a name to a SID\n");
	printf("\t--interface              Print out Library interface details\n");
	printf("\t--listgroups             List the groups in all know domains\n");	
	printf("\t--listusers              List the users in all know domains\n");	
	printf("\n");

	return;
}


int main(int argc, char *argv[])
{
	int ret;
	
	/* Parse options starting at first real parameters */

	if ((ret = ParseArgs(argc-1, &argv[1])) != 0) {
		PrintUsage(argv[0]);
	}

	return ret;	
}
