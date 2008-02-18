/* 
   Additional auth support routines for OS X.

   Copyright (C) Danilo Almeida 2007
   
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
   
   You should have received a copy of the GNU Library General Public
   License along with this library; if not, write to the
   Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA  02111-1307, USA.   
*/

#include "winbind_client.h"
#include "wbl.h"

/* For mac_ds_get_user_groups */
#define AUTH_FAILURE -1
#define AUTH_SUCCESS 0

/* For mac_ds_authenticate and mac_ds_change_password */
/* TODO: Put this in common file (wbl.h?) */
#ifndef PAM_WINBIND_CONFIG_FILE
#define PAM_WINBIND_CONFIG_FILE "/etc/security/pam_lwidentity.conf"
#endif

WBL_STATUS mac_ds_authenticate(const char *username, const char *password,
			       bool is_auth_only,
			       WBL_LOG_CALLBACK log_function,
			       WBL_LOG_CALLBACK message_function,
			       void* context)
{
	WBL_STATUS wblStatus;
	WBL_STATE* state = NULL;

	wblStatus = WblStateCreate(&state, log_function, message_function,
				   context, NULL, PAM_WINBIND_CONFIG_FILE, 0, NULL);
	if (wblStatus) {
		goto cleanup;
	}

	wblStatus = WblAuthenticate(state, username, password);
	if (wblStatus) {
		goto cleanup;
	}

	wblStatus = WblCreateHomeDirectory(state, username);
	if (wblStatus) {
		goto cleanup;
	}

	wblStatus = WblCreateK5Login(state, username, NULL);
	if (wblStatus) {
		goto cleanup;
	}

cleanup:
	if (state) {
		WblStateRelease(state);
	}

	return wblStatus;
}

WBL_STATUS mac_ds_change_password(const char *username,
				  const char *old_password,
				  const char *password,
				  WBL_LOG_CALLBACK log_function,
				  WBL_LOG_CALLBACK message_function,
				  void* context)
{
	WBL_STATUS wblStatus;
	WBL_STATE* state = NULL;

	wblStatus = WblStateCreate(&state, log_function, message_function,
				   context, NULL, PAM_WINBIND_CONFIG_FILE, 0, NULL);
	if (wblStatus) {
		goto cleanup;
	}

	wblStatus = WblChangePassword(state, username, old_password, password);

cleanup:
	if (state) {
		WblStateRelease(state);
	}

	return wblStatus;
}

WBL_STATUS mac_ds_get_principal(const char *username,
				char** principal_name,
				WBL_LOG_CALLBACK log_function,
				WBL_LOG_CALLBACK message_function,
				void* context)
{
	WBL_STATUS wblStatus;
	WBL_STATE* state = NULL;
	const char* result = NULL;

	wblStatus = WblStateCreate(&state, log_function, message_function,
				   context, NULL, PAM_WINBIND_CONFIG_FILE, 0, NULL);
	if (wblStatus) {
		goto cleanup;
	}

	wblStatus = WblQueryUserPrincipalName(state, username, &result);
	if (wblStatus) {
		goto cleanup;
	}

	/* the principal name is part of the state, so dup it */
	*principal_name = strdup(result);
	if (!*principal_name) {
		wblStatus = WBL_STATUS_MEMORY_INSUFFICIENT;
		goto cleanup;
	}

	wblStatus = WBL_STATUS_OK;

cleanup:
	if (state) {
		WblStateRelease(state);
	}

	if (wblStatus) {
		*principal_name = NULL;
	}

	return wblStatus;
}

void mac_ds_free_principal(char* principal_name)
{
	if (principal_name) {
		free(principal_name);
	}
}

/* This is essentially a more streamlined version of NSS's initgroups_dyn */
int mac_ds_get_user_groups(const char *user, gid_t **groups, int *num_groups)
{
	int result;
	NSS_STATUS ret;
	struct winbindd_request request;
	struct winbindd_response response = { 0 };
	int i;
	int num_gids;
	gid_t *gid_list;

#ifdef DEBUG_NSS
	fprintf(stderr, "[%5d]: getgroups(%s)\n", getpid(), user);
#endif

	ZERO_STRUCT(request);
	ZERO_STRUCT(response);

	strncpy(request.data.username, user,
		sizeof(request.data.username) - 1);

	ret = winbindd_request_response(WINBINDD_GETGROUPS, &request, &response);
	if (ret != NSS_STATUS_SUCCESS) {
		/* Make sure that we do not try to free if we failed */
		memset(&response, 0, sizeof(response));
		result = AUTH_FAILURE;
		goto done;
	}

	num_gids = response.data.num_entries;
	gid_list = (gid_t *)response.extra_data.data;

#ifdef DEBUG_NSS
	fprintf(stderr, "[%5d]: getgroups(%s): got NSS_STATUS_SUCCESS "
			"and %d gids\n", getpid(),
			user, num_gids);
#endif
	if (gid_list == NULL) {
		result = AUTH_FAILURE;
		goto done;
	}

	*groups = malloc(num_gids * sizeof(**groups));
	if (!*groups) {
		result = AUTH_FAILURE;
		goto done;
	}

	memcpy(*groups, gid_list, sizeof(gid_list[0]) * num_gids);
	*num_groups = num_gids;

	result = AUTH_SUCCESS;

done:
	winbindd_free_response(&response);

#ifdef DEBUG_NSS
	fprintf(stderr, "[%5d]: getgroups(%s) returns %d\n", getpid(),
		user, result);
#endif
	return result;
}

