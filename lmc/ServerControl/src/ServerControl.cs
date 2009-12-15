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
using System.Collections;
using System.Drawing;
using System.Net;
using System.Net.Sockets;
using System.Windows.Forms;
using Likewise.LMC.ServerControl.Properties;
using Likewise.LMC.Utilities;

namespace Likewise.LMC.ServerControl
{
    /// <summary>
    /// This class is used to encapsulate all Likewise
    /// server management functionality. It is dropped into
    /// the Likewise Console in its main body panel. It knows
    /// how to display a Welcome page that prompts for a server
    /// to manage. It can also be told to manage a specific
    /// machine.
    /// </summary>
    public partial class CServerControl : UserControl
    {
        #region Constants

        // number of microseconds to wait for a socket to be openable. 250 ms.
        private const int usSocketWait = 250*1000;

        #endregion

        #region Class data

        // the currently shown control
        private Control controlShown = null;

        // the cached Manage object (a singleton)
        private Manage controlManage = null;

        // cached application directory
        private string sAppDirectory = "";

        private StandardPage.ViewStyle currentViewStyle = StandardPage.ViewStyle.DETAIL_VIEW;

        #endregion

        #region Constructor

        #region Accessor
        public Manage ControlManage
        {
            get
            {
                return controlManage;
            }
        }
        #endregion

        public CServerControl()
        {
            InitializeComponent();

            // create the Manage control only once
            controlManage = new Manage(this);
            controlManage.Dock = DockStyle.Fill;
        }

        #endregion

        #region Public Methods

        public void ShowManage()
        {
            if (controlShown != controlManage)
            {
                CloseShown();

                controlManage.BringToFront();
                controlManage.Dock = DockStyle.Fill;
                Controls.Add(controlManage);

                controlManage.Show();
                controlManage.Select();

                // force a repaint since we're having some
                // trouble getting the navigation bar to initial
                // draw correctly. This is probably because
                // the control formatting is happening before
                // the window is actually shown.
                controlManage.Invalidate(true);

                controlShown = controlManage;
            }
        }

        /// <summary>
        /// Shows the Manage form and directs it at a particular
        /// machine.
        /// </summary>
        /// <param name="sHostname">The machine to manage</param>
        /// <param name="viewStyle">The style of list view to use, if a listview is used</param>
        /// <returns>TRUE if was able to manage the given machine. FALSE if a
        ///          network or credential error occured. </returns>
        public bool ManageHost(string sHostname, StandardPage.ViewStyle viewStyle)
        {

            // first, normalize the incoming hostname
            Hostinfo hn = new Hostinfo(sHostname);

            // now, set the host type
            if (!CheckHostInfo(hn))
            {

                string sMsg = string.Format(Resources.Error_InvalidComputerType, sHostname);
                ShowError(sMsg, MessageBoxButtons.OK);
                return false;
            }

            // do this first so that we can do any slow stuff before we start
            // changing pages
            if (!controlManage.ManageHost(hn))
                return false;

            SetViewStyle(viewStyle);

            // show the management control
            ShowManage();

            string str = String.Format(Resources.Connected_As,
                                       hn.creds.UserName,
                                       hn.hostName);

            controlManage.SetStatusMesaage(str, 1);

            return true;
        }

        /// <summary>
        /// Shows the Manage form and directs it at a particular
        /// machine.
        /// </summary>
        /// <param name="sHostname">The machine to manage</param>
        /// <returns>TRUE if was able to manage the given machine. FALSE if a
        ///          network or credential error occured. </returns>
        public bool ManageHost(string sHostname)
        {
            return ManageHost(sHostname, currentViewStyle);
        }

        /// <summary>
        /// Called by the container of ServerControl to initialize the ServerControl
        /// persistent store. It is up to the container (e.g. CESM) to figure out how
        /// IT will persist this store (e.g. in a file or in the registry).
        /// </summary>
        /// <param name="ht"></param>
        public void SetPersistedState(Hashtable ht)
        {
            // pass it on to the Manage object
            controlManage.PersistedState = ht;
        }

        public void SetIPlugInRootNode(LACTreeNode tn)
        {
            controlManage.SetRootParentNode(tn);
        }

        public void ShowControl(LACTreeNode node)
        {
            controlManage.ShowControl(node);
        }

        public void SetLWTreeView(LWTreeView tv)
        {
            controlManage.SetLWTreeView(tv);

        }

        /// <summary>
        /// This must be called after SetLWTreeView has been called, otherwise it will return null.
        /// </summary>
        /// <returns></returns>
        public LACTreeNode LoadPlugins()
        {
            LACTreeNode rootNode = controlManage.LoadPlugins();

            ShowManage();

            return rootNode;

        }

        /// <summary>
        /// Called by the container of ServerControl to retrieve a Hashtable representing
        /// the ServerControl persistent store. The container (e.g. CESM) uses this method
        /// to retrieve the store and save it somewhere (e.g. in a file or in the registry).
        /// </summary>
        /// <returns></returns>
        public Hashtable GetPersistentState()
        {
            // get from the Manage object
            return controlManage.PersistedState;
        }

        /// <summary>
        /// Refreshes displayed information
        /// </summary>
        public override void Refresh()
        {
            if (controlShown == controlManage)
            {
                controlManage.Refresh();
            }
            else
                base.Refresh();
        }

        /// <summary>
        /// Called when the server control is being closed (e.g. on shutdown)
        /// </summary>
        public void OnClose()
        {
            controlManage.OnClose();
        }

        /// <summary>
        /// Displays prompt in standard Likewise form
        /// </summary>
        /// <param name="sMessage">Message to show</param>
        /// <param name="buttons">Buttons to display</param>
        /// <returns>The selected button</returns>
        public DialogResult Prompt(string sMessage, MessageBoxButtons buttons)
        {
            // use standard MessageBox for now
            return MessageBox.Show(Logger.limitLineLength(sMessage), Resources.Caption_Console, buttons, MessageBoxIcon.Question);
        }

        /// <summary>
        /// Displays an informatory message in standard Likewise form
        /// </summary>
        /// <param name="sMessage"></param>
        public void Note(string sMessage)
        {
            // use standard MessageBox for now
            MessageBox.Show(
                Logger.limitLineLength(sMessage),
                Resources.Caption_Console,
                MessageBoxButtons.OK,
                MessageBoxIcon.Information);
        }

        /// <summary>
        /// Called to display a standard error.
        /// </summary>
        /// <param name="sMessage">The message to show</param>
        /// <param name="buttons"></param>
        /// <returns></returns>
        public DialogResult ShowError(string sMessage, MessageBoxButtons buttons)
        {
            DialogResult result;

            // use standard MessageBox for now
            result = MessageBox.Show(
                    Logger.limitLineLength(sMessage),
                    Resources.Caption_Console, buttons,
                    MessageBoxIcon.Error);

            return result;
        }

        /// <summary>
        /// Called to display a message.
        /// </summary>
        /// <param name="sMessage">The message to show</param>
        /// <param name="buttons"></param>
        /// <returns></returns>
        public DialogResult ShowMessage(string sMessage, MessageBoxButtons buttons, MessageBoxIcon icon)
        {
            DialogResult result;

            // use standard MessageBox for now
            result = MessageBox.Show(
                Logger.limitLineLength(sMessage),
                Resources.Caption_Console,
                buttons,
                icon);

            return result;
        }

        public DialogResult ShowError(string sMessage)
        {
            DialogResult result;

            // use standard MessageBox for now
            Logger.Log(String.Format(
                "ServerControl.ShowError({0})",
                sMessage),
                Logger.LogLevel.Error);

            result = MessageBox.Show(
                    Logger.limitLineLength(sMessage),
                    Resources.Caption_Console,
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error);

            return result;
        }

        public DialogResult ShowError(Control ctl,string sMessage)
        {
            DialogResult result;

            // use standard MessageBox for now
            Logger.Log(String.Format(
                "ServerControl.ShowError({0})",
                sMessage),
                Logger.LogLevel.Error);

            result = MessageBox.Show(ctl,
                    Logger.limitLineLength(sMessage),
                    Resources.Caption_Console,
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error);

            return result;
        }


        public void SetViewStyle(StandardPage.ViewStyle view)
        {
            if (currentViewStyle != view)
            {
                currentViewStyle = view;
                controlManage.SetViewStyle(view);
            }
        }


        public void ShowActionPane(bool bShowActionPane)
        {
            controlManage.ShowActionPane(bShowActionPane);
        }

        /// <summary>
        /// Calls the parent (via interface, if implemented) to change the cursor used
        /// </summary>
        /// <param name="cursor">Cursor to use</param>
        public void SetHostCursor(System.Windows.Forms.Cursor cursor)
        {
            Logger.Log("ServerControl.SetHostCursor", Logger.manageLogLevel);

            if (Parent != null && Parent is ISCHost)
            {
                ISCHost sch = (ISCHost)Parent;
                sch.SetCursor(cursor);
            }
        }

        #endregion

        #region Property Accessors

        public string ApplicationDirectory
        {
            get { return sAppDirectory; }
            set { sAppDirectory = value; }
        }

        public Manage manage
        {
            get
            {
                return controlManage;
            }
        }

        #endregion

        #region Helper Functions

        /// <summary>
        /// Closes the currently shown control
        /// </summary>
        private void CloseShown()
        {
            if (controlShown != null)
            {
                controlShown.Hide();
                Controls.Remove(controlShown);
                controlShown = null;

                Update();
            }
        }

        /// <summary>
        /// Shows a control centered in the ServerControl window
        /// </summary>
        /// <param name="c">The control to show</param>
        private void ShowCentered(Control c)
        {
            // get sizes
            Size szUs = Size;
            Size szC = c.Size;

            // calculate position
            int x = (szUs.Width - szC.Width)/2;
            int y = (szUs.Height - szC.Height)/2;

            if (x < 0)
                x = 0;
            if (y < 0)
                y = 0;

            // set, add and show
            controlShown = c;
            Controls.Add(c);
            c.Location = new Point(x, y);
            c.Show();
            ActiveControl = c;
        }

        /// <summary>
        /// Determines whether a machine is a Windows machine, Linux, etc.
        /// </summary>
        /// <param name="hn">The machine to test</param>
        /// <returns>The machine type</returns>
        private static bool CheckHostInfo(Hostinfo hn)
        {
            // First, let's get an IP address for it.
            try
            {
                // if we can't resolve the name, give up
                // TODO: do this async so that we can abort it
                IPAddress[] arips = Dns.GetHostAddresses(hn.hostName);
                if (arips.Length == 0)
                    return false;

                IPAddress addr = arips[0];

                // set this up front for later
                hn.IsSmbAvailable = checkPort(addr, 445);

                // see if we can open up port 22
                hn.IsSSHAvailable = checkPort(addr, 22);

                // check netbios port
                hn.IsNetBiosAvailable = checkPort(addr, 139);
            }
            catch (Exception)
            {
                return false;
            }

            return true;
        }

        /// <summary>
        /// Determines whether a port is available on a given machine
        /// </summary>
        /// <param name="addr"></param>
        /// <param name="nPort"></param>
        /// <returns></returns>
        private static bool checkPort(IPAddress addr, int nPort)
        {
            try
            {
                // use the first address
                Socket sock = new Socket(AddressFamily.InterNetwork,
                                         SocketType.Stream,
                                         ProtocolType.Tcp);

                // set blocking to false
                sock.Blocking = false;

                try
                {
                    sock.Connect(addr, nPort);
                }
                catch (SocketException sex)
                {
                    if (sex.SocketErrorCode != SocketError.WouldBlock)
                    {
                        sock.Close();
                        return false;
                    }
                }

                // now, check status, but wait only yea long
                bool bAvailable = sock.Poll(usSocketWait, SelectMode.SelectWrite);
                sock.Close();

                return bAvailable;
            }
            catch (Exception)
            {
                return false;
            }
        }

        #endregion
    }
}