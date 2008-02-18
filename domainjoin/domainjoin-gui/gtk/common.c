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

#include "common.h"

void
dialog_insert_likewise_logo(GtkDialog* dialog)
{
    GtkWidget* image = gtk_image_new_from_file (SHARE_DIR "/likewise-open/likewise-logo.png");
    
    gtk_box_pack_start(GTK_BOX(dialog->action_area), image, FALSE, FALSE, 0);
    gtk_button_box_set_child_secondary(GTK_BUTTON_BOX(dialog->action_area), image, TRUE);
    gtk_widget_show(image);
}
