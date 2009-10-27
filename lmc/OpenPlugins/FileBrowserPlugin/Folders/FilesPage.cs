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
using Likewise.LMC.Plugins.FileBrowser.Properties;
using Likewise.LMC.ServerControl;
using Likewise.LMC.NETAPI;

namespace Likewise.LMC.Plugins.FileBrowser
{
public partial class FilesPage : StandardPage
{
    #region Class data
    private ListViewColumnSorter lvwColumnSorter;
    private FileBrowserIPlugIn plugin; 
    #endregion
    
    #region Constructor
    public FilesPage()
    {
        InitializeComponent();
        
        // Create an instance of a ListView column sorter and assign it
        // to the ListView control.
        lvwColumnSorter = new ListViewColumnSorter();
        this.lvFilePage.ListViewItemSorter = lvwColumnSorter;
        
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
        plugin = pi as FileBrowserIPlugIn;        
        hn = plugin.HostInfo;

        Refresh();
    }
    
    public override void Refresh()
    {
        base.Refresh();
        Hostinfo hn = ctx as Hostinfo;
        
        const int NUM_COLUMNS = 5;
        
        //pixels to use to give a visible margin between columns
        const int MARGIN = 10;
        
        if (lvFilePage.Items.Count != 0)
        {
            lvFilePage.Items.Clear();
        }

        if (!String.IsNullOrEmpty(hn.hostName))
        {
            this.lblCaption.Text = string.Format(this.lblCaption.Text, hn.hostName);
        }
        Dictionary<int, string[]> FileList =null;

        if (Configurations.currentPlatform != LikewiseTargetPlatform.Windows)
        {
            if (plugin.fileHandle != null && plugin.fileHandle.Handle != null)
                FileList = SharesAPI.EnumFiles(plugin.fileHandle.Handle, hn.creds, hn.hostName);
            else
            {
                Logger.Log("FilesPage.Refresh: SharesAPI.Handle returned null", Logger.LogLevel.Error);
                return;
            }
        }
        else
            FileList = SharesAPI.EnumFiles(IntPtr.Zero, hn.creds, hn.hostName);
        
        if (FileList == null)
        {
            Logger.Log("FilesPage.Refresh: FileList == null", Logger.LogLevel.Error);
            return;
        }
        
        foreach (int i in FileList.Keys)
        {
            ListViewItem lvItem = new ListViewItem(FileList[i]);
            lvFilePage.Items.Add(lvItem);
        }
        
        int minColumnWidth = (this.Width / NUM_COLUMNS) / 2;
        foreach (ColumnHeader ch in lvFilePage.Columns)
        {
            if (ch.Index != 4)
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
        lvFilePage.Columns[NUM_COLUMNS - 2].Width = this.Width;
    }
    #endregion

    #region Helper functions

    public ContextMenu GetTreeContextMenu()
    {
        ContextMenu cm = new ContextMenu();

        MenuItem m_Item = new MenuItem("Disconnect &All Open Files", new EventHandler(On_MenuClick));
        cm.MenuItems.Add(m_Item);

        m_Item = new MenuItem("-");
        cm.MenuItems.Add(m_Item);

        m_Item = new MenuItem("All Tas&ks", new EventHandler(On_MenuClick));

        MenuItem innerM_Item = new MenuItem("Disconnect &All Open Files", new EventHandler(On_MenuClick));
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

        if (m != null && m.Text.Trim().Equals("Disconnect &All Open Files"))
        {
            DialogResult dlg = MessageBox.Show(this, "Are you sure you wish to close all files?",
                             CommonResources.GetString("Caption_Console"),
                             MessageBoxButtons.YesNo, MessageBoxIcon.None,
                             MessageBoxDefaultButton.Button1);

            if (dlg == DialogResult.OK)
            {
                Hostinfo hn = ctx as Hostinfo;

                foreach (ListViewItem Item in lvFilePage.Items)
                {
                    string sFileId = (string)Item.SubItems[4].Text;

                    try
                    {
                        int nFileId = int.Parse(sFileId);

                        if (plugin.fileHandle != null)
                            SharesAPI.CloseFile(IntPtr.Zero, hn.creds, hn.hostName, nFileId);
                        else
                            SharesAPI.CloseFile(plugin.fileHandle.Handle, hn.creds, hn.hostName, nFileId);
                    }
                    catch (Exception ex)
                    {
                        string sMsg = string.Format(Resources.Error_UnableToCloseFile, ex.Message);
                        container.ShowError(sMsg);
                        break;
                    }
                }
                //Just do refresh once the files got closed on the treenode.
                treeNode.sc.ShowControl(treeNode);
            }
        }

        if (m != null && m.Text.Trim().Equals("&Refresh"))
        {
            treeNode.sc.ShowControl(treeNode);
            return;
        }

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
    }

    #endregion

    #region Event handlers
    private void acClose_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
    {
        if (lvFilePage.SelectedItems.Count != 1)
        {
            return;
        }
        
        // get the accessing machine and user name of the session to close
        ListViewItem Item = lvFilePage.SelectedItems[0];
        string sFileId = (string)Item.SubItems[4].Text;
        
        try
        {
            int nFileId = int.Parse(sFileId);
            Hostinfo hn = ctx as Hostinfo;

            if (plugin.fileHandle != null)
                SharesAPI.CloseFile(IntPtr.Zero, hn.creds, hn.hostName, nFileId);
            else
                SharesAPI.CloseFile(plugin.fileHandle.Handle, hn.creds, hn.hostName, nFileId);

            Refresh();
        }
        catch (Exception ex)
        {
            string sMsg = string.Format(Resources.Error_UnableToCloseFile, ex.Message);
            container.ShowError(sMsg);
        }
    }
    
    private void lvFilePage_SelectedIndexChanged(object sender, EventArgs e)
    {
    }
    
    private void lvFilePage_MouseUp(object sender, MouseEventArgs e)
    {
        ListView lvSender = sender as ListView;
        if (lvSender != null && e.Button == MouseButtons.Right && lvFilePage.SelectedItems.Count == 1)
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
    
    private void lvFilePage_ColumnClick(object sender, ColumnClickEventArgs e)
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
        this.lvFilePage.Sort();
    }
    #endregion
}
}

