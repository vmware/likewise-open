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
using System.Windows.Forms;
using System.Text;
using Likewise.LMC.Utilities;
using Likewise.LMC.LDAP;
using Likewise.LMC.LDAP.Interop;
using Likewise.LMC.ServerControl;


namespace Likewise.LMC.Plugins.ADUCPlugin
{
public partial class ADEditPage : MPPage, IDirectoryPropertiesPage
{
    #region Class Data

    // dialog actions
    public enum EditDialogAction { ACTION_OK, ACTION_APPLY, ACTION_CANCEL };
    private ADUCDirectoryNode dirnode = null;
    private ADEditPageObject _OriginalPageObject = null;
    private ADEditPageObject _modifiedPageObject = null;
    public EditPageObject _EditPageObject = null;
    private MPContainer _parentDlg = null;

    private ListViewColumnSorter lvwColumnSorter;
    private List<ListViewItem> attributeList = null;

    public List<string> allowedAttributes = null;

    private List<LdapEntry> ldapEntries = null;
    private LDAPSchemaCache schemaCache = null;

    private string[] objectClasses = null;
    private List<string> MandatoryAttributes = null;
    private delegate void AddRangeDelegate(ListViewItem[] range);

    #endregion

    #region Constructors
    public ADEditPage(MPContainer parentDlg)
    {
        pageID = "EditProperitiesAdvanced";
        InitializeComponent();

        // Create an instance of a ListView column sorter and assign it
        // to the ListView control.
        lvwColumnSorter = new ListViewColumnSorter();
        this.lvAttrs.ListViewItemSorter = lvwColumnSorter;

        SetPageTitle("Advanced");
        _modifiedPageObject = new ADEditPageObject();
        _EditPageObject = new EditPageObject(lvAttrs);
        _parentDlg = parentDlg;
    }
    #endregion

    #region Initialization Methods

    /// <summary>
    /// Method to load data to the tab pages while loading
    /// Gets the all attribute list for the selected AD object by querying the Ldap Message.
    /// </summary>
    /// <param name="ce"></param>
    /// <param name="servername"></param>
    /// <param name="username"></param>
    /// <param name="dirNode"></param
    public void SetData(CredentialEntry ce, string servername, string username, ADUCDirectoryNode dirNode)
    {
        try
        {
            dirnode = dirNode;
            InitLdapMessage();
            schemaCache = dirnode.LdapContext.SchemaCache;

            if (objectClasses != null && objectClasses.Length != 0)
            {
                MandatoryAttributes = new List<string>();
                foreach (string objectClass in objectClasses)
                {
                    LdapClassType classtype = schemaCache.GetSchemaTypeByObjectClass(objectClass) as LdapClassType;
                    if (classtype != null && classtype.MandatoryAttributes != null)
                    {
                        foreach (string attr in classtype.MandatoryAttributes)
                        {
                            MandatoryAttributes.Add(attr);
                        }
                    }
                }
                if (dirnode.ObjectClass.Trim().Equals("user", StringComparison.InvariantCultureIgnoreCase) ||
                    dirnode.ObjectClass.Trim().Equals("group", StringComparison.InvariantCultureIgnoreCase) ||
                    dirnode.ObjectClass.Trim().Equals("computer", StringComparison.InvariantCultureIgnoreCase))
                {
                    MandatoryAttributes.Add("objectSid");
                    MandatoryAttributes.Add("sAMAccountName");
                    if (!MandatoryAttributes.Contains("cn"))
                    {
                        MandatoryAttributes.Add("cn");
                    }
                }
            }

            FillAttributeList(true, out _modifiedPageObject);
            ParentContainer.DataChanged = false;
            if (_modifiedPageObject != null)
            {
                _OriginalPageObject = (ADEditPageObject)_modifiedPageObject.Clone();
            }
            else
            {
                _OriginalPageObject = new ADEditPageObject();
            }
            UpdateApplyButton();
        }
        catch (Exception e)
        {
            Logger.LogException("ADEditPage.SetData", e);
        }
    }

    /// <summary>
    /// Method to query and initialize the LdapMessage
    /// </summary>
    private void InitLdapMessage()
    {
        int ret = -1;
        if (dirnode == null)
        {
            return;
        }

        ldapEntries = new List<LdapEntry>();

        string[] attrs = { "name", "allowedAttributes", null };

        List<LdapEntry> innerLdapEntries = null;
        ret = dirnode.LdapContext.ListChildEntriesSynchronous
        (dirnode.DistinguishedName,
        LdapAPI.LDAPSCOPE.BASE,
        "(objectClass=*)",
        attrs,
        false,
        out innerLdapEntries);

        if (innerLdapEntries == null)
        {
            return;
        }

        LdapEntry ldapNextEntry = innerLdapEntries[0];

        string[] attrsList = ldapNextEntry.GetAttributeNames();

        Logger.Log("the number of attributes are " + attrsList.Length, Logger.ldapLogLevel);

        allowedAttributes = new List<string>();
        if (attrsList != null)
        {
            foreach (string attr in attrsList)
            {
                LdapValue[] attrValues = ldapNextEntry.GetAttributeValues(attr, dirnode.LdapContext);
                if (attrValues != null && attrValues.Length > 0)
                {
                    foreach (LdapValue attrValue in attrValues)
                    {
                        if (attrValue.stringData.Trim().Equals("nTSecurityDescriptor"))
                        {
                            continue;
                        }
                        allowedAttributes.Add(attrValue.stringData);
                    }
                }
            }
        }

        string[] attrfull = new string[allowedAttributes.Count + 2];
        int i;
        attrfull[0] = "dummy";
        for (i = 0; i < allowedAttributes.Count; i++)
        {
            attrfull[i + 1] = allowedAttributes[i];
        }
        attrfull[i + 1] = null;

        ret = dirnode.LdapContext.ListChildEntriesSynchronous
        (dirnode.DistinguishedName,
        LdapAPI.LDAPSCOPE.BASE,
        "(objectClass=*)",
        attrfull,
        false,
        out ldapEntries);

        if (ldapEntries == null)
        {
            return;
        }

        ldapNextEntry = ldapEntries[0];

        LdapValue[] ldapValues = ldapNextEntry.GetAttributeValues("objectClass", dirnode.LdapContext);
        if (ldapValues != null && ldapValues.Length > 0)
        {
            objectClasses = new string[ldapValues.Length];
            int index = 0;
            foreach (LdapValue Oclass in ldapValues)
            {
                objectClasses[index] = Oclass.stringData;
                index++;
            }
        }
    }

    /// <summary>
    /// Method to set the Enable/Disable state to the Appy button.
    /// </summary>
    private void UpdateApplyButton()
    {
        if ((_OriginalPageObject == null && _modifiedPageObject == null) ||
        (_modifiedPageObject != null && _modifiedPageObject.Equals(_OriginalPageObject)))
        {
            ParentContainer.btnApply.Enabled = ParentContainer.DataChanged;
        }
        else
        {
            ParentContainer.DataChanged = true;
            ParentContainer.btnApply.Enabled = true;
        }
    }

    /// <summary>
    /// Method to get all attributes to the specified AD Object.
    /// </summary>
    /// <param name="AddNotset"></param>
    /// <param name="_modifiedPageObject"></param>
    private void FillAttributeList(bool AddNotset, out ADEditPageObject _modifiedPageObject)
    {
        _modifiedPageObject = new ADEditPageObject();
        lvAttrs.Items.Clear();

        if (ldapEntries == null ||
            ldapEntries.Count == 0 ||
            allowedAttributes == null)
        {
            return;
        }

        LdapEntry ldapNextEntry = ldapEntries[0];

        string[] attrsFullList = new string[allowedAttributes.Count];

        allowedAttributes.CopyTo(attrsFullList);

        attributeList = new List<ListViewItem>();

        foreach (string attr in attrsFullList)
        {
            string sValue = "";
            string sAttrType = "Optional";


            foreach (string mandatoryAttribute in MandatoryAttributes)
            {
                if (String.Equals(mandatoryAttribute, attr, StringComparison.InvariantCultureIgnoreCase))
                {
                    sAttrType = "Mandatory";
                    continue;
                }
            }

            LdapValue[] attrValues = ldapNextEntry.GetAttributeValues(attr, dirnode.LdapContext);
            if (attrValues != null && attrValues.Length > 0)
            {
                foreach (LdapValue value in attrValues)
                {
                    //need do a type check before we assign the values, the type check has been done in getAttributeValues
                    sValue = sValue + ";" + value.stringData;
                }
                if (String.Equals(attr, "objectSid", StringComparison.InvariantCultureIgnoreCase))
                {
                    _parentDlg.objectSidBytes = attrValues[0].byteData;
                }
                if (String.Equals(attr, "objectGUID", StringComparison.InvariantCultureIgnoreCase))
                {
                    _parentDlg.objectGUIDBytes = attrValues[0].byteData;
                }
            }

            if (sValue.StartsWith(";"))
            {
                sValue = sValue.Substring(1).Trim();
            }

            if (string.Compare(sValue, "") == 0)
            {
                sValue = "<Not Set>";
            }

            SchemaType schemaType = schemaCache.GetSchemaTypeByDisplayName(attr);

            if (schemaType == null)
            {
                string[] addItem = null;
                string[] slvItem = {
                        attr,
                        "",
                        sValue,
                        sAttrType,
                        "false"
                    };
                if (String.Equals(attr, "objectGUID", StringComparison.InvariantCultureIgnoreCase) ||
                    String.Equals(attr, "objectSid", StringComparison.InvariantCultureIgnoreCase))
                {
                    slvItem[1] = "Octet String";
                }
                addItem = slvItem;
                ListViewItem lvItem = new ListViewItem(addItem);
                FillListView(AddNotset, attributeList, lvItem);
            }
            else
            {
                string sADSType = GetADSTypeString(schemaType);

                if (!String.Equals(sValue, "<Not Set>", StringComparison.InvariantCultureIgnoreCase) &&
                    String.Equals(schemaType.DataType.ToString(), ADSType.ADSTYPE_UTC_TIME.ToString(), StringComparison.InvariantCultureIgnoreCase))
                {
                    string sDate = "", sTime = "";
                    sDate = sValue.Substring(0, 4);
                    sDate += "/" + sValue.Substring(4, 2);
                    sDate += "/" + sValue.Substring(6, 2);

                    sTime = sValue.Substring(8, 2);
                    sTime += ":" + sValue.Substring(10, 2);
                    sTime += ":" + sValue.Substring(12, 2);

                    sValue = Convert.ToDateTime(sDate).ToShortDateString();
                    sValue += " " + Convert.ToDateTime(sTime).ToLongTimeString();
                }
                string[] slvItem = {
                        attr,
                        sADSType,
                        sValue,
                        sAttrType,
                        "false"
                    };
                ListViewItem lvItem = new ListViewItem(slvItem);
                lvItem.Tag = schemaType;
                FillListView(AddNotset, attributeList, lvItem);
            }
        }

        PopulateListView();
        _EditPageObject = new EditPageObject(lvAttrs);
    }

    /// <summary>
    /// Method to fill the listview with the attribute list
    /// </summary>
    private void PopulateListView()
    {
        lvAttrs.Items.Clear();
        if (attributeList != null && attributeList.Count > 0)
        {
            ListViewItem[] lvItemArr = new ListViewItem[attributeList.Count];
            int i = 0;

            foreach (ListViewItem lvitem in attributeList)
            {

                lvItemArr[i] = lvitem;
                i++;
            }

            //lvAttrs.Items.AddRange(lvItemArr);
            AddRangeDelegate d = ThreadSafeAddRange;
            this.Invoke(d, new object[]
            {
                lvItemArr
            }
            );
        }
        lvwColumnSorter.Order = SortOrder.Ascending;
        this.lvAttrs.Sort();
    }

    /// <summary>
    /// Adding the Array of listview items through the AddRange function call
    /// </summary>
    /// <param name="range"></param>
    private void ThreadSafeAddRange(ListViewItem[] range)
    {
        if (range != null && range.Length > 0)
        {
            this.lvAttrs.Items.AddRange(range);
        }
    }

    /// <summary>
    /// Method that returns the string specified based on Attribute syntax
    /// </summary>
    /// <param name="schemaType"></param>
    /// <returns></returns>
    public static string GetADSTypeString(SchemaType schemaType)
    {
        ADSType syntaxID = schemaType.DataType;
        switch (syntaxID)
        {
            case ADSType.ADSTYPE_DN_STRING:
            return "Distinguished Name";
            case ADSType.ADSTYPE_CASE_IGNORE_STRING:
            if (schemaType.AttributeSyntax.Equals("2.5.5.2"))
            {
                return "Object Identifier";
            }
            else if (schemaType.AttributeSyntax.Equals("2.5.5.12"))
            {
                return "Unicode String";
            }
            else if (schemaType.AttributeSyntax.Equals("2.5.5.13"))
            {
                return "Presentation Address";
            }
            else
            {
                return "Case Insensitive String";
            }
            case ADSType.ADSTYPE_CASE_EXACT_STRING:
            return "Case Sensitive String";
            case ADSType.ADSTYPE_PRINTABLE_STRING:
            return "IA5-String";
            case ADSType.ADSTYPE_NUMERIC_STRING:
            return "Numerical String";
            case ADSType.ADSTYPE_DN_WITH_STRING:
            if (schemaType.AttributeSyntax.Equals("2.5.5.14"))
            {
                return "Unicode String";
            }
            else if (schemaType.AttributeSyntax.Equals("2.5.5.7"))
            {
                return "DN Binary";//"DN with Binary";
            }
            else
            {
                return "DN With String";
            }
            case ADSType.ADSTYPE_BOOLEAN:
            return "Boolean";
            case ADSType.ADSTYPE_INTEGER:
            return "Integer";
            case ADSType.ADSTYPE_OCTET_STRING:
            if ((schemaType.AttributeSyntax.Equals("2.5.5.17")))
            {
                return "SID";
            }
            else
            {
                return "Octet String";
            }
            case ADSType.ADSTYPE_UTC_TIME:
            return "UTC Coded Time";
            case ADSType.ADSTYPE_NT_SECURITY_DESCRIPTOR:
            return "NT Security Descriptor";
            case ADSType.ADSTYPE_LARGE_INTEGER:
            return "Larger Integer/Interval";
            default :
            return "";
        }
    }

    /// <summary>
    /// Method to fill the attribute list
    /// </summary>
    /// <param name="AddNotset"></param>
    /// <param name="attrList"></param>
    /// <param name="lvItem"></param>
    private void FillListView(bool AddNotset, List<ListViewItem> attrList,ListViewItem lvItem)
    {
        if (attrList == null)
        {
            return;
        }

        if (cbMandatoryAttr.Checked && cbOptionalAttr.Checked)
        {
            if (AddNotset)
            {
                attrList.Add(lvItem);
            }
            else
            {
                if (!(lvItem.SubItems[2].Text.Trim().Equals("<Not Set>".Trim())))
                {
                    attrList.Add(lvItem);
                }
            }
            return;
        }
        if (cbMandatoryAttr.Checked && !cbOptionalAttr.Checked)
        {
            if (AddNotset)
            {
                if (lvItem.SubItems[3].Text.Trim().Equals("Mandatory".Trim()))
                {
                    attrList.Add(lvItem);
                }
            }
            else
            {
                if (!(lvItem.SubItems[2].Text.Trim().Equals("<Not Set>".Trim())) &&
                (lvItem.SubItems[3].Text.Trim().Equals("Mandatory".Trim())))
                {
                    attrList.Add(lvItem);
                }
            }
            return;
        }
        if (!cbMandatoryAttr.Checked && cbOptionalAttr.Checked)
        {
            if (AddNotset)
            {
                if (((lvItem.SubItems[3].Text.Trim().Equals("Optional".Trim())) &&
                (lvItem.SubItems[2].Text.Trim().Equals("<Not Set>".Trim())) ||
                (lvItem.SubItems[3].Text.Trim() == "")))
                {
                    attrList.Add(lvItem);
                }
            }
            else
            {
                if (!(lvItem.SubItems[2].Text.Trim().Equals("<Not Set>".Trim())) &&
                ((lvItem.SubItems[3].Text.Trim().Equals("Optional".Trim())) ||
                (lvItem.SubItems[3].Text.Trim() == "")))
                {
                    attrList.Add(lvItem);
                }
            }
            return;
        }
        if (!cbMandatoryAttr.Checked && !cbOptionalAttr.Checked && cbValueAttr.Checked)
        {
            lvAttrs.Items.Clear();
        }
    }

    /// <summary>
    /// Method to find the specified item from the list view.
    /// </summary>
    /// <param name="name"></param>
    /// <returns></returns>
    private ListViewItem FindlvItem(string name)
    {
        foreach (ListViewItem lvItem in lvAttrs.Items)
        {
            if (lvItem.SubItems[0].Text.Equals(name, StringComparison.InvariantCultureIgnoreCase))
            {
                return lvItem;
            }
        }
        return null;
    }

    /// <summary>
    /// Saves the modified attribute values in AD Schema template
    /// </summary>
    /// <returns></returns>
    public bool OnApply()
    {
        bool dnIsChanged = false;

        if (lvAttrs == null || lvAttrs.Items.Count == 0)
        {
            return true;
        }

        foreach (ListViewItem lvItem in lvAttrs.Items)
        {
            if (lvItem.SubItems == null ||
                lvItem.SubItems.Count < 5 ||
                lvItem.SubItems[4].Text != "true")
            {
                continue;
            }

            if (!HandleLvItem(lvItem, ref dnIsChanged))
            {
                return false;
            }
        }
        return true;
    }

    private bool HandleLvItem(ListViewItem lvItem, ref bool dnIsChanged)
    {

        int ret = -1;
        //DistinguishedName
        //if the attribute we want to change is dn, use rename
        if (String.Equals(lvItem.SubItems[0].Text, "distinguishedName", StringComparison.InvariantCultureIgnoreCase))
        {
            MessageBox.Show(this, "The attribute cannot be modified because it is owned by the system.",
            CommonResources.GetString("Caption_Console"),
            MessageBoxButtons.OK,
            MessageBoxIcon.Exclamation);
            return false;
            //HandleLVDistinguishedName(lvItem, ref dnIsChanged, ref ret);
        }
        //if the attribute we want to change is CN, use rename
        else if (String.Equals(lvItem.SubItems[0].Text, "cn", StringComparison.InvariantCultureIgnoreCase) ||
                String.Equals(lvItem.SubItems[0].Text, "ou", StringComparison.InvariantCultureIgnoreCase))
        {
            MessageBox.Show(this, "The attribute cannot be modified because it is owned by the system.",
            CommonResources.GetString("Caption_Console"),
            MessageBoxButtons.OK,
            MessageBoxIcon.Exclamation);
            return false;
            //HandleLVCommonName(lvItem, ref dnIsChanged, ref ret);
        }
        else if (String.Equals(lvItem.SubItems[0].Text, "name", StringComparison.InvariantCultureIgnoreCase))
        {
            MessageBox.Show(this, "The attribute cannot be modified because it is owned by the system.",
            CommonResources.GetString("Caption_Console"),
            MessageBoxButtons.OK,
            MessageBoxIcon.Exclamation);
            return false;
            //else
            //    HandleLVName(lvItem, ref dnIsChanged, ref ret);
        }
        else if (String.Equals(lvItem.SubItems[0].Text, "member", StringComparison.InvariantCultureIgnoreCase))
        {
            HandleLVName(lvItem, ref dnIsChanged, ref ret);
        }
        else if (String.Equals(lvItem.SubItems[0].Text, "memberOf", StringComparison.InvariantCultureIgnoreCase))
        {
            HandleLVMemberOf(lvItem, ref dnIsChanged, ref ret);
        }
        else
        {
            HandleLVDefault(lvItem, ref dnIsChanged, ref ret);
        }
        if (ret == 0)
        {
            lvItem.SubItems[4].Text = "false";
            LACTreeNode parentnode = (LACTreeNode)dirnode.Parent;

            ADUCDirectoryNode parentdirnode = parentnode as ADUCDirectoryNode;

            if (parentdirnode != null)
            {
                parentdirnode.Refresh();
            }

            _parentDlg.AttrModified = true;
            return true;
        }
        else
        {
            if (ret == 53)
            {
                string sMsg = "Access to the attribute is not permitted because the attribute is owned by the Security Accounts Manager(SAM)";
                MessageBox.Show(this,
                sMsg,
                CommonResources.GetString("Caption_Console"),
                MessageBoxButtons.OK,
                MessageBoxIcon.Exclamation);
            }
            else if (ret != -1 || ret != 19)
            {
                container.ShowError(ErrorCodes.LDAPString(ret));
            }

            return true;
        }

    }
    #endregion

    #region helper functions

    private void HandleLVDistinguishedName(ListViewItem lvItem, ref bool dnIsChanged, ref int ret)
    {
        string basedn = dirnode.DistinguishedName;
        DirectoryContext dirContext = dirnode.LdapContext;

        string fullnewDn = lvItem.SubItems[2].Text;

        if (fullnewDn != null)
        {
            string[] splits = fullnewDn.Split(',');
            if (!dnIsChanged)
            {
                ret = dirContext.RenameSynchronous(basedn, splits[0], null);
                dnIsChanged = true;
                ListViewItem cnItem = FindlvItem("cn");
                if (cnItem != null && !cnItem.SubItems[2].Text.Trim().Equals(splits[0].Substring(3)))
                {
                    cnItem.SubItems[2].Text = splits[0].Substring(3);
                    cnItem.SubItems[4].Text = "true";
                    dnIsChanged = true;
                }
                ListViewItem ouItem = FindlvItem("ou");
                if (ouItem != null && !ouItem.SubItems[2].Text.Trim().Equals(splits[0].Substring(3)))
                {
                    ouItem.SubItems[2].Text = splits[0].Substring(3);
                    ouItem.SubItems[4].Text = "true";
                }
                ListViewItem nameItem = FindlvItem("name");
                if (nameItem != null && !nameItem.SubItems[2].Text.Trim().Equals(splits[0].Substring(3)))
                {
                    nameItem.SubItems[2].Text = splits[0].Substring(3);
                    nameItem.SubItems[4].Text = "true";
                }
                _parentDlg.newDn = splits[0];
            }

        }
    }

    private void HandleLVCommonName(ListViewItem lvItem, ref bool dnIsChanged, ref int ret)
    {
        string basedn = dirnode.DistinguishedName;
        DirectoryContext dirContext = dirnode.LdapContext;
        string prefix;
        string newRDn = lvItem.SubItems[2].Text;

        prefix = dirnode.DistinguishedName.Substring(0, 3);
        newRDn = string.Concat(prefix, newRDn);


        if (newRDn != null)
        {
            if (!dnIsChanged)
            {
                ret = dirContext.RenameSynchronous(basedn, newRDn, null);
                _parentDlg.newDn = newRDn;
                dnIsChanged = true;
                ListViewItem dnItem = FindlvItem("distinguishedName");
                if (dnItem != null)
                {
                    LACTreeNode parentnode = (LACTreeNode)dirnode.Parent;
                    ADUCDirectoryNode parentdirnode = parentnode as ADUCDirectoryNode;
                    if (parentdirnode != null)
                    {
                        dnItem.SubItems[2].Text = string.Concat(newRDn, ",", parentdirnode.DistinguishedName);
                        dnItem.SubItems[4].Text = "true";
                    }
                }
                ListViewItem nameItem = FindlvItem("name");
                if (nameItem != null && !nameItem.SubItems[2].Text.Trim().Equals(lvItem.SubItems[2].Text))
                {
                    nameItem.SubItems[2].Text = lvItem.SubItems[2].Text;
                    nameItem.SubItems[4].Text = "true";
                }
            }
        }
    }

    private void HandleLVName(ListViewItem lvItem, ref bool dnIsChanged, ref int ret)
    {
        string basedn = dirnode.DistinguishedName;
        DirectoryContext dirContext = dirnode.LdapContext;
        string prefix;
        string newRDn = lvItem.SubItems[2].Text;

        prefix = dirnode.DistinguishedName.Substring(0, 3);
        newRDn = string.Concat(prefix, newRDn);

        if (newRDn != null)
        {
            if (!dnIsChanged)
            {
                ret = dirContext.RenameSynchronous(basedn, newRDn, null);
                _parentDlg.newDn = newRDn;
                dnIsChanged = true;
                ListViewItem dnItem = FindlvItem("distinguishedName");
                if (dnItem != null)
                {
                    string oldDn = dirnode.DistinguishedName;
                    LACTreeNode parentnode = (LACTreeNode)dirnode.Parent;
                    ADUCDirectoryNode parentdirnode = parentnode as ADUCDirectoryNode;
                    if (parentdirnode != null)
                    {
                        dnItem.SubItems[2].Text = string.Concat(newRDn, ",", parentdirnode.DistinguishedName);
                    }
                }
                ListViewItem cnItem = FindlvItem("cn");
                if (cnItem != null && !cnItem.SubItems[2].Text.Trim().Equals(lvItem.SubItems[2].Text))
                {
                    cnItem.SubItems[2].Text = lvItem.SubItems[2].Text;
                    cnItem.SubItems[4].Text = "true";
                }
                ListViewItem ouItem = FindlvItem("ou");
                if (ouItem != null && !ouItem.SubItems[2].Text.Trim().Equals(lvItem.SubItems[2].Text))
                {
                    ouItem.SubItems[2].Text = lvItem.SubItems[2].Text;
                    ouItem.SubItems[4].Text = "true";
                }
            }
        }
    }

    private void HandleLVMember(ListViewItem lvItem, ref bool dnIsChanged, ref int ret)
    {
        string basedn = dirnode.DistinguishedName;
        DirectoryContext dirContext = dirnode.LdapContext;
        string membersConcated = lvItem.SubItems[2].Text;

        if (membersConcated.Equals("<Not Set>", StringComparison.InvariantCultureIgnoreCase))
        {
            string[] members_values = { null };
            LDAPMod memberattr_Info =
            new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "member",
            members_values);

            LDAPMod[] attrinfo = new LDAPMod[] { memberattr_Info };

            ret = dirnode.LdapContext.ModifySynchronous(dirnode.DistinguishedName, attrinfo);
        }
        else
        {

            string[] members = membersConcated.Split(';');

            string[] members_values = new string[members.Length + 1];

            int i = 0;
            foreach (string member in members)
            {
                members_values[i] = member;
                i++;
            }
            members_values[i] = null;


            LDAPMod memberattr_Info =
            new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "member",
            members_values);

            LDAPMod[] attrinfo = new LDAPMod[] { memberattr_Info };

            ret = dirnode.LdapContext.ModifySynchronous(dirnode.DistinguishedName, attrinfo);
        }
    }

    private void HandleLVMemberOf(ListViewItem lvItem, ref bool dnIsChanged, ref int ret)
    {
        string basedn = dirnode.DistinguishedName;
        DirectoryContext dirContext = dirnode.LdapContext;
        string modifiedMemberof = lvItem.SubItems[2].Text;
        string origianlMemberof = lvItem.SubItems[2].Tag as string;

        if (!modifiedMemberof.Equals(origianlMemberof, StringComparison.InvariantCultureIgnoreCase))
        {

            if (modifiedMemberof.Equals("<Not Set>", StringComparison.InvariantCultureIgnoreCase))
            {
                string[] origianlMembers = origianlMemberof.Split(';');
                List<string> removedGroups = new List<string>();
                List<string> memberOfDnlist = new List<string>();

                foreach (string str in origianlMembers)
                {
                    memberOfDnlist.Add(str);
                    removedGroups.Add(str);
                }

                bool retVal = MemOfPages.OnApply_helper(memberOfDnlist, null, removedGroups, dirnode, this);
                if (retVal)
                {
                    ret = 0;
                }
            }
            else if (String.Equals(origianlMemberof, "<Not Set>", StringComparison.InvariantCultureIgnoreCase))
            {
                string[] modifiedMembers = modifiedMemberof.Split(';');
                List<string> addedGroups = new List<string>();
                List<string> memberofDnlist = new List<string>();

                foreach (string str in modifiedMembers)
                {
                    memberofDnlist.Add(str);
                    addedGroups.Add(str);
                }

                bool retVal = MemOfPages.OnApply_helper(memberofDnlist, addedGroups, null, dirnode, this);
                if (retVal)
                {
                    ret = 0;
                }
            }
            else
            {
                string[] modifiedMembers = modifiedMemberof.Split(';');
                string[] origianlMembers = origianlMemberof.Split(';');
                List<string> addedGroups = new List<string>();
                List<string> removedGroups = new List<string>();
                List<string> memberOfDnlist = new List<string>();

                foreach (string str in origianlMembers)
                {
                    memberOfDnlist.Add(str);
                }

                foreach (string origMember in origianlMembers)
                {
                    bool foundInMod = false;

                    foreach (string modMember in modifiedMembers)
                    {
                        if (origMember.Equals(modMember, StringComparison.InvariantCultureIgnoreCase))
                        {
                            foundInMod = true;
                            break;
                        }
                    }

                    //then origMember is deleted from memberOf attributes
                    if (!foundInMod)
                    {
                        removedGroups.Add(origMember);
                    }
                }

                foreach (string modMember in modifiedMembers)
                {
                    bool foundInOrig = false;

                    foreach (string origMember in origianlMembers)
                    {
                        if (String.Equals(modMember, origMember, StringComparison.InvariantCultureIgnoreCase))
                        {
                            foundInOrig = true;
                            break;
                        }
                    }

                    //then modMember is added to memberOf attributes
                    if (!foundInOrig)
                    {
                        addedGroups.Add(modMember);
                    }
                }

                bool retVal = MemOfPages.OnApply_helper(
                                memberOfDnlist,
                                addedGroups,
                                removedGroups,
                                dirnode,
                                this);
                if (retVal)
                {
                    ret = 0;
                }
            }
        }
    }

    private void HandleLVDefault(ListViewItem lvItem, ref bool dnIsChanged, ref int ret)
    {
        string basedn = dirnode.DistinguishedName;
        DirectoryContext dirContext = dirnode.LdapContext;
        string value = lvItem.SubItems[2].Text;
        if (String.Equals(value, "<Not Set>", StringComparison.InvariantCultureIgnoreCase))
        {
            string[] objectClass_values = { null };
            LDAPMod attr_Info =
            new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, lvItem.SubItems[0].Text,
            objectClass_values);
            LDAPMod[] attrinfo = new LDAPMod[] { attr_Info };
            ret = dirContext.ModifySynchronous(basedn, attrinfo);

            //need recover the previous value
            if (ret != 0)
            {
                lvItem.SubItems[2].Text = lvItem.SubItems[2].Tag as string;
            }
        }
        else
        {
            //application crashes for single digit value
            if (value.StartsWith("0x", StringComparison.InvariantCultureIgnoreCase))
            {
                value = value.Replace("ox", "-");
                value = value.Substring(1);
            }

            string[] objectClass_values = { value, null };
            LDAPMod attr_Info =
            new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, lvItem.SubItems[0].Text,
            objectClass_values);
            LDAPMod[] attrinfo = new LDAPMod[] { attr_Info };
            ret = dirContext.ModifySynchronous(basedn, attrinfo);
        }
    }


    #endregion

    #region Events

    /// <summary>
    /// Shows the specified editor control based on the selected attribute syntax from the listview.
    /// </summary>
    /// <param name="sender"></param>
    /// <param name="e"></param>
    private void btnEdit_Click(object sender, EventArgs e)
    {
        if (lvAttrs.SelectedItems.Count == 0)
        {
            return;
        }
        SchemaType schemaType = null;
        ListViewItem selectedItem = lvAttrs.SelectedItems[0];

        if (selectedItem.Text.Trim().Equals("objectGUID") || selectedItem.Text.Trim().Equals("objectSid"))
        {
            OctetStringAttributeEditor _octStringAttrForm = new OctetStringAttributeEditor(selectedItem);
            if (_octStringAttrForm.ShowDialog(this) == DialogResult.OK)
            {
                if (_octStringAttrForm.sOctextStringAttrValue != "")
                {
                    lvAttrs.SelectedItems[0].SubItems[2].Text = _octStringAttrForm.sOctextStringAttrValue;
                    lvAttrs.SelectedItems[0].SubItems[2].Tag = _octStringAttrForm.origsOctextStringAttrValue;
                    lvAttrs.SelectedItems[0].SubItems[4].Text = "true";
                    if (!_modifiedPageObject.IsListItemChanged)
                    {
                        _modifiedPageObject.IsListItemChanged = true;
                    }
                }
            }
            return;
        }
        else
        {
            if (selectedItem.Tag != null)
            {
                schemaType = selectedItem.Tag as SchemaType;
            }
            if (schemaType != null)
            {
                switch (schemaType.DataType)
                {
                    case ADSType.ADSTYPE_INTEGER:
                    case ADSType.ADSTYPE_LARGE_INTEGER:
                    case ADSType.ADSTYPE_NUMERIC_STRING:
                    if (schemaType.IsSingleValued)
                    {
                        IntegerAttributeEditor _IntForm = new IntegerAttributeEditor(selectedItem);
                        if (schemaType.DataType == ADSType.ADSTYPE_LARGE_INTEGER)
                        {
                            _IntForm.Text = "Large Integer Attribute Editor";
                        }
                        else
                        {
                            _IntForm.Text = "Integer Attribute Editor";
                        }
                        DialogResult dlg = _IntForm.ShowDialog(this);
                        if (dlg == DialogResult.OK)
                        {
                            if (_IntForm.IntAttrValue != "")
                            {
                                lvAttrs.SelectedItems[0].SubItems[2].Text = _IntForm.IntAttrValue;
                                lvAttrs.SelectedItems[0].SubItems[2].Tag = _IntForm.OrigIntAttrValue;
                                lvAttrs.SelectedItems[0].SubItems[4].Text = "true";

                                if (!_modifiedPageObject.IsListItemChanged)
                                {
                                    _modifiedPageObject.IsListItemChanged = true;
                                }
                            }
                        }
                    }
                    else
                    {
                        MultiValuedStringEditor _mulvalstringForm = new MultiValuedStringEditor(selectedItem, true);
                        if (_mulvalstringForm.ShowDialog(this) == DialogResult.OK)
                        {
                            if (_mulvalstringForm.sMultiValuedStringAttrValue != "")
                            {
                                lvAttrs.SelectedItems[0].SubItems[2].Text = _mulvalstringForm.sMultiValuedStringAttrValue;
                                lvAttrs.SelectedItems[0].SubItems[4].Text = "true";
                                if (!_modifiedPageObject.IsListItemChanged)
                                {
                                    _modifiedPageObject.IsListItemChanged = true;
                                }
                            }
                        }
                    }
                    break;
                    case ADSType.ADSTYPE_OCTET_STRING:

                    if (schemaType.IsSingleValued)
                    {
                        OctetStringAttributeEditor _octStringAttrForm = new OctetStringAttributeEditor(selectedItem);
                        if (_octStringAttrForm.ShowDialog(this) == DialogResult.OK)
                        {
                            if (_octStringAttrForm.sOctextStringAttrValue != "")
                            {
                                lvAttrs.SelectedItems[0].SubItems[2].Text = _octStringAttrForm.sOctextStringAttrValue;
                                lvAttrs.SelectedItems[0].SubItems[2].Tag = _octStringAttrForm.origsOctextStringAttrValue;
                                lvAttrs.SelectedItems[0].SubItems[4].Text = "true";
                                if (!_modifiedPageObject.IsListItemChanged)
                                {
                                    _modifiedPageObject.IsListItemChanged = true;
                                }
                            }
                        }
                    }
                    else
                    {
                        MultiValuedOctetEditor _mulvaloctStringAttrForm = new MultiValuedOctetEditor(selectedItem);
                        if (_mulvaloctStringAttrForm.ShowDialog(this) == DialogResult.OK)
                        {
                            if (_mulvaloctStringAttrForm.sMultiValuedOctetAttrValue != "")
                            {
                                lvAttrs.SelectedItems[0].SubItems[2].Text = _mulvaloctStringAttrForm.sMultiValuedOctetAttrValue;
                                lvAttrs.SelectedItems[0].SubItems[2].Tag = _mulvaloctStringAttrForm.origsMultiValuedOctetAttrValue;
                                lvAttrs.SelectedItems[0].SubItems[4].Text = "true";
                                if (!_modifiedPageObject.IsListItemChanged)
                                {
                                    _modifiedPageObject.IsListItemChanged = true;
                                }
                            }
                        }
                    }
                    break;
                    case ADSType.ADSTYPE_CASE_IGNORE_STRING:
                    case ADSType.ADSTYPE_DN_WITH_BINARY:
                    case ADSType.ADSTYPE_DN_WITH_STRING:
                    case ADSType.ADSTYPE_CASE_EXACT_STRING:
                    case ADSType.ADSTYPE_DN_STRING:
                    case ADSType.ADSTYPE_PRINTABLE_STRING:
                    case ADSType.ADSTYPE_NT_SECURITY_DESCRIPTOR:
                    if (schemaType.IsSingleValued)
                    {
                        StringAttributeEditor _stringForm = new StringAttributeEditor(selectedItem);
                        if (_stringForm.ShowDialog(this) == DialogResult.OK)
                        {
                            if (_stringForm.stringAttrValue != "")
                            {
                                lvAttrs.SelectedItems[0].SubItems[2].Text = _stringForm.stringAttrValue;
                                lvAttrs.SelectedItems[0].SubItems[2].Tag = _stringForm.origstringAttrValue;
                                lvAttrs.SelectedItems[0].SubItems[4].Text = "true";
                                if (!_modifiedPageObject.IsListItemChanged)
                                {
                                    _modifiedPageObject.IsListItemChanged = true;
                                }
                            }
                        }
                    }
                    else
                    {
                        //this should be the place that "memberOf" attribute gets edited.
                        MultiValuedStringEditor _mulvalstringForm = new MultiValuedStringEditor(selectedItem, false);
                        if (_mulvalstringForm.ShowDialog(this) == DialogResult.OK)
                        {
                            if (_mulvalstringForm.sMultiValuedStringAttrValue != "")
                            {
                                lvAttrs.SelectedItems[0].SubItems[2].Text = _mulvalstringForm.sMultiValuedStringAttrValue;
                                lvAttrs.SelectedItems[0].SubItems[2].Tag = _mulvalstringForm.origsMultiValuedStringAttrValue;
                                lvAttrs.SelectedItems[0].SubItems[4].Text = "true";
                                if (!_modifiedPageObject.IsListItemChanged)
                                {
                                    _modifiedPageObject.IsListItemChanged = true;
                                }
                            }
                        }
                    }
                    break;
                    case ADSType.ADSTYPE_UTC_TIME:
                    if (schemaType.IsSingleValued)
                    {
                        TimeAttributeEditor _timeForm = new TimeAttributeEditor(selectedItem);
                        if (_timeForm.ShowDialog(this) == DialogResult.OK)
                        {
                            if (_timeForm.sDateAttrValue != "" && _timeForm.sTimeAttrValue != "")
                            {
                                lvAttrs.SelectedItems[0].SubItems[2].Text = _timeForm.sDateAttrValue + " " + _timeForm.sTimeAttrValue;
                                lvAttrs.SelectedItems[0].SubItems[4].Text = "true";
                                if (!_modifiedPageObject.IsListItemChanged)
                                {
                                    _modifiedPageObject.IsListItemChanged = true;
                                }
                            }

                        }
                    }
                    else
                    {
                        MultiValuedTimeEditor _MulTimeForm = new MultiValuedTimeEditor(selectedItem);
                        if (_MulTimeForm.ShowDialog(this) == DialogResult.OK)
                        {
                            if (_MulTimeForm.sMultiValuedtimeAttrValue != "")
                            {
                                lvAttrs.SelectedItems[0].SubItems[2].Text = _MulTimeForm.sMultiValuedtimeAttrValue;
                                lvAttrs.SelectedItems[0].SubItems[4].Text = "true";
                                if (!_modifiedPageObject.IsListItemChanged)
                                {
                                    _modifiedPageObject.IsListItemChanged = true;
                                }
                            }
                        }
                    }
                    break;
                    case ADSType.ADSTYPE_BOOLEAN:
                    if (schemaType.IsSingleValued)
                    {
                        BooleanAttributeEditor _booleanForm = new BooleanAttributeEditor(selectedItem);
                        if (_booleanForm.ShowDialog(this) == DialogResult.OK)
                        {
                            if (_booleanForm.sBooleanAttrValue != "")
                            {
                                lvAttrs.SelectedItems[0].SubItems[2].Text = _booleanForm.sBooleanAttrValue;
                                lvAttrs.SelectedItems[0].SubItems[4].Text = "true";
                                if (!_modifiedPageObject.IsListItemChanged)
                                {
                                    _modifiedPageObject.IsListItemChanged = true;
                                }
                            }
                        }
                    }
                    else
                    {
                        MultiValuedBooleanEditor _MulValueBooleanForm = new MultiValuedBooleanEditor(selectedItem);
                        if (_MulValueBooleanForm.ShowDialog(this) == DialogResult.OK)
                        {
                            if (_MulValueBooleanForm.sMultiValuedBooleanAttrValue != "")
                            {
                                lvAttrs.SelectedItems[0].SubItems[2].Text = _MulValueBooleanForm.sMultiValuedBooleanAttrValue;
                                lvAttrs.SelectedItems[0].SubItems[2].Tag = _MulValueBooleanForm.origsMultiValuedBooleanAttrValue;
                                lvAttrs.SelectedItems[0].SubItems[4].Text = "true";
                                if (!_modifiedPageObject.IsListItemChanged)
                                {
                                    _modifiedPageObject.IsListItemChanged = true;
                                }
                            }
                        }
                    }

                    break;
                }
            }
        }
        UpdateApplyButton();
        lvAttrs.Focus();
    }

    private void cbMandatoryAttr_CheckedChanged(object sender, EventArgs e)
    {
        if (cbValueAttr.Checked)
        {
            FillAttributeList(false, out _modifiedPageObject);
        }
        else
        {
            FillAttributeList(true, out _modifiedPageObject);
        }
        UpdateApplyButton();
    }

    private void cbOptionalAttr_CheckedChanged(object sender, EventArgs e)
    {
        if (cbValueAttr.Checked)
        {
            FillAttributeList(false, out _modifiedPageObject);
        }
        else
        {
            FillAttributeList(true, out _modifiedPageObject);
        }
        UpdateApplyButton();
    }

    private void cbValueAttr_CheckedChanged(object sender, EventArgs e)
    {
        if (!cbMandatoryAttr.Checked && !cbOptionalAttr.Checked)
        {
            lvAttrs.Items.Clear();
        }

        if (cbValueAttr.Checked)
        {
            FillAttributeList(false, out _modifiedPageObject);
        }
        else
        {
            FillAttributeList(true, out _modifiedPageObject);
        }
        UpdateApplyButton();
    }

    private void lvAttrs_SelectedIndexChanged(object sender, EventArgs e)
    {
        if (lvAttrs.SelectedItems.Count > 0)
        {
            btnEdit.Enabled = true;
        }
        else
        {
            btnEdit.Enabled = false;
        }
    }

    private void lvAttrs_ColumnClick(object sender, ColumnClickEventArgs e)
    {
        // Determine if clicked column is already the column that is being sorted.
        if (e.Column == lvwColumnSorter.SortColumn)
        {
            // Reverse the current sort direction for this column.
            if (lvwColumnSorter.Order == SortOrder.Ascending)
            {
                lvwColumnSorter.Order = SortOrder.Descending;
            }
            else
            {
                lvwColumnSorter.Order = SortOrder.Ascending;
            }
        }
        else
        {
            // Set the column number that is to be sorted; default to ascending.
            lvwColumnSorter.SortColumn = e.Column;
            lvwColumnSorter.Order = SortOrder.Ascending;
        }

        // Perform the sort with these new sort options.
        this.lvAttrs.Sort();
    }

    #endregion
}

public class ADEditPageObject : ICloneable
{
    #region Class Data
    private bool _sIsMadatory;
    private bool _sIsOptional;
    private bool _sIsAttributeValues;
    private bool _bIsListItemChanged;
    #endregion

    #region Constructors
    public ADEditPageObject()
    {
        _sIsMadatory = true;
        _sIsOptional = true;
        _sIsAttributeValues = false;
        _bIsListItemChanged = false;
    }
    #endregion

    #region Initialization Methods
    public override bool Equals(object obj)
    {
        if (obj == null)
        {
            return false;
        }
        if (!(obj is ADEditPageObject))
        {
            return false;
        }
        return GetHashCode() == (obj as ADEditPageObject).GetHashCode();
    }

    public virtual object Clone()
    {
        // Create a shallow copy first
        ADEditPageObject other = MemberwiseClone() as ADEditPageObject;
        other._sIsMadatory = _sIsMadatory;
        other._sIsOptional = _sIsOptional;
        other._sIsAttributeValues = _sIsAttributeValues;
        other._bIsListItemChanged = _bIsListItemChanged;
        return other;
    }


    public override int GetHashCode()
    {
        StringBuilder sb = new StringBuilder();
        sb.Append(_sIsMadatory.ToString());
        sb.Append(_sIsOptional.ToString());
        sb.Append(_sIsAttributeValues.ToString());
        sb.Append(_bIsListItemChanged.ToString());
        return sb.ToString().GetHashCode();
    }
    #endregion

    #region accessor functions
    public bool IsMandatory
    {
        get
        {
            return _sIsMadatory;
        }
        set
        {
            _sIsMadatory = value;
        }
    }
    public bool IsOptional
    {
        get
        {
            return _sIsOptional;
        }
        set
        {
            _sIsOptional = value;
        }
    }
    public bool IsAttributeValues
    {
        get
        {
            return _sIsAttributeValues;
        }
        set
        {
            _sIsAttributeValues = value;
        }
    }
    public bool IsListItemChanged
    {
        get
        {
            return _bIsListItemChanged;
        }
        set
        {
            _bIsListItemChanged = value;
        }
    }
    #endregion
}

public class EditPageObject : ADEditPageObject
{
    #region Class Data
    private ListView _AttributeList = null;
    public ADEditPageObject _ADEditPageObject = null;
    #endregion

    #region Constructors
    public EditPageObject(ListView _ListView)
    {
        _AttributeList = _ListView;
    }
    #endregion

    #region accessor functions
    public ListView AttributeList
    {
        get
        {
            return _AttributeList;
        }
        set
        {
            _AttributeList = value;
        }
    }
    #endregion
}

}
