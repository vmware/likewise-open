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
public partial class StringAttributeEditor : Form
{
    #region Class Data
    ListViewItem lvSelectedItem;
    public string stringAttrValue = "";
    public string origstringAttrValue = "";
    #endregion
    
    #region Constructors
    public StringAttributeEditor()
    {
        InitializeComponent();
    }
    
    public StringAttributeEditor(ListViewItem lvItem)
    : this()
    {
        this.lvSelectedItem = lvItem;
    }
    #endregion
    
    #region Events
    
    /// <summary>
    /// Loads the listbox with the attribute value list
    /// </summary>
    /// <param name="sender"></param>
    /// <param name="e"></param>
    private void StringAttributeEditor_Load(object sender, EventArgs e)
    {
        if (lvSelectedItem != null)
        {
            lblselectedAttr.Text = lvSelectedItem.SubItems[0].Text;
            txtAttrValue.Text = lvSelectedItem.SubItems[2].Text;
            origstringAttrValue = lvSelectedItem.SubItems[2].Text;
        }
    }
    
    private void btnClear_Click_1(object sender, EventArgs e)
    {
        txtAttrValue.Text = "<not set>";
    }
    
    private void btnCancel_Click(object sender, EventArgs e)
    {
        this.Close();
    }
    
    /// <summary>
    /// Gives the specified attribute value
    /// </summary>
    /// <param name="sender"></param>
    /// <param name="e"></param>
    private void btnOk_Click_1(object sender, EventArgs e)
    {
        if (txtAttrValue.Text.Trim() == "")
        {
            MessageBox.Show(
            this,
            "Value can not be empty",
            CommonResources.GetString("Caption_Console"),
            MessageBoxButtons.OK,
            MessageBoxIcon.Asterisk);
            return;
        }
        else
        {
            stringAttrValue = txtAttrValue.Text;
            this.DialogResult = DialogResult.OK;
            this.Close();
        }
    }
    #endregion
}
}
