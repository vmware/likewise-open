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

namespace Likewise.LMC.Plugins.ADUCPlugin
{
public partial class ObjectAddFinalPage : Likewise.LMC.ServerControl.WizardPage
{
    #region Class Data
    private ADObjectAddDlg _objectAddDlg;
    private IPlugInContainer _container = null;
    private StandardPage _parentPage = null;
    //private string pageName;
    #endregion
    
    #region Constructors
    public ObjectAddFinalPage()
    {
        InitializeComponent();
    }
    
    public ObjectAddFinalPage(ADObjectAddDlg objectAddDlg,IPlugInContainer container, StandardPage parentPage)
    : this()
    {
        this._objectAddDlg = objectAddDlg;
        this._container = container;
        this._parentPage = parentPage;
        //this.pageName = "finalnewObjectPage";
    }
    #endregion
    
    #region Override Methods
    public override bool OnWizardFinish()
    {
        bool IsgroupTypeValid = false, IsObjectGroup = false;
        foreach (string key in _objectAddDlg.objectInfo.htMandatoryAttrList.Keys)
        {
            if (key.Trim().Equals("groupType"))
            {
                string svalue = _objectAddDlg.objectInfo.htMandatoryAttrList[key] as string;
                switch (svalue)
                {
                    case "4":
                    case "2":
                    case "6":
                    case "2147483644":
                    case "2147483640":
                    case "2147483646":
                        IsgroupTypeValid = true;
                        break;
                    default:
                        MessageBox.Show(this, "The specified group type is invalid", CommonResources.GetString("Caption_Console"),
                        MessageBoxButtons.OK, MessageBoxIcon.Error);
                        IsgroupTypeValid = false;
                        break;
                }
                IsObjectGroup = true;
                break;
            }
        }
        if (!IsgroupTypeValid && IsObjectGroup)
            return false;
        _objectAddDlg.objectInfo.commit = true;
        return base.OnWizardFinish();
    }
    
    public override bool OnWizardCancel()
    {
        _objectAddDlg.objectInfo.commit = false;
        return base.OnWizardCancel();
    }

    public override string OnWizardBack()
    {
        try
        {
            return _objectAddDlg.objectInfo.addedPages[--ObjectInfo.PageIndex];
        }
        catch
        {
            return ObjectAddWelcomePage.PAGENAME;
        }
    }
    
    public override bool OnSetActive()
    {
        Wizard.enableButton(WizardDialog.WizardButton.Cancel);
        Wizard.enableButton(WizardDialog.WizardButton.Back);
        Wizard.showButton(WizardDialog.WizardButton.Finish);
        Wizard.hideButton(WizardDialog.WizardButton.Next);
        Wizard.hideButton(WizardDialog.WizardButton.Start);
        Wizard.enableButton(WizardDialog.WizardButton.Finish);
        
        return true;
    }
    #endregion
    
    #region Events
    
    /// <summary>
    /// Shows up the ObjectAddMoreAttributes dialog to define the attributes for the selected object class
    /// </summary>
    /// <param name="sender"></param>
    /// <param name="e"></param>
    private void MoreAttrbutton_Click(object sender, EventArgs e)
    {
        ObjectAddMoreAttributesDlg f = new ObjectAddMoreAttributesDlg(_objectAddDlg, _container, _parentPage);
        f.SetData(null, null, _objectAddDlg.choosenClass, null, _objectAddDlg.schemaCache);
        f.ShowDialog(this);
    }
    #endregion
}
}

