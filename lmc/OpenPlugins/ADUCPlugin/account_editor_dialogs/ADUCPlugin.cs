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

using System.Windows.Forms;
using System.Reflection;
using Likewise.LMC.LDAP;
using Likewise.LMC.Utilities;
using Likewise.LMC.LDAP.Interop;
using Likewise.LMC.Plugins.ADUCPlugin;
using Likewise.LMC.Plugins.ADUCPlugin.Properties;
using Likewise.LMC.ServerControl;
using Likewise.LMC.Netlogon;
using Likewise.LMC.UtilityUIElements;
using System;
using System.Collections.Generic;
using System.Net;
using System.Xml;
using System.Threading;

namespace Likewise.LMC.Plugins.ADUCPlugin
{
public class ADUCPlugin: IPlugIn
{
    #region Class Data
    public IPlugInContainer _container;
    private Hostinfo         _hn = null;
    public LACTreeNode _pluginNode = null;
    private DirectoryContext _adContext = null;
    private string _rootDN;
    private string _shortDomainName = null;
    
    private string _defaultUnixHomedir = null;
    private string _loginShell = null; 
    
    private Dictionary<string, ADUCDomain> _domainList = new Dictionary<string, ADUCDomain>();
    private ADUCDomain _currentDomain = null;
    
    private bool _usingSimpleBind = false;
    public bool bIsNetInitCalled = false;
    private ListView _aducPagelvChildNodes;
    
    public Dictionary<string, Form> Propertywindowhandles = new Dictionary<string, Form>();

    private List<IPlugIn> _extPlugins = null;
    
    #endregion
    
    #region IPlugIn Members
    
    public Hostinfo HostInfo
    {
        get
        {
            return _hn;
        }
        set
        {
            _hn = value;
        }
    }                              

    public List<IPlugIn> ExtPlugins
    {
        get
        {
            return _extPlugins;
        }
    }
    
    public ListView aducPagelvChildNodes
    {
        get
        {
            return _aducPagelvChildNodes;
        }
        set
        {
            _aducPagelvChildNodes = value;
        }
    }

    public void AddExtPlugin(IPlugIn extPlugin)
    {
        if (_extPlugins == null)
        {
            _extPlugins = new List<IPlugIn>();
        }

        _extPlugins.Add(extPlugin);
    }
    
    /// <summary>
    /// Gets the name of the current plugin
    /// </summary>
    /// <returns></returns>
    public string GetName()
    {
        Logger.Log("ADUCPlugin.GetName", Logger.manageLogLevel);
        
        return Resources.ADUCTitle;
    }

    public string GetDescription()
    {
        return Resources.ADUCDescription;
    }

    public string GetPluginDllName()
    {
        return "Likewise.LMC.Plugins.ADUCPlugin.dll";        
    }

    public IContextType GetContextType()
    {
        return IContextType.Hostinfo;
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
            Logger.LogException("ADUCPlugin.SerializePluginInfo()", ex);
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
            Logger.LogException("ADUCPlugin.DeserializePluginInfo()", ex);
        }
    }
    
    /// <summary>
    /// Initializes the plugin container information for the selected plugin
    /// </summary>
    /// <param name="container"></param>
    public void Initialize(IPlugInContainer container)
    {
        Logger.Log("ADUCPlugin.Initialize", Logger.manageLogLevel);
        
        _container = container;
    }
    
    /// <summary>
    /// If Host information is not null then refreshes the ADUC plugin for each domain on mouse left click
    /// Else reinitializes the ADUC plugin node
    /// </summary>
    /// <param name="hn"></param>
    public void SetContext(IContext ctx)
    {
        Hostinfo hn = ctx as Hostinfo;
        
        Logger.Log(String.Format("ADUCPlugin.SetHost(hn: {0})",
        hn == null ? "null" : hn.ToString()), Logger.manageLogLevel);
        
        _hn = hn;
        
        if (_hn != null &&
        !String.IsNullOrEmpty(_hn.domainName) &&
        !(_usingSimpleBind && !Hostinfo.HasCreds(_hn)))
        {            
            Logger.Log("ADUCPlugin.SetHost: connecting to domain");
           
            ConnectToDomain();
            
            if (!_usingSimpleBind && !_hn.IsConnectionSuccess)
            {
                _usingSimpleBind = true;
            }

            //sethost on all the extension plugins
            if (_extPlugins != null)
            {
                foreach (IPlugIn extPlugin in _extPlugins)
                    extPlugin.SetContext(_hn);
            }
        }
    }
    
    public IContext GetContext()
    {
        return _hn;
    }
    
    /// <summary>
    /// Gets the ADUC Plugin Node
    /// </summary>
    /// <returns></returns>
    public LACTreeNode GetPlugInNode()
    {
        Logger.Log("ADUCPlugin.GetPluginNode", Logger.manageLogLevel);
        
        return GetADUCRootNode();
    }
    
    /// <summary>
    /// If HostInfo is not null,lists the all child nodes on click of any node in left hand pane of ADUC
    /// </summary>
    /// <param name="parentNode"></param>
    public void EnumChildren(LACTreeNode parentNode)
    {
        Logger.Log("ADUCPlugin.EnumChildren", Logger.manageLogLevel);
        
        if (!Hostinfo.HasCreds(_hn))
        {
            //attempt to retrieve host info information from kerberos
            Hostinfo defaultHostInfo = new Hostinfo();
            
            //assume for now that the AD hostname is the same as the LDAP domain
            defaultHostInfo.creds.MachineName = defaultHostInfo.domainName;
            defaultHostInfo.hostName = defaultHostInfo.domainName;
            
            Logger.Log(defaultHostInfo.domainName, Logger.manageLogLevel);
            
            //LDAP is not yet kerberized, so set password to null to make sure the user gets prompted
            defaultHostInfo.creds.Password = null;
        }
    }
    
    /// <summary>
    /// Gets the Plugin directory context object which initializes the LDAP base
    /// </summary>
    /// <returns></returns>
    public DirectoryContext GetpluginContext()
    {
        Logger.Log("ADUCPlugin.GetpluginContext", Logger.manageLogLevel);
        
        return _adContext;
    }
    
    /// <summary>
    /// Returns the Root Domain if plugin connected ti any domain
    /// </summary>
    /// <returns></returns>
    public string GetRootDN()
    {
        Logger.Log("ADUCPlugin.GetRootDN", Logger.manageLogLevel);
        
        return _rootDN;
    }
    
    public object GetLdapAP()
    {
        Logger.Log("ADUCPlugin.GetLdapAP", Logger.manageLogLevel);
        
        return null;
        
    }
    
    /// <summary>
    /// Sets the cursor state while processing....
    /// </summary>
    /// <param name="cursor"></param>
    public void SetCursor(System.Windows.Forms.Cursor cursor)
    {
        Logger.Log("ADUCPlugin.SetCursor", Logger.manageLogLevel);
        
        if (_container != null)
        {
            _container.SetCursor(cursor);
        }
    }
    
    /// <summary>
    /// Is a delegate that will be called when we click on any node from the treeview
    /// And Initializes the contextmenu
    /// </summary>
    /// <param name="nodeClicked"></param>
    /// <returns></returns>
    public ContextMenu GetTreeContextMenu(LACTreeNode nodeClicked)
    {
        Logger.Log("ADUCPlugin.GetTreeContextMenu", Logger.manageLogLevel);
        
        if (nodeClicked == null)
        {
            return null;
        }
        else
        {
            ADUCPage aducPage = (ADUCPage) nodeClicked.PluginPage;
            if (aducPage == null)
            {
                Type type = nodeClicked.NodeType;
                
                object o = Activator.CreateInstance(type);
                if (o is IPlugInPage)
                {
                    ((IPlugInPage)o).SetPlugInInfo(_container, nodeClicked.Plugin, nodeClicked, (LWTreeView) nodeClicked.TreeView, nodeClicked.sc);
                    aducPage = (ADUCPage)nodeClicked.PluginPage;
                }
                
            }
            ContextMenu aducContextMenu = null;
            if (aducPage != null)
            {
                if (nodeClicked is ADUCDirectoryNode)
                {
                    aducContextMenu = aducPage.GetTreeContextMenu_curr(nodeClicked as ADUCDirectoryNode);
                }
                else
                {
                    aducContextMenu = aducPage.GetTreeContextMenu();
                }
            }
            if (_pluginNode == nodeClicked)
            {
                if (aducContextMenu == null)
                {
                    aducContextMenu = new ContextMenu();
                }
                
                MenuItem m_item = new MenuItem("Connect to Domain", new EventHandler(cm_OnConnect));
                aducContextMenu.MenuItems.Add(0, m_item);
                
                aducContextMenu.MenuItems.Add(new MenuItem("-"));
                
                m_item = new MenuItem("Refresh", new EventHandler(cm_OnMenuClick));
                m_item.Tag = nodeClicked;
                aducContextMenu.MenuItems.Add(m_item);
                
                aducContextMenu.MenuItems.Add(new MenuItem("-"));
                
                m_item = new MenuItem("Help", new EventHandler(aducPage.cm_OnMenuClick));
                aducContextMenu.MenuItems.Add(m_item);
            }
            
            ADUCDirectoryNode dirnode = nodeClicked as ADUCDirectoryNode;
            
            return aducContextMenu;
        }
    }
    
    public void SetSingleSignOn(bool useSingleSignOn)
    {
        _usingSimpleBind = !useSingleSignOn;
    }
    
    #endregion
    
    #region HelperFunctions
    
    /// <summary>
    /// Event raises when we click on any contextmenu item
    /// And then performs the specified action
    /// </summary>
    /// <param name="sender"></param>
    /// <param name="e"></param>
    private void cm_OnMenuClick(object sender, EventArgs e)
    {
        // assure that the sender is a MenuItem
        MenuItem mi = sender as MenuItem;
        LACTreeNode lacnode = mi.Tag as LACTreeNode;
        if (lacnode != null && lacnode.Nodes.Count != 0)
        {
            ADUCDirectoryNode rootNode = lacnode.Nodes[0] as ADUCDirectoryNode;
            ADUCPage pluginpage = lacnode.PluginPage as ADUCPage;
            if (pluginpage != null)
            {
                pluginpage.TreeNode.IsModified = true;
                rootNode.IsModified = true;
                pluginpage.RefreshPluginPage();
                pluginpage.TreeNode.TreeView.SelectedNode = pluginpage.TreeNode;
                rootNode.ListChildren();
                if (rootNode.IsExpanded)
                {
                    rootNode.toggleLACNode();
                }
            }
        }
    }
    
    /// <summary>
    /// Method will be called when we try to connect to Host from the ADUC Plugin
    /// </summary>
    /// <param name="sender"></param>
    /// <param name="e"></param>
    private void cm_OnConnect(object sender, EventArgs e)
    {
        bool initialConnect = true;

        while (true)
        {
            SelectDomainDialog domainDlg = null;
            if (initialConnect)
            {
                domainDlg = new SelectDomainDialog(_hn.domainName, _hn.creds.UserName);
                if (domainDlg.ShowDialog() == DialogResult.OK)
                {
                    _hn.domainName = domainDlg.GetDomain();

                    if (!domainDlg.UseDefaultUserCreds())
                    {
                        _hn.creds.UserName = domainDlg.GetUsername();
                        _hn.creds.Password = domainDlg.GetPassword();
                    }
                }
            }

            if (!ConnectToDomain())
            {
                MessageBox.Show(
                   "Unable to connect to domain.",
                   CommonResources.GetString("Caption_Console"),
                   MessageBoxButtons.OK,
                   MessageBoxIcon.Exclamation);

                if (!domainDlg.UseDefaultUserCreds())
                {
                    CredentialsDialog credsDialog = new CredentialsDialog(_hn.creds.UserName);
                    if (credsDialog.ShowDialog() == DialogResult.OK)
                    {
                        initialConnect = false;
                        continue;
                    }
                }
            }
        }
    }
    
    /// <summary>
    /// Method that creates and initializes the ADUC Rootnode
    /// </summary>
    /// <returns></returns>
    private LACTreeNode GetADUCRootNode()
    {
        Logger.Log("ADUCPlugin.GetADUCRootNode", Logger.manageLogLevel);
        
        if (_pluginNode == null)
        {
            _hn = new Hostinfo();            
            _pluginNode = Manage.CreateIconNode(Resources.ADUCTitle,
            Resources.ADUC,
            typeof(ADUCPage),
            this);
            _pluginNode.IsPluginNode = true;
        }
        _pluginNode.ImageIndex = (int)Manage.ManageImageType.Generic;
        _pluginNode.SelectedImageIndex = (int)Manage.ManageImageType.Generic;
        
        
        return _pluginNode;
    }
    
    /// <summary>
    /// Will be called when we click on 'Set Target Machine Info' menu item and if plugin is already connected to domain
    /// Will gets all nodes along with its childs
    /// </summary>
    private bool ConnectToDomain()
    {
        Logger.Log("ADUCPlugin.ConnectToDomain", Logger.manageLogLevel);
        
        if (_usingSimpleBind &&
        (!Hostinfo.HasCreds(_hn) || _hn.creds.Invalidated))
        {
            _container.ShowError("ADUC cannot connect to domain due to invalid credentials");
            _hn.IsConnectionSuccess = false;
            return _hn.IsConnectionSuccess;
        }
        
        //Make sure one ADUC plugin only hosts one domain

        if (_aducPagelvChildNodes != null)
        {
            _aducPagelvChildNodes.Items.Clear();
        }
        
        if (_pluginNode != null && _pluginNode.Nodes.Count != 0)
        {
            foreach (TreeNode treeNode in _pluginNode.Nodes)
            {
                LACTreeNode lacTreeNode = treeNode as LACTreeNode;
                if (lacTreeNode != null && lacTreeNode is LACTreeNode)
                {
                    if (!lacTreeNode._IsPlugIn)
                    {
                        _pluginNode.Nodes.Remove(treeNode);
                    }
                    else
                    {
                        ListViewItem lvItem = new ListViewItem(treeNode.Text);
                        lvItem.Tag = treeNode;
                        lvItem.ImageIndex = (int)treeNode.ImageIndex;
                        _aducPagelvChildNodes.Items.Add(lvItem);
                    }
                }
            }
        }
        
        _currentDomain = new ADUCDomain();
        _currentDomain.HostInfo = _hn;
        _currentDomain.plugin = this;
        
        if (_currentDomain.ConnectToDomain(_usingSimpleBind) != null)
        {
            if (!_domainList.ContainsKey(_currentDomain.rootDN.ToLower()))
            {
                _domainList.Add(_currentDomain.rootDN.ToLower(), _currentDomain);
            }
            
            _pluginNode.Text = string.Format("Active Directory Users & Computers [{0}]", _hn.domainName);
            
            if (_currentDomain.rootNode != null)
            {
                _currentDomain.rootNode.sc = _pluginNode.sc;
                //_pluginNode.Nodes.Add(domain.rootNode);
                bool IsFound = false;
                if (_pluginNode != null && _pluginNode.Nodes.Count != 0)
                {
                    int index = 0;
                    foreach (TreeNode treeNode in _pluginNode.Nodes)
                    {
                        IsFound = false;
                        ADUCDirectoryNode node = treeNode as ADUCDirectoryNode;
                        if (node != null && node.Name.Trim().ToLower().Equals(_currentDomain.rootNode.Name.Trim().ToLower()))
                        {
                            IsFound = true;
                            _pluginNode.Nodes.RemoveAt(index);
                            _pluginNode.Nodes.Insert(index, _currentDomain.rootNode);
                            break;
                        }
                        index++;
                    }
                }
                if (!IsFound)
                {
                    _pluginNode.Nodes.Add(_currentDomain.rootNode);
                }
                
                _currentDomain.rootNode.Collapse();
                _currentDomain.rootNode.ExpandAll();
            }
            
            _adContext = _currentDomain.adContext;
            _rootDN = _currentDomain.rootDN;
            _shortDomainName = _currentDomain.ShortDomainName;
            
            _defaultUnixHomedir = _currentDomain.DefaultUnixHomeDir;
            
            _loginShell = _currentDomain.LoginShell;          
            _hn.IsConnectionSuccess = true;
        }
        else
        {            
            _hn.IsConnectionSuccess = false;
        }
        
        return _hn.IsConnectionSuccess;
    }
    
    
    /// <summary>
    /// Method used to print the Ldap contents to the log
    /// </summary>
    /// <param name="logLevel"></param>
    private void printLdapContentsToLog(Logger.LogLevel logLevel)
    {
        
        if (Logger.currentLogLevel < logLevel)
        {
            return;
        }
        
        string[] search_attrs = { null };
        
        string distinguishedName = "cn=schema,cn=configuration,dc=corpqa,dc=centeris,dc=com";
        
        DateTime start = DateTime.Now;
        LdapMessage ldapMessage = _adContext.SearchSynchronous(distinguishedName,
        LdapAPI.LDAPSCOPE.SUB_TREE,
        "objectClass=*",
        search_attrs,
        false);
        
        
        DateTime finish = DateTime.Now;
        Logger.Log(String.Format("finish ldap schema: scope=1 basedn={0} delta-time={1}",
        distinguishedName, finish - start), Logger.timingLogLevel);
        
        if (ldapMessage == null)
        {
            Logger.Log("ldapMessage = null");
        }
        else
        {
            List<LdapEntry> ldapEntries = ldapMessage.Ldap_Get_Entries();
            if (ldapEntries == null || ldapEntries.Count == 0)
            {
                return;
            }
            
            foreach (LdapEntry ldapNextEntry in ldapEntries)
            {
                string s = ldapNextEntry.GetDN();
                Logger.Log(String.Format("DN = {0}", s));
                
                List<string> attrNames = new List<string>();
                string[] tmpAttrNames = ldapNextEntry.GetAttributeNames();
                foreach (string attrName in tmpAttrNames)
                {
                    attrNames.Add(attrName);
                }
                if (!attrNames.Contains("objectClass"))
                {
                    attrNames.Add("objectClass");
                }
                
                foreach (string attr in attrNames)
                {
                    LdapValue[] values = ldapNextEntry.GetAttributeValues(attr, _adContext);
                    if (values != null && values.Length > 0)
                    {
                        foreach (LdapValue value in values)
                        {
                            if (value != null)
                            {
                                Logger.Log(String.Format("\t{0} ==> {1}", attr, value.stringData));
                            }
                            else
                            {
                                Logger.Log(String.Format("\t{0} ==> NULL", attr));
                            }
                        }
                    }
                    else
                    {
                        Logger.Log(String.Format("\t{0} ==> UNSET", attr));
                    }
                }
            }
        }
    }
    
    #endregion
    
    #region accessor functions
    public DirectoryContext adContext
    {
        get
        {
            return _adContext;
        }
    }
    
    public Dictionary<string, ADUCDomain> domainList
    {
        get
        {
            return _domainList;
        }
    }
    
    public ADUCDomain currentDomain
    {
        get
        {
            return _currentDomain;
        }
    }
    
    public string ShortDomainName
    {
        get
        {
            return _shortDomainName;
        }
    }
    
    public string DefaultUnixHomeDir
    {
        get
        {
            return _defaultUnixHomedir;
        }
        set
        {
            _defaultUnixHomedir = value;
        }
    }
    
    public string LoginShell
    {
        get
        {
            return _loginShell;
        }
        set
        {
            _loginShell = value;
        }
    }
    #endregion
    
}
}
