/*
 * Copyright (C) Likewise Software 2007.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
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

#ifndef __JOINDIALOG_H__
#define __JOINDIALOG_H__

#include <gtk/gtk.h>

#define JOINDIALOG_CLOSE 0
#define JOINDIALOG_JOIN 1

struct JoinDialog;

typedef struct JoinDialog JoinDialog;

JoinDialog* joindialog_new(const char* computer, const char* domain);
int joindialog_run(JoinDialog* dialog);
const char* joindialog_get_computer_name(JoinDialog* dialog);
const char* joindialog_get_domain_name(JoinDialog* dialog);
const char* joindialog_get_ou_name(JoinDialog* dialog);
gboolean joindialog_get_modify_hosts(JoinDialog* dialog);
GtkWindow* joindialog_get_gtk_window(JoinDialog* dialog);
void joindialog_delete(JoinDialog* dialog);

#endif
