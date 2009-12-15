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

#define WAITSCREEN
// #define SHOWCREATEANDCLOSE

using System;
using System.Collections;
using System.ComponentModel;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Reflection;
using System.Windows.Forms;
using Likewise.LMC.Utilities;
using Likewise.LMC.ServerControl.Properties;
using Likewise.LMC.Netlogon;
using Microsoft.Win32;
using System.Net;
using System.Net.Sockets;
using System.Xml;
using System.Collections.Generic;
#if !QUARTZ
using Centeris.Likewise.Auth;
#endif
using System.Text;

namespace Likewise.LMC.ServerControl
{

    /// <summary>
    /// This class handles the bulk of the UI composition when
    /// ServerControl has been told to manage a particular host.
    /// It iterates through the various IPlugIns and asks them to
    /// manifest their UI artifacts (typically, Tabs for the
    /// navigation control and Role objects). When tabs are
    /// selected, it instantiates and activates the necessary
    /// visual elements.
    ///
    /// The class is also the "container" backreference for all
    /// IPlugIn's and IPlugInPages. Through its implementation of
    /// the IPlugInContainer interface, it provides services to
    /// all IPlugIns (i.e. the methods to add Tabs and Roles).
    /// </summary>
    public partial class Manage : UserControl, IPlugInContainer
    {

        #region Constants

        private const int ERROR_ACCESS_DENIED = 5;

#if !QUARTZ
        private const string sRootPluginName = @"Likewise.LMC.Plugins.RootPlugin.dll";
#else
        private const string sRootPluginName = @"Likewise.LMC.Plugins.RootPlugin.unix.dll";
#endif
        private const string sKey = @"SOFTWARE\Likewise\Enterprise";

#if !QUARTZ
        private bool _bShowActionPane = true;
        private string rootDomain = null;
#else
        private bool _bShowActionPane = false;
#endif
        private LACTreeNode _rootNode = null;

        private LACTreeNode currentShownNode = null;
        public static bool bSSOFailed = false;

        public LACTreeNode rootNode
        {
            get
            {
                return _rootNode;
            }
        }

        public LACTreeNode NodeShown
        {
            get
            {
                return currentShownNode;
            }
        }

        //credentails ini file path
        private string sCredentialsFilePath = null;

#if !QUARTZ
        private NetworkCredentialCache credCache = new NetworkCredentialCache();
#endif

        #endregion

        #region Private classes

        // private class used to communicated with background worker thread
        private class WorkerInfo
        {
            private Hostinfo hi;
            private bool bSuccess = false;
            private Exception ex = null;
            public WorkerInfo(Hostinfo hi)
            {
                this.hi = hi;
            }

            public Hostinfo Machine
            {
                get
                {
                    return hi;
                }
            }

            public bool Succeeded
            {
                get
                {
                    return bSuccess;
                }
                set
                {
                    bSuccess = value;
                }
            }

            public Exception ErrorException
            {
                get
                {
                    return ex;
                }
                set
                {
                    ex = value;
                }
            }

        }

        #endregion

        #region Class Data

        public enum ManageImageType
        {
            Computer = 0,   //computer.ico
            Group = 1,      //Group_16.ico
            User = 2,       //User_16.ico
            Container = 3, //Folder.ico
            OrganizationalUnit = 4,//OrganizationalUnit.ico
            GPO = 5,//GPO.ico
            Generic = 6,     //ADUC.ico
            DisabledUser = 7,//DisabledUser.ico
            DisabledComputer = 8,//DisabledUser.ico
            ContainerOpen = 9,     //FolderOpen.ico
            EventLog = 10,  //EventViewer_48.ico
            CellManager = 11,   //Cell_48.ico
            LikewiseIconsole = 12, //identity.ico
            BlockedOrganizationalUnit = 13
        }

        // some convenient colors. These are defaults that map
        // to Likewise 1.0 console colors.
        private Color cgradDarkTop = Color.FromArgb(132, 162, 195);
        private Color cgradDarkBot = Color.FromArgb(10, 42, 67);
        private Color cgradLightTop = Color.FromArgb(197, 214, 230);
        private Color cgradLightBot = Color.FromArgb(95, 134, 178);
        private Color cgradOrangeTop = Color.FromArgb(254, 156, 64);
        private Color cgradOrangeBot = Color.FromArgb(179, 36, 0);

        private Color cUnselText = Color.FromArgb(136, 142, 146);

        // back reference
        private CServerControl sc = null;

        // currently visible control
        private Control controlVisible = null;

        // current hostname
        private Hostinfo _hn;

        // well known objects
        private Hashtable htWKOs = new Hashtable();

        // state data
        private Hashtable htState = new Hashtable();

        private LACTreeNode _IPlugInRootNode = null;

        private static LWTreeView _LWTreeView = null;

        private static ImageList manageStaticImageList = null;  //16x16
        private static ImageList manageStaticLargeImageList = null; //48x48

        private StandardPage.ViewStyle currentViewStyle = StandardPage.ViewStyle.DETAIL_VIEW;

        private IPlugIn getTargetMachineRequestor;

        private string sLDAPPath;

        public LWTreeView lmcLWTreeView
        {
            get
            {
                return _LWTreeView;
            }
        }

        #endregion

        #region Constructor

        public Manage()
        {
            InitializeComponent();

        }

        public Manage(CServerControl sc) : this()
        {
            Logger.Log("Manage.Manage", Logger.manageLogLevel);

            sCredentialsFilePath = Path.Combine(Application.UserAppDataPath, @"LMCCredentials.ini");

            manageStaticImageList = this.manageImageList;
            manageStaticLargeImageList = this.manageLargeImageList;

            this.sc = sc;
        }

        #endregion

        #region Public Methods (typically, for ServerControl)

        /// <summary>
        /// Called when picker or some other interface (Welcome
        /// form, e.g.) indicates that the user wants to manage
        /// a particular machine.
        /// </summary>
        /// <param name="hn">A Hostinfo object that identifies the
        ///                  machine to be managed.</param>
        /// <returns>False if user cancelled out of creds form</returns>
        public bool ManageHost(Hostinfo hn)
        {
            Logger.Log(String.Format("Manage.ManageHost: hn: {0}", hn == null ? "null" : hn.hostName), Logger.manageLogLevel);

            string saveLdapPath = this.sLDAPPath;

#if !QUARTZ
            this.sLDAPPath = Util.GetLdapDomainPath(null, null);

            try
            {
                string server, protocol, dc, cn;
                String rootDomainPath = Util.GetLdapRootDomainPath(GetDomainName(), GetCredentials());
                Util.CrackPath(rootDomainPath, out protocol, out server, out cn, out dc);
                this.rootDomain = Util.DNToDomainName(dc).ToLower();
            }
            catch (Exception e)
            {
                this.sLDAPPath = saveLdapPath;
                throw e;
            }
#endif

            if (hn == null)
            {
                Logger.LogMsgBox("Manage::ManageHost(hn): hn == null");
                return false;
            }

            _hn = hn;

            LACTreeNode rootNode = LoadPlugins();

            foreach (LACTreeNode treeNode in rootNode.Nodes)
            {
                treeNode.Expand();
            }

#if !QUARTZ
            sLDAPPath = Util.GetLdapDomainPath(_hn.domainName, null);
#else
            sLDAPPath = _hn.domainName;
#endif
            return true;
        }

        public void LoadRootPlugin(LACTreeNode consoleRootPlugin)
        {

            if (_LWTreeView == null)
            {
                return;
            }

            if (consoleRootPlugin == null)
            {
                return;
            }

            _LWTreeView.Nodes.Clear();
            _LWTreeView.ImageList = manageImageList;
            _LWTreeView.ItemHeight = 16;

            _rootNode = consoleRootPlugin;

            if (rootNode == null)
            {
                Logger.LogMsgBox("Manage::LoadRootPlugin: rootNode == null");
                throw new Exception("Unable to load the root plugin");
            }

            _LWTreeView.Nodes.Add(rootNode);
            _LWTreeView.SelectedNode = rootNode;

            rootNode.Expand();

            foreach (LACTreeNode treeNode in rootNode.Nodes)
            {
                treeNode.ImageIndex = treeNode.SelectedImageIndex = (int)ManageImageType.Generic;
                foreach (LACTreeNode childNode in treeNode.Nodes)
                {
                    if (childNode.ImageIndex < 0 || (childNode.ImageIndex >= ((int)ManageImageType.Container)))
                    {
                        childNode.ImageIndex = childNode.SelectedImageIndex = (int)ManageImageType.Container;
                    }
                }
            }

            ShowControl(rootNode);
        }

        public LACTreeNode LoadPlugins()
        {
            // now iterate through all the IPlugIns allowing them to instantiate their UI. Sort
            // the IPlugIn names and do this in alfa order
            LACTreeNode rootNode = GetRootNode();

            LoadRootPlugin(rootNode);

            return rootNode;

        }

        /// <summary>
        /// Forces the current control to repaint itself
        /// </summary>
        public override void Refresh()
        {
            Logger.Log("Manage.Refresh", Logger.manageLogLevel);

            if (currentShownNode != null)
            {
                currentShownNode.IsModified = true;
                ShowControl(currentShownNode);
            }
            else
                base.Refresh();
        }

        public void SetViewStyle(StandardPage.ViewStyle view)
        {
            Logger.Log("Manage.SetViewStyle", Logger.manageLogLevel);

            if (currentViewStyle != view)
            {
                currentViewStyle = view;
                if (controlVisible is StandardPage)
                {
                    ((StandardPage)controlVisible).SetViewStyle(view);
                }
            }
        }

        /// <summary>
        /// Called by the framework when the MainForm is closing.
        /// </summary>
        public void OnClose()
        {
            Logger.Log("Manage.OnClose", Logger.manageLogLevel);

            if (controlVisible is IPlugInPage)
                ((IPlugInPage)controlVisible).OnClose();

            ResetManageState();
        }

        internal void SetRootParentNode(LACTreeNode tn)
        {
            Logger.Log(String.Format("Manage.SetRootParentNode: tn: {0}", tn == null ? "null" : tn.Name), Logger.manageLogLevel);

            _IPlugInRootNode = tn;
        }

        #endregion

        #region Property Accessors
        /// <summary>
        /// Returns the hashtable used to keep persistent information.
        /// TODO: use a more abstract return type.
        /// </summary>
        public Hashtable PersistedState
        {
            get
            {
                return htState;
            }
            set
            {
                htState = value;
            }
        }

        public static ImageList ImageList
        {
            get
            {
                return manageStaticImageList;
            }

        }

        public static ImageList LargeImageList
        {
            get
            {
                return manageStaticLargeImageList;
            }
        }

        #endregion

        #region Private helper functions

        /// <summary>
        /// Makes the waitform visible and centered in the right place
        /// </summary>

        /// <summary>
        /// Remove all the references to the lastly managed host.
        /// </summary>
        private void ResetManageState()
        {
            Logger.Log("Manage.ResetManageState", Logger.manageLogLevel);

            // close the visible control
            if (controlVisible != null)
            {
                if (controlVisible is IPlugInPage)
                    ((IPlugInPage)controlVisible).OnClose();
                controlVisible = null;
            }

            // ...and all controls
            pnlBody.Controls.Clear();

            // and the WKO list, too
            foreach (object o in htWKOs.Values)
                if (o is IDisposable)
                    ((IDisposable)o).Dispose();

            htWKOs.Clear();
        }

        /// <summary>
        /// Finds an object in the Well Known Object table.
        /// </summary>
        /// <param name="sName">The name of the object for which to search</param>
        /// <returns>The found object or null if not found</returns>
        public object FindWKO(string sName)
        {
            return htWKOs[sName];
        }

        private LACTreeNode GetRootNode()
        {
            Logger.Log("Manage.GetRootNode", Logger.manageLogLevel);

            LACTreeNode rootNode = null;

            try
            {
                string rootPluginPath = Path.Combine(Application.StartupPath, sRootPluginName);


                Assembly rootPluginDll = Assembly.LoadFile(rootPluginPath);
                LACTreeNodeList nodesInAssembly = LoadPlugInsFromAssembly(rootPluginDll);
                if (nodesInAssembly.Count > 0)
                {
                    rootNode = nodesInAssembly[0];
                    //don't show the hostname in the root node caption; use the default.
                    //rootNode.Text = _hn.hostName;
                }
            }
            catch (BadImageFormatException)
            {
                //
                // Ignore DLLs that are not managed code.
                //
            }
            catch (Exception e)
            {
                Logger.LogMsgBox(String.Format("Manage::GetRootNode() failed: {0}", e.GetType()));
                Logger.Log(e.ToString(), Logger.LogLevel.Error);


            }

            return rootNode;
        }

        private void BuildPluginList(LACTreeNode rootNode)
        {
            Logger.Log(String.Format("Manage.BuildPluginList(node: {0})",
                rootNode == null ? "null" : rootNode.Name), Logger.manageLogLevel);

            string[] dllPathList = Directory.GetFiles(Application.StartupPath, "*.dll");
            foreach (string dllPath in dllPathList)
            {
                if (Path.GetFileName(dllPath) == sRootPluginName)
                    continue;

                try
                {
                    Assembly pluginDll = Assembly.LoadFile(dllPath);
                    LACTreeNodeList nodesInAssembly = LoadPlugInsFromAssembly(pluginDll);
                    rootNode.Nodes.AddRange(nodesInAssembly.ToArray());
                }
                catch (BadImageFormatException)
                {
                    //
                    // Ignore DLLs that are not managed code.
                    //
                }
                catch (Exception e)
                {
                    Debugger.Log(9, "Exception", e.ToString());
                }
            }
        }

        public LACTreeNodeList  LoadPlugInsFromAssembly(Assembly a)
        {
            Logger.Log(String.Format("Manage.LoadPlugInsFromAssembly(assembly: {0})",
                a == null ? "null" : a.FullName), Logger.manageLogLevel);

            LACTreeNodeList nodeList = new LACTreeNodeList();

            try
            {
                Type[] atyp = a.GetTypes();
                foreach (Type t in atyp)
                {
                    if (t.GetInterface("Likewise.LMC.ServerControl.IPlugIn") != null)
                    {
                        IPlugIn pi = (IPlugIn)Activator.CreateInstance(t);

                        // get its name
                        pi.Initialize(this);
                        if (pi.GetContextType() == IContextType.DbConninfo)
                        {
                            pi.SetContext(new DbConnInfo());
                        }
                        else if (pi.GetContextType() == IContextType.Hostinfo)
                        {
                            pi.SetContext(new Hostinfo());
                        }

                        LACTreeNode pluginNode = pi.GetPlugInNode();
                        if (pluginNode != null)
                        {
                            pluginNode.Text = pi.GetName();
                            pluginNode.Name = pi.GetName();
                            pluginNode.sc = this.sc;
                            nodeList.Add(pluginNode);

                            Logger.Log(String.Format("Manage.LoadPlugInsFromAssembly (plugin: {0}, iconID: {1})",
                                 pluginNode.Name, pluginNode.ImageIndex), Logger.manageLogLevel);

                        }

                        else
                        {
                            Logger.Log(String.Format("Manage.LoadPlugInsFromAssembly FAILED! (pluginNode == null) (plugin: {0})",
                                pi.GetName()), Logger.manageLogLevel);
                        }


                    }
                }
            }
            catch (Exception e)
            {
                Debugger.Log(9, "Exception", e.ToString());
            }

            return nodeList;
        }

        public void ShowControl(LACTreeNode node)
        {
            if (node == null)
            {
                Logger.LogMsgBox("Manage.ShowControl: node == null!");
                return;
            }

            Logger.Log(String.Format("Manage.ShowControl{0}", node.Name), Logger.manageLogLevel);

            // zap the old node's control
            if (currentShownNode!=null)
                currentShownNode.PluginPage = null;

            // peg the new one
            currentShownNode = node;

            Type type = node.NodeType;
            IPlugIn piArg = node.Plugin;

            DateTime start = DateTime.Now;
            node.sc = sc;

            // create an object of the designated type
            //System.Runtime.Remoting.ObjectHandle o1 = Activator.CreateInstance(type.Assembly.FullName, type.FullName);
            //object o = o1.Unwrap();
            object o = null;
            try {
                o = Activator.CreateInstance(type, null);
            }
            catch (Exception ex) {
                Logger.LogException("Manage.ShowControl()", ex);
                Assembly a = Assembly.LoadFrom(type.Assembly.GetName().Name + ".dll");
                Type[] atyp = a.GetTypes();
                foreach (Type t in atyp)
                {
                    if ((t.GetInterface("Likewise.LMC.ServerControl.IPlugInPage") != null) &&
                        type.FullName.Trim().Equals(t.FullName.Trim(), StringComparison.InvariantCultureIgnoreCase))
                    {
                        o = Activator.CreateInstance(t);
                    }
                }
            }

#if SHOWCREATEANDCLOSE
            DateTime dtNow = DateTime.Now;
            Console.WriteLine(dtNow.ToShortTimeString() + " Created window of type: " + type.ToString());
#endif

            // should be a control
            if (o is Control)
            {
                // if supports the necessary interface, let it know who
                // is responsible for it. Note: this may cause the page
                // to do data access that can be slow, so let's do it
                // before we display the page
                if (o is IPlugInPage)
                {
                    ((IPlugInPage)o).SetPlugInInfo(this, piArg, node, _LWTreeView, sc);

                    ((IPlugInPage)o).SetContext(node.GetContext());

                    ((IPlugInPage)o).ShowActionPane(_bShowActionPane);

                }

                if (o is StandardPage)
                {
                    ((StandardPage)o).SetViewStyle(currentViewStyle);
                }

                Control c = (Control)o;

                // close the current control
                if (controlVisible != null)
                {
                    if (controlVisible is IPlugInPage)
                    {
                        ((IPlugInPage)controlVisible).OnClose();
#if SHOWCREATEANDCLOSE
                        Console.WriteLine("Closed window of type: " + controlVisible.GetType().ToString());
#endif
                    }

                    controlVisible.Hide();
                    pnlBody.Controls.Remove(controlVisible);

                    // [mvellon] 08-02-2006 Call dispose here so that we give up external resources immediately
                    // rather than waiting for garbage collection
                    controlVisible.Dispose();

                }

                // make the new tab visible
                c.Dock = DockStyle.Fill;
                SuspendLayout();
                pnlBody.Controls.Add(c);
                ResumeLayout();
                c.Show();
                c.Select();

                controlVisible = c;
                if (controlVisible is StandardPage)
                    node.PluginPage = controlVisible as StandardPage;
                else
                    node.PluginPage = null;

                piArg.EnumChildren(node);

                _LWTreeView.SelectedNode = node;
            }

        }

        /// <summary>
        /// Enables/disables the application
        /// </summary>
        /// <param name="bModal">If True, then disables the app. Enables otherwise.</param>
        private void SetModalState(bool bModal)
        {
            Control cTop = this;
            while (cTop.Parent != null)
                cTop = cTop.Parent;

            if (bModal)
            {
                cTop.Enabled = false;
            }
            else
            {
                cTop.Enabled = true;

                // bring to foreground, too
                cTop.BringToFront();
            }
        }

        private bool GetMachineInfoDelegate(string[] fields, Object context)
        {
            uint fieldsRequested = (uint)((UInt32)context);

            Logger.Log(String.Format(
                "Manage.GetMachineInfoDelegate: fieldsRequested=0x{0:x}",
                fieldsRequested),
                Logger.manageLogLevel);

            string shortHostName = null;
            string fullyQualifiedHostName = null;
            string FQDN = null;
            string fullyQualifiedDCName = null;
            string shortDomainName = null;
            string userName = null;
            string password = null;
            string errorMessage = null;
            bool result = true;

            int fieldIndex = 0;

            _hn = new Hostinfo();
            _hn.creds = new CredentialEntry();

            if (((uint)Hostinfo.FieldBitmaskBits.SHORT_HOSTNAME & fieldsRequested) > 0)
            {
                shortHostName = fields[fieldIndex++];
                if (!Hostinfo.ValidateHostName(shortHostName, out errorMessage))
                {
                    result = false;
                    Logger.Log("GetMachineInfoDelegate: invalid SHORT_HOSTNAME: " + errorMessage, Logger.manageLogLevel);
                }
                else
                {
                    _hn.hostName = shortHostName;
                    Logger.Log("GetMachineInfoDelegate: assigned SHORT_HOSTNAME:\n" + _hn.ToString(), Logger.manageLogLevel);
                }

            }

            if (result && ((uint)Hostinfo.FieldBitmaskBits.FQ_HOSTNAME & fieldsRequested) > 0)
            {
                fullyQualifiedHostName = fields[fieldIndex++];
                if (!Hostinfo.ValidateFullyQualifiedHostName(fullyQualifiedHostName, out errorMessage))
                {
                    result = false;
                    Logger.Log("GetMachineInfoDelegate: invalid FQ_HOSTNAME: " + errorMessage, Logger.manageLogLevel);
                }
                else
                {
                    _hn.hostName = fullyQualifiedHostName;
                    Logger.Log("GetMachineInfoDelegate: assigned FQ_HOSTNAME:\n" + _hn.ToString(), Logger.manageLogLevel);
                }
            }

            if (result && ((uint)Hostinfo.FieldBitmaskBits.FQDN & fieldsRequested) > 0)
            {
                FQDN = fields[fieldIndex++];
                if (!Hostinfo.ValidateFQDN(FQDN, out errorMessage))
                {
                    result = false;
                    Logger.Log("GetMachineInfoDelegate: invalid FQDN: " + errorMessage, Logger.manageLogLevel);
                }
                else
                {
                    _hn.domainName = FQDN;
                    Logger.Log("GetMachineInfoDelegate: assigned FQDN:\n" + _hn.ToString(), Logger.manageLogLevel);
                }
            }

            if (result && ((uint)Hostinfo.FieldBitmaskBits.FQ_DCNAME & fieldsRequested) > 0)
            {
                fullyQualifiedDCName = fields[fieldIndex++];
                if (!Hostinfo.ValidateFullyQualifiedDCName(fullyQualifiedDCName, out errorMessage))
                {
                    result = false;
                    Logger.Log("GetMachineInfoDelegate: invalid FQ_DCNAME: " + errorMessage, Logger.manageLogLevel);
                }
                else if (String.IsNullOrEmpty(_hn.hostName))
                {
                    _hn.domainControllerName = fullyQualifiedDCName;
                    Logger.Log("GetMachineInfoDelegate: assigned FQ_DCNAME:\n" + _hn.ToString(), Logger.manageLogLevel);
                }
            }

            if (result && ((uint)Hostinfo.FieldBitmaskBits.CREDS_NT4DOMAIN & fieldsRequested) > 0)
            {
                shortDomainName = fields[fieldIndex++];
                if (!Hostinfo.ValidateShortDomainName(shortDomainName, out errorMessage))
                {
                    result = false;
                    Logger.Log("GetMachineInfoDelegate: invalid CREDS_NT4DOMAIN: " + errorMessage, Logger.manageLogLevel);
                }
                else
                {
                    _hn.creds.Domain = shortDomainName;
                    Logger.Log("GetMachineInfoDelegate: assigned CREDS_NT4DOMAIN:\n" + _hn.ToString(), Logger.manageLogLevel);
                }
            }

            if (result && ((uint)Hostinfo.FieldBitmaskBits.CREDS_USERNAME & fieldsRequested) > 0)
            {
                userName = fields[fieldIndex++];
                if (!Hostinfo.ValidateUserName(userName, out errorMessage))
                {
                    result = false;
                    Logger.Log("GetMachineInfoDelegate: invalid CREDS_USERNAME: " + errorMessage, Logger.manageLogLevel);
                }
                else
                {
                    _hn.creds.UserName = userName;
                    Logger.Log("GetMachineInfoDelegate: assigned CREDS_USERNAME:\n" + _hn.ToString(), Logger.manageLogLevel);
                }
            }

            if (result && ((uint)Hostinfo.FieldBitmaskBits.CREDS_PASSWORD & fieldsRequested) > 0)
            {
                password = fields[fieldIndex++];
                if (!Hostinfo.ValidatePassword(password, password, out errorMessage))
                {
                    result = false;
                    Logger.Log("GetMachineInfoDelegate: invalid CREDS_PASSWORD: " + errorMessage, Logger.manageLogLevel);
                }
                else
                {
                    _hn.creds.Password = password;
                    Logger.Log("GetMachineInfoDelegate: assigned CREDS_PASSWORD:\n" + _hn.ToString(), Logger.manageLogLevel);
                }
            }

            if (!result)
            {
                sc.ShowError(errorMessage);
                return false;
            }

            _hn.creds.MachineName = _hn.hostName;

            if (String.IsNullOrEmpty(_hn.domainControllerName) &&
              !String.IsNullOrEmpty(_hn.domainName))
            {
                try
                {
                    CNetlogon.LWNET_DC_INFO DCInfo;
                    uint netlogonError = CNetlogon.GetDCName(_hn.domainName, 0, out DCInfo);
                    if (netlogonError == 0 && !String.IsNullOrEmpty(DCInfo.DomainControllerName))
                    {
                        _hn.domainControllerName = DCInfo.DomainControllerName;
                        if (String.IsNullOrEmpty(_hn.creds.Domain))
                        {
                            _hn.creds.Domain = DCInfo.NetBIOSDomainName;
                        }
                    }
                    else
                    {
                        _hn.domainControllerName = _hn.domainName;
                    }
                    if (netlogonError == 0 && !String.IsNullOrEmpty(DCInfo.NetBIOSDomainName))
                    {
                        if (shortDomainName != null)
                        {
                            if (!shortDomainName.Trim().ToUpper().Equals(DCInfo.NetBIOSDomainName.Trim().ToUpper()))
                            {
                                errorMessage = "Invalid NT4-Style Domain Name";

                                sc.ShowError(errorMessage);
                                return false;
                            }
                        }
                        else
                        {
                            _hn.creds.Domain = DCInfo.NetBIOSDomainName;
                        }
                    }
                }
                catch(Exception ex)
                {
                    Logger.Log("Exception occured while getting DCInfo ," + ex.Message);
                    return false;
                }
            }

            SaveTargetMachineInfoToIni(_hn);

            getTargetMachineRequestor.SetContext(_hn);

            if (_hn.IsConnectionSuccess == false)
            {
                return false;
            }

            return true;
        }

        /// <summary>
        /// Method to create the ini file, if doesn't exits.
        /// </summary>
        /// <param name="Path"></param>
        private void CreateCredentialsIni(string Path)
        {
            FileStream fileStream = null;
            try
            {
                Logger.Log(String.Format(
                    "Manage.CreateCredentialsIni: creating {0}", Path),
                    Logger.manageLogLevel);
                fileStream = new FileStream(Path, FileMode.OpenOrCreate, FileAccess.ReadWrite);
            }
            catch (Exception)
            {
                Logger.Log(String.Format(
                    "Manage.CreateCredentialsIni: failed to create {0}", Path),
                    Logger.manageLogLevel);
            }
            finally
            {
                if (fileStream != null)
                    fileStream.Close();
            }
        }

        /// <summary>
        /// Method to store the plugin's host info to ini file.
        /// </summary>
        /// <param name="_hn"></param>
        private void SaveTargetMachineInfoToIni(Hostinfo _hn)
        {
            Logger.Log(String.Format(
                "Manage.SaveTargetMachineInfoToIni: saving the target machine info to ini: _hn =\n {0}",
                _hn == null ? "<null>" : _hn.ToString()),
                Logger.manageLogLevel);

            string sPath = Path.Combine(Application.UserAppDataPath, @"Temp.ini");

            CreateCredentialsIni(sCredentialsFilePath);

            StreamReader reader = new StreamReader(sCredentialsFilePath);
            FileStream fStream = new FileStream(sPath, FileMode.OpenOrCreate, FileAccess.ReadWrite, FileShare.ReadWrite);
            StreamWriter writer = new StreamWriter(fStream);

            string sPlugInName = GetPlugInName(getTargetMachineRequestor);

            Logger.Log(String.Format(
                "Manage.SaveTargetMachineInfoToIni: plugin name={0}",
                sPlugInName==null ? "<null>" : sPlugInName),
                Logger.manageLogLevel);

            if (!String.IsNullOrEmpty(sPlugInName))
            {
                string currentLine = reader.ReadLine();

                while (!(currentLine != null && currentLine.Trim().Equals(sPlugInName)) &&
                       !reader.EndOfStream)
                {
                    writer.WriteLine(currentLine);
                    currentLine = reader.ReadLine();
                }

                writer.WriteLine(sPlugInName);
                if (!String.IsNullOrEmpty(_hn.hostName))
                {
                    writer.WriteLine("hostname=" + _hn.hostName.Trim());
                }
                if (_hn.creds != null && !String.IsNullOrEmpty(_hn.creds.UserName))
                {
                    writer.WriteLine("username=" + _hn.creds.UserName.Trim());
                }
                if (!String.IsNullOrEmpty(_hn.domainName))
                {
                    writer.WriteLine("domainFQDN=" + _hn.domainName.Trim());
                }
                if (_hn.creds != null && !String.IsNullOrEmpty(_hn.creds.Domain))
                {
                    writer.WriteLine("domainShort=" + _hn.creds.Domain.Trim());
                }
                writer.WriteLine("");

                //make sure the remainder of the old file is present in the new one.
                currentLine = reader.ReadLine();
                while (!(currentLine != null && currentLine.Trim().Length > 0 && currentLine.Trim()[0] == '[') &&
                        !reader.EndOfStream)
                {
                    currentLine = reader.ReadLine();
                }
                while (!reader.EndOfStream)
                {
                    writer.WriteLine(currentLine);
                    currentLine = reader.ReadLine();
                }
                if (reader.EndOfStream)
                {
                    writer.WriteLine("");
                }
            }
            writer.Flush();

            writer.Close();
            reader.Close();
            fStream.Close();
            fStream.Dispose();

            if (File.Exists(sCredentialsFilePath) && File.Exists(sPath))
            {
                File.Delete(sCredentialsFilePath);
                File.Move(sPath, sCredentialsFilePath);
            }
        }

        /// <summary>
        /// Reads the target machine info for the current plugin from credentilas ini and assign to Hostinfo
        /// </summary>
        /// <param name="requestor"></param>
        /// <param name="_hn"></param>
        public void GetConnectedHostInfoFromIni(IPlugIn requestor, Hostinfo _hn)
        {
            string sPlugInName = GetPlugInName(requestor);

            bool IsFileExists = !string.IsNullOrEmpty(sCredentialsFilePath) && Path.IsPathRooted(sCredentialsFilePath) && File.Exists(sCredentialsFilePath);

            if (!IsFileExists)
                return;

            StreamReader reader = new StreamReader(sCredentialsFilePath);

            while (!reader.EndOfStream)
            {
                string currentLine = reader.ReadLine();
                if (currentLine != null && currentLine.Trim().Equals(sPlugInName))
                {
                    currentLine = reader.ReadLine();

                    int index = 0;
                    if (currentLine != null && currentLine.Trim().IndexOf("hostname=") >= 0 &&
                        String.IsNullOrEmpty(_hn.hostName))
                    {
                        currentLine = currentLine.Trim();
                        index = (currentLine.IndexOf('=') + 1);
                        _hn.hostName = currentLine.Substring(index, (currentLine.Length - index));
                        currentLine = reader.ReadLine();
                    }

                    if (currentLine != null && currentLine.Trim().IndexOf("username=") >= 0 &&
                        String.IsNullOrEmpty(_hn.creds.UserName))
                    {
                        currentLine = currentLine.Trim();
                        index = (currentLine.IndexOf('=') + 1);
                        _hn.creds.UserName = currentLine.Substring(index, (currentLine.Length - index));
                        currentLine = reader.ReadLine();
                    }

                    if (currentLine != null && currentLine.Trim().IndexOf("domainFQDN=") >= 0 &&
                        String.IsNullOrEmpty(_hn.domainName))
                    {
                        currentLine = currentLine.Trim();
                        index = (currentLine.IndexOf('=') + 1);
                        _hn.domainName = currentLine.Substring(index, (currentLine.Length - index));
                        currentLine = reader.ReadLine();
                    }

                    if (currentLine != null && currentLine.IndexOf("domainShort=") >= 0 &&
                        String.IsNullOrEmpty(_hn.creds.Domain))
                    {
                        currentLine = currentLine.Trim();
                        index = (currentLine.IndexOf('=') + 1);
                        _hn.creds.Domain = currentLine.Substring(index, (currentLine.Length - index));
                    }
                    break;
                }
            }
            reader.Close();
        }

        // public string SaveConsoleSettingsToXml(bool staturbar, Size maxPos, Size minPos)
        public string SaveConsoleSettingsToXml( FrameState state )
        {
            XmlDocument XmlDoc = new XmlDocument();

            string filePath = Path.Combine(Configurations.tempDirectory, "Console1.lmc");

            try
            {
                XmlDoc.LoadXml(Resources.ConsoleSettings);

                if (XmlDoc == null)
                {
                    return null;
                }
                XmlElement frameNode = (XmlElement)XmlDoc.GetElementsByTagName("FrameState")[0];
                if (frameNode != null && frameNode.Attributes["ShowStatusBar"] != null)
                {
                    frameNode.Attributes["ShowMaximized"].Value = state.bShowMaximized.ToString();
                    frameNode.Attributes["ShowStatusBar"].Value = state.bShowStatusBar.ToString();
                    frameNode.Attributes["ShowNavigationBar"].Value = state.bShowNavigationBar.ToString();
                    frameNode.Attributes["ShowNavigationTree"].Value = state.bShowTreeControl.ToString();
                    frameNode.Attributes["ReadOnly"].Value = state.bReadOnly.ToString();
                }
                XmlElement rectNode = (XmlElement)XmlDoc.GetElementsByTagName("Rectangle")[0];
                if (rectNode != null)
                {
                    rectNode.Attributes["Top"].Value = state.top.ToString();;
                    rectNode.Attributes["Left"].Value = state.left.ToString();;
                    rectNode.Attributes["Bottom"].Value = state.bottom.ToString();
                    rectNode.Attributes["Right"].Value = state.right.ToString();
                }
                XmlElement StateNode = (XmlElement)XmlDoc.GetElementsByTagName("State")[0];
                if (StateNode != null)
                {
                    foreach (string sKey in htState.Keys)
                    {
                        XmlElement stateElement = StateNode.OwnerDocument.CreateElement("Key");
                        stateElement.SetAttribute("Name", sKey);
                        if (htState[sKey] is string)
                        {
                            stateElement.SetAttribute("Type", "String");
                            stateElement.InnerText = htState[sKey] as string;
                        }
                        else if (htState[sKey] is byte[])
                        {
                            stateElement.SetAttribute("Type", "ByteArray");
                            stateElement.InnerText = Convert.ToBase64String(htState[sKey] as byte[]);
                        }
                        StateNode.AppendChild(stateElement);
                    }
                }
                XmlElement ViewsNode = (XmlElement)XmlDoc.GetElementsByTagName("Views")[0];
                if (ViewsNode != null)
                {
                    int Id = 0;
                    XmlElement viewElement = null;
                    TreeNodeCollection childNodes = lmcLWTreeView.Nodes[0].Nodes;

                    foreach (LACTreeNode pluginNode in childNodes)
                    {
                        pluginNode.Plugin.SerializePluginInfo(pluginNode, ref Id, out viewElement, ViewsNode, lmcLWTreeView.SelectedNode);
                        if (viewElement != null)
                        {
                            ViewsNode.AppendChild(viewElement);
                        }
                    }
                }
                XmlDoc.Save(filePath);
            }
            catch(Exception ex)
            {
                Logger.LogException("Manage.SaveConsoleSettingsToXml()", ex);
            }

            return filePath;
        }

        private void RecursivePluginNodeInfo(XmlNode node, ref LACTreeNode pluginNode, string nodepath)
        {
            IPlugIn pi = null;

            try
            {
                string nodename = node.Attributes["NodeName"].Value as string;
                string nodeDll = Path.Combine(Application.StartupPath, node.Attributes["NodeDll"].Value as string);
                Assembly pluginDll = Assembly.LoadFile(nodeDll);
                Type[] atyp = pluginDll.GetTypes();
                foreach (Type t in atyp)
                {
                    if (t.GetInterface("Likewise.LMC.ServerControl.IPlugIn") != null)
                    {
                        pi = (IPlugIn)Activator.CreateInstance(t);

                        pi.Initialize(this);
                        pluginNode = pi.GetPlugInNode();
                        if (pluginNode == null)
                            return;
                        break;
                    }
                }

                if (Convert.ToBoolean(node.Attributes["NodeSelected"].Value))
                {
                    pluginNode.IsSelected = true;
                }

                if (nodename.IndexOf('[') >= 0 || nodename.IndexOf('(') >= 0)
                {
                    if (nodename.Contains("["))
                        nodename = nodename.Substring(0, nodename.IndexOf('['));
                    else if (nodename.Contains("("))
                        nodename = nodename.Substring(0, nodename.IndexOf('('));
                }
                pluginNode.Name = pluginNode.Text = nodename;

                pi.DeserializePluginInfo(node, ref pluginNode, nodepath);

                if (node.ChildNodes.Count != 0)
                {
                    foreach (XmlNode childnode in node.ChildNodes)
                    {
                        if (childnode.Name.Trim().Equals("View"))
                        {
                            LACTreeNode innerPluginnode = null;
                            RecursivePluginNodeInfo(childnode, ref innerPluginnode, nodepath);
                            if (innerPluginnode != null)
                            {
                                pluginNode.Nodes.Add(innerPluginnode);
                            }
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                Logger.LogException("Manage.RecursivePluginNodeInfo()", ex);
            }
        }

        public void ReadPluginNodeInfoFromConsoleSettings(string filePath, FrameState state)
        {
            XmlDocument xmldoc = new XmlDocument();

            try
            {
                xmldoc.Load(filePath);

                if (xmldoc == null)
                {
                    return;
                }

                // restore the main window state
                string nodepath = "LMC_ConsoleFile/FrameState";
                XmlNode FrameStateNode = xmldoc.SelectSingleNode(nodepath);
                if (FrameStateNode != null)
                {
                    state.bShowMaximized = bool.Parse(FrameStateNode.Attributes["ShowMaximized"].Value);
                    state.bShowStatusBar = bool.Parse(FrameStateNode.Attributes["ShowStatusBar"].Value);
                    state.bShowNavigationBar = bool.Parse(FrameStateNode.Attributes["ShowNavigationBar"].Value);
                    state.bShowTreeControl = bool.Parse(FrameStateNode.Attributes["ShowNavigationTree"].Value);
                    state.bReadOnly = bool.Parse(FrameStateNode.Attributes["ReadOnly"].Value);
                }

                nodepath = "LMC_ConsoleFile/FrameState/Rectangle";
                XmlNode RectangleNode = xmldoc.SelectSingleNode(nodepath);
                if (RectangleNode != null)
                {
                    state.left = int.Parse(RectangleNode.Attributes["Left"].Value);
                    state.top = int.Parse(RectangleNode.Attributes["Top"].Value);
                    state.right = int.Parse(RectangleNode.Attributes["Right"].Value);
                    state.bottom = int.Parse(RectangleNode.Attributes["Bottom"].Value);
                }

                // reload persisted state
                nodepath = "LMC_ConsoleFile/State";
                XmlNode StateNode = xmldoc.SelectSingleNode(nodepath);
                if (StateNode != null && StateNode.ChildNodes.Count != 0)
                {
                    htState.Clear();
                    foreach (XmlNode node in StateNode.ChildNodes)
                    {
                        string sKey = node.Attributes["Name"].Value as string;
                        string sType = node.Attributes["Type"].Value as string;

                        object value = null;
                        if (sType == "String")
                            value = node.InnerText;
                        else
                            value = Convert.FromBase64String(node.InnerText);

                        htState.Add(sKey, value);
                    }
                }

                nodepath = "LMC_ConsoleFile/Views";
                XmlNode ViewsNode = xmldoc.SelectSingleNode(nodepath);
                if (ViewsNode != null && ViewsNode.ChildNodes.Count != 0)
                {
                    foreach (XmlNode node in ViewsNode.ChildNodes)
                    {
                        LACTreeNode pluginnode = null;
                        RecursivePluginNodeInfo(node, ref pluginnode, nodepath);

                        if (pluginnode != null)
                        {
                            rootNode.Nodes.Add(pluginnode);
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                Logger.LogException("Manage.ReadPluginNodeInfoFromConsoleSettings()", ex);
            }
        }

        private void SetSelectedNodeToTreeView(LACTreeNode pluginNode)
        {
            if (pluginNode.IsSelected)
            {
                rootNode.TreeView.SelectedNode = pluginNode;
            }
            else
            {
                if (pluginNode.Nodes.Count != 0)
                {
                    foreach (LACTreeNode node in pluginNode.Nodes)
                    {
                        SetSelectedNodeToTreeView(node);
                    }
                }
            }
        }

        #endregion

        #region IPlugInContainer Members

        void IPlugInContainer.SetRootParentNode(LACTreeNode tn)
        {
            Logger.Log(String.Format("Manage.SetRootParentNode: tn: {0}", tn == null ? "null" : tn.Name), Logger.manageLogLevel);

            _IPlugInRootNode = tn;
        }

        public void SetLWTreeView(LWTreeView tv)
        {
            Logger.Log(String.Format("Manage.SetLWTreeView: tv: {0}", tv == null ? "null" : "non-null"), Logger.manageLogLevel);

            _LWTreeView = tv;
            tv.ImageList = manageStaticImageList;

        }

        LACTreeNode IPlugInContainer.GetRootParentNode()
        {
            Logger.Log("Manage.GetRootParentNode", Logger.manageLogLevel);
            return _IPlugInRootNode;
        }



        public bool GetConnectionInfoFromNetlogon(
                        IPlugIn requestor,
                        Hostinfo hn,
                        uint fieldsRequested,
                        out uint fieldsFilled)
        {
            uint error = 0;

            fieldsFilled = 0;
            bool result = true;

            Logger.Log("Manage.GetConnectionInfoFromNetlogon");

            if (!String.IsNullOrEmpty(hn.domainName))
            {
                return result;
            }

            error = CNetlogon.GetCurrentDomain(out hn.domainName);

            if (error == 0 && !String.IsNullOrEmpty(hn.domainName))
            {
                fieldsFilled |= (uint)Hostinfo.FieldBitmaskBits.FQDN;
                CNetlogon.LWNET_DC_INFO DCInfo;

                uint netlogonError = CNetlogon.GetDCName(hn.domainName, 0, out DCInfo);

                if (netlogonError == 0)
                {
                    if (!String.IsNullOrEmpty(DCInfo.DomainControllerName))
                    {
                        hn.domainControllerName = DCInfo.DomainControllerName;
                        fieldsFilled |= (uint)Hostinfo.FieldBitmaskBits.FQ_DCNAME;
                    }

                    if (!String.IsNullOrEmpty(DCInfo.NetBIOSDomainName))
                    {
                        hn.creds.Domain = DCInfo.NetBIOSDomainName;
                        fieldsFilled |= (uint)Hostinfo.FieldBitmaskBits.CREDS_NT4DOMAIN;
                    }
                    if (!String.IsNullOrEmpty(DCInfo.FullyQualifiedDomainName))
                    {
                        hn.domainName = DCInfo.FullyQualifiedDomainName;
                        fieldsFilled |= (uint)Hostinfo.FieldBitmaskBits.FQDN;
                    }
                }
                else
                {
                    hn.domainControllerName = hn.domainName;
                    result = false;
                }
            }

            Logger.Log(String.Format(
                "Manage.GetConnectionInfoFromNetlogon returning {0}",
                result),
                Logger.manageLogLevel);

            return result;

        }

        private bool GetTargetMachineInfoDbConnInfo(IPlugIn requestor, DbConnInfo DbConn, uint fieldsRequested)
        {
            return false;
        }


        private bool GetTargetMachineInfoHostInfo(IPlugIn requestor, Hostinfo hn, uint fieldsRequested)
        {
            string caption = "Set Target Machine";
            string groupBoxCaption = requestor.GetName();
            string[] descriptions, hints, fieldContents;
            int numFields = 0;
            int fieldIndex = 0;
            int securityFieldIndex = 0;
            bool okWithoutModify = true;
            uint fieldsFilled = 0;

            Logger.Log(String.Format("Manage.GetTargetMachineInfo: requestor: {0} fields: 0x{1:x}",
                requestor == null ? "null" : requestor.GetName(),
                fieldsRequested),
                Logger.manageLogLevel);

            getTargetMachineRequestor = requestor;

            if (fieldsRequested == Hostinfo.FieldBitmaskBits.FQDN)
            {
                requestor.SetSingleSignOn(true);
            }

            if (hn == null)
            {
                hn = new Hostinfo();
            }

            if (String.IsNullOrEmpty(hn.creds.Domain) && !String.IsNullOrEmpty(System.Environment.UserDomainName))
            {
                hn.creds.Domain = System.Environment.UserDomainName;
                fieldsFilled |= (uint)Hostinfo.FieldBitmaskBits.CREDS_NT4DOMAIN;
            }

            if (String.IsNullOrEmpty(hn.creds.UserName) && !String.IsNullOrEmpty(System.Environment.UserName))
            {
                hn.creds.UserName = System.Environment.UserName;
                fieldsFilled |= (uint)Hostinfo.FieldBitmaskBits.CREDS_USERNAME;
            }

            if (String.IsNullOrEmpty(hn.hostName) && !String.IsNullOrEmpty(System.Environment.MachineName))
            {
                hn.hostName = System.Environment.MachineName;
                hn.creds.MachineName = System.Environment.MachineName;
                fieldsFilled |= (uint)Hostinfo.FieldBitmaskBits.SHORT_HOSTNAME;
            }

            try
            {
                if (!GetConnectionInfoFromNetlogon(requestor, hn, fieldsRequested, out fieldsFilled))
                {
                    bSSOFailed = true;
                }
                else
                {
                    //if all fields requested were filled
                    if ((fieldsRequested & (~fieldsFilled)) == 0 ||
                        (fieldsRequested & (~fieldsFilled)) == 4)
                    {
                        Logger.Log(String.Format(
                            "GetConnectionInfoFromNetlogon: requested: {0}  filled: {1}  hn: {2}",
                            fieldsRequested, fieldsFilled, hn.ToString()), Logger.manageLogLevel);

                        if (((uint)Hostinfo.FieldBitmaskBits.FORCE_USER_PROMPT & fieldsRequested) == 0)
                        {
                            requestor.SetContext(hn);
                            if (hn.IsConnectionSuccess)
                            {
                                return true;
                            }
                            else
                            {
                                bSSOFailed = true;
                                requestor.SetSingleSignOn(false);
                            }
                        }
                    }
                }

                if (bSSOFailed)
                {
                    bSSOFailed = false;
                    Configurations.SSOFailed = true;

                    string errMsg = "Single sign-on failed.  Please make sure you have joined the domain," +
                              "and that you have cached kerberos credentials on your system.  " +
                              "Click OK for manual authentication.";
                    sc.ShowError(errMsg);

                    return false;
                }

                //this won't overwrite existing values in hn.
                GetConnectedHostInfoFromIni(requestor, hn);

                if (((uint)Hostinfo.FieldBitmaskBits.SHORT_HOSTNAME & fieldsRequested) > 0) { numFields++; }
                if (((uint)Hostinfo.FieldBitmaskBits.FQ_HOSTNAME & fieldsRequested) > 0) { numFields++; }
                if (((uint)Hostinfo.FieldBitmaskBits.FQDN & fieldsRequested) > 0) { numFields++; }
                if (((uint)Hostinfo.FieldBitmaskBits.FQ_DCNAME & fieldsRequested) > 0) { numFields++; }
                if (((uint)Hostinfo.FieldBitmaskBits.CREDS_NT4DOMAIN & fieldsRequested) > 0) { numFields++; }
                if (((uint)Hostinfo.FieldBitmaskBits.CREDS_USERNAME & fieldsRequested) > 0) { numFields++; }
                if (((uint)Hostinfo.FieldBitmaskBits.CREDS_PASSWORD & fieldsRequested) > 0) { numFields++; }

                descriptions = new string[numFields];
                hints = new string[numFields];
                fieldContents = new string[numFields];

                if (((uint)Hostinfo.FieldBitmaskBits.SHORT_HOSTNAME & fieldsRequested) > 0)
                {
                    descriptions[fieldIndex] = "Short Hostname";
                    fieldContents[fieldIndex] = hn.shortName;
                    hints[fieldIndex++] = "ex: mycomputer";
                }

                if (((uint)Hostinfo.FieldBitmaskBits.FQ_HOSTNAME & fieldsRequested) > 0)
                {
                    descriptions[fieldIndex] = "Fully Qualified Hostname";
                    fieldContents[fieldIndex] = hn.hostName;
                    hints[fieldIndex++] = "ex: mycomputer.mycorp.com";
                }

                if (((uint)Hostinfo.FieldBitmaskBits.FQDN & fieldsRequested) > 0)
                {
                    descriptions[fieldIndex] = "Fully Qualified Domain Name";
                    fieldContents[fieldIndex] = hn.domainName;
                    hints[fieldIndex++] = "ex: mycorp.com";
                }

                if (((uint)Hostinfo.FieldBitmaskBits.FQ_DCNAME & fieldsRequested) > 0)
                {
                    descriptions[fieldIndex] = "Domain Controller FQDN";
                    fieldContents[fieldIndex] = hn.domainControllerName;
                    hints[fieldIndex++] = "ex: dc-2.mycorp.com";
                }

                if (((uint)Hostinfo.FieldBitmaskBits.CREDS_NT4DOMAIN & fieldsRequested) > 0)
                {
                    descriptions[fieldIndex] = "NT4-style Domain Name";
                    fieldContents[fieldIndex] = hn.creds.Domain;
                    hints[fieldIndex++] = "ex: CORP";
                }

                if (((uint)Hostinfo.FieldBitmaskBits.CREDS_USERNAME & fieldsRequested) > 0)
                {
                    descriptions[fieldIndex] = "Username";
                    fieldContents[fieldIndex] = hn.creds.UserName;
                    hints[fieldIndex++] = "ex: flastname3";
                }

                if (((uint)Hostinfo.FieldBitmaskBits.CREDS_PASSWORD & fieldsRequested) > 0)
                {
                    securityFieldIndex = fieldIndex;
                    fieldContents[fieldIndex] = hn.creds.Password;
                    descriptions[fieldIndex] = "Password";
                    hints[fieldIndex++] = "";
                }

                StringRequestDialog dlg = new StringRequestDialog(
                    GetMachineInfoDelegate,
                    caption,
                    groupBoxCaption,
                    descriptions,
                    hints,
                    fieldContents,
                    (UInt32)fieldsRequested);

                dlg.allowOKWithoutModify = okWithoutModify;
                if (securityFieldIndex > 0)
                {
                    dlg.SetSecurityStatus(securityFieldIndex, true);
                }

                dlg.ShowDialog();

                if(!dlg.bDialogResult)
                {
                    hn.IsConnectionSuccess = false;
                    return false;
                }
            }
            catch
            {
                hn.IsConnectionSuccess = false;
                return true;
            }

            return true;
        }

        public bool GetTargetMachineInfo(IPlugIn requestor, IContext ctx, uint fieldsRequested)
        {
            if (requestor.GetContextType() == IContextType.Hostinfo)
            {
                return GetTargetMachineInfoHostInfo(requestor, (Hostinfo)ctx, fieldsRequested);
            }
            else if (requestor.GetContextType() == IContextType.DbConninfo)
            {
                return GetTargetMachineInfoDbConnInfo(requestor, (DbConnInfo)ctx, fieldsRequested);
            }

            return false;
        }

        /// <summary>
        /// Gets the current plugin name
        /// </summary>
        /// <param name="requestor"></param>
        /// <returns></returns>
        private string GetPlugInName(IPlugIn requestor)
        {
            string sPlugInName = requestor.GetName();

            switch (sPlugInName.Trim())
            {
                case "Active Directory Users & Computers":
                    sPlugInName = "[ADUC]";
                    break;
                case "Local Users and Groups":
                    sPlugInName = "[LUG]";
                    break;
                case "File and Print":
                    sPlugInName = "[File and Print]";
                    break;
                case "Likewise Cell Manager":
                    sPlugInName = "[Likewise Cell Manager]";
                    break;
                case "Event Viewer":
                    sPlugInName = "[Event Viewer]";
                    break;
                case "Group Policy Management":
                    sPlugInName = "[GPMC]";
                    break;
                case "Group Policy Object Editor":
                    sPlugInName = "[GPOE]";
                    break;
            }
            return sPlugInName;
        }

        public void ShowActionPane(bool bShowActionPane)
        {
            Logger.Log("Manage.ShowActionPane", Logger.manageLogLevel);
            _bShowActionPane = bShowActionPane;
            IPlugInPage pPage = controlVisible as IPlugInPage;
            if (pPage != null)
            {
                pPage.ShowActionPane(_bShowActionPane);
            }
        }

        /// <summary>
        /// Displays prompt in standard Likewise form
        /// </summary>
        /// <param name="sMessage">Message to show</param>
        /// <param name="buttons">Buttons to display</param>
        /// <returns>The selected button</returns>
        public DialogResult Prompt(string sMessage, MessageBoxButtons buttons)
        {
            Logger.Log(String.Format("Manage.Prompt: {0}", sMessage), Logger.manageLogLevel);

            // pass back to server control
            return sc.Prompt(sMessage, buttons);
        }

        /// <summary>
        /// Displays an informatory message in standard Likewise form
        /// </summary>
        /// <param name="sMessage"></param>
        public void Note(string sMessage)
        {
            Logger.Log(String.Format("Manage.Note: {0}", sMessage), Logger.manageLogLevel);

            // pass back to server control
            sc.Note(sMessage);
        }

        /// <summary>
        /// Displays an error message in standard Likewise form
        /// </summary>
        /// <param name="sMessage">Error message</param>
        /// <param name="buttons">Buttons to display</param>
        /// <returns>As per the selected button</returns>
        public DialogResult ShowError(string sMessage, MessageBoxButtons buttons)
        {
            Logger.Log(String.Format("Manage.ShowError: {0}", sMessage), Logger.manageLogLevel);

            // pass back to server control
            return sc.ShowError(sMessage, buttons);
        }

        /// <summary>
        /// Displays an error message and the "ok" button.
        /// </summary>
        /// <param name="sMessage">Error message</param>
        public void ShowError(string sMessage)
        {
            this.ShowError(sMessage, MessageBoxButtons.OK);
        }

        public DialogResult ShowError(Control ctl, string sMessage)
        {
            return sc.ShowError(ctl, sMessage);
        }

        /// <summary>
        /// Displays a message in standard Likewise form
        /// </summary>
        /// <param name="sMessage">message</param>
        /// <param name="buttons">Buttons to display</param>
        /// <returns>As per the selected button</returns>
        public DialogResult ShowMessage(string sMessage, MessageBoxButtons buttons, MessageBoxIcon icon)
        {
            Logger.Log(String.Format("Manage.ShowMessage: {0}", sMessage), Logger.manageLogLevel);

            // pass back to server control
            return sc.ShowMessage(sMessage, buttons, icon);
        }

        /// <summary>
        /// Displays a message in standard Likewise form
        /// </summary>
        /// <param name="sMessage">message</param>
        /// <param name="buttons">Buttons to display</param>
        /// <returns>As per the selected button</returns>
        public DialogResult ShowMessage(string sMessage, MessageBoxButtons buttons)
        {
            Logger.Log(String.Format("Manage.ShowMessage: {0}", sMessage), Logger.manageLogLevel);

            return sc.ShowMessage(sMessage, buttons, MessageBoxIcon.Information);
        }

        /// <summary>
        /// Displays an message and the "ok" button.
        /// </summary>
        /// <param name="sMessage">message</param>
        public void ShowMessage(string sMessage)
        {
            this.ShowMessage(sMessage, MessageBoxButtons.OK);
        }

        /// <summary>
        /// Saves sData associated with the given name. Care should be taken to
        /// assure that sName is unique.
        /// </summary>
        /// <param name="sName">The name to associate with the data</param>
        /// <param name="sData">The string data to be saved</param>
        public void SaveState(string sName, string sData)
        {
            if (htState.Contains(sName))
            {
                htState[sName] = sData;
            }
            else
            {
                htState.Add(sName, sData);
            }
        }

        /// <summary>
        /// Saves binary array data associated with the given name. Care should be taken to assure
        /// that sName is unique
        /// </summary>
        /// <param name="sName">The name to associate with the data</param>
        /// <param name="abData">The data to be saved</param>
        public void SaveState(string sName, byte[] abData)
        {
            if (htState.Contains(sName))
            {
                htState[sName] = abData;
            }
            else
            {
                htState.Add(sName, abData);
            }
        }

        /// <summary>
        /// Retrieves the data associated with a name.
        /// </summary>
        /// <param name="sName">The name whose data should be returned</param>
        /// <returns>An object representing the data. The caller should test the return type
        /// to assure that it is as expected.</returns>
        public object LoadState(string sName)
        {
            return htState[sName];
        }

        //sSubName will be "QUERY" or "DESCRIPTION"
        public List<string> LoadStateKeyContains(string sFolderName, string sSubName)
        {
            string sKey = "";
            List<string> sKeyList = new List<string>();

            foreach (DictionaryEntry deEntry in htState)
            {
                sKey = deEntry.Key.ToString();
                if (sKey.Contains(sFolderName) && sKey.Contains(sSubName))
                {
                    sKeyList.Add(sKey);
                }
            }

            return sKeyList;
        }

        public void RemoveState(string sName)
        {
            htState.Remove(sName);
        }

        /// <summary>
        /// Returns the Likewise documentation directory.
        /// </summary>
        /// <returns>Documentation directory path.</returns>
        public static string GetDocDir()
        {
            RegistryKey rk = Registry.LocalMachine;
            RegistryKey rkCESM = rk.OpenSubKey(@"SOFTWARE\Likewise\Enterprise");

            string docDir = "";
            if (rkCESM != null)
            {
                docDir = (string)rkCESM.GetValue("ApplicationPath") + @"\documentation\";
            }
            else
            {
                docDir = Environment.CurrentDirectory + @"\";
            }

            return docDir;
        }

        public static LACTreeNode CreateNode(string Name, Bitmap image, Type type, IPlugIn plugin)
        {
            Logger.Log(String.Format("Manage.CreateNode: {0}", Name), Logger.manageLogLevel);

            Bitmap bmp = new Bitmap(image, 32, 32);
            LACTreeNode lacNode = new LACTreeNode(Name, bmp, type, plugin);
            lacNode.Name = Name;
            return lacNode;
        }

        public static LACTreeNode CreateIconNode(string Name, Icon icon, Type type, IPlugIn plugin)
        {
            Logger.Log(String.Format("Manage.CreateIconNode: {0}", Name), Logger.manageLogLevel);

            LACTreeNode lacNode = new LACTreeNode(Name, icon, type, plugin);
            lacNode.Name = Name;
            return lacNode;
        }

        public static void InitSerializePluginInfo(LACTreeNode pluginNode, IPlugIn ip, ref int Id, out XmlElement viewElement, XmlElement ViewsNode, TreeNode SelectedNode)
        {
            viewElement = null;

            viewElement = ViewsNode.OwnerDocument.CreateElement("View");
            viewElement.SetAttribute("NodeID", Id.ToString());
            viewElement.SetAttribute("NodeName", pluginNode.Name);
            viewElement.SetAttribute("NodeDll", ip.GetPluginDllName());
            if (pluginNode.Equals(SelectedNode))
                viewElement.SetAttribute("NodeSelected", true.ToString());
            else
                viewElement.SetAttribute("NodeSelected", false.ToString());
            Id++;
        }

        public static void CreateAppendHostInfoElement(Hostinfo hn, ref XmlElement parentElement, out XmlElement HostInfoElement)
        {
            HostInfoElement = parentElement.OwnerDocument.CreateElement("HostInfo");

            if (hn == null)
                return;

            XmlElement DomainShortEle, DomainFQDNEle, HostnameEle, usernameEle;

            if (!String.IsNullOrEmpty(hn.domainName))
            {
                DomainFQDNEle = HostInfoElement.OwnerDocument.CreateElement("DomainFQDN");
                DomainFQDNEle.InnerXml = hn.domainName;
                HostInfoElement.AppendChild(DomainFQDNEle);
            }
            if (!String.IsNullOrEmpty(hn.shortName))
            {
                DomainShortEle = HostInfoElement.OwnerDocument.CreateElement("DomainShort");
                DomainShortEle.InnerXml = hn.shortName;
                HostInfoElement.AppendChild(DomainShortEle);
            }
            if (!String.IsNullOrEmpty(hn.hostName))
            {
                HostnameEle = HostInfoElement.OwnerDocument.CreateElement("HostName");
                HostnameEle.InnerXml = hn.hostName;
                HostInfoElement.AppendChild(HostnameEle);
            }
            if (!String.IsNullOrEmpty(hn.creds.UserName))
            {
                usernameEle = HostInfoElement.OwnerDocument.CreateElement("UserName");
                usernameEle.InnerXml = hn.creds.UserName;
                HostInfoElement.AppendChild(usernameEle);
            }

            parentElement.AppendChild(HostInfoElement);
        }

        public static void CreateAppendDbConnInfoElement(DbConnInfo DbConnInfo, ref XmlElement parentElement, out XmlElement DbConnInfoElement)
        {
            DbConnInfoElement = parentElement.OwnerDocument.CreateElement("DbConnectionInfo");

            if (DbConnInfo == null)
                return;

            XmlElement DbProviderEle, DbConnStrEle;

            if (!String.IsNullOrEmpty(DbConnInfo.sDbProvider))
            {
                DbProviderEle = DbConnInfoElement.OwnerDocument.CreateElement("DbProviderName");
                DbProviderEle.InnerXml = DbConnInfo.sDbProvider;
                DbConnInfoElement.AppendChild(DbProviderEle);
            }
            if (!String.IsNullOrEmpty(DbConnInfo.sConnString))
            {
                DbConnStrEle = DbConnInfoElement.OwnerDocument.CreateElement("DbConnectionString");
                DbConnStrEle.InnerXml = DbConnInfo.sConnString;
                DbConnInfoElement.AppendChild(DbConnStrEle);
            }

            parentElement.AppendChild(DbConnInfoElement);
        }

        public static void CreateAppendGPOInfoElement(GPObjectInfo gpoInfo, ref XmlElement parentElement, out XmlElement gpoInfoElement)
        {
            gpoInfoElement = parentElement.OwnerDocument.CreateElement("GPOInfo");

            if (gpoInfo == null)
                return;

            XmlElement gpoEle;

            gpoEle = gpoInfoElement.OwnerDocument.CreateElement("GPODistinguishedName");
            gpoEle.InnerXml = gpoInfo.DistinguishedName;
            gpoInfoElement.AppendChild(gpoEle);

            gpoEle = gpoInfoElement.OwnerDocument.CreateElement("GPODisplayName");
            gpoEle.InnerXml = gpoInfo.DisplayName;
            gpoInfoElement.AppendChild(gpoEle);
        }

        public static void DeserializeHostInfo(XmlNode node, ref LACTreeNode pluginNode, string nodepath, ref Hostinfo hn, bool bIsGPOPlugin)
        {
            XmlNode hostnode = node.SelectSingleNode("HostInfo");

            if (hostnode == null) return;

            if (hn == null)
                hn = new Hostinfo();

            foreach (XmlNode child in hostnode.ChildNodes)
            {
                if (child.Name.Trim().Equals("DomainFQDN"))
                {
                    hn.domainName = child.InnerXml;
                    hn.domainControllerName = child.InnerXml;
                    // split the provided name into parts
                    string[] aparts = child.InnerXml.Split(new char[] { '.' });
                    hn.creds.Domain = aparts[0];
                }
                else if (child.Name.Trim().Equals("DomainShort"))
                {
                    hn.shortName = child.InnerXml;
                }
                else if (child.Name.Trim().Equals("HostName"))
                {
                    hn.hostName = child.InnerXml;
                }
                else if (child.Name.Trim().Equals("UserName"))
                {
                    hn.creds.UserName = child.InnerXml;
                }
                else if (bIsGPOPlugin && child.Name.Trim().Equals("GPOInfo"))
                {
                    string sDN = null;
                    string sDisplayname = null;
                    foreach (XmlNode gpoNode in child.ChildNodes)
                    {
                        if (gpoNode.Name.Trim().Equals("GPODistinguishedName"))
                        {
                            sDN = gpoNode.InnerXml;
                        }
                        else if (gpoNode.Name.Trim().Equals("GPODisplayName"))
                        {
                            sDisplayname = gpoNode.InnerXml;
                        }
                    }
                    GPObjectInfo gpoInfo = new GPObjectInfo(null, sDN, sDisplayname);
                    hn.Tag = gpoInfo;
                }
            }
            pluginNode.Tag = hn;
        }

        public static void DeserializeDbConnInfo(XmlNode node, ref LACTreeNode pluginNode, string nodepath, ref DbConnInfo DbConnInfo)
        {
            XmlNode DbConnnode = node.SelectSingleNode("DbConnectionInfo");

            if (DbConnnode == null) return;

            if (DbConnInfo == null)
                DbConnInfo = new DbConnInfo();

            foreach (XmlNode child in DbConnnode.ChildNodes)
            {
                if (child.Name.Trim().Equals("DbProviderName"))
                {
                    DbConnInfo.sDbProvider = child.InnerXml;
                }
                else if (child.Name.Trim().Equals("DbConnectionString"))
                {
                    DbConnInfo.sConnString = child.InnerXml;
                }
            }
            pluginNode.Tag = DbConnInfo;
        }

        /// <summary>
        /// Called when state has changed significantly and we need to reestablish the
        /// UI composition.
        /// </summary>
        public void Reinitialize()
        {
            Logger.Log("Manage.Reinitialize", Logger.manageLogLevel);

            // basically, "reset" the server name
            ManageHost(_hn);
        }

        /// <summary>
        /// Called when we no longer want to manage the current machine.
        /// </summary>
        public void Logout()
        {
            //do nothing
        }

        public void SetStatusMesaage(string message, int panel)
        {

            Logger.Log(String.Format("Manage.SetStatusMesaage: {0}", message), Logger.manageLogLevel);

            try
            {

                StatusBar sbStatus;
                sbStatus = (StatusBar)this.Parent.Parent.Controls["StatusBar1"];
                sbStatus.Panels[panel].Text = message;
                if (panel == 0)
                {
                    sbStatus.Panels[panel].Alignment = HorizontalAlignment.Left;
                }
                else
                {
                    sbStatus.Panels[panel].Alignment = HorizontalAlignment.Right;
                }
            }
            catch (Exception)
            {
            }
        }

        /// <summary>
        /// Calls the parent (via interface, if implemented) to change the cursor used
        /// </summary>
        /// <param name="cursor">Cursor to use</param>
        public void SetCursor(System.Windows.Forms.Cursor cursor)
        {
            Logger.Log("Manage.SetCursor", Logger.manageLogLevel);

            if (sc != null)
            {
                sc.SetHostCursor(cursor);
            }
        }

        #endregion

        // Extra interface implementation
        #region
        /// <summary>
        /// Returns the application directory.
        /// </summary>
        /// <returns></returns>
        public string ApplicationDirectory()
        {
            string  applicationPath = null;

            // get the ApplicationPath
#if !QUARTZ
            RegistryKey hklm = Registry.LocalMachine;
            RegistryKey hklmApp = hklm.OpenSubKey(sKey);
            if (hklmApp != null)
                applicationPath = (string)hklmApp.GetValue("ApplicationPath");
            else
#endif
                applicationPath = Environment.CurrentDirectory;

            return applicationPath;
        }

        public string GetRootDomainName()
        {
            if (_hn == null || _hn.domainName == null || _hn.domainName == "")
            {
                _hn = new Hostinfo();

                try
                {
#if !QUARTZ
                    // get the root domain name
                    string server, protocol, dc, cn;
                    String rootDomainPath = Util.GetLdapRootDomainPath(GetDomainName(), GetCredentials());
                    server = protocol = dc = cn = null;
                    Util.CrackPath(rootDomainPath, out protocol, out server, out cn, out dc);
                    _hn.domainName = Util.DNToDomainName(dc).ToLower();
#endif
                }
                catch (Exception ex)
                {
                    Logger.Log("Unable to determine root domain name for " + GetDomainName() + ". " + ex.Message);
                    _hn.domainName = GetDomainName();
                }
            }

            return _hn.domainName;
        }

        /// <summary>
        /// Called to perform logging. By default, this method logs an "informational" message
        /// of level 1.
        /// </summary>
        /// <param name="sMessage"></param>
        public void Log(string sMessage)
        {
            Logger.Log(sMessage, TraceEventType.Information, Logger.LogLevel.Panic);
        }

        /// <summary>
        /// Called to perform logging.
        /// </summary>
        /// <param name="sMessage">Message to be logged</param>
        /// <param name="tet">Type of message</param>
        /// <param name="nLevel">Some indicator of verbosity (1=low).</param>
        public void Log(string sMessage, TraceEventType tet, int nLevel)
        {
            Logger.Log(sMessage, tet, nLevel);
        }

        public string GetDomainName()
        {
            if (_hn == null || _hn.domainName == null || _hn.domainName == "")
            {
                _hn = new Hostinfo();

                if (sLDAPPath != null && sLDAPPath != "")
                {
#if !QUARTZ
                    string protocol, server, cn, dc;
                    Util.CrackPath(sLDAPPath, out protocol, out server, out cn, out dc);
                    _hn.domainName = Util.DNToDomainName(dc);
#endif
                }
            }

            return _hn.domainName;
        }

        public NetworkCredential GetCredentials()
        {
            if (_hn == null)
                return null;
            return _hn.creds;
        }

#if !QUARTZ
        public NetworkCredentialCache NetworkCredentialCache
        {
            get
            {
                return credCache;
            }
            set
            {
                credCache = value;
            }
        }
#endif

        public string GetLDAPPath()
        {
            return sLDAPPath;
        }


        /// <summary>
        /// Returns the logs directory.
        /// </summary>
        /// <returns>logs directory</returns>
        public string LogsDirectory()
        {
            return Logger.Filename;
        }

        /// <summary>
        /// Returns the directory containing various configuration files.
        /// </summary>
        /// <returns>config directory</returns>
        public string ResourcesDirectory()
        {
            string applicationPath = null;

            // get the ApplicationPath
#if !QUARTZ
            RegistryKey hklm = Registry.LocalMachine;
            RegistryKey hklmApp = hklm.OpenSubKey(sKey);
            if (hklmApp != null)
                applicationPath = (string)hklmApp.GetValue("ApplicationPath");
            else
#endif
                applicationPath = Environment.CurrentDirectory;

            return applicationPath + "\\Resources";
        }




        #endregion


    }

    // used to save/restore state
    public class FrameState
    {
        public bool bShowMaximized;
        public bool bShowStatusBar;
        public bool bShowNavigationBar;
        public bool bShowTreeControl;
        public bool bReadOnly;
        public int top, left;
        public int bottom, right;

        public FrameState()
        {
        }
    }
}
