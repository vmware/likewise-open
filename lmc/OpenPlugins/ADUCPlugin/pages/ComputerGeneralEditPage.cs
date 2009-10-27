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
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using Likewise.LMC.LDAP;
using Likewise.LMC.LDAP.Interop;
using Likewise.LMC.LMConsoleUtils;
using Likewise.LMC.AuthUtils;
using Likewise.LMC.ServerControl;
using System.DirectoryServices;

namespace Likewise.LMC.Plugins.ADUCPlugin
{
public partial class ComputerGeneralEditPage : MPPage, IDirectoryPropertiesPage
{
    #region Class region
    
    private ComputerGenerelEditObject _editObject = null;
    private ComputerGenerelEditObject _originalObject = null;
    private ADUCDirectoryNode dirnode = null;
    
    #endregion
    
    public ComputerGeneralEditPage()
    {
        pageID = "ComputerGeneralEditProperities";
        InitializeComponent();
        SetPageTitle("General");
        
        _editObject = new ComputerGenerelEditObject();
        _originalObject = new ComputerGenerelEditObject();
    }
    
    #region Private Methods
    
    /// <summary>
    /// Queries and fills the ldap message for the selected computer
    /// Gets the attribute list from AD for computer schema attribute.
    /// search for the attributes dNSHostName, cn or name and displays them in a controls
    /// </summary>
    /// <param name="ce"></param>
    /// <param name="servername"></param>
    /// <param name="name"></param>
    /// <param name="dirnode"></param>
    public void SetData(CredentialEntry ce, string servername, string name, ADUCDirectoryNode dirnode)
    {
        try
        {
            this.dirnode = dirnode;
            int ret = -1;
            List<LdapEntry> ldapEntries = null;
            
            ret = dirnode.LdapContext.ListChildEntriesSynchronous
            (dirnode.DistinguishedName,
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
                    
                    sValue = sValue.Substring(0, sValue.Length );
                    
                    if (string.Compare(sValue, "") == 0)
                    {
                        sValue = "<Not Set>";
                    }
                    
                    if (string.Compare(attr, "cn") == 0)
                    {
                        this.lblComputerName.Text = sValue;
                    }
                    
                    if (string.Compare(attr, "sAMAccountName") == 0)
                    {
                        if (sValue.EndsWith("$"))
                        {
                            this.txtCName.Text = sValue.Substring(0, sValue.Length - 1);
                        }
                        else
                        {
                            this.txtCName.Text = sValue;
                        }
                    }
                    
                    if (string.Compare(attr, "description") == 0)
                    {
                        this.txtDescription.Text = sValue;
                        _editObject.Description = sValue;
                    }
                    
                    if (string.Compare(attr, "dNSHostName") == 0)
                    {
                        this.txtDNSName.Text = sValue;
                    }
                    
                    if (string.Compare(attr, "userAccountControl") == 0)
                    {
                        int userCtrlVal = 0;
                        if (attrValues != null && attrValues.Length > 0)
                        {
                            userCtrlVal = Convert.ToInt32(attrValues[0].stringData);
                        }
                        string userCtrlBinStr = UserGroupUtils.DecimalToBase(userCtrlVal, 16);
                        _editObject.UserCtrlBinStr = userCtrlVal;
                        
                        this.txtRole.Text = "Workstation or server";
                        if (userCtrlBinStr.Length >= 3)
                        {
                            //Determine role of computer
                            if (userCtrlBinStr.Length == 3)
                            {
                                //examine the third position from the left (2=NORMAL_ACCOUNT)
                                if (userCtrlBinStr[0] == '2')
                                {
                                    this.txtRole.Text = "Normal computer";
                                }
                                
                                //examine the third position from the left (2=INTERDOMAIN_TRUST_ACCOUNT)
                                if (userCtrlBinStr[0] == '8')
                                {
                                    this.txtRole.Text = "Inter domain trust computer";
                                }
                            }
                            else
                            {
                                //examine the forth position from the left (2=WORKSTATION_TRUST_ACCOUNT)
                                if (userCtrlBinStr[userCtrlBinStr.Length - 4] == '1')
                                {
                                    this.txtRole.Text = "Workstation or server";
                                }
                                //examine the forth position from the left (2=SERVER_TRUST_ACCOUNT)
                                if (userCtrlBinStr[userCtrlBinStr.Length - 4] == '2')
                                {
                                    this.txtRole.Text = "Domain controller";
                                }
                            }
                        }
                        if (userCtrlBinStr.Length >= 5)
                        {
                            //Determine whether this user is TRUSTED_FOR_DELEGATION
                            //examine the fifth position from the left (8=TRUSTED_FOR_DELEGATION, 0=NOT TRUSTED)
                            //TRUSTED_FOR_DELEGATION
                            if (userCtrlBinStr[userCtrlBinStr.Length - 5] == '8')
                            {
                                this.checkBoxTrust.CheckedChanged -= new System.EventHandler(this.checkBoxTrust_CheckedChanged);
                                checkBoxTrust.Checked = true;
                                this.checkBoxTrust.CheckedChanged += new System.EventHandler(this.checkBoxTrust_CheckedChanged);
                            }
                            else if (userCtrlBinStr[userCtrlBinStr.Length - 5] == '0')
                            {
                                checkBoxTrust.Checked = false;
                            }
                        }
                        else
                        {
                            checkBoxTrust.Checked = false;
                        }
                        
                        _editObject.DelegateTrust = checkBoxTrust.Checked;
                    }
                }
            }
            UpdateOriginalData();
            UpdateApplyButton();
        }
        catch (Exception e)
        {
            container.ShowError(e.Message);
        }
    }
    
    private void UpdateApplyButton()
    {
        if ((_originalObject == null && _editObject == null) ||
        (_editObject != null && _editObject.Equals(_originalObject)))
        {
            ParentContainer.DataChanged = false;
            ParentContainer.btnApply.Enabled = false;
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
            _originalObject = (ComputerGenerelEditObject)_editObject.Clone();
        }
        else
        {
            _originalObject = new ComputerGenerelEditObject();
        }
    }
    
    /// <summary>
    /// Modifies the specified attributes for the selected AD Object either "user" to AD Schema template
    /// </summary>
    /// <returns></returns>
    public bool OnApply()
    {
        List<LDAPMod> attrlist = new List<LDAPMod>();
        //the following portion of code uses openldap "ldap_Modify_s"
        string basedn = dirnode.DistinguishedName;
        DirectoryContext dirContext = dirnode.LdapContext;
        string[] objectClass_values = null;

        if (!(_editObject.Description.Equals(_originalObject.Description)))
        {
            if (String.IsNullOrEmpty(_editObject.Description))
                objectClass_values = new string[] { null };
            else
                objectClass_values = new string[] { _editObject.Description, null };

            LDAPMod attr =
            new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "description",
            objectClass_values);
            attrlist.Add(attr);
        }
        
        if (!_editObject.DelegateTrust.Equals(_originalObject.DelegateTrust))
        {
            int userCtrlBinStr = _editObject.UserCtrlBinStr;
            
            if (_editObject.DelegateTrust)
            {
                userCtrlBinStr += 524288;
            }
            else
            {
                userCtrlBinStr -= 524288;
            }
            
            string[] userControl_values = { userCtrlBinStr.ToString(), null };
            LDAPMod userControl_Info =
            new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "userAccountControl", userControl_values);
            
            attrlist.Add(userControl_Info);
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
        if (ret != 0)
        {
            string sMsg = ErrorCodes.LDAPString(ret);
            container.ShowMessage(sMsg);
            return false;
        }
        UpdateOriginalData();
        return true;
    }
    
    #endregion
    
    #region Events
    
    private void checkBoxTrust_CheckedChanged(object sender, EventArgs e)
    {
        _editObject.DelegateTrust = checkBoxTrust.Checked;
        if (checkBoxTrust.Checked)
        {
            string sMsg =
            "This option allows the computer to be trusted for " +
            "delegation.\nTrusting the computer for delegation is " +
            "a security-sensitive operation. \nIt should not be done " +
            "indiscriminately. For more information see Help.";
            container.ShowMessage(sMsg);
        }
        UpdateApplyButton();
    }
    
    private void txtDescription_TextChanged(object sender, EventArgs e)
    {
        _editObject.Description = txtDescription.Text;
        UpdateApplyButton();
    }
    
    #endregion
}

public class ComputerGenerelEditObject : ICloneable
{
    public string Description = string.Empty;
    public bool DelegateTrust = false;
    public int UserCtrlBinStr = 0;
    
    #region override Methods
    public override bool Equals(object obj)
    {
        if (obj == null)
        {
            return false;
        }
        if (!(obj is ComputerGenerelEditObject))
        {
            return false;
        }
        return GetHashCode() == (obj as ComputerGenerelEditObject).GetHashCode();
    }
    
    public virtual object Clone()
    {
        // Create a shallow copy first
        ComputerGenerelEditObject other = MemberwiseClone() as ComputerGenerelEditObject;
        other.Description = Description;
        other.DelegateTrust = DelegateTrust;
        other.UserCtrlBinStr = UserCtrlBinStr;
        return other;
    }
    
    public override int GetHashCode()
    {
        StringBuilder sb = new StringBuilder();
        sb.Append(Description);
        sb.Append(DelegateTrust);
        sb.Append(UserCtrlBinStr.ToString());
        return sb.ToString().GetHashCode();
    }
    #endregion
    
}
}
