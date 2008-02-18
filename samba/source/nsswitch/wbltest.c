/* 
   Test code for WBL.

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
#include <termios.h>

#ifndef PAM_WINBIND_CONFIG_FILE
#define PAM_WINBIND_CONFIG_FILE "/etc/security/pam_lwidentity.conf"
#endif

WBL_STATUS test_authenticate(const char *username, const char *password,
			bool do_more,
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

	if (do_more) {
		wblStatus = WblCreateHomeDirectory(state, username);
		if (wblStatus) {
			goto cleanup;
		}

		wblStatus = WblCreateK5Login(state, username, NULL);
		if (wblStatus) {
			goto cleanup;
		}
	}

cleanup:
	if (state) {
		WblStateRelease(state);
	}

	return wblStatus;
}

WBL_STATUS test_change_password(const char *username,
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

WBL_STATUS test_principal(const char *username,
			  char** principal,
			  WBL_LOG_CALLBACK log_function,
			  WBL_LOG_CALLBACK message_function,
			  void* context)
{
	WBL_STATUS wblStatus;
	WBL_STATE* state = NULL;
	const char* const_principal;

	wblStatus = WblStateCreate(&state, log_function, message_function,
				   context, NULL, PAM_WINBIND_CONFIG_FILE, 0, NULL);
	if (wblStatus) {
		goto cleanup;
	}

	wblStatus = WblQueryUserPrincipalName(state, username, &const_principal);
	if (wblStatus) {
		goto cleanup;
	}

	*principal = strdup(const_principal);
	if (!*principal) {
		wblStatus = WBL_STATUS_MEMORY_INSUFFICIENT;
		goto cleanup;
	}

cleanup:
	if (state) {
		WblStateRelease(state);
	}

	return wblStatus;}


void LogCallback(void* context, WBL_LOG_LEVEL level, const char* format, va_list args)
{
	char* alloc_output = NULL;
	char* fail_output = "*** Failed to allocate memory while formatting log message ***";
	char* output;

	vasprintf(&alloc_output, format, args);
	output = alloc_output ? alloc_output : fail_output;
	printf("[%d]: %s\n", level, output);
	if (alloc_output) {
		free(alloc_output);
	}
}


char* get_password(const char* prompt)
{
	char buffer[129] = { 0 };
	int index = 0;
	struct termios old_termios;
	struct termios new_termios;
	char ch;
	char* password = NULL;

	if (prompt) {
		printf("%s", prompt);
		fflush(stdout);
	}

	tcgetattr(0, &old_termios);
	memcpy(&new_termios, &old_termios, sizeof(old_termios));
	new_termios.c_lflag &= ~(ECHO);
	tcsetattr(0, TCSANOW, &new_termios);

	while (index < sizeof(buffer) - 1) {
		if (read(0, &ch, 1)) {
			if (ch != '\n') {
				buffer[index++] = ch;
			} else {
				break;
			}
		} else {
			if (errno) {
				goto cleanup;
			}
		}
	}

	if (index == sizeof(buffer)) {
		goto cleanup;
	}

	if (index > 0) {
		password = strdup(buffer);
	}
cleanup:
	tcsetattr(0, TCSANOW, &old_termios);

	return password;
}

void usage(const char* program)
{
	printf("usage: %s auth <username>\n", program);
	printf("       %s password <username>\n", program);
	printf("       %s principal <username>\n", program);
	exit(1);
}

int main(int argc, char** argv)
{
	WBL_STATUS status = WBL_STATUS_ERROR;
	char* command = NULL;
	char* username = NULL;
	char* password = NULL;
	char* old_password = NULL;
	char* verify_password = NULL;
	char* principal = NULL;

	if (argc < 3) {
		usage(argv[0]);
		goto cleanup;
	}

	command = argv[1];
	username = argv[2];

	if (!strcmp(command, "auth")) {
		password = get_password("Password: ");
		if (!password) {
			printf("failed to get password\n");
			goto cleanup;
		}
		
		status = test_authenticate(username, password, false, LogCallback, LogCallback, NULL);
	} else if (!strcmp(command, "password")) {
		old_password = get_password("Current Password: ");
		if (!old_password) {
			printf("failed to get password\n");
			goto cleanup;
		}

		password = get_password("New Password: ");
		if (!password) {
			printf("failed to get password\n");
			goto cleanup;
		}

		verify_password = get_password("Verify Password: ");
		if (!verify_password) {
			printf("failed to get password\n");
			goto cleanup;
		}

		if (strcmp(password, verify_password)) {
			printf("Passwords do not match!\n");
			goto cleanup;
		}

		status = test_change_password(username, old_password, password, LogCallback, LogCallback, NULL);
	} else if (!strcmp(command, "principal")) {
		status = test_principal(username, &principal, LogCallback, LogCallback, NULL);
		if (status) {
			goto cleanup;
		}
		printf("Principal is '%s'\n", principal);
	} else {
		usage(argv[0]);
		goto cleanup;
	}

cleanup:
	if (password) {
		free(password);
	}
	if (old_password) {
		free(old_password);
	}
	if (verify_password) {
		free(verify_password);
	}
	if (principal) {
		free(principal);
	}
	printf("Exit code: %d\n", status);
	return status;
}

