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
    public partial class MultiStringValueEditorDialog : Form
    {
        #region Class Data

        private SubKeyValueInfo ValueInfo = null;
        private RegistryValueInfo regValueInfo = null;
        private StringBuilder sbInuputData = new StringBuilder();
        private RegistryViewerPlugin plugin = null;
        private bool bIsAdd = false;
        private object sDataBuf = null;

        #endregion

        #region Properties
      
        #endregion

        #region Constructors

        public MultiStringValueEditorDialog()
        {
            InitializeComponent();
        }

        public MultiStringValueEditorDialog(object valueInfo, bool IsAdd, RegistryViewerPlugin _plugin)
            : this()
        {
            if (valueInfo is SubKeyValueInfo)
                this.ValueInfo = valueInfo as SubKeyValueInfo;
            else
                this.regValueInfo = valueInfo as RegistryValueInfo;

            this.txtValuename.ReadOnly = !IsAdd;
            this.bIsAdd = IsAdd;
            this.plugin = _plugin;

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
                ValueInfo.sValue = txtValuename.Text.Trim();
                ValueInfo.sDataBuf = String.IsNullOrEmpty(richTextBoxValueData.Text.Trim()) ? new string[] { "(value not set)" } : FormatMulitString();
            }
            else
            {
                regValueInfo.pValueName = txtValuename.Text.Trim();
                regValueInfo.bDataBuf = String.IsNullOrEmpty(richTextBoxValueData.Text.Trim()) ? new string[] { "(value not set)" } : FormatMulitString();
            }
            
            this.DialogResult = DialogResult.OK;

            this.Close();
        }

        private void btnCancel_Click(object sender, EventArgs e)
        {
            this.Close();
        }

        private void txtValuename_TextChanged(object sender, EventArgs e)
        {
            btnOk.Enabled = !String.IsNullOrEmpty(txtValuename.Text.Trim());
        }

        #endregion

        #region Helper functions

        private void SetInputData()
        {
            if (bIsAdd)
            {
                this.txtValuename.BorderStyle = BorderStyle.Fixed3D;
                btnOk.Enabled = false;
                return;
            }

            txtValuename.Text = ValueInfo != null ? ValueInfo.sValue : regValueInfo.pValueName;

            if (ValueInfo != null)
                RegistryInteropWrapperWindows.Win32RegKeyValueData(ValueInfo.sParentKey, ValueInfo.sValue, out sDataBuf);
            else
            {
                RegistryInteropWrapper.ApiRegGetValue(plugin.handle.Handle, regValueInfo, out sDataBuf);
                sDataBuf = new ASCIIEncoding().GetString(sDataBuf as byte[]).Split('\n');
            }

            string[] sStringdata = sDataBuf as string[];
            if (sStringdata != null && sStringdata.Length != 0)
            {
                foreach (string data in sStringdata)
                {
                    sbInuputData.AppendLine(data);
                }
            }

            richTextBoxValueData.Text = sbInuputData.ToString().Replace("\r\n\r\n", "\r\n");
        }

        private string[] FormatMulitString()
        {
            string sTemp = richTextBoxValueData.Text;
            string[] sTempArray = String.IsNullOrEmpty(richTextBoxValueData.Text.Trim()) ? new string[] { "" } : sTemp.Split(new string[] { "\n" }, StringSplitOptions.None);
            string[] sValueBuf = new string[1];
            bool firstTime = true;
            int index = 0;
            bool bIsEmptyFound = false;
            // For loop to remove empty lines in the multistring
            // Need to alert the user that the empty string is found and removed
            for (int i = 0; i < sTempArray.Length; i++)
            {
                if (sTempArray[i].CompareTo("") == 0 && i != (sTempArray.Length - 1))
                {
                    bIsEmptyFound = true;
                    continue;
                }
                else
                {
                    if (firstTime)
                    {
                        sValueBuf[index++] = sTempArray[i];
                        firstTime = false;
                    }
                    else
                    {
                        Array.Resize<string>(ref sValueBuf, index + 1);
                        sValueBuf[index++] = sTempArray[i];
                    }
                }
            }           

            if (bIsEmptyFound)
                MessageBox.Show("Data of type REG_MULTI_SZ cannot contain empty strings.\nRegistry Viewer will remove all empty strings found.",
                                "Warning",
                                MessageBoxButtons.OK,
                                MessageBoxIcon.Error);

            return sValueBuf;
        }

        private bool ValidateInputData()
        {
            if (string.IsNullOrEmpty(txtValuename.Text))
            {
                RegistryHelper.DisplayErrorNotification();
                return false;
            }

            if (bIsAdd && ValueInfo != null && RegistryHelper.CheckDuplicateValue(ValueInfo.sParentKey, txtValuename.Text.Trim()))
                return false;

            return true;
        }

        #endregion      
        
    }
}