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
using System;
using System.Xml;
using System.Collections.Generic;
using Likewise.LMC.Plugins.FileBrowser.Properties;
using Likewise.LMC.ServerControl;
using Likewise.LMC.Utilities;
using Likewise.LMC.UtilityUIElements;
using Likewise.LMC.NETAPI;
using Likewise.LMC.FileClient;
using Likewise.LMC.Plugins.FileBrowser;


namespace Likewise.LMC.Plugins.FileBrowser
{
    public class FileBrowserIPlugIn : IPlugIn
    {
        #region Enum variables

        public enum PluginNodeType
        {
            SHARES,
            SESSIONS,
            OPENFILES,
            PRINTERS,
            UNDEFINED
        }
        #endregion

        #region Class data

        private IPlugInContainer _container;
        private Hostinfo _hn;
        private LACTreeNode _pluginNode;
        public FileHandle fileHandle = null;
        private string _disconnectShare = null;

        private List<IPlugIn> _extPlugins = null;

        #endregion

        #region IPlugIn Members

        public Hostinfo HostInfo
        {
            get
            {
                return _hn;
            }
        }

        public string GetName()
        {
            Logger.Log("FileBrowserPlugIn.GetName", Logger.FileBrowserLogLevel);

            return Resources.sTitleTopLevelTab;
        }

        public string GetDescription()
        {
            return Resources.PluginDescription;
        }

        public string GetPluginDllName()
        {
            return "Likewise.LMC.Plugins.FileBrowser.dll";
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
                XmlElement gpoInfoElement = null;
                GPObjectInfo gpoInfo = pluginNode.Tag as GPObjectInfo;

                Manage.InitSerializePluginInfo(pluginNode, this, ref Id, out viewElement, ViewsNode, SelectedNode);

                Manage.CreateAppendHostInfoElement(_hn, ref viewElement, out HostInfoElement);
                Manage.CreateAppendGPOInfoElement(gpoInfo, ref HostInfoElement, out gpoInfoElement);

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
                Logger.LogException("FileBrowserPlugin.SerializePluginInfo()", ex);
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
                Logger.LogException("FileBrowserPlugin.DeserializePluginInfo()", ex);
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

        public void Initialize(
            IPlugInContainer container
            )
        {
            Logger.Log("FileBrowserPlugIn.Initialize", Logger.FileBrowserLogLevel);

            _container = container;
        }

        public void SetContext(
            IContext ctx
            )
        {
            Hostinfo hn = ctx as Hostinfo;

            Logger.Log(String.Format("FileBrowserPlugIn.SetHost(hn: {0}\n)",
            hn == null ? "<null>" : hn.ToString()), Logger.FileBrowserLogLevel);

            bool deadTree = false;

            if (_pluginNode != null &&
                _pluginNode.Nodes != null &&
                _hn != null &&
                hn != null &&
                hn.hostName !=
                _hn.hostName)
            {
                foreach (TreeNode node in _pluginNode.Nodes)
                {
                    _pluginNode.Nodes.Remove(node);
                }
                deadTree = true;
            }

            _hn = hn;

            if (HostInfo == null)
            {
                _hn = new Hostinfo();
            }

            if (_pluginNode != null && _pluginNode.Nodes.Count == 0)
            {
                BuildNodesToPlugin();
            }

            if (deadTree && _pluginNode != null)
            {
                _pluginNode.SetContext(_hn);
            }
        }

        public IContext GetContext(
            )
        {
            return _hn;
        }

        public LACTreeNode GetPlugInNode(
            )
        {
            return GetFileBrowserNode();
        }

        private List<NETRESOURCE> GetNetworkConnections(
            )
        {
            NETRESOURCE NetResource = new NETRESOURCE();

            NetResource.dwScope = ResourceScope.RESOURCE_CONNECTED;
            NetResource.dwUsage = ResourceUsage.RESOURCEUSAGE_ALL;
            NetResource.dwType = ResourceType.RESOURCETYPE_DISK;

            return GetChildNetResources(ResourceScope.RESOURCE_CONNECTED,
                                        ResourceType.RESOURCETYPE_DISK,
                                        ResourceUsage.RESOURCEUSAGE_ALL,
                                        NetResource);
        }

        private List<NETRESOURCE> GetSharesForServer(
            string serverName
            )
        {
            NETRESOURCE NetResource = new NETRESOURCE();

            NetResource.dwScope = ResourceScope.RESOURCE_GLOBALNET;
            NetResource.dwUsage = ResourceUsage.RESOURCEUSAGE_CONNECTABLE;
            NetResource.dwType = ResourceType.RESOURCETYPE_DISK;
            NetResource.pRemoteName = serverName;

            return GetChildNetResources(ResourceScope.RESOURCE_GLOBALNET,
                                        ResourceType.RESOURCETYPE_DISK,
                                        ResourceUsage.RESOURCEUSAGE_CONNECTABLE,
                                        NetResource);
        }

        private List<NETRESOURCE> GetChildNetResources(
            ResourceScope dwScope,
            ResourceType dwType,
            ResourceUsage dwUsage,
            NETRESOURCE NetResource
            )
        {
            WinError error = WinError.NO_ERROR;
            IntPtr handle = new IntPtr();
            List<NETRESOURCE> nrList = new List<NETRESOURCE>();

            error = FileClient.FileClient.BeginEnumNetResources(dwScope,
                                                                dwType,
                                                                dwUsage,
                                                                NetResource,
                                                                out handle);

            if (error == WinError.NO_ERROR)
            {
                error = FileClient.FileClient.EnumNetResources(handle, out nrList);

                FileClient.FileClient.EndEnumNetResources(handle);
            }

            return nrList;
        }

        private void AddShareNodes(
            LACTreeNode parentNode,
            List<NETRESOURCE> NetResources
            )
        {
            foreach (NETRESOURCE NetResource in NetResources)
            {
                string name = NetResource.pLocalName;

                if (name == null)
                    name = NetResource.pRemoteName;

                TreeNode[] found = parentNode.Nodes.Find(name, false);

                if (found == null || found.Length == 0)
                {
                    LACTreeNode node = Manage.CreateIconNode(name,
                                         Resources.SharedFolder2,
                                         typeof(FilesDetailPage),
                                         this);
                    node.Tag = NetResource;
                    parentNode.Nodes.Add(node);
                }
            }
        }

        private void AddFolderNode(
            LACTreeNode parentNode,
            NETRESOURCE NetResource
            )
        {
            string name = NetResource.pLocalName;

            if (name == null)
                name = NetResource.pRemoteName;

            LACTreeNode node = Manage.CreateIconNode(name,
                                                     Resources.SharedFolder2,
                                                     typeof(FilesDetailPage),
                                                     this);
            node.Tag = NetResource;
            parentNode.Nodes.Add(node);
        }

        private void RefreshNetworkTreeNode()
        {
            TreeNode[] networkNode = this._pluginNode.Nodes.Find(Resources.Network, false);

            if (networkNode != null && networkNode.Length > 0)
            {
                LACTreeNode network = networkNode[0] as LACTreeNode;
                EnumChildren(network);
            }
        }

        public void EnumChildren(LACTreeNode parentNode)
        {
            List<NETRESOURCE> NetResources = new List<NETRESOURCE>();

            if (parentNode == _pluginNode)
            {
                BuildNodesToPlugin();
                return;
            }

            if (parentNode.Name.Equals("Network"))
            {
                //NetResources = GetNetworkConnections();
                NetResources = GetSharesForServer("\\\\glenn-mac");
                AddShareNodes(parentNode, NetResources);
                //RefreshNetworkTreeNode();
                return;
            }

            if (parentNode.Name.Equals("Devices"))
            {
                return;
            }

            if (parentNode.Name.Equals("Computer"))
            {
                return;
            }

            NETRESOURCE NetResource = parentNode.Tag as NETRESOURCE;

            if (NetResource != null)
            {
                NetResources = GetChildNetResources(ResourceScope.RESOURCE_GLOBALNET,
                                                    ResourceType.RESOURCETYPE_ANY,
                                                    ResourceUsage.RESOURCEUSAGE_CONTAINER,
                                                    NetResource);
                foreach (NETRESOURCE nr in NetResources)
                {
                    if ((nr.dwUsage & ResourceUsage.RESOURCEUSAGE_CONTAINER) ==
                        ResourceUsage.RESOURCEUSAGE_CONTAINER)
                    {
                        AddFolderNode(parentNode, nr);
                    }

                    // Add contents to the list for the node.
                }
            }
        }

        public void SetCursor(System.Windows.Forms.Cursor cursor)
        {
            Logger.Log("FileBrowserPlugIn.SetCursor", Logger.FileBrowserLogLevel);

            if (_container != null)
            {
                _container.SetCursor(cursor);
            }
        }

        public ContextMenu GetTreeContextMenu(LACTreeNode nodeClicked)
        {
            this._disconnectShare = null;

            if (nodeClicked == null)
            {
                return null;
            }
            else
            {
                ContextMenu fileBrowserContextMenu = null;

                StandardPage fileBrowserPage = (StandardPage)nodeClicked.PluginPage;

                if (fileBrowserPage == null)
                {
                    Type type = nodeClicked.NodeType;
                    object o = Activator.CreateInstance(type);
                    if (o is IPlugInPage)
                    {
                        ((IPlugInPage)o).SetPlugInInfo(_container, nodeClicked.Plugin, nodeClicked, (LWTreeView)nodeClicked.TreeView, nodeClicked.sc);
                    }
                }
                if (_pluginNode == nodeClicked)
                {
                }
                else if (nodeClicked.Name.Trim().Equals(Resources.Network))
                {
                    fileBrowserContextMenu = new ContextMenu();

                    MenuItem m_item = new MenuItem("Connect to share...", new EventHandler(cm_OnConnectToShare));
                    fileBrowserContextMenu.MenuItems.Add(0, m_item);
                }
                else if (nodeClicked.Parent.Name.Trim().Equals(Resources.Network))
                {
                    fileBrowserContextMenu = new ContextMenu();

                    MenuItem m_item = new MenuItem("Disconnect from share...", new EventHandler(cm_OnDisconnectShare));
                    fileBrowserContextMenu.MenuItems.Add(0, m_item);
                    this._disconnectShare = nodeClicked.Name;
                }
                else if (nodeClicked.Name.Trim().Equals(Resources.Computer))
                {
                    FilesDetailPage detailsPage = fileBrowserPage as FilesDetailPage;
                    if (detailsPage != null)
                    {
                        fileBrowserContextMenu = detailsPage.GetTreeContextMenu();
                    }
                }
                else if (nodeClicked.Name.Trim().Equals(Resources.Home))
                {
                    FilesDetailPage detailsPage = fileBrowserPage as FilesDetailPage;
                    if (detailsPage != null)
                    {
                        fileBrowserContextMenu = detailsPage.GetTreeContextMenu();
                    }
                }
                else if (nodeClicked.Name.Trim().Equals(Resources.Desktop))
                {
                    FilesDetailPage detailsPage = fileBrowserPage as FilesDetailPage;
                    if (detailsPage != null)
                    {
                        fileBrowserContextMenu = detailsPage.GetTreeContextMenu();
                    }
                }
                else if (nodeClicked.Name.Trim().Equals(Resources.Documents))
                {
                    FilesDetailPage detailsPage = fileBrowserPage as FilesDetailPage;
                    if (detailsPage != null)
                    {
                        fileBrowserContextMenu = detailsPage.GetTreeContextMenu();
                    }
                }
                else if (nodeClicked.Name.Trim().Equals(Resources.Music))
                {
                    FilesDetailPage detailsPage = fileBrowserPage as FilesDetailPage;
                    if (detailsPage != null)
                    {
                        fileBrowserContextMenu = detailsPage.GetTreeContextMenu();
                    }
                }
                else if (nodeClicked.Name.Trim().Equals(Resources.Pictures))
                {
                    FilesDetailPage detailsPage = fileBrowserPage as FilesDetailPage;
                    if (detailsPage != null)
                    {
                        fileBrowserContextMenu = detailsPage.GetTreeContextMenu();
                    }
                }
                else if (nodeClicked.Name.Trim().Equals(Resources.Videos))
                {
                    FilesDetailPage detailsPage = fileBrowserPage as FilesDetailPage;
                    if (detailsPage != null)
                    {
                        fileBrowserContextMenu = detailsPage.GetTreeContextMenu();
                    }
                }
                return fileBrowserContextMenu;
            }
        }

        public void SetSingleSignOn(bool useSingleSignOn)
        {
            // do nothing
        }

        #endregion

        #region Private helper functions

        private LACTreeNode GetFileBrowserNode()
        {
            Logger.Log("FileBrowserPlugIn.GetFileBrowserNode", Logger.FileBrowserLogLevel);

            if (_pluginNode == null)
            {
                Icon ic = Resources.SharedFolder2;
                _pluginNode = Manage.CreateIconNode("File Browser", ic, typeof(FilesBrowserPluginPage), this);
                _pluginNode.ImageIndex = (int)Manage.ManageImageType.Generic;
                _pluginNode.SelectedImageIndex = (int)Manage.ManageImageType.Generic;    
               
                _pluginNode.IsPluginNode = true;
            }

            return _pluginNode;
        }

        private void BuildNodesToPlugin()
        {
            if (_pluginNode != null)
            {
                Icon iconNetShare = Resources.SharedFolder2;
                Icon iconFolder = Resources.SharedFolder2;
                Icon iconComputer = Resources.SharedFolder2;
                Icon iconHome = Resources.SharedFolder2;

                LACTreeNode devicesNode = Manage.CreateIconNode(Resources.Devices, iconComputer, typeof(FilesDetailPage), this);
                _pluginNode.Nodes.Add(devicesNode);

                LACTreeNode dvdNode = Manage.CreateIconNode(Resources.CDDVD, iconComputer, typeof(FilesDetailPage), this);
                devicesNode.Nodes.Add(dvdNode);

                LACTreeNode networkNode = Manage.CreateIconNode(Resources.Network, iconNetShare, typeof(FilesDetailPage), this);
                _pluginNode.Nodes.Add(networkNode);

                LACTreeNode computerNode = Manage.CreateIconNode(Resources.Computer, iconComputer, typeof(FilesDetailPage), this);
                _pluginNode.Nodes.Add(computerNode);

                LACTreeNode homeNode = Manage.CreateIconNode(Resources.Home, iconHome, typeof(FilesDetailPage), this);
                computerNode.Nodes.Add(homeNode);

                LACTreeNode deskNode = Manage.CreateIconNode(Resources.Desktop, iconFolder, typeof(FilesDetailPage), this);
                computerNode.Nodes.Add(deskNode);

                LACTreeNode docNode = Manage.CreateIconNode(Resources.Documents, iconFolder, typeof(FilesDetailPage), this);
                computerNode.Nodes.Add(docNode);

                LACTreeNode musicNode = Manage.CreateIconNode(Resources.Music, iconFolder, typeof(FilesDetailPage), this);
                computerNode.Nodes.Add(musicNode);

                LACTreeNode pictNode = Manage.CreateIconNode(Resources.Pictures, iconFolder, typeof(FilesDetailPage), this);
                computerNode.Nodes.Add(pictNode);

                LACTreeNode videoNode = Manage.CreateIconNode(Resources.Videos, iconFolder, typeof(FilesDetailPage), this);
                computerNode.Nodes.Add(videoNode);
            }
        }

        private void cm_OnConnectToShare(object sender, EventArgs e)
        {
            WinError error = WinError.ERROR_SUCCESS;
            bool determinePath = true;
            bool initialConnect = false;
            bool showPathError = false;
            string username = null;
            string path = null;

            while (true)
            {
                if (determinePath)
                {
                    // Determine share path to connect to
                    ConnectToShareDialog connectDialog = new ConnectToShareDialog(path, showPathError);

                    if (connectDialog.ShowDialog() == DialogResult.OK)
                    {
                        showPathError = false;
                        determinePath = false;
                        path = connectDialog.GetPath();

                        if (connectDialog.UseAlternateUserCreds())
                        {
                            error = WinError.ERROR_ACCESS_DENIED;
                        }
                        else
                        {
                            initialConnect = true;
                        }
                    }
                    else
                    {
                        break;
                    }
                }

                if (initialConnect)
                {
                    Application.UseWaitCursor = true;
                    error = FileClient.FileClient.CreateConnection(path, null, null);
                    Application.UseWaitCursor = false;
                    initialConnect = false;
                }

                if (error == WinError.ERROR_SUCCESS)
                {
                    // Refresh Network connection list and exit
                    break;
                }

                if (error == WinError.ERROR_BAD_NET_NAME)
                {
                    // Show share path connect dialog to allow the user to correct the bad path
                    //MessageBox.Show("The network path is unavailable", "File connection error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    showPathError = true;
                    determinePath = true;
                    continue;
                }

                if (error == WinError.ERROR_DOWNGRADE_DETECTED ||
                    error == WinError.ERROR_ACCESS_DENIED ||
                    error == WinError.ERROR_SESSION_CREDENTIAL_CONFLICT ||
                    error == WinError.ERROR_BAD_USERNAME ||
                    error == WinError.ERROR_INVALID_PASSWORD ||
                    error == WinError.ERROR_LOGON_TYPE_NOT_GRANTED)
                {
                    // Prompt for updated user credentials to access share
                    CredentialsDialog credDialog = new CredentialsDialog(username, this);

                    if (credDialog.ShowDialog() == DialogResult.OK)
                    {
                        if (credDialog.UseDefaultUserCreds())
                        {
                            initialConnect = true;
                            username = null;
                            continue;
                        }

                        username = credDialog.GetUsername();

                        Application.UseWaitCursor = true;
                        error = FileClient.FileClient.CreateConnection(path,
                                                                       username,
                                                                       credDialog.GetPassword());
                        Application.UseWaitCursor = false;
                        continue;
                    }
                    else
                    {
                        // Cancel Connect To attempt
                        break;
                    }
                }
                else
                {
                    // Encounter unexpected error
                    break;
                }
            }

            RefreshNetworkTreeNode();
        }

        private void cm_OnDisconnectShare(object sender, EventArgs e)
        {
            string name = this._disconnectShare;
            WinError error = FileClient.FileClient.DeleteConnection(name);

            this._pluginNode.Nodes.Remove(this._pluginNode.Nodes.Find(name, true)[0]);
            RefreshNetworkTreeNode();
        }

        #endregion
    }
}
