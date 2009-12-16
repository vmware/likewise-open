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
using System.Collections;
using Likewise.LMC.Utilities;
using Likewise.LMC.ServerControl;
using Likewise.LMC.LDAP;

namespace Likewise.LMC.Plugins.ADUCPlugin
{
public partial class TimeAttributeEditor : Form
{
    #region Class Data
    ListViewItem lvSelectedItem;
    
    public string sTimeAttrValue = "";
    public string sDateAttrValue = "";
    #endregion
    
    #region Constructors
    public TimeAttributeEditor()
    {
        InitializeComponent();
    }
    public TimeAttributeEditor(ListViewItem lvItem)
    : this()
    {
        this.lvSelectedItem = lvItem;
    }
    #endregion
    
    #region Events
    private void btnCancel_Click(object sender, EventArgs e)
    {
        this.Close();
    }
    
    /// <summary>
    /// Loads the listbox with the attribute value list
    /// </summary>
    /// <param name="sender"></param>
    /// <param name="e"></param>
    private void TimeAttributeEditor_Load(object sender, EventArgs e)
    {
        try
        {
            if (lvSelectedItem != null)
            {
                string sTime = "";
                lblSelectedValue.Text = lvSelectedItem.SubItems[0].Text;
                string[] sTempArr= lvSelectedItem.SubItems[2].Text.ToString().Split(' ');
                dtDate.Value = Convert.ToDateTime(sTempArr[0]);
                if (sTempArr.Length > 1)
                {
                    for (int i = 1; i < sTempArr.Length; i++)
                    {
                        sTime += sTempArr[i];
                    
}
                
}

                    
                    if (!sTime.Trim().Equals(string.Empty))
                    {
                        dtTime.Value = Convert.ToDateTime(sTime);
                    }
                }
            }
            catch (Exception )
            {
                //This is to handle the junk data at initial loading
                //Later we need to log this one
            }
            
        }
        
        private void btnOK_Click(object sender, EventArgs e)
        {
            if (dtDate.Checked)
            {
                sDateAttrValue = dtDate.Value.ToShortDateString().ToString();
            }
            if (dtTime.Checked)
            {
                sTimeAttrValue = dtTime.Value.ToLongTimeString().ToString();
            }
            this.DialogResult = DialogResult.OK;
            this.Close();
        }
        #endregion
        
    }
}
