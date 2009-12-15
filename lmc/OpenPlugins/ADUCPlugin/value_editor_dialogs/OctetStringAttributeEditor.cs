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
using System.Text.RegularExpressions;
using Likewise.LMC.ServerControl;
using Likewise.LMC.Utilities;

namespace Likewise.LMC.Plugins.ADUCPlugin
{
public partial class OctetStringAttributeEditor : Form
{
    #region Class Data
    ListViewItem lvSelectedItem;
    public string sOctextStringAttrValue = "";
    public string origsOctextStringAttrValue = "";
    public string sPrevOctextStringAttrValue = "";
    private string sOctetLabel = "";
    private string sOctetString = "";
    private int iCurrentMode = 16;
    private int iTargetMode = 16;
    private int iPadLength = 2;
    #endregion
    
    #region Constructors
    public OctetStringAttributeEditor()
    {
        InitializeComponent();
    }
    
    public OctetStringAttributeEditor(ListViewItem lvItem)
    : this()
    {
        this.lvSelectedItem = lvItem;
    }
    
    public OctetStringAttributeEditor(string OctetLabel,string OctetString )
    : this()
    {
        this.lvSelectedItem = null;
        this.sOctetString = OctetString;
        this.sOctetLabel = OctetLabel;
    }
    #endregion
    
    #region Events
    private void OctetStringAttributeEditor_Load(object sender, EventArgs e)
    {
        cbEditValue.SelectedIndex = 0;
        if (lvSelectedItem != null)
        {
            lblselectedAttr.Text = lvSelectedItem.SubItems[0].Text;
            origsOctextStringAttrValue = lvSelectedItem.SubItems[0].Text;
            rtbAttr.Text = lvSelectedItem.SubItems[2].Text.Trim().Replace("0x", "");
            sPrevOctextStringAttrValue = rtbAttr.Text;
        }
        else
        {
            lblselectedAttr.Text = this.sOctetLabel;
            rtbAttr.Text = this.sOctetString.Trim().Replace("0x", "");
        }
    }
    
    private void btnClear_Click_1(object sender, EventArgs e)
    {
        rtbAttr.Text = "";
    }
    
    private void btnCancel_Click(object sender, EventArgs e)
    {
        this.Close();
    }
    
    private void btnOk_Click_1(object sender, EventArgs e)
    {
        if (rtbAttr.Text.Trim() == "")
        {
            this.sOctextStringAttrValue = "<Not Set>";
            this.DialogResult = DialogResult.OK;
            this.Close();
            return;
        }
        else
        {
            if (!ValidateEntries(iCurrentMode))
            {
                return;
            }
            ConvertValues(iCurrentMode, 16,2);
            sOctextStringAttrValue = PrefixWithHexa( rtbAttr.Text.Trim());
            this.DialogResult = DialogResult.OK;
            this.Close();
        }
    }
    
    private void cbEditValue_SelectedIndexChanged(object sender, EventArgs e)
    {
        try
        {
            if (!ValidateEntries(iCurrentMode))
            {
                //this.cbEditValue.SelectedIndexChanged -= new System.EventHandler(this.cbEditValue_SelectedIndexChanged);
                //switch (iCurrentMode)
                //{
                //    case 16:
                //        this.cbEditValue.SelectedIndex = 0;
                //        break;
                //    case 8:
                //        this.cbEditValue.SelectedIndex = 1;
                //        break;
                //    case 10:
                //        this.cbEditValue.SelectedIndex = 2;
                //        break;
                //    case 2:
                //        this.cbEditValue.SelectedIndex = 3;
                //        break;
                //}
                //this.cbEditValue.SelectedIndexChanged += new System.EventHandler(this.cbEditValue_SelectedIndexChanged);
                
                //return;
                rtbAttr.Text = sPrevOctextStringAttrValue;
            }
            
            if (cbEditValue.SelectedIndex == 0)
            {
                iTargetMode = 16;
                iPadLength = 2;
            }
            else if (cbEditValue.SelectedIndex == 1)
            {
                iTargetMode = 8;
                iPadLength = 3;
            }
            else if (cbEditValue.SelectedIndex == 2)
            {
                iTargetMode = 10;
                iPadLength = 3;
            }
            else if (cbEditValue.SelectedIndex == 3)
            {
                iTargetMode = 2;
                iPadLength = 8;
            }
            
            ConvertValues(iCurrentMode, iTargetMode, iPadLength);
        }
        catch (Exception ex)
        {
            Logger.LogException("OctetStringAttributeEditor.cbEditValue_SelectedIndexChanged", ex);
        }
    }
    
    #endregion
    
    #region Helper Methods
    /// <summary>
    /// Convert from one base number system to a target number system
    /// </summary>
    /// <param name="iBaseMode"></param>
    /// <param name="iTargetMode"></param>
    /// <param name="iPadLength"></param>
    private void ConvertValues(int iBaseMode, int iTargetMode, int iPadLength)
    {
        if (iCurrentMode == iTargetMode)
        {
            return;
        }
        iCurrentMode = iTargetMode;
        if (rtbAttr.Text.Trim().ToLower().Equals("<not set>") || rtbAttr.Text.Trim().Equals(string.Empty))
        {
            return;
        }
        string[] sTmpArr = rtbAttr.Text.Trim().Split(' ');
        string sResult = "";
        for (int i = 0; i < sTmpArr.Length; i++)
        {
            if (sTmpArr[i].ToString().Trim() != "")
            {
                sResult += " " + (Convert.ToString(Convert.ToInt32(sTmpArr[i].ToString(), iBaseMode), iTargetMode)).PadLeft(iPadLength, '0');
            
}
        
}

            
            rtbAttr.Text = sResult.ToUpper().Trim();
            sPrevOctextStringAttrValue = rtbAttr.Text;
            iCurrentMode = iTargetMode;
        }
        
        private bool ValidateEntries(int iMode)
        {
            string sResult = "", sTemp = "", sErrorMsg="";
            int iWidth = 0, iMaxVal=0;
            bool bValid = true;
            Regex regHexa = new Regex("^[\\s0-9A-F]{2}$");
            Regex regOct = new Regex("^[\\s0-7]{3}$");
            Regex regDeci = new Regex("^[\\s0-9]{3}$");
            Regex regBin = new Regex("^[\\s0-1]{8}$");
            try
            {
                rtbAttr.Text = rtbAttr.Text.Trim();
                if (rtbAttr.Text.Trim().Equals("<Not Set>", StringComparison.InvariantCultureIgnoreCase))
                {
                    return true;
                }
                switch (iMode)
                {
                    case 16: //Hexa
                    #region Hexa
                    iWidth = 2;
                    for (int i = 0; i < rtbAttr.Text.Length; )
                    {
                        if (i + iWidth > rtbAttr.Text.Length)
                        {
                            sTemp = rtbAttr.Text.Substring(i);
                        }
                        else
                        {
                            sTemp = rtbAttr.Text.Substring(i, iWidth);
                        }
                        
                        if (!regHexa.IsMatch(sTemp) ||
                        (sTemp.ToString().Trim().Equals(string.Empty) && sTemp.Length > 1))
                        {
                            bValid = false;
                            sErrorMsg = "Invalid format. A hexadecimal string must contain sets of two digits \nbetween 0 and FF. Each set must be separated by a space, e.g.,'01 11 AB F1'.";
                            break;
                        }
                        else
                        {
                            sResult += " " + sTemp.Trim().PadLeft(iWidth, '0');
                        }
                        i += (iWidth + 1);
                    }
                    break;
                    #endregion Hexa
                    case 8: //Octal
                    #region Octal
                    iWidth = 3;
                    iMaxVal = 377;
                    for (int i = 0; i < rtbAttr.Text.Length; )
                    {
                        if (i + iWidth > rtbAttr.Text.Length)
                        {
                            sTemp = rtbAttr.Text.Substring(i);
                        }
                        else
                        {
                            sTemp = rtbAttr.Text.Substring(i, iWidth);
                        }
                        
                        if (!regOct.IsMatch(sTemp)||(Convert.ToInt16(sTemp.Trim()) > iMaxVal) ||
                        (sTemp.ToString().Trim().Equals(string.Empty) && sTemp.Length > 1))
                        {
                            bValid = false;
                            sErrorMsg = "Invalid format. A Octal string must contain sets of three digits \nbetween 0 and 377. Each set must be separated by a space, e.g.,'022 151 377 005'.";
                            break;
                        }
                        else
                        {
                            sResult += " " + sTemp.Trim().PadLeft(iWidth, '0');
                        }
                        i += (iWidth + 1);
                    }
                    break;
                    #endregion Octal
                    case 10: //Decimal
                    #region Decimal
                    iWidth = 3;
                    iMaxVal = 255;
                    for (int i = 0; i < rtbAttr.Text.Length; )
                    {
                        if (i + iWidth > rtbAttr.Text.Length)
                        {
                            sTemp = rtbAttr.Text.Substring(i);
                        }
                        else
                        {
                            sTemp = rtbAttr.Text.Substring(i, iWidth);
                        }
                        
                        if ((!regDeci.IsMatch(sTemp)) ||
                        (sTemp.ToString().Trim().Equals(string.Empty) && sTemp.Length > 1))
                        {
                            bValid = false;
                            sErrorMsg = "Invalid format. A Decimal string must contain sets of three digits \nbetween 0 and 255. Each set must be separated by a space, e.g.,'051 211 255 009'.";
                            break;
                        }
                        else
                        {
                            sResult += " " + sTemp.Trim().PadLeft(iWidth, '0');
                        }
                        i += (iWidth + 1);
                    }
                    break;
                    #endregion Decimal
                    case 2: //Binary
                    #region Binary
                    iWidth = 8;
                    for (int i = 0; i < rtbAttr.Text.Length; )
                    {
                        if (i + iWidth > rtbAttr.Text.Length)
                        {
                            sTemp = rtbAttr.Text.Substring(i);
                        }
                        else
                        {
                            sTemp = rtbAttr.Text.Substring(i, iWidth);
                        }
                        
                        if ((!regBin.IsMatch(sTemp)) ||
                        (sTemp.ToString().Trim().Equals(string.Empty) && sTemp.Length > 1))
                        {
                            bValid = false;
                            sErrorMsg = "Invalid format. A Binary string must contain sets of \neight binary digits between '00000000' and '11111111'. \nEach set must be separated by a space, e.g.,'00110011 11111111'.";
                            break;
                        }
                        else
                        {
                            sResult += " " + sTemp.Trim().PadLeft(iWidth, '0');
                        }
                        i += (iWidth + 1);
                    }
                    break;
                    #endregion Binary
                }
                if (bValid)
                {
                    rtbAttr.Text = sResult.ToString().Trim();
                    return true;
                }
                else
                {
                    MessageBox.Show(
                    this,
                    sErrorMsg,
                    CommonResources.GetString("Caption_Console"),
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Warning);
                    return false;
                }
            }
            catch (Exception ex)
            {
                Logger.LogException("OctetStringAttributeEditor.ValidateEntries", ex);
                return false;
            }
            
        }
        
        private string PrefixWithHexa(string sVal)
        {
            if (sVal.Trim() != "")
            {
                string[] sTmpArr = sVal.Trim().Split(' ');
                string sResult = "";
                for (int i = 0; i < sTmpArr.Length; i++)
                {
                    if (sTmpArr[i].ToString().Trim() != "")
                    {
                        sResult += " " + "0x" + sTmpArr[i].ToString().Trim();
                    }
                }
                return sResult.Trim();
                
            }
            return sVal;
        }
        #endregion
        
    }
    
}
