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
using System.Collections.Generic;
using System.Windows.Forms;
using Likewise.LMC.Utilities;
using Likewise.LMC.Plugins.LUG.Properties;
using Likewise.LMC.ServerControl;
using Likewise.LMC.NETAPI;


namespace Likewise.LMC.Plugins.LUG
{
public struct UserProperties
{
    public string userName;
    public string fullName;
    public string description;
    public bool cannotChange;
    public bool mustChange;
    public bool neverExpires;
    public bool isDisabled;
    public bool isLocked;
    
    public UserProperties(string userName, string fullName, string description, bool cannotChange,
    bool mustChange, bool neverExpires, bool isDisabled, bool isLocked)
    {
        this.userName = userName;
        this.fullName = fullName;
        this.description = description;
        this.cannotChange = cannotChange;
        this.mustChange = mustChange;
        this.neverExpires = neverExpires;
        this.isDisabled = isDisabled;
        this.isLocked = isLocked;
    }
}



public partial class UserPropertiesDlg : MPContainer
{
    
    #region Class Data
    
    private string _servername;
    private string _username;
    
    private EditUserPage editUserPage = null;
    private EditSimpleListPage userMemberOf = null;
    
    private List<ListViewItem> Groups = null;
    
    #endregion
    
    #region Constructors
    
    public UserPropertiesDlg(IPlugInContainer container, StandardPage parentPage)
    : base(container, parentPage)
    {
        Groups = new List<ListViewItem>();
        
        InitializeComponent();
        
        InitializePages();
        
    }
    
    #endregion
    
    #region override methods
    
    protected override void ValueChangedHandler(object sender, EventArgs e)
    {
        base.ValueChangedHandler(sender, e);
        bDataWasChanged = true;
    }
    
    #endregion
    
    #region Events
    
    protected override bool Apply(EditDialogAction actionCause)
    {
        bool b = base.Apply(actionCause);
        
        if (b)
        {
            try
            {
                if (userMemberOf.Visible)
                {
                    editUserPage.Description = userMemberOf.Description;
                }
                else
                {
                    userMemberOf.Description = editUserPage.Description;
                }
                
                LUGPage lup = (LUGPage)this.ParentPage;
                lup.EditLUG(this);
                
                userMemberOf.ApplyMembers();
                
            }
            catch(Exception e)
            {
                b = false;
                Logger.LogException("UserPropertiesDlg.Apply", e);
            }
        }
        
        
        return b;
    }
    
    #endregion
    
    #region Helper Methods
    
    
    private void InitializePages()
    {
        // EditUserPage
        editUserPage = new EditUserPage();
        this.AddPage(editUserPage,
        new MPMenuItem(editUserPage.PageID, Resources.Caption_General, Resources.Caption_General),
        MPMenu.POSITION_END
        );
        
        
        userMemberOf =
        new EditSimpleListPage(Resources.Caption_MemberOf, _username,
        Resources.LocalGroup_32,
        "User", "Is a member of the following groups: ", "Group",
        GetUserDescription, GetGroupList, AddUserToGroup, DeleteUserFromGroup,
        (EditDialog) this);
        
        this.AddPage(userMemberOf,
        new MPMenuItem(userMemberOf.PageID, Resources.Caption_MemberOf, Resources.Caption_MemberOf),
        MPMenu.POSITION_END
        );
    }
    
    
    public string GetUserDescription()
    {
        return userProperties.description;
    }
    
    public string[] GetGroupList()
    {
        if (ParentPage == null)
            return null;

        Hostinfo hn = ParentPage.GetContext() as Hostinfo;

        if (hn != null && hn.creds == null)
        {
            string [] groups;

            if (LUGAPI.NetGetGroups(_servername, _username, out groups) == 0)
            {
                return groups;
            }
            else
            {
                return null;
            }
        }
        else
        {
            return null;
        }
    }
    
    public bool AddUserToGroup(string group)
    {
        if (ParentPage == null)
            return false;

        Hostinfo hn = ParentPage.GetContext() as Hostinfo;

        if (hn != null && hn.creds == null)
        {
            return !Convert.ToBoolean(LUGAPI.NetAddGroupMember(_servername, group, _username));
        }
        else
        {
            return false;
        }
    }
    
    public bool DeleteUserFromGroup(string group)
    {
        if (ParentPage == null)
            return false;

        Hostinfo hn = ParentPage.GetContext() as Hostinfo;

        if (hn != null && hn.creds == null)
        {
            return !Convert.ToBoolean(LUGAPI.NetDeleteUserFromGroup(_servername, group, _username));
        }
        else
        {
            return false;
        }
    }    
    
    public void SetData(CredentialEntry ce, string servername, string username)
    {
        
        Logger.Log(String.Format(
        "UserPropertiesDlg.SetData({0}, {1}, {2}) called",
        ce,
        servername,
        username),
        Logger.netAPILogLevel);
        
        _servername = servername;
        _username = username;
        
        this.Text = String.Format(this.Text, username);
        
        foreach (MPPage page in this.GetPages())
        {
            IPropertiesPage ipp = page as IPropertiesPage;
            if (ipp != null)
            {
                ipp.SetData(ce, servername, username);
            }
        }
        
    }
    
    #endregion
    
    #region Accessors
    
    public UserProperties userProperties
    {
        get
        {
            return editUserPage.userProperties;
        }
    }
    
    
    #endregion
    
}
}


