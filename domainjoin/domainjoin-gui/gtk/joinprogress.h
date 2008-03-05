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

#ifndef __JOINPROGRESS_H__
#define __JOINPROGRESS_H__

#include <glade/glade.h>
#include <gtk/gtk.h>

#include <stdint.h>
#include <cterr.h>
#include <lwexc.h>

#define JOINPROGRESS_CLOSE 0
#define JOINPROGRESS_ERROR 1

struct JoinProgressDialog;

typedef struct JoinProgressDialog JoinProgressDialog;

JoinProgressDialog* joinprogress_new(GtkWindow* parent, const char* title);
int joinprogress_run(JoinProgressDialog* dialog);
void joinprogress_update(JoinProgressDialog* dialog, gdouble ratio, const char* description);
void joinprogress_raise_error(JoinProgressDialog* dialog, LWException* exc);
LWException* joinprogress_get_error(JoinProgressDialog* dialog);
void joinprogress_done(JoinProgressDialog* dialog);
void joinprogress_delete(JoinProgressDialog* dialog);
GtkWindow* joinprogress_get_gtk_window(JoinProgressDialog* dialog);

#endif
