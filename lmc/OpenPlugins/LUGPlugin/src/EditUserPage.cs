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
public partial class EditUserPage : MPPage, IPropertiesPage
{
    #region Class Data
    
    private string _servername = "";
    private string _username = "";
    
    #endregion
    
    #region Constructors
    public EditUserPage()
    {
        this.pageID = "UserProperitiesGeneral";
        InitializeComponent();
        SetPageTitle(Resources.Caption_General);
        
        this.tbFullName.Select();
    }
    #endregion
    
    #region Initialization Methods
    public void SetData(CredentialEntry ce, string servername, string username)
    {
        
        _servername = servername;
        _username = username;
        
        this.lbUser.Text = String.Format(this.lbUser.Text, username);
        
        try
        {
            LUGAPI.LUGInfo userInfo;

            if (LUGAPI.NetGetUserInfo(ce, _servername, _username, out userInfo) == 0)
            {
                GetUserInfoDelegate(userInfo);
            }
        }
        catch (Exception e)
        {
            Logger.LogException("EditUserPage.SetData", e);
        }
        
    }
    public void GetUserInfoDelegate(LUGAPI.LUGInfo userInfo)
    {
        this.tbDescription.Text = userInfo.description;
        this.tbFullName.Text = userInfo.fullname;
        
        this.cbAccountLockedOut.Checked = (userInfo.flags & LUGAPI.UF_LOCKOUT) == 0 ? false : true;
        if (cbAccountLockedOut.Checked == true)
        {
            cbAccountLockedOut.Enabled = true;
        }
        
        this.cbCannotChange.Checked = (userInfo.flags & LUGAPI.UF_PASSWD_CANT_CHANGE) == 0 ? false : true;
        this.cbIsDisabled.Checked = (userInfo.flags & LUGAPI.UF_ACCOUNTDISABLE) == 0 ? false : true;
        this.cbNeverExpires.Checked = (userInfo.flags & LUGAPI.UF_DONT_EXPIRE_PASSWD) == 0 ? false : true;
        this.cbMustChange.Checked = (userInfo.flags & LUGAPI.UF_PASSWORD_EXPIRED) == 0 ? false : true;
    }
    #endregion
    
    #region accessor functions
    
    public UserProperties userProperties
    {
        get
        {
            UserProperties result = new UserProperties(
            _username,
            tbFullName.Text,
            tbDescription.Text,
            cbCannotChange.Checked,
            cbMustChange.Checked,
            cbNeverExpires.Checked,
            cbIsDisabled.Checked,
            cbAccountLockedOut.Checked);
            return result;
        }
    }
    
    public string Description
    {
        get
        {
            return tbDescription.Text;
        }
        set
        {
            tbDescription.Text = value;
        }
    }
    
    #endregion
    
    #region EventHandlers
    private void cbCannotChange_CheckedChanged(object sender, EventArgs e)
    {
        if (!cbNeverExpires.Checked && !cbCannotChange.Checked)
        {
            cbMustChange.Enabled = true;
        }
        else if (cbMustChange.Enabled)
        {
            cbMustChange.Enabled = false;
        }
    }
    
    private void cbMustChange_CheckedChanged(object sender, EventArgs e)
    {
        if (cbMustChange.Checked)
        {
            cbNeverExpires.Checked = false;
            cbNeverExpires.Enabled = false;
            cbCannotChange.Checked = false;
            cbCannotChange.Enabled = false;
        }
        else
        {
            cbNeverExpires.Enabled = true;
            cbCannotChange.Enabled = true;
        }
    }
    
    private void cbNeverExpirers_CheckedChanged(object sender, EventArgs e)
    {
        if (!cbNeverExpires.Checked && !cbCannotChange.Checked)
        {
            cbMustChange.Enabled = true;
        }
        else if (cbMustChange.Enabled)
        {
            cbMustChange.Enabled = false;
        }
    }
    
    #endregion
    
}
}

