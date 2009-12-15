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
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using Likewise.LMC.LDAP.Interop;
using Likewise.LMC.ServerControl;
using Likewise.LMC.LDAP;

namespace Likewise.LMC.Plugins.ADUCPlugin
{
    public partial class ADGroupAddDlg : Form
    {
        #region Class Data

        IPlugInContainer IPlugInContainer;
        public GroupInfo groupInfo;
        private ADUCDirectoryNode _dirnode;

        #endregion

        #region Constructors

        public ADGroupAddDlg()
        {
            InitializeComponent();
            groupInfo = new GroupInfo();
        }
        public ADGroupAddDlg(IPlugInContainer container, StandardPage parentPage, ADUCDirectoryNode dirnode)
            : this()
        {
            int ret = -1;
            this.IPlugInContainer = container;
            this._dirnode = dirnode;
            if (dirnode != null)
            {
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
                //As of now we are not getting canonicalName attribute in the list because of paging issue
                //LdapValue[] attr = ldapNextEntry.GetAttributeValues("canonicalName", dirnode.LdapContext);
                //if (attr != null && attr.Length > 0)
                //    this.lblCreatein.Text = "Create in: " + attr[0].stringData;

                //As of now we are taking "DistinguishedName" and spliting and displaying it.
                string[] sData = dirnode.DistinguishedName.Split(',');
                string sOutput = "";
                for (int i = sData.Length - 1; i >= 0; i--)
                {
                    if (sData[i].ToString().Trim().StartsWith("DC", StringComparison.InvariantCultureIgnoreCase))
                    {
                        sOutput = sData[i].ToString().Trim().Substring(3) + "." + sOutput;
                    }
                    else if (sData[i].ToString().Trim().StartsWith("OU", StringComparison.InvariantCultureIgnoreCase) || sData[i].ToString().Trim().StartsWith("CN", StringComparison.InvariantCultureIgnoreCase))
                    {
                        sOutput += "/" + sData[i].ToString().Trim().Substring(3);
                    }
                }
                if (sOutput.EndsWith("."))
                {
                    sOutput += "/";
                }
                sOutput = sOutput.Replace("./", "/");

                this.txtcreatein.Text = "Create in: " + sOutput;
            }
        }
        #endregion

        #region Event Handlers

        private void btnOk_Click(object sender, EventArgs e)
        {
            if (_dirnode != null)
            {
                string filterquery = string.Format("(&(objectClass=*)(name={0}))", txtGroupname.Text.Trim());
                List<LdapEntry> ldapEntries = null;
                int ret = _dirnode.LdapContext.ListChildEntriesSynchronous
                (_dirnode.DistinguishedName,
                LdapAPI.LDAPSCOPE.ONE_LEVEL,
                filterquery,
                null,
                false,
                out ldapEntries);

                if (ldapEntries != null && ldapEntries.Count != 0)
                {
                    string sMsg = string.Format(
                    "Windows cannot create the object {0} because \n" +
                    "An attempt was made to add an object to the directory with a name that is already in use.",
                    txtGroupname.Text.Trim());
                    MessageBox.Show(this, sMsg, CommonResources.GetString("Caption_Console"),
                    MessageBoxButtons.OK, MessageBoxIcon.Error);
                    return;
                }
            }

            this.DialogResult = DialogResult.OK;
            groupInfo.GroupName = txtGroupname.Text.Trim();
            groupInfo.PreWindowsgroupname = txtprewin.Text.Trim();
            this.groupInfo.commit = true;
            groupInfo.groupType = Checkgroup_Type();
            this.Close();
        }

        private void btnCancel_Click(object sender, EventArgs e)
        {
            groupInfo.commit = false;
            this.Close();
        }

        private void txtGroupname_TextChanged(object sender, EventArgs e)
        {
            this.txtprewin.Text = txtGroupname.Text.Trim();

            if (txtGroupname.Text.Trim() != "")
            {
                btnOk.Enabled = true;
            }
            else
            {
                btnOk.Enabled = false;
            }
        }

        private void txtprewin_TextChanged(object sender, EventArgs e)
        {
            if (txtGroupname.Text.Trim() != "" && txtprewin.Text.Trim() != "")
            {
                btnOk.Enabled = true;
            }
            else
            {
                btnOk.Enabled = false;
            }
        }

        private void rbtn_CheckedChanged(object sender, EventArgs e)
        {
            groupInfo.groupType = Checkgroup_Type();
        }

        #endregion

        #region Methods

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

        public class GroupInfo
        {
            #region Class Data
            public string GroupName = "";
            public string PreWindowsgroupname = "";
            //may add more attributes for group
            public bool commit = false;
            public string groupType = "";
            #endregion
        }
    }
}


