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
using Likewise.LMC.Utilities;
using Likewise.LMC.Registry;

namespace Likewise.LMC.Plugins.RegistryViewerPlugin
{
    public partial class RegistryAddSubKeyDlg : Form
    {
        #region Class Data

        private string sKeyName = string.Empty;
        private object SubKey = null;
        private bool bIsKeyObject = false;

        #endregion

        #region Access Specifiers

        public string KeyName
        {
            get
            {
                return sKeyName;
            }
			set
			{
				sKeyName = value;
			}
        }

        #endregion

        #region Constructors

        public RegistryAddSubKeyDlg()
        {
            InitializeComponent();
        }

        public RegistryAddSubKeyDlg(string oldKey, bool bIsKey, object sKey)
            : this()
        {
            if (!String.IsNullOrEmpty(oldKey))
                this.txtKeyname.Text = oldKey;
            else
                this.txtKeyname.Text = "New Key #1";

            if (bIsKey)
                this.label1.Text = "Registry key name";
            else
                this.label1.Text = "Registry value name";

            bIsKeyObject = bIsKey;
            SubKey = sKey;
        }

        #endregion

        #region Events

        private void txtKeyname_TextChanged(object sender, EventArgs e)
        {
            btnOk.Enabled = !String.IsNullOrEmpty(txtKeyname.Text.Trim());
        }

        private void btnOk_Click(object sender, EventArgs e)
        {
            if (!ValidateInputData())
                return;

            this.DialogResult = DialogResult.OK;
            sKeyName = txtKeyname.Text.Trim();

            this.Close();
        }

        private void btnCancel_Click(object sender, EventArgs e)
        {
            this.DialogResult = DialogResult.Cancel;
            this.Close();
        }

        #endregion

        #region Helper functions

        private bool ValidateInputData()
        {
            string sMsg = string.Empty;
            if (bIsKeyObject)
            {
                if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
                    return !RegistryHelper.CheckDuplicateKey(SubKey, txtKeyname.Text.Trim());
            }
            else
            {
                if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
                    return !RegistryHelper.CheckDuplicateValue(SubKey, txtKeyname.Text.Trim());
            }

            return true;
        }

        #endregion
    }
}