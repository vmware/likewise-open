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
using System.Collections;
using System.Text;
using System.Windows.Forms;
using Likewise.LMC.Utilities;
using Likewise.LMC.LDAP.Interop;
using Likewise.LMC.ServerControl;
using Likewise.LMC.LDAP;

namespace Likewise.LMC.Plugins.ADUCPlugin
{
    public partial class MultiItemsGeneralEditPage : MPPage, IDirectoryPropertiesPage
    {
        #region Class Data

        private ADUCDirectoryNode dirnode = null;        
        private MultiItemPropertiesDlg parentDlg = null;

        #endregion

        #region Constructors

        public MultiItemsGeneralEditPage(MPContainer parent)
        {
            this.pageID = "MultiItemsGeneralEditPage";
            InitializeComponent();
            SetPageTitle("General");
            this.parentDlg = parent as MultiItemPropertiesDlg;
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
                this.dirnode = dirnode;
                SetControlStatus();

                int iOuCount = 0; int iUserCount = 0; int iCompCount = 0; int iGroupCount = 0; int iContactCount = 0;
                int iOtherCount = 0; int iTotalSummary = 0;

                foreach (ADUCDirectoryNode dn in this.parentDlg.ObjectCounts)
                {                    
                    if (dn != null)
                    {                        
                        if (dn.ObjectClass.Trim().Equals("OrganizationalUnit", StringComparison.InvariantCultureIgnoreCase))
                        {
                            iOuCount++;
                        }
                        else if (dn.ObjectClass.Trim().Equals("user", StringComparison.InvariantCultureIgnoreCase))
                        {
                            iUserCount++;
                        }
                        else if (dn.ObjectClass.Trim().Equals("group", StringComparison.InvariantCultureIgnoreCase))
                        {
                            iGroupCount++;
                        }
                        else if (dn.ObjectClass.Trim().Equals("computer", StringComparison.InvariantCultureIgnoreCase))
                        {
                            iCompCount++;
                        }
                        else if (dn.ObjectClass.Trim().Equals("contact", StringComparison.InvariantCultureIgnoreCase))
                        {
                            iContactCount++;
                        }
                        else
                        {
                            iOtherCount++;
                        }
                    }
                }
                if (iOuCount != 0)
                {
                    lblOUs.Show();
                    lblOUs.Text = iOuCount.ToString();
                    iTotalSummary += iOuCount;
                }
                if (iUserCount != 0)
                {
                    lblUsers.Show();
                    iTotalSummary += iUserCount;
                    lblUsers.Text = iUserCount.ToString();
                }
                if (iGroupCount != 0)
                {
                    lblGroups.Show();
                    iTotalSummary += iGroupCount;
                    lblGroups.Text = iGroupCount.ToString();
                }
                if (iCompCount != 0)
                {
                    lblComputers.Show();
                    iTotalSummary += iCompCount;
                    lblComputers.Text = iCompCount.ToString();
                }
                if (iContactCount != 0)
                {
                    lblContacts.Show();
                    iTotalSummary += iContactCount;
                    lblContacts.Text = iContactCount.ToString();
                }
                if (iOtherCount != 0)
                {
                    lblOthers.Show();
                    iTotalSummary += iOtherCount;
                    lblOthers.Text = iOtherCount.ToString();
                }
                lblSummary.Text = iTotalSummary.ToString();
            }
            catch (Exception e)
            {
                Logger.LogException("MultiItemsGeneralEditPage.SetData", e);
            }
        }

        private void SetControlStatus()
        {
            lblOUs.Hide();
            lblUsers.Hide();
            lblGroups.Hide();
            lblComputers.Hide();
            lblContacts.Hide();
            lblOthers.Hide();
        }

        /// <summary>
        /// Modifies the specified attributes for the selected AD Object either "user" to AD Schema template
        /// </summary>
        /// <returns></returns>
        public bool OnApply()
        {
            if (checkBox.Checked)
            {
                List<LDAPMod> ldapAttrlist = new List<LDAPMod>();
                List<LDAPMod> attrlist = new List<LDAPMod>();
                //the following portion of code uses openldap "ldap_Modify_s"               
                DirectoryContext dirContext = dirnode.LdapContext;
                string[] objectClass_values = null;

                objectClass_values = txtDescription.Text.Trim() == string.Empty ? new string[] { null } : new string[] { txtDescription.Text.Trim(), null };
                LDAPMod attr =
                new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "description",
                objectClass_values);
                attrlist.Add(attr);

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

                checkBox.Checked = false;
            }
            return true;
        }


        #endregion

        #region Events

        private void checkBox_CheckedChanged(object sender, EventArgs e)
        {
            txtDescription.Enabled = checkBox.Checked;
            label2.Enabled = checkBox.Checked;
            ParentContainer.DataChanged = checkBox.Checked;
            ParentContainer.btnApply.Enabled = checkBox.Checked;
        }

        #endregion
    }
}
