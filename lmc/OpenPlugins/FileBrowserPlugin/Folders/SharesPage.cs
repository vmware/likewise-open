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
using System.Diagnostics;
using System.Drawing;
using System.Windows.Forms;
using System.Collections.Generic;
using System.IO;

using Likewise.LMC.LMConsoleUtils;
using Likewise.LMC.Plugins.FileBrowser.Properties;
using Likewise.LMC.NETAPI;
using Likewise.LMC.ServerControl;

namespace Likewise.LMC.Plugins.FileBrowser
{
public partial class SharesPage : StandardPage
{
    #region Class data
    private ListViewColumnSorter lvwColumnSorter;
    private FileBrowserIPlugIn.PluginNodeType nodeType;
    private FileBrowserIPlugIn plugin; 

    //Set columns to Listview as per the node type
    private int numColumns = 0;
    private string[] columnLabels = null;
    private ColumnHeader[] columnHeaders = null;

    #endregion
    
    #region Constructor

    public SharesPage()
    {
        InitializeComponent();

        // Create an instance of a ListView column sorter and assign it
        // to the ListView control.
        lvwColumnSorter = new ListViewColumnSorter();
        this.lvSharePage.ListViewItemSorter = lvwColumnSorter;
    }

    public SharesPage(FileBrowserIPlugIn.PluginNodeType nodetype)
    {
        InitializeComponent();

        // Create an instance of a ListView column sorter and assign it
        // to the ListView control.
        lvwColumnSorter = new ListViewColumnSorter();
        this.lvSharePage.ListViewItemSorter = lvwColumnSorter;

        Icon ic = new Icon(Resources.SharedFolder, 48, 48);
        this.picture.Image = ic.ToBitmap();
        this.nodeType = nodetype;
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

        SetListviewColumns();

        Refresh();
    }

    public override void Refresh()
    {
        base.Refresh();
        Hostinfo hn = ctx as Hostinfo;

        if (nodeType == FileBrowserIPlugIn.PluginNodeType.SHARES)
        {
            Dictionary<int, string[]> ShareList = null;

            if (lvSharePage.Items.Count != 0)
            {
                lvSharePage.Items.Clear();
            }

            if (Configurations.currentPlatform != LikewiseTargetPlatform.Windows)
            {
                if (plugin.fileHandle != null && plugin.fileHandle.Handle != null)
                    ShareList = SharesAPI.EnumShares(plugin.fileHandle.Handle, hn.creds, hn.hostName);
                else
                {
                    Logger.Log("SharesPage.Refresh: SharesAPI.Handle returned null", Logger.LogLevel.Error);
                    return;
                }
            }
            else
            {
                ShareList = SharesAPI.EnumShares(IntPtr.Zero, hn.creds, hn.hostName);
            }

            if (ShareList == null)
            {
                Logger.Log("SharesPage.Refresh: SharesAPI.EnumShares returned null", Logger.LogLevel.Error);
                return;
            }

            foreach (int i in ShareList.Keys)
            {
                ListViewItem lvItem = new ListViewItem(ShareList[i]);
                lvSharePage.Items.Add(lvItem);
            }
        }
        else
        {
            ListViewItem[] itemlist = new ListViewItem[treeNode.Nodes.Count];
            int index = 0;

            foreach (LACTreeNode node in treeNode.Nodes)
            {
                ListViewItem item = new ListViewItem(new string[] { node.Text });
                itemlist[index] = item;
                index++;
            }
            if (itemlist.Length != 0)
                lvSharePage.Items.AddRange(itemlist);
        }
    }

    #endregion


    #region Private helper functions

    private void AutoResize()
    {
        const int NUM_COLUMNS = 4;

        //pixels to use to give a visible margin between columns
        const int MARGIN = 10;

        lvSharePage.AutoResizeColumns(ColumnHeaderAutoResizeStyle.ColumnContent);

        int minColumnWidth = (this.Width / NUM_COLUMNS) / 2;
        foreach (ColumnHeader ch in lvSharePage.Columns)
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
        lvSharePage.Columns[NUM_COLUMNS - 1].Width = this.Width;        
    }

    private void SetListviewColumns()
    {
        if (lvSharePage.Columns.Count != 0)
        {
            lvSharePage.Columns.Clear();
        }

        if (nodeType == FileBrowserIPlugIn.PluginNodeType.UNDEFINED)
        {
            this.lvSharePage.MultiSelect = false;
            this.lblCaption.Text = "Select shared folders to view";
            columnLabels = new string[] { "Name" };
        }
        else if (nodeType == FileBrowserIPlugIn.PluginNodeType.SHARES)
        {
            Hostinfo hn = ctx as Hostinfo;

            this.lvSharePage.MultiSelect = true;            

            if (!String.IsNullOrEmpty(hn.hostName))
            {
                this.lblCaption.Text = string.Format(this.lblCaption.Text, hn.hostName);
            }
            columnLabels = new string[] { "Share", "Path", "Description", "Current Users" };
        }
        else
        {
            throw new ArgumentException("SharesPage must be initialized as either FileBrowserIPlugIn.PluginNodeType.SHARES, " +
            "FileBrowserIPlugIn.PluginNodeType.UNDEFINED, but was not");
        }

        numColumns = columnLabels.Length;
        columnHeaders = new ColumnHeader[numColumns];
        for (int i = 0; i < numColumns; i++)
        {
            columnHeaders[i] = new ColumnHeader();
            columnHeaders[i].Text = columnLabels[i];
            columnHeaders[i].Width = 125;
        }

        this.lvSharePage.Columns.AddRange(columnHeaders);
    }

    public ContextMenu GetTreeContextMenu()
    {
        ContextMenu cm = new ContextMenu();

        MenuItem m_Item = new MenuItem("New File &Share...", new EventHandler(acNewShare_LinkClicked));
        cm.MenuItems.Add(m_Item);

        m_Item = new MenuItem("-");
        cm.MenuItems.Add(m_Item);

        m_Item = new MenuItem("All Tas&ks", new EventHandler(On_MenuClick));

        MenuItem innerM_Item = new MenuItem("New File &Share", new EventHandler(acNewShare_LinkClicked));
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

        if (m != null && m.Text.Trim().Equals("&Properties"))
        {
            ListViewItem Item = GetSelectedItem();

            if (Item == null)
            {
                return;
            }

            string sShare = (string)Item.SubItems[0].Text;
            string[] Shareinfo = null;
            Hostinfo hn = ctx as Hostinfo;

            if (plugin.fileHandle != null)
                Shareinfo = SharesAPI.GetShareInfo(plugin.fileHandle.Handle, sShare, hn.hostName);
            else
                Shareinfo = SharesAPI.GetShareInfo(IntPtr.Zero, sShare, hn.hostName);

            if (Shareinfo != null && Shareinfo.Length != 0)
            {
                if (Shareinfo[0].Equals("share"))
                {
                    SharePropertiesDlg shareDlg = new SharePropertiesDlg(container, this, plugin, hn);
                    shareDlg.SetData(hn.creds, sShare, Shareinfo);
                    shareDlg.ShowDialog(this);
                }
                else
                {
                    string sMsg = "This has been shared for administrative purposes.\n The Share permissions and file security cannot be set";
                    container.ShowMessage(sMsg);
                }
            }
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

        if (m != null && m.Text.Trim().Equals("&Refresh"))
        {
            treeNode.sc.ShowControl(treeNode);
        }
    }

    #endregion
    
    #region Event handlers
    private ListViewItem GetSelectedItem()
    {
        if (lvSharePage.SelectedItems.Count != 1)
        {
            return null;
        }
        
        return lvSharePage.SelectedItems[0];
    }

    private void acNewShare_LinkClicked(object sender, EventArgs e)
    {
        //Process p = SharesAPI.RunAddAShareWizard(hn.creds, hn.hostName);
        //if (p != null && Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
        //{
        //    ProcessUtil.AlertWhenDone(p, WizardDone);
        //}
        //else
        //{
        NewShareWizardDlg dlg = new NewShareWizardDlg(this.container, this, plugin);
            if (dlg.ShowDialog(this) == DialogResult.OK)
            {
                if (dlg.sharedFolderList.Count != 0)
                {
                    foreach (ShareInfo shareinfo in dlg.sharedFolderList)
                    {
                        //try
                        //{
                        //SharesAPI.NetShareAdd(plugin.fileHandle.Handle,
                        //}
                    }
                }
            }
        //}
        treeNode.sc.ShowControl(treeNode);
    }
    
    private void WizardDone(int nExitCode)
    {
        //Refresh();
        treeNode.sc.ShowControl(treeNode);
    }
    
    private void acLaunchMMC_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
    {
        Hostinfo hn = ctx as Hostinfo;
        SharesAPI.RunMMC(hn.creds, hn.hostName);
    }
    
    private void acViewShare_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
    {
        ListViewItem Item = GetSelectedItem();
        if (Item == null)
        {
            return;
        }
        string sShare = (string)Item.SubItems[0].Text;
        Hostinfo hn = ctx as Hostinfo;
        if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
        {
            SharesAPI.ViewShare(hn.creds, hn.hostName, sShare);
        }
        else
        {
            if (File.Exists(sShare))
            {
                ProcessStartInfo startInfo = new ProcessStartInfo("nautilus", sShare);
                startInfo.RedirectStandardOutput = true;
                startInfo.UseShellExecute = false;
                startInfo.Verb = "open";
                startInfo.WindowStyle = ProcessWindowStyle.Normal;
                Process.Start(startInfo);
                return;
            }
        }
    }

    private void acDelete_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
    {
        // get the sharename
        ListViewItem Item = GetSelectedItem();
        if (Item == null)
        {
            return;
        }
        string sShareName = (string)Item.SubItems[0].Text;
        Hostinfo hn = ctx as Hostinfo;
        
        // first, prompt for confirmation
        string sMsg = string.Format(Resources.Prompt_DeleteShare, sShareName);
        if (container.Prompt(sMsg, MessageBoxButtons.OKCancel) != DialogResult.OK)
        {
            return;
        }        
        // delete and refresh
        if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
        {
            SharesAPI.DeleteShare(IntPtr.Zero, hn.creds, hn.hostName, sShareName);
        }
        else
        {
            SharesAPI.DeleteShare(plugin.fileHandle.Handle, hn.creds, hn.hostName, sShareName);
        }
        
        Refresh();
    }
    
    private void lvSharePage_SelectedIndexChanged(object sender, EventArgs e)
    {
    }
    
    private void lvSharePage_MouseUp(object sender, MouseEventArgs e)
    {
        ListView lvSender = sender as ListView;
        if (lvSender != null && e.Button == MouseButtons.Right && lvSharePage.SelectedItems.Count == 1)
        {
            ListViewHitTestInfo hti = lvSender.HitTest(e.X, e.Y);
            if (hti != null && hti.Item != null)
            {
                ListViewItem lvitem = hti.Item;
                if (!lvitem.Selected)
                {
                    lvitem.Selected = true;
                }
                ContextMenu cm = new ContextMenu();               
                MenuItem m_item = new MenuItem("&Properties", new EventHandler(On_MenuClick));
                cm.MenuItems.Add(m_item);

                cm.Show(lvSender, new Point(e.X, e.Y));
            }
        }
    }
    
    private void lvSharePage_ColumnClick(object sender, ColumnClickEventArgs e)
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
        this.lvSharePage.Sort();
    }
    #endregion          
}
}

