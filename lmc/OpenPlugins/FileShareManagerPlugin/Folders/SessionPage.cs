/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

using System;
using System.Data;
using System.Drawing;
using System.Windows.Forms;
using System.Collections.Generic;
using System.Diagnostics;

using Likewise.LMC.LMConsoleUtils;
using Likewise.LMC.AuthUtils;
using Likewise.LMC.Plugins.FileShareManager.Properties;
using Likewise.LMC.ServerControl;
using Likewise.LMC.NETAPI;

namespace Likewise.LMC.Plugins.FileShareManager
{
public partial class SessionPage : StandardPage
{
    #region Class data
    private ListViewColumnSorter lvwColumnSorter;
    private FileShareManagerIPlugIn plugin; 
    #endregion
    
    #region Constructor
    public SessionPage()
    {
        InitializeComponent();
        
        // Create an instance of a ListView column sorter and assign it
        // to the ListView control.
        lvwColumnSorter = new ListViewColumnSorter();
        this.lvSessionPage.ListViewItemSorter = lvwColumnSorter;
        
        Icon ic = new Icon(Resources.SharedFolder, 48, 48);
        this.picture.Image = ic.ToBitmap();
    }
    #endregion
    
    #region IPlugInPage Members
    public override void SetPlugInInfo(IPlugInContainer container, IPlugIn pi, LACTreeNode treeNode, LWTreeView lmctreeview, CServerControl sc)
    {
        Hostinfo hn = ctx as Hostinfo;

        base.SetPlugInInfo(container, pi, treeNode, lmctreeview, sc);
        bEnableActionMenu = false;

        plugin = pi as FileShareManagerIPlugIn;
        hn = plugin.HostInfo;

        Refresh();
    }
    
    public override void Refresh()
    {
        base.Refresh();
        
        const int NUM_COLUMNS = 6;
        
        //pixels to use to give a visible margin between columns
        const int MARGIN = 10;

        Hostinfo hn = ctx as Hostinfo;

        if (!String.IsNullOrEmpty(hn.hostName))
        {
            this.lblCaption.Text = string.Format(this.lblCaption.Text, hn.hostName);
        }
        
        if (lvSessionPage.Items.Count != 0)
        {
            lvSessionPage.Items.Clear();
        }

        Dictionary<int, string[]> SessionList = Session.EnumSessions(hn.creds, hn.hostName);
        
        if (SessionList == null)
        {
            Logger.Log("SessionPage.Refresh: SessionList == null");
            return;
        }
        
        foreach (int i in SessionList.Keys)
        {
            ListViewItem lvItem = new ListViewItem(SessionList[i]);
            lvSessionPage.Items.Add(lvItem);
        }
        
        int minColumnWidth = (this.Width / NUM_COLUMNS) / 2;
        foreach (ColumnHeader ch in lvSessionPage.Columns)
        {
            if (ch.Index != 0)
            {
                if (ch.Width + MARGIN < minColumnWidth)
                {
                    ch.Width = minColumnWidth;
                }
                else
                {
                    ch.Width += MARGIN;
                }
            }
        }
        
        //HACK: make sure that rightmost column is always covers
        //any remaining space on the right side of the list view
        lvSessionPage.Columns[NUM_COLUMNS - 1].Width = this.Width;
    }
    #endregion

    #region Helper functions

    public ContextMenu GetTreeContextMenu()
    {
        ContextMenu cm = new ContextMenu();

        MenuItem m_Item = new MenuItem("Disconnect &All Sessions", new EventHandler(On_MenuClick));
        cm.MenuItems.Add(m_Item);

        m_Item = new MenuItem("-");
        cm.MenuItems.Add(m_Item);

        m_Item = new MenuItem("All Tas&ks", new EventHandler(On_MenuClick));

        MenuItem innerM_Item = new MenuItem("Disconnect &All Sessions", new EventHandler(On_MenuClick));
        m_Item.MenuItems.Add(innerM_Item);
        cm.MenuItems.Add(m_Item);

        m_Item = new MenuItem("-");
        cm.MenuItems.Add(m_Item);

        m_Item = new MenuItem("&Refresh", new EventHandler(On_MenuClick));
        cm.MenuItems.Add(m_Item);

        m_Item = new MenuItem("-");
        cm.MenuItems.Add(m_Item);

        m_Item = new MenuItem("&Help", new EventHandler(On_MenuClick));
        cm.MenuItems.Add(m_Item);

        return cm;
    }

    private void On_MenuClick(object sender, EventArgs e)
    {
        MenuItem m = sender as MenuItem;

        if (m != null && m.Text.Trim().Equals("&Help"))
        {
            ProcessStartInfo psi = new ProcessStartInfo();
            psi.UseShellExecute = true;
            psi.FileName = CommonResources.GetString("LAC_Help");
            psi.Verb = "open";
            psi.WindowStyle = ProcessWindowStyle.Normal;
            Process.Start(psi);
            return;
        }       

        if (m != null && m.Text.Trim().Equals("&Refresh"))
        {
            treeNode.sc.ShowControl(treeNode);
        }

        if (m != null && m.Text.Trim().Equals("Disconnect &All Sessions"))
        {
            DialogResult dlg = MessageBox.Show(this, "Are you sure you wish to close all sessions?",
                           CommonResources.GetString("Caption_Console"),
                           MessageBoxButtons.YesNo, MessageBoxIcon.None,
                           MessageBoxDefaultButton.Button1);

            if (dlg == DialogResult.OK)
            {
                foreach (ListViewItem Item in lvSessionPage.Items)
                {
                    string sUserMachine = (string)Item.SubItems[1].Text;
                    string sUser = (string)Item.SubItems[0].Text;

                    try
                    {
                        Hostinfo hn = ctx as Hostinfo;
                        Session.DeleteSession(hn.creds, hn.hostName, sUserMachine, sUser);
                    }
                    catch (Exception ex)
                    {
                        string sMsg = string.Format(Resources.Error_DeleteSessionError, ex.Message);
                        container.ShowError(sMsg);
                        break;
                    }
                }
                //Just do refresh once the files got closed on the treenode.
                treeNode.sc.ShowControl(treeNode);
            }
        }
    }

    #endregion

    #region Event handlers
    private void acClose_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
    {
        if (lvSessionPage.SelectedItems.Count != 1)
        {
            return;
        }
        
        // get the accessing machine and user name of the session to close
        ListViewItem Item = lvSessionPage.SelectedItems[0];
        string sUserMachine = (string)Item.SubItems[1].Text;
        string sUser = (string)Item.SubItems[0].Text;
        Hostinfo hn = ctx as Hostinfo;
        
        Session.DeleteSession(hn.creds, hn.hostName, sUserMachine, sUser);

        treeNode.sc.ShowControl(treeNode);
    }
    
    private void lvSessionPage_SelectedIndexChanged(object sender, EventArgs e)
    {
    }
    
    private void lvSessionPage_MouseUp(object sender, MouseEventArgs e)
    {
        ListView lvSender = sender as ListView;
        if (lvSender != null && e.Button == MouseButtons.Right && lvSessionPage.SelectedItems.Count == 1)
        {
            ListViewHitTestInfo hti = lvSender.HitTest(e.X, e.Y);
            if (hti != null && hti.Item != null)
            {
                ListViewItem lvitem = hti.Item;
                if (!lvitem.Selected)
                {
                    lvitem.Selected = true;
                }
            }
        }
    }
    
    private void lvSessionPage_ColumnClick(object sender, ColumnClickEventArgs e)
    {
        // Determine if clicked column is already the column that is being sorted.
        if (e.Column == lvwColumnSorter.SortColumn)
        {
            // Reverse the current sort direction for this column.
            if (lvwColumnSorter.Order == SortOrder.Ascending)
            {
                lvwColumnSorter.Order = SortOrder.Descending;
            }
            else
            {
                lvwColumnSorter.Order = SortOrder.Ascending;
            }
        }
        else
        {
            // Set the column number that is to be sorted; default to ascending.
            lvwColumnSorter.SortColumn = e.Column;
            lvwColumnSorter.Order = SortOrder.Ascending;
        }
        
        // Perform the sort with these new sort options.
        this.lvSessionPage.Sort();
    }
    #endregion
}
}

