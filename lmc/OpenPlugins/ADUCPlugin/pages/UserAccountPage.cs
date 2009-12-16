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
using System.DirectoryServices;

namespace Likewise.LMC.Plugins.ADUCPlugin
{
    public partial class UserAccountPage : MPPage, IDirectoryPropertiesPage
    {
        #region Class Data

        private ADUCDirectoryNode dirnode;
        private ADUCPlugin plugin;

        private bool bAcountDisable, bNeverExpiresPwd, bMustChangePwd, bUserCannotChange,
                     bStorePwd, bSmartCardRequired, bAccSensitive, bUseDESDescription, bNotKrbAuthentication = false;

        private string Logonname = string.Empty;
        private string PreLogonname = string.Empty;
        private string sUserWorkStations = string.Empty;
        private string pwdLastSet = string.Empty;

        private string[] OptionsList = new string[]
                                {
                                    "User must change passowrd at next logon",
                                    "User cannot change password",
                                    "Password never expires",
                                    "Store password using reversible encryption",
                                    "Account is disabled",
                                    "Smart card is required for interactive logon",
                                    "Account is sensitive and cannot be delegated",
                                    "Use DES encryption types for this account",
                                    "Do not require Kerberos preauthentication"
                                };

        #endregion

        #region Constructors

        public UserAccountPage()
        {
            InitializeComponent();

            this.pageID = "UserAccountPage";
            SetPageTitle("Account");
        }

        #endregion

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
                this.plugin = dirnode.Plugin as ADUCPlugin;
                DirectoryContext dirContext = dirnode.LdapContext;
                Logonname = "";
                PreLogonname = "";

                //first obtain the current userAccountControl value
                DirectoryEntry de = new DirectoryEntry(string.Format("LDAP://{0}/{1}", dirContext.DomainName, dirnode.DistinguishedName));
                int userCtrlInt = Convert.ToInt32(de.Properties["userAccountControl"].Value.ToString());
                long pwdLastSet = Convert.ToInt64(de.Properties["pwdLastSet"].Value.ToString());
                sUserWorkStations = de.Properties["userWorkstations"].Value as string;

                if (de.Properties["userPrincipalName"].Value != null)
                {
                    Logonname = de.Properties["userPrincipalName"].Value as string;
                    Logonname = Logonname.IndexOf('@') >= 0 ? Logonname.Substring(0, Logonname.IndexOf('@')) : Logonname;
                    txtlogon.Text = Logonname;
                }

                txtpreLogonname.Text = de.Properties["sAMAccountName"].Value as string;
                PreLogonname = txtpreLogonname.Text.Trim();

                txtDomian.Text = dirContext.DomainName.Substring(0, dirContext.DomainName.IndexOf('.')).ToUpper() + @"\";
                cbDomain.Items.Add(string.Concat("@", dirContext.DomainName.ToUpper()));
                cbDomain.SelectedIndex = 0;

                double accountExpires = Convert.ToInt64(de.Properties["accountExpires"].Value);

                if (accountExpires == 9223372036854775807)
                    rbNever.Checked = true;
                else
                {
                    rbEndOf.Checked = true;
                    ConvertFromUnixTimestamp(accountExpires);
                }
                try
                {
                    string userCtrlBinStr = UserGroupUtils.DecimalToBase(userCtrlInt, 2);

                    if (userCtrlBinStr.Length >= 10 && userCtrlBinStr[userCtrlBinStr.Length - 10] == '1')
                    {
                        bMustChangePwd = true;
                    }
                    if (userCtrlBinStr.Length >= 10 && userCtrlBinStr[userCtrlBinStr.Length - 2] == '1')
                    {
                        bAcountDisable = true;
                    }
                    if (userCtrlBinStr.Length >= 17 && userCtrlBinStr[userCtrlBinStr.Length - 17] == '1')
                    {
                        bNeverExpiresPwd = true;
                        bMustChangePwd = false;
                    }
                    if (userCtrlBinStr.Length >= 10 && userCtrlBinStr[userCtrlBinStr.Length - 7] == '1'
                        && pwdLastSet != 0)
                    {
                        bUserCannotChange = true;
                    }
                    if (userCtrlBinStr.Length >= 8 && userCtrlBinStr[userCtrlBinStr.Length - 8] == '1')
                    {
                        bStorePwd = true;
                    }
                    if (userCtrlBinStr.Length >= 19 && userCtrlBinStr[userCtrlBinStr.Length - 19] == '1')
                    {
                        bSmartCardRequired = true;
                    }
                    if (userCtrlBinStr.Length >= 21 && userCtrlBinStr[userCtrlBinStr.Length - 21] == '1')
                    {
                        bAccSensitive = true;
                    }
                    if (userCtrlBinStr.Length >= 22 && userCtrlBinStr[userCtrlBinStr.Length - 22] == '1')
                    {
                        bUseDESDescription = true;
                    }
                    if (userCtrlBinStr.Length >= 23 && userCtrlBinStr[userCtrlBinStr.Length - 23] == '1')
                    {
                        bNotKrbAuthentication = true;
                    }
                }
                catch
                {
                }

                FillUserOptions();

                dateTimePicker.Enabled = rbEndOf.Checked;

                this.ParentContainer.DataChanged = false;
                this.ParentContainer.btnApply.Enabled = false;
            }
            catch (Exception e)
            {
                Logger.LogException("UserAccountPage.SetData", e);
            }
        }

#endregion

        #region Helper Functions

        private void FillUserOptions()
        {
            ListUserOptions.Items.AddRange(OptionsList);

            foreach (string option in OptionsList)
            {
                int i = ListUserOptions.Items.IndexOf(option);
                switch (i)
                {
                    case 0: ListUserOptions.SetItemChecked(i, bMustChangePwd);
                        break;

                    case 1: ListUserOptions.SetItemChecked(i, bUserCannotChange);
                        break;

                    case 2: ListUserOptions.SetItemChecked(i, bNeverExpiresPwd);
                        break;

                    case 3: ListUserOptions.SetItemChecked(i, bStorePwd);
                        break;

                    case 4: ListUserOptions.SetItemChecked(i, bAcountDisable);
                        break;

                    case 5: ListUserOptions.SetItemChecked(i, bSmartCardRequired);
                        break;

                    case 6: ListUserOptions.SetItemChecked(i, bAccSensitive);
                        break;

                    case 7: ListUserOptions.SetItemChecked(i, bUseDESDescription);
                        break;

                    case 8: ListUserOptions.SetItemChecked(i, bNotKrbAuthentication);
                        break;
                }
            }
        }

        private void UpdateApplyButton()
        {
            this.ParentContainer.btnApply.Enabled = this.ParentContainer.DataChanged;
        }

        private UInt64 ConvertToUnixTimestamp(DateTime date)
        {
            UInt64 unixTimestamp = (UInt64)((TimeSpan)(date - new DateTime(1970, 1, 1, 0, 0, 0))).TotalSeconds;
            Logger.Log("ADUCPage.ConvertToUnixTimestamp : " + unixTimestamp.ToString());
            return unixTimestamp;
        }

        //.NET appears to lack these very elementary interoperability
        //functions for reading UNIX timestamps, even though
        //this capability is available in C/C++ in Windows.
        private void ConvertFromUnixTimestamp(double unixTimestamp)
        {
            try
            {
                DateTime origin = new DateTime(1970, 1, 1, 0, 0, 0).AddSeconds(unixTimestamp);
                dateTimePicker.Value = origin;
            }
            catch (Exception ex)
            {
                Logger.LogException("UserAccountPage:ConvertFromUnixTimestamp", ex);
            }
        }

        private long CalculateUserAccountControl()
        {
            long userAccCtrl = 512;

            foreach (string option in OptionsList)
            {
                int i = ListUserOptions.Items.IndexOf(option);

                if (ListUserOptions.GetItemChecked(i))
                {
                    switch (i)
                    {
                        case 0:
                            pwdLastSet = "0";
                            break;

                        case 1: userAccCtrl += 64;
                            pwdLastSet = ConvertToUnixTimestamp(DateTime.Now).ToString();
                            break;

                        case 2: userAccCtrl += 65536;
                            break;

                        case 3: userAccCtrl += 128;
                            break;

                        case 4: userAccCtrl += 2;
                            break;

                        case 5: userAccCtrl += 262144;
                            break;

                        case 6: userAccCtrl += 1048576;
                            break;

                        case 7: userAccCtrl += 2097152;
                            break;

                        case 8: userAccCtrl += 4194304;
                            break;
                    }
                }
            }
            return userAccCtrl;

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

			if (ListUserOptions.GetItemChecked(0) && ListUserOptions.GetItemChecked(1))
            {
                string Msg = "You cannot select both 'User must change passowrd at next logon' and 'User cannot change password'\nfor the same user";
                MessageBox.Show(this, Msg, CommonResources.GetString("Caption_Console"), MessageBoxButtons.OK, MessageBoxIcon.Information);
                ListUserOptions.SetItemChecked(1, false);
                return false;
            }

            if (ListUserOptions.GetItemChecked(0) && ListUserOptions.GetItemChecked(2))
            {
                string Msg = "You have selected 'Password never expires'. \nThe user will not be required to change the password at next logon.";
                MessageBox.Show(this, Msg, CommonResources.GetString("Caption_Console"), MessageBoxButtons.OK, MessageBoxIcon.Information);
                ListUserOptions.SetItemChecked(0, false);
                return false;
            }

            //the following portion of code uses openldap "ldap_Modify_s"
            string basedn = dirnode.DistinguishedName;
            DirectoryContext dirContext = dirnode.LdapContext;
            string[] objectClass_values = null;

            if (Logonname != null && !(Logonname.Trim().Equals(txtlogon.Text.Trim())))
            {
                objectClass_values = new string[] { txtlogon.Text.Trim(), null };
                LDAPMod attr =
                new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "userPrincipalName",
                objectClass_values);
                attrlist.Add(attr);
            }

            if (txtpreLogonname.Text.Trim().Length > 0 && !(PreLogonname.Trim().Equals(txtpreLogonname.Text.Trim())))
            {
                objectClass_values = new string[] { txtpreLogonname.Text.Trim(), null };
                LDAPMod attr =
                new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "sAMAccountName",
                objectClass_values);
                attrlist.Add(attr);
            }
            if (dateTimePicker.Enabled && dateTimePicker.Value != null)
            {
                objectClass_values = new string[] { ConvertToUnixTimestamp(dateTimePicker.Value).ToString(), null };
                LDAPMod attr =
                new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "accountExpires",
                objectClass_values);
                attrlist.Add(attr);
            }

            if (!String.IsNullOrEmpty(pwdLastSet))
            {
                objectClass_values = new string[] { pwdLastSet, null };
                LDAPMod attr =
                new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "pwdLastSet",
                objectClass_values);
                attrlist.Add(attr);
            }

            //userWorkstations attribute
            if (String.IsNullOrEmpty(sUserWorkStations))
                objectClass_values = new string[] { null };
            else
                objectClass_values = new string[] { sUserWorkStations, null };
            LDAPMod attri =
            new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "userWorkstations",
            objectClass_values);
            attrlist.Add(attri);

            if (ListUserOptions.SelectedIndices.Count > 0)
            {
                objectClass_values = new string[] { CalculateUserAccountControl().ToString(), null };
                LDAPMod attr =
                new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "userAccountControl",
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
            else
            {
                DirectoryEntry de = new DirectoryEntry(dirnode.DistinguishedName);
                de.Properties["pwdLastSet"].Value = pwdLastSet;
                de.CommitChanges();
            }
            return true;
        }

        #endregion

        #region Events

        private void btnLogonHours_Click(object sender, EventArgs e)
        {
            this.ParentContainer.DataChanged = txtlogon.Text.Trim().Length > 0;
            UpdateApplyButton();
        }

        private void btnLogonTo_Click(object sender, EventArgs e)
        {
            LogOnWorkStationsEditor _LogonstringForm = new LogOnWorkStationsEditor(sUserWorkStations);
            if (_LogonstringForm.ShowDialog(this) == DialogResult.OK)
            {
                if (!String.IsNullOrEmpty(_LogonstringForm.sMultiValuedStringAttrValue))
                {
                    sUserWorkStations = _LogonstringForm.sMultiValuedStringAttrValue;
                }
                else
                    sUserWorkStations = "";
            }

            this.ParentContainer.DataChanged = true;
            UpdateApplyButton();
        }

        private void rbNever_CheckedChanged(object sender, EventArgs e)
        {
            if (rbNever.Checked)
                bNeverExpiresPwd = rbNever.Checked;

            this.ParentContainer.DataChanged = true;
            UpdateApplyButton();
        }

        private void rbEndOf_CheckedChanged(object sender, EventArgs e)
        {
            dateTimePicker.Enabled = rbEndOf.Checked;

            this.ParentContainer.DataChanged = dateTimePicker.Enabled;
            UpdateApplyButton();
        }

        private void ListUserOptions_ItemCheck(object sender, ItemCheckEventArgs e)
        {
            this.ParentContainer.DataChanged = ListUserOptions.SelectedIndices.Count > 0;
            UpdateApplyButton();
        }

        private void cbAccLocked_CheckedChanged(object sender, EventArgs e)
        {
            this.ParentContainer.DataChanged = true;
            UpdateApplyButton();
        }

        private void txtlogon_TextChanged(object sender, EventArgs e)
        {
            Logonname = txtlogon.Text.Trim();
            this.ParentContainer.DataChanged = txtpreLogonname.Text.Trim().Length > 0;
            UpdateApplyButton();
        }

        private void txtpreLogonname_TextChanged(object sender, EventArgs e)
        {
            this.ParentContainer.DataChanged = txtpreLogonname.Text.Trim().Length > 0;
            UpdateApplyButton();
        }

        private void dateTimePicker_ValueChanged(object sender, EventArgs e)
        {
            dateTimePicker.Enabled = true;
            dateTimePicker.BringToFront();

            this.ParentContainer.DataChanged = true;
            UpdateApplyButton();
        }

        #endregion
    }
}
