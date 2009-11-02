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
using System.Text;
using Microsoft.Win32;
using System.Windows.Forms;
using Likewise.LMC.Utilities;

namespace Likewise.LMC.Registry
{
    #region public classes

    public class RegistryEnumKeyInfo
    {
        public int totalEntries;
        public int maxKeyLength;        
        public string sKeyname;
        public string sClassName;
        public string OrigKey;
        public ulong nameSize;
        public uint sSubKeyCount;
        public uint sValueCount;
        public RegistryApi.FILETIME filetime;
        public IntPtr pRootKey;
        public IntPtr pKey;      

        public void initializeToNull()
        {
            totalEntries = 0;
            maxKeyLength = 0;
            nameSize = 0;
            sSubKeyCount = 0;
            sValueCount = 0;
            sKeyname = string.Empty;
            sClassName = string.Empty;
            OrigKey = string.Empty;
            filetime = new RegistryApi.FILETIME();
            pRootKey = IntPtr.Zero;
            pKey = IntPtr.Zero;
        }
    }

    public class RegistryValueInfo
    {
        public bool bIsDefault = false;
        public string sKeyname;
        public int maxKeyLength;        
        public string pValueName;
        public ulong pcchValueName;
        public ulong pType;
        public object bDataBuf;
        public ulong pcData;
        public IntPtr pParentKey;

        public void initializeToNull()
        {
            maxKeyLength = 0;
            pType = 0;
            pcData = 0;
            pcchValueName = 0;
            bDataBuf = null;
            pValueName = string.Empty;
            sKeyname = string.Empty;
        }
    }

    public class SubKeyInfo
    {       
        public string sKey;
        public string sData;
        public string sValue;
        public HKEY hKey;
        public RegistryKey sSubKey;
        public LWRegistryValueKind RegDataType;      
    }

    public class SubKeyValueInfo
    {     
        public string sData;
        public string sValue;       
        public object sDataBuf;
        public bool IsDefaultValue = false;
        public int intDataType;
        public HKEY hKey;
        public RegistryKey sParentKey;
        public LWRegistryValueKind RegDataType;
    }

    public class RegistryHelper
    {
        public static bool CheckDuplicateKey(object parentKey, string subKeyName)
        {
            if (subKeyName.Contains("\\"))
            {
                string sMsg = "The LAC cannot rename the key. Specify a key name without (\\).";
                MessageBox.Show(sMsg, "Error Renaming Key", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return true;
            }

            bool retValue = RegistryInteropWrapperWindows.Win32CheckDuplicateKey(parentKey, subKeyName);
            if (retValue)
            {
                string sMsg = "The LAC cannot rename the key. The specified Key name already exists. Type another name and try again.";
                MessageBox.Show(sMsg, "Error Renaming Key", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }          

            return retValue;
        }

        public static bool CheckDuplicateValue(object parentKey, string valueName)
        {
            RegistryKey sParentKey = parentKey as RegistryKey;
            bool retValue = RegistryInteropWrapperWindows.Win32CheckDuplicateValue(sParentKey, valueName);
            if (retValue)
            {
                string sMsg = "The LAC cannot rename the Value. The specified value name already exists. Type another name and try again.";
                MessageBox.Show(sMsg, "Error Renaming Value", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }

            return retValue;
        }

        public static bool CheckMaxValue(string value, out string retString, LWRegistryValueKind regDataType)
        {
            retString = value;

            switch (regDataType)
            {
                case LWRegistryValueKind.REG_DWORD:
                    if (UInt64.Parse(value) > UInt32.MaxValue)
                    {
                        string sMsg = "The decimal value entered is greater than the maximum value of a DWORD. \n Should the value be truncated in order to continue?";
                        DialogResult dResult = MessageBox.Show(sMsg, "Overflow", MessageBoxButtons.YesNo, MessageBoxIcon.Question);
                        if (dResult == DialogResult.Yes)
                            retString = UInt32.MaxValue.ToString();
                        else
                            return false;
                    }
                    break;

                case LWRegistryValueKind.REG_QUADWORD:
                    if (double.Parse(value) > UInt64.MaxValue)
                    {
                        string sMsg = "The decimal value entered is greater than the maximum value of a DWORD. \n Should the value be truncated in order to continue?";
                        DialogResult dResult = MessageBox.Show(sMsg, "Overflow", MessageBoxButtons.YesNo, MessageBoxIcon.Question);
                        if (dResult == DialogResult.Yes)
                            retString = UInt64.MaxValue.ToString();
                        else
                            return false;
                    }
                    break;
            }
            return true;
        }


        public static void DisplayErrorNotification()
        {
            string sMsg = "Enter the value name";
            MessageBox.Show(sMsg, "Error in Value Name", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }

        public static string GetFormatSpecificType(ulong valueKind)
        {
            string sType = string.Empty;

            switch (valueKind)
            {
                case (ulong)LWRegistryValueKind.REG_BINARY:
                    sType = "hex:";
                    break;

                case (ulong)LWRegistryValueKind.REG_DWORD:
                    sType = "dword:";
                    break;

                case (ulong)LWRegistryValueKind.REG_EXPAND_SZ:
                    sType = "hex(2):";
                    break;

                case (ulong)LWRegistryValueKind.REG_SZ:
                case (ulong)RegistryApi.REG_PLAIN_TEXT:
                    break;

                case (ulong)LWRegistryValueKind.REG_MULTI_SZ:
                    sType = "hex(7):";
                    break;

                case (ulong)LWRegistryValueKind.REG_QUADWORD:
                    sType = "qword:";
                    break;

                case (ulong)LWRegistryValueKind.REG_RESOURCE_LIST:
                case (ulong)LWRegistryValueKind.REG_UNKNOWN:
                    sType = "hex(8):";
                    break;
                
                case (ulong)RegistryApi.REG_FULL_RESOURCE_DESCRIPTOR:
                case (ulong)RegistryApi.REG_RESOURCE_REQUIREMENTS_LIST:
                    sType = "hex(9):";
                    break;

                default:
                    break;
            }
            return sType;
        }

        public static void GetValueKind(SubKeyValueInfo valueInfo, string sType)
        {
            string[] splits = sType.Split(':');
            switch (splits[0])
            {
                case "hex":
                    valueInfo.RegDataType = LWRegistryValueKind.REG_BINARY;
                    break;

                case "dword":
                    valueInfo.RegDataType = LWRegistryValueKind.REG_DWORD;
                    break;

                case "qword":
                    valueInfo.RegDataType = LWRegistryValueKind.REG_QUADWORD;
                    break;

                case "":
                    valueInfo.RegDataType = LWRegistryValueKind.REG_SZ;
                    break;

                case "hex(7)":
                    valueInfo.RegDataType = LWRegistryValueKind.REG_MULTI_SZ;
                    break;

                case "hex(2)":
                    valueInfo.RegDataType = LWRegistryValueKind.REG_EXPAND_SZ;
                    break;

                case "hex(8)":
                    valueInfo.RegDataType = LWRegistryValueKind.REG_RESOURCE_LIST;
                    break;

                case "hex(9)":
                    valueInfo.RegDataType = LWRegistryValueKind.REG_RESOURCE_REQUIREMENTS_LIST;
                    break;

                default:
                    valueInfo.RegDataType = LWRegistryValueKind.REG_SZ;
                    break;
            }
        }

        public static void GetValueKind(RegistryValueInfo valueInfo, string sType)
        {
            string[] splits = sType.Split(':');
            switch (splits[0])
            {
                case "hex":
                    valueInfo.pType = (ulong)RegistryApi.REG_BINARY;
                    break;

                case "dword":
                    valueInfo.pType = (ulong)RegistryApi.REG_DWORD;
                    break;

                case "qword":
                    valueInfo.pType = (ulong)LWRegistryValueKind.REG_QUADWORD;
                    break;

                case "":
                    valueInfo.pType = (ulong)RegistryApi.REG_SZ;
                    break;

                case "hex(7)":
                    valueInfo.pType = (ulong)RegistryApi.REG_MULTI_SZ;
                    break;

                case "hex(2)":
                    valueInfo.pType = (ulong)RegistryApi.REG_EXPAND_SZ;
                    break;

                case "hex(8)":
                    valueInfo.pType = (ulong)RegistryApi.REG_RESOURCE_LIST;
                    break;

                case "hex(9)":
                    valueInfo.pType = (ulong)RegistryApi.REG_RESOURCE_REQUIREMENTS_LIST;
                    break;

                default:
                    valueInfo.pType = (ulong)RegistryApi.REG_SZ;
                    break;
            }
        }

        public static void GetValueData(SubKeyValueInfo valueInfo, string sData)
        {
            ASCIIEncoding encoder = new ASCIIEncoding();
            string[] splits = sData.Split(':');

            switch (valueInfo.RegDataType)
            {
                case LWRegistryValueKind.REG_BINARY:
                    string[] sDataArry = splits[2].Split(',');
                    byte[] byts = new byte[sDataArry.Length];
                    for (int idx = 0; idx < sDataArry.Length; idx++)
                        byts[idx] = Convert.ToByte(sDataArry[idx], 16);
                    valueInfo.sDataBuf = byts;
                    break;

                case LWRegistryValueKind.REG_DWORD:
                    valueInfo.sDataBuf = ((int)UInt32.Parse(splits[1], System.Globalization.NumberStyles.HexNumber)).ToString();
                    break;

                case LWRegistryValueKind.REG_QUADWORD:
                    valueInfo.sDataBuf = ((long)UInt64.Parse(splits[1], System.Globalization.NumberStyles.HexNumber)).ToString();
                    break;

                case LWRegistryValueKind.REG_EXPAND_SZ:
                    string[] eDataArry = splits[2].Split(',');
                    byte[] eByts = new byte[eDataArry.Length];
                    for (int idx = 0, index = 0; idx < eDataArry.Length; idx++)
                    {
                        if (eDataArry[idx] != "00" && !String.IsNullOrEmpty(eDataArry[idx]))
                        {
                            eByts[index] = Convert.ToByte(eDataArry[idx], 16);
                            index++;
                        }
                    }
                    valueInfo.sData = new ASCIIEncoding().GetString(eByts);
                    break;

                case LWRegistryValueKind.REG_SZ:
                    if (splits[0].StartsWith("\""))
                        splits[0] = splits[0].Substring(1);
                    if (splits[0].EndsWith("\""))
                        splits[0] = splits[0].Substring(0, splits[0].Length - 1);
                    valueInfo.sData = splits[0];
                    break;

                case LWRegistryValueKind.REG_MULTI_SZ:
                    string[] sDataArray = splits[2].Split(',');
                    List<byte> bytlist = new List<byte>();
                    for (int idx = 0; idx < sDataArray.Length; idx += 2)
                        bytlist.Add(Convert.ToByte(sDataArray[idx], 16));
                    byte[] mbyts = new byte[bytlist.Count];
                    bytlist.CopyTo(mbyts);
                    valueInfo.sDataBuf = encoder.GetString(mbyts).Split(new char[] { '\0' }, StringSplitOptions.RemoveEmptyEntries);
                    break;

                case LWRegistryValueKind.REG_RESOURCE_LIST:
                case LWRegistryValueKind.REG_UNKNOWN:
                    string[] sRDataArray = splits[1].Split(',');
                    byte[] rbyts = new byte[sRDataArray.Length];
                    for (int idx = 0; idx < sRDataArray.Length; idx++)
                        rbyts[idx] = Convert.ToByte(sRDataArray[idx], 16);
                    valueInfo.sDataBuf = rbyts;
                    break;

                default:
                    break;
            }
        }

        public static void GetValueData(RegistryValueInfo valueInfo, string sData)
        {
            ASCIIEncoding encoder = new ASCIIEncoding();
            string[] splits = sData.Split(':');

            try
            {

                switch (valueInfo.pType)
                {
                    case (ulong)RegistryApi.REG_SZ:
                    case (ulong)RegistryApi.REG_EXPAND_SZ:
                    case (ulong)RegistryApi.REG_PLAIN_TEXT:
                        if (!String.IsNullOrEmpty(splits[0]))
                        {
                            byte[] sByts = encoder.GetBytes(splits[0].Substring(1));
                            Array.Resize<byte>(ref sByts, sByts.Length - 1);
                            valueInfo.bDataBuf = sByts;
                        }
                        else
                            valueInfo.bDataBuf = new byte[] { 0 };
                        break;

                    case (ulong)RegistryApi.REG_DWORD:
                        uint dValue = UInt32.Parse(splits[1], System.Globalization.NumberStyles.HexNumber);
                        byte[] dwDataArry = BitConverter.GetBytes(dValue);
                        valueInfo.bDataBuf = dwDataArry;
                        break;

                    case (ulong)RegistryApi.REG_QWORD:
                        ulong qValue = UInt64.Parse(splits[1], System.Globalization.NumberStyles.HexNumber);
                        byte[] qwDataArry = BitConverter.GetBytes(qValue);
                        valueInfo.bDataBuf = qwDataArry;
                        break;

                    case (ulong)RegistryApi.REG_MULTI_SZ:
                        string[] sDataArray = splits[2].Split(',');
                        List<byte> bytlist = new List<byte>();
                        for (int idx = 0; idx < sDataArray.Length; idx += 2)
                            bytlist.Add(Convert.ToByte(sDataArray[idx], 16));
                        byte[] mbyts = new byte[bytlist.Count];
                        bytlist.CopyTo(mbyts);
                        valueInfo.bDataBuf = encoder.GetString(mbyts).Split(new char[] { '\0' }, StringSplitOptions.RemoveEmptyEntries); 
                        RegistryInteropWrapper.RegModifyKeyValue(valueInfo, out mbyts);
                        valueInfo.bDataBuf = mbyts;
                        break;

                    default:
                        string[] sDataArry = splits[2].Split(',');
                        if (sDataArry != null)
                        {
                            byte[] byts = new byte[sDataArry.Length];
                            for (int idx = 0; idx < sDataArry.Length; idx++)
                            {
                                if (!String.IsNullOrEmpty(sDataArry[idx]))
                                    byts[idx] = Convert.ToByte(sDataArry[idx], 16);
                            }
                            valueInfo.bDataBuf = byts;
                        }
                        break;
                }
            }
            catch (Exception ex)
            {
                Logger.LogException("GetValueData() for the value " + valueInfo.pValueName, ex);
            }
        }

        public static object GetFormatSpecificData(SubKeyValueInfo valueInfo)
        {
            object sData = null;

            switch (valueInfo.RegDataType)
            {
                case LWRegistryValueKind.REG_BINARY:
                    valueInfo.sDataBuf = valueInfo.sParentKey.GetValue(valueInfo.sValue, null, RegistryValueOptions.DoNotExpandEnvironmentNames);
                    byte[] byts = valueInfo.sDataBuf as byte[];
                    sData = GetBinaryData(byts);
                    break;

                case LWRegistryValueKind.REG_DWORD:
                    valueInfo.sDataBuf = valueInfo.sParentKey.GetValue(valueInfo.sValue, null, RegistryValueOptions.DoNotExpandEnvironmentNames);
                    sData = RegistryUtils.DecimalToBase((UInt32)Convert.ToInt32(valueInfo.sDataBuf), 16).PadLeft(8, '0');
                    break;

                case LWRegistryValueKind.REG_EXPAND_SZ:
                    valueInfo.sDataBuf = valueInfo.sParentKey.GetValue(valueInfo.sValue, null, RegistryValueOptions.DoNotExpandEnvironmentNames);
                    byte[] eByts = new ASCIIEncoding().GetBytes(valueInfo.sDataBuf.ToString() + "\r\n");
                    List<byte> eBytList = new List<byte>();
                    foreach (byte byt in eByts)
                    {
                        if (byt == 10 || byt == 13)
                            eBytList.Add((byte)00);
                        else
                        {
                            eBytList.Add(byt);
                            eBytList.Add((byte)00);
                        }
                    }
                    eByts = new byte[eBytList.Count];
                    eBytList.CopyTo(eByts);
                    sData = GetBinaryData(eByts);
                    break;

                case LWRegistryValueKind.REG_SZ:
                    valueInfo.sDataBuf = valueInfo.sParentKey.GetValue(valueInfo.sValue, null, RegistryValueOptions.DoNotExpandEnvironmentNames);
                    sData = string.Concat("\"", valueInfo.sDataBuf, "\"");
                    break;

                case LWRegistryValueKind.REG_MULTI_SZ:
                    string[] sDataArry = valueInfo.sParentKey.GetValue(valueInfo.sValue, null, RegistryValueOptions.DoNotExpandEnvironmentNames) as string[];
                    StringBuilder sTempArry = new StringBuilder();
                    List<byte> bytList = new List<byte>();
                    foreach (string value in sDataArry)
                        sTempArry.AppendLine(value);
                    byte[] sByts = new ASCIIEncoding().GetBytes(sTempArry.ToString() + "\r\n");
                    foreach (byte byt in sByts)
                    {
                        if (byt == 10 || byt == 13)
                            bytList.Add((byte)00);
                        else
                        {
                            bytList.Add(byt);
                            bytList.Add((byte)00);
                        }
                    }
                    sByts = new byte[bytList.Count];
                    bytList.CopyTo(sByts);
                    sData = GetBinaryData(sByts);
                    break;

                case LWRegistryValueKind.REG_QUADWORD:
                    valueInfo.sDataBuf = valueInfo.sParentKey.GetValue(valueInfo.sValue, null, RegistryValueOptions.DoNotExpandEnvironmentNames);
                    sData = RegistryUtils.DecimalToBase((UInt64)Convert.ToUInt32(valueInfo.sDataBuf), 16).PadLeft(16, '0');
                    break;

                case LWRegistryValueKind.REG_RESOURCE_LIST:
                case LWRegistryValueKind.REG_UNKNOWN:
                    string[] sKey = valueInfo.sParentKey.ToString().Split(new char[] { '\\' } , 2);
                    object data = RegistryInteropWrapperWindows.RegGetValue(RegistryInteropWrapperWindows.GetRegistryHive(valueInfo.hKey), sKey[1], valueInfo.sValue, out valueInfo.intDataType);
                    byte[] bBinarydata = data as byte[];
                    sData = GetBinaryData(bBinarydata);
                    break;

                default:
                    break;
            }
            return sData;
        }

        public static object GetFormatSpecificData(RegistryValueInfo valueInfo)
        {
            object sData = null;

            switch (valueInfo.pType)
            {
                case (ulong)RegistryApi.REG_BINARY:
                case (ulong)RegistryApi.REG_RESOURCE_LIST:
                case (ulong)RegistryApi.REG_UNKNOWN:
                case (ulong)RegistryApi.REG_FULL_RESOURCE_DESCRIPTOR:
                case (ulong)RegistryApi.REG_RESOURCE_REQUIREMENTS_LIST:               
                    byte[] byts = valueInfo.bDataBuf as byte[];
                    Array.Resize<byte>(ref byts, (int)valueInfo.pcData);
                    sData = GetBinaryData(byts);
                    break;

                case (ulong)RegistryApi.REG_EXPAND_SZ:
                    string sTemp = new ASCIIEncoding().GetString(valueInfo.bDataBuf as byte[]);
                    byte[] eByts = new ASCIIEncoding().GetBytes(sTemp + "\n");
                    List<byte> bytList = new List<byte>();
                    foreach (byte byt in eByts)
                    {
                        if (byt == 10 || byt == 13)
                            bytList.Add((byte)00);
                        else
                        {
                            bytList.Add(byt);
                            bytList.Add((byte)00);
                        }
                    }
                    eByts = new byte[bytList.Count];
                    bytList.CopyTo(eByts);
                    sData = GetBinaryData(eByts);
                    break;

                case (ulong)RegistryApi.REG_MULTI_SZ:
                    string[] sArry = new ASCIIEncoding().GetString(valueInfo.bDataBuf as byte[]).Split('\n');
                    StringBuilder sTempArry = new StringBuilder();
                    List<byte> mBytList = new List<byte>();
                    foreach (string value in sArry)
                    {
                        sTempArry.Append(value);
                        sTempArry.Append("\r\n");
                    }
                    byte[] mByts = new ASCIIEncoding().GetBytes(sTempArry.ToString());
                    foreach (byte byt in mByts)
                    {
                        if (byt == 10 || byt == 13)
                            mBytList.Add((byte)00);
                        else
                        {
                            mBytList.Add(byt);
                            mBytList.Add((byte)00);
                        }
                    }
                    mByts = new byte[mBytList.Count];
                    mBytList.CopyTo(mByts);
                    sData = GetBinaryData(mByts);
                    break;

                case (ulong)RegistryApi.REG_DWORD:
                    sData = String.Format("{0:X2}", BitConverter.ToUInt32(valueInfo.bDataBuf as byte[], 0)).PadLeft(8, '0');
                    break;

                case (ulong)RegistryApi.REG_PLAIN_TEXT:
                case (ulong)RegistryApi.REG_SZ:
                    sData = new ASCIIEncoding().GetString(valueInfo.bDataBuf as byte[]);
                    sData = string.Concat("\"", sData, "\"");
                    break;

                case (ulong)RegistryApi.REG_QWORD:
                    //sData = RegistryUtils.DecimalToBase(Convert.ToInt32(new ASCIIEncoding().GetString(valueInfo.bDataBuf as byte[])), 16).PadLeft(16, '0');
                    sData = String.Format("{0:X2}", BitConverter.ToUInt64(valueInfo.bDataBuf as byte[], 0)).PadLeft(16, '0');
                    sData = string.Concat("\"", sData, "\"");
                    break;

                default:
                    break;
            }
            return sData;
        }

        public static object GetBinaryData(byte[] byts)
        {
            StringBuilder sTempVal = new StringBuilder();
            string sTempValue = string.Empty;
            int lineCount = 0;

            for (int idx = 0; idx < byts.Length; idx++)
            {
                lineCount++;
                if (lineCount <= 29)
                    sTempValue += BitConverter.ToString(byts, idx, 1) + ",";
                else
                {
                    sTempValue = sTempValue.EndsWith(",") ? sTempValue.Substring(0, sTempValue.LastIndexOf(",")) : sTempValue;
                    sTempVal.Append(sTempValue + @"\");
                    sTempVal.Append("\n");
                    sTempValue = BitConverter.ToString(byts, idx, 1) + ",";
                    lineCount = 0;
                }
            }

            if (byts.Length <= 29)
                sTempVal.Append(sTempValue.EndsWith(",") ? sTempValue.Substring(0, sTempValue.LastIndexOf(",")) : sTempValue);

            sTempValue = sTempVal.ToString().EndsWith("\n") ? sTempVal.ToString().Substring(0, sTempVal.ToString().Length - 1) : sTempVal.ToString();

            return sTempValue;
        }
    }

    #endregion

}
