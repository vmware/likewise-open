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
using System.ComponentModel;
using System.Drawing;
using System.Net;
using System.Diagnostics;
using System.Windows.Forms;
using System.Collections.Generic;
using System.Threading;
using Likewise.LMC.LMConsoleUtils;
using Likewise.LMC.Properties;
using Likewise.LMC.ServerControl;
using System.IO;

namespace Likewise.LMC
{
    /// <summary>
    /// Main Likewise Management Console Form
    /// </summary>
    public class LMCMainForm : Form, ISCHost
    {
        private string sConsoleFile = null;
        private string _fileName = "LMCConsole1";
        private bool IsFileOpened = false;
        private bool IsSetingsCanSave = false;        

        #region delegates
        delegate void SetCursorCallback(System.Windows.Forms.Cursor cursor);
        #endregion

        #region Constants

        private string formId = "LMCMainForm";

        #endregion

        #region Visual Studio designer vars

        private IContainer components;
        private MainMenu mainMenu1;
        private MenuItem menuItem_About;
        private MenuItem menuItem_Close;
        private MenuItem menuItem_File;
        private MenuItem menuItem_Help;
        private MenuItem menuItem12;
        private MenuItem menuItem3;
        private Panel pivotPanel;
        private CServerControl sc;
        private StatusBar statusBar1;
        private StatusBarPanel statusBarPanel1;
        private StatusBarPanel statusBarPanel2;
        private StatusBarPanel statusBarPanel3;

        #endregion

        #region Other Windows Forms data

        // special windows message used for RPC
        // private const string sMessageCesmRPC = "Likewise.LMC.RPC";
        private ImageList imageList1;
        private ImageList imageListLarge24;
        private Panel panel1;
        private Splitter splitter1;
        private LWTreeView navTree;
        #endregion

        #region other data

        public static List<LACTreeNode> navigationHistory = new List<LACTreeNode>();
        private MenuItem addremove_plugin;
        private ImageList imageListLarge32;

        public static int navigationPosition = -1;
        private MenuItem menuItem_Open;
        private OpenFileDialog openFileDialog;
        private MenuItem menuItem_New;
        private MenuItem menuItem_SaveAs;
        private MenuItem menuItem_Save;

        #endregion

        #region Constructor

        public LMCMainForm()
        {
            //
            // Required for Windows Form Designer support
            //
            InitializeComponent();      

            Closed += new EventHandler(MainForm_Closed);

            formId += DateTime.Now.ToString("_yyyy_MM_dd_hh_mm_ss_");
        }

        public LMCMainForm(string sConsoleFile)
            : this()
        {
            this.sConsoleFile = sConsoleFile;
        }

        #endregion

        #region Public Methods

        public string getFormId()
        {
            return formId;
        }

        public CServerControl SC
        {
            get
            {
                return sc;
            }
        }

        #endregion


        #region Overridden methods

        protected override void OnLoad(EventArgs e)
        {
            base.OnLoad(e);
            sc.SetLWTreeView(navTree);
            
            sc.LoadPlugins();

            if (sConsoleFile != null && File.Exists(sConsoleFile))
            {
                // load up the console file
                ReadConsoleFile(sConsoleFile);
            }
        }

        protected override void Dispose(bool disposing)
        {
            if (disposing)
            {
                // save the ServerControl persistent state
                sc.OnClose();
                if (components != null)
                {
                    components.Dispose();
                }
            }

            base.Dispose(disposing);
        }

        #endregion

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(LMCMainForm));
            this.mainMenu1 = new System.Windows.Forms.MainMenu(this.components);
            this.menuItem_File = new System.Windows.Forms.MenuItem();
            this.menuItem_New = new System.Windows.Forms.MenuItem();
            this.menuItem_Open = new System.Windows.Forms.MenuItem();
            this.menuItem_Save = new System.Windows.Forms.MenuItem();
            this.menuItem_SaveAs = new System.Windows.Forms.MenuItem();
            this.addremove_plugin = new System.Windows.Forms.MenuItem();
            this.menuItem_Close = new System.Windows.Forms.MenuItem();
            this.menuItem_Help = new System.Windows.Forms.MenuItem();
            this.menuItem3 = new System.Windows.Forms.MenuItem();
            this.menuItem12 = new System.Windows.Forms.MenuItem();
            this.menuItem_About = new System.Windows.Forms.MenuItem();
            this.statusBar1 = new System.Windows.Forms.StatusBar();
            this.statusBarPanel1 = new System.Windows.Forms.StatusBarPanel();
            this.statusBarPanel2 = new System.Windows.Forms.StatusBarPanel();
            this.statusBarPanel3 = new System.Windows.Forms.StatusBarPanel();
            this.imageList1 = new System.Windows.Forms.ImageList(this.components);
            this.pivotPanel = new System.Windows.Forms.Panel();
            this.navTree = new Likewise.LMC.ServerControl.LWTreeView();
            this.panel1 = new System.Windows.Forms.Panel();
            this.imageListLarge24 = new System.Windows.Forms.ImageList(this.components);
            this.splitter1 = new System.Windows.Forms.Splitter();
            this.imageListLarge32 = new System.Windows.Forms.ImageList(this.components);
            this.openFileDialog = new System.Windows.Forms.OpenFileDialog();
            this.sc = new Likewise.LMC.ServerControl.CServerControl();
            ((System.ComponentModel.ISupportInitialize)(this.statusBarPanel1)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.statusBarPanel2)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.statusBarPanel3)).BeginInit();
            this.pivotPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // mainMenu1
            // 
            this.mainMenu1.MenuItems.AddRange(new System.Windows.Forms.MenuItem[] {
            this.menuItem_File,
            this.menuItem_Help});
            // 
            // menuItem_File
            // 
            this.menuItem_File.Index = 0;
            this.menuItem_File.MenuItems.AddRange(new System.Windows.Forms.MenuItem[] {
            this.menuItem_New,
            this.menuItem_Open,
            this.menuItem_Save,
            this.menuItem_SaveAs,
            this.addremove_plugin,
            this.menuItem_Close});
            this.menuItem_File.Text = "&File";
            // 
            // menuItem_New
            // 
            this.menuItem_New.Index = 0;
            this.menuItem_New.Shortcut = System.Windows.Forms.Shortcut.CtrlN;
            this.menuItem_New.Text = "&New";
            this.menuItem_New.Click += new System.EventHandler(this.menuItem_New_Click);
            // 
            // menuItem_Open
            // 
            this.menuItem_Open.Index = 1;
            this.menuItem_Open.Shortcut = System.Windows.Forms.Shortcut.CtrlO;
            this.menuItem_Open.Text = "&Open...";
            this.menuItem_Open.Click += new System.EventHandler(this.menuItem_Open_Click);
            // 
            // menuItem_Save
            // 
            this.menuItem_Save.Index = 2;
            this.menuItem_Save.Shortcut = System.Windows.Forms.Shortcut.CtrlS;
            this.menuItem_Save.Text = "&Save";
            this.menuItem_Save.Click += new System.EventHandler(this.menuItem_Save_Click);
            // 
            // menuItem_SaveAs
            // 
            this.menuItem_SaveAs.Index = 3;
            this.menuItem_SaveAs.Text = "Save &As...";
            this.menuItem_SaveAs.Click += new System.EventHandler(this.menuItem_SaveAs_Click);
            // 
            // addremove_plugin
            // 
            this.addremove_plugin.Index = 4;
            this.addremove_plugin.Text = "Add/Re&move Plug-in...";
            this.addremove_plugin.Click += new System.EventHandler(this.Menu_addremoveplugin_clicked);
            // 
            // menuItem_Close
            // 
            this.menuItem_Close.Index = 5;
            this.menuItem_Close.Text = "E&xit";
            this.menuItem_Close.Click += new System.EventHandler(this.menuItem_Exit_Click);
            // 
            // menuItem_Help
            // 
            this.menuItem_Help.Index = 1;
            this.menuItem_Help.MenuItems.AddRange(new System.Windows.Forms.MenuItem[] {
            this.menuItem3,
            this.menuItem12,
            this.menuItem_About});
            this.menuItem_Help.Text = "&Help";
            // 
            // menuItem3
            // 
            this.menuItem3.Index = 0;
            this.menuItem3.Text = "&Technical Support";
            this.menuItem3.Click += new System.EventHandler(this.menuItem3_Click);
            // 
            // menuItem12
            // 
            this.menuItem12.Index = 1;
            this.menuItem12.Text = "-";
            // 
            // menuItem_About
            // 
            this.menuItem_About.Index = 2;
            this.menuItem_About.Text = "&About Likewise Management Console";
            this.menuItem_About.Click += new System.EventHandler(this.menuItem_About_Click);
            // 
            // statusBar1
            // 
            this.statusBar1.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.statusBar1.Location = new System.Drawing.Point(0, 539);
            this.statusBar1.Name = "statusBar1";
            this.statusBar1.Panels.AddRange(new System.Windows.Forms.StatusBarPanel[] {
            this.statusBarPanel1,
            this.statusBarPanel2,
            this.statusBarPanel3});
            this.statusBar1.ShowPanels = true;
            this.statusBar1.Size = new System.Drawing.Size(792, 23);
            this.statusBar1.TabIndex = 0;
            // 
            // statusBarPanel1
            // 
            this.statusBarPanel1.AutoSize = System.Windows.Forms.StatusBarPanelAutoSize.Spring;
            this.statusBarPanel1.Name = "statusBarPanel1";
            this.statusBarPanel1.Width = 525;
            // 
            // statusBarPanel2
            // 
            this.statusBarPanel2.BorderStyle = System.Windows.Forms.StatusBarPanelBorderStyle.None;
            this.statusBarPanel2.Name = "statusBarPanel2";
            this.statusBarPanel2.Width = 240;
            // 
            // statusBarPanel3
            // 
            this.statusBarPanel3.BorderStyle = System.Windows.Forms.StatusBarPanelBorderStyle.None;
            this.statusBarPanel3.Name = "statusBarPanel3";
            this.statusBarPanel3.Width = 10;
            // 
            // imageList1
            // 
            this.imageList1.ImageStream = ((System.Windows.Forms.ImageListStreamer)(resources.GetObject("imageList1.ImageStream")));
            this.imageList1.TransparentColor = System.Drawing.Color.Transparent;
            this.imageList1.Images.SetKeyName(0, "");
            this.imageList1.Images.SetKeyName(1, "");
            this.imageList1.Images.SetKeyName(2, "");
            this.imageList1.Images.SetKeyName(3, "");
            this.imageList1.Images.SetKeyName(4, "");
            this.imageList1.Images.SetKeyName(5, "");
            this.imageList1.Images.SetKeyName(6, "");
            this.imageList1.Images.SetKeyName(7, "back.bmp");
            this.imageList1.Images.SetKeyName(8, "");
            this.imageList1.Images.SetKeyName(9, "");
            this.imageList1.Images.SetKeyName(10, "");
            this.imageList1.Images.SetKeyName(11, "");
            this.imageList1.Images.SetKeyName(12, "");
            this.imageList1.Images.SetKeyName(13, "");
            this.imageList1.Images.SetKeyName(14, "");
            this.imageList1.Images.SetKeyName(15, "BlockedOrganizationalUnit.ico");
            // 
            // pivotPanel
            // 
            this.pivotPanel.AutoScroll = true;
            this.pivotPanel.Controls.Add(this.navTree);
            this.pivotPanel.Controls.Add(this.panel1);
            this.pivotPanel.Dock = System.Windows.Forms.DockStyle.Left;
            this.pivotPanel.Location = new System.Drawing.Point(0, 0);
            this.pivotPanel.Name = "pivotPanel";
            this.pivotPanel.Size = new System.Drawing.Size(231, 539);
            this.pivotPanel.TabIndex = 0;
            // 
            // navTree
            // 
            this.navTree.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.navTree.Dock = System.Windows.Forms.DockStyle.Fill;
            this.navTree.HideSelection = false;
            this.navTree.Location = new System.Drawing.Point(0, 16);
            this.navTree.Name = "navTree";
            this.navTree.Size = new System.Drawing.Size(231, 523);
            this.navTree.TabIndex = 0;
            this.navTree.AfterCollapse += new System.Windows.Forms.TreeViewEventHandler(this.treeView1_AfterCollapse);
            this.navTree.AfterSelect += new System.Windows.Forms.TreeViewEventHandler(this.treeView1_AfterSelect);
            this.navTree.NodeMouseClick += new System.Windows.Forms.TreeNodeMouseClickEventHandler(this.treeView1_NodeMouseClick);
            this.navTree.AfterExpand += new System.Windows.Forms.TreeViewEventHandler(this.treeView1_AfterExpand);
            // 
            // panel1
            // 
            this.panel1.BackColor = System.Drawing.SystemColors.Window;
            this.panel1.Dock = System.Windows.Forms.DockStyle.Top;
            this.panel1.Location = new System.Drawing.Point(0, 0);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(231, 16);
            this.panel1.TabIndex = 1;
            // 
            // imageListLarge24
            // 
            this.imageListLarge24.ImageStream = ((System.Windows.Forms.ImageListStreamer)(resources.GetObject("imageListLarge24.ImageStream")));
            this.imageListLarge24.TransparentColor = System.Drawing.SystemColors.MenuBar;
            this.imageListLarge24.Images.SetKeyName(0, "back-lgr.ico");
            this.imageListLarge24.Images.SetKeyName(1, "next-lgr.ico");
            this.imageListLarge24.Images.SetKeyName(2, "reload-lgr.ico");
            // 
            // splitter1
            // 
            this.splitter1.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.splitter1.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.splitter1.Location = new System.Drawing.Point(231, 0);
            this.splitter1.Name = "splitter1";
            this.splitter1.Size = new System.Drawing.Size(8, 539);
            this.splitter1.TabIndex = 2;
            this.splitter1.TabStop = false;
            this.splitter1.SplitterMoved += new System.Windows.Forms.SplitterEventHandler(this.splitter1_SplitterMoved);
            // 
            // imageListLarge32
            // 
            this.imageListLarge32.ImageStream = ((System.Windows.Forms.ImageListStreamer)(resources.GetObject("imageListLarge32.ImageStream")));
            this.imageListLarge32.TransparentColor = System.Drawing.SystemColors.MenuBar;
            this.imageListLarge32.Images.SetKeyName(0, "back-lgr.ico");
            this.imageListLarge32.Images.SetKeyName(1, "next-lgr.ico");
            this.imageListLarge32.Images.SetKeyName(2, "reload-lgr.ico");
            // 
            // openFileDialog
            // 
            this.openFileDialog.DefaultExt = "lmc";
            this.openFileDialog.FileName = "openFileDialog";
            this.openFileDialog.Filter = "Likewise Management Cosole(*.lmc)|*.lmc";
            this.openFileDialog.ReadOnlyChecked = true;
            this.openFileDialog.ShowReadOnly = true;
            // 
            // sc
            // 
            this.sc.ApplicationDirectory = "C:\\Program Files\\Likewise\\Enterprise";
            this.sc.AutoScroll = true;
            this.sc.BackColor = System.Drawing.SystemColors.Window;
            this.sc.BackgroundImageLayout = System.Windows.Forms.ImageLayout.None;
            this.sc.Dock = System.Windows.Forms.DockStyle.Fill;
            this.sc.ForeColor = System.Drawing.SystemColors.WindowText;
            this.sc.Location = new System.Drawing.Point(239, 0);
            this.sc.Margin = new System.Windows.Forms.Padding(2);
            this.sc.Name = "sc";
            this.sc.Size = new System.Drawing.Size(553, 539);
            this.sc.TabIndex = 1;
            // 
            // LMCMainForm
            // 
            this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
            this.BackColor = System.Drawing.SystemColors.MenuBar;
            this.ClientSize = new System.Drawing.Size(792, 562);
            this.Controls.Add(this.sc);
            this.Controls.Add(this.splitter1);
            this.Controls.Add(this.pivotPanel);
            this.Controls.Add(this.statusBar1);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Menu = this.mainMenu1;
            this.MinimumSize = new System.Drawing.Size(800, 600);
            this.Name = "LMCMainForm";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Likewise Management Console";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.LMCMainForm_FormClosing);
            ((System.ComponentModel.ISupportInitialize)(this.statusBarPanel1)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.statusBarPanel2)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.statusBarPanel3)).EndInit();
            this.pivotPanel.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        #region Helper functions  

        private void MainForm_Closed(object sender, EventArgs e)
        {
            //Hide();
            //Application.Exit();
        }
         
        private void handleRefresh()
        {
            if (sc.manage.NodeShown != null)
            {
                sc.manage.Refresh();                
            }            
        }

        private string GetDefaultConsoleDirectory()
        {
            string initialDir = string.Empty;

            try
            {
                initialDir = Path.Combine(Configurations.tempDirectory, "Likewise");

                if (!Directory.Exists(initialDir))
                {
                    Directory.CreateDirectory(initialDir);
                    initialDir = Path.Combine(initialDir, "Console Settings");
                    Directory.CreateDirectory(initialDir);
                }
                else
                    initialDir = Path.Combine(initialDir, "Console Settings");
            }
            catch (Exception ex)
            {
                Logger.LogException("LMCMainForm.GetDefaultConsoleDirectory()", ex);
            }

            return initialDir;
        }

        private bool OpenSaveDialogToSaveSettings()
        {
            try
            {
                if (IsSetingsCanSave)
                {
                    SaveFileDialog savefileDailog = new SaveFileDialog();
                    savefileDailog.AddExtension = true;
                    savefileDailog.InitialDirectory = GetDefaultConsoleDirectory();
                    savefileDailog.Filter = @"Likewise Administrative Cosole(*.lmc)|*.lmc";
                    savefileDailog.OverwritePrompt = true;
                    savefileDailog.ValidateNames = true;
                    savefileDailog.FileName = _fileName;
                    if (savefileDailog.ShowDialog(this) == DialogResult.OK)
                    {
                        _fileName = savefileDailog.FileName;
                        string fullPath = Path.GetFullPath(_fileName);

                        FrameState fs = GetFrameState();
                        string tempFile = sc.manage.SaveConsoleSettingsToXml(fs);

                        if (File.Exists(fullPath))
                            File.Delete(fullPath);
                        File.Move(tempFile, fullPath);

                        IsSetingsCanSave = false;
                    }
                }
            }
            catch (Exception ex)
            {
                IsSetingsCanSave = true;
                Logger.LogException("LMCMainForm.OpenSaveDialogToSaveSettings()", ex);
            }

            return true;
        }

        private string FormatFilePath(string filePath)
        {
            if (string.IsNullOrEmpty(filePath))
                return filePath;

            try
            {
                FileInfo fileInfo = new FileInfo(filePath);
                if (fileInfo != null)
                {
                    return fileInfo.Name;
                }
            }
            catch (Exception ex)
            {
                Logger.LogException("LMCMainForm.FormatFilePath()", ex);
            }

            return filePath;
        }

        #endregion

        #region ISCHost Members

        //Using a worker thread is required for thread safety
        private void SetCursorUnsafe(System.Windows.Forms.Cursor cursor)
        {
            if (cursor == null)
            {
                Logger.Log(
                    "LMCMainForm.SetCursorWorker called cursor=null",
                    Logger.manageLogLevel);
                return;
            }

            if (cursor == Cursors.Default)
            {
                Logger.Log("LMCMainForm.SetCursor: DEFAULT", Logger.manageLogLevel);
            }
            else if (cursor == Cursors.WaitCursor)
            {
                Logger.Log("LMCMainForm.SetCursor: WAIT", Logger.manageLogLevel);
            }
            else
            {
                Logger.Log("LMCMainForm.SetCursor: OTHER", Logger.manageLogLevel);
            }

            this.Cursor = cursor;
        }

        /// <summary>
        /// Calls the parent (via interface, if implemented) to change the cursor used
        /// </summary>
        /// <param name="cursor">Cursor to use</param>
        public void SetCursor(System.Windows.Forms.Cursor cursor)
        {
            SetCursorCallback d = new SetCursorCallback(SetCursorUnsafe);
            this.Invoke(d, new object[] { cursor });

        }


        #endregion

        #region Event handlers

        private void menuItem_About_Click(object sender, EventArgs e)
        {
            AboutForm af = new AboutForm();
            af.ShowDialog(this);
        }

        private void menuItem_Exit_Click(object sender, EventArgs e)
        {
            Close();
        }

        private void Menu_addremoveplugin_clicked(object sender, EventArgs e)
        {
            LACTreeNode nodetoDisplay = sc.ControlManage.lmcLWTreeView.SelectedNode as LACTreeNode;
            AddRemovePluginDlg f = new AddRemovePluginDlg(sc.ControlManage, sc.ControlManage.rootNode, sc.ControlManage.lmcLWTreeView);

            f.StartPosition = FormStartPosition.CenterParent;

            DialogResult dlg = f.ShowDialog(this);

            if (dlg == DialogResult.OK)
            {
                IsSetingsCanSave = true;

                if (sc.ControlManage.lmcLWTreeView.Nodes[0].Nodes.Contains(nodetoDisplay))
                {
                    sc.ControlManage.lmcLWTreeView.SelectedNode = nodetoDisplay;
                }
                else
                {
                    sc.ControlManage.lmcLWTreeView.SelectedNode = sc.ControlManage.lmcLWTreeView.Nodes[0];
                }
                sc.ControlManage.lmcLWTreeView.Select();

                ShowControl(sc.ControlManage.lmcLWTreeView.SelectedNode as LACTreeNode);
            }
        }

        public void ShowHelp()
        {
            ProcessStartInfo psi = new ProcessStartInfo();
            psi.UseShellExecute = true;
            psi.FileName = "http://www.likewisesoftware.com/support";
            psi.Verb = "open";
            psi.WindowStyle = ProcessWindowStyle.Normal;
            Process.Start(psi);
        }


        private void menuItem3_Click(object sender, EventArgs e)
        {
            ShowHelp();
        }

        public void ShowControl(LACTreeNode node)
        {
            sc.ShowControl(node);
        }

        private void ShowNode(LACTreeNode node)
        {
            if (node != null)
            {
                ShowControl(node);
            }
        }

        private void treeView1_NodeMouseClick(object sender, TreeNodeMouseClickEventArgs e)
        {
            LACTreeNode node = (LACTreeNode)e.Node;

            if (sender != null && node != null)
            {
                if (e.Button == System.Windows.Forms.MouseButtons.Right)
                {
                    Logger.Log("treeView1_NodeMouseClick: Right Click", Logger.manageLogLevel);

                    // first, see if the page implements a context menu
                    ContextMenu cm = null;
                    if (node.PluginPage != null)
                        cm = node.PluginPage.GetPlugInContextMenu();

                    if (cm==null && node.Plugin != null)
                        cm = node.Plugin.GetTreeContextMenu(node);

                    if (cm != null && node.TreeView != null)
                    {
                        cm.Show(node.TreeView, new Point(e.X, e.Y));
                    }
                }
                else
                {
                    if (node.TreeView.SelectedNode != node)
                    {
                        node.TreeView.SelectedNode = node;
                        node.TreeView.Select();

                        ShowNode(node);

                        if (node != null && node.TreeView != null)
                        {
                            node.TreeView.SelectedNode = node;
                            node.TreeView.Select();
                        }
                    }

                    Logger.Log(
                        "treeView1_NodeMouseClick: Left Click",
                        Logger.manageLogLevel);

                }

            }
        }

        private void treeView1_AfterCollapse(object sender, TreeViewEventArgs e)
        {
            LACTreeNode node = (LACTreeNode)e.Node;

            if (sender != null && node != null)
            {
                if (node.ImageIndex == (int)Manage.ManageImageType.ContainerOpen)
                {
                    node.ImageIndex = (int)Manage.ManageImageType.Container;
                    node.SelectedImageIndex = (int)Manage.ManageImageType.Container;
                }
            }

        }

        private void treeView1_AfterExpand(object sender, TreeViewEventArgs e)
        {
            LACTreeNode node = (LACTreeNode)e.Node;

            if (sender != null && node != null)
            {
                if (node.ImageIndex == (int)Manage.ManageImageType.Container)
                {
                    node.ImageIndex = (int)Manage.ManageImageType.ContainerOpen;
                    node.SelectedImageIndex = (int)Manage.ManageImageType.ContainerOpen;
                }

            }
        }

        private void treeView1_AfterSelect(object sender, TreeViewEventArgs e)
        {
            LACTreeNode node = (LACTreeNode)e.Node;

            int lastIndex = navigationHistory.Count - 1;

            if (sender != null && node != null)
            {

                Logger.Log(String.Format(
                    "treeView1_AfterSelect({0})", 
                    node.Name),
                    Logger.manageLogLevel);
            }
        }       

        private void splitter1_SplitterMoved(object sender, SplitterEventArgs e)
        {
            //Refresh();
        }

        private void LMCMainForm_FormClosing(object sender, FormClosingEventArgs e)
        {
            if (sc.manage.rootNode.Nodes.Count == 0)
                return;

            e.Cancel = true;

            FormCollection openForms = Application.OpenForms;

            if (openForms != null && openForms.Count != 1)
            {
                string sMsg = "You must close all dialog boxes before you can close the Administrative Console.";
                MessageBox.Show(this, sMsg, CommonResources.GetString("Caption_Console"), MessageBoxButtons.OK, MessageBoxIcon.Error);

                foreach (Form f in openForms)
                {
                    f.BringToFront();
                }

                return;
            }

            try
            {
                if (IsSetingsCanSave)
                {
                    DialogResult dlg = MessageBox.Show(this, string.Format(Resources.SaveSettingsMessage, FormatFilePath(_fileName)), "Likewise Management Console", MessageBoxButtons.YesNoCancel, MessageBoxIcon.Exclamation);
                    if (dlg == DialogResult.Yes)
                    {
                        e.Cancel = true;

                        if (OpenSaveDialogToSaveSettings())
                        {
                            e.Cancel = false;

                            this.FormClosing -= new System.Windows.Forms.FormClosingEventHandler(this.LMCMainForm_FormClosing);
                            Application.Exit();
                        }
                    }
                    else if (dlg == DialogResult.No)
                    {
                        this.FormClosing -= new System.Windows.Forms.FormClosingEventHandler(this.LMCMainForm_FormClosing);
                        Close();
                        Application.Exit();
                    }
                    else
                    {
                        e.Cancel = true;
                    }
                }
                else
                {
                    this.FormClosing -= new System.Windows.Forms.FormClosingEventHandler(this.LMCMainForm_FormClosing);                    
                    Application.Exit();
                }
            }
            catch (Exception ex)
            {
                Logger.LogException("LMCMainForm.LMCMainForm_FormClosing()", ex);
            }
        }

        private void menuItem_Open_Click(object sender, EventArgs e)
        {
            string file = string.Empty;

            try
            {
                string initialDir = GetDefaultConsoleDirectory();
                openFileDialog.InitialDirectory = initialDir;
                openFileDialog.FileName = "";

                if (openFileDialog.ShowDialog(this) == DialogResult.OK)
                {
                    if (sc.manage.rootNode.Nodes.Count != 0)
                    {
                        DialogResult dlg = MessageBox.Show(this, string.Format(Resources.SaveSettingsMessage, FormatFilePath(_fileName)), "Likewise Management Console", MessageBoxButtons.YesNoCancel, MessageBoxIcon.Exclamation);
                        if (dlg == DialogResult.Yes)
                        {
                            OpenSaveDialogToSaveSettings();

                            //file = Path.Combine(initialDir, _fileName);
                            //string tempFile = sc.manage.SaveConsoleSettingsToXml(true, this.MaximumSize, this.MinimumSize);

                            //if (File.Exists(file))
                            //    File.Delete(file);
                            //File.Move(tempFile, file);
                        }
                        else if (dlg == DialogResult.Cancel)
                            return;

                        sc.manage.rootNode.Nodes.Clear();
                    }

                    _fileName = openFileDialog.FileName;
                    _fileName = Path.GetFullPath(_fileName);
                    if (File.Exists(_fileName))
                        ReadConsoleFile(_fileName);
                }
            }
            catch (Exception ex)
            {
                Logger.LogException("LMCMainForm.menuItem_Open_Click ()", ex);
            }
        }

        private void ReadConsoleFile(string sFileName)
        {
            FileInfo fileInfo = new FileInfo(sFileName);
            if (fileInfo != null && fileInfo.Extension.Trim().Equals(".lmc"))
            {
                FrameState fs = new FrameState();
                sc.manage.ReadPluginNodeInfoFromConsoleSettings(sFileName, fs);

                // reestablish frame state
                if (fs.bShowMaximized)
                    this.WindowState = FormWindowState.Maximized;
                statusBar1.Visible = fs.bShowStatusBar;
                this.pivotPanel.Visible = this.splitter1.Visible = fs.bShowTreeControl;
                this.menuItem_Save.Enabled = fs.bReadOnly ? false : true;

                if (navTree.SelectedNode.Nodes.Count == 0)
                {
                    navTree.SelectedNode.Expand();
                    sc.ShowControl(navTree.SelectedNode as LACTreeNode);
                }
                else
                {
                    LACTreeNode node = navTree.SelectedNode.Nodes[0] as LACTreeNode;
                    sc.ShowControl(node);
                    node.Expand();
                    navTree.SelectedNode = node;
                }
                navTree.Select();                
                IsFileOpened = true;
            }
        }

        private void menuItem_New_Click(object sender, EventArgs e)
        {
            try
            {
                if (sc.manage.rootNode.Nodes.Count != 0)
                {
                    if (IsSetingsCanSave)
                    {
                        DialogResult dlg = MessageBox.Show(this, string.Format(Resources.SaveSettingsMessage, FormatFilePath(_fileName)), "Likewise Management Console", MessageBoxButtons.YesNoCancel, MessageBoxIcon.Exclamation);
                        if (dlg == DialogResult.Yes)
                        {
                            if (!IsFileOpened)
                            {
                                OpenSaveDialogToSaveSettings();
                            }
                            else
                            {
                                _fileName = Path.Combine(GetDefaultConsoleDirectory(), _fileName);
                                FrameState fs = GetFrameState();
                                string tempFile = sc.manage.SaveConsoleSettingsToXml(fs);

                                if (File.Exists(_fileName))
                                    File.Delete(_fileName);
                                File.Move(tempFile, _fileName);
                            }
                        }
                        else if (dlg == DialogResult.Cancel)
                            return;
                    }
                }
                sc.manage.rootNode.Nodes.Clear();

                ShowControl(sc.manage.rootNode);
            }
            catch (Exception ex)
            {
                Logger.LogException("LMCMainForm.menuItem_New_Click ()", ex);
            }
        }

        private void menuItem_SaveAs_Click(object sender, EventArgs e)
        {
            IsSetingsCanSave = true;
            OpenSaveDialogToSaveSettings();
        }         

        private void menuItem_Save_Click(object sender, EventArgs e)
        {
            try
            {
                if (sc.manage.rootNode.Nodes.Count == 0)
                    return; 

                if (!IsFileOpened)
                {
                    OpenSaveDialogToSaveSettings();
                }
                else
                {
                    IsSetingsCanSave = false;      
                    
                    _fileName = Path.Combine(GetDefaultConsoleDirectory(), _fileName);
                    FrameState fs = GetFrameState();
                    string tempFile = sc.manage.SaveConsoleSettingsToXml(fs);

                    if (File.Exists(_fileName))
                        File.Delete(_fileName);
                    File.Move(tempFile, _fileName);
                    
                }
            }
            catch (Exception ex)
            {
                Logger.LogException("LMCMainForm.menuItem_Save_Click ()", ex);
            }
        }

        #endregion

        private FrameState GetFrameState()
        {
            FrameState fs = new FrameState();

            fs.bShowMaximized = this.WindowState == FormWindowState.Maximized;
            fs.bShowStatusBar = statusBar1.Visible;
            fs.bShowTreeControl = navTree.Visible;
            fs.bReadOnly = this.menuItem_Save.Enabled ? false : true;
            fs.left = this.Location.X;
            fs.top = this.Location.Y;
            fs.bottom = fs.top + this.Size.Height - 1;
            fs.right = fs.left + this.Size.Width - 1;

            return fs;
        }
    }
}