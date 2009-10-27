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
using Likewise.LMC.AuthUtils;
using Likewise.LMC.LMConsoleUtils;
using Likewise.LMC.LDAP;
using Likewise.LMC.LDAP.Interop;
using System.DirectoryServices;

namespace Likewise.LMC.Plugins.ADUCPlugin
{
    public partial class UserProfilePage : MPPage, IDirectoryPropertiesPage
    {
        #region Class Data

        private string profilePath, scriptPath, homeDirectory, homeDrive;

        private string[] Charlist ={ "D:","E:","F:","G:",
                                     "H:","I:","J:","K:",
                                     "L:","M:","N:","O:",
                                     "P:","Q:","R:","S:",
                                     "T:","U:","V:","W:",
                                     "X:","Y:","Z:"};

        private ADUCDirectoryNode dirnode;
        private bool bIsOpened = false;

        #endregion

        #region Constructors

        public UserProfilePage()
        {
            InitializeComponent();

            this.pageID = "UserProfilePage";
            SetPageTitle("Profile");
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

                DirectoryContext dirContext = dirnode.LdapContext;

                //first obtain the current userAccountControl value
                DirectoryEntry de = new DirectoryEntry(string.Format("LDAP://{0}/{1}", dirContext.DomainName, dirnode.DistinguishedName));

                profilePath = de.Properties["profilePath"].Value as string;
                scriptPath = de.Properties["scriptPath"].Value as string;
                homeDirectory = de.Properties["homeDirectory"].Value as string;
                homeDrive = de.Properties["homeDrive"].Value as string;

                foreach (string ch in Charlist)
                {
                    cbDrive.Items.Add(ch);
                }

                if (homeDrive != null)
                {
                    rbConnect.Checked = true;
                    txtConnect.Text = homeDirectory;
                    int selectedIndex = 0;
                    if (cbDrive.Items.Contains(homeDrive))
                    {
                        selectedIndex = cbDrive.Items.IndexOf(homeDrive);
                    }
                    else
                    {
                        selectedIndex = cbDrive.Items.Add(homeDrive);
                    }
                    cbDrive.SelectedIndex = selectedIndex;
                }
                else
                {
                    rbLocalPath.Checked = true;
                    txtLocalPath.Text = homeDirectory;
                    cbDrive.SelectedIndex = 0;
                }

                txtProfilePath.Text = profilePath;
                txtLogonScript.Text = scriptPath;   

                ParentContainer.DataChanged = false;

                ParentContainer.btnApply.Enabled = false;
            }
            catch (Exception e)
            {
                Logger.LogException("UserGeneralEditPage.SetData", e);
            }
        }       

        #endregion

        #region Helper Functions

        private void UpdateApplyButton()
        {
            this.ParentContainer.btnApply.Enabled = this.ParentContainer.DataChanged;
        }

        /// <summary>
        /// Modifies the specified attributes for the selected AD Object either "user" to AD Schema template
        /// </summary>
        /// <returns></returns>
        public bool OnApply()
        {
            if (rbConnect.Checked)
            {
                string Connect = txtConnect.Text.Trim();
                bool IsValid = true;
                if (String.IsNullOrEmpty(Connect) ||
                    Connect.Length < 3)
                {
                    IsValid = false;
                }
                else if (Connect.Length == 3 && Connect.IndexOf(@"\\") == 0)
                {
                    MessageBox.Show(this, "The home folder could not be created because: The filename, directory name, or valume" +
                                          "label syntax is incorrect",
                          CommonResources.GetString("Caption_Console"), MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                    return false;
                }
                else
                {
                    string[] Slashsplits = Connect.Substring(2).Split('\\');
                    if (String.IsNullOrEmpty(Connect) ||
                        Slashsplits.Length != 2 ||
                        txtConnect.Text.Trim().IndexOf(@"\\") != 0 ||
                        Connect.EndsWith(@"\"))
                    {
                        IsValid = false;
                    }
                }
                if (!IsValid)
                {
                    MessageBox.Show(this, "The specified path is not valid. Enter a valid network server path using the form:\n\\\\server\\share\\folder",
                           CommonResources.GetString("Caption_Console"), MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                    return false;
                }
            }

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

            if (txtProfilePath.Text.Trim().Length > 0)
            {
                objectClass_values = new string[] { txtProfilePath.Text.Trim(), null };
                LDAPMod attr =
                new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "profilePath",
                objectClass_values);
                attrlist.Add(attr);
            }
            if (txtLogonScript.Text.Trim().Length > 0)
            {
                objectClass_values = new string[] { txtLogonScript.Text.Trim(), null };
                LDAPMod attr =
                new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "scriptPath",
                objectClass_values);
                attrlist.Add(attr);
            }
            if (rbConnect.Checked)
            {
                if (txtConnect.Text.Trim().Length > 0)
                {
                    objectClass_values = new string[] { txtConnect.Text.Trim(), null };
                    LDAPMod attr =
                    new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "homeDirectory",
                    objectClass_values);
                    attrlist.Add(attr);
                }
                if (cbDrive.SelectedItem != null)
                {
                    objectClass_values = new string[] { cbDrive.SelectedItem.ToString(), null };
                    LDAPMod attr =
                    new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "homeDrive",
                    objectClass_values);
                    attrlist.Add(attr);
                }
            }
            else
            {
                if (txtLocalPath.Text.Trim().Length > 0)
                {
                    objectClass_values = new string[] { txtLocalPath.Text.Trim(), null };
                    LDAPMod attr =
                    new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "homeDirectory",
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
            else
            {
                if (rbConnect.Checked)
                {
                    string sMsg = string.Empty;
                    if (!bIsOpened)
                    {
                        sMsg = string.Format("The {0} home folder was not created because the path was not found. This could be caused by listing\n" +
                                      "non-existent intermediate folders or by not finding the server or share. The user account has been updated" +
                                      "with the new home folder value but you must create the folder manually", txtConnect.Text.Trim());
                        bIsOpened = true;
                    }
                    else
                    {
                        sMsg = "The home folder could not be created becuase: The network location cannot be reached." +
                            "\nFor information about network troubleshooting, see the google map";
                    }

                    MessageBox.Show(this, sMsg,
                       CommonResources.GetString("Caption_Console"), MessageBoxButtons.OK, MessageBoxIcon.Exclamation);                   
                }
            }

            return true;
        }       

        #endregion

        #region Event Handlers

        private void txtProfilePath_TextChanged(object sender, EventArgs e)
        {
            ParentContainer.DataChanged = txtProfilePath.Text.Trim().Length > 0;
            UpdateApplyButton();
        }

        private void txtLogonScript_TextChanged(object sender, EventArgs e)
        {
            ParentContainer.DataChanged = txtLogonScript.Text.Trim().Length > 0;
            UpdateApplyButton();
        }

        private void txtLocalPath_TextChanged(object sender, EventArgs e)
        {
            ParentContainer.DataChanged = txtLocalPath.Text.Trim().Length > 0;
            UpdateApplyButton();
        }

        private void txtConnect_TextChanged(object sender, EventArgs e)
        {
            ParentContainer.DataChanged = txtConnect.Text.Trim().Length > 0;
            UpdateApplyButton();
        }        

        private void rbLocalPath_CheckedChanged(object sender, EventArgs e)
        {
            txtLocalPath.Enabled = rbLocalPath.Checked;

            ParentContainer.DataChanged = true;
            UpdateApplyButton();
        }       

        private void rbConnect_CheckedChanged(object sender, EventArgs e)
        {
            txtConnect.Enabled = cbDrive.Enabled = rbConnect.Checked;

            ParentContainer.DataChanged = true;
            UpdateApplyButton();
        }       

        private void cbDrive_SelectedIndexChanged(object sender, EventArgs e)
        {
            ParentContainer.DataChanged = true;
            UpdateApplyButton();
        }

        #endregion
    }
}
