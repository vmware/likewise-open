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
using SchemaType = Likewise.LMC.LDAP.SchemaType;
using Likewise.LMC.LDAP;
using Likewise.LMC.ServerControl;
using Likewise.LMC.AuthUtils;

namespace Likewise.LMC.Plugins.ADUCPlugin
{
public partial class ObjectAddAttributesTab : MPPage, IDirectoryPropertiesPage
{
    #region Class Data
    private ADObjectAddDlg _objectAddDlg;
    
    private Dictionary<string, AttributeInfo> _newAttributesList = new Dictionary<string, AttributeInfo>();
    
    private static Regex regNum = new Regex("^[-]?[0-9]+(\\.?[0-9]+)?$");
    
    private static Regex regDateTime = new Regex("^(?=\\d)((?<month>(0?[13578])|1[02]|(0?[469]|11)(?!.31)|0?2(?(.29)(?=.29.((1[6-9]|[2-9]\\d)(0[48]|[2468][048]|[13579][26])|(16|[2468][048]|[3579][26])00))|(?!.3[01])))(?<sep>[-./])(?<day>0?[1-9]|[12]\\d|3[01])\\k<sep>(?<year>(1[6-9]|[2-9]\\d)\\d{2})(?(?=\x20\\d)\x20|$))?(?<time>((0?[1-9]|1[012])(:[0-5]\\d){0,2}(?i:\x20[AP]M))|([01]\\d|2[0-3])(:[0-5]\\d){1,2})?$");
    
    private static Regex regHexaPrefixed = new Regex("^(0[xX])[0-9A-Fa-f]{1,2}$");
    #endregion
    
    #region Constructors
    public ObjectAddAttributesTab()
    {
        InitializeComponent();
        pageID = "ObjectAddAttributesTab";
        SetPageTitle("Attributes");
    }
    
    public ObjectAddAttributesTab(ADObjectAddDlg objectAddDlg)
    : this()
    {
        this._objectAddDlg = objectAddDlg;
        _newAttributesList = _objectAddDlg.objectInfo._AttributesList;
    }
    
    #endregion
    
    #region Events
    private void cbProOptionorMandatory_SelectedIndexChanged(object sender, EventArgs e)
    {
        this.chooseMandOrOptionList();
    }
    
    
    /// <summary>
    /// Enables/Disables the controls based on the attribute selection
    /// </summary>
    /// <param name="sender"></param>
    /// <param name="e"></param>
    private void cbProperty_SelectedIndexChanged(object sender, EventArgs e)
    {
        btnClear.Enabled = false;
        listboxValues.Items.Clear();
        if (cbProperty.SelectedItem == null)
        {
            return;
        }
        if (_newAttributesList == null)
        {
            return;
        }
        
        string selectedAttr = cbProperty.SelectedItem.ToString();
        if (_newAttributesList.ContainsKey(selectedAttr))
        {
            AttributeInfo AttrInfo = _newAttributesList[selectedAttr];
            SchemaType AttrSchema = AttrInfo.schemaInfo;
            if (AttrSchema.AttributeDisplayName.Equals(selectedAttr, StringComparison.InvariantCultureIgnoreCase))
            {
                txtSyntax.Text = GetADSTypeString(AttrSchema);
                if (AttrSchema.IsSingleValued)
                {
                    txtValues.Show();
                    txtValues.Text = AttrInfo.sAttributeValue;
                    listboxValues.Hide();
                    btnSet.Text = "&Set";
                    btnClear.Text = "&Clear";
                }
                else
                {
                    txtValues.Hide();
                    string[] sValues = AttrInfo.sAttributeValue.Split(';');
                    foreach (string s in sValues)
                    {
                        listboxValues.Items.Add(s);
                    }
                    listboxValues.Show();
                    btnSet.Text = "A&dd";
                    btnClear.Text = "&Remove";
                }
            }
        }
    }
    
    private void txtEditAttribute_TextChanged(object sender, EventArgs e)
    {
        if (txtEditAttribute.Text.Trim() == "")
        {
            btnSet.Enabled = false;
        }
        else
        {
            btnSet.Enabled = true;
        }
    }
    
    
    /// <summary>
    /// Set the value to the selected attribute
    /// </summary>
    /// <param name="sender"></param>
    /// <param name="e"></param>
    private void btnSet_Click(object sender, EventArgs e)
    {
        if (cbProperty.Items.Count == 0 || cbProperty.SelectedItem == null)
        {
            return;
        }
        if (cbProOptionorMandatory.Items.Count == 0 || cbProOptionorMandatory.SelectedItem == null)
        {
            return;
        }
        string Message;
        bool bSuccess = ValidateData(txtEditAttribute.Text.Trim().ToString(), txtSyntax.Text.ToString(), out Message);
        if (!bSuccess)
        {
            container.ShowMessage(Message);
            txtEditAttribute.Clear();
            txtEditAttribute.Focus();
            return;
        }
        string sAttribute = cbProperty.SelectedItem.ToString();
        string sAttributeType = cbProOptionorMandatory.SelectedItem.ToString();
        string sAttributeValue = "";
        
        if (btnSet.Text.Trim().Equals("A&dd"))
        {
            if (listboxValues.Items.Contains("<not set>".Trim()))
            {
                listboxValues.Items.Clear();
            }
            listboxValues.Items.Add(txtEditAttribute.Text.Trim());
        }
        else
        {
            txtValues.Text = txtEditAttribute.Text.Trim();
        }
        
        if (_newAttributesList.ContainsKey(sAttribute))
        {
            AttributeInfo AttrInfo = _newAttributesList[sAttribute];
            AttrInfo.sAttributeType = sAttributeType;
            SchemaType AttrSchema = AttrInfo.schemaInfo;
            if (AttrSchema != null && AttrSchema.IsSingleValued)
            {
                AttrInfo.sAttributeValue = txtValues.Text.Trim();
            }
            else
            {
                foreach (object item in listboxValues.Items)
                {
                    sAttributeValue = sAttributeValue + ";" + item.ToString();
                }
                if (sAttributeValue.StartsWith(";"))
                {
                    sAttributeValue = sAttributeValue.Substring(1);
                }
                AttrInfo.sAttributeValue = sAttributeValue;
            }
            _newAttributesList[sAttribute] = AttrInfo;
            
            if (_objectAddDlg.objectInfo.htMandatoryAttrList.Contains(sAttribute))
            {
                _objectAddDlg.objectInfo.htMandatoryAttrList[sAttribute] = AttrInfo.sAttributeValue;
            }
            else
            {
                _objectAddDlg.objectInfo.htMandatoryAttrList.Add(sAttribute, AttrInfo.sAttributeValue);
            }
        }
        btnClear.Enabled = listboxValues.SelectedItems.Count != 0 || txtValues.Text.Trim() != string.Empty;
        ParentContainer.DataChanged = true;
        ParentContainer.btnApply.Enabled = true;
        
        txtEditAttribute.Text = "";
        txtEditAttribute.Focus();
    }
    
    
    /// <summary>
    /// Clears the attribute value for the selected attribute to undefine it
    /// </summary>
    /// <param name="sender"></param>
    /// <param name="e"></param>
    private void btnClear_Click(object sender, EventArgs e)
    {
        if (btnClear.Text.Trim().Equals("&Clear"))
        {
            if (_objectAddDlg.objectInfo.htMandatoryAttrList.Contains(cbProperty.SelectedItem.ToString()))
            {
                _objectAddDlg.objectInfo.htMandatoryAttrList.Remove(cbProperty.SelectedItem.ToString());
            }
            if (_newAttributesList.ContainsKey(cbProperty.SelectedItem.ToString()))
            {
                AttributeInfo AttrInfo = _newAttributesList[cbProperty.SelectedItem.ToString()];
                AttrInfo.sAttributeValue = "<not set>";
                _newAttributesList[cbProperty.SelectedItem.ToString()] = AttrInfo;
            }
            txtEditAttribute.Text = txtValues.Text.Trim();
            txtValues.Text = "<not set>";
            btnSet.Enabled = true;
            btnClear.Enabled = false;
        }
        else
        {
            txtValues.Text = txtEditAttribute.Text.Trim();
            AttributeInfo AttrInfo = null;
            if (_newAttributesList.ContainsKey(cbProperty.SelectedItem.ToString()))
            {
                AttrInfo = _newAttributesList[cbProperty.SelectedItem.ToString()];
            }
            
            int idx = listboxValues.SelectedIndex;
            if (idx >= 0)
            {
                txtEditAttribute.Text = listboxValues.Items[idx].ToString();
                List<Object> ItemList = new List<Object>();
                int index = 0;
                
                for (index = 0; index < listboxValues.Items.Count; index++)
                {
                    if (index != idx)
                    {
                        ItemList.Add(listboxValues.Items[index]);
                    }
                }
                Object[] ItemsArr = new Object[ItemList.Count];
                index = 0;
                foreach (Object item in ItemList)
                {
                    ItemsArr[index] = item;
                    index++;
                }
                
                listboxValues.Items.Clear();
                listboxValues.Items.AddRange(ItemsArr);
                
                btnClear.Enabled = false;
            }
            if (listboxValues.Items.Count == 1 && listboxValues.Items[0].ToString().Trim().Equals("<not set>".Trim()))
            {
                if (_objectAddDlg.objectInfo.htMandatoryAttrList.Contains(cbProperty.SelectedItem.ToString()))
                {
                    _objectAddDlg.objectInfo.htMandatoryAttrList.Remove(cbProperty.SelectedItem.ToString());
                }
                
                AttrInfo.sAttributeValue = "<not set>";
                _newAttributesList[cbProperty.SelectedItem.ToString()] = AttrInfo;
            }
            else
            {
                AttrInfo.sAttributeValue = "";
                foreach (string item in listboxValues.Items)
                {
                    AttrInfo.sAttributeValue += "," + item;
                }
                if (AttrInfo.sAttributeValue.StartsWith(","))
                {
                    AttrInfo.sAttributeValue = AttrInfo.sAttributeValue.Substring(1);
                }
                _newAttributesList[cbProperty.SelectedItem.ToString()] = AttrInfo;
            }
        }
    }
    
    
    /// <summary>
    /// For each attribute selection set the attribute values
    /// If it is not defined set attribute values with "<Not Set">
    /// </summary>
    /// <param name="sender"></param>
    /// <param name="e"></param>
    private void listboxValues_SelectedIndexChanged(object sender, EventArgs e)
    {
        if (listboxValues.SelectedItems.Count != 0)
        {
            if (!listboxValues.Items.Contains("<not set>"))
            {
                btnClear.Enabled = true;
            }
        }
        if (listboxValues.Items.Count == 0)
        {
            btnClear.Enabled = false;
            listboxValues.Items.Add("<not set>");
        }
    }
    
    #endregion
    
    #region Overriden Methods
    
    public bool OnApply()
    {
        _objectAddDlg.objectInfo._AttributesList = _newAttributesList;
        return true;
    }
    
    #endregion
    
    #region IDirectoryPropertiesPage Members
    
    
    /// <summary>
    /// initializes the Mandatory and Optional list objects with respect to the attribute types
    /// </summary>
    /// <param name="ce"></param>
    /// <param name="servername"></param>
    /// <param name="name"></param>
    /// <param name="node"></param>
    public void SetData(CredentialEntry ce, string servername, string name, ADUCDirectoryNode node)
    {
        string initialSelectedAttrSyntax = "";
        
        if (_newAttributesList == null)
        {
            return;
        }
        
        foreach (AttributeInfo attrInfo in _newAttributesList.Values)
        {
            if (_objectAddDlg.objectInfo.htMandatoryAttrList != null && !_objectAddDlg.objectInfo.htMandatoryAttrList.Contains(attrInfo.sAttributename))
            {
                if (attrInfo.sAttributeType.Equals("Mandatory", StringComparison.InvariantCultureIgnoreCase))
                {
                    cbMandProperty.Add(attrInfo.sAttributename);
                    cbBothProperty.Add(attrInfo.sAttributename);
                }
                else if (attrInfo.sAttributeType.Equals("Optional", StringComparison.InvariantCultureIgnoreCase))
                {
                    cbOptionProperty.Add(attrInfo.sAttributename);
                    cbBothProperty.Add(attrInfo.sAttributename);
                }
            }
        }
        
            /*foreach (string Attr in _newAttributesList.Keys)
            {
                if (_objectAddDlg.objectInfo.htMandatoryAttrList != null)
                {
                    if (!_objectAddDlg.objectInfo.htMandatoryAttrList.Contains(Attr))
                        cbProperty.Items.Add(Attr);
                }
                else
                    cbProperty.Items.Add(Attr);
            }*/
        
        {
            this.chooseMandOrOptionList();
        }
        
        initialSelectedAttrSyntax = _objectAddDlg.ClassAttributeList[0].AttributeSyntax;
        
        cbProOptionorMandatory.SelectedIndex = 1;
        lbClass.Text = _objectAddDlg.choosenClass;
    }
    
    #endregion
    
    #region Helper Methods
    
    
    /// <summary>
    /// Validate the all form controls
    /// </summary>
    /// <param name="AttributeValue"></param>
    /// <param name="Syntax"></param>
    /// <param name="Message"></param>
    /// <returns></returns>
    public static bool ValidateData(string AttributeValue, string Syntax, out string Message)
    {
        Message = string.Empty;
        switch (Syntax)
        {
            case "INTEGER":
            if (!regNum.IsMatch(AttributeValue.Trim()))
            {
                Message = "One or more of the values are not in the correct format.";
                return false;
            }
            break;
            case "GeneralizedTime":
            if (!regDateTime.IsMatch(AttributeValue.Trim()))
            {
                Message = "One or more of the values are not in the correct format.\n(i.e. MM/DD/YYYY HH:MM:SS)";
                return false;
            }
            break;
            case "OctetString":
            string[] sHexa = AttributeValue.Trim().Split(' ');
            for (int i = 0; i < sHexa.Length; i++)
            {
                if (!regHexaPrefixed.IsMatch(sHexa[i].Trim()))
                {
                    Message = "One or more of the values are not in the correct format.\n(i.e. 0x00 0x12 0x34 etc.)";
                    return false;
                }
            }
            break;
            case "Boolean":
            if ((AttributeValue.Trim().ToUpper() != "TRUE") && (AttributeValue.Trim().ToUpper() != "FALSE"))
            {
                Message = "One or more of the values are not in the correct format.\n(i.e. TRUE or FALSE)";
                return false;
            }
            break;
            default:
            return true;
        }
        return true;
    }
    
    
    /// <summary>
    /// Returns the attribute type based on attribute syntax
    /// </summary>
    /// <param name="schemaType"></param>
    /// <returns></returns>
    public static string GetADSTypeString(SchemaType schemaType)
    {
        ADSType syntaxID = schemaType.DataType;
        switch (syntaxID)
        {
            case ADSType.ADSTYPE_DN_STRING:
            return "DN";
            case ADSType.ADSTYPE_CASE_IGNORE_STRING:
            if (schemaType.AttributeSyntax.Equals("2.5.5.2"))
            {
                return "OID";
            }
            else if (schemaType.AttributeSyntax.Equals("2.5.5.4"))
            {
                return "CaseIgnoreString";
            }
            else if (schemaType.AttributeSyntax.Equals("2.5.5.12"))
            {
                return "DirectoryString";
            }
            else if (schemaType.AttributeSyntax.Equals("2.5.5.13"))
            {
                return "PresentationAddress";
            }
            return "IA5String";
            case ADSType.ADSTYPE_CASE_EXACT_STRING:
            return "CaseSensitiveString";
            case ADSType.ADSTYPE_PRINTABLE_STRING:
            return "PrintableString";
            case ADSType.ADSTYPE_NUMERIC_STRING:
            return "NumericString";
            case ADSType.ADSTYPE_DN_WITH_STRING:
            return "DNWithString";
            case ADSType.ADSTYPE_BOOLEAN:
            return "Boolean";
            case ADSType.ADSTYPE_INTEGER:
            return "INTEGER";
            case ADSType.ADSTYPE_OCTET_STRING:
            return "OctetString";
            case ADSType.ADSTYPE_UTC_TIME:
            return "GeneralizedTime";
            case ADSType.ADSTYPE_NT_SECURITY_DESCRIPTOR:
            return "NTSecurityDescriptor";
            case ADSType.ADSTYPE_LARGE_INTEGER:
            return "INTEGER8";
            default:
            return "";
        }
    }
    
    /// <summary>
    /// Method which gives the list with either "Mandatory", "Optional", or "both"
    /// </summary>
    private void chooseMandOrOptionList()
    {
        cbProperty.Items.Clear();
        
        if (cbProOptionorMandatory.SelectedItem != null)
        {
            string selected = cbProOptionorMandatory.SelectedItem.ToString();
            
            if (selected.Equals("Mandatory", StringComparison.InvariantCultureIgnoreCase))
            {
                foreach (string str in cbMandProperty)
                {
                    cbProperty.Items.Add(str);
                }
            }
            else if (selected.Equals("Optional", StringComparison.InvariantCultureIgnoreCase))
            {
                foreach (string str in cbOptionProperty)
                {
                    cbProperty.Items.Add(str);
                }
            }
            else if (selected.Equals("Both", StringComparison.InvariantCultureIgnoreCase))
            {
                foreach (string str in cbBothProperty)
                {
                    cbProperty.Items.Add(str);
                }
            }
            
            if (cbProperty.Items.Count > 0)
            {
                cbProperty.SelectedIndex = 0;
            }
            
        }
    }
    #endregion
    
}
}

