/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
* -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software    2007-2008
 * All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <glade/glade.h>
#include <gtk/gtk.h>

#include <time.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>

#include "joindialog.h"
#include "joinprogress.h"
#include "joinerror.h"
#include "joinauth.h"
#include "statusdialog.h"

#include <djapi.h>
#include <djlogger.h>
#include <djmodule.h>
#include <djhostinfo.h>
#include <ctstrutils.h>

FILE* log_handle;

JoinDialog* global_join_dialog = NULL;
StatusDialog* global_status_dialog = NULL;


// This is a hack to prevent SCIM from being
// reloaded when we switch dialogs
static gboolean
close_stale_dialogs(gpointer data)
{
    if (global_join_dialog)
    {
	joindialog_delete(global_join_dialog);
	global_join_dialog = NULL;
    }

    if (global_status_dialog)
    {
	statusdialog_delete(global_status_dialog);
	global_status_dialog = NULL;
    }

    return FALSE;
}

static void
log_begin()
{
    log_handle = tmpfile();

    if (!log_handle)
    {
	fprintf(stderr, "Could not open log file: %s\n", strerror(errno));
	exit(1);
    }

    // Log to file
    dj_init_logging_to_file_handle(LOG_LEVEL_VERBOSE, log_handle);
    // Also log stderr to file
    dup2(fileno(log_handle), 2);
}

static void
log_end()
{
    dj_close_log();
    log_handle = NULL;
}

static void
log_copy(const char* dest)
{
    char buffer[2048];
    FILE* destfile = fopen(dest, "w");
    size_t amount;

    fseek(log_handle, 0, SEEK_SET);

    while ((amount = fread(buffer, 1, sizeof(buffer), log_handle)) > 0)
    {
	fwrite(buffer, 1, amount, destfile);
    }

    clearerr(log_handle);
    fseek(log_handle, 0, SEEK_END);

    fclose(destfile);
}

static struct
{
    gboolean dirty;
    const char* computer;
    const char* domain;
    const char* ou;
    const char* user;
    const char* password;
} join_state;

static char*
safe_strdup(const char* src)
{
    if (!src)
	return NULL;
    else
	return strdup(src);
}

static void
safe_free(void** ptr)
{
    if (*ptr)
	free(*ptr);
    *ptr = NULL;
}

#define SAFE_FREE(var) safe_free((void**) &var);

static void
fill_state(JoinDialog* dialog)
{
    if (join_state.computer && 
	strcmp(join_state.computer, 
	       joindialog_get_computer_name(dialog)))
    {
	join_state.dirty = TRUE;
    }

    SAFE_FREE(join_state.computer);
    SAFE_FREE(join_state.domain);
    SAFE_FREE(join_state.ou);
    
    join_state.computer = safe_strdup(joindialog_get_computer_name(dialog));
    join_state.ou = safe_strdup(joindialog_get_ou_name(dialog));
    join_state.domain = safe_strdup(joindialog_get_domain_name(dialog));
}

static void
fill_state_auth(JoinAuthDialog* auth_dialog)
{
    SAFE_FREE(join_state.user);
    SAFE_FREE(join_state.domain);

    join_state.user = safe_strdup(joinauth_get_user(auth_dialog));
    join_state.password = safe_strdup(joinauth_get_password(auth_dialog));
}

static void
free_state()
{
    SAFE_FREE(join_state.computer);
    SAFE_FREE(join_state.domain);
    SAFE_FREE(join_state.ou);
    SAFE_FREE(join_state.user);
    SAFE_FREE(join_state.domain);

    join_state.dirty = FALSE;
}

/* Dirty, horrible hacks to allow use of new file chooser dialog widget
   despite old RHEL3 build machine */

#if GTK_MINOR_VERSION < 6
#define GTK_FILE_CHOOSER_ACTION_SAVE 1
typedef int GtkFileChooserAction;
static GtkWidget *(*gtk_file_chooser_dialog_new)(const gchar *title,
				       GtkWindow            *parent,
				       GtkFileChooserAction  action,
				       const gchar          *first_button_text,
				       ...);
static const gchar* (*gtk_file_chooser_get_filename)(void*);
#define GTK_FILE_CHOOSER(ptr) ((void*)(ptr))
#endif

static void
show_error_dialog(GtkWindow* parent, LWException* exc)
{
    if(gtk_minor_version < 6)
    {
	GtkDialog* dialog = gtk_message_dialog_new(parent,
						   GTK_DIALOG_DESTROY_WITH_PARENT,
						   GTK_MESSAGE_ERROR,
						   GTK_BUTTONS_CLOSE,
						   "%s: %s",
						   exc->shortMsg,
						   exc->longMsg);
	gtk_dialog_run(dialog);
	gtk_widget_destroy(dialog);
	return;
    }
    else
    {
	JoinErrorDialog* error_dialog = joinerror_new(parent, exc);
	int result;
	
#if GTK_MINOR_VERSION < 6
	{
	    void* handle = dlopen(NULL, RTLD_LAZY);
	    
	    gtk_file_chooser_dialog_new = dlsym(handle, "gtk_file_chooser_dialog_new");
	    gtk_file_chooser_get_filename = dlsym(handle, "gtk_file_chooser_get_filename");
	    
	    dlclose(handle);
	}
#endif
	
	do
	{
	    switch ((result = joinerror_run(error_dialog)))
	    {
	    case JOINERROR_CLOSE:
		break;
	    case JOINERROR_SAVE_LOG:
	    {
		GtkDialog* file_dialog;
		
		file_dialog = GTK_DIALOG(gtk_file_chooser_dialog_new(
					     "Save Log", parent, GTK_FILE_CHOOSER_ACTION_SAVE,
					     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					     GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL));
		
		if (gtk_dialog_run(file_dialog) == GTK_RESPONSE_ACCEPT)
		{
		    log_copy(
			gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file_dialog)));
		}
		
		gtk_widget_destroy(GTK_WIDGET(file_dialog));
		break;
	    }
	    }
	} while (result != JOINERROR_CLOSE);
	
	joinerror_delete(error_dialog);
    }
}

typedef struct JoinInfo
{
    JoinProgressDialog* dialog;
    JoinProcessOptions options;
    gboolean noModifyHosts;
} JoinInfo;

static void PrintWarning(JoinProcessOptions *options, const char *title, const char *message)
{
    LWException *_exc = NULL;
    PSTR warningTitle = NULL;
    if(CENTERROR_IS_OK(CTAllocateStringPrintf(&warningTitle, "Warning: %s", title)))
    {
        LW_RAISE_EX(&_exc, CENTERROR_DOMAINJOIN_WARNING, warningTitle, "%s", message);
        gdk_threads_enter();
        show_error_dialog(joinprogress_get_gtk_window(
                    (JoinProgressDialog *)options->userData), _exc);
        gdk_threads_leave();
    }
    LW_HANDLE(&_exc);
    CT_SAFE_FREE_STRING(warningTitle);
}

static void*
join_worker(gpointer data)
{
    JoinInfo *info = (JoinInfo*) data;
    JoinProgressDialog* dialog = info->dialog;
    JoinProcessOptions *options = &info->options;
    LWException* exc = NULL;
    ModuleState *hostnameState;

    LW_TRY(&exc, DJInitModuleStates(options, &LW_EXC));
    joinprogress_update(dialog, 0.0, "Joining");

    hostnameState = DJGetModuleStateByName(options, "hostname");
    if(info->noModifyHosts)
    {
        if(hostnameState != NULL)
            hostnameState->runModule = FALSE;
    }
    else if(hostnameState->lastResult < FullyConfigured)
        hostnameState->runModule = TRUE;
    options->userData = dialog;
    options->warningCallback = PrintWarning;
    LW_TRY(&exc, DJRunJoinProcess(options, &LW_EXC));

cleanup:

    if (exc)
    {
	joinprogress_raise_error(dialog, exc);
    }
    else
    {
	joinprogress_done(dialog);
    }

    return NULL;
}

static void*
leave_worker(gpointer data)
{
    JoinProgressDialog* dialog = (JoinProgressDialog*) data;
    JoinProcessOptions options;
    LWException* exc = NULL;
    
    DJZeroJoinProcessOptions(&options);
    options.joiningDomain = FALSE;
    options.warningCallback = PrintWarning;
    LW_CLEANUP_CTERR(&exc, DJGetComputerName(&options.computerName));
    LW_TRY(&exc, DJInitModuleStates(&options, &LW_EXC));
    
    joinprogress_update(dialog, 0.0, "Leaving");

    options.userData = dialog;
    LW_TRY(&exc, DJRunJoinProcess(&options, &LW_EXC));

cleanup:

    if (exc)
    {
	joinprogress_raise_error(dialog, exc);
    }
    else
    {
	joinprogress_done(dialog);
    }
    DJFreeJoinProcessOptions(&options);

    return NULL;
}

static void
do_join(JoinDialog* dialog, LWException** exc)
{
    JoinProgressDialog* progress_dialog = NULL;
    JoinAuthDialog* auth_dialog = NULL;

    auth_dialog = joinauth_new(joindialog_get_gtk_window(dialog));

    if (!auth_dialog)
    {
	fprintf(stderr, "Could not create window: out of memory");
	exit(1);
    }

    if (joinauth_run(auth_dialog) == JOINAUTH_OK)
    {
	JoinInfo info = {0};

	fill_state(dialog);
	fill_state_auth(auth_dialog);
	
	join_state.user = safe_strdup(joinauth_get_user(auth_dialog));
	join_state.password = safe_strdup(joinauth_get_password(auth_dialog));
	join_state.computer = safe_strdup(joindialog_get_computer_name(dialog));
	join_state.ou = safe_strdup(joindialog_get_ou_name(dialog));
	join_state.domain = safe_strdup(joindialog_get_domain_name(dialog));

    DJZeroJoinProcessOptions(&info.options);
	info.options.username = safe_strdup(joinauth_get_user(auth_dialog));
	info.options.password = safe_strdup(joinauth_get_password(auth_dialog));
	info.options.computerName = safe_strdup(joindialog_get_computer_name(dialog));
	info.options.ouName = safe_strdup(joindialog_get_ou_name(dialog));
	info.options.domainName = safe_strdup(joindialog_get_domain_name(dialog));
    info.options.joiningDomain = TRUE;

	joinauth_delete(auth_dialog);

	progress_dialog = joinprogress_new(joindialog_get_gtk_window(dialog), "Joining Domain");

	if (!progress_dialog)
	{
	    fprintf(stderr, "Could not create window: out of memory");
	    exit(1);
	}

	info.dialog = progress_dialog;
	info.noModifyHosts = !joindialog_get_modify_hosts(dialog);

	g_thread_create(join_worker, &info, FALSE, NULL);

	if (joinprogress_run(progress_dialog) == JOINPROGRESS_ERROR)
	{
	    LWException* _exc = joinprogress_get_error(progress_dialog);

	    show_error_dialog(joindialog_get_gtk_window(dialog),
			      _exc);

	    LW_HANDLE(&_exc);

	    // Now that the error has been displayed, switch
	    // back to running the progress dialog so the user
	    // can close it.
	    joinprogress_run(progress_dialog);
	}

	joinprogress_delete(progress_dialog);
        DJFreeJoinProcessOptions(&info.options);
    }
    else
    {
	// Just close the dialog
	joinauth_delete(auth_dialog);
    }
}

static gboolean
join_mode(LWException** exc)
{
    JoinDialog* dialog = NULL;
    int result;
    gboolean quit = FALSE;

    g_idle_add(close_stale_dialogs, NULL);  

    dialog = joindialog_new(join_state.computer, join_state.domain);
   
    if (!dialog)
    {
	LW_CLEANUP_CTERR(exc, CENTERROR_OUT_OF_MEMORY);
    }

    switch ((result = joindialog_run(dialog)))
    {
    case JOINDIALOG_CLOSE:
	quit = TRUE;
	break;
    case JOINDIALOG_JOIN:
	LW_TRY(exc, do_join(dialog, &LW_EXC));
	quit = FALSE;
	break;
    }

cleanup:
    
    if (dialog)
	global_join_dialog = dialog;

    return quit;
}

static gboolean
status_mode(LWException** exc)
{
    StatusDialog* dialog = NULL;
    int result;
    gboolean quit = FALSE;

    g_idle_add(close_stale_dialogs, NULL);

    dialog = statusdialog_new(join_state.computer, join_state.domain);
    
    if (!dialog)
    {
	LW_CLEANUP_CTERR(exc, CENTERROR_OUT_OF_MEMORY);
    }

    switch ((result = statusdialog_run(dialog)))
    {
    case STATUSDIALOG_CLOSE:
	quit = TRUE;
	break;
    case STATUSDIALOG_LEAVE:
    {
	JoinProgressDialog* progress_dialog;
	progress_dialog = joinprogress_new(statusdialog_get_gtk_window(dialog), "Leaving Domain");

	if (!progress_dialog)
	{
	    fprintf(stderr, "Could not create window: out of memory");
	    exit(1);
	}

	g_thread_create(leave_worker, progress_dialog, FALSE, NULL);

	if (joinprogress_run(progress_dialog) == JOINPROGRESS_ERROR)
	{
	    LWException* _exc = joinprogress_get_error(progress_dialog);

	    show_error_dialog(statusdialog_get_gtk_window(dialog),
			      _exc);

	    LW_HANDLE(&_exc);

	    // Now that the error has been displayed, switch
	    // back to running the progress dialog so the user
	    // can close it.
	    joinprogress_run(progress_dialog);
	}

	joinprogress_delete(progress_dialog);

	quit = FALSE;
	break;
    }

    }

cleanup:
    
    if (dialog)
	global_status_dialog = dialog;

    return quit;
}

void
ensure_gtk_version(int major, int minor, int micro, LWException** exc)
{
    const char* msg;

    if ((msg = gtk_check_version(major, minor, micro)))
    {
	LW_RAISE_EX(exc, CENTERROR_INCOMPATIBLE_LIBRARY,
		    "Incompatible library detected", 
<<<<<<< HEAD:domainjoin-gui/gtk/main.c
		    "%s.  Likewise does not support graphical domain joins on this platform.  "
=======
		    "%s.  Likewise Open does not support graphical domain joins on this platform.  "
>>>>>>> Rename string referring to "Likewise Enterprise" to "Likewise Open".
Update paths from "likewise" to "likewise-open":domainjoin-gui/gtk/main.c
		    "Please use the command-line domain join application instead.",
		    msg);
    }
}

int
main(int argc, char** argv)
{
    LWException* exc = NULL;
    gboolean quit = FALSE;

    g_thread_init(NULL);
    gdk_threads_init();
    gdk_threads_enter();

    gtk_init(&argc, &argv);

    log_begin();

    do
    {
	char* computer;
	char* domain;

	LW_TRY(&exc, ensure_gtk_version(2, 6, 0, &LW_EXC));

	LW_TRY(&exc, DJQuery((char**) &computer, (char**) &domain, NULL, &LW_EXC));

	free_state();

	join_state.computer = computer;
	join_state.domain = domain;
	
	if (join_state.domain)
	{
	    quit = status_mode(&exc);
	}
	else
	{
	    quit = join_mode(&exc);
	}

    } while (!quit);

cleanup:

    if (exc)
    {
	show_error_dialog(NULL, exc);
    }
    
    gdk_threads_leave();
    log_end();

    return 0;
}

