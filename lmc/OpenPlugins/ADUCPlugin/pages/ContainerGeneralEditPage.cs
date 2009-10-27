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
public partial class ContainerGeneralEditPage : MPPage, IDirectoryPropertiesPage
{
    #region Class data
    private ContainerGenerelEditObject _editObject = null;
    private ContainerGenerelEditObject _originalObject = null;
    private ADUCDirectoryNode dirnode = null;
    #endregion
    
    #region Constructors
    public ContainerGeneralEditPage()
    {
        this.pageID = "ContainerGeneralEditProperities";
        InitializeComponent();
        SetPageTitle("General");
        
        _editObject = new ContainerGenerelEditObject();
        _originalObject = new ContainerGenerelEditObject();
    }
    #endregion
    
    #region Initialization methods
    /// <summary>
    /// Queries and fills the ldap message for the selected Container
    /// Gets the attribute list from AD for Container schema attribute.
    /// search for the attributes description, ou or name and displays them in a controls
    /// </summary>
    /// <param name="ce"></param>
    /// <param name="servername"></param>
    /// <param name="name"></param>
    /// <param name="dirnode"></param>
    public void SetData(CredentialEntry ce, string servername, string name, ADUCDirectoryNode dirnode)
    {
        try
        {
            _editObject = new ContainerGenerelEditObject();
            int ret = -1;
            this.dirnode = dirnode;
            
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
                    
                    if (string.Compare(sValue, "") == 0)
                    {
                        sValue = "<Not Set>";
                    }
                    
                    if (string.Compare(attr, "description") == 0)
                    {               
                        this.txtDescription.Text = attrValues[0].stringData;
                        _editObject.Description = sValue;
                    }
                    
                    if (string.Compare(attr, "cn") == 0)
                    {
                        this.userNamelabel.Text = sValue;
                    }
                }
            }
            
            if (_editObject != null)
            {
                _originalObject = (ContainerGenerelEditObject)_editObject.Clone();
            }
            else
            {
                _originalObject = new ContainerGenerelEditObject();
            }
            UpdateApplyButton();
            
        }
        catch (Exception e)
        {
            container.ShowError(e.Message);
        }
        // throw new NotImplementedException();
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
            _originalObject = (ContainerGenerelEditObject)_editObject.Clone();
        }
        else
        {
            _originalObject = new ContainerGenerelEditObject();
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
        
        if (_editObject.Description != "" &&
        !(_editObject.Description.Equals(_originalObject.Description)))
        {
            objectClass_values = new string[] { _editObject.Description, null };
            LDAPMod attr =
            new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "description",
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
        if (ret != 0)
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
    private void txtDescription_TextChanged(object sender, EventArgs e)
    {
        _editObject.Description = txtDescription.Text;
        UpdateApplyButton();
    }
    #endregion
}

public class ContainerGenerelEditObject : ICloneable
{
    public string Description = string.Empty;
    
    #region override Methods
    public override bool Equals(object obj)
    {
        if (obj == null)
        {
            return false;
        }
        if (!(obj is ContainerGenerelEditObject))
        {
            return false;
        }
        return GetHashCode() == (obj as ContainerGenerelEditObject).GetHashCode();
    }
    
    public virtual object Clone()
    {
        // Create a shallow copy first
        ContainerGenerelEditObject containerobject = MemberwiseClone() as ContainerGenerelEditObject;
        containerobject.Description = Description;
        return containerobject;
    }
    
    public override int GetHashCode()
    {
        return Description.GetHashCode();
    }
    #endregion
}
}
