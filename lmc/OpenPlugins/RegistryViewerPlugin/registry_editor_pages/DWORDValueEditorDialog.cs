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
    public partial class DWORDValueEditorDialog : Form
    {
        #region Class Data

        private SubKeyValueInfo ValueInfo = null;
        private RegistryValueInfo regValueInfo = null;
        private string sInputVal = string.Empty;
        private bool bIsAdd = false;

        #endregion

        #region Properties       

        #endregion

        #region Constructors

        public DWORDValueEditorDialog()
        {
            InitializeComponent();
        }

        public DWORDValueEditorDialog(object valueInfo, bool bIsAdd)
            : this()
        {
            if (valueInfo is SubKeyValueInfo)
                this.ValueInfo = valueInfo as SubKeyValueInfo;
            else
                this.regValueInfo = valueInfo as RegistryValueInfo;

            this.txtValuename.ReadOnly = !bIsAdd;
            this.bIsAdd = bIsAdd;            

            SetInputData();
        }

        #endregion

        #region Event Handlers

        private void btnOk_Click(object sender, EventArgs e)
        {  
            if (!ValidateInputData())
                return;

            object dataBuf = null;
            if (ValueInfo != null)
                ValueInfo.sValue = txtValuename.Text.Trim();
            else
                regValueInfo.pValueName = txtValuename.Text.Trim();

            if (String.IsNullOrEmpty(txtValuedata.Text.Trim()))
                dataBuf = 0;
            else
            {
                if (radioButtonDecimal.Checked)
                {
                    uint value = 0;
                    uint.TryParse(txtValuedata.Text.Trim(), out value);
                    dataBuf = (int)value;
                }
                else
                {
                    if (ValueInfo != null)
                    {
                        if (ValueInfo.RegDataType == LWRegistryValueKind.REG_DWORD)
                            dataBuf = (int)UInt32.Parse(txtValuedata.Text.Trim(), System.Globalization.NumberStyles.HexNumber);
                        else
                            dataBuf = (long)UInt64.Parse(txtValuedata.Text.Trim(), System.Globalization.NumberStyles.HexNumber);
                    }
                    else
                    {
                        if (regValueInfo.pType == (ulong)LWRegistryValueKind.REG_DWORD)
                            dataBuf = (int)UInt32.Parse(txtValuedata.Text.Trim(), System.Globalization.NumberStyles.HexNumber);
                        else
                            dataBuf = (long)UInt64.Parse(txtValuedata.Text.Trim(), System.Globalization.NumberStyles.HexNumber);
                    }
                }
            }

            if (ValueInfo != null)
                ValueInfo.sDataBuf = dataBuf;
            else
                regValueInfo.bDataBuf = dataBuf;

            this.DialogResult = DialogResult.OK;
            this.Close();
        }

        private void btnCancel_Click(object sender, EventArgs e)
        {
            this.Close();
        }

        private void radioButtonDecimal_CheckedChanged(object sender, EventArgs e)
        {
            if (radioButtonDecimal.Checked && !String.IsNullOrEmpty(txtValuedata.Text.Trim()))
            {
                txtValuedata.MaxLength = 10;
                 try
                {
                    if (ValueInfo != null)
                        txtValuedata.Text = (ValueInfo.RegDataType == LWRegistryValueKind.REG_DWORD) ?
                            UInt32.Parse(txtValuedata.Text, System.Globalization.NumberStyles.HexNumber).ToString() :
                            UInt64.Parse(txtValuedata.Text, System.Globalization.NumberStyles.HexNumber).ToString();
                    else
                        txtValuedata.Text = (regValueInfo.pType == (ulong)RegistryApi.REG_DWORD) ?
                            UInt32.Parse(txtValuedata.Text, System.Globalization.NumberStyles.HexNumber).ToString() :
                            UInt64.Parse(txtValuedata.Text, System.Globalization.NumberStyles.HexNumber).ToString();
                }
                catch{}
            }
        }

        private void radioButtonHexno_CheckedChanged(object sender, EventArgs e)
        {
            if (radioButtonHexno.Checked && !String.IsNullOrEmpty(txtValuedata.Text.Trim()))
            {
                string sOutValue = string.Empty;
                if (!RegistryHelper.CheckMaxValue(txtValuedata.Text, out sOutValue,
                 (ValueInfo != null) ? ValueInfo.RegDataType : (LWRegistryValueKind)regValueInfo.pType))
                {
                    radioButtonDecimal.Checked = true;
                    return;
                }
                else
                    txtValuedata.Text = sOutValue;

                txtValuedata.MaxLength = 8;
                if (ValueInfo != null)
                    txtValuedata.Text =
                        (ValueInfo.RegDataType == LWRegistryValueKind.REG_DWORD) ?
                        String.Format("{0:X2}", UInt32.Parse(txtValuedata.Text)) :
                        String.Format("{0:X2}", UInt64.Parse(txtValuedata.Text));
                else
                    txtValuedata.Text =
                        (regValueInfo.pType == (ulong)RegistryApi.REG_DWORD) ?
                        String.Format("{0:X2}", UInt32.Parse(txtValuedata.Text)) :
                        String.Format("{0:X2}", UInt64.Parse(txtValuedata.Text));
                sInputVal = txtValuedata.Text;
            }
        }

        private void txtValuedata_KeyPress(object sender, KeyPressEventArgs e)
        {
            if (radioButtonDecimal.Checked)
            {
                if ((Char.IsDigit(e.KeyChar)) || (e.KeyChar == 8))
                    e.Handled = false;
                else
                    e.Handled = true;
            }
            else if (radioButtonHexno.Checked)
            {
                if ((Char.IsDigit(e.KeyChar)) ||                  
                    (e.KeyChar == 8) ||
                    (e.KeyChar.ToString().Equals("A", StringComparison.InvariantCultureIgnoreCase)) ||
                    (e.KeyChar.ToString().Equals("B", StringComparison.InvariantCultureIgnoreCase)) ||
                    (e.KeyChar.ToString().Equals("C", StringComparison.InvariantCultureIgnoreCase)) ||
                    (e.KeyChar.ToString().Equals("D", StringComparison.InvariantCultureIgnoreCase)) ||
                    (e.KeyChar.ToString().Equals("E", StringComparison.InvariantCultureIgnoreCase)) ||
                    (e.KeyChar.ToString().Equals("F", StringComparison.InvariantCultureIgnoreCase)))
                    e.Handled = false;
                else
                    e.Handled = true;
            }
        }

        private void txtValuedata_KeyUp(object sender, KeyEventArgs e)
        {
            sInputVal = txtValuedata.Text.Trim();
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

            if (ValueInfo != null)
            {
                txtValuename.Text = ValueInfo.sValue;
                txtValuedata.Text = sInputVal = String.Format("{0:X}", (uint)(int)ValueInfo.sDataBuf);
            }
            else
            {
                txtValuename.Text = regValueInfo.pValueName;
                string sData = regValueInfo.bDataBuf as string;
                if (!string.IsNullOrEmpty(sData) && sData.IndexOf('(') >= 0)
                {
                    sInputVal = sData.Substring(sData.IndexOf('x') + 1, sData.IndexOf('(') - 2);
                    txtValuedata.Text = String.Format("{0:X}", sInputVal);
                }
                else
                    txtValuedata.Text = sInputVal = String.Format("{0:X}", sData);               
            }
        }

        private string FormatDWORDData()
        { 
            string sOvalue = string.Empty;
           
            if (radioButtonDecimal.Checked)
            {
                string sTemp = String.Format("{0:X}", UInt32.Parse(txtValuedata.Text));
                sOvalue = string.Concat("0x", sTemp, "(", txtValuedata.Text.Trim(), ")");
            }
            else
            {
                uint iTemp = UInt32.Parse(txtValuedata.Text, System.Globalization.NumberStyles.HexNumber);
                sOvalue = string.Concat("0x", txtValuedata.Text.Trim(), "(", iTemp.ToString(), ")");
            }

            return sOvalue;
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

            string sTemp = string.Empty;

            if (radioButtonDecimal.Checked)
            {
                string sOutValue = string.Empty;
                if (!RegistryHelper.CheckMaxValue(
                            String.IsNullOrEmpty(txtValuedata.Text) ? "0" : txtValuedata.Text,
                            out sOutValue,
                            (ValueInfo != null) ? ValueInfo.RegDataType : (LWRegistryValueKind)regValueInfo.pType))
                    return false;
                else
                    txtValuedata.Text = sOutValue;
            }

            return true;
        }

        #endregion          
        
    }
}