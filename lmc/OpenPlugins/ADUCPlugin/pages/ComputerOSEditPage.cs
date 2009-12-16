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
using Likewise.LMC.Utilities;
using Likewise.LMC.ServerControl;
using System.DirectoryServices;


namespace Likewise.LMC.Plugins.ADUCPlugin
{
    public partial class ComputerOSEditPage : MPPage, IDirectoryPropertiesPage
    {
        #region Class region

        private ADUCDirectoryNode dirnode = null;
        private string operatingSystem = string.Empty;
        private string operatingSystemServicePack = string.Empty;
        private string operatingSystemVersion = string.Empty;

        #endregion

        public ComputerOSEditPage()
        {
            pageID = "ComputerOSEditPage";
            InitializeComponent();
            SetPageTitle("Operating System");
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

                        sValue = sValue.Substring(0, sValue.Length);

                        if (string.Compare(sValue, "") == 0)
                        {
                            sValue = "<Not Set>";
                        }

                        if (string.Compare(attr, "operatingSystem") == 0)
                        {
                            txtName.Text = sValue;
                            operatingSystem = txtName.Text.Trim();
                        }

                        if (string.Compare(attr, "operatingSystemServicePack") == 0)
                        {
                            txtServicePack.Text = sValue;
                            operatingSystemServicePack = txtServicePack.Text.Trim();
                        }

                        if (string.Compare(attr, "operatingSystemVersion") == 0)
                        {
                            txtVersion.Text = sValue;
                            operatingSystemVersion = txtVersion.Text.Trim();
                        }
                    }
                }
                this.ParentContainer.DataChanged = false;
                UpdateApplyButton();
            }
            catch (Exception e)
            {
                container.ShowError(e.Message);
            }
        }

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
            return true;
        }

        #endregion

        #region Events


        #endregion
    }

}
