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
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using Likewise.LMC.ServerControl;
using Likewise.LMC.Plugins.ADUCPlugin.Properties;
using Likewise.LMC.LMConsoleUtils;

namespace Likewise.LMC.Plugins.ADUCPlugin
{
public partial class AddUsertoGroup : Form
{
    #region class data
    IPlugInContainer IPlugInContainer;
    
    public static int MEMOF_PAGE = 1;
    public static int MEMBERS_PAGE = 2;
    
    private ADUCPlugin _addplugin;
    private LACTreeNode _aducRootnode;
    private int _PageType;
    
    public AdduserInGroupInfo groupInfo;
    #endregion
    
    
    #region Constructors
    public AddUsertoGroup()
    {
        InitializeComponent();
        groupInfo = new AdduserInGroupInfo();
    }
    
    /// <summary>
    /// Adds all the "user"/"group" objects to the tree
    /// </summary>
    /// <param name="container"></param>
    /// <param name="parentPage"></param>
    /// <param name="plugin"></param>
    /// <param name="PageType"></param>
    public AddUsertoGroup(IPlugInContainer container, MPPage parentPage, ADUCPlugin plugin, int PageType)
    : this()
    {
        this.IPlugInContainer = container;
        _addplugin = plugin;
        _aducRootnode = _addplugin.GetPlugInNode();
        _PageType = PageType;
        
        if (_aducRootnode.Nodes.Count > 0)
        {
            ADUCDirectoryNode rootNode =
            ADUCDirectoryNode.GetDirectoryRoot(_addplugin.GetpluginContext(),            
            _addplugin.GetRootDN(),
            Resources.ADUC, typeof (ADUCPage), _addplugin);
            
            if (rootNode != null)
            {
                treeView1.Nodes.Add(rootNode);
                rootNode.Refresh();
                treeView1.ExpandAll();
            }
        }
        else
        {
            Logger.Log("The LWTreeView control does not have any nodes.");
        }
    }
    #endregion
    
    
    #region Initialization Methods
    
    /// <summary>
    /// only list those children that are group type
    /// </summary>
    /// <param name="sender"></param>
    /// <param name="e"></param>
    private void treeView1_AfterSelect(object sender, TreeViewEventArgs e)
    {
        // When a Node is selected, ShowNode already gets called because of LWTreeView1_NodeMouseClick
        LACTreeNode node = e.Node as LACTreeNode;
        
        if (node is ADUCDirectoryNode)
        {
            ADUCDirectoryNode dirnode = node as ADUCDirectoryNode;
            
            dirnode.ListChildren();
            groupInfo.groupName = dirnode.DistinguishedName;
            groupInfo.objectName = dirnode.DistinguishedName;
        }
    }
    
    /// <summary>
    /// Gets the selected "user" or "group" info from the tree
    /// </summary>
    /// <param name="sender"></param>
    /// <param name="e"></param>
    private void treeView1_NodeMouseClick(object sender, TreeNodeMouseClickEventArgs e)
    {
        if (sender != null)
        {
            bool IsValidObject = false;
            if (e.Button != System.Windows.Forms.MouseButtons.Right)
            {
                LACTreeNode node = e.Node as LACTreeNode;
                
                ADUCDirectoryNode dirnode = node as ADUCDirectoryNode;
                
                if (dirnode != null)
                {
                    if (_PageType == MEMOF_PAGE)
                    {
                        dirnode.ListGroupOUChildren();
                        if (dirnode.ObjectClass.Trim().ToLower().Equals("group"))
                        {
                            IsValidObject = true;
                        }
                        groupInfo.groupName = dirnode.DistinguishedName;
                    }
                    
                    if (_PageType == MEMBERS_PAGE)
                    {
                        dirnode.ListGroupAndUserOUChildren();
                        if (dirnode.ObjectClass.Trim().ToLower().Equals("group") ||
                        dirnode.ObjectClass.Trim().ToLower().Equals("user"))
                        {
                            IsValidObject = true;
                        }
                        groupInfo.objectName = dirnode.DistinguishedName;
                    }
                }
                if (!IsValidObject)
                {
                    Okbtn.Enabled = false;
                }
                else
                {
                    Okbtn.Enabled = true;
                }
            }
        }
    }
    
    private void Okbtn_Click(object sender, EventArgs e)
    {
        this.DialogResult = DialogResult.OK;
        this.Close();
    }
    
    private void CancelBtn_Click(object sender, EventArgs e)
    {
        groupInfo.groupName = null;
        this.Close();
    }
    #endregion
}


public class AdduserInGroupInfo
{
    #region Class Data
    public string groupName = "";
    public string objectName = "";
    public ADUCDirectoryNode newParentDirnode;
    //may add more attributes for group
    #endregion
}
}
