using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using Likewise.LMC.FileClient;
using Likewise.LMC.Plugins.FileBrowser.Properties;
using Likewise.LMC.Plugins.FileBrowser;
using Likewise.LMC.Utilities;

namespace Likewise.LMC.Plugins.FileBrowser
{
    public partial class SelectDestinationDialog : Form
    {
        private string path = null;
        private List<string> _activeShares = new List<string>();
        private LikewiseTargetPlatform platform = Configurations.currentPlatform;
        private string _localDiskRoot = null;

        public enum SELECT_DESTINATION_OPERATION
        {
            COPY_FILE,
            COPY_DIRECTORY,
            MOVE_FILE,
            MOVE_DIRECTORY
        }

        public SelectDestinationDialog()
        {
            InitializeComponent();
        }

        public SelectDestinationDialog(
            string path,
            SELECT_DESTINATION_OPERATION op,
            FileBrowserIPlugIn fbPlugin)
            : this()
        {
            string operation = "Copy file";
            Icon ic = Resources.Library;

            _activeShares = fbPlugin.GetActiveShares();
            _localDiskRoot = fbPlugin.GetLocalDiskRoot();

            if (op == SELECT_DESTINATION_OPERATION.COPY_DIRECTORY)
                operation = "Copy directory";
            else if (op == SELECT_DESTINATION_OPERATION.MOVE_FILE)
                operation = "Move file";
            else if (op == SELECT_DESTINATION_OPERATION.MOVE_DIRECTORY)
                operation = "Move directory";

            this.tbPath.Text = path;
            this.Name = operation;

            BuildTopNodes();
            EnumChildren((FileBrowserNode)tvDestinationTree.Nodes.Find("Network", false)[0]);
        }

        public string GetPath()
        {
            return path;
        }

        private void CancelBtn_Click(object sender, EventArgs e)
        {
            this.DialogResult = DialogResult.Cancel;
            this.Close();
        }

        private void OKBtn_Click(object sender, EventArgs e)
        {
            bool okayToExit = true;

            // Copy the values from the edit field.
            this.path = tbPath.Text;

            if (string.IsNullOrEmpty(path))
            {
                okayToExit = false;
            }

            if (okayToExit == true)
            {
                this.DialogResult = DialogResult.OK;
                this.Close();
            }
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
            FileBrowserNode parentNode,
            List<NETRESOURCE> NetResources
            )
        {
            foreach (NETRESOURCE NetResource in NetResources)
            {

                TreeNode[] found = parentNode.Nodes.Find(NetResource.pRemoteName, false);

                if (found == null || found.Length == 0)
                {
                    FileBrowserNode node = new FileBrowserNode(NetResource.pRemoteName, Resources.Share, null, null);
                    node.FBNodeType = FileBrowserNode.FileBrowserNopeType.SHARE;
                    node.Path = NetResource.pRemoteName;
                    parentNode.Nodes.Add(node);
                }
            }
        }

        private void AddShareNodes(
            FileBrowserNode parentNode,
            List<string> Shares
            )
        {
            foreach (string Share in Shares)
            {

                TreeNode[] found = parentNode.Nodes.Find(Share, false);

                if (found == null || found.Length == 0)
                {
                    FileBrowserNode node = new FileBrowserNode(Share, Resources.Share, null, null);
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
                FileBrowserNode node = new FileBrowserNode(folderName, Resources.Folder, null, null);
                node.FBNodeType = FileBrowserNode.FileBrowserNopeType.DIRECTORY;
                node.Path = parentNode.Path + "\\" + folderName;
                parentNode.Nodes.Add(node);
            }
        }

        public void EnumChildren(FileBrowserNode parentNode)
        {
            List<NETRESOURCE> NetResources = new List<NETRESOURCE>();
            FileBrowserNode node = parentNode as FileBrowserNode;

            if (node == null)
            {
                return;
            }

            tbPath.Text = "";

            if (node.FBNodeType == FileBrowserNode.FileBrowserNopeType.ROOT)
            {
                // Enumerate the child nodes of "File Browser" tree root
                BuildTopNodes();
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
                    AddShareNodes(parentNode, _activeShares);
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

                // Update the text box with the current item path value
                tbPath.Text = parentNode.Path;

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

        private void BuildTopNodes()
        {
            if (tvDestinationTree != null)
            {
                Icon iconNetShare = Resources.Share;
                Icon iconFolder = Resources.Folder;
                Icon iconComputer = Resources.Computer_48;
                Icon iconHome = Resources.Home;

                if (tvDestinationTree.Nodes.Find(Resources.String_Network, false).Length == 0)
                {
                    FileBrowserNode networkNode = new FileBrowserNode(Resources.String_Network, iconNetShare, null, null);
                    networkNode.FBNodeType = FileBrowserNode.FileBrowserNopeType.CATEGORY;
                    tvDestinationTree.Nodes.Add(networkNode);
                }

                if (tvDestinationTree.Nodes.Find(Resources.String_Computer, false).Length == 0)
                {
                    FileBrowserNode computerNode = new FileBrowserNode(Resources.String_Computer, iconComputer, null, null);
                    computerNode.FBNodeType = FileBrowserNode.FileBrowserNopeType.CATEGORY;
                    tvDestinationTree.Nodes.Add(computerNode);

                    FileBrowserNode localDiskRootNode = new FileBrowserNode(_localDiskRoot, iconFolder, null, null);
                    localDiskRootNode.Path = _localDiskRoot;
                    localDiskRootNode.FBNodeType = FileBrowserNode.FileBrowserNopeType.DIRECTORY;
                    computerNode.Nodes.Add(localDiskRootNode);

                    /* Disabled for now - These could be suitable common local user paths that are worth browsing.
                    FileBrowserNode homeNode = new FileBrowserNode(Resources.String_Home, iconHome, null, null);
                    computerNode.Path = "~/";
                    homeNode.FBNodeType = FileBrowserNode.FileBrowserNopeType.CATEGORY;
                    computerNode.Nodes.Add(homeNode);

                    FileBrowserNode deskNode = new FileBrowserNode(Resources.String_Desktop, iconFolder, null, null);
                    deskNode.Path = "~/Desktop/";
                    deskNode.FBNodeType = FileBrowserNode.FileBrowserNopeType.CATEGORY;
                    computerNode.Nodes.Add(deskNode);

                    FileBrowserNode docNode = new FileBrowserNode(Resources.String_Documents, iconFolder, null, null);
                    docNode.Path = "~/Documents/";
                    docNode.FBNodeType = FileBrowserNode.FileBrowserNopeType.CATEGORY;
                    computerNode.Nodes.Add(docNode);

                    FileBrowserNode musicNode = new FileBrowserNode(Resources.String_Music, iconFolder, null, null);
                    musicNode.Path = "~/Music/";
                    musicNode.FBNodeType = FileBrowserNode.FileBrowserNopeType.CATEGORY;
                    computerNode.Nodes.Add(musicNode);

                    FileBrowserNode pictNode = new FileBrowserNode(Resources.String_Pictures, iconFolder, null, null);
                    pictNode.Path = "~/Photos/";
                    pictNode.FBNodeType = FileBrowserNode.FileBrowserNopeType.CATEGORY;
                    computerNode.Nodes.Add(pictNode);

                    FileBrowserNode videoNode = new FileBrowserNode(Resources.String_Videos, iconFolder, null, null);
                    videoNode.Path = "~/Movies/";
                    videoNode.FBNodeType = FileBrowserNode.FileBrowserNopeType.CATEGORY;
                    computerNode.Nodes.Add(videoNode);
                  */
                }
            }
        }

        private void lwDestinationTree_AfterSelect(object sender, TreeViewEventArgs e)
        {
            FileBrowserNode node = e.Node as FileBrowserNode;
            EnumChildren(node);
        }
    }
}