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
using Likewise.LMC.FileClient;
using Likewise.LMC.Plugins.FileBrowser;


namespace Likewise.LMC.Plugins.FileBrowser
{
    public class FileBrowserIPlugIn : IPlugIn
    {
        #region Class data

        private LikewiseTargetPlatform platform = Configurations.currentPlatform;
        private IPlugInContainer _container;
        private Hostinfo _hn;
        private FileBrowserNode _pluginNode;
        private LACTreeNode _currentNode = null;
        private LACTreeNode _selectedNode = null;
        private List<IPlugIn> _extPlugins = null;
        private string PathSeparator = "/";
        private List<string> RemoteShares = new List<string>();
        private View _currentView = View.LargeIcon;

        #endregion

        public List<string> GetActiveShares()
        {
            return RemoteShares;
        }

        public View GetCurrentViewStyle()
        {
            return _currentView;
        }

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

            return Resources.String_TitleTopLevelTab;
        }

        public string GetDescription()
        {
            return Resources.String_PluginDescription;
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
        }

        public void DeserializePluginInfo(XmlNode node, ref LACTreeNode pluginNode, string nodepath)
        {
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

            if (platform == LikewiseTargetPlatform.Windows)
            {
                FileClient.FileClient.SetWindowsPlatform();
                PathSeparator = "\\";
            }
            else
            {
                Services.ServiceManagerApi.LW_SERVICE_STATUS status;

                string[] LWISServices = Services.ServiceManagerInteropWrapper.ApiLwSmEnumerateServices();

                IntPtr lwio = Services.ServiceManagerInteropWrapper.ApiLwSmAcquireServiceHandle("lwio");
                status = Services.ServiceManagerInteropWrapper.ApiLwSmQueryServiceStatus(lwio);
                if (status.state != Likewise.LMC.Services.ServiceManagerApi.LW_SERVICE_STATE.LW_SERVICE_STATE_RUNNING)
                {
                    Console.WriteLine("Going to start lwio service since it is not running\n");
                    Services.ServiceManagerInteropWrapper.ApiLwSmStartService(lwio);
                }
                Services.ServiceManagerInteropWrapper.ApiLwSmReleaseServiceHandle(lwio);

                IntPtr rdr = Services.ServiceManagerInteropWrapper.ApiLwSmAcquireServiceHandle("rdr");
                status = Services.ServiceManagerInteropWrapper.ApiLwSmQueryServiceStatus(rdr);
                if (status.state != Likewise.LMC.Services.ServiceManagerApi.LW_SERVICE_STATE.LW_SERVICE_STATE_RUNNING)
                {
                    Console.WriteLine("Going to start rdr driver since it is not running\n");
                    Services.ServiceManagerInteropWrapper.ApiLwSmStartService(rdr);
                }
                Services.ServiceManagerInteropWrapper.ApiLwSmReleaseServiceHandle(rdr);

                IntPtr pvfs = Services.ServiceManagerInteropWrapper.ApiLwSmAcquireServiceHandle("pvfs");
                status = Services.ServiceManagerInteropWrapper.ApiLwSmQueryServiceStatus(pvfs);
                if (status.state != Likewise.LMC.Services.ServiceManagerApi.LW_SERVICE_STATE.LW_SERVICE_STATE_RUNNING)
                {
                    Console.WriteLine("Going to start pvfs driver since it is not running\n");
                    Services.ServiceManagerInteropWrapper.ApiLwSmStartService(pvfs);
                }
                Services.ServiceManagerInteropWrapper.ApiLwSmReleaseServiceHandle(pvfs);
            }

            _container = container;
        }

        public void SetContext(
            IContext ctx
            )
        {
            Hostinfo hn = ctx as Hostinfo;

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

            if (_pluginNode != null)
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

                TreeNode[] found = parentNode.Nodes.Find(NetResource.pRemoteName, false);

                if (found == null || found.Length == 0)
                {
                    FileBrowserNode node = new FileBrowserNode(NetResource.pRemoteName, Resources.Share, typeof(FilesDetailPage), this);
                    node.FBNodeType = FileBrowserNode.FileBrowserNopeType.SHARE;
                    node.Path = NetResource.pRemoteName;
                    parentNode.Nodes.Add(node);
                }
            }
        }

        private void AddShareNodes(
            LACTreeNode parentNode,
            List<string> Shares
            )
        {
            foreach (string Share in Shares)
            {

                TreeNode[] found = parentNode.Nodes.Find(Share, false);

                if (found == null || found.Length == 0)
                {
                    FileBrowserNode node = new FileBrowserNode(Share, Resources.Share, typeof(FilesDetailPage), this);
                    node.FBNodeType = FileBrowserNode.FileBrowserNopeType.SHARE;
                    node.Path = Share;
                    parentNode.Nodes.Add(node);
                }
            }
        }

        private void AddDirFolderNodes(
            FileBrowserNode parentNode,
            string folderName
            )
        {
            TreeNode[] found = parentNode.Nodes.Find(folderName, false);

            if (found == null || found.Length == 0)
            {
                FileBrowserNode node = new FileBrowserNode(folderName, Resources.Folder, typeof(FilesDetailPage), this);
                node.FBNodeType = FileBrowserNode.FileBrowserNopeType.DIRECTORY;

                if (parentNode.Path.Equals("/"))
                {
                     node.Path = parentNode.Path + folderName;
                }
                else
                {
                    node.Path = parentNode.Path + PathSeparator + folderName;
                }
                parentNode.Nodes.Add(node);
            }
        }

        private void RefreshNetworkTreeNode()
        {
            TreeNode[] networkNode = this._pluginNode.Nodes.Find(Resources.String_Network, false);

            if (networkNode != null && networkNode.Length > 0)
            {
                EnumChildren(networkNode[0] as LACTreeNode);
            }
        }

        public void EnumChildren(LACTreeNode parentNode)
        {
            List<NETRESOURCE> NetResources = new List<NETRESOURCE>();
            FileBrowserNode node = parentNode as FileBrowserNode;

            if (node == null)
            {
                return;
            }

            if (node.FBNodeType == FileBrowserNode.FileBrowserNopeType.ROOT)
            {
                // Enumerate the child nodes of "File Browser" tree root
                BuildNodesToPlugin();
                return;
            }

            if (node.Name.Equals("Network"))
            {
                // Enumerate the child nodes of the "Network" tree node
                if (platform == LikewiseTargetPlatform.Windows)
                {
                    NetResources = GetNetworkConnections();
                    AddShareNodes(parentNode, NetResources);
                }
                else
                {
                    AddShareNodes(parentNode, RemoteShares);
                }
                return;
            }

            if (node.Name.Equals("Computer"))
            {
                return;
            }

            if (node.FBNodeType == FileBrowserNode.FileBrowserNopeType.SHARE ||
                node.FBNodeType == FileBrowserNode.FileBrowserNopeType.DIRECTORY)
            {
                // Enum files and directories under share/directory path.
                List<FileItem> FileList = FileClient.FileClient.EnumFiles(node.Path, false);

                if (FileList == null || FileList.Count == 0)
                {
                    return;
                }

                foreach (FileItem File in FileList)
                {
                    if (File.IsDirectory)
                    {
                        AddDirFolderNodes(node, File.FileName);
                    }
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
            FileBrowserNode node = nodeClicked as FileBrowserNode;
            ContextMenu fileBrowserContextMenu = null;
            MenuItem menuItem = null;
            StandardPage fileBrowserPage = (StandardPage)nodeClicked.PluginPage;

            _selectedNode = _pluginNode.TreeView.SelectedNode as LACTreeNode;
            _currentNode = nodeClicked;

            if (fileBrowserPage == null)
            {
                Type type = nodeClicked.NodeType;
                object o = Activator.CreateInstance(type);
                if (o is IPlugInPage)
                {
                    ((IPlugInPage)o).SetPlugInInfo(_container, nodeClicked.Plugin, nodeClicked, (LWTreeView)nodeClicked.TreeView, nodeClicked.sc);
                }
            }

            if (node == null)
            {
                return fileBrowserContextMenu;
            }

            if (node.FBNodeType == FileBrowserNode.FileBrowserNopeType.ROOT)
            {
                fileBrowserContextMenu = new ContextMenu();

                menuItem = new MenuItem("View Details", cm_OnSetView_Detail);
                if (_currentView == View.Details)
                    menuItem.Checked = true;
                fileBrowserContextMenu.MenuItems.Add(menuItem);

                menuItem = new MenuItem("View Large Icons", cm_OnSetView_LargeIcon);
                if (_currentView == View.LargeIcon)
                    menuItem.Checked = true;
                fileBrowserContextMenu.MenuItems.Add(menuItem);

                menuItem = new MenuItem("View Small Icons", cm_OnSetView_SmallIcon);
                if (_currentView == View.SmallIcon)
                    menuItem.Checked = true;
                fileBrowserContextMenu.MenuItems.Add(menuItem);

                menuItem = new MenuItem("View List", cm_OnSetView_List);
                if (_currentView == View.List)
                    menuItem.Checked = true;
                fileBrowserContextMenu.MenuItems.Add(menuItem);

                return fileBrowserContextMenu;
            }

            fileBrowserContextMenu = new ContextMenu();

            menuItem = new MenuItem("Expand", cm_OnExpand);
            fileBrowserContextMenu.MenuItems.Add(0, menuItem);

            if (node.Name.Equals(Resources.String_Network))
            {
                menuItem = new MenuItem("Connect share", new EventHandler(cm_OnConnectToShare));
                fileBrowserContextMenu.MenuItems.Add(menuItem);
            }
            else if (node.Parent.Name.Equals(Resources.String_Network))
            {
                menuItem = new MenuItem("Disconnect share", new EventHandler(cm_OnDisconnectShare));
                fileBrowserContextMenu.MenuItems.Add(menuItem);
            }

            if (node.FBNodeType == FileBrowserNode.FileBrowserNopeType.SHARE ||
                node.FBNodeType == FileBrowserNode.FileBrowserNopeType.DIRECTORY)
            {
                menuItem = new MenuItem("Refresh", cm_OnRefresh);
                fileBrowserContextMenu.MenuItems.Add(menuItem);

                menuItem = new MenuItem("New folder", new EventHandler(cm_OnCreateFolder));
                fileBrowserContextMenu.MenuItems.Add(menuItem);

                menuItem = new MenuItem("Copy", new EventHandler(cm_OnCopy));
                fileBrowserContextMenu.MenuItems.Add(menuItem);
            }

            if (node.FBNodeType == FileBrowserNode.FileBrowserNopeType.DIRECTORY)
            {
                menuItem = new MenuItem("Move", new EventHandler(cm_OnMove));
                fileBrowserContextMenu.MenuItems.Add(menuItem);

                menuItem = new MenuItem("Delete", new EventHandler(cm_OnDelete));
                fileBrowserContextMenu.MenuItems.Add(menuItem);

                menuItem = new MenuItem("Rename", new EventHandler(cm_OnRename));
                fileBrowserContextMenu.MenuItems.Add(menuItem);
            }

            return fileBrowserContextMenu;
        }

        public void SetSingleSignOn(bool useSingleSignOn)
        {
            // do nothing
        }

        public bool PluginSelected()
        {
            return true;
        }

        #endregion

        #region Private helper functions

        private FileBrowserNode GetFileBrowserNode()
        {
            if (_pluginNode == null)
            {
                Icon ic = Resources.Library;
                _pluginNode = new FileBrowserNode("File Browser", ic, typeof(FilesBrowserPluginPage), this);
                _pluginNode.ImageIndex = (int)Manage.ManageImageType.Generic;
                _pluginNode.SelectedImageIndex = (int)Manage.ManageImageType.Generic;
                _pluginNode.FBNodeType = FileBrowserNode.FileBrowserNopeType.ROOT;

                _pluginNode.IsPluginNode = true;
            }

            return _pluginNode;
        }

        private void BuildNodesToPlugin()
        {
            if (_pluginNode != null)
            {
                Icon iconNetShare = Resources.Share;
                Icon iconFolder = Resources.Folder;
                Icon iconComputer = Resources.Computer_48;
                Icon iconHome = Resources.Home;

                if (_pluginNode.Nodes.Find(Resources.String_Network, false).Length == 0)
                {
                    FileBrowserNode networkNode = new FileBrowserNode(Resources.String_Network, iconNetShare, typeof(FilesDetailPage), this);
                    networkNode.FBNodeType = FileBrowserNode.FileBrowserNopeType.CATEGORY;
                    _pluginNode.Nodes.Add(networkNode);
                }

                if (_pluginNode.Nodes.Find(Resources.String_Computer, false).Length == 0)
                {
                    FileBrowserNode computerNode = new FileBrowserNode(Resources.String_Computer, iconComputer, typeof(FilesDetailPage), this);
                    computerNode.Path = "";
                    computerNode.FBNodeType = FileBrowserNode.FileBrowserNopeType.CATEGORY;
                    _pluginNode.Nodes.Add(computerNode);

                    FileBrowserNode localDiskRootNode = new FileBrowserNode("[HD]", iconFolder, typeof(FilesDetailPage), this);
                    localDiskRootNode.Path = "";
                    localDiskRootNode.FBNodeType = FileBrowserNode.FileBrowserNopeType.DIRECTORY;
                    computerNode.Nodes.Add(localDiskRootNode);

                    /* Disabled for now - These could be suitable common local user paths that are worth browsing.
                    if (platform != LikewiseTargetPlatform.Windows)
                    {
                        FileBrowserNode homeNode = new FileBrowserNode(Resources.String_Home, iconHome, typeof(FilesDetailPage), this);
                        computerNode.Path = "~/";
                        homeNode.FBNodeType = FileBrowserNode.FileBrowserNopeType.CATEGORY;
                        computerNode.Nodes.Add(homeNode);

                        FileBrowserNode deskNode = new FileBrowserNode(Resources.String_Desktop, iconFolder, typeof(FilesDetailPage), this);
                        deskNode.Path = "~/Desktop/";
                        deskNode.FBNodeType = FileBrowserNode.FileBrowserNopeType.CATEGORY;
                        computerNode.Nodes.Add(deskNode);

                        FileBrowserNode docNode = new FileBrowserNode(Resources.String_Documents, iconFolder, typeof(FilesDetailPage), this);
                        docNode.Path = "~/Documents/";
                        docNode.FBNodeType = FileBrowserNode.FileBrowserNopeType.CATEGORY;
                        computerNode.Nodes.Add(docNode);

                        FileBrowserNode musicNode = new FileBrowserNode(Resources.String_Music, iconFolder, typeof(FilesDetailPage), this);
                        musicNode.Path = "~/Music/";
                        musicNode.FBNodeType = FileBrowserNode.FileBrowserNopeType.CATEGORY;
                        computerNode.Nodes.Add(musicNode);

                        FileBrowserNode pictNode = new FileBrowserNode(Resources.String_Pictures, iconFolder, typeof(FilesDetailPage), this);
                        pictNode.Path = "~/Photos/";
                        pictNode.FBNodeType = FileBrowserNode.FileBrowserNopeType.CATEGORY;
                        computerNode.Nodes.Add(pictNode);

                        FileBrowserNode videoNode = new FileBrowserNode(Resources.String_Videos, iconFolder, typeof(FilesDetailPage), this);
                        videoNode.Path = "~/Movies/";
                        videoNode.FBNodeType = FileBrowserNode.FileBrowserNopeType.CATEGORY;
                        computerNode.Nodes.Add(videoNode);
                    }
                  */
                }
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
                    // Refresh RemoteShares list and exit
                    RemoteShares.Add(path);
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
                    CredentialsDialog credDialog = new CredentialsDialog(username);

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
            Application.UseWaitCursor = false;
        }

        private void cm_OnDisconnectShare(object sender, EventArgs e)
        {
            string name = null;

            if (_currentNode != null &&
                _selectedNode != null &&
                _currentNode == _selectedNode)
            {
                name = _currentNode.Name;

                WinError error = FileClient.FileClient.DeleteConnection(name);

                // Update the RemoteShares list (for systems missing WNetEnumResource API
                RemoteShares.Remove(name);

                this._pluginNode.Nodes.Remove(this._pluginNode.Nodes.Find(name, true)[0]);
                RefreshNetworkTreeNode();
            }
        }

        private void cm_OnRefresh(object sender, EventArgs e)
        {
            if (_currentNode != null &&
                _selectedNode != null &&
                _currentNode == _selectedNode)
            {
                _currentNode.Nodes.Clear();
                EnumChildren(_currentNode);
                _currentNode.Expand();
            }
        }

        private void cm_OnExpand(object sender, EventArgs e)
        {
            if (_currentNode != null &&
                _selectedNode != null &&
                _currentNode == _selectedNode)
            {
                EnumChildren(_currentNode);
                _currentNode.Expand();
            }
        }

        private void cm_OnCopy(object sender, EventArgs e)
        {
            if (_currentNode != null &&
                _selectedNode != null &&
                _currentNode == _selectedNode)
            {
                FileBrowserNode node = _currentNode as FileBrowserNode;
                FileBrowserNode parent = _currentNode.Parent as FileBrowserNode;

                // Determine destingation to copy to
                SelectDestinationDialog destinationDialog = new SelectDestinationDialog(node.Path, SelectDestinationDialog.SELECT_DESTINATION_OPERATION.COPY_DIRECTORY, this);

                if (destinationDialog.ShowDialog() == DialogResult.OK)
                {
                    WinError error = FileClient.FileClient.CopyDirectory(parent.Path,
                                                                         destinationDialog.GetPath(),
                                                                         node.Name,
                                                                         node.Name,
                                                                         true);

                    if (error == WinError.ERROR_FILE_EXISTS ||
                        error == WinError.ERROR_ALREADY_EXISTS)
                    {
                        error = FileClient.FileClient.CopyDirectory(parent.Path,
                                                                    destinationDialog.GetPath(),
                                                                    node.Name,
                                                                    "Copy of " + node.Name,
                                                                    true);
                    }

                    if (error != WinError.NO_ERROR)
                    {
                        string message = "Copy directory operation failed. Error: " + error.ToString();
                        MessageBox.Show(message, "Could not copy directory", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    }
                }
            }
        }

        private void cm_OnMove(object sender, EventArgs e)
        {
            if (_currentNode != null &&
                _selectedNode != null &&
                _currentNode == _selectedNode)
            {
                FileBrowserNode node = _currentNode as FileBrowserNode;
                FileBrowserNode parent = _currentNode.Parent as FileBrowserNode;

                // Determine destingation to copy to
                SelectDestinationDialog destinationDialog = new SelectDestinationDialog(node.Path, SelectDestinationDialog.SELECT_DESTINATION_OPERATION.MOVE_DIRECTORY, this);

                if (destinationDialog.ShowDialog() == DialogResult.OK)
                {
                    WinError error = FileClient.FileClient.MoveDirectory(parent.Path,
                                                                         destinationDialog.GetPath(),
                                                                         node.Name,
                                                                         node.Name,
                                                                         true);

                    if (error == WinError.ERROR_FILE_EXISTS ||
                        error == WinError.ERROR_ALREADY_EXISTS)
                    {
                        error = FileClient.FileClient.MoveDirectory(parent.Path,
                                                                    destinationDialog.GetPath(),
                                                                    node.Name,
                                                                    "Copy of " + node.Name,
                                                                    true);
                    }

                    if (error != WinError.NO_ERROR)
                    {
                        string message = "Move directory operation failed. Error: " + error.ToString();
                        MessageBox.Show(message, "Could not move directory", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    }
                    else
                    {
                        _currentNode.Remove();
                    }
                }
            }
        }

        private void cm_OnDelete(object sender, EventArgs e)
        {
            if (_currentNode != null &&
                _selectedNode != null &&
                _currentNode == _selectedNode)
            {
                FileBrowserNode node = _currentNode as FileBrowserNode;
                DialogResult result = MessageBox.Show("Are you sure that you want to delete the directory called: " + node.Name + "?",
                                                      "File Browser",
                                                      MessageBoxButtons.YesNo);

                if (result == DialogResult.Yes)
                {
                    WinError error = FileClient.FileClient.DeleteDirectory(node.Path);

                    if (error != WinError.NO_ERROR)
                    {
                        string message = "Delete directory operation failed. Error: " + error.ToString();
                        MessageBox.Show(message, "Could not remove directory", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    }
                    else
                    {
                        node.Remove();
                    }
                }
            }
        }

        private void cm_OnRename(object sender, EventArgs e)
        {
            if (_currentNode != null &&
                _selectedNode != null &&
                _currentNode == _selectedNode)
            {
                FileBrowserNode node = _currentNode as FileBrowserNode;
                FileBrowserNode parent = _currentNode.Parent as FileBrowserNode;
                string newName = "";

                if (node != null &&
                    parent != null)
                {
                    // Determine destingation to copy to
                    RenameDialog renameDialog = new RenameDialog(node.Name, RenameDialog.RENAME_OPERATION.RENAME_DIRECTORY, this);

                    if (renameDialog.ShowDialog() == DialogResult.OK)
                    {
                        newName = renameDialog.GetName();

                        WinError error = FileClient.FileClient.MoveDirectory(parent.Path, parent.Path, node.Name, newName, true);

                        if (error != WinError.NO_ERROR)
                        {
                            string message = "Rename directory operation failed. Error: " + error.ToString();
                            MessageBox.Show(message, "Could not rename directory", MessageBoxButtons.OK, MessageBoxIcon.Error);
                        }
                    }
                }
            }
        }

        private void cm_OnCreateFolder(object sender, EventArgs e)
        {
            if (_currentNode != null &&
                _selectedNode != null &&
                _currentNode == _selectedNode)
            {
                FileBrowserNode node = _currentNode as FileBrowserNode;
                string newName = "";

                if (node != null)
                {
                    // Determine destingation to copy to
                    RenameDialog renameDialog = new RenameDialog("New Folder", RenameDialog.RENAME_OPERATION.NEW_FOLDER_NAME, this);

                    if (renameDialog.ShowDialog() == DialogResult.OK)
                    {
                        newName = node.Path + PathSeparator + renameDialog.GetName();

                        WinError error = FileClient.FileClient.CreateDirectory(newName);

                        if (error != WinError.NO_ERROR)
                        {
                            string message = "Create directory operation failed. Error: " + error.ToString();
                            MessageBox.Show(message, "Could not create new directory", MessageBoxButtons.OK, MessageBoxIcon.Error);
                        }
                    }
                }
            }
        }

        private void cm_OnSetView_Detail(object sender, EventArgs e)
        {
            _currentView = View.Details;

            if (_selectedNode != null)
            {
                FilesDetailPage fdp = _selectedNode.PluginPage as FilesDetailPage;
                if (fdp != null)
                {
                    fdp.Refresh();
                }
            }
        }

        private void cm_OnSetView_LargeIcon(object sender, EventArgs e)
        {
            _currentView = View.LargeIcon;

            if (_selectedNode != null)
            {
                FilesDetailPage fdp = _selectedNode.PluginPage as FilesDetailPage;
                if (fdp != null)
                {
                    fdp.Refresh();
                }
            }
        }

        private void cm_OnSetView_SmallIcon(object sender, EventArgs e)
        {
            _currentView = View.SmallIcon;

            if (_selectedNode != null)
            {
                FilesDetailPage fdp = _selectedNode.PluginPage as FilesDetailPage;
                if (fdp != null)
                {
                    fdp.Refresh();
                }
            }
        }

        private void cm_OnSetView_List(object sender, EventArgs e)
        {
            _currentView = View.List;

            if (_selectedNode != null)
            {
                FilesDetailPage fdp = _selectedNode.PluginPage as FilesDetailPage;
                if (fdp != null)
                {
                    fdp.Refresh();
                }
            }
        }

        #endregion
    }
}
