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

#include "statusdialog.h"
#include "common.h"

#include <glade/glade.h>
#include <gtk/gtk.h>

struct StatusDialog
{
    GtkDialog* dialog;
    GtkEntry* computer_entry;
    GtkEntry* domain_entry;
};

StatusDialog*
statusdialog_new(const char* computer, const char* domain)
{
    GladeXML* xml = glade_xml_new (DOMAINJOIN_XML, "StatusDialog", NULL);
    StatusDialog* dialog = g_new0(StatusDialog, 1);

    if (!dialog)
	return NULL;

    dialog->dialog = GTK_DIALOG(glade_xml_get_widget(xml, "StatusDialog"));
    g_assert(dialog->dialog != NULL);
    g_object_ref(G_OBJECT(dialog->dialog));

    dialog->computer_entry = GTK_ENTRY(glade_xml_get_widget(xml, "StatusComputerEntry"));
    g_assert(dialog->computer_entry != NULL);
    g_object_ref(G_OBJECT(dialog->computer_entry));

    if (computer)
	gtk_entry_set_text(dialog->computer_entry, computer);

    dialog->domain_entry = GTK_ENTRY(glade_xml_get_widget(xml, "StatusDomainEntry"));
    g_assert(dialog->domain_entry != NULL);
    g_object_ref(G_OBJECT(dialog->domain_entry));

    if (domain)
	gtk_entry_set_text(dialog->domain_entry, domain);

    dialog_insert_likewise_logo(dialog->dialog);

    return dialog;
}

int
statusdialog_run(StatusDialog* dialog)
{
    return gtk_dialog_run(dialog->dialog);
}

GtkWindow*
statusdialog_get_gtk_window(StatusDialog* dialog)
{
    return GTK_WINDOW(dialog->dialog);
}

void
statusdialog_delete(StatusDialog* dialog)
{
    g_object_unref(G_OBJECT(dialog->dialog));
    g_object_unref(G_OBJECT(dialog->computer_entry));
    g_object_unref(G_OBJECT(dialog->domain_entry));

    gtk_widget_destroy(GTK_WIDGET(dialog->dialog));
    
    g_free(dialog);
}
