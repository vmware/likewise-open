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
using Likewise.LMC.ServerControl;
using Likewise.LMC.Utilities;
using Likewise.LMC.LDAP;
using Likewise.LMC.LDAP.Interop;

namespace Likewise.LMC.Plugins.ADUCPlugin
{
    public partial class GroupGeneralEditPage : MPPage, IDirectoryPropertiesPage
    {
        #region Class region

        private GroupGenerelEditObject _editObject = null;
        private GroupGenerelEditObject _originalObject = null;
        private ADUCDirectoryNode dirnode = null;

        #endregion

        public GroupGeneralEditPage()
        {
            pageID = "GroupGeneralEditProperities";
            InitializeComponent();
            SetPageTitle("General");

            _editObject = new GroupGenerelEditObject();
            _originalObject = new GroupGenerelEditObject();
        }

        #region Initialization Methods

        /// <summary>
        /// Queries and fills the ldap message for the selected group
        /// Gets the attribute list from AD for group schema attribute.
        /// search for the attributes description, cn or name and displays them in a controls
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

                        sValue = sValue.Substring(0, sValue.Length);

                        if (string.Compare(sValue, "") == 0)
                        {
                            sValue = "<Not Set>";
                        }

                        if (string.Compare(attr, "cn") == 0)
                        {
                            this.lblGroupName.Text = sValue;
                        }

                        if (string.Compare(attr, "sAMAccountName") == 0)
                        {
                            _editObject.Name = sValue;
                            this.txtPrewinGroup.Text = sValue;
                        }

                        if (string.Compare(attr, "description") == 0)
                        {
                            this.txtDescription.Text = sValue;
                            _editObject.Description = sValue;
                        }

                        if (string.Compare(attr, "mail") == 0)
                        {
                            this.txtEmail.Text = sValue;
                            _editObject.Email = sValue;
                        }

                        if (string.Compare(attr, "groupType") == 0)
                        {
                            EnableCheckBox(sValue);
                            _editObject.GroupType = sValue;
                        }

                      
                        if (string.Compare(attr, "info") == 0)
                        {
                            this.txtNotes.Text = sValue;
                            _editObject.Notes = sValue;
                        }
                    }
                }
                if (_editObject != null)
                {
                    _originalObject = (GroupGenerelEditObject)_editObject.Clone();
                }
                else
                {
                    _originalObject = new GroupGenerelEditObject();
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
                _originalObject = (GroupGenerelEditObject)_editObject.Clone();
            }
            else
            {
                _originalObject = new GroupGenerelEditObject();
            }
        }

        /// <summary>
        /// Modifies the specified attributes for the selected AD Object either "user" to AD Schema template
        /// </summary>
        /// <returns></returns>
        public bool OnApply()
        {
            if (txtPrewinGroup.Text.Equals(string.Empty))
            {
                string sMsg =
                "This object must have a pre-Windows 2000 name." +
                "Enter a pre-Windows 2000 name, and then try again";
                container.ShowError(sMsg);
                return false;
            }
            List<LDAPMod> attrlist = new List<LDAPMod>();
            //the following portion of code uses openldap "ldap_Modify_s"
            string basedn = dirnode.DistinguishedName;
            DirectoryContext dirContext = dirnode.LdapContext;
            string[] objectClass_values = null;

            if (_editObject.Name != "" &&
            !(_editObject.Name.Equals(_originalObject.Name)))
            {
                objectClass_values = new string[] { _editObject.Name, null };
                LDAPMod attr =
                new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "sAMAccountName",
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
            if (_editObject.Email != "" &&
            !(_editObject.Email.Equals(_originalObject.Email)))
            {
                objectClass_values = new string[] { _editObject.Email, null };
                LDAPMod attr =
                new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "mail",
                objectClass_values);
                attrlist.Add(attr);
            }
            if (_editObject.Notes != "" &&
            !(_editObject.Notes.Equals(_originalObject.Notes)))
            {
                objectClass_values = new string[] { _editObject.Notes, null };
                LDAPMod attr =
                new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "info",
                objectClass_values);
                attrlist.Add(attr);
            }
            if (_editObject.GroupType != "" &&
           !(_editObject.GroupType.Equals(_originalObject.GroupType)))
            {
                if (_originalObject.GroupType.Equals("-2147483643"))
                {
                    return false;
                }
                else
                {
                    objectClass_values = new string[] { _editObject.GroupType, null };
                    LDAPMod attr =
                    new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "groupType",
                    objectClass_values);
                    attrlist.Add(attr);
                }
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

        private void txtPrewinGroup_TextChanged(object sender, EventArgs e)
        {
            if (txtPrewinGroup.Text.Trim().Length > 0)
            {
                _editObject.Name = txtPrewinGroup.Text;
                UpdateApplyButton();
            }
        }

        private void txtDescription_TextChanged(object sender, EventArgs e)
        {
            _editObject.Description = txtDescription.Text;
            UpdateApplyButton();
        }

        private void txtEmail_TextChanged(object sender, EventArgs e)
        {
            _editObject.Email = txtEmail.Text;
            UpdateApplyButton();
        }

        private void txtNotes_TextChanged(object sender, EventArgs e)
        {
            _editObject.Notes = txtNotes.Text;
            UpdateApplyButton();
        }

        private void rbtn_CheckedChanged(object sender, EventArgs e)
        {
            _editObject.GroupType = Checkgroup_Type();          
            UpdateApplyButton();
        }

        #endregion

        #region Methods

        private void EnableCheckBox(string gType)
        {
            switch (gType)
            {
                case "-2147483643":
                    rbtnDomainLocal.Text = "Builtin local";
                    rbtnDomainLocal.Checked = true;
                    rbtnSecurity.Checked = true;
                    groupboxGroupScope.Enabled = false;
                    groupBoxGroupType.Enabled = false;
                    break;

                case "-2147483644":
                    rbtnDomainLocal.Checked = true;
                    rbtnSecurity.Checked = true;
                    rbtnGlobal.Enabled = false;                  
                    break;

                case "-2147483646":
                    rbtnGlobal.Checked = true;
                    rbtnSecurity.Checked = true;
                    rbtnDomainLocal.Enabled = false;
                    break;

                case "-2147483640":
                    rbtnUniversal.Checked = true;
                    rbtnSecurity.Checked = true;
                    break;

                case "4":
                    rbtnDomainLocal.Checked = true;
                    rbtnDistribution.Checked = true;
                    rbtnGlobal.Enabled = false; 
                    break;

                case "2":
                    rbtnGlobal.Checked = true;
                    rbtnDistribution.Checked = true;
                    rbtnDomainLocal.Enabled = false;
                    break;

                case "8":
                    rbtnUniversal.Checked = true;
                    rbtnDistribution.Checked = true;
                    break;

                default:
                    rbtnGlobal.Checked = true;
                    rbtnSecurity.Checked = true;
                    rbtnDomainLocal.Enabled = false;
                    break;
            }          
        }

        private string Checkgroup_Type()
        {
            string gType = string.Empty;
            if (rbtnSecurity.Checked)
            {
                if (rbtnDomainLocal.Checked)
                {
                    gType = "-2147483644";
                }
                else if (rbtnGlobal.Checked)
                {
                    gType = "-2147483646";
                }
                else if (rbtnUniversal.Checked)
                {
                    gType = "-2147483640";
                }
            }
            else if (rbtnDistribution.Checked)
            {
                if (rbtnDomainLocal.Checked)
                {
                    gType = "4";
                }
                else if (rbtnGlobal.Checked)
                {
                    gType = "2";
                }
                else if (rbtnUniversal.Checked)
                {
                    gType = "8";
                }
            }
            return gType;
        }

        #endregion        
    }

    public class GroupGenerelEditObject : ICloneable
    {
        public string Name = string.Empty;
        public string Description = string.Empty;
        public string Email = string.Empty;
        public string Notes = string.Empty;
        public string GroupType = string.Empty;

        #region override Methods
        public override bool Equals(object obj)
        {
            if (obj == null)
            {
                return false;
            }
            if (!(obj is GroupGenerelEditObject))
            {
                return false;
            }
            return GetHashCode() == (obj as GroupGenerelEditObject).GetHashCode();
        }

        public virtual object Clone()
        {
            // Create a shallow copy first
            GroupGenerelEditObject other = MemberwiseClone() as GroupGenerelEditObject;
            other.Description = Description;
            other.Name = Name;
            other.Email = Email;
            other.Notes = Notes;
            other.GroupType = GroupType;
            return other;
        }

        public override int GetHashCode()
        {
            StringBuilder sb = new StringBuilder();
            sb.Append(Description);
            sb.Append(Name);
            sb.Append(Notes);
            sb.Append(Email);
            sb.Append(GroupType);
            return sb.ToString().GetHashCode();
        }
        #endregion

    }
}
