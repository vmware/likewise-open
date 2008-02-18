/* 
   Unix SMB/CIFS implementation.

   Winbind daemon machine password updater

   Copyright (C) 2007 Danilo Almeida (dalmeida@centeris.com)

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  
*/

#include "includes.h"
#include "winbindd.h"
#include <iniparser.h>

#define POLICY_NOT_SET_MACHINE_PASSWORD_TIMEOUT (-1)

typedef struct {
	int machine_password_timeout;
} policy_t;

typedef struct {
	struct timed_event* expire_password_event;
	struct timed_event* refresh_policy_event;
	time_t policy_mod_time;
	policy_t policy;
	bool need_retry_change;
	bool is_registered;
} expire_password_state_t;

#define EXPIRE_PASSWORD_EVENT_NAME "expire_password_handler (machine password)"
#define REFRESH_POLICY_EVENT_NAME  "refresh_policy_handler (machine password)"

#define CHANGE_PASSWORD_RETRY_INTERVAL (6 * 60 * 60) /* 6 hours */
#define MIN_POLICY_REFRESH_INTERVAL (1 * 60) /* 1 minute */

static expire_password_state_t expire_password_state = { 0 };

static expire_password_state_t *get_state(void)
{
	return &expire_password_state;
}

static void init_empty_policy(policy_t *policy)
{
	ZERO_STRUCTP(policy);
	policy->machine_password_timeout = POLICY_NOT_SET_MACHINE_PASSWORD_TIMEOUT;
}

#define INVALID_KEY ((char*)-1)

static bool policy_getint(dictionary *dict, const char *key, int *result)
/**
 * Get an integer value from the policy dictionary.
 *
 * @param dict [IN] Dictionary representing policy file.
 * @param key [IN] Key in the dictionary.
 * @param result [IN/OUT] Returns result, if found.
 *
 * @return False on error (allocating memeory).
 * True on success, which includes leaving result alone
 * because there is no setting present for the key.
 */
{
	char *str;

	str = iniparser_getstring(dict, key, INVALID_KEY);
	if (!str) {
		DEBUG(0, ("Failure to read policy key: %s\n", key));
		return False;
	}
	else if (str == INVALID_KEY) {
		/* leave result alone */
		return True;
	}

	*result = atoi(str);

	return True;
}

static bool refresh_policy(policy_t *policy, time_t* mod_time)
/**
 * Refresh policy settings.
 *
 * @param policy [IN/OUT] Policy blob to update.  (Not modified on failure.)
 * @param mod_time [IN/OUT] Policy file mod_time.  (Not modified on failure.)
 *
 * @return success (True/False).
 */
{
	bool success = False;
	const char *policy_file = lp_policy_file();
	time_t old_mod_time = *mod_time;
	time_t new_mod_time = 0;
	dictionary *dict = NULL;
	policy_t new_policy;

	if (!policy_file || !file_exist(policy_file, NULL)) {
		DEBUG(3, ("No policy in effect (%s)\n", policy_file ? policy_file : "n/a"));
		init_empty_policy(policy);
		success = True;
		goto done;
	}

	/* Read modification time _before_ reading the file */
	new_mod_time = file_modtime(policy_file);
	if (!new_mod_time) {
		DEBUG(0, ("Failed to get modification time for %s\n", policy_file));
		goto done;
	}

	if (new_mod_time == old_mod_time) {
		DEBUG(10, ("Policy file %s not modified\n", policy_file));
		success = True;
		goto done;
	}

	dict = iniparser_load(policy_file);
	if (!dict) {
		DEBUG(0, ("Failure to read polcify file %s\n", policy_file));
		goto done;
	}

	init_empty_policy(&new_policy);

	if (!policy_getint(dict, "global:machine password timeout", &new_policy.machine_password_timeout)) {
		goto done;
	}

	DEBUG(3, ("Policy from \"%s\": machine password timeout = %d\n",
		  policy_file, new_policy.machine_password_timeout));
	

	*policy = new_policy;
	success = True;

done:
	if (dict) {
		iniparser_freedict(dict);
	}

	if (success) {
		*mod_time = new_mod_time;
	}

	return success;
}

static time_t get_policy_refresh_time(void)
{
	return MAX(lp_policy_refresh_interval(), MIN_POLICY_REFRESH_INTERVAL);
}

#define KRB5CCNAME_ENV_VAR "KRB5CCNAME"
#define KRB5CCNAME "MEMORY:winbind_change_password_ccache"

static bool change_machine_password(void)
{
	bool success = False;
	ADS_STATUS ads_status;
	ADS_STRUCT *ads = NULL;
	char *machine_principal = NULL;
	const char* old_ccache;

	/* set up own own password cache so as not to interfere with
	   anything else. */
	old_ccache = getenv(KRB5CCNAME_ENV_VAR);
	if (setenv(KRB5CCNAME_ENV_VAR, KRB5CCNAME, 1) < 0) {
		DEBUG(1, ("Failed to set ccache location.\n"));
		goto done;
	}

	ads = ads_init(lp_realm(), lp_workgroup(), NULL);
	if (!ads) {
		DEBUG(0, ("Failed to initialize ADS connection.\n"));
		goto done;
	}

	ads->auth.password = secrets_fetch_machine_password(lp_workgroup(), NULL, NULL);
	ads->auth.realm = SMB_STRDUP(lp_realm());
	strupper_m(ads->auth.realm);
	/* ads->auth.user_name is automatically populated by ads_connect() */

	ads_kdestroy(KRB5CCNAME);

	ads_status = ads_connect(ads);
	if (!ADS_ERR_OK(ads_status)) {
		DEBUG(1, ("Failed to connect to DC: %s\n", ads_errstr(ads_status)));
		goto done;
	}

	if (asprintf(&machine_principal, "%s$@%s", global_myname(), ads->auth.realm) < 0) {
		DEBUG(0, ("Failed to allocate UPN.\n"));
		goto done;
	}

	ads_status = ads_change_trust_account_password(ads, machine_principal);
	if (!ADS_ERR_OK(ads_status)) {
		DEBUG(0, ("Machine password change failed: %s\n", ads_errstr(ads_status)));
		goto done;
	}
    
	if (lp_use_kerberos_keytab()) {
		DEBUG(3, ("Attempting to update system keytab with new password.\n"));
		if (ads_keytab_create_default(ads)) {
			DEBUG(0, ("Failed to update system keytab.\n"));
			/* ignore error and pretend success */
		}
	}

	success = True;

	/* Note that we leave any existing ticket caches along because
	   they will continue to be valid despite the password having change.
	   That is part of the beauty of Kerberos. */
 

 done:
	if (success) {
		DEBUG(3, ("Machine password change succeeded.\n"));
	}

	if (ads) {
		ads_destroy(&ads);
	}

	if (machine_principal) {
		SAFE_FREE(machine_principal);
	}

	ads_kdestroy(KRB5CCNAME);

	if (old_ccache) {
		if (setenv(KRB5CCNAME_ENV_VAR, old_ccache, 1) < 0) {
			smb_panic("Failed to restore environment");
		}
	} else {
#ifdef HAVE_SETENV
		unsetenv(KRB5CCNAME_ENV_VAR);
#else
		/* Do nothing -- unsetenv() cannot be implemented in terms
		   of putenv(), so lib/replace does not implement it.
		   So we just leave the variable as is.   Note that his
		   is safe as we will revert the variable if it was set
		   (old_ccache). */
#endif
	}

	return success;
}

static time_t get_expire_timeout(void)
{
	if (get_state()->policy.machine_password_timeout !=  POLICY_NOT_SET_MACHINE_PASSWORD_TIMEOUT) {
		return get_state()->policy.machine_password_timeout;
	}
	return lp_machine_password_timeout();
}

static bool get_mod_time(const char* domain_name, time_t *mod_time)
{
	bool success = False;
	char *password = NULL;
	uint32 channel;

	password = secrets_fetch_machine_password(domain_name, mod_time, &channel);
	if (!password) {
		DEBUG(0, ("Failure to read machine password\n"));
		goto done;
	}

	if (*mod_time <= 0) {
		DEBUG(0, ("Invalid timestamp for machine password\n"));
		goto done;
	}

	success = True;

 done:
	if (password) {
		SAFE_FREE(password);
	}

	if (!success) {
		*mod_time = 0;
	}

	return success;
}

static bool change_machine_password_and_compute_next(time_t now_time, time_t *mod_time)
{
	bool success = False;

	success = change_machine_password();
	if (!success) {
		DEBUG(0, ("Failed to change machine password.\n"));
		goto done;
	}

	/* Get the current mod_time so we can re-schedule.  We get the
	   udpated mod_time recorded in the new password to make sure
	   that we schedule correctly as opposed to using the current
	   now_time. */
	if (!get_mod_time(lp_workgroup(), mod_time)) {
		DEBUG(3, ("Using current time as mod time\n"));
		*mod_time = now_time;
	}

done:
	return success;
}

static void expire_password_handler(struct event_context *event_ctx, struct timed_event *te, const struct timeval *now, void *private_data)
{
	expire_password_state_t *state = (expire_password_state_t *) private_data;

	bool success = False;
	time_t timeout;
	time_t now_time = now->tv_sec;
	time_t mod_time;
	time_t expire_time;
	time_t remaining_time;
	bool can_expire = True;
	bool need_retry_schedule = False;

	timeout = get_expire_timeout();
	if (timeout <= 0) {
		DEBUG(3, ("Machine password does not expire.\n"));
		can_expire = False;
	}

	if (state->need_retry_change) {
		DEBUG(3, ("Retrying from previous attempt.\n"));
		success = change_machine_password_and_compute_next(now_time, &mod_time);
		if (success) {
			state->need_retry_change = False;
		}
		goto done;
	}

	if (!can_expire) {
		success = True;
		goto done;
	}

	/* Check whether we are expired */

	if (!get_mod_time(lp_workgroup(), &mod_time)) {
		DEBUG(0, ("Could not get machine password mod time.\n"));
		need_retry_schedule = True;
		goto done;
	}

	expire_time = mod_time + timeout;
	remaining_time = expire_time - now_time;

	DEBUG(3, ("now = %ld, mod = %ld, timeout = %ld, expire = %ld, remaining = %ld.\n", now_time, mod_time, timeout, expire_time, remaining_time));

	if (remaining_time > 0) {
		DEBUG(3, ("Machine password had not yet expired (%ld seconds remaining).\n", (long) remaining_time));
		success = True;
		goto done;
	}

	DEBUG(3, ("Machine password has expired (%ld seconds ago).\n", remaining_time));

	success = change_machine_password_and_compute_next(now_time, &mod_time);
	if (!success) {
		state->need_retry_change = True;
		goto done;
	}

done:
	if (state->need_retry_change || need_retry_schedule) {
		DEBUG(3, ("Retrying in %d seconds.\n", CHANGE_PASSWORD_RETRY_INTERVAL));
		SMB_ASSERT(!success);
		evt_reschedule_timed_event(te, timeval_current_ofs(CHANGE_PASSWORD_RETRY_INTERVAL, 0));
	} else if (!can_expire) {
		DEBUG(3, ("Unscheduling since password does not expire.\n"));
		SMB_ASSERT(success);
		evt_unschedule_timed_event(te);
	} else {
		expire_time = mod_time + timeout;
		remaining_time = expire_time - now_time;
		DEBUG(3, ("Scheduling next in about %ld seconds.\n", remaining_time));
		evt_reschedule_timed_event(te, timeval_set(expire_time, 0));
	}
}

static void refresh_policy_handler(struct event_context *event_ctx, struct timed_event *te, const struct timeval *now, void *private_data)
{
	expire_password_state_t *state = (expire_password_state_t *) private_data;
	time_t old_mod_time = state->policy_mod_time;

	if (!refresh_policy(&state->policy, &state->policy_mod_time)) {
		DEBUG(0, ("Could not refresh policy\n"));
	}
	else if (old_mod_time != state->policy_mod_time) {
		/* Something changed, so re-queue expire handler. */
		evt_reschedule_timed_event(state->expire_password_event,
					   timeval_current());
	}

	evt_reschedule_timed_event(te, timeval_current_ofs(get_policy_refresh_time(), 0));
}


bool init_expire_machine_password_events(struct event_context *event_ctx)
{
	bool success = False;
	expire_password_state_t *state = get_state();

	if (state->is_registered) {
		DEBUG(0, ("Expire machine password events already registered\n"));
		smb_panic("expire machine password events already registered");
		goto done;
	}

	if ( (lp_server_role() != ROLE_DOMAIN_MEMBER) || 
	     (lp_security() != SEC_ADS) )
	{
		/* If not an ADS domain member, do not fail, but make registration a no-op. */
		state->is_registered = True;
		success = True;
		goto done;
	}

	if (!refresh_policy(&state->policy, &state->policy_mod_time)) {
		DEBUG(0, ("Could not initialize policy\n"));
		goto done;
	}

	/* Let the event handler re-schedule itself for the appropriate time */
	state->expire_password_event = evt_create_scheduled_timed_event(event_ctx,
									NULL,
									timeval_current(),
									EXPIRE_PASSWORD_EVENT_NAME,
									expire_password_handler,
									state);
	if (!state->expire_password_event) {
		DEBUG(0, ("Failed to create expire password event\n"));
		goto done;
	}

	state->refresh_policy_event = evt_create_scheduled_timed_event(event_ctx,
								       NULL,
								       timeval_current_ofs(get_policy_refresh_time(), 0),
								       REFRESH_POLICY_EVENT_NAME,
								       refresh_policy_handler,
								       state);
	if (!state->refresh_policy_event) {
		DEBUG(0, ("Failed to create refresh policy event\n"));
		goto done;
	}

	state->is_registered = True;
	success = True;

	DEBUG(0, ("Registered password events.\n"));

 done:
	if (!success) {
		evt_destroy_timed_event(state->expire_password_event);
		evt_destroy_timed_event(state->refresh_policy_event);
		ZERO_STRUCTP(state);
	}

	return success;
}
