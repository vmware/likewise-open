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
using Likewise.LMC.Plugins.ServiceManagerPlugin.Properties;

namespace Likewise.LMC.Plugins.ServiceManagerPlugin
{
    public partial class ServicesComputerOptionsPage : Form
    {
        #region Class Data

        public int sRestartMunites = 0;
        public string sRebootMsg = string.Empty;

        private string _hostname = string.Empty;
        private string _servicename = string.Empty;

        #endregion

        #region COnstructors

        public ServicesComputerOptionsPage(string hostname, string servicename)
        {
            InitializeComponent();

            this._hostname = hostname;
            this._servicename = servicename;
        }

        #endregion        

        #region Event Hadlers  
     
        private void btnOk_Click(object sender, EventArgs e)
        {
            if(String.IsNullOrEmpty(txtMinutes.Text.Trim()))
            {
                MessageBox.Show("Please enter a positive integer", CommonResources.GetString("Caption_Console"),
                                MessageBoxButtons.OK);
                return;
            }

            sRestartMunites = int.Parse(txtMinutes.Text.Trim());
            sRebootMsg = richTextBoxMsg.Text.Trim();

            Close();
        }

        private void richTextBoxMsg_TextChanged(object sender, EventArgs e)
        {
            cbRestart.Checked = true;
            sRebootMsg = richTextBoxMsg.Text.Trim();
        }

        private void cbRestart_CheckedChanged(object sender, EventArgs e)
        {
            if (cbRestart.Checked)
            {
                if (string.IsNullOrEmpty(sRebootMsg))
                    richTextBoxMsg.Text = string.Format(Resources.sRebootMsg, _hostname, _servicename, _hostname, _hostname);
                else
                    richTextBoxMsg.Text = sRebootMsg;
            }
            else
                richTextBoxMsg.Text = "";
        }

        private void btnCancel_Click(object sender, EventArgs e)
        {
            Close();
        }      

        private void ServicesComputerOptionsPage_Load(object sender, EventArgs e)
        {
            if (!String.IsNullOrEmpty(sRebootMsg))
            {
                cbRestart.Checked = true;
                richTextBoxMsg.Text = sRebootMsg;
            }
            txtMinutes.Text = sRestartMunites.ToString();
        }

        #endregion
    }
}