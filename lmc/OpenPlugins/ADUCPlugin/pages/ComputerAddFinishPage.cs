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

namespace Likewise.LMC.Plugins.ADUCPlugin
{
public partial class ComputerAddFinishPage : WizardPage
{
    #region Class Data
    private ADComputerAddDlg _computerAddDlg;
    public static string PAGENAME = "ComputerAddFinishPage";
    #endregion
    
    #region Constructors
    public ComputerAddFinishPage()
    {
        InitializeComponent();
    }
    
    public ComputerAddFinishPage(ADComputerAddDlg computerAddDlg)
    : this()
    {
        this._computerAddDlg = computerAddDlg;
        this.txtCreatein.Text = "Create in: " + computerAddDlg.computerInfo.DomainName;
        
    }
    #endregion
    
    #region Override Methods
    public override string OnWizardBack()
    {
        if (this._computerAddDlg.computerInfo.IsComputerGUID)
        {
            return ComputerAddHostServerPage.PAGENAME;
        }
        else
        {
            return ComputerAddManagedPage.PAGENAME;
        }
    }
    
    
    public override bool OnWizardFinish()
    {
        _computerAddDlg.computerInfo.commit = true;
        return base.OnWizardFinish();
    }
    
    public override bool OnWizardCancel()
    {
        _computerAddDlg.computerInfo.commit = false;
        return base.OnWizardCancel();
    }
    
    /// <summary>
    /// When activating the control on a wizard dialog loads the specific data.
    /// </summary>
    /// <returns></returns>
    public override bool OnSetActive()
    {
        this._computerAddDlg.Text = "New Object - Computer";
        Wizard.enableButton(WizardDialog.WizardButton.Cancel);
        Wizard.enableButton(WizardDialog.WizardButton.Back);
        Wizard.showButton(WizardDialog.WizardButton.Finish);
        Wizard.enableButton(WizardDialog.WizardButton.Finish);
        Wizard.hideButton(WizardDialog.WizardButton.Next);
        Wizard.disableButton(WizardDialog.WizardButton.Next);
        Wizard.disableButton(WizardDialog.WizardButton.Start);
        Wizard.hideButton(WizardDialog.WizardButton.Start);
        
        this.SummeryrichTextBox.Text = "Full name: " + this._computerAddDlg.computerInfo.ComputerName;
        
        if (this._computerAddDlg.computerInfo.IsPreWindowsComputer)
        {
            this.SummeryrichTextBox.Text = SummeryrichTextBox.Text + "\n" + "\n" + "Allow pre-Windows 2000 computers to use this account";
        }
        
        if (this._computerAddDlg.computerInfo.IsBackUpDomainComputer && this._computerAddDlg.computerInfo.IsPreWindowsComputer)
        {
            this.SummeryrichTextBox.Text = SummeryrichTextBox.Text + "\n" + "Allow pre-Windows 2000 back up domain controllers to use this account";
        }
        
        if (this._computerAddDlg.computerInfo.IsBackUpDomainComputer && !this._computerAddDlg.computerInfo.IsPreWindowsComputer)
        {
            this.SummeryrichTextBox.Text = SummeryrichTextBox.Text + "\n" + "\n" + "Allow pre-Windows 2000 back up domain controllers to use this account";
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
