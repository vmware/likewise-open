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
using Likewise.LMC.Utilities;
using Likewise.LMC.LDAP.Interop;
using Likewise.LMC.ServerControl;
using Likewise.LMC.LDAP;
using System.Globalization;


namespace Likewise.LMC.Plugins.ADUCPlugin
{
    public partial class OUGeneralEditPage : MPPage, IDirectoryPropertiesPage
    {
        #region Class data
        private OUGenerelEditObject _editObject = null;
        private OUGenerelEditObject _originalObject = null;
        private ADUCDirectoryNode dirnode = null;        
        #endregion

        #region Constructors
        public OUGeneralEditPage()
        {
            this.pageID = "OUGeneralEditProperities";
            InitializeComponent();
            SetPageTitle("General");

            _editObject = new OUGenerelEditObject();
            _originalObject = new OUGenerelEditObject();
        }
        #endregion

        #region Initialization methods

        /// <summary>
        /// Queries and fills the ldap message for the selected OU
        /// Gets the attribute list from AD for OU schema attribute.
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
                InitializeCountryNames();
                _editObject = new OUGenerelEditObject();
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
                            this.txtDescription.Text = sValue;
                            _editObject.Description = sValue;
                        }

                        if (string.Compare(attr, "street") == 0)
                        {
                            this.rtbStreet.Text = sValue;
                            _editObject.Street = sValue;
                        }

                        if (string.Compare(attr, "l") == 0)
                        {
                            this.txtCity.Text = sValue;
                            _editObject.City = sValue;
                        }

                        if (string.Compare(attr, "st") == 0)
                        {
                            this.txtstate.Text = sValue;
                            _editObject.State = sValue;
                        }

                        if (string.Compare(attr, "postalCode") == 0)
                        {
                            this.txtZip.Text = sValue;
                            _editObject.PostalCode = sValue;
                        }

                        if (string.Compare(attr, "co") == 0)
                        {
                            bool bEntryFound = false;
                            for (int i = 0; i < cbcountry.Items.Count; i++)
                            {
                                if (sValue.Trim().ToLower().Equals(cbcountry.Items[i].ToString().Trim().ToLower()))
                                {
                                    cbcountry.SelectedIndex = i;
                                    bEntryFound = true;
                                    break;
                                }
                            }
                            if (!bEntryFound)
                            {
                                cbcountry.Items.Add(sValue);
                                cbcountry.SelectedIndex = cbcountry.Items.Count - 1;
                            }                           
                            _editObject.Country = sValue;
                        }

                        if (string.Compare(attr, "ou") == 0)
                        {
                            this.userNamelabel.Text = sValue;
                        }
                    }
                }
                if (_editObject != null)
                {
                    _originalObject = (OUGenerelEditObject)_editObject.Clone();
                }
                else
                {
                    _originalObject = new OUGenerelEditObject();
                }
                ParentContainer.DataChanged = false;
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
                _originalObject = (OUGenerelEditObject)_editObject.Clone();
            }
            else
            {
                _originalObject = new OUGenerelEditObject();
            }
        }

        private void InitializeCountryNames()
        {
            if (cbcountry.Items.Count != 0)
            {
                cbcountry.Items.Clear();
            }
                       
            List<string> list = new List<string>();
            foreach (CultureInfo info in CultureInfo.GetCultures(CultureTypes.SpecificCultures))
            {
                string culture = info.DisplayName;
                if (culture.IndexOf('(') > 0)
                {
                    culture = culture.Substring(culture.IndexOf('(') + 1, culture.IndexOf(')') - culture.IndexOf('(') - 1);
                }
                if (culture.Contains(","))
                {
                    foreach (string str in culture.Split(','))
                    {
                        if (!list.Contains(str))
                        {
                            list.Add(str.Trim());
                        }
                    }
                }
                else if (!list.Contains(culture))
                {
                    list.Add(culture.Trim());
                }
            }
            string[] countries = list.ToArray();

            if (countries != null && countries.Length > 0)
            {
                cbcountry.Items.AddRange(countries);
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
            if (_editObject.Street != "" &&
            !(_editObject.Street.Equals(_originalObject.Street)))
            {
                objectClass_values = new string[] { _editObject.Street, null };
                LDAPMod attr =
                new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "street",
                objectClass_values);
                attrlist.Add(attr);
            }
            if (_editObject.City != "" &&
            !(_editObject.City.Equals(_originalObject.City)))
            {
                objectClass_values = new string[] { _editObject.City, null };
                LDAPMod attr =
                new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "l",
                objectClass_values);
                attrlist.Add(attr);
            }
            if (_editObject.State != "" &&
            !(_editObject.State.Equals(_originalObject.State)))
            {
                objectClass_values = new string[] { _editObject.State, null };
                LDAPMod attr =
                new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "st",
                objectClass_values);
                attrlist.Add(attr);
            }
            if (_editObject.PostalCode != "" &&
            !(_editObject.PostalCode.Equals(_originalObject.PostalCode)))
            {
                objectClass_values = new string[] { _editObject.PostalCode, null };
                LDAPMod attr =
                new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "postalCode",
                objectClass_values);
                attrlist.Add(attr);
            }
            if (_editObject.Country != "" &&
            !(_editObject.Country.Equals(_originalObject.Country)))
            {
                objectClass_values = new string[] { _editObject.Country, null };
                LDAPMod attr =
                new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "co",
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

        private void rtbStreet_TextChanged(object sender, EventArgs e)
        {
            _editObject.Street = rtbStreet.Text;
            UpdateApplyButton();
        }

        private void txtCity_TextChanged(object sender, EventArgs e)
        {
            _editObject.City = txtCity.Text;
            UpdateApplyButton();
        }

        private void txtstate_TextChanged(object sender, EventArgs e)
        {
            _editObject.State = txtstate.Text;
            UpdateApplyButton();
        }

        private void txtZip_TextChanged(object sender, EventArgs e)
        {
            _editObject.PostalCode = txtZip.Text;
            UpdateApplyButton();
        }

        private void cbcountry_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (cbcountry.SelectedItem == null)
            {
                return;
            }
            _editObject.Country = cbcountry.SelectedItem.ToString();
            UpdateApplyButton();
        }

        #endregion
    }

    public class OUGenerelEditObject : ICloneable
    {
        public string Description = string.Empty;
        public string Street = string.Empty;
        public string City = string.Empty;
        public string State = string.Empty;
        public string PostalCode = string.Empty;
        public string Country = string.Empty;

        #region override Methods
        public override bool Equals(object obj)
        {
            if (obj == null)
            {
                return false;
            }
            if (!(obj is OUGenerelEditObject))
            {
                return false;
            }
            return GetHashCode() == (obj as OUGenerelEditObject).GetHashCode();
        }

        public virtual object Clone()
        {
            // Create a shallow copy first
            OUGenerelEditObject OUObject = MemberwiseClone() as OUGenerelEditObject;
            OUObject.Description = Description;
            OUObject.Street = Street;
            OUObject.City = City;
            OUObject.State = State;
            OUObject.PostalCode = PostalCode;
            OUObject.Country = Country;
            return OUObject;
        }

        public override int GetHashCode()
        {
            StringBuilder sb = new StringBuilder();
            sb.Append(Description);
            sb.Append(Street);
            sb.Append(City);
            sb.Append(State);
            sb.Append(PostalCode);
            sb.Append(Country);
            return sb.ToString().GetHashCode();
        }
        #endregion

    }
}
