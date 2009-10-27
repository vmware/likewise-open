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
using Likewise.LMC.LDAP.Interop;
using Likewise.LMC.ServerControl;
using Likewise.LMC.AuthUtils;
using Likewise.LMC.LDAP;
using System.Globalization;

namespace Likewise.LMC.Plugins.ADUCPlugin
{
    public partial class MultiItemsAddressEditPage : MPPage, IDirectoryPropertiesPage
    {
        #region Class Data

        private MPContainer parentDlg = null;
        private ADUCDirectoryNode dirnode = null;
        private bool bMultiUserSelected = false;

        #endregion

        public MultiItemsAddressEditPage(MPContainer parent, bool bMultiUser)
        {
            this.pageID = "UserMultiSelectAddressProperities";
            InitializeComponent();
            SetPageTitle("Address");
            this.parentDlg = parent;
            bMultiUserSelected = bMultiUser;
            HideCheckbox();
        }

        #region Initialization Methods
        public void SetData(CredentialEntry ce, string servername, string name, ADUCDirectoryNode dirnode)
        {
            try
            {
                InitializeCountryNames();
                if (!bMultiUserSelected)
                {
                    int ret = -1;
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

                            if (string.Compare(attr, "streetAddress") == 0)
                            {
                                this.txtStreet.Text = sValue;
                                chkStreet.Checked = true;
                            }
                            if (string.Compare(attr, "postOfficeBox") == 0)
                            {
                                this.txtPOBox.Text = sValue;
                                chkPO.Checked = true;
                            }
                            if (string.Compare(attr, "l") == 0)
                            {
                                this.txtCity.Text = sValue;
                                chkCity.Checked = true;
                            }
                            if (string.Compare(attr, "st") == 0)
                            {
                                this.txtState.Text = sValue;
                                chkState.Checked = true;
                            }
                            if (string.Compare(attr, "postalCode") == 0)
                            {
                                this.txtZip.Text = sValue;
                                chkZip.Checked = true;
                            }
                            if (string.Compare(attr, "co") == 0)
                            {
                                bool bEntryFound = false;
                                for (int i = 0; i < cbCountry.Items.Count; i++)
                                {
                                    if (sValue.Trim().Equals(cbCountry.Items[i].ToString().Trim()))
                                    {
                                        cbCountry.SelectedIndex = i;
                                        bEntryFound = true;
                                        break;
                                    }
                                }
                                if (bEntryFound)
                                {
                                    this.cbCountry.Items.Add(sValue);
                                    this.cbCountry.SelectedIndex = cbCountry.Items.Count - 1;
                                }
                                chkCountry.Checked = true;
                            }
                        }
                    }
                }
                else if (bMultiUserSelected)
                {
                    this.dirnode = dirnode;
                    txtCity.Text = "";
                    txtPOBox.Text = "";
                    txtStreet.Text = "";
                    txtState.Text = "";
                    txtZip.Text = "";
                }
                ParentContainer.DataChanged = false;
            }
            catch (Exception e)
            {
                Logger.LogException("UserMultiEditPage.SetData", e);
            }
        }


        public bool OnApply()
        {
            List<LDAPMod> attrlist = new List<LDAPMod>();
            LDAPMod attr = null;
            DirectoryContext dirContext = dirnode.LdapContext;
            string[] objectClass_values = null;

            string street = txtStreet.Text.Trim();
            if (street.Contains("\n"))
            {
                street = street.Replace("\n", "\r\n");
            }

            objectClass_values = street == string.Empty ? new string[] { null } : new string[] { street, null };                
            attr =
            new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "streetAddress",
            objectClass_values);
            if (bMultiUserSelected && chkStreet.Checked)
            {
                attrlist.Add(attr);
            }
            else if (!bMultiUserSelected && !String.IsNullOrEmpty(txtStreet.Text.Trim()))
            {
                attrlist.Add(attr);
            }

            objectClass_values = txtPOBox.Text.Trim() == string.Empty ? new string[] { null } : new string[] { txtPOBox.Text.Trim(), null };                
            attr =
            new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "postOfficeBox",
            objectClass_values);           
            if (bMultiUserSelected && chkPO.Checked)
            {
                attrlist.Add(attr);
            }
            else if (!bMultiUserSelected && !String.IsNullOrEmpty(txtPOBox.Text.Trim()))
            {
                attrlist.Add(attr);
            }

            objectClass_values = txtCity.Text.Trim() == string.Empty ? new string[] { null } : new string[] { txtCity.Text.Trim(), null };                
            attr =
            new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "l",
            objectClass_values);           
            if (bMultiUserSelected && chkCity.Checked)
            {
                attrlist.Add(attr);
            }
            else if (!bMultiUserSelected && !String.IsNullOrEmpty(txtCity.Text.Trim()))
            {
                attrlist.Add(attr);
            }

            objectClass_values = txtState.Text.Trim() == string.Empty ? new string[] { null } : new string[] { txtState.Text.Trim(), null };                
            attr =
            new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "st",
            objectClass_values);           
            if (bMultiUserSelected && chkState.Checked)
            {
                attrlist.Add(attr);
            }
            else if (!bMultiUserSelected && !String.IsNullOrEmpty(txtState.Text.Trim()))
            {
                attrlist.Add(attr);
            }

            objectClass_values = txtZip.Text.Trim() == string.Empty ? new string[] { null } : new string[] { txtZip.Text.Trim(), null };                
            attr =
            new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "postalCode",
            objectClass_values);           
            if (bMultiUserSelected && chkZip.Checked)
            {
                attrlist.Add(attr);
            }
            else if (!bMultiUserSelected && !String.IsNullOrEmpty(txtZip.Text.Trim()))
            {
                attrlist.Add(attr);
            }

            objectClass_values = cbCountry.Text.Trim() == string.Empty ? new string[] { null } : new string[] { cbCountry.Text.Trim(), null };                
            attr =
            new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "co",
            objectClass_values);           
            if (bMultiUserSelected && chkCountry.Checked)
            {
                attrlist.Add(attr);
            }
            else if (!bMultiUserSelected && !String.IsNullOrEmpty(cbCountry.Text.Trim()))
            {
                attrlist.Add(attr);
            }

            SetControlStatus();

            LDAPMod[] attrArry = attrlist.ToArray();
            int ret = -1;
            if (attrArry != null && attrArry.Length != 0)
            {
                List<object> dirnodes = new List<object>();
                if (parentDlg is MultiItemPropertiesDlg)
                {
                    MPContainer _MultiItemPropertiesDlg = parentDlg as MPContainer;                    
                    dirnodes = _MultiItemPropertiesDlg.ObjectCounts;
                }
                else
                {
                    MPContainer _ADUserPropertiesDlg = parentDlg as MPContainer;
                    dirnodes = _ADUserPropertiesDlg.ObjectCounts;
                }
                foreach (ADUCDirectoryNode dn in dirnodes)
                {
                    if (dn != null)
                    {
                        ret = dirContext.ModifySynchronous(dn.DistinguishedName, attrArry);
                    }
                    if (ret != 0)
                    {
                        string sMsg = ErrorCodes.LDAPString(ret);                        
                        container.ShowError(sMsg);
                        return false;
                    }
                }
            }

            return true;
        }
        #endregion

        #region Helper functions

        private void SetControlStatus()
        {
            chkStreet.Checked = false;
            chkPO.Checked = false;
            chkCity.Checked = false;
            chkState.Checked = false;
            chkZip.Checked = false;
            chkCountry.Checked = false;

            txtCity.Text = txtPOBox.Text = txtState.Text = txtStreet.Text = txtZip.Text = "";
            cbCountry.Enabled = false;
        }

        private void HideCheckbox()
        {
            if (!bMultiUserSelected)
            {
                chkStreet.Visible = false;
                chkPO.Visible = false;
                chkCity.Visible = false;
                chkState.Visible = false;
                chkZip.Visible = false;
                chkCountry.Visible = false;

                lbCity.Visible = true;
                lbCountry.Visible = true;
                lbPOBox.Visible = true;
                lbState.Visible = true;
                lbStreet.Visible = true;
                lbZip.Visible = true;

                txtStreet.Enabled = true;
                txtPOBox.Enabled = true;
                txtCity.Enabled = true;
                txtState.Enabled = true;
                txtZip.Enabled = true;
                cbCountry.Enabled = true;
            }
        }

        private void InitializeCountryNames()
        {
            if (cbCountry.Items.Count != 0)
            {
                cbCountry.Items.Clear();
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
                list.Sort();
                countries = list.ToArray();
                cbCountry.Items.AddRange(countries);
            }
        }

     
        private void UpdateApplyButton()
        {
            ParentContainer.DataChanged = true;
            ParentContainer.btnApply.Enabled = true;
        }

        #endregion

        #region Events

        private void chkStreet_CheckedChanged(object sender, EventArgs e)
        {
            if (chkStreet.Checked)
            {
                txtStreet.Enabled = true;
                UpdateApplyButton();
            }
            else
                txtStreet.Enabled = false;
        }

        private void chkPO_CheckedChanged(object sender, EventArgs e)
        {
            if (chkPO.Checked)
            {
                txtPOBox.Enabled = true;
                UpdateApplyButton();
            }
            else
                txtPOBox.Enabled = false;
        }

        private void chkCity_CheckedChanged(object sender, EventArgs e)
        {
            if (chkCity.Checked)
            {
                txtCity.Enabled = true;
                UpdateApplyButton();
            }
            else
                txtCity.Enabled = false;
        }

        private void chkState_CheckedChanged(object sender, EventArgs e)
        {
            if (chkState.Checked)
            {
                txtState.Enabled = true;
                UpdateApplyButton();
            }
            else
                txtState.Enabled = false;
        }

        private void chkZip_CheckedChanged(object sender, EventArgs e)
        {
            if (chkZip.Checked)
            {
                txtZip.Enabled = true;
                UpdateApplyButton();
            }
            else
                txtZip.Enabled = false;
        }

        private void chkCountry_CheckedChanged(object sender, EventArgs e)
        {
            if (chkCountry.Checked)
            {
                cbCountry.Enabled = true;
                UpdateApplyButton();
            }
            else
                cbCountry.Enabled = false;
        }        

        private void txtStreet_TextChanged(object sender, EventArgs e)
        {
            UpdateApplyButton();
        }

        private void txtPOBox_TextChanged(object sender, EventArgs e)
        {
            UpdateApplyButton();
        }

        private void txtCity_TextChanged(object sender, EventArgs e)
        {
            UpdateApplyButton();
        }

        private void txtState_TextChanged(object sender, EventArgs e)
        {
            UpdateApplyButton();
        }

        private void txtZip_TextChanged(object sender, EventArgs e)
        {
            UpdateApplyButton();
        }

        private void cbCountry_SelectedIndexChanged(object sender, EventArgs e)
        {
            UpdateApplyButton();
        }

        #endregion
    }
}
