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
        public int Count = 0;
        private FileBrowserIPlugIn plugin;
        private Icon FolderIcon = new Icon(Resources.Folder, 32, 32);
        private Icon FolderOpenIcon = new Icon(Resources.FolderOpen, 32, 32);
        private Icon FileIcon = new Icon(Resources.Document, 32, 32);

        #endregion

        #region Constructor

        public FilesDetailPage(
            )
        {
            InitializeComponent();

            // Create an instance of a ListView column sorter and assign it
            // to the ListView control.
            lvwColumnSorter = new ListViewColumnSorter();
            this.lvFilePage.ListViewItemSorter = lvwColumnSorter;

            this.picture.Image = FolderOpenIcon.ToBitmap();
        }

        #endregion

        #region IPlugInPage Members

        public override void SetPlugInInfo(
            IPlugInContainer container,
            IPlugIn pi,
            LACTreeNode treeNode,
            LWTreeView lmctreeview,
            CServerControl sc
            )
        {
            base.SetPlugInInfo(container, pi, treeNode, lmctreeview, sc);
            bEnableActionMenu = false;
            plugin = pi as FileBrowserIPlugIn;

            Refresh();
        }

        public override void Refresh(
            )
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
                    type = "File";
                }

                if (File.CreationTime != new DateTime())
                {
                    creation = File.CreationTime.ToString();
                }

                if (File.LastWriteTime != new DateTime())
                {
                    modified = File.LastWriteTime.ToString();
                }

                size = File.FileSize.ToString() + " KB";
                string[] file = { File.FileName, creation, modified, type, size };

                ListViewItem lvItem = new ListViewItem(file);
                lvFilePage.Items.Add(lvItem);
            }
        }

        #endregion

        #region File operations

        private void DoFileDelete(
            ListViewItem item
            )
        {
            FileBrowserNode node = base.TreeNode as FileBrowserNode;
            DialogResult result = MessageBox.Show("Are you sure that you want to delete the file called: " + item.Text + "?",
                                      "File Browser",
                                      MessageBoxButtons.YesNo);

            if (result == DialogResult.Yes)
            {
                string path = node.Path + "\\" + item.Text;
                WinError error = FileClient.FileClient.apiDeleteFile(path);

                if (error == WinError.NO_ERROR)
                {
                    lvFilePage.BeginUpdate();
                    lvFilePage.Items.Remove(item);
                    lvFilePage.EndUpdate();
                    Refresh();
                }
            }
        }

        private void DoDirectoryDelete(
            ListViewItem item
            )
        {
            FileBrowserNode node = base.TreeNode as FileBrowserNode;
            DialogResult result = MessageBox.Show("Are you sure that you want to delete the directory called: " + item.Text + "?",
                                      "File Browser",
                                      MessageBoxButtons.YesNo);

            if (result == DialogResult.Yes)
            {
                string path = node.Path + "\\" + item.Text;
                WinError error = FileClient.FileClient.apiRemoveDirectory(path);

                if (error == WinError.NO_ERROR)
                {
                    lvFilePage.BeginUpdate();
                    lvFilePage.Items.Remove(item);
                    lvFilePage.EndUpdate();
                    Refresh();
                }
            }
        }

        private void DoFileMove(
            ListViewItem item
            )
        {
            FileBrowserNode node = base.TreeNode as FileBrowserNode;
            string destination = "";

            // Determine destingation to move to
            SelectDestinationDialog destinationDialog = new SelectDestinationDialog(node.Path, SelectDestinationDialog.SELECT_DESTINATION_OPERATION.MOVE_FILE, plugin);

            if (destinationDialog.ShowDialog() == DialogResult.OK)
            {
                destination = destinationDialog.GetPath() + "\\" + item.Text;

                string path = node.Path + "\\" + item.Text;
                WinError error = FileClient.FileClient.apiMoveFile(path, destination);

                if (error == WinError.NO_ERROR)
                {
                    lvFilePage.BeginUpdate();
                    lvFilePage.Items.Remove(item);
                    lvFilePage.EndUpdate();
                    Refresh();
                }
                else
                {
                    string message = "Move file operation failed. Error: " + error.ToString();
                    MessageBox.Show(message, "Could not move file", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            }
        }

        private void DoDirectoryMove(
            ListViewItem item
            )
        {
            FileBrowserNode node = base.TreeNode as FileBrowserNode;
            string destination = "";

            // Determine destingation to move to
            SelectDestinationDialog destinationDialog = new SelectDestinationDialog(node.Path, SelectDestinationDialog.SELECT_DESTINATION_OPERATION.MOVE_DIRECTORY, plugin);

            if (destinationDialog.ShowDialog() == DialogResult.OK)
            {
                destination = destinationDialog.GetPath() + "\\" + item.Text;

                string path = node.Path + "\\" + item.Text;
                WinError error = FileClient.FileClient.apiMoveDirectory(path, destination);

                if (error == WinError.NO_ERROR)
                {
                    lvFilePage.BeginUpdate();
                    lvFilePage.Items.Remove(item);
                    lvFilePage.EndUpdate();
                    Refresh();
                }
                else
                {
                    string message = "Move directory operation failed. Error: " + error.ToString();
                    MessageBox.Show(message, "Could not move directory", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            }
        }

        private void DoFileCopy(
            ListViewItem item
            )
        {
            FileBrowserNode node = base.TreeNode as FileBrowserNode;
            string destination = "";

            // Determine destingation to copy to
            SelectDestinationDialog destinationDialog = new SelectDestinationDialog(node.Path, SelectDestinationDialog.SELECT_DESTINATION_OPERATION.COPY_FILE, plugin);

            if (destinationDialog.ShowDialog() == DialogResult.OK)
            {
                destination = destinationDialog.GetPath() + "\\" + item.Text;

                string path = node.Path + "\\" + item.Text;
                WinError error = FileClient.FileClient.apiCopyFile(path, destination, true);

                if (error == WinError.ERROR_FILE_EXISTS)
                {
                    destination = destinationDialog.GetPath() + "\\Copy of " + item.Text;
                    error = FileClient.FileClient.apiCopyFile(path, destination, true);
                }

                if (error == WinError.NO_ERROR)
                {
                    Refresh();
                }
                else
                {
                    string message = "Copy file operation failed. Error: " + error.ToString();
                    MessageBox.Show(message, "Could not copy file", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            }
        }

        private void DoDirectoryCopy(
            ListViewItem item
            )
        {
            FileBrowserNode node = base.TreeNode as FileBrowserNode;
            string destination = "";

            // Determine destingation to copy to
            SelectDestinationDialog destinationDialog = new SelectDestinationDialog(node.Path, SelectDestinationDialog.SELECT_DESTINATION_OPERATION.COPY_DIRECTORY, plugin);

            if (destinationDialog.ShowDialog() == DialogResult.OK)
            {
                destination = destinationDialog.GetPath() + "\\" + item.Text;

                string path = node.Path + "\\" + item.Text;
                WinError error = FileClient.FileClient.apiCopyDirectory(path, destination, true);

                if (error == WinError.ERROR_FILE_EXISTS)
                {
                    destination = destinationDialog.GetPath() + "\\Copy of " + item.Text;
                    error = FileClient.FileClient.apiCopyDirectory(path, destination, true);
                }

                if (error == WinError.NO_ERROR)
                {
                    Refresh();
                }
                else
                {
                    string message = "Copy directory operation failed. Error: " + error.ToString();
                    MessageBox.Show(message, "Could not copy directory", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            }
        }

        private void DoNewFolder(
            )
        {
            FileBrowserNode node = base.TreeNode as FileBrowserNode;
            string name = node.Path + "\\New Folder";
        }

        private void DoNewTextFile(
            )
        {
            FileBrowserNode node = base.TreeNode as FileBrowserNode;
            string name = node.Path + "\\TextFile.txt";
        }

        #endregion

        #region Helper functions

        public ContextMenu GetTreeContextMenu(
            )
        {
            ContextMenu cm = new ContextMenu();

            // Not used

            return cm;
        }

        private void On_MenuClick(
            object sender,
            EventArgs e
            )
        {
            // Not used
        }

        #endregion

        #region Event handlers

        private void lvFilePage_SelectedIndexChanged(
            object sender,
            EventArgs e
            )
        {
            if (lvFilePage.SelectedItems.Count != 1)
            {
                return;
            }

            ListViewItem Item = lvFilePage.SelectedItems[0];
            lblCaption.Text = Item.SubItems[0].Text;

            if (Item.SubItems[3].Text == "Directory")
            {
                this.picture.Image = FolderOpenIcon.ToBitmap();
            }

            if (Item.SubItems[3].Text == "File")
            {
                this.picture.Image = FileIcon.ToBitmap();
            }
        }

        private void lvFilePage_MouseUp(
            object sender,
            MouseEventArgs e
            )
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

        private void lvFilePage_ColumnClick(
            object sender,
            ColumnClickEventArgs e
            )
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

        private void lvFilePage_MouseClick(
            object sender,
            MouseEventArgs e
            )
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
                else
                {
                    contextMenuStrip.Items.Clear();
                    contextMenuStrip.Items.Add("New folder");
                    contextMenuStrip.Items.Add("New text file");

                    Point pt = new Point(e.X, e.Y);
                    contextMenuStrip.Show(lvFilePage, pt);
                }
            }
        }

        private void contextMenuStrip_ItemClicked(
            object sender,
            ToolStripItemClickedEventArgs e
            )
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

            switch (option)
            {
                case "Delete file":
                    DoFileDelete(item);
                    break;

                case "Delete directory":
                    DoDirectoryDelete(item);
                    break;

                case "Move file":
                    DoFileMove(item);
                    break;

                case "Move directory":
                    DoDirectoryMove(item);
                    break;

                case "Copy file":
                    DoFileCopy(item);
                    break;

                case "Copy directory":
                    DoDirectoryCopy(item);
                    break;

                case "New folder":
                    DoNewFolder();
                    break;

                case "New text file":
                    DoNewTextFile();
                    break;

                default:
                    break;
            }
        }

        private void lvFilePage_DoubleClick(
            object sender,
            EventArgs
            e)
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

            plugin.EnumChildren(node);

            if (node != null)
            {
                if (node.TreeView != null)
                {
                    parent.TreeView.SelectedNode = node;
                    treeNode = node;

                    if (node.Nodes.Count > 0)
                    {
                        node.Expand();
                    }

                    Refresh();
                }
            }
        }

        #endregion
    }
}

