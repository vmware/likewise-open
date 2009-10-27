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
using Likewise.LMC.LMConsoleUtils;
using Likewise.LMC.ServerControl;

namespace Likewise.LMC.Plugins.ADUCPlugin
{
public partial class ADDeleteComputerConfirmDialog : Form
{
    public bool bRb1Checked = false;
    public bool bRb2Checked = false;
    public bool bRb3Checked = false;
    
    public ADDeleteComputerConfirmDialog(ADUCDirectoryNode dirnode)
    {
        InitializeComponent();
        
        string lblText = label1.Text;
        label1.Text = string.Format(lblText, dirnode.Text);
    }
    
    private void btnDelete_Click(object sender, EventArgs e)
    {
        if (radioButton1.Checked)
        {
            Logger.Log("radioButton1.Checked" + radioButton1.Checked);
            string UserMsg = "Deleting this object should only be used to delete domain controller that is permanently offline. \n"+
            "Use the Active Directory Installation Wizard (DCPROMO) to demote a domain controller that is still functioning.";
            ShowUserMessage(UserMsg);
            bRb1Checked = radioButton1.Checked;
        }
        else if (radioButton2.Checked)
        {
            Logger.Log("radioButton2.Checked" + radioButton2.Checked);
            string UserMsg = "Deleting this object not required to mange Active Directory replication and may inhibit its operation. \n" +
            "Please see your Active Directory documentation for more information on managing Active Directory replication.";
            ShowUserMessage(UserMsg);
            bRb2Checked = radioButton2.Checked;
        }
        else if (radioButton3.Checked)
        {
            Logger.Log("radioButton3.Checked" + radioButton3.Checked);
            bRb3Checked = radioButton3.Checked;
        }
        this.DialogResult = DialogResult.OK;
        this.Close();
        return;
    }
    
    private void ShowUserMessage(string msg)
    {
        MessageBox.Show(this, msg, CommonResources.GetString("Caption_Console"), MessageBoxButtons.OK);
        return;
    }
    
    private void btnCancel_Click(object sender, EventArgs e)
    {
        this.Close();
    }
    
    private void radioButton1_CheckedChanged(object sender, EventArgs e)
    {
        bRb1Checked = radioButton1.Checked;
    }
    
    private void radioButton2_CheckedChanged(object sender, EventArgs e)
    {
        bRb2Checked = radioButton2.Checked;
    }
    
    private void radioButton3_CheckedChanged(object sender, EventArgs e)
    {
        bRb3Checked = radioButton3.Checked;
    }
}
}
