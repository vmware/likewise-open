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

using Likewise.LMC.Utilities;
using Likewise.LMC.Plugins.FileBrowser.Properties;
using Likewise.LMC.ServerControl;
using Likewise.LMC.NETAPI;
using Likewise.LMC.FileClient;

namespace Likewise.LMC.Plugins.FileBrowser
{
    public partial class FilesDetailPage : StandardPage
    {
        #region Class data

        public static bool IsMultiselect = false;
        private ListViewColumnSorter lvwColumnSorter;
        private FileBrowserNode[] ChildNodes = null;
        public int Count = 0;
        private FileBrowserIPlugIn plugin;

        #endregion

        #region Constructor

        public FilesDetailPage()
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
            base.SetPlugInInfo(container, pi, treeNode, lmctreeview, sc);
            bEnableActionMenu = false;
            plugin = pi as FileBrowserIPlugIn;

            Refresh();
        }

        public override void Refresh()
        {
            List<FileItem> FileList = null;

            base.Refresh();

            FileBrowserNode node = base.TreeNode as FileBrowserNode;

            if (lvFilePage.Items.Count != 0)
            {
                lvFilePage.Items.Clear();
            }

            if (String.IsNullOrEmpty(node.Path))
            {
                this.lblCaption.Text = node.Text;

                // Show children of treenode (File Browser, Network, Computer, etc)
            }
            else
            {
                this.lblCaption.Text = string.Format(this.lblCaption.Text, node.Path);
                FileList = FileClient.FileClient.EnumFiles(node.Path, false);
            }

            if (FileList == null || FileList.Count == 0)
            {
                return;
            }

            foreach (FileItem File in FileList)
            {
                string creation = "";
                string modified = "";
                string size = "";
                string type = "Directory";

                if (!File.IsDirectory)
                {
                    if (File.CreationTime != new DateTime())
                    {
                        creation = File.CreationTime.ToString();
                    }

                    if (File.LastWriteTime != new DateTime())
                    {
                        modified = File.LastWriteTime.ToString();
                    }

                    size = File.FileSize.ToString() + " KB";
                    type = "File";
                }

                string[] file = { File.FileName, creation, modified, type, size };

                ListViewItem lvItem = new ListViewItem(file);
                lvFilePage.Items.Add(lvItem);
            }
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
    /*
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
    */

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

    /*
            // get the accessing machine and user name of the session to close
            ListViewItem Item = lvFilePage.SelectedItems[0];
            string sFileId = (string)Item.SubItems[0].Text;

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
    */
        }

        private void lvFilePage_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (lvFilePage.SelectedItems.Count != 1)
            {
                return;
            }

            ListViewItem Item = lvFilePage.SelectedItems[0];
            lblCaption.Text = Item.SubItems[0].Text;
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

        private void lvFilePage_MouseClick(object sender, MouseEventArgs e)
        {
            if (e.Button == MouseButtons.Right)
            {
                ListViewHitTestInfo hit = lvFilePage.HitTest(e.X, e.Y);

                if (hit != null)
                {
                    int count = lvFilePage.SelectedItems.Count;

                    while (count > 0)
                    {
                        count--;
                        lvFilePage.SelectedItems[count].Selected = false;
                    }

                    hit.Item.Selected = true;

                    if (hit.Item.SubItems[3].Text == "Directory")
                    {
                        contextMenuStrip.Items.Clear();
                        contextMenuStrip.Items.Add("Delete directory");
                        contextMenuStrip.Items.Add("Move directory");
                        contextMenuStrip.Items.Add("Copy directory");
                    }
                    else if (hit.Item.SubItems[3].Text == "File")
                    {
                        contextMenuStrip.Items.Clear();
                        contextMenuStrip.Items.Add("Delete file");
                        contextMenuStrip.Items.Add("Move file");
                        contextMenuStrip.Items.Add("Copy file");
                    }
                    else
                    {
                        return;
                    }

                    Point pt = new Point(e.X, e.Y);
                    contextMenuStrip.Show(lvFilePage, pt);
                }
            }
        }

        private void contextMenuStrip_ItemClicked(object sender, ToolStripItemClickedEventArgs e)
        {
            FileBrowserNode node = base.TreeNode as FileBrowserNode;
            int count = lvFilePage.SelectedItems.Count;
            string option = e.ClickedItem.Text;

            if (node == null)
            {
                return;
            }

            contextMenuStrip.Hide();

            if (count != 1)
            {
                return;
            }

            ListViewItem item = lvFilePage.SelectedItems[0];

            string  message = "Want to " + option + ":  " + item.Text;
            MessageBox.Show(message);
        }

        private void lvFilePage_DoubleClick(object sender, EventArgs e)
        {
            FileBrowserNode parent = base.TreeNode as FileBrowserNode;
            int count = lvFilePage.SelectedItems.Count;

            if (parent == null)
            {
                return;
            }

            if (count != 1)
            {
                return;
            }

            TreeNode[] nodes = base.TreeNode.Nodes.Find(lvFilePage.SelectedItems[0].Text, true);

            if (nodes == null || nodes.Length == 0)
            {
                return;
            }

            LACTreeNode node = nodes[0] as LACTreeNode;

            if (node != null)
            {
                node.EnsureVisible();
                node.IsSelected = true;
            }
        }

        #endregion
    }
}

