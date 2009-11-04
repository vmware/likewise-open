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
using Likewise.LMC.Plugins.LUG.Properties;
using Likewise.LMC.ServerControl;
using System.Windows.Forms;
using Likewise.LMC.Utilities;
using Likewise.LMC.NETAPI;
using Likewise.LMC.Netlogon;
using System;
using System.Xml;
using System.Collections.Generic;
using Likewise.LMC.Plugins.LUG.src;
using Likewise.LMC.UtilityUIElements;

namespace Likewise.LMC.Plugins.LUG
{
class LUGPlugIn: IPlugIn
{
    #region Constants
    
    // Don't localize this!
    private const string sNameIPlugIn = "Local Users and Groups";
    
    #endregion
    
    #region Class data
    
    private IPlugInContainer _container = null;
    public Hostinfo _hn = null;
    private LACTreeNode _pluginNode = null;
    private LACTreeNode uNode = null;
    private LACTreeNode gNode = null;
    public bool _usingManualCreds = false;
    public bool IsNetApiInitCalled = false;

    private List<IPlugIn> _extPlugins = null;

    #endregion
    
    #region IPlugIn Members
    
    public string GetName()
    {
        return sNameIPlugIn;
    }

    public string GetDescription()
    {
        return "Lets you manage Likewise local users and groups on a computer";
    }

    public string GetPluginDllName()
    {
#if !QUARTZ
        return "Likewise.LMC.Plugins.LUGPlugin.dll";
#else
        return "Likewise.LMC.Plugins.LUGPlugin.unix.dll";
#endif
    }

    public IContextType GetContextType()
    {
        return IContextType.Hostinfo;
    }

    void IPlugIn.Initialize(IPlugInContainer container)
    {
        Logger.Log("LUGManagerPlugin.Initialize", Logger.manageLogLevel);

        _container = container;
    }

    public void SerializePluginInfo(LACTreeNode pluginNode, ref int Id, out XmlElement viewElement, XmlElement ViewsNode, TreeNode SelectedNode)
    {
        viewElement = null;

        try
        {
            if (pluginNode == null || !pluginNode._IsPlugIn)
                return;

            XmlElement HostInfoElement = null;

            Manage.InitSerializePluginInfo(pluginNode, this, ref Id, out viewElement, ViewsNode, SelectedNode);
            Manage.CreateAppendHostInfoElement(_hn, ref viewElement, out HostInfoElement);

            if (pluginNode != null && pluginNode.Nodes.Count != 0)
            {
                foreach (LACTreeNode lacnode in pluginNode.Nodes)
                {
                    XmlElement innerelement = null;
                    pluginNode.Plugin.SerializePluginInfo(lacnode, ref Id, out innerelement, viewElement, SelectedNode);
                    if (innerelement != null)
                    {
                        viewElement.AppendChild(innerelement);
                    }
                }
            }
        }
        catch (Exception ex)
        {
            Logger.LogException("LUGPlugin.SerializePluginInfo()", ex);
        }
    }

    public void DeserializePluginInfo(XmlNode node, ref LACTreeNode pluginNode, string nodepath)
    {
        try
        {
            Manage.DeserializeHostInfo(node, ref pluginNode, nodepath, ref _hn, false);
            pluginNode.Text = this.GetName();
            pluginNode.Name = this.GetName();
        }
        catch (Exception ex)
        {
            Logger.LogException("LUGPlugin.DeserializePluginInfo()", ex);
        }
    }
    
    public void Initialize(IPlugInContainer container)
    {
        _container = container;
    }
    
    public void SetContext(IContext ctx)
    {
        Hostinfo hn = ctx as Hostinfo;

        Logger.Log(String.Format("LUGPlugin.SetHost(hn: {0})",
        hn == null ? "null" : hn.hostName), Logger.manageLogLevel);
        
        _hn = hn;
        
        if (_hn == null)
        {
            _hn = new Hostinfo();
        }

        if (_pluginNode != null &&
        (!String.IsNullOrEmpty(hn.hostName)))// && Hostinfo.HasCreds(hn)))
        //!(_usingManualCreds && !Hostinfo.HasCreds(_hn)))
        {
            hn.IsConnectionSuccess =
            Session.EnsureNullSession(hn.hostName, hn.creds);

            _pluginNode.SetContext(_hn);

            hn.IsConnectionSuccess = true;

            if (!hn.IsConnectionSuccess)
            {
                _container.ShowError(String.Format("Unable to connect with {0}", hn.hostName));
                _usingManualCreds = true;
            }
        }
    }
    
    public IContext GetContext()
    {
        return _hn;
    }
    
    public LACTreeNode GetPlugInNode()
    {
        return GetLUGNode();
    }
    
    public void EnumChildren(LACTreeNode parentNode)
    {
        return;
    }
    
    public void SetCursor(System.Windows.Forms.Cursor cursor)
    {
        if (_container != null)
        {
            _container.SetCursor(cursor);
        }
    }
    
    public ContextMenu GetTreeContextMenu(LACTreeNode nodeClicked)
    {
        ContextMenu cm = null;
        
        if (nodeClicked == _pluginNode)
        {
            cm = new ContextMenu();
            MenuItem m_item = new MenuItem("Connect to...", new EventHandler(cm_OnConnect));
            m_item.Tag = nodeClicked;
            cm.MenuItems.Add(m_item);
        }
        else if (nodeClicked == uNode)
        {
            cm = new ContextMenu();
            MenuItem m_item = new MenuItem("Add New User", new EventHandler(cm_OnCreateUser));
            m_item.Tag = nodeClicked;
            cm.MenuItems.Add(m_item);
        }
        else if (nodeClicked == gNode)
        {
            cm = new ContextMenu();
            MenuItem m_item = new MenuItem("Add New Group", new EventHandler(cm_OnCreateGroup));
            m_item.Tag = nodeClicked;
            cm.MenuItems.Add(m_item);
        }
        
        return cm;
    }
    
    public void SetSingleSignOn(bool useSingleSignOn)
    {
        //_usingManualCreds = useSingleSignOn;
    }


    public void AddExtPlugin(IPlugIn extPlugin)
    {
        if (_extPlugins == null)
        {
            _extPlugins = new List<IPlugIn>();
        }

        _extPlugins.Add(extPlugin);
    }

    public bool PluginSelected()
    {
        return SelectComputer();
    }
    
    #endregion
    
    #region context menus
    private void cm_OnConnect(object sender, EventArgs e)
    {
        Logger.Log(String.Format(
        "LUGPlugin.cm_OnConnect: _usingManualCreds: {0}, hn: {1}",
        _usingManualCreds, _hn));

        SelectComputer();
    }

    private void cm_OnCreateUser(object sender, EventArgs e)
    {
        MenuItem m_item = sender as MenuItem;
        LACTreeNode userNode = m_item.Tag as LACTreeNode;
        if (userNode != null)
        {
            LUGPage page = null;

            if (userNode.PluginPage == null)
            {
                Type type = userNode.NodeType;

                object o = Activator.CreateInstance(type);
                if (o is IPlugInPage)
                {
                    ((IPlugInPage)o).SetPlugInInfo(_container, userNode.Plugin, userNode, (LWTreeView)userNode.TreeView, userNode.sc);
                    page = (LUGPage)userNode.PluginPage;
                }

            }
            else
            {
                page = (LUGPage)userNode.PluginPage;
            }
            if (page != null)
            {
                page.CreateDlg();
            }
        }
    }

    private void cm_OnCreateGroup(object sender, EventArgs e)
    {
        MenuItem m_item = sender as MenuItem;
        LACTreeNode groupNode = m_item.Tag as LACTreeNode;
        if (groupNode != null)
        {
            LUGPage page = null;

            if (groupNode.PluginPage == null)
            {
                Type type = groupNode.NodeType;

                object o = Activator.CreateInstance(type);
                if (o is IPlugInPage)
                {
                    ((IPlugInPage)o).SetPlugInInfo(_container, groupNode.Plugin, groupNode, (LWTreeView)groupNode.TreeView, groupNode.sc);
                    page = (LUGPage)groupNode.PluginPage;
                }
            }
            else
            {
                page = (LUGPage)gNode.PluginPage;
            }
            if (page != null)
            {
                page.CreateDlg();
            }
        }
    }
    
    #endregion
    
    #region Private helper functions
    private LACTreeNode GetLUGNode()
    {
        
        if (_pluginNode == null)
        {
            Logger.Log(
            "LUGPlugIn.GetLUGNode: _pluginNode == null",
            Logger.manageLogLevel);
            
            _pluginNode = new LACTreeNode("Local Users and Groups", Resources.LocalGroup_16, typeof(DummyLUGPage), this);

            uNode = new LACTreeNode("Users", Resources.LocalGroup_16, typeof(LocalUserPage), this);
            uNode.sc = _pluginNode.sc;
            _pluginNode.Nodes.Add(uNode);

            gNode = new LACTreeNode("Groups", Resources.LocalGroup_16, typeof(LocalGroupPage), this);
            gNode.sc = _pluginNode.sc;
            _pluginNode.Nodes.Add(gNode);
            
        } 
        
        foreach (TreeNode node in _pluginNode.Nodes)
        {
            LACTreeNode lacNode = (LACTreeNode)node;
            if (lacNode != null)
            {
                lacNode.SetContext(_hn);
                
                if (lacNode.NodeType == typeof(LocalUserPage))
                {
                    lacNode.ImageIndex = (int)Manage.ManageImageType.User;
                    lacNode.SelectedImageIndex = (int)Manage.ManageImageType.User;
                }
                else if (lacNode.NodeType == typeof(LocalGroupPage))
                {
                    lacNode.ImageIndex = (int)Manage.ManageImageType.Group;
                    lacNode.SelectedImageIndex = (int)Manage.ManageImageType.Group;
                }
            }
        }
        _pluginNode.ImageIndex = (int)Manage.ManageImageType.Group;
        _pluginNode.SelectedImageIndex = (int)Manage.ManageImageType.Group;

        _pluginNode.IsPluginNode = true;

        return _pluginNode;
    }

    private bool SelectComputer()
    {
        Logger.Log(String.Format(
        "LUGPlugin.SelectComputer: hn: {0}",
        _hn));

        SelectComputerDialog selectDlg = new SelectComputerDialog(
            _hn.hostName,
            _hn.creds.UserName);

        if (selectDlg.ShowDialog() == DialogResult.OK)
        {
            int result = (int)ErrorCodes.WIN32Enum.ERROR_SUCCESS;

            _hn.hostName = selectDlg.GetHostname();

            if (!selectDlg.UseDefaultUserCreds())
            {
                _hn.creds.UserName = selectDlg.GetUsername();
                _hn.creds.Password = selectDlg.GetPassword();
            }
            else
            {
                _hn.creds.UserName = null;
                _hn.creds.Password = null;
            }

            result = (int)LUGAPI.NetAddConnection(
                _hn.hostName,
                _hn.creds.UserName,
                _hn.creds.Password);

            if (result != (int)ErrorCodes.WIN32Enum.ERROR_SUCCESS)
            {
                MessageBox.Show(
                   "Unable to connect to system:\n" + ErrorCodes.WIN32String(result),
                   "Likewise Administrative Console",
                   MessageBoxButtons.OK,
                   MessageBoxIcon.Exclamation);

                return false;
            }

            return true;
        }

        return false;
    }

    #endregion
    
}

}
