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
using Likewise.LMC.Registry;

namespace Likewise.LMC.Plugins.RegistryViewerPlugin
{
    public partial class StringEditorDialog : Form
    {
        #region ClassData

        private SubKeyValueInfo ValueInfo = null;
        private RegistryValueInfo regValueInfo = null;
        private bool bIsAdd = false;
       
        #endregion

        #region Properties
               
        #endregion

        #region Constructors

        public StringEditorDialog()
        {
            InitializeComponent();
        }

        public StringEditorDialog(object valueInfo, bool IsAdd)
            : this()
        {
            if (valueInfo is SubKeyValueInfo)
                this.ValueInfo = valueInfo as SubKeyValueInfo;
            else
                this.regValueInfo = valueInfo as RegistryValueInfo;

            this.txtValueName.ReadOnly = !IsAdd;
            this.bIsAdd = IsAdd;

            SetInputData();
        }

        #endregion
       
        #region Event Handlers

        private void btnOk_Click(object sender, EventArgs e)
        {
            if (!ValidateInputData())
                return;

            if (ValueInfo != null)
            {
                ValueInfo.sValue = txtValueName.Text.Trim();
                ValueInfo.sData = String.IsNullOrEmpty(txtValueData.Text.Trim()) ? "(value not set)" : txtValueData.Text.Trim();
            }
            else
            {
                regValueInfo.pValueName = txtValueName.Text.Trim();
                regValueInfo.bDataBuf = String.IsNullOrEmpty(txtValueData.Text.Trim()) ? "(value not set)" : txtValueData.Text.Trim();
            }

            this.DialogResult = DialogResult.OK;
            this.Close();
        }

        private void btnCancel_Click(object sender, EventArgs e)
        {
            this.Close();
        }     

        private void txtValueName_TextChanged(object sender, EventArgs e)
        {
            if ((ValueInfo != null && ValueInfo.RegDataType == LWRegistryValueKind.REG_EXPAND_SZ) ||
                (regValueInfo != null && regValueInfo.pType == (ulong)RegistryApi.REG_EXPAND_SZ))
                btnOk.Enabled = !String.IsNullOrEmpty(txtValueName.Text.Trim());
        }

        private void txtValueData_TextChanged(object sender, EventArgs e)
        {
            txtValueData.Text = txtValueData.Text;
        }

        #endregion
               
        #region Helper functions

        private void SetInputData()
        {
            if (bIsAdd)
            {
                txtValueData.Text = txtValueName.Text = string.Empty;
                this.txtValueName.BorderStyle = BorderStyle.Fixed3D;

                if ((ValueInfo != null && ValueInfo.RegDataType == LWRegistryValueKind.REG_EXPAND_SZ) ||
                    (regValueInfo != null && regValueInfo.pType == (ulong)RegistryApi.REG_EXPAND_SZ))
                    btnOk.Enabled = false;

                return;
            }

            txtValueName.Text = ValueInfo != null ? ValueInfo.sValue : regValueInfo.pValueName;
            txtValueData.Text = ValueInfo != null ? ValueInfo.sData : regValueInfo.bDataBuf.ToString();
        }

        private bool ValidateInputData()
        {
            //if (bIsAdd && ValueInfo != null && RegistryHelper.CheckDuplicateValue(ValueInfo.sParentKey, txtValueName.Text.Trim()))
                //return false;

            if (ValueInfo != null)
            {
                if (!ValueInfo.IsDefaultValue && string.IsNullOrEmpty(txtValueName.Text))
                {
                    RegistryHelper.DisplayErrorNotification();
                    return false;
                }
            }

            return true;
        }

        #endregion   
    }
}