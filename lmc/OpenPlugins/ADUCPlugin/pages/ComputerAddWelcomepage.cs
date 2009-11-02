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
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using Likewise.LMC.ServerControl;
using Likewise.LMC.Utilities;
using Likewise.LMC.LDAP;

namespace Likewise.LMC.Plugins.ADUCPlugin
{
public partial class ComputerAddWelcomepage : WizardPage
{
    #region Class Data
    private ADComputerAddDlg _computerAddDlg;
    public static string PAGENAME = "ComputerAddWelcomepage";
    private ADUCDirectoryNode _dirnode;
    #endregion
    
    #region Constructors
    public ComputerAddWelcomepage()
    {
        InitializeComponent();
    }
    
    public ComputerAddWelcomepage(ADComputerAddDlg computerAddDlg,ADUCDirectoryNode dirnode)
    : this()
    {
        this._computerAddDlg = computerAddDlg;
        this._computerAddDlg.Text = "New Object - Computer";
        this._dirnode = dirnode;
        this.txtcreatein.Text = "Create in: " + computerAddDlg.computerInfo.DomainName;
    }
    #endregion
    
    #region Override Methods
    public override string OnWizardNext()
    {
        string logonname = txtCompName.Text.Trim();  
        bool IsInvalidDigit = false;
        int index = 0;
        while (index < logonname.Length)
        {
            if (!Char.IsLetterOrDigit(logonname[index]) &&
                logonname[index] != '-')
            {                
                IsInvalidDigit = true;
                break;
            }
            else
            {
                index++;
            }
        }        
      
        if (IsInvalidDigit)
        {
            DialogResult dlg = MessageBox.Show(
            this,
            "The computer name " +
            txtCompName.Text.Trim() +
            " contains one or more non-stadard characters. Standard characters include\n"+
            "letters(A-Z,a-z), digits(0-9), and hyphens(-). Using a non-stadard name might affect ability to\n"+
            "interoperate with other computers, unless your network is using  the Microsoft DNS Server.\n"+
            "Do you want to use this any wany?",
            CommonResources.GetString("Caption_Console"),
            MessageBoxButtons.YesNo,
            MessageBoxIcon.Exclamation);
            if (dlg == DialogResult.No)
            {
                return WizardDialog.NoPageChange;
            }
        }

        string filterquery = string.Empty;      

        if (!String.IsNullOrEmpty(txtWindowsCompName.Text.Trim()))
        {
            filterquery = string.Format("(&(objectClass=computer)(sAMAccountName={0}))", txtWindowsCompName.Text.Trim());
            if (CheckNameExistanceinAD(filterquery, _dirnode.LdapContext.DomainName))
            {
                string sMsg =
                             "Windows cannot create the new computer object because " +
                             "the pre-Windows 2000 computer name  " +
                             txtWindowsCompName.Text +
                            " is already in use.\n Select another name, and then try again.";
                MessageBox.Show(this, sMsg, CommonResources.GetString("Caption_Console"),
                MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                return WizardDialog.NoPageChange;
            }
        }

        if (!String.IsNullOrEmpty(txtCompName.Text.Trim()))
        {
            filterquery = string.Format("(&(objectClass=computer)(name={0}))", txtCompName.Text.Trim());
            if (CheckNameExistanceinAD(filterquery, _dirnode.DistinguishedName))
            {
                string sMsg =
                             "Windows cannot create the new computer object because " +
                             "the name  " +
                             txtCompName.Text +
                            " is already in use.\n Select another name, and then try again.";
                MessageBox.Show(this, sMsg, CommonResources.GetString("Caption_Console"),
                MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                return WizardDialog.NoPageChange;
            }
        }

        this._computerAddDlg.computerInfo.ComputerName = txtCompName.Text.Trim();
        this._computerAddDlg.computerInfo.PreWindowsCName = txtWindowsCompName.Text.Trim() + "$"; ;
        this._computerAddDlg.computerInfo.IsPreWindowsComputer = cbWindowsComputer.Checked;
        this._computerAddDlg.computerInfo.IsBackUpDomainComputer = cbBackupComputer.Checked;
        this._computerAddDlg.computerInfo.UserAccountControl = UpdateUserAccountComp();
       
        
        return base.OnWizardNext();
    }
    
    
    public override bool OnSetActive()
    {
        Wizard.disableButton(WizardDialog.WizardButton.Back);
        Wizard.enableButton(WizardDialog.WizardButton.Cancel);
        Wizard.showButton(WizardDialog.WizardButton.Next);
        Wizard.hideButton(WizardDialog.WizardButton.Start);
        Wizard.disableButton(WizardDialog.WizardButton.Start);
        if (this._computerAddDlg.computerInfo.ComputerName.Trim() != "")
        {
            Wizard.enableButton(WizardDialog.WizardButton.Next);
        }
        else
        {
            Wizard.disableButton(WizardDialog.WizardButton.Next);
        }
        Wizard.disableButton(WizardDialog.WizardButton.Finish);
        Wizard.hideButton(WizardDialog.WizardButton.Finish);
        return true;
    }
    
    private int UpdateUserAccountComp()
    {
        int sUserAccountControl = 4128;
        if (cbWindowsComputer.Checked)
        {
            sUserAccountControl = 4128;
        }
        else if ((cbBackupComputer.Checked))
        {
            sUserAccountControl = 8224;
        }
        return sUserAccountControl;
    }

    private bool CheckNameExistanceinAD(string filterquery, string basedn)
    {
        List<LdapEntry> ldapEntries = null;

        string[] attrList = new string[]
        {
            "objectClass", 
            null
        };

        int ret = _dirnode.LdapContext.ListChildEntriesSynchronous
        (basedn,
        Likewise.LMC.LDAP.Interop.LdapAPI.LDAPSCOPE.SUB_TREE,
        filterquery,
        attrList,
        false,
        out ldapEntries);

        if (ldapEntries != null && ldapEntries.Count != 0)
        {
            return true;
        }
        return false;
    }

    #endregion
    
    #region Event Handlers
    private void txtCompName_TextChanged(object sender, EventArgs e)
    {
        if (txtCompName.Text.Trim().Length <= 15 && !txtCompName.Text.Trim().Contains("."))
        {
            this.txtWindowsCompName.Text = txtCompName.Text.Trim().ToUpper();
        }
        if (txtCompName.Text.Trim() != "")
        {
            Wizard.enableButton(WizardDialog.WizardButton.Next);
        }
        if (txtCompName.Text.Trim() == "")
        {
            Wizard.disableButton(WizardDialog.WizardButton.Next);
        }
    }
    
    private void txtWindowsCompName_TextChanged(object sender, EventArgs e)
    {
        if (txtWindowsCompName.Text.Trim() != "")
        {
            Wizard.enableButton(WizardDialog.WizardButton.Next);
        }
        if (txtWindowsCompName.Text.Trim() == "")
        {
            Wizard.disableButton(WizardDialog.WizardButton.Next);
        }
    }
    
    private void txtWindowsCompName_KeyPress(object sender, KeyPressEventArgs e)
    {
        if (e.KeyChar == '.')
        {
            e.Handled = true;
        }           
           
        if (Char.IsLetter(e.KeyChar))
        {
            e.KeyChar = Char.ToUpper(e.KeyChar);
        }
    }
    
    private void btnChange_Click(object sender, EventArgs e)
    {        
        Logger.LogMsgBox("ComputerAddWelcompage.btChange_Click not implemented");        
        
        //the functionalty below will not work in linux, so has been commented out and replaced with an error message.
            /*
            string servername = this._computerAddDlg.hn.hostName;
            ADObject[] adObject = DsPicker.Show(this.Handle, DsPicker.DialogType.SELECT_USERS, servername, false);
            if (adObject != null && adObject.Length >= 1)
            {
                foreach (ADObject ado in adObject)
                    this.txtUserGroup.Text = ado.Name;
            }
             */
    }
    #endregion   
}
}
