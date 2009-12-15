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


namespace Likewise.LMC.Plugins.ADUCPlugin
{
    public partial class UserMultiselectGeneralEditPage : MPPage, IDirectoryPropertiesPage
    {
        #region classData
        //private UserMultiEditObject _editObject = null;
        //private UserMultiEditObject _originalObject = null;
        private ADUCDirectoryNode dirnode = null;
        private MultiItemPropertiesDlg parentDlg = null;
        #endregion

        public UserMultiselectGeneralEditPage(MPContainer parent)
        {
            this.pageID = "UserMultiSelectProperities";
            InitializeComponent();
            SetPageTitle("General");
            this.parentDlg = parent as MultiItemPropertiesDlg;
            //InitializeComponent();
        }

        #region Initialize Methods
        /// <summary>
        ///
        /// </summary>
        public void SetData(CredentialEntry ce, string servername, string name, ADUCDirectoryNode dirnode)
        {
            try
            {
                this.dirnode = dirnode;
                SetControlStatus();
            }
            catch (Exception e)
            {
                Logger.LogException("UserMultiEditPage.SetData", e);
            }
        }
        public bool OnApply()
        {
            List<LDAPMod> attrlist = new List<LDAPMod>();
            LDAPMod attr=null;
                DirectoryContext dirContext = dirnode.LdapContext;
                string[] objectClass_values = null;

                if (chkDescription.Checked)
                {
                    objectClass_values = txtDescription.Text.Trim() == string.Empty ? new string[] { null } : new string[] { txtDescription.Text.Trim(), null };
                    attr =
                    new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "description",
                    objectClass_values);
                    attrlist.Add(attr);
                }

                if (chkEmail.Checked)
                {
                    objectClass_values = txtEmail.Text.Trim() == string.Empty ? new string[] { null } : new string[] { txtEmail.Text.Trim(), null };
                    attr =
                    new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "mail",
                    objectClass_values);
                    attrlist.Add(attr);
                }
                if (chkOffice.Checked)
                {
                    objectClass_values = txtOffice.Text.Trim() == string.Empty ? new string[] { null } : new string[] { txtOffice.Text.Trim(), null };
                    attr =
                    new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "physicalDeliveryOfficeName",
                    objectClass_values);
                    attrlist.Add(attr);
                }
                if (chkTelephone.Checked)
                {
                    objectClass_values = txtTelephone.Text.Trim() == string.Empty ? new string[] { null } : new string[] { txtTelephone.Text.Trim(), null };
                    attr =
                    new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "telephoneNumber",
                    objectClass_values);
                    attrlist.Add(attr);
                }

                if (chkWebpage.Checked)
                {
                    objectClass_values = txtWebpage.Text.Trim() == string.Empty ? new string[] { null } : new string[] { txtWebpage.Text.Trim(), null };
                    attr =
                    new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "wWWHomePage",
                    objectClass_values);
                    attrlist.Add(attr);
                }
                SetControlStatus();
                LDAPMod[] attrArry = attrlist.ToArray();
                int ret = -1;
                if (attrArry != null && attrArry.Length != 0)
                {
                    foreach (ADUCDirectoryNode dn in this.parentDlg.ObjectCounts)
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
                else
                {
                    return true;
                }

            return true;
        }

        private void SetControlStatus()
        {
            chkDescription.Checked = false;
            chkEmail.Checked = false;
            chkOffice.Checked = false;
            chkTelephone.Checked = false;
            chkWebpage.Checked = false;
            chkFax.Checked = false;

            txtDescription.Text = "";
            txtEmail.Text = "";
            txtOffice.Text = "";
            txtTelephone.Text = "";
            txtWebpage.Text = "";
        }

        #endregion

        private void chkDescription_CheckedChanged(object sender, EventArgs e)
        {
            if (chkDescription.Checked)
            {
                ParentContainer.DataChanged = this.ParentContainer.btnApply.Enabled = chkDescription.Checked;
                txtDescription.Enabled = true;
            }
            else
                txtDescription.Enabled = false;
        }

        private void chkOffice_CheckedChanged(object sender, EventArgs e)
        {
            if (chkOffice.Checked)
            {
                ParentContainer.DataChanged = this.ParentContainer.btnApply.Enabled = chkDescription.Checked;
                txtOffice.Enabled = true;
            }
            else
                txtOffice.Enabled = false;
        }

        private void chkTelephone_CheckedChanged(object sender, EventArgs e)
        {
            if (chkTelephone.Checked)
            {
                ParentContainer.DataChanged = this.ParentContainer.btnApply.Enabled = chkDescription.Checked;
                txtTelephone.Enabled = true;
            }
            else
                txtTelephone.Enabled = false;
        }

        private void chkWebpage_CheckedChanged(object sender, EventArgs e)
        {
            if (chkWebpage.Checked)
            {
                ParentContainer.DataChanged = this.ParentContainer.btnApply.Enabled = chkDescription.Checked;
                txtWebpage.Enabled = true;
            }
            else
                txtWebpage.Enabled = false;
        }

        private void chkEmail_CheckedChanged(object sender, EventArgs e)
        {
            if (chkEmail.Checked)
            {
                ParentContainer.DataChanged = this.ParentContainer.btnApply.Enabled = chkDescription.Checked;
                txtEmail.Enabled = true;
            }
            else
                txtEmail.Enabled = false;
        }

        private void chkFax_CheckedChanged(object sender, EventArgs e)
        {
            if (chkFax.Checked)
            {
                ParentContainer.DataChanged = this.ParentContainer.btnApply.Enabled = chkEmail.Checked;
                txtFax.Enabled = true;
            }
            else
                txtFax.Enabled = false;
        }
    }
}
