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
using System.Text.RegularExpressions;
using System.Windows.Forms;
using Likewise.LMC.ServerControl;

namespace Likewise.LMC.Plugins.ADUCPlugin
{
public partial class IntegerAttributeEditor : Form
{
    #region Class Data
    ListViewItem lvSelectedItem;
    
    public string IntAttrValue = "";
    public string OrigIntAttrValue = "";
    Regex regNum = new Regex("^[-]?([0-9])+$");
    #endregion
    
    #region Constructors
    public IntegerAttributeEditor()
    {
        InitializeComponent();
    }
    
    public IntegerAttributeEditor(ListViewItem lvItem)
    : this()
    {
        this.lvSelectedItem = lvItem;
    }
    #endregion
    
    #region Events
    
    /// <summary>
    /// Loads the data
    /// </summary>
    /// <param name="sender"></param>
    /// <param name="e"></param>
    private void IntegerAttributeEditor_Load(object sender, EventArgs e)
    {
        if (lvSelectedItem != null)
        {
            lblSelectedAttr.Text = lvSelectedItem.SubItems[0].Text;
            txtAttrValue.Text = lvSelectedItem.SubItems[2].Text;
            OrigIntAttrValue = lvSelectedItem.SubItems[2].Text;
        }
    }

    private void btnOk_Click(object sender, EventArgs e)
    {
        if (lblSelectedAttr.Text.Trim().Equals("badPasswordTime") ||
           lblSelectedAttr.Text.Trim().Equals("lockoutTime") ||
           lblSelectedAttr.Text.Trim().Equals("maxStorage") ||
           lblSelectedAttr.Text.Trim().Equals("msDS-Cached-Membership-Time-Stamp") ||
           lblSelectedAttr.Text.Trim().Equals("pwdLastSet") ||
           lblSelectedAttr.Text.Trim().Equals("uSNChanged") ||
           lblSelectedAttr.Text.Trim().Equals("uSNCreated") ||
           lblSelectedAttr.Text.Trim().Equals("uSNDSALastObjRemoved") ||
           lblSelectedAttr.Text.Trim().Equals("uSNSource") ||
           lblSelectedAttr.Text.Trim().Equals("uSNLastObjRem"))
        {
            IntAttrValue = "0";
            this.DialogResult = DialogResult.OK;
            this.Close();
        }
        else if (!ValidateData())
        {
            return;
        }

        IntAttrValue = txtAttrValue.Text;
        this.DialogResult = DialogResult.OK;
        this.Close();

    }

    private void btnClear_Click(object sender, EventArgs e)
    {
        txtAttrValue.Text = "<not set>";
    }
    
    private void btnCancel_Click(object sender, EventArgs e)
    {
        this.Close();
    }
    #endregion
    
    #region helper functions
    
    /// <summary>
    /// Validate the data
    /// </summary>
    /// <returns></returns>
    private bool ValidateData()
    {
        string value = this.txtAttrValue.Text.Trim();

        if (value.Trim().Equals("<not set>"))
        {
            return true;
        }

        if (this.Text.Trim().Equals("Large Integer Attribute Editor"))
        {
            if (value.Length > 19 || value.Length == 0)
            {
                string sMsg =
                "The value entered can contain only digits between 0 and 9.\n " +
                "If a negative value is desired a minus sign can be placed\n " +
                "as the first character and value should not exceed more than 19 characters";
                MessageBox.Show(
                this,
                sMsg,
                CommonResources.GetString("Caption_Console"),
                MessageBoxButtons.OK,
                MessageBoxIcon.Warning);
                return false;
            }
        }
        else if (value.Length > 10 || value.Length == 0)
        {
            MessageBox.Show(this,
            "The value entered can contain only digits between 0 and 9.\nIf a negative value is desired a minus sign can be placed \nas the first character and value should not exceed more than 10 characters",
            CommonResources.GetString("Caption_Console"), MessageBoxButtons.OK, MessageBoxIcon.Warning);
            return false;
        }

        if ((value.Contains("-") && !value.StartsWith("-")) || !regNum.IsMatch(value))
        {
            MessageBox.Show(this,
            "The value entered can contain only digits between 0 and 9.\nIf a negative value is desired a minus sign can be placed \nas the first character.",
            CommonResources.GetString("Caption_Console"), MessageBoxButtons.OK, MessageBoxIcon.Warning);
            return false;
        }

        return true;
    }
    #endregion
}
}
