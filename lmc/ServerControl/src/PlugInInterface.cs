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
using System.Windows.Forms;
using System.Xml;
using Likewise.LMC.Utilities;
using System.Net;
#if !QUARTZ
using Centeris.Likewise.Auth;
#endif
using System.Collections.Generic;


namespace Likewise.LMC.ServerControl
{
    /// <summary>
    /// Defines the methods that must be provided by a IPlugIn
    /// </summary>
    public interface IPlugIn
    {
        string GetName();
        string GetDescription();
        string GetPluginDllName();
        IContextType GetContextType();
        void Initialize(IPlugInContainer container);
        void SetContext(IContext ctx);
        IContext GetContext();
        LACTreeNode GetPlugInNode();
        void EnumChildren(LACTreeNode parentNode);
        void SetCursor(System.Windows.Forms.Cursor cursor);
        void SetSingleSignOn(bool useSingleSignOn);
        ContextMenu GetTreeContextMenu(LACTreeNode nodeClicked);
        void AddExtPlugin(IPlugIn extPlugin);
        bool PluginSelected();

        void SerializePluginInfo(LACTreeNode pluginNode, ref int Id, out XmlElement viewElement, XmlElement ViewsNode, TreeNode SelectedNode);
        void DeserializePluginInfo(XmlNode node, ref LACTreeNode pluginnode, string nodepath);

    }

    /// <summary>
    /// This interface must be implemented by any "page" controls
    /// that need a back reference to their plug in
    /// </summary>
    public interface IPlugInPage
    {
        // called to initialize a page with its back references
        void SetPlugInInfo(IPlugInContainer container, IPlugIn pi, LACTreeNode treeNode, LWTreeView lmcTreeview, CServerControl sc);

        void SetPlugInInfo(IPlugInContainer container, IPlugIn pi);

        // called when a page is about to be "closed" (un-displayed)
        void OnClose();

        ContextMenu GetPlugInContextMenu();

        void ShowActionPane(bool bShowActionPane);

        void SetContext(IContext ctx);
        IContext GetContext();

        void Clear();

        void RefreshPluginPage();

        void PropertyPageProxy(object o);
    }

    /// <summary>
    /// Defines the methods that must be provided to a IPlugIn
    /// </summary>
    public interface IPlugInContainer
    {
        bool GetTargetMachineInfo(IPlugIn requestor, IContext ctx, uint fieldsRequested);

        // display prompts, notes and error forms
        DialogResult Prompt(string sMessage, MessageBoxButtons buttons);
        void Note(string sMessage);
        DialogResult ShowError(string sMessage, MessageBoxButtons buttons);
        void ShowError(string sMessage);
        DialogResult ShowError(Control ctl, string sMessage);
        DialogResult ShowMessage(string sMessage, MessageBoxButtons buttons, MessageBoxIcon icon);
        DialogResult ShowMessage(string sMessage, MessageBoxButtons buttons);
        void ShowMessage(string sMessage);

        // used to save/load persistent state
        void SaveState(string sName, string sData);
        void SaveState(string sName, byte[] abData);
        object LoadState(string sName);
        void RemoveState(string sName);
        List<string> LoadStateKeyContains(string sFolderName, string sSubName);

        // This methods tells the container that state has changed significantly and that it
        // should re-query all the IPlugIns for proper tab state.
        void Reinitialize();

        // Tells the container to stop managing the current machine and return to a non-machine specific UI
        void Logout();

        void SetRootParentNode(LACTreeNode tn);

        void SetLWTreeView(LWTreeView tv);

        void SetStatusMesaage(string message,int panel);

        LACTreeNode GetRootParentNode();

        void ShowControl(LACTreeNode node);

        // finds a "well known object"
        object FindWKO(string sName);

        void SetCursor(System.Windows.Forms.Cursor cursor);

        #region
       // Extra interface definition to be able to load Likewise iConsole as plugin

        string ApplicationDirectory();

        string GetRootDomainName();

        void Log(string sMessage);

        void Log(string sMessage, System.Diagnostics.TraceEventType traceEventType, int nLeven);

        // returns the current domain name
        string GetDomainName();

        NetworkCredential GetCredentials();

#if !QUARTZ
        NetworkCredentialCache NetworkCredentialCache
        {
            get;
            set;
        }
#endif

        string GetLDAPPath();

        string LogsDirectory();

        string ResourcesDirectory();


        #endregion
    }

    /// <summary>
    /// Defines methods that must be provided by MPPage
    /// </summary>
    public interface IMPPageInterface
    {
        /// <summary>
        /// Set or get the parent MPContainer object.
        /// </summary>
        EditDialog ParentContainer
        {
            set;
            get;
        }

        /// <summary>
        /// Set or get the parent IPlugInContainer.
        /// </summary>
        IPlugInContainer IPlugInContainer
        {
            set;
            get;
        }

        /// <summary>
        /// Used to check if there is an input error on the page.
        /// </summary>
        /// <returns>true if there is an inputer error, false otherwise</returns>
        bool HasError();

        /// <summary>
        /// Get the page ID.
        /// </summary>
        String PageID
        {
            get;
        }
    }

    /// <summary>
    /// This simple interface can be used to allow an object to fire a handler
    /// whenever the objects data is modified. Used by the MPPageContainer class.
    /// </summary>
    public interface IDataModifiedNotify
    {
        /// <summary>
        /// Set the handler to call when objects data is modified.
        /// </summary>
        /// <param name="eh"></param>
        void AddDataModifiedHandler(EventHandler eh);
    }

    /// <summary>
    /// Methods that must be implemented by the container that hosts
    /// server control
    /// </summary>
    public interface ISCHost
    {
        void SetCursor(System.Windows.Forms.Cursor cursor);
    }


    /// <summary>
    /// Defines methods that must be provided by the containing application
    /// </summary>
    public interface MainApp
    {
        // gets the credentials in use
        NetworkCredential GetCredentials();

#if !QUARTZ
        // get the credential cache
        NetworkCredentialCache NetworkCredentialCache
        {
            get;
            set;
        }
#endif

        string[] AppArguments
        {
            get;
        }

        // Sets the state of navigation widgets
        void SetNavState(bool bCanForward, bool bCanBack, bool bCanRefresh);

        // Displays a prompt
        DialogResult Prompt(string sMessage, MessageBoxButtons buttons);

        // Display credential prompt for the given domain
        bool DomainPrompt(string sDomain, out string sUsername, out string sPassword);

        // Displays a note
        void Note(string sMessage);

        // Displays an error message
        DialogResult ShowError(string sMessage, MessageBoxButtons buttons);

        // Writes an entry to the log file
        void Log(string sMessage, System.Diagnostics.TraceEventType tet, int nLevel);

        // Returns the application directory
        string ApplicationDirectory();

        // Returns the log directory
        string LogsDirectory();

        // Returns the resources directory
        string ResourcesDirectory();

        // Returns the documentation directory
        string DocumentationDirectory();

        // Prompt user for domain name and credentials
        void ManageDomainPrompt();

        // Acts as if the application was re-started
        // Pass false not to show the welcome panel if otherwise it would be shown
        void RefreshApp(bool bNoWelcomePanel);

        // Show a help page for the topic
        void ShowHelpPage(Control parent, string helpKeyword);

    }

     /// <summary>
    /// Defines methods that must be provided by the containing application
    /// </summary>
    public interface IContext
    {
        // get the connection state
        bool IsConnectionSuccess
        {
            get;
            set;
        }

        // get the Tag for object caching
        object Tag
        {
            get;
            set;
        }
    }

    public enum IContextType
    {
        Rootinfo,
        Hostinfo,
        DbConninfo
    };
}
