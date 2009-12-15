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
using System.Text.RegularExpressions;
using Likewise.LMC.ServerControl;

namespace Likewise.LMC.Plugins.ADUCPlugin
{
public partial class ComputerAddManagedPage : WizardPage
{
    #region Class Data
    private ADComputerAddDlg _computerAddDlg;
    public static string PAGENAME = "ComputerAddManagedPage";
    Regex GUIDRegex = new Regex("^\\{[\\d\\w\\s]{8}\\-[\\d\\w\\s]{4}\\-[\\d\\w\\s]{4}\\-[\\d\\w\\s]{4}\\-[\\d\\w\\s]{12}\\}$", RegexOptions.Compiled);
    #endregion
    
    #region Constructors
    public ComputerAddManagedPage()
    {
        InitializeComponent();
    }
    
    public ComputerAddManagedPage(ADComputerAddDlg computerAddDlg)
    : this()
    {
        this._computerAddDlg = computerAddDlg;
        this._computerAddDlg.Text = "Managed";
        
        if (this._computerAddDlg.computerInfo.IsComputerGUID)
        {
            this.checkBox.Checked = true;
            this.textBoxGUID.Text = this._computerAddDlg.computerInfo.ComputerGUID;
        }
        else
        {
            this.checkBox.Checked = false;
        }
        this.txtCreatein.Text = "Create in: " + computerAddDlg.computerInfo.DomainName;
    }
    #endregion
    
    #region Override Methods
    public override string OnWizardNext()
    {
        string result = "";
        if (checkBox.Checked)
        {
            if (GUIDRegex.IsMatch(textBoxGUID.Text.Trim()))
            {
                this._computerAddDlg.computerInfo.ComputerGUID = this.textBoxGUID.Text.Trim();
            }
            result = ComputerAddHostServerPage.PAGENAME;
        }
        else
        {
            result = ComputerAddFinishPage.PAGENAME;
        }
        return result;
    }
    
    public override string OnWizardBack()
    {
        return base.OnWizardBack();
    }
    
    public override bool OnSetActive()
    {
        Wizard.enableButton(WizardDialog.WizardButton.Next);
        Wizard.enableButton(WizardDialog.WizardButton.Back);
        Wizard.enableButton(WizardDialog.WizardButton.Cancel);
        Wizard.enableButton(WizardDialog.WizardButton.Cancel);
        Wizard.showButton(WizardDialog.WizardButton.Next);
        if (checkBox.Checked)
        {
            if (this._computerAddDlg.computerInfo.ComputerGUID.Trim() == "")
            {
                Wizard.disableButton(WizardDialog.WizardButton.Next);
            }
            else
            {
                Wizard.enableButton(WizardDialog.WizardButton.Next);
            }
        }
        Wizard.disableButton(WizardDialog.WizardButton.Finish);
        Wizard.hideButton(WizardDialog.WizardButton.Finish);
        Wizard.disableButton(WizardDialog.WizardButton.Start);
        Wizard.hideButton(WizardDialog.WizardButton.Start);
        
        if (this._computerAddDlg.computerInfo.IsComputerGUID)
        {
            this.textBoxGUID.Text = this._computerAddDlg.computerInfo.ComputerGUID;
        }
        
        return true;
    }
    #endregion
    
    #region Event Handlers
    private void checkBox_CheckedChanged(object sender, EventArgs e)
    {
        this.textBoxGUID.Enabled = checkBox.Checked;
        this.label3.Enabled = checkBox.Checked;
        if (!checkBox.Checked)
        {
            this._computerAddDlg.computerInfo.IsComputerGUID = false;
            Wizard.enableButton(WizardDialog.WizardButton.Next);
        }
        else if (GUIDRegex.IsMatch(textBoxGUID.Text.Trim()) && checkBox.Checked)
        {
            Wizard.enableButton(WizardDialog.WizardButton.Next);
        }
        else
        {
            Wizard.disableButton(WizardDialog.WizardButton.Next);
        }
    }
    
    private void textBoxGUID_TextChanged(object sender, EventArgs e)
    {
        if (GUIDRegex.IsMatch(textBoxGUID.Text.Trim()))
        {
            this._computerAddDlg.computerInfo.IsComputerGUID = true;
            Wizard.enableButton(WizardDialog.WizardButton.Next);
            
        }
        else
        {
            Wizard.disableButton(WizardDialog.WizardButton.Next);
        }
    }
    #endregion
}
}
