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

using System.Drawing;
using System.Windows.Forms;
using System.Collections.Generic;
using Likewise.LMC.ServerControl;
using Likewise.LMC.Utilities;

namespace Likewise.LMC.Plugins.Root
{
public partial class ConsolePage : ServerControl.StandardPage
{
    #region Constructor
    public ConsolePage()
    {
        InitializeComponent();

    }

    /// <summary>
    /// Sets the Plugin information to base
    /// List the all child nodes, then refreshes the listview with the all child treenodes.
    /// </summary>
    /// <param name="ccontainer"></param>
    /// <param name="pi"></param>
    /// <param name="treeNode"></param>
    /// <param name="lmctreeview"></param>
    public override void SetPlugInInfo(IPlugInContainer ccontainer, IPlugIn pi, LACTreeNode treeNode, LWTreeView lmctreeview, CServerControl sc)
    {
        if (treeNode.sc != null)
        {
            smallimagelist = treeNode.sc.manage.manageImageList as ImageList;
            Lagreimagelist = treeNode.sc.manage.manageLargeImageList as ImageList;
        }

        base.SetPlugInInfo(ccontainer, pi, treeNode, lmctreeview, sc);

        if (treeNode != null)
        {
            Refresh();
        }
    }
    /// <summary>
    /// Method to change the listview view style.
    /// </summary>
    public override void Refresh()
    {
        Logger.Log("RootPlugin: ConsolePage.Refresh", Logger.manageLogLevel);

        //don't actually refresh if the only thing that has changed is the view style.
        if (currentViewStyleChanged)
        {
            currentViewStyleChanged = false;
        }

        if (treeNode != null)
        {
            RefreshListview(treeNode);
        }
        else
        {
            base.Refresh();
        }
    }

    private void RefreshListview(LACTreeNode lacnode)
    {
        lacnode.Expand();

        List<ListViewItem> nodelist = new List<ListViewItem>();
        foreach (LACTreeNode node in lacnode.Nodes)
        {
            if (node.sc == null)
            {
                node.sc = treeNode.sc;
            }
            ListViewItem lvitem = new ListViewItem(new string[]
            {
                node.Text
            }
            );
            lvitem.Tag = node;
            lvitem.ImageIndex = node.ImageIndex;
            nodelist.Add(lvitem);
        }
        ListViewItem[] lvItems = new ListViewItem[nodelist.Count];
        nodelist.CopyTo(lvItems);
    }

    #endregion

    private void lvChildNodes_MouseDoubleClick(object sender, MouseEventArgs e)
    {
        ListView lvSender = sender as ListView;
        if (lvSender != null)
        {
            ListViewHitTestInfo hti = lvSender.HitTest(e.X, e.Y);
            if (hti != null && hti.Item != null)
            {
                ListViewItem lvItem = hti.Item;

                if (!lvItem.Selected)
                {
                    lvItem.Selected = true;
                }

                if (lvItem.Tag != null)
                {
                    LACTreeNode pluginnode = lvItem.Tag as LACTreeNode;
                    ContextMenu contextmenu = null;
                    if (pluginnode != null)
                    {
                        lmctreeview.SelectedNode = pluginnode;
                        lmctreeview.Select();
                        if (e.Button == MouseButtons.Right)
                        {
                            IPlugIn plugin = pluginnode.Plugin;
                            contextmenu = plugin.GetTreeContextMenu(pluginnode);
                        }
                        else if (e.Button == MouseButtons.Left)
                        {
                            if (pluginnode.Nodes.Count != 0)
                            {
                                pluginnode.sc.manage.ShowControl(pluginnode);
                            }
                            else
                            {
                                IPlugIn plugin = pluginnode.Plugin;
                                contextmenu = plugin.GetTreeContextMenu(pluginnode);
                            }
                        }
                    }
                    if (contextmenu != null)
                    {
                        contextmenu.Show(lvSender, new Point(e.X, e.Y));
                    }
                    else
                    {
                        Logger.Log(
                        "ConsolePage::lvChildNodes_MouseDoubleClick, menu == null",
                        Logger.manageLogLevel);
                    }
                }
            }
        }
    }

    private void lvChildNodes_MouseUp(object sender, MouseEventArgs e)
    {
        ListView lvSender = sender as ListView;
        if (lvSender != null && e.Button == MouseButtons.Right)
        {
            ListViewHitTestInfo hti = lvSender.HitTest(e.X, e.Y);
            if (hti != null && hti.Item != null)
            {
                ListViewItem lvItem = hti.Item;
                ContextMenu menu = null;

                if (!lvItem.Selected)
                {
                    lvItem.Selected = true;
                }

                if (lvItem.Tag != null)
                {
                    if (e.Button == MouseButtons.Right)
                    {
                        LACTreeNode pluginnode = lvItem.Tag as LACTreeNode;
                        IPlugIn plugin = pluginnode.Plugin;
                        menu = plugin.GetTreeContextMenu(pluginnode);
                    }
                }
                if (menu != null)
                {
                    menu.Show(lvSender, new Point(e.X, e.Y));
                }
                else
                {
                    Logger.Log(
                    "ConsolePage::lvChildNodes_MouseUp, menu == null",
                    Logger.manageLogLevel);
                }
            }
        }
    }
}
}
