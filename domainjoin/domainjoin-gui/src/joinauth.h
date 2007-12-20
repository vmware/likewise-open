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

#ifndef __JOINAUTH_H__
#define __JOINAUTH_H__

#include <gtk/gtk.h>

#define JOINAUTH_CANCEL 0
#define JOINAUTH_OK 1

struct JoinAuthDialog;

typedef struct JoinAuthDialog JoinAuthDialog;

JoinAuthDialog* joinauth_new(GtkWindow* parent);
void joinauth_delete(JoinAuthDialog* dialog);
int joinauth_run(JoinAuthDialog* dialog);
const char* joinauth_get_user(JoinAuthDialog* dialog);
const char* joinauth_get_password(JoinAuthDialog* dialog);

#endif
