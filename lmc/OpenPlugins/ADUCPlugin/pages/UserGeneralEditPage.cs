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
using System.Windows.Forms;
using Likewise.LMC.LMConsoleUtils;
using Likewise.LMC.AuthUtils;
using Likewise.LMC.LDAP.Interop;
using Likewise.LMC.ServerControl;
using Likewise.LMC.LDAP;

namespace Likewise.LMC.Plugins.ADUCPlugin
{
public partial class UserGeneralEditPage : MPPage, IDirectoryPropertiesPage
{
    
    #region Class Data
    
    private UserGenerelEditObject _editObject = null;
    private UserGenerelEditObject _originalObject = null;
    private ADUCDirectoryNode dirnode = null;
    
    #endregion
    
    #region Constructors
    
    public UserGeneralEditPage()
    {
        this.pageID = "UserGeneralEditProperities";
        InitializeComponent();
        SetPageTitle("General");
        
        _editObject = new UserGenerelEditObject();
        _originalObject = new UserGenerelEditObject();
    }
    
    #endregion
    
    #region Initialization Methods
    
    /// <summary>
    /// Queries and fills the ldap message for the selected User
    /// Gets the attribute list from AD for User schema attribute.
    /// search for the attributes givenName, displayName, sAMAccountName,
    /// memberOf, sAMAccountType, userPrincipalName, sn and displays them in a controls
    /// </summary>
    /// <param name="ce"></param>
    /// <param name="servername"></param>
    /// <param name="name"></param>
    /// <param name="dirnode"></param>
    public void SetData(CredentialEntry ce, string servername, string name, ADUCDirectoryNode dirnode)
    {
        try
        {
            int ret = -1;
            _editObject = new UserGenerelEditObject();
            this.dirnode = dirnode;
            
            List<LdapEntry> ldapEntries = null;
            
            ret = dirnode.LdapContext.ListChildEntriesSynchronous(
            dirnode.DistinguishedName,
            LdapAPI.LDAPSCOPE.BASE,
            "(objectClass=*)",
            null,
            false,
            out ldapEntries);
            
            if (ldapEntries == null || ldapEntries.Count == 0)
            {
                return;
            }
            
            LdapEntry ldapNextEntry = ldapEntries[0];
            
            string[] attrsList = ldapNextEntry.GetAttributeNames();
            
            if (attrsList != null)
            {
                foreach (string attr in attrsList)
                {
                    string sValue = "";
                    
                    LdapValue[] attrValues = ldapNextEntry.GetAttributeValues(attr, dirnode.LdapContext);
                    
                    if (attrValues != null && attrValues.Length > 0)
                    {
                        foreach (LdapValue value in attrValues)
                        {
                            sValue = sValue + "," + value.stringData;
                        }
                    }
                    
                    if (sValue.StartsWith(","))
                    {
                        sValue = sValue.Substring(1);
                    }
                    if (string.Compare(sValue, "") == 0)
                    {
                        sValue = "<Not Set>";
                    }
                    
                    if (string.Compare(attr, "cn") == 0)
                    {
                        this.lblUserName.Text = sValue;
                    }
                    
                    if (string.Compare(attr, "givenName") == 0)
                    {
                        this.FnametextBox.Text = sValue;
                        _editObject.FirstName = sValue;
                    }
                    
                    if (string.Compare(attr, "initials") == 0)
                    {
                        this.InitialTextBox.Text = sValue;
                        _editObject.Initails = sValue;
                    }
                    
                    if (string.Compare(attr, "sn") == 0)
                    {
                        this.LnametextBox.Text = sValue;
                        _editObject.LastName = sValue;
                    }
                    
                    if (string.Compare(attr, "displayName") == 0)
                    {
                        this.DisplayNametextBox.Text = sValue;
                        _editObject.DisplayName = sValue;
                    }
                    
                    if (string.Compare(attr, "description") == 0)
                    {
                        this.DescriptextBox.Text = sValue;
                        _editObject.Description = sValue;
                    }
                    
                    if (string.Compare(attr, "physicalDeliveryOfficeName") == 0)
                    {
                        this.OfficetextBox.Text = sValue;
                        _editObject.Office = sValue;
                    }
                    
                    if (string.Compare(attr, "telephoneNumber") == 0)
                    {
                        this.TelephonetextBox.Text = sValue;
                        _editObject.TelephoneNumber = sValue;
                    }
                    
                    if (string.Compare(attr, "mail") == 0)
                    {
                        this.emailtextBox.Text = sValue;
                        _editObject.Email = sValue;
                    }
                    
                    if (string.Compare(attr, "wWWHomePage") == 0)
                    {
                        this.webpagetextBox.Text = sValue;
                        _editObject.WebPage = sValue;
                    }
                    if (string.Compare(attr, "url") == 0)
                    {
                        _editObject.WebPageOther = sValue;
                    }
                    if (string.Compare(attr, "otherTelephone") == 0)
                    {
                        sValue = sValue.Replace(',', ';');
                        _editObject.TelephoneNumberOther = sValue;
                    }
                }
            }
            
            if (_editObject != null)
            {
                _originalObject = (UserGenerelEditObject)_editObject.Clone();
            }
            else
            {
                _originalObject = new UserGenerelEditObject();
            }
            ParentContainer.DataChanged = false;
            UpdateApplyButton();
        }
        catch (Exception e)
        {
            Logger.LogException("UserGeneralEditPage.SetData", e);
        }
    }
    
    
    private void UpdateApplyButton()
    {
        if ((_originalObject == null && _editObject == null) ||
            (_editObject != null && _editObject.Equals(_originalObject)))
        {
            ParentContainer.btnApply.Enabled = ParentContainer.DataChanged;
        }
        else
        {
            ParentContainer.DataChanged = true;
            ParentContainer.btnApply.Enabled = true;
        }
    }
    
    private void UpdateOriginalData()
    {
        if (_editObject != null)
        {
            _originalObject = (UserGenerelEditObject)_editObject.Clone();
        }
        else
        {
            _originalObject = new UserGenerelEditObject();
        }
    }
    
    /// <summary>
    /// Modifies the specified attributes for the selected AD Object either "user" to AD Schema template
    /// </summary>
    /// <returns></returns>
    public bool OnApply()
    {
        List<LDAPMod> ldapAttrlist = new List<LDAPMod>();
        List<LDAPMod> attrlist = new List<LDAPMod>();      

        if (dirnode == null ||
            String.IsNullOrEmpty(dirnode.DistinguishedName) ||
            dirnode.LdapContext == null)
        {
            return true;
        }       

        //the following portion of code uses openldap "ldap_Modify_s"
        string basedn = dirnode.DistinguishedName;
        DirectoryContext dirContext = dirnode.LdapContext;
        string[] objectClass_values = null;        

        if (_editObject.FirstName != "" &&
        !(_editObject.FirstName.Equals(_originalObject.FirstName)))
        {
            objectClass_values = new string[] { _editObject.FirstName, null };
            LDAPMod attr =
            new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "givenName",
            objectClass_values);
            attrlist.Add(attr);
        }
        if (_editObject.Initails != "" &&
        !(_editObject.Initails.Equals(_originalObject.Initails)))
        {
            objectClass_values = new string[] { _editObject.Initails, null };
            LDAPMod attr =
            new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "initials",
            objectClass_values);
            attrlist.Add(attr);
        }
        if (_editObject.LastName != "" &&
        !(_editObject.LastName.Equals(_originalObject.LastName)))
        {
            objectClass_values = new string[] { _editObject.LastName, null };
            LDAPMod attr =
            new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "sn",
            objectClass_values);
            attrlist.Add(attr);
        }
        if (_editObject.Description != "" &&
        !(_editObject.Description.Equals(_originalObject.Description)))
        {
            objectClass_values = new string[] { _editObject.Description, null };
            LDAPMod attr =
            new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "description",
            objectClass_values);
            attrlist.Add(attr);
        }
        if (_editObject.DisplayName != "" &&
        !(_editObject.DisplayName.Equals(_originalObject.DisplayName)))
        {
            objectClass_values = new string[] { _editObject.DisplayName, null };
            LDAPMod attr =
            new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "displayName",
            objectClass_values);
            attrlist.Add(attr);
        }
        if (_editObject.Office != "" &&
        !(_editObject.Office.Equals(_originalObject.Office)))
        {
            objectClass_values = new string[] { _editObject.Office, null };
            LDAPMod attr =
            new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "physicalDeliveryOfficeName",
            objectClass_values);
            attrlist.Add(attr);
        }
        if (_editObject.Email != "" &&
        !(_editObject.Email.Equals(_originalObject.Email)))
        {
            objectClass_values = new string[] { _editObject.Email, null };
            LDAPMod attr =
            new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "mail",
            objectClass_values);
            attrlist.Add(attr);
        }
        if (_editObject.WebPage != "" &&
        !(_editObject.WebPage.Equals(_originalObject.WebPage)))
        {
            objectClass_values = new string[] { _editObject.WebPage, null };
            LDAPMod attr =
            new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "wWWHomePage",
            objectClass_values);
            attrlist.Add(attr);
        }
        if (_editObject.TelephoneNumber != "" &&
        !(_editObject.TelephoneNumber.Equals(_originalObject.TelephoneNumber)))
        {
            objectClass_values = new string[] { _editObject.TelephoneNumber, null };
            LDAPMod attr =
            new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "telephoneNumber",
            objectClass_values);
            attrlist.Add(attr);
        }
        if (_editObject.TelephoneNumberOther != "" &&
        !(_editObject.TelephoneNumberOther.Equals(_originalObject.TelephoneNumberOther)))
        {
            _editObject.TelephoneNumberOther += ";";
            string[] split = _editObject.TelephoneNumberOther.Split(';');
            split[split.Length - 1] = null;
            
            objectClass_values = split;
            
            LDAPMod TelephoneNumberOther =
            new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "otherTelephone", objectClass_values);
            ldapAttrlist.Add(TelephoneNumberOther);
            
            attrlist.Add(TelephoneNumberOther);
        }
        
        if (_editObject.WebPageOther != "" &&
        !(_editObject.WebPageOther.Equals(_originalObject.WebPageOther)))
        {
            objectClass_values = new string[] { _editObject.WebPageOther, null };
            LDAPMod attr =
            new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "url",
            objectClass_values);
            attrlist.Add(attr);
        }
       
        LDAPMod[] attrArry = attrlist.ToArray();
        int ret = -1;
        if (attrArry != null && attrArry.Length != 0)
        {
            ret = dirContext.ModifySynchronous(basedn, attrArry);
        }
        else
        {
            return true;
        }
        if (ret !=0)
        {
            string sMsg = ErrorCodes.LDAPString(ret);
            container.ShowError(sMsg);
            return false;
        }        
        UpdateOriginalData();
       
        return true;
    }
    
    
    #endregion
    
    #region Events
    
    private void FnametextBox_TextChanged(object sender, EventArgs e)
    {
        _editObject.FirstName = FnametextBox.Text;
        UpdateApplyButton();
    }
    
    private void InitialTextBox_TextChanged(object sender, EventArgs e)
    {
        _editObject.Initails = InitialTextBox.Text;
        UpdateApplyButton();
    }
    
    private void LnametextBox_TextChanged(object sender, EventArgs e)
    {
        _editObject.LastName = LnametextBox.Text;
        UpdateApplyButton();
    }
    
    private void DisplayNametextBox_TextChanged(object sender, EventArgs e)
    {
        _editObject.DisplayName = DisplayNametextBox.Text;
        UpdateApplyButton();
    }
    
    private void DescriptextBox_TextChanged(object sender, EventArgs e)
    {
        _editObject.Description = DescriptextBox.Text;
        UpdateApplyButton();
    }
    
    private void OfficetextBox_TextChanged(object sender, EventArgs e)
    {
        _editObject.Office = OfficetextBox.Text;
        UpdateApplyButton();
    }
    
    private void TelephonetextBox_TextChanged(object sender, EventArgs e)
    {
        _editObject.TelephoneNumber = TelephonetextBox.Text;
        UpdateApplyButton();
    }
    
    private void emailtextBox_TextChanged(object sender, EventArgs e)
    {
        _editObject.Email = emailtextBox.Text;
        UpdateApplyButton();
    }
    
    private void webpagetextBox_TextChanged(object sender, EventArgs e)
    {
        _editObject.WebPage = webpagetextBox.Text;
        UpdateApplyButton();
    }
    
    private void btnTelOther_Click(object sender, EventArgs e)
    {
        OthersStringEditor stringForm = new OthersStringEditor(_editObject.TelephoneNumberOther,"Phone Number(others)");
        if (stringForm.ShowDialog(this) == DialogResult.OK)
        {
            if (stringForm.sMultiValuedStringAttrValue != "")
            {
                _editObject.TelephoneNumberOther = stringForm.sMultiValuedStringAttrValue;
                UpdateApplyButton();
            }
        }
    }
    
    private void btnWebOther_Click(object sender, EventArgs e)
    {
        OthersStringEditor stringForm = new OthersStringEditor(_editObject.WebPageOther, "Web Page Address(others)");
        if (stringForm.ShowDialog(this) == DialogResult.OK)
        {
            if (stringForm.sMultiValuedStringAttrValue != "")
            {
                _editObject.WebPageOther = stringForm.sMultiValuedStringAttrValue;
                UpdateApplyButton();
            }
        }
    }
    
    #endregion
}

public class UserGenerelEditObject :ICloneable
{
    public string FirstName = string.Empty;
    public string LastName = string.Empty;
    public string Initails = string.Empty;
    public string Description = string.Empty;
    public string Email = string.Empty;
    public string DisplayName = string.Empty;
    public string Office = string.Empty;
    public string WebPage = string.Empty;
    public string TelephoneNumber = string.Empty;
    public string WebPageOther = string.Empty;
    public string TelephoneNumberOther = string.Empty;
    
    #region override Methods
    public override bool Equals(object obj)
    {
        if (obj == null)
        {
            return false;
        }
        if (!(obj is UserGenerelEditObject))
        {
            return false;
        }
        return GetHashCode() == (obj as UserGenerelEditObject).GetHashCode();
    }
    
    public virtual object Clone()
    {
        // Create a shallow copy first
        UserGenerelEditObject other = MemberwiseClone() as UserGenerelEditObject;
        other.Description = Description;
        other.FirstName = FirstName;
        other.Initails = Initails;
        other.LastName = LastName;
        other.DisplayName = DisplayName;
        other.Email = Email;
        other.Office = Office;
        other.TelephoneNumber = TelephoneNumber;
        other.WebPage = WebPage;
        other.TelephoneNumberOther = TelephoneNumberOther;
        other.WebPageOther = WebPageOther;
        return other;
    }
    
    public override int GetHashCode()
    {
        StringBuilder sb = new StringBuilder();
        sb.Append(Description);
        sb.Append(FirstName);
        sb.Append(Initails);
        sb.Append(LastName);
        sb.Append(DisplayName);
        sb.Append(Email);
        sb.Append(Office);
        sb.Append(TelephoneNumber);
        sb.Append(WebPage);
        sb.Append(TelephoneNumberOther);
        sb.Append(WebPageOther);
        return sb.ToString().GetHashCode();
    }
    #endregion
    
}
}
