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
using System.Drawing;
using System.Windows.Forms;
using System.Collections.Generic;
using Likewise.LMC.Utilities;
using Likewise.LMC.Plugins.LUG.Properties;
using Likewise.LMC.ServerControl;
using System.Diagnostics;
using System.Net;
using Likewise.LMC.NETAPI;
using Likewise.LMC.Netlogon;
using Likewise.LMC.Plugins.LUG.src;

namespace Likewise.LMC.Plugins.LUG
{

public partial class LUGPage : StandardPage
{
    
    #region class data
    
    private ListViewColumnSorter lvwColumnSorter;
    private UInt32 PageSize = 50;
    private UInt32 nOffset = 0;
    public ListViewItem[] lvArr;
    
    public enum LUGStatusIcon
    {
        User,
        UserDisabled,
        Group
    }

    public delegate uint EnumLUG(string serverName, int resumeHandle, out LUGAPI.LUGEnumStatus enumStatus);

    public delegate uint DeleteLUG(string servername, string username);

    public delegate uint RenameLUG(string servername, string oldname, string newname);

    private delegate void AddRangeDelegate(ListViewItem[] range);
    private delegate void ComboAddRangeDelegate(string[] range);
    
    private delegate void AutoResizeDelegate();
    
    private LUGAPI.LUGType memberType = LUGAPI.LUGType.Undefined;
    private string memberTypeString = "";
    
    private EnumLUG enumLUG = null;
    private DeleteLUG deleteLUG = null;
    private RenameLUG renameLUG = null;
    
    private LUGStatusIcon imageLUG = LUGStatusIcon.User;
    private LUGStatusIcon imageLUGDisabled = LUGStatusIcon.UserDisabled;
    
    private int numColumns = 0;
    
    private string[] columnLabels = null;
    
    private ColumnHeader[] columnHeaders = null;
    
    //pixels to use to give a visible margin between columns
    private int columnMargin = 10;
    
    #endregion
    
    #region Constructor
    
    public LUGPage()
    {
        
    }

    public LUGPage(LUGAPI.LUGType type)
    {
        memberType = type;

        InitializeComponent();
        this.lvLUGBETA.Scrollable = Configurations.useListScrolling;

        // Create an instance of a ListView column sorter and assign it
        // to the ListView control.
        lvwColumnSorter = new ListViewColumnSorter();
        this.lvLUGBETA.ListViewItemSorter = lvwColumnSorter;

        if (type == LUGAPI.LUGType.Dummy)
        {
            lvLUGBETA.SuspendLayout();
            memberTypeString = "Blank";
            enumLUG = LUGAPI.NetEnumUsers;
            this.lblCaption.Text = "Select \"Users\" or \"Groups\" to view entries";
            columnLabels = new string[] { "Name" };

            lvLUGBETA.GridLines = false;
            lvLUGBETA.ResumeLayout();
        }
        else if (type == LUGAPI.LUGType.User)
        {
            //The following controls are added here rather than in InitializeComponent,
            //so that User and Group pages can have different sets of controls.

            memberTypeString = "User";
            enumLUG = LUGAPI.NetEnumUsers;
            deleteLUG = LUGAPI.NetDeleteUser;
            renameLUG = LUGAPI.NetRenameUser;

            columnLabels = new string[] { "", "Status", "Username", "Full Name", "Description" };

            lvLUGBETA.GridLines = false;
        }
        else if (type == LUGAPI.LUGType.Group)
        {
            //The following controls are added here rather than in InitializeComponent,
            //so that User and Group pages can have different sets of controls.
            memberTypeString = "Group";
            enumLUG = LUGAPI.NetEnumGroups;
            deleteLUG = LUGAPI.NetDeleteGroup;
            renameLUG = LUGAPI.NetRenameGroup;

            imageLUG = LUGStatusIcon.Group;
            imageLUGDisabled = LUGStatusIcon.Group;

            columnLabels = new string[] { "", "Group Name", "Description" };

            lvLUGBETA.GridLines = false;
        }
        else
        {
            throw new ArgumentException("LUGPage must be initialized as either LUGType.Group, LUGType.User, or LUGType.Dummy, but was not");
        }
        numColumns = columnLabels.Length;

        columnHeaders = new ColumnHeader[numColumns];

        for (int i = 0; i < numColumns; i++)
        {
            columnHeaders[i] = new ColumnHeader();
            columnHeaders[i].Text = columnLabels[i];
        }

        this.lvLUGBETA.Columns.AddRange(columnHeaders);
    }
    
    #endregion
    
    #region IPlugInPage Members
    
    public override void SetPlugInInfo(IPlugInContainer container, IPlugIn pi, LACTreeNode treeNode, LWTreeView lcmtreeview,CServerControl sc)
    {
        if (memberType == LUGAPI.LUGType.Dummy)
        {
            base.SetPlugInInfo(container, pi, treeNode, lmctreeview, sc);
            
            bEnableActionMenu = false;

            Refresh();
            return;
        }
        this.lmctreeview = lcmtreeview;

        lblCaption.Text = String.Format(lblCaption.Text, memberTypeString + "s", "{0}");       

        // Let the base do its thing
        base.SetPlugInInfo(container, pi, treeNode, lmctreeview, sc);
        Refresh();
    }
    
    
    public override void SetContext(IContext ctx)
    {
        Hostinfo hn = ctx as Hostinfo;

        if (Hostinfo.HasCreds(hn))
        {
            Session.EnsureNullSession(hn.hostName, hn.creds);
        }
        base.SetContext(hn);
    }
    
    public override void Clear()
    {
        if (lvLUGBETA != null)
        {
            lvLUGBETA.Clear();
        }
        
    }
    
    #endregion
    
    #region override functions
    public override void Refresh()
    {
        Logger.Log("LUGPage.Refresh()", Logger.manageLogLevel);
        
        PopulatePage();
        base.Refresh();
    }
    
    #endregion
    
    #region member functions
    
    public bool ChangePassword(string password)
    {
        Logger.Log(String.Format("LUGPage.ChangePassowrd({0}) called", password), Logger.netAPILogLevel);
        
        if (lvLUGBETA.SelectedItems.Count != 1)
        {
            return false;
        }
        
        bool retValue = true;
        ListViewItem item = lvLUGBETA.SelectedItems[0];
        string user = item.SubItems[2].Text;
        Hostinfo hn = ctx as Hostinfo;
        
        try
        {
            retValue = !Convert.ToBoolean(LUGAPI.NetChangePassword(hn.hostName, user, password));
        }
        catch (Exception e)
        {
            retValue = false;
            Logger.LogException("LUGPage.ChangePassword", e);
        }
        return retValue;
    }
    
    public bool EditLUG(object o)
    {
        bool retValue = false;
        
        
        try
        {
            string comment = "";
            
            if (memberType == LUGAPI.LUGType.User)
            {
                UserProperties up = ((UserPropertiesDlg)o).userProperties;
                Hostinfo hn = ctx as Hostinfo;
                
                string fullname = "";
                uint flags = 0;

                LUGAPI.LUGInfo userInfo;
                LUGAPI.NetGetUserInfo(hn.hostName, up.userName, out userInfo);

                flags = userInfo.flags;
                
                uint old_flags = userInfo.flags;
                
                Logger.Log(String.Format("LUGPage.EditLUG(user): flags: expired:{0}, cant_change:{1}, never_expires:{2}, disabled:{3}, lockout:{4}, old_flags: {5}",
                up.mustChange,
                up.cannotChange,
                up.neverExpires,
                up.isDisabled,
                up.isLocked,
                LUGAPI.flagDescription(flags)), Logger.netAPILogLevel);
                
                // Clear flags we are interested in...
                flags &= ~(LUGAPI.UF_PASSWORD_EXPIRED);
                flags &= ~(LUGAPI.UF_PASSWD_CANT_CHANGE);
                flags &= ~(LUGAPI.UF_DONT_EXPIRE_PASSWD);
                flags &= ~(LUGAPI.UF_ACCOUNTDISABLE);
                flags &= ~(LUGAPI.UF_LOCKOUT);
                
                // Set flags per user request...
                flags |= up.mustChange ? LUGAPI.UF_PASSWORD_EXPIRED : 0;
                flags |= up.cannotChange ? LUGAPI.UF_PASSWD_CANT_CHANGE : 0;
                flags |= up.isDisabled ? LUGAPI.UF_ACCOUNTDISABLE : 0;
                flags |= up.neverExpires ? LUGAPI.UF_DONT_EXPIRE_PASSWD : 0;
                flags |= up.isLocked ? LUGAPI.UF_LOCKOUT : 0;
                
                Logger.Log(String.Format("LUGPage.EditLUG(user) new_flags: {0}",
                LUGAPI.flagDescription(flags)), Logger.netAPILogLevel);
                
                if (fullname != null && fullname != up.fullName)
                {
                    LUGAPI.NetEditUserFullName(hn.hostName, up.userName, up.fullName);
                }
                
                if (comment != null && comment != up.description)
                {
                    LUGAPI.NetEditUserDescription(hn.hostName, up.userName, up.description);
                }
                
                if (old_flags != flags)
                {
                    LUGAPI.NetEditUserFlags(hn.hostName, up.userName, flags);
                }
                
                retValue = true;
                
                
                Refresh();
                
            }
            else if (memberType == LUGAPI.LUGType.Group)
            {
                
                GroupPropertiesDlg gpDlg = (GroupPropertiesDlg)o;
                Hostinfo hn = ctx as Hostinfo;
                
                if (gpDlg != null)
                {
                    string description;

                    LUGAPI.NetGetGroupInfo(
                        hn.hostName, 
                        gpDlg.GroupName,
                        out description
                        );
                    
                    if (description == null || description != gpDlg.Description)
                    {
                        LUGAPI.NetEditGroupDescription(hn.hostName, gpDlg.GroupName, gpDlg.Description);
                        Refresh();
                    }
                }
            }
        }
        catch (Exception e)
        {
            retValue = false;
            Logger.LogException("EditLUG", e);
        }
        
        return retValue;
    }
    
    public bool AddLUG(object o)
    {
        bool retValue = false;
        
        try
        {
            if (memberType == LUGAPI.LUGType.User)
            {
                
                NewUserDlg nu = (NewUserDlg)o;
                if (nu != null)
                {
                    
                    Logger.Log(String.Format("LUGPage.AddLUG mustChange={0}, cannotChange={1}, isDisabled={2}, neverExpires={3}",
                    nu.MustChange, nu.CannotChange, nu.IsDisabled, nu.NeverExpires));
                    
                    uint flags = LUGAPI.UF_SCRIPT;
                    Hostinfo hn = ctx as Hostinfo;
                    
                    flags |= nu.MustChange ? LUGAPI.UF_PASSWORD_EXPIRED : 0;
                    
                    flags |= nu.CannotChange ? LUGAPI.UF_PASSWD_CANT_CHANGE : 0;
                    
                    flags |= nu.IsDisabled ? LUGAPI.UF_ACCOUNTDISABLE : 0;
                    
                    flags |= nu.NeverExpires ? LUGAPI.UF_DONT_EXPIRE_PASSWD : 0;

                    if (!Convert.ToBoolean(LUGAPI.NetAddUser(
                        hn.hostName,
                        nu.User,
                        nu.Password,
                        nu.FullName,
                        nu.Description,
                        flags)))
                    {
                        string[] row = new string[] { "", nu.IsDisabled ? "Disabled" : "", nu.User, nu.FullName, nu.Description };
                        InsertLUGToListView(row);
                        return true;
                    }
                }
            }
            else if (memberType == LUGAPI.LUGType.Group)
            {
                NewGroupDlg ng = (NewGroupDlg)o;
                Hostinfo hn = ctx as Hostinfo;
                if (ng != null)
                {
                    if (!Convert.ToBoolean(LUGAPI.NetAddGroup(
                        hn.hostName,
                        ng.GroupName,
                        ng.Description)))
                    {
                        string[] row = new string[] { "", ng.GroupName, ng.Description };
                        InsertLUGToListView(row);
                        return true;
                    }
                }
            }
        }
        catch (Exception)
        {
            retValue = false;
        }
        return retValue;
    }
    
    public string GetDomain()
    {
        Hostinfo hn = ctx as Hostinfo;
        return hn.domainName;
    }
    
    #endregion
    
    #region helper functions
    
    private void PopulatePage()
    {
        try
        {
            if (lvLUGBETA.Items.Count != 0)
            {
                lvLUGBETA.Items.Clear();
            }
            numColumns = lvLUGBETA.Columns.Count;
            if (memberType == LUGAPI.LUGType.Dummy)
            {               
                List<ListViewItem> nodelist = new List<ListViewItem>();
                foreach (LACTreeNode node in treeNode.Nodes)
                {
                    ListViewItem lvitem = new ListViewItem(new string[]
                    {
                        node.Text
                    }
                    );
                    if (node.NodeType == typeof(LocalUserPage))
                    {
                        lvitem.ImageIndex = 0;
                    }
                    else if (node.NodeType == typeof(LocalGroupPage))
                    {
                        lvitem.ImageIndex = 2;
                    }
                    lvitem.Tag = node;                    
                    nodelist.Add(lvitem);
                }
                ListViewItem[] lvItems = new ListViewItem[nodelist.Count];
                nodelist.CopyTo(lvItems);
                lvLUGBETA.Items.AddRange(lvItems);                 
                return;
            }
            else
            {
                if (ctx == null)
                {
                    ctx = new Hostinfo();
                }
                Hostinfo hn = ctx as Hostinfo;

                //GetMachineInfo(); 
                if (lblCaption.Text.IndexOf("{0}") >= 0)
                {
                    if (!String.IsNullOrEmpty(hn.hostName))
                    {
                        lblCaption.Text = string.Format(lblCaption.Text, hn.hostName);
                    }
                    else
                    {
                        lblCaption.Text = string.Format(lblCaption.Text, System.Environment.MachineName);
                    }
                }

                try
                {
                    //container.SetCursor(Cursors.WaitCursor);

                    LUGAPI.LUGEnumStatus enumStatus;
                    enumLUG(hn.hostName, 0, out enumStatus);
                    enumLUGCallback(enumStatus);
                }
                catch (AuthSessionException e)
                {
                    Logger.LogException("LUGPage.GetMachineInfo", e);
                }
                catch (Exception e)
                {
                    Logger.LogException("LUGPage.GetMachineInfo", e);
                }
            }
        }
        
        catch (Exception e)
        {
            String sMsg = "Unknown LUG Type!";
            if (memberType == LUGAPI.LUGType.User)
            {
                sMsg = string.Format(Resources.Error_CannotRetrieveUserInfo, e.Message);
            }
            else if (memberType == LUGAPI.LUGType.Group)
            {
                sMsg = string.Format(Resources.Error_CannotRetrieveGroupInfo, e.Message);
            }
            
            container.ShowError(sMsg);
            
            Logger.Log(String.Format("LUGPage.PopulatePage Exception: {0}", e));
            
        }
    }
    
    // <summary>
    // This method queries the indicated server and sets up various data in the Hostinfo structure.
    // It also establishes a set of working credentials for the machine
    // </summary>
    // <returns>FALSE if unable to manage the machine</returns>
    private void GetMachineInfo()
    {
        Hostinfo hn = ctx as Hostinfo;
        Logger.Log(String.Format(
        "GetMachineInfo called for LUG Plugin.  hn: {0}",
        !Hostinfo.HasCreds(hn) ? "empty" : hn.hostName),
        Logger.manageLogLevel);
        
        Hostinfo defaultHostInfo = null;
        
        //if Hostinfo is empty, attempt to retrieve details using kerberos
        if (!Hostinfo.HasCreds(hn))
        {
            defaultHostInfo = new Hostinfo();
        }
        else
        {
            defaultHostInfo = hn.Clone();
        }        
        
        //check if we are joined to a domain -- if not, use simple bind
        uint requestedFields = (uint)Hostinfo.FieldBitmaskBits.FQ_HOSTNAME;
        
        if (!this.container.GetTargetMachineInfo(this.pi, defaultHostInfo, requestedFields))
        {
            Logger.Log(
            "Could find information about target machine",
            Logger.netAPILogLevel);
            hn = null;
        }
        
        if (hn.creds != null && String.IsNullOrEmpty(hn.creds.MachineName))
        {
            hn.creds.MachineName = hn.hostName;
        }                
    }
    
    private void enumLUGCallback(LUGAPI.LUGEnumStatus enumStatus)
    {
        if (!validHandle)
        {
            return;
        }
        if (enumStatus.entries != null && enumStatus.entries.Count > 0)
        {
            if (Convert.ToInt32(enumStatus.entries.Count - nOffset) > PageSize)
            {
                lvArr = new ListViewItem[PageSize];
            }
            else
            {
                lvArr = new ListViewItem[Convert.ToInt32(enumStatus.entries.Count - nOffset)];
            }
            
            for (int i = 0, offset =Convert.ToInt32(nOffset); i < PageSize ; i++, offset++)
            {
                if (offset == Convert.ToInt32(enumStatus.entries.Count))
                {
                    break;
                }
                lvArr[i] = new ListViewItem(enumStatus.entries[offset]);
                if (enumStatus.entries[offset][1] == LUGAPI.Disabled)
                {
                    lvArr[i].ImageIndex = (int)imageLUGDisabled;
                }
                else
                {
                    lvArr[i].ImageIndex = (int)imageLUG;
                }
            }
            AddRangeDelegate d = ThreadSafeAddRange;
            this.Invoke(d, new object[]
            {
                lvArr
            }
            );
            
            AutoResizeDelegate d2 = ThreadSafeAutoResize;
            Hostinfo hn = ctx as Hostinfo;
            this.Invoke(d2);
            if (enumStatus.moreEntries)
            {
                try
                {
                    if (enumStatus.type == LUGAPI.LUGType.User)
                    {
                        LUGAPI.NetEnumUsers(
                            hn.hostName,
                            enumStatus.resumeHandle,
                            out enumStatus);
                        enumLUGCallback(enumStatus);
                    }
                    else if (enumStatus.type == LUGAPI.LUGType.Group)
                    {
                        LUGAPI.NetEnumGroups(
                            hn.hostName,
                            enumStatus.resumeHandle,
                            out enumStatus);
                        enumLUGCallback(enumStatus);
                    }
                }
                catch(Exception ex)
                {
                    Logger.Log(ex.StackTrace);
                }
                //container.SetCursor(Cursors.Default);
            }
            else
            {
                //container.SetCursor(Cursors.Default);
            }
        }
    }

    private void ThreadSafeAddRange(ListViewItem[] range)
    {
        if (range != null && range.Length > 0)
        {
            this.lvLUGBETA.Items.AddRange(range);
        }
    }
    
    private void ThreadSafeAutoResize()
    {
        this.AutoResizePage();
    }
    
    private void InsertLUGToListView(string[] LUG)
    {
        if (LUG != null)
        {
            ListViewItem lvi = new ListViewItem(LUG);
            
            if (LUG[1] == LUGAPI.Disabled)
            {
                lvi.ImageIndex = (int)imageLUGDisabled;
            }
            else
            {
                lvi.ImageIndex = (int)imageLUG;
            }
            
            lvLUGBETA.Items.Add(lvi);
            AutoResizePage();
        }
    }
    
    private void AutoResizePage()
    {
        
        if (lvLUGBETA == null || lvLUGBETA.Columns == null || lvLUGBETA.Columns.Count < 2)
        {
            return;
        }
        
        numColumns = lvLUGBETA.Columns.Count;
        
        int minColumnWidth = (Width / numColumns) / 2;
        
        int widthExpansion = Width;
        
        
        lvLUGBETA.AutoResizeColumns(ColumnHeaderAutoResizeStyle.ColumnContent);
        
        
        for (int i = 1; i < numColumns; i++)
        {
            widthExpansion -= lvLUGBETA.Columns[i].Width;
        }
        
        if (widthExpansion <= 0)
        {
            return;
        }
        
        for (int i = 1; i < numColumns; i++)
        {
            int requestedIncrease = 0;
            
            if (lvLUGBETA.Columns[i].Width + columnMargin < minColumnWidth)
            {
                requestedIncrease = minColumnWidth - lvLUGBETA.Columns[i].Width;
            }
            else
            {
                requestedIncrease = columnMargin;
            }
            
            if (requestedIncrease > 0)
            {
                if (requestedIncrease > widthExpansion)
                {
                    requestedIncrease = widthExpansion;
                }
                
                lvLUGBETA.Columns[i].Width += requestedIncrease;
                widthExpansion -= requestedIncrease;
            }
        }
    }
    
    private void SetPasswordDlg()
    {
        Logger.Log("LUGPage.SetPassword() called", Logger.netAPILogLevel);
        
        if (lvLUGBETA.SelectedItems.Count != 1)
        {
            return;
        }
        
        ListViewItem item = lvLUGBETA.SelectedItems[0];
        string user = item.SubItems[2].Text;
        
        try
        {
            string caption = "Set New Password";
            string[] hints = { "", "" };
            string[] descriptions = { "New Password:", "Confirm Password:" };
            StringRequestDialog dlg = new StringRequestDialog(
            SetPasswordDelegate,
            caption, caption,
            descriptions,
            hints,
            new string[] { "", "" },
            null);
            dlg.SetSecurityStatus(0, true);
            dlg.SetSecurityStatus(1, true);
            dlg.ShowDialog();
        }
        catch (Exception e)
        {
            Logger.LogException("LUGPage.SetPasswordDlg", e);
        }
    }
    
    private bool SetPasswordDelegate(string[] password, Object context)
    {
        string errorMessage = null;
        if (Hostinfo.ValidatePassword(password[0], password[1], out errorMessage))
        {
            return this.ChangePassword(password[0]);
        }
        else
        {
            Logger.ShowUserError(errorMessage);
            Logger.Log("LUGPage.SetPasswordDelegate() error: " + errorMessage);
            return false;
        }
        
    }
    
    public void CreateDlg()
    {
        Logger.Log("LUGPage.CreateDlg() called", Logger.netAPILogLevel);
        
        try
        {
            if (memberType == LUGAPI.LUGType.User)
            {
                NewUserDlg nuDlg = new NewUserDlg(this.container, this);
                nuDlg.ShowDialog(this);
                Refresh();
            }
            else if (memberType == LUGAPI.LUGType.Group)
            {
                Hostinfo hn = ctx as Hostinfo;
                NewGroupDlg ngDlg = new NewGroupDlg(this.container, this, hn, base.pi);
                ngDlg.ShowDialog(this);
                Refresh();
            }
            
            
        }
        catch (Exception except)
        {
            container.ShowError(except.Message);
        }
    }
    
    private void RenameDlg()
    {
        
        Logger.Log("LUGPage.Rename() called", Logger.netAPILogLevel);
        
        if (lvLUGBETA.SelectedItems.Count != 1)
        {
            return;
        }
        
        try
        {
            string oldName = "";
            string caption = "";
            string groupBoxCaption = "";
            string hint = "";
            string description = "";
            ListViewItem item = lvLUGBETA.SelectedItems[0];
            if (memberType == LUGAPI.LUGType.User)
            {
                oldName = item.SubItems[2].Text;
                caption = "Rename User";
                description = "New Name:";
                hint = "e.g. myNewName";
            }
            else if (memberType == LUGAPI.LUGType.Group)
            {
                oldName = item.SubItems[1].Text;
                caption = "Rename Group";
                description = "New Name:";
                hint = "e.g., ourNewGroupName";
            }

            StringRequestDialog dlg = new StringRequestDialog(
            LUGRenameDelegate,
            caption,
            groupBoxCaption,
            new string[]
            {
                description
            }
            ,
            new string[]
            {
                hint
            }
            ,
            new string[]
            {
                oldName
            }
            ,
            oldName);
            dlg.ShowDialog();
            
        }
        catch (Exception except)
        {
            container.ShowError(except.Message);
        }
    }
    
    private bool LUGRenameDelegate(string[] newNames, Object oldName)
    {
        if (newNames == null || newNames.Length == 0)
        {
            return false;
        }
        string newName = newNames[0];
        string oldNameStr = (string)oldName;
        
        if (!String.IsNullOrEmpty(oldNameStr))
        {
            Hostinfo hn = ctx as Hostinfo;
            this.renameLUG(hn.hostName, oldNameStr, newName);
            Refresh();
            return true;
        }
        return false;
    }
    
    private void DeleteDlg()
    {
        if (lvLUGBETA.SelectedItems.Count != 1)
        {
            return;
        }
        
        try
        {
            string s = "";
            string LUG = "";
            ListViewItem item = lvLUGBETA.SelectedItems[0];
            if (memberType == LUGAPI.LUGType.User)
            {
                LUG = item.SubItems[2].Text;
                s = String.Format(Resources.DeleteUserMessage, LUG);
            }
            else if (memberType == LUGAPI.LUGType.Group)
            {
                LUG = item.SubItems[1].Text;
                s = String.Format(Resources.DeleteGroupMessage, LUG);
            }
            
            DialogResult dlgRes = container.Prompt(s, MessageBoxButtons.YesNo);
            
            if (dlgRes == DialogResult.Yes)
            {
                Hostinfo hn = ctx as Hostinfo;
                deleteLUG(hn.hostName, LUG);
                Refresh();
            }
        }
        catch (Exception except)
        {
            container.ShowError(except.Message);
        }
    }
    
    private void ShowLUGPropertiesDlg()
    {
        Logger.Log("LUGPage.ShowLUGProperties() called", Logger.netAPILogLevel);
        
        if (memberType == LUGAPI.LUGType.User)
        {
            if (lvLUGBETA.SelectedItems.Count != 1)
            {
                return;
            }
            
            ListViewItem item = lvLUGBETA.SelectedItems[0];
            string user = item.SubItems[2].Text;
            
            Logger.Log(String.Format("LUGPage.ShowLUGProperties() user={0}", user), Logger.netAPILogLevel);
            
            try
            {
                UserPropertiesDlg lugDlg = new UserPropertiesDlg(container, this);
                Hostinfo hn = ctx as Hostinfo;
                
                lugDlg.SetData(hn.creds, hn.hostName, user);
                
                lugDlg.ShowDialog();
                
            }
            catch (Exception e)
            {
                Logger.LogException("LUGPage.ShowLUGPropertiesDlg (user)", e);
            }
        }
        else if (memberType == LUGAPI.LUGType.Group)
        {
            if (lvLUGBETA.SelectedItems.Count != 1)
            {
                return;
            }
            
            try
            {                
                ListViewItem item = lvLUGBETA.SelectedItems[0];
                
                string group = item.SubItems[1].Text;
                
                Logger.Log(String.Format("LUGPage.ShowLUGProperties() group={0}", group), Logger.netAPILogLevel);
                
                GroupPropertiesDlg gpDlg = new GroupPropertiesDlg(this.container, this, pi);
                Hostinfo hn = ctx as Hostinfo;
                gpDlg.SetData(hn.creds, hn.hostName, group);
                gpDlg.ShowDialog();
            }
            catch (Exception e)
            {
                Logger.LogException("LUGPage.ShowLUGPropertiesDlg (group)", e);
            }
        }
    }

    private ContextMenu BuildLUGContextMenu()
    {
        ContextMenu cm = new ContextMenu();

        MenuItem m_item = null;

        if (lvLUGBETA.SelectedItems.Count == 0)
        {
            if (memberType == LUGAPI.LUGType.User)
                m_item = new MenuItem("New &User...", new EventHandler(On_MenuClick));
            else 
                m_item = new MenuItem("New &Group...", new EventHandler(On_MenuClick));
            cm.MenuItems.Add(0, m_item);
        }
        else
        {
            if (memberType == LUGAPI.LUGType.User)  
                m_item = new MenuItem("&Set Password...", new EventHandler(On_MenuClick));
            else
                m_item = new MenuItem("&Add to Group...", new EventHandler(On_MenuClick));
            cm.MenuItems.Add(0, m_item);
            
            m_item = new MenuItem("&Rename...", new EventHandler(On_MenuClick));           
            cm.MenuItems.Add(m_item);

            m_item = new MenuItem("&Properties...", new EventHandler(On_MenuClick));
            cm.MenuItems.Add(m_item);

            m_item = new MenuItem("&Delete", new EventHandler(On_MenuClick));
            cm.MenuItems.Add(m_item);
        }

        m_item = new MenuItem("-");
        cm.MenuItems.Add(m_item);

        m_item = new MenuItem("&Refresh", new EventHandler(On_MenuClick));        
        cm.MenuItems.Add(m_item);

        m_item = new MenuItem("-");
        cm.MenuItems.Add(m_item);

        m_item = new MenuItem("&Help", new EventHandler(On_MenuClick));
        cm.MenuItems.Add(cm.MenuItems.Count, m_item);

        return cm;       
    }

    private void On_MenuClick(object sender, EventArgs args)
    {
        MenuItem mi = sender as MenuItem;

        switch (mi.Text.Trim())
        {
            case "New &User...":
            case "New &Group...":
                CreateDlg();
                break;

            case "&Add to Group...":
                ShowLUGPropertiesDlg();
                break;

            case "&Set Password...":
                SetPasswordDlg();
                break;

            case "&Rename...":
                RenameDlg();
                break;

            case "&Properties...":
                ShowLUGPropertiesDlg();
                break;

            case "&Delete":
                DeleteDlg();
                break;

            case "&Refresh":
                treeNode.IsModified = true;
                treeNode.sc.ShowControl(treeNode);               
                break;

            case "&Help":
                ProcessStartInfo psi = new ProcessStartInfo();
                psi.UseShellExecute = true;
                psi.FileName = CommonResources.GetString("LAC_Help");
                psi.Verb = "open";
                psi.WindowStyle = ProcessWindowStyle.Normal;
                Process.Start(psi);               
                break;
        }
    }

    #endregion    

    #region Events
    
    private void acAddToGroup_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
    {
        ShowLUGPropertiesDlg();
    }
    
    private void acNewLUG_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
    {
        CreateDlg();
    }
    
    private void acDelete_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
    {
        DeleteDlg();
    }
    
    private void acSetPassword_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
    {
        SetPasswordDlg();
    }
    
    private void acProperties_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
    {
        ShowLUGPropertiesDlg();
    }
    
    private void acRename_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
    {
        RenameDlg();
    }   


    private void lvLUGBETA_DoubleClick(object sender, EventArgs e)
    {
        if (lvLUGBETA.SelectedItems.Count == 0)
            return;

        if (memberType == LUGAPI.LUGType.Dummy)
        {
            ListViewItem selectedItem = lvLUGBETA.SelectedItems[0];
            if (selectedItem.Tag != null && selectedItem.Tag is LACTreeNode)
            {
                LACTreeNode node = selectedItem.Tag as LACTreeNode;
                if (node != null)
                {
                    node.TreeView.SelectedNode = node;
                    node.TreeView.Select();
                    node.sc = treeNode.sc;
                    node.sc.ShowControl(node);
                }
            }
        }
        else 
        {
            ShowLUGPropertiesDlg();
        }
    }
    
    private void lvLUGBETA_MouseUp(object sender, MouseEventArgs e)
    {
        ContextMenu cm = null;
        ListView lvSender = sender as ListView;
        if (lvSender != null && e.Button == MouseButtons.Right)
        {
            ListViewHitTestInfo hti = lvSender.HitTest(e.X, e.Y);

            if (memberType == LUGAPI.LUGType.Dummy)
            {
                ListViewItem selectedItem = lvLUGBETA.SelectedItems[0];
                if (selectedItem.Tag != null && selectedItem.Tag is LACTreeNode)
                {
                    LACTreeNode node = selectedItem.Tag as LACTreeNode;
                    if (node != null)
                    {
                        LUGPlugIn plugin = node.Plugin as LUGPlugIn;
                        cm = plugin.GetTreeContextMenu(node);
                    }
                }
            }
            else  {
                cm = BuildLUGContextMenu();      
            } 
            cm.Show(lvSender, new Point(e.X, e.Y));
        }
    }
    
    private void lvLUGBETA_KeyDown(object sender, KeyEventArgs e)
    {
        if (e.KeyCode == Keys.Enter)
        {
            if (lvLUGBETA.SelectedItems.Count == 1)
            {
                ShowLUGPropertiesDlg();
            }
            e.Handled = true;
        }
    }
    
    private void lvLUGBETA_ColumnClick(object sender, ColumnClickEventArgs e)
    {
        // Determine if clicked column is already the column that is being sorted.
        if (e.Column == lvwColumnSorter.SortColumn)
        {
            // Reverse the current sort direction for this column.
            if (lvwColumnSorter.Order == SortOrder.Ascending)
            {
                lvwColumnSorter.Order = SortOrder.Descending;
            }
            else
            {
                lvwColumnSorter.Order = SortOrder.Ascending;
            }
        }
        else
        {
            // Set the column number that is to be sorted; default to ascending.
            lvwColumnSorter.SortColumn = e.Column;
            lvwColumnSorter.Order = SortOrder.Ascending;
        }
        
        // Perform the sort with these new sort options.
        this.lvLUGBETA.Sort();
    }
    
    #endregion
    
    private void cbLog_SelectedIndexChanged(object sender, EventArgs e)
    {
        Hostinfo hn = ctx as Hostinfo;
        lvLUGBETA.Items.Clear();

        //enumLUG(hn.creds, hn.hostName, 0, enumLUGCallback);

        LUGAPI.LUGEnumStatus enumStatus;
        enumLUG(hn.hostName, 0, out enumStatus);
        enumLUGCallback(enumStatus);
    }
}

    public interface IPropertiesPage
    {
        void SetData(CredentialEntry ce, string servername, string name);
    }

    public interface IUIInitialize
    {
        void SetData(string servername, string userOrGroup);
    }
}



