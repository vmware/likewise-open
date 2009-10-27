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
using System.Text.RegularExpressions;
using Likewise.LMC.Registry;

namespace Likewise.LMC.Plugins.RegistryViewerPlugin
{
    public partial class BinaryValueEditorDialog : Form
    {
        #region Class Data

        private SubKeyValueInfo ValueInfo = null;
        private RegistryValueInfo regValueInfo = null;
        private RegistryViewerPlugin plugin = null;
        private string sOutValue = string.Empty;        
        private bool bIsAdd = false;
        private string sKey = string.Empty;

        private Regex regEx = new Regex("^[0-9a-fA-F]{2}$", RegexOptions.Compiled);

        private const int byteLength = 8;
        private const int columnLength = 17;
        private int numOfCells = 0;

        #endregion

        #region Properties
       
        #endregion

        #region Constructors

        public BinaryValueEditorDialog()
        {
            InitializeComponent();
        }

        public BinaryValueEditorDialog(object valueInfo, bool IsAdd,
                                       RegistryViewerPlugin _plugin, string key)
            : this()
        {
            Logger.Log("BinaryValueEditorDialog.BinaryValueEditorDialog", Logger.RegistryViewerLoglevel);

            if (valueInfo is SubKeyValueInfo)
            {
                this.ValueInfo = valueInfo as SubKeyValueInfo;
                this.LWDataBinarydata.EditMode = DataGridViewEditMode.EditOnEnter;
            }
            else
            {
                this.regValueInfo = valueInfo as RegistryValueInfo;
                this.plugin = _plugin;
                this.sKey = key;
                this.LWDataBinarydata.EditMode = DataGridViewEditMode.EditProgrammatically;
            }
           
            this.txtValuename.ReadOnly = !IsAdd;
            this.bIsAdd = IsAdd;       

            foreach (DataGridViewColumn column in LWDataBinarydata.Columns)
            {
                column.CellTemplate.KeyEntersEditMode(new KeyEventArgs(Keys.A | Keys.B | Keys.C | Keys.D | Keys.E | Keys.F));                  
            }           

            SetInputData();
        }

        #endregion

        #region Helper functions

        private bool ValidateInputData()
        {
            if (string.IsNullOrEmpty(txtValuename.Text.Trim()))
            {
                RegistryHelper.DisplayErrorNotification();
                return false;
            }

            if (bIsAdd && ValueInfo != null && RegistryHelper.CheckDuplicateValue(ValueInfo.sParentKey, txtValuename.Text.Trim()))
                return false;

            if (ValueInfo != null)
                ValueInfo.sValue = txtValuename.Text.Trim();
            else
                regValueInfo.pValueName = txtValuename.Text.Trim();

            return true;
        }

        private void SetInputData()
        {
            object data = null;

            try
            {
                Logger.Log("BinaryValueEditorDialog.SetInputData - Updating the binary value", Logger.RegistryViewerLoglevel);

                if (bIsAdd)
                {
                    this.txtValuename.BorderStyle = BorderStyle.Fixed3D;
                    this.btnOk.Enabled = false;

                    if (ValueInfo != null)
                        ValueInfo.RegDataType = LWRegistryValueKind.REG_BINARY;
                    else
                        regValueInfo.pType = (ulong)RegistryApi.REG_BINARY;

                    LWDataBinarydata.Rows.Add(new string[] { "0000" });
                    return;
                }

                txtValuename.Text = ValueInfo != null ? ValueInfo.sValue : regValueInfo.pValueName;

                // Note this method can be invoked in two ways
                // 1. Double click a REG_BINARY key value
                // 2. Right click any Key and select Modify Binary Value
                // We need to find the key type and handle different scenarios
                if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
                {
                    ValueInfo.RegDataType = RegistryInteropWrapperWindows.Win32RegKeyValueKind(ValueInfo.sParentKey, ValueInfo.sValue);
                    RegistryInteropWrapperWindows.Win32RegKeyValueData(ValueInfo.sParentKey, ValueInfo.sValue, out data);

                    if (data == null && ValueInfo.RegDataType == LWRegistryValueKind.REG_RESOURCE_LIST)
                    {
                        string[] sKey = ValueInfo.sParentKey.ToString().Split(new char[] { '\\' } , 2);
                        data = RegistryInteropWrapperWindows.RegGetValue(RegistryInteropWrapperWindows.GetRegistryHive(ValueInfo.hKey), sKey[1], ValueInfo.sValue, out ValueInfo.intDataType);
                    }
                }
                else
                    RegistryInteropWrapper.ApiRegGetValue(plugin.handle.Handle, regValueInfo, out data);

                if (data == null)
                {
                    Logger.Log("BinaryValueEditorDialog.SetInputData - RegistryKey.GetValue returns null", Logger.RegistryViewerLoglevel);
                    LWDataBinarydata.Rows.Add(new string[] { "0000" });
                    return;
                }

                if (ValueInfo != null)
                {
                    switch (ValueInfo.RegDataType)
                    {
                        case LWRegistryValueKind.REG_BINARY:
                        case LWRegistryValueKind.REG_RESOURCE_LIST:
                            ParseBinaryData(data);
                            break;

                        case LWRegistryValueKind.REG_EXPAND_SZ:
                        case LWRegistryValueKind.REG_SZ:
                            ParseStringData(data.ToString() + "\r\n");
                            break;

                        case LWRegistryValueKind.REG_MULTI_SZ:
                            ParseMultiStringData(data);
                            break;

                        case LWRegistryValueKind.REG_DWORD:
                            ParseDWordData(data);
                            break;

                        case LWRegistryValueKind.REG_QUADWORD:
                            ParseQWordData(data);
                            break;

                        case LWRegistryValueKind.REG_NONE:
                            throw new NotImplementedException("Not supported data type");

                        default:
                            break;
                    }
                }
                else
                {
                    switch (regValueInfo.pType)
                    {
                        case (ulong)RegistryApi.REG_BINARY:
                        case (ulong)RegistryApi.REG_FULL_RESOURCE_DESCRIPTOR:
                        case (ulong)RegistryApi.REG_RESOURCE_LIST:
                        case (ulong)RegistryApi.REG_RESOURCE_REQUIREMENTS_LIST:
                            ParseBinaryData(data);
                            break;

                        case (ulong)RegistryApi.REG_SZ:
                        case (ulong)RegistryApi.REG_PLAIN_TEXT:
                        case (ulong)RegistryApi.REG_EXPAND_SZ:
                            ParseStringData(new ASCIIEncoding().GetString(data as byte[]));
                            break;

                        case (ulong)RegistryApi.REG_MULTI_SZ:
                            ParseMultiStringData(data);
                            break;

                        case (ulong)RegistryApi.REG_DWORD:
                            ParseDWordData(data);
                            break;

                        case (ulong)RegistryApi.REG_QWORD:
                            ParseQWordData(data);
                            break;

                        case (ulong)RegistryApi.REG_NONE:
                            throw new NotImplementedException("Not supported data type");

                        default:
                            break;
                    }

                    this.LWDataBinarydata.BeginEdit(true);
                    this.LWDataBinarydata.Rows[0].Selected = true;
                }
                //LWDataBinarydata.AutoResizeColumns(DataGridViewAutoSizeColumnsMode.DisplayedCellsExceptHeader);
            }
            catch (Exception ex)
            {
                Logger.LogException("BinaryValueEditorDialog.SetInputData()", ex);
            }
        }


        private void ParseMultiStringData(object data)
        {
            string[] sTempStrings = null;

            if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
                sTempStrings = data as string[];
            else
                sTempStrings = new ASCIIEncoding().GetString(data as byte[]).Split('\n');

            StringBuilder sBuilder = new StringBuilder();
            foreach (string tempString in sTempStrings)
            {
                if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
                    sBuilder = sBuilder.AppendLine(tempString);
                else
                {
                    sBuilder = sBuilder.Append(tempString);
                    sBuilder = sBuilder.Append("\r\n");
                }
            }
            ParseStringData(Configurations.currentPlatform == LikewiseTargetPlatform.Windows ? (sBuilder.ToString() + "\r\n") : sBuilder.ToString());
        }

        private void ParseDWordData(object data)
        {
            string stringData = string.Empty;
            ASCIIEncoding encoder = new ASCIIEncoding();
            
            Logger.Log("BinaryValueEditorDialog.ParseDWordData - Given string - " + stringData, Logger.RegistryViewerLoglevel);

            if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
                stringData = string.Format("{0:X8}", (uint)(int)data);
            else
                stringData = string.Format("{0:X8}", (uint)BitConverter.ToInt32(data as byte[], 0));
           
            string[] listOfItems = new string[columnLength];
            listOfItems[0] = "0000";

            for (int i = 1, k = 0; i <= 4; i++, k += 2)
            {
                listOfItems[i] = stringData.Substring(k, 2);
                int bitValue = Int32.Parse(listOfItems[i], System.Globalization.NumberStyles.HexNumber);

                if (bitValue <= 20)
                    listOfItems[i + byteLength] = ".";
                else
                    listOfItems[i + byteLength] = Encoding.Unicode.GetString(BitConverter.GetBytes(bitValue));
            }
            Array.Reverse(listOfItems, 1, 4);
            Array.Reverse(listOfItems, 9, 4);
            LWDataBinarydata.Rows.Add(listOfItems);
        }

        private void ParseQWordData(object data)
        {
            string stringData = string.Empty;
            ASCIIEncoding encoder = new ASCIIEncoding();

            Logger.Log("BinaryValueEditorDialog.ParseDWordData - Given string - " + stringData, Logger.RegistryViewerLoglevel);

            if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
                stringData = string.Format("{0:X16}", (ulong)(long)data);
            else
                stringData = string.Format("{0:X16}", (uint)BitConverter.ToInt64(data as byte[], 0));

            string[] listOfItems = new string[columnLength];
            listOfItems[0] = "0000";

            for (int i = 1, k = 0; i <= 8; i++, k += 2)
            {
                listOfItems[i] = stringData.Substring(k, 2);
                long bitValue = Int64.Parse(listOfItems[i], System.Globalization.NumberStyles.HexNumber);

                if (bitValue <= 20)
                    listOfItems[i + byteLength] = ".";
                else
                    listOfItems[i + byteLength] = Encoding.Unicode.GetString(BitConverter.GetBytes(bitValue));
            }
            Array.Reverse(listOfItems, 1, 8);
            Array.Reverse(listOfItems, 9, 8);
            LWDataBinarydata.Rows.Add(listOfItems);
        }

        private void ParseStringData(object data)
        {
            string stringData = string.Empty;
            byte[] sBinarydata = null;
            ASCIIEncoding encoder = new ASCIIEncoding();

            stringData = data.ToString();
            sBinarydata = encoder.GetBytes(stringData);

            Logger.Log("BinaryValueEditorDialog.ParseDWordData - Given string - " + stringData, Logger.RegistryViewerLoglevel);

            if ((regValueInfo != null && (regValueInfo.pType != (ulong)RegistryApi.REG_MULTI_SZ)) ||
                (ValueInfo != null && (ValueInfo.RegDataType != LWRegistryValueKind.REG_MULTI_SZ)))
                stringData += "\n";

            // If the string is "test\r\n" the output must be "t0e0s0t000"
            int cellCount = this.numOfCells = (stringData.Length * 2) + 2;
            int numOfRows = (cellCount / byteLength) + 1;
            int index = 0;
            int intStartIndex = 0;

            for (int rows = 1; rows <= numOfRows; rows++)
            {
                string[] listOfItems = new string[columnLength];
                listOfItems[0] = string.Format("{0:X4}", intStartIndex); 
               
                for (int i = 1; i <= byteLength; i++)
                {
                    try
                    {
                        // Condition to avoid \r\n. \r will be 13 and \n will be 10
                        if (sBinarydata[index] == 13 || sBinarydata[index] == 10)
                        {
                            listOfItems[i] = "00";
                            listOfItems[i + byteLength] = ".";
                        }
                        else
                        {
                            listOfItems[i] = BitConverter.ToString(sBinarydata, index, 1);
                            listOfItems[i + byteLength] = ((int)sBinarydata[index] <= 20) ? "." : Encoding.Default.GetString(new byte[] { sBinarydata[index] });
                            i++;
                            listOfItems[i] = "00";
                            listOfItems[i + byteLength] = ".";
                        }
                    }
                    catch (Exception err)
                    {
                        Logger.LogException("BinayValueEditorDialog, index " + index, err);
                        break;
                    }
                    index++;
                }
                cellCount -= byteLength;
                LWDataBinarydata.Rows.Add(listOfItems);
                intStartIndex += byteLength;

                if (index >= sBinarydata.Length)
                    break;
            }
        }

        private void ParseBinaryData(object data)
        {
            byte[] sBinarydata = data as byte[];
            int numOfRows = (sBinarydata.Length / byteLength) + 1;
            this.numOfCells = sBinarydata.Length + 1;
            int index = 0, intStartIndex = 0;
            for (int rows = 1; rows <= numOfRows; rows++)
            {
                string[] listOfItems = new string[columnLength];
                listOfItems[0] = string.Format("{0:X4}", intStartIndex);
                for (int i = 1; i <= byteLength; i++)
                {
                    try
                    {
                        listOfItems[i] = BitConverter.ToString(sBinarydata, index, 1);
                        listOfItems[i + byteLength] = ((int)sBinarydata[index] <= 20) ? "." : Encoding.Default.GetString(new byte[] { sBinarydata[index] }).ToUpper();
                    }
                    catch (Exception err)
                    {
                        Logger.LogException("BinayValueEditorDialog, index " + index, err);
                        break;
                    }
                    index++;
                }
                
                LWDataBinarydata.Rows.Add(listOfItems);
                intStartIndex += byteLength;
            }
        }

        private bool FormatOutputData()
        {
            int index = 0;
            int dataLength;
            byte[] byts;
            ASCIIEncoding encoder = new ASCIIEncoding();

            try
            {
                dataLength = (LWDataBinarydata.Rows.Count) * byteLength;
                dataLength = dataLength == 0 ? byteLength : dataLength;
                byts = new byte[dataLength];

                this.LWDataBinarydata.EndEdit();

                ulong dwRegDataType = ValueInfo != null ? (ulong)ValueInfo.RegDataType : regValueInfo.pType;

                if ((dwRegDataType == (ulong)LWRegistryValueKind.REG_BINARY) ||
                   (dwRegDataType == (ulong)LWRegistryValueKind.REG_RESOURCE_LIST) ||
                   (dwRegDataType == (ulong)RegistryApi.REG_BINARY) ||
                   (dwRegDataType == (ulong)RegistryApi.REG_FULL_RESOURCE_DESCRIPTOR) ||
                   (dwRegDataType == (ulong)RegistryApi.REG_RESOURCE_LIST) ||
                   (dwRegDataType == (ulong)RegistryApi.REG_RESOURCE_REQUIREMENTS_LIST) ||
                   (dwRegDataType == (ulong)RegistryApi.REG_UNKNOWN))
                {
                    foreach (DataGridViewRow drRow in LWDataBinarydata.Rows)
                    {
                        for (int i = 1; i <= byteLength; i++, index++)
                        {
                            object temp = drRow.Cells[i].Value;
                            if (temp == null)
                                break;

                            ValidateInputData(ref temp);
                            byts[index] = Convert.ToByte(temp.ToString(), 16);
                        }
                    }
                    byte[] tempBytes = new byte[index];
                    Array.Copy(byts, tempBytes, index);
                    if (ValueInfo != null)
                        ValueInfo.sDataBuf = tempBytes;
                    else
                        regValueInfo.bDataBuf = tempBytes;
                }
                else if ((dwRegDataType == (ulong)LWRegistryValueKind.REG_SZ) ||
                   (dwRegDataType == (ulong)LWRegistryValueKind.REG_EXPAND_SZ) ||
                   (dwRegDataType == (ulong)RegistryApi.REG_SZ) ||
                   (dwRegDataType == (ulong)RegistryApi.REG_EXPAND_SZ) ||
                   (dwRegDataType == (ulong)RegistryApi.REG_PLAIN_TEXT))
                {
                    foreach (DataGridViewRow drRow in LWDataBinarydata.Rows)
                    {
                        for (int i = 1; i <= byteLength; i += 2, index++)
                        {
                            object temp = drRow.Cells[i].Value;
                            if (temp == null || !Char.IsLetterOrDigit(temp.ToString(), 0))
                                break;

                           ValidateInputData(ref temp);
                            byts[index] = Convert.ToByte(temp.ToString(), 16);
                        }
                    }

                    byte[] tempSBytes = new byte[index];
                    Array.Copy(byts, tempSBytes, index);
                    if (ValueInfo != null)
                        ValueInfo.sData = encoder.GetString(tempSBytes);
                    else
                        regValueInfo.bDataBuf = encoder.GetString(tempSBytes);
                }
                else if ((dwRegDataType == (ulong)LWRegistryValueKind.REG_MULTI_SZ) ||
                     (dwRegDataType == (ulong)RegistryApi.REG_MULTI_SZ))
                {
                    foreach (DataGridViewRow drRow in LWDataBinarydata.Rows)
                    {
                        for (int i = 1; i <= byteLength; i += 2, index++)
                        {
                            object temp = drRow.Cells[i].Value;
                            if (temp == null || !Char.IsLetterOrDigit(temp.ToString(), 0))
                                break;

                            ValidateInputData(ref temp);
                            byts[index] = Convert.ToByte(temp.ToString(), 16);
                            dataLength++;
                        }
                    }
                    Array.Resize<byte>(ref byts, index);

                    if (ValueInfo != null)
                        ValueInfo.sDataBuf = encoder.GetString(byts).Split(new char[] { '\0' }, StringSplitOptions.RemoveEmptyEntries);
                    else
                    {
                        regValueInfo.bDataBuf = encoder.GetString(byts).Split(new char[] { '\0' }, StringSplitOptions.RemoveEmptyEntries);
                    }
                }
                else if ((dwRegDataType == (ulong)LWRegistryValueKind.REG_DWORD) ||
                      (dwRegDataType == (ulong)RegistryApi.REG_DWORD))
                {
                    StringBuilder sBuilder = new StringBuilder();
                    foreach (DataGridViewRow drRow in LWDataBinarydata.Rows)
                    {
                        for (int i = 1; i <= 4; i++)
                        {
                            object temp = drRow.Cells[i].Value;
                            if (temp == null)
                                break;

                            ValidateInputData(ref temp);
                            sBuilder.Append(temp.ToString());
                        }
                    }

                    string[] listOfItems = new string[4];
                    for (int i = 0, k = 0; i < 4; i++, k += 2)
                    {
                        listOfItems[i] = sBuilder.ToString().Substring(k, 2);
                    }

                    Array.Reverse(listOfItems, 0, 4);
                    sBuilder = new StringBuilder();
                    for (int i = 0; i < listOfItems.Length; i++)
                    {
                        sBuilder.Append(listOfItems[i]);
                    }

                    if (byts != null && byts.Length != 0)
                    {
                        if (ValueInfo != null)
                        {
                            ValueInfo.sDataBuf = (int)UInt32.Parse(sBuilder.ToString().Trim(), System.Globalization.NumberStyles.HexNumber);
                        }
                        else
                        {
                            regValueInfo.bDataBuf = (int)UInt32.Parse(sBuilder.ToString().Trim(), System.Globalization.NumberStyles.HexNumber);
                        }
                    }
                }

                else if ((dwRegDataType == (ulong)LWRegistryValueKind.REG_QUADWORD) ||
                 (dwRegDataType == (ulong)RegistryApi.REG_QWORD))
                {
                    StringBuilder sBuilder = new StringBuilder();
                    foreach (DataGridViewRow drRow in LWDataBinarydata.Rows)
                    {
                        for (int i = 1; i <= 8; i++)
                        {
                            object temp = drRow.Cells[i].Value;
                            if (temp == null || !Char.IsLetterOrDigit(temp.ToString(), 0))
                                break;

                            ValidateInputData(ref temp);
                            sBuilder.Append(temp.ToString());
                        }
                    }

                    string[] listOfItems = new string[8];
                    for (int i = 0, k = 0; i < 8; i++, k += 2)
                    {
                        listOfItems[i] = sBuilder.ToString().Substring(k, 2);
                    }

                    Array.Reverse(listOfItems, 0, 8);
                    sBuilder = new StringBuilder();
                    for (int i = 0; i < listOfItems.Length; i++)
                    {
                        sBuilder.Append(listOfItems[i]);
                    }

                    if (ValueInfo != null)
                    {
                        ValueInfo.sDataBuf = (long)UInt64.Parse(encoder.GetString(byts), System.Globalization.NumberStyles.HexNumber);
                    }
                    else
                    {
                        regValueInfo.bDataBuf = (long)UInt64.Parse(encoder.GetString(byts), System.Globalization.NumberStyles.HexNumber);
                    }
                }
                else
                {
                    if (ValueInfo != null)
                        ValueInfo.sDataBuf = "(value not set)";
                    else
                        regValueInfo.bDataBuf = "(value not set)";
                }
            }
            catch (Exception ex)
            {
                Logger.LogException("BinaryValueEditorDialog.FormatOutputData()", ex);
            }
            return true;
        }

        private void ValidateInputData(ref object sCellValue)
        {
            if (sCellValue.ToString().Length > 2)
                sCellValue = sCellValue.ToString().Substring(0, 2);

            else if (sCellValue.ToString().Length < 2)
                sCellValue = "0" + sCellValue.ToString();

            if (regEx.IsMatch(sCellValue.ToString()))
                return;
            else
            {
                char[] charArray = sCellValue.ToString().ToCharArray();
                if (charArray != null && charArray.Length == 2)
                {
                    sCellValue = null;
                    foreach (char chr in charArray)
                        if (regEx.IsMatch(chr.ToString()))
                            sCellValue += chr.ToString();
                        else
                            sCellValue += "0";
                }
            }
        }     


        /*private void ParseBinaryData(object data)
        {
            byte[] sBinarydata = data as byte[];
            int numOfRows = sBinarydata.Length / 8;
            if ((sBinarydata.Length % 8) != 0)
                numOfRows += 1;
            int index = 0;
            for (int rows = 1; rows <= numOfRows; rows++)
            {
                string[] listOfItems = new string[17];
                string stringIndex = RegistryUtils.DecimalToBase(index, 16);
                switch (stringIndex.Length)
                {
                    case 1:
                        listOfItems[0] = "000" + stringIndex;
                        break;
                    case 2:
                        listOfItems[0] = "00" + stringIndex;
                        break;
                    case 3:
                        listOfItems[0] = "0" + stringIndex;
                        break;
                    case 4:
                        listOfItems[0] = stringIndex;
                        break;
                    default:
                        listOfItems[0] = "0000";
                        break;
                }

                for (int i = 1; i <= 8; i++)
                {
                    try
                    {
                        listOfItems[i] = BitConverter.ToString(sBinarydata, index, 1);
                        if (listOfItems[i] == "00")
                            listOfItems[i + 8] = ".";
                        else
                            listOfItems[i + 8] = System.Text.Encoding.Default.GetString(new byte[] { sBinarydata[index] }).ToUpper();
                    }
                    catch (Exception err)
                    {
                        Logger.LogException("BinayValueEditorDialog, index " + index, err);
                    }
                    index++;
                }

                LWDataBinarydata.Rows.Add(listOfItems);
            }
        }*/

        /*private void FormatOutputData()
        {
            int index = 0;
            int byteLength = (LWDataBinarydata.Rows.Count / 2) * 8;
            byte[] byts = new byte[byteLength];
            ASCIIEncoding encoder = new ASCIIEncoding();

            foreach (DataGridViewRow drRow in LWDataBinarydata.Rows)
            {
                for (int i = 1; i <= 8; i++)
                {
                    try
                    {
                        byts[index] = Convert.ToByte(drRow.Cells[i].Value.ToString(), 16);
                        index++;
                    }
                    catch (Exception err)
                    {
                        Logger.LogException("BinayValueEditorDialog, index " + i, err);
                    }
                }
            }

            switch (valueKind)
            {
                case "Binary":
                    ValueInfo.sDataBuf = byts;
                    break;
                case "String":
                    if (byts != null && byts.Length != 0)
                    {
                        ValueInfo.sDataBuf = encoder.GetString(byts);
                    }
                    break;
                case "DWord":
                    if (byts != null && byts.Length != 0)
                    {
                        ValueInfo.sDataBuf = encoder.GetString(byts);
                        ValueInfo.sDataBuf = RegistryUtils.DecimalToBase(Convert.ToInt32(ValueInfo.sDataBuf.ToString()), 16);

                    }
                    break;
                default:
                    ValueInfo.sDataBuf = "(value not set)";
                    break;
            }
        }*/

        /*private void ParseStringData(object data)
        {
            string stringData = data.ToString();
            Logger.Log("BinaryValueEditorDialog.ParseDWordData - Given string - " + stringData, Logger.RegistryViewerLoglevel);
            ASCIIEncoding encoder = new ASCIIEncoding();
            byte[] sBinarydata = encoder.GetBytes(stringData);
            int cellCount = this.numOfCells = (stringData.Length * 2) + 2;
            int numOfRows = (cellCount / byteLength) + 1;       
            int index = 0;
            int intStartIndex = 0;
            for (int rows = 1; rows <= numOfRows; rows++)
            {
                string[] listOfItems;
                if (rows <= numOfRows)
                {
                    listOfItems = new string[19];                    
                }
                else
                    listOfItems = new string[17];

                string stringIndex = RegistryUtils.DecimalToBase(intStartIndex, 16);
                switch (stringIndex.Length)
                {
                    case 1:
                        listOfItems[0] = "000" + stringIndex;
                        break;
                    case 2:
                        listOfItems[0] = "00" + stringIndex;
                        break;
                    case 3:
                        listOfItems[0] = "0" + stringIndex;
                        break;
                    case 4:
                        listOfItems[0] = stringIndex;
                        break;
                    default:
                        listOfItems[0] = "0000";
                        break;
                }
                Logger.Log("String Length --> " + stringData.Length + ", Number of rows --> " + numOfRows);
                for (int i = 1; i <= 8; i++)
                {
                    if (index >= sBinarydata.Length)
                    {
                        listOfItems[i++] = "00";
                        listOfItems[i] = "00";
                        break;
                    }
                    try
                    {
                        listOfItems[i] = BitConverter.ToString(sBinarydata, index, 1);
                        listOfItems[i + 8] = System.Text.Encoding.Default.GetString(new byte[] { sBinarydata[index] }).ToUpper();
                        i++;
                        listOfItems[i] = "00";
                        listOfItems[i + 8] = ".";
                    }
                    catch (Exception err)
                    {
                        Logger.LogException("BinayValueEditorDialog, index " + index, err);
                    }
                    index++;
                }

                LWDataBinarydata.Rows.Add(listOfItems);
                intStartIndex += 8;
            }
        }*/

        private void Do_CellSelection()
        {
            if (LWDataBinarydata.SelectedCells.Count == 0)
                return;

            DataGridViewCell cell = LWDataBinarydata.SelectedCells[0];
            DataGridViewRow drRow = LWDataBinarydata.SelectedCells[0].OwningRow;

            if ((drRow.Index + 1) == this.LWDataBinarydata.Rows.Count)
                if ((cell.ColumnIndex - 1) > (this.numOfCells % byteLength))
                    return;

            switch (cell.ColumnIndex)
            {
                case 0:
                    SendKeys.Send("{Tab}");
                    break;

                default:
                    if (cell.ColumnIndex <= byteLength)
                    {
                        SendKeys.Send("{INSERT}");
                        cell.Selected = true;                        
                        drRow.Cells[cell.ColumnIndex + byteLength].Selected = true;
                    }
                    else
                    {
                        SendKeys.Send("{INSERT}");
                        cell.Selected = true;
                        drRow.Cells[cell.ColumnIndex - byteLength].Selected = true;
                    }
                    break;
            }
        }     
           
#endregion 

        #region Event Handlers

        private void btnOk_Click(object sender, EventArgs e)
        {
            if (!ValidateInputData())
                return;

            FormatOutputData();  

            this.btnOk.Enabled = false;
            this.DialogResult = DialogResult.OK;
            this.Close();
        }

        private void btnCancel_Click(object sender, EventArgs e)
        {
            this.Close();
        }

        private void LWDataBinarydata_CellMouseUp(object sender, DataGridViewCellMouseEventArgs e)
        {
            if (LWDataBinarydata.SelectedCells.Count == 0)
                return;

            Do_CellSelection();
        }

        private void LWDataBinarydata_KeyPress(object sender, KeyPressEventArgs e)
        {
            if ((Char.IsDigit(e.KeyChar)) ||
                    (e.KeyChar == 32) ||
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

            if (LWDataBinarydata.SelectedCells.Count != 0)
            {
                if (LWDataBinarydata.SelectedCells[0].Value != null)
                {
                    LWDataBinarydata.SelectedCells[0].Value = LWDataBinarydata.SelectedCells[0].Value.ToString().Length != 2 ? e.KeyChar.ToString() + "0" : e.KeyChar.ToString();
                }
            }
        }

        private void txtValuename_TextChanged(object sender, EventArgs e)
        {
            btnOk.Enabled = !String.IsNullOrEmpty(txtValuename.Text.Trim());
        }

        private void LWDataBinarydata_CellBeginEdit(object sender, DataGridViewCellCancelEventArgs e)
        {
            switch (e.ColumnIndex)
            {
                case 0:                    
                    break;

                default:
                    DataGridViewCell cell = LWDataBinarydata.Rows[e.RowIndex].Cells[e.ColumnIndex]; 
                    if (cell.Value != null)
                    {
                        if (cell.Value.ToString().Length < 2)
                            cell.Value = cell.Value.ToString() + "0";
                        else if (e.ColumnIndex == 9 && cell.Value.ToString().Length == 2)
                        {
                            string sStartIndex = LWDataBinarydata.Rows[e.RowIndex].Cells[0].Value.ToString();
                            int iCellValue = RegistryUtils.BaseToDecimal(sStartIndex, 10);
                            sStartIndex = iCellValue.ToString().PadLeft(2, '0');
                            LWDataBinarydata.Rows.Add(new string[] { sStartIndex });
                        }                      
                    }
                    break;
            }
        }

        private void LWDataBinarydata_EditingControlShowing(object sender, DataGridViewEditingControlShowingEventArgs e)
        {
            e.Control.KeyPress += new KeyPressEventHandler(this.LWDataBinarydata_KeyPress);
        }

        #endregion  
        
    }
}