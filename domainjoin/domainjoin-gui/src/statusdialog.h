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

#ifndef __STATUSDIALOG_H__
#define __STATUSDIALOG_H__

#include <gtk/gtk.h>

#define STATUSDIALOG_CLOSE 0
#define STATUSDIALOG_LEAVE 1

struct StatusDialog;

typedef struct StatusDialog StatusDialog;

StatusDialog* statusdialog_new(const char* computer, const char* domain);
int statusdialog_run(StatusDialog* dialog);
GtkWindow* statusdialog_get_gtk_window(StatusDialog* dialog);
void statusdialog_delete(StatusDialog* dialog);

#endif
