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
public partial class ADRenameDlg : Form
{
    #region Class Data
    IPlugInContainer IPlugInContainer;
    
    public string rename = "";
    public string objname = "";
    private string obj_type = "";
    #endregion
    
    #region Constructors
    public ADRenameDlg()
    {
        InitializeComponent();
    }
    
    public ADRenameDlg(IPlugInContainer container, StandardPage parentPage , string name,string objclass)
    : this()
    {
        this.IPlugInContainer = container;
        objname = name;
        RenametextBox.Text = name;
        obj_type = objclass;
    }
    #endregion
    
    #region Event Handlers
    private void btnOK_Click(object sender, EventArgs e)
    {
        if (RenametextBox.Text.Length > 64)
        {
            if (obj_type.Equals("organizationalUnit", StringComparison.InvariantCultureIgnoreCase))
            {
                MessageBox.Show("LAC cannot complete the rename operation on \n" +
                objname + " " + "A value for the attribute was not in the acceptable range of values \n \n" +
                "Name-related properties on this object might now be out of sync.Contact your system administrator.", CommonResources.GetString("Caption_Console"), MessageBoxButtons.OK, MessageBoxIcon.Error);
                rename = null;
                return;
            }
            else if (obj_type.Equals("user", StringComparison.InvariantCultureIgnoreCase) || obj_type.Equals("group", StringComparison.InvariantCultureIgnoreCase))
            {
                MessageBox.Show("The name you entered is too long.Names cannot contain more than 64 characters.This name willbe shortened to 64 \n" +
                "characters", CommonResources.GetString("Caption_Console"), MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
            }
            
            
        }
        if (RenametextBox.Text.Length > 64 && obj_type.Equals("group", StringComparison.InvariantCultureIgnoreCase))
        {
            rename = RenametextBox.Text.Trim().Substring(0, 64);
        }
        else if (RenametextBox.Text.Length > 64 && obj_type.Equals("user", StringComparison.InvariantCultureIgnoreCase))
        {
            rename = RenametextBox.Text.Trim().Substring(0, 64);
        }
        else
        {
            rename = RenametextBox.Text.Trim();
        }
        this.Close();
    }
    
    
    private void btkcancel_Click(object sender, EventArgs e)
    {
        rename = null;
        this.Close();
    }
    
    private void RenametextBox_TextChanged(object sender, EventArgs e)
    {
        if (!RenametextBox.Text.Trim().Equals("") && !RenametextBox.Text.Trim().Equals(objname))
        {
            btnOK.Enabled = true;
        }
        else
        {
            btnOK.Enabled = false;
        }
    }
    
    #endregion
}
}
