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
using Likewise.LMC.Utilities;
using Likewise.LMC.Plugins.LUG.Properties;
using Likewise.LMC.ServerControl;
using Likewise.LMC.NETAPI;

namespace Likewise.LMC.Plugins.LUG
{
public partial class GroupPropertiesDlg : MPContainer
{
    #region Class Data
    
    private string _servername;
    private string _group;
    
    private EditSimpleListPage groupPropertiesPage;
    
    private IPlugIn plugin;
    private Hostinfo hn = null;
    
    #endregion
    
    #region Constructors
    
    public GroupPropertiesDlg(IPlugInContainer container, StandardPage parentPage, IPlugIn plugin)
    : base(container, parentPage)
    {
        this.plugin = plugin;
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
                
                LUGPage lup = (LUGPage)this.ParentPage;
                lup.EditLUG(this);
                
                groupPropertiesPage.ApplyMembers();
                
            }
            catch (Exception e)
            {
                b = false;
                Logger.LogException("GroupPropertiesDlg.Apply", e);
            }
        }
        
        
        return b;
    }
    
    #endregion
    
    #region Helper Methods
    
    private void InitializePages()
    {
        groupPropertiesPage =
        new EditSimpleListPage("Members", _group,
        Resources.LocalGroup_32,
        "Group", "Contains the following users: ", "User",
        GetGroupDescription, GetUserList, AddUserToGroup, DeleteUserFromGroup,
        (EditDialog)this);
        
        this.AddPage(groupPropertiesPage,
        new MPMenuItem(groupPropertiesPage.PageID, "Members", "Members"),
        MPMenu.POSITION_END
        );
        
        
    }
    
    
    public string GetGroupDescription()
    {
        hn = plugin.GetContext() as Hostinfo;
        if (hn != null /*&& Hostinfo.HasCreds(hn)*/)
        {
            string description;

            if (LUGAPI.NetGetGroupInfo(hn.creds, _servername, _group, out description) == 0)
            {
                return description;
            }
            else
            {
                return "";
            }
        }
        else
        {
            return "";
        }
        
    }
    
    public string[] GetUserList()
    {
        hn = plugin.GetContext() as Hostinfo;
        if (hn != null /*&& Hostinfo.HasCreds(hn)*/)
        {
            string[] members;

            if (LUGAPI.NetGetGroupMembers(hn.creds, _servername, _group, out members) == 0)
            {
                return members;
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
    
    public bool AddUserToGroup(string user)
    {
        hn = plugin.GetContext() as Hostinfo;
        if (hn != null /*&& Hostinfo.HasCreds(hn)*/)
        {
            return !Convert.ToBoolean(LUGAPI.NetAddGroupMember(hn.creds, _servername, _group, user));
        }
        else
        {
            return false;
        }
        
    }
    
    public bool DeleteUserFromGroup(string user)
    {
        hn = plugin.GetContext() as Hostinfo;
        if (hn != null /*&& Hostinfo.HasCreds(hn)*/)
        {
            return !Convert.ToBoolean(LUGAPI.NetDeleteUserFromGroup(hn.creds, _servername, _group, user));
        }
        else
        {
            return false;
        }
    }
    
    
    
    public void SetData(CredentialEntry ce, string servername, string group)
    {
        _servername = servername;
        _group = group;
        
        Text = String.Format(Text, group);
        
        foreach (MPPage page in GetPages())
        {
            IPropertiesPage ipp = page as IPropertiesPage;
            ipp.SetData(ce, servername, group);
        }
    }
    #endregion
    
    #region Accessors
    
    public string GroupName
    {
        get
        {
            return _group;
        }
    }
    
    public string Description
    {
        get
        {
            return groupPropertiesPage.Description;
        }
    }
    #endregion
    
}
}

