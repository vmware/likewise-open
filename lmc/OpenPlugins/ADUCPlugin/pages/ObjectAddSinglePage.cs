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
using SchemaType=Likewise.LMC.LDAP.SchemaType;

namespace Likewise.LMC.Plugins.ADUCPlugin
{
public partial class ObjectAddSinglePage : Likewise.LMC.ServerControl.WizardPage
{
    #region Class Data
    private ADObjectAddDlg _objectAddDlg;
    private static string pageName;
    #endregion
    
    
    #region Constructors
    public ObjectAddSinglePage()
    {
        InitializeComponent();
    }
    
    
    /// <summary>
    /// Overriden constructor will get and initializes attribute list from the selected object class from the treeview
    /// </summary>
    /// <param name="objectAddDlg"></param>
    /// <param name="attrName"></param>
    public ObjectAddSinglePage(ADObjectAddDlg objectAddDlg, string attrName)
    : this()
    {
        this.Name = attrName;
        this._objectAddDlg = objectAddDlg;
        this.attrNameLabel.Text = attrName;

        SchemaType syntax = _objectAddDlg.schemaCache.GetSchemaTypeByDisplayName(attrName);
        if (syntax != null)
        {
            this.cnSyntaxlabel.Text = ObjectAddAttributesTab.GetADSTypeString(syntax);
        }
        pageName = attrName;
        this.cntextBox.Text = "";
    }
    #endregion
    
    #region Override Methods
    public override bool OnSetActive()
    {
        Wizard.enableButton(WizardDialog.WizardButton.Cancel);
        Wizard.enableButton(WizardDialog.WizardButton.Back);
        Wizard.hideButton(WizardDialog.WizardButton.Start);
        if (this.cntextBox.Text.Trim() != "")
        {
            Wizard.enableButton(WizardDialog.WizardButton.Next);
        }
        else
        {
            Wizard.disableButton(WizardDialog.WizardButton.Next);
        }
        Wizard.showButton(WizardDialog.WizardButton.Next);
        Wizard.hideButton(WizardDialog.WizardButton.Finish);
        return true;
    }
    
    public override string OnWizardNext()
    {
        string Message;
        string AttrSyntax = this.cnSyntaxlabel.Text.ToString();
        string Attrvalue = this.cntextBox.Text.ToString();
        if (AttrSyntax != "" && Attrvalue != "")
        {
            bool bSuccess = ObjectAddAttributesTab.ValidateData(Attrvalue, AttrSyntax, out Message);
            if (!bSuccess)
            {
                MessageBox.Show(
                this,
                Message,
                CommonResources.GetString("Caption_Console"),
                MessageBoxButtons.OK,
                MessageBoxIcon.Exclamation);
                this.cntextBox.Clear();
                this.cntextBox.Focus();
                return WizardDialog.NoPageChange;
            }
        }
        if (_objectAddDlg.objectInfo.htMandatoryAttrList.ContainsKey(this.attrNameLabel.Text.Trim()))
        {
            _objectAddDlg.objectInfo.htMandatoryAttrList.Remove(this.attrNameLabel.Text.Trim());           
        }
        _objectAddDlg.objectInfo.htMandatoryAttrList.Add(this.attrNameLabel.Text.Trim(), this.cntextBox.Text);

        if (_objectAddDlg.objectInfo.addedPages.Count != 0&&
            _objectAddDlg.objectInfo.addedPages.Count > ObjectInfo.PageIndex)
        {
            return _objectAddDlg.objectInfo.addedPages[ObjectInfo.PageIndex++];
        }

        return base.OnWizardNext();
    }

    public override string OnWizardBack()
    {
        try
        {
            string sCurrentPage = _objectAddDlg.objectInfo.addedPages[ObjectInfo.PageIndex - 1];

            if (this.Name.Trim().Equals(sCurrentPage))
            {
                --ObjectInfo.PageIndex;
            }

            return _objectAddDlg.objectInfo.addedPages[--ObjectInfo.PageIndex];
        }
        catch
        {
            return ObjectAddWelcomePage.PAGENAME;
        }
    }
    #endregion
    
    #region Events
    private void cntextBox_TextChanged(object sender, EventArgs e)
    {
        if (this.cntextBox.Text.Trim() != "")
        {
            Wizard.enableButton(WizardDialog.WizardButton.Next);
        }
        else
        {
            Wizard.disableButton(WizardDialog.WizardButton.Next);
        }
    }
    
    private void cntextBox_KeyPress(object sender, KeyPressEventArgs e)
    {
        //if (pageName == "groupType")
        //{
        //    if (!Char.IsDigit(e.KeyChar) && (!(e.KeyChar == 8)) && (!(e.KeyChar == 22)) && (!(e.KeyChar == 3)))
        //        e.Handled = true;
        //}
    }
    #endregion
}
}

