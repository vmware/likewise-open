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
using Likewise.LMC.LMConsoleUtils;
using Likewise.LMC.NETAPI;
using Likewise.LMC.AuthUtils;
using Likewise.LMC.Plugins.FileBrowser;

namespace Likewise.LMC.Plugins.FileBrowser
{
    public partial class ShareGeneralEditPage : MPPage, IDirectoryPropertiesPage
    {
        #region Constructors

        public ShareGeneralEditPage()
        {
            InitializeComponent();
            pageID = "ShareGeneralEditPage";
            SetPageTitle("General");
        }

        #endregion

        #region IDirectoryPropertiesPage Members

        public void SetData(CredentialEntry ce, string sharename, object shareinfo)
        {
            try
            {
                if (shareinfo != null)
                {
                    string[] Shareinfo = shareinfo as string[];
                    if (Shareinfo != null)
                    {
                        this.textBoxSharename.Text = Shareinfo[1];
                        this.textBoxPath.Text = Shareinfo[2];
                        this.textBoxComment.Text = Shareinfo[3];
                        if (Shareinfo[5] == "-1")
                        {
                            radioButtonMaxAllowed.Checked = true;
                            radioButtonAllowedNum.Checked = false;
                        }
                        else
                        {
                            radioButtonAllowedNum.Checked = true;
                            radioButtonMaxAllowed.Checked = false;
                            numericUpDown.Enabled = true;

                            this.numericUpDown.ValueChanged -= new System.EventHandler(this.numericUpDown_ValueChanged);
                            numericUpDown.Value = Convert.ToDecimal(Shareinfo[5]);
                            this.numericUpDown.ValueChanged += new System.EventHandler(this.numericUpDown_ValueChanged);
                        }
                    }
                }

                this.ParentContainer.DataChanged = false;
                this.ParentContainer.btnApply.Enabled = false;
            }
            catch (Exception e)
            {
                container.ShowError(e.Message);
            }
        }

        public bool OnApply()
        {
            return true;
        }

        #endregion

        #region Events

        private void radioButtonAllowedNum_CheckedChanged(object sender, EventArgs e)
        {
            numericUpDown.Enabled = radioButtonAllowedNum.Checked;
        }        

        private void numericUpDown_ValueChanged(object sender, EventArgs e)
        {
            this.ParentContainer.DataChanged = true;
            this.ParentContainer.btnApply.Enabled = true;
        }
        
        private void textBoxComment_TextChanged(object sender, EventArgs e)
        {
            this.ParentContainer.DataChanged = true;
            this.ParentContainer.btnApply.Enabled = true;
        }

        #endregion
    }
}
