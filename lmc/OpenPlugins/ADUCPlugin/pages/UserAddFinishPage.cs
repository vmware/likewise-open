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
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using Likewise.LMC.ServerControl;
using Likewise.LMC.LDAP;

namespace Likewise.LMC.Plugins.ADUCPlugin
{
public partial class UserAddFinishPage : WizardPage
{
    #region Class Data
    private ADUserAddDlg _userAddDlg;
    private ADUCDirectoryNode _dirnode = null;
    #endregion
    
    #region Constructors
    public UserAddFinishPage()
    {
        InitializeComponent();
    }

    public UserAddFinishPage(ADUserAddDlg userAddDlg, ADUCDirectoryNode dirnode)
        : this()
    {
        this._dirnode = dirnode;
        this._userAddDlg = userAddDlg;
        this.txtcreatein.Text = "Create in: " + this._userAddDlg.userInfo.OUPath;
    }

    #endregion
    
    #region Override Methods
    public override bool OnWizardFinish()
    {
        string filterquery = string.Format("(&(objectClass=user)(name={0}))", this._userAddDlg.userInfo.fullName);
        List<LdapEntry> ldapEntries = null;

        string[] attrList = new string[]
        {
            "objectClass", null
        };

        int ret = _dirnode.LdapContext.ListChildEntriesSynchronous(
        _dirnode.DistinguishedName,
        Likewise.LMC.LDAP.Interop.LdapAPI.LDAPSCOPE.ONE_LEVEL,
        filterquery,
        attrList,
        false,
        out ldapEntries);
        if (ldapEntries != null && ldapEntries.Count != 0)
        {
            string sMsg = string.Format("LAC cannot create the new user object becuase the name {0} is already in use.\n" +
            "Select another name, and then try again", this._userAddDlg.userInfo.fullName);
            MessageBox.Show(this, sMsg, CommonResources.GetString("Caption_Console"),
            MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
            return false;
        }

        if (_userAddDlg.userInfo.passWord.Equals(string.Empty) || _userAddDlg.userInfo.retypedpassWord.Equals(string.Empty) || _userAddDlg.userInfo.passWord.Length < _userAddDlg.userInfo.MinPasswordLength)
        {
            string sMsg = string.Format(
            "LAC cannot create the object {0} because:\n" +
            "Unable to update the password.The value provided for " +
            "the new password does not meet the length, complexity, or\n" +
            "history requirement of the domain.",
            _userAddDlg.userInfo.fullName);
            MessageBox.Show(
            sMsg,
            CommonResources.GetString("Caption_Console"),
            MessageBoxButtons.OK,
            MessageBoxIcon.Error);
            return false;
        }
        _userAddDlg.userInfo.commit = true;
        return base.OnWizardFinish();
    }
    
    public override bool OnWizardCancel()
    {
        _userAddDlg.userInfo.commit = false;
        return base.OnWizardCancel();
    }
    
    /// <summary>
    /// Overriden function which displays the label text in finish page based on selected values
    /// </summary>
    /// <returns></returns>
    public override bool OnSetActive()
    {
        Wizard.enableButton(WizardDialog.WizardButton.Cancel);
        Wizard.enableButton(WizardDialog.WizardButton.Back);
        Wizard.showButton(WizardDialog.WizardButton.Finish);
        Wizard.showButton(WizardDialog.WizardButton.Finish);
        Wizard.hideButton(WizardDialog.WizardButton.Next);
        Wizard.hideButton(WizardDialog.WizardButton.Start);
        Wizard.enableButton(WizardDialog.WizardButton.Finish);
        
        this.SummeryrichTextBox.Text = "";
        
        if (!this._userAddDlg.userInfo.copyfrom.Equals(string.Empty))
        {
            this.SummeryrichTextBox.Text =
            SummeryrichTextBox.Text +
            this._userAddDlg.userInfo.copyfrom +
            "\n\n";
        }
        
        this.SummeryrichTextBox.Text = SummeryrichTextBox.Text + "Full name: " + 
                                        this._userAddDlg.userInfo.fullName + "\n" + "\n" + 
                                        "User logon name:" + this._userAddDlg.userInfo.logonName;
        
        if (this._userAddDlg.userInfo.bMustChangePwd)
        {
            this.SummeryrichTextBox.Text =
            SummeryrichTextBox.Text +
            "\n\nThe user must change the password at next logon";
        }
        
        if (this._userAddDlg.userInfo.bCannotChangePwd)
        {
            this.SummeryrichTextBox.Text =
            SummeryrichTextBox.Text +
            "\n\nThe user cannot change the password";
        }
        
        if (this._userAddDlg.userInfo.bNeverExpiresPwd && this._userAddDlg.userInfo.bCannotChangePwd)
        {
            this.SummeryrichTextBox.Text =
            SummeryrichTextBox.Text +
            "\nThe password never expires";
        }
        
        if (this._userAddDlg.userInfo.bNeverExpiresPwd && !this._userAddDlg.userInfo.bCannotChangePwd)
        {
            this.SummeryrichTextBox.Text =
            SummeryrichTextBox.Text +
            "\n\nThe password never expires";
        }
        
        if (this._userAddDlg.userInfo.bAcountDisable)
        {
            if (this._userAddDlg.userInfo.bNeverExpiresPwd ||
            this._userAddDlg.userInfo.bCannotChangePwd ||
            this._userAddDlg.userInfo.bMustChangePwd)
            {
                this.SummeryrichTextBox.Text =
                SummeryrichTextBox.Text +
                "\nThe account is disabled";
            }
            else
            {
                this.SummeryrichTextBox.Text =
                SummeryrichTextBox.Text +
                "\n\nThe account is disabled";
            }
        }        
        
        if (this.SummeryrichTextBox.Text.StartsWith("\n"))
        {
            this.SummeryrichTextBox.Text = this.SummeryrichTextBox.Text.Substring(1);
        }
        
        return true;
    }
    #endregion
    
}
}

