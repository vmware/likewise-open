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
using Likewise.LMC.ServerControl;
using Likewise.LMC.LDAP.Interop;
using Likewise.LMC.LDAP;

namespace Likewise.LMC.Plugins.ADUCPlugin
{
    public partial class ADRenameUserDlg : Form
    {
        #region Class Data
        private ADUCDirectoryNode _dirnode;
        public string logonname = "";
        public RenameUserInfo renameUserInfo;
        #endregion

        #region Constructors

        public ADRenameUserDlg()
        {
            InitializeComponent();
        }

        public ADRenameUserDlg(ADUCDirectoryNode dirnode)
            : this()
        {
            this._dirnode = dirnode;

            this.renameUserInfo = new RenameUserInfo();

            int ret = -1;

            List<LdapEntry> ldapEntries = null;

            ret = _dirnode.LdapContext.ListChildEntriesSynchronous(
            _dirnode.DistinguishedName,
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

                    LdapValue[] attrValues = ldapNextEntry.GetAttributeValues(attr, _dirnode.LdapContext);

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

                    if (string.Compare(attr, "cn") == 0)
                    {
                        this.FullNametextbox.Text = sValue;
                        renameUserInfo.fullName = sValue;
                    }

                    if (string.Compare(attr, "displayName") == 0)
                    {
                        this.displaynametextBox.Text = sValue;
                        renameUserInfo.displayName = sValue;
                    }

                    if (string.Compare(attr, "givenName") == 0)
                    {
                        this.FnametextBox.Text = sValue;
                        renameUserInfo.fName = sValue;
                    }

                    if (string.Compare(attr, "initials") == 0)
                    {
                        this.InitialtextBox.Text = sValue;
                        renameUserInfo.initials = sValue;
                    }

                    if (string.Compare(attr, "sn") == 0)
                    {
                        this.LnametextBox.Text = sValue;
                        renameUserInfo.lName = sValue;
                    }

                    if (string.Compare(attr, "userPrincipalName") == 0)
                    {
                        string[] pre = sValue.Split('@');
                        this.logonNametextBox.Text = pre[0].Trim();
                        renameUserInfo.logonName = sValue;
                    }

                    if (string.Compare(attr, "sAMAccountName") == 0)
                    {
                        this.userlogonPretextBox.Text = sValue;
                        renameUserInfo.userPrelogonname = sValue;
                    }
                }
            }

            string[] prefixes = dirnode.LdapContext.DomainName.Split('.');
            string prefix = string.Concat(prefixes[0].ToUpper(), "\\");
            this.prelogontextBox.Text = prefix;

            this.domainNamecomboBox.Items.Add(dirnode.LdapContext.DomainName);
            this.domainNamecomboBox.SelectedIndex = 0;
        }

        #endregion

        #region Private Methods

        /// <summary>
        /// Sets the "Ok" button state
        /// </summary>
        private void setButtonState()
        {
            if (this.renameUserInfo.fullName != "" &&
            this.renameUserInfo.logonName != "" &&
            this.renameUserInfo.userPrelogonname != "")
            {
                this.btnOK.Enabled = true;
            }
            else
            {
                this.btnOK.Enabled = false;
            }
        }

        private void GetUserFullName()
        {
            this.renameUserInfo.displayName = "";

            if (this.renameUserInfo.fName != "")
            {
                this.renameUserInfo.displayName = this.renameUserInfo.displayName + this.renameUserInfo.fName;
            }

            if (this.renameUserInfo.initials != "")
            {
                if (this.renameUserInfo.displayName != "")
                {
                    this.renameUserInfo.displayName = this.renameUserInfo.displayName + " " + this.renameUserInfo.initials + ".";
                }
                else
                {
                    this.renameUserInfo.displayName = this.renameUserInfo.displayName + this.renameUserInfo.initials + ".";
                }
            }

            if (this.renameUserInfo.lName != "")
            {
                if (this.renameUserInfo.displayName != "")
                {
                    this.renameUserInfo.displayName =
                        this.renameUserInfo.displayName + " " +
                        this.renameUserInfo.lName;
                }
                else
                {
                    this.renameUserInfo.displayName = this.renameUserInfo.displayName + this.renameUserInfo.lName;
                }
            }
        }

        #endregion


        #region Event Handlers

        private void FnametextBox_TextChanged(object sender, EventArgs e)
        {
            this.renameUserInfo.fName = this.FnametextBox.Text.Trim();

            GetUserFullName();

            if (this.renameUserInfo.displayName != "")
            {
                this.displaynametextBox.Text = this.renameUserInfo.displayName;
            }
            setButtonState();
        }

        private void InitialtextBox_TextChanged(object sender, EventArgs e)
        {
            this.renameUserInfo.initials = this.InitialtextBox.Text.Trim();

            GetUserFullName();

            if (this.renameUserInfo.displayName != "")
            {
                this.displaynametextBox.Text = this.renameUserInfo.displayName;
            }
            setButtonState();
        }

        private void LnametextBox_TextChanged(object sender, EventArgs e)
        {
            this.renameUserInfo.lName = this.LnametextBox.Text.Trim();

            GetUserFullName();

            if (this.renameUserInfo.displayName != "")
            {
                this.displaynametextBox.Text = this.renameUserInfo.displayName;
            }
            setButtonState();
        }

        private void fullnametextBox_TextChanged(object sender, EventArgs e)
        {
            setButtonState();
        }

        private void logonNametextBox_TextChanged(object sender, EventArgs e)
        {
            if (this.logonNametextBox.Text.Trim().Length <= 20)
            {
                this.userlogonPretextBox.Text = this.logonNametextBox.Text.Trim();
            }
        }

        private void userlogonPretextBox_TextChanged(object sender, EventArgs e)
        {
            this.renameUserInfo.userPrelogonname = this.userlogonPretextBox.Text.Trim();

            setButtonState();
        }

        private void displaynametextBox_TextChanged(object sender, EventArgs e)
        {
            this.renameUserInfo.displayName = this.displaynametextBox.Text.Trim();

            setButtonState();
        }

        private void btnOK_Click(object sender, EventArgs e)
        {
            int ret = -1;
            int i = 0;
            string lname = string.Empty;
            bool Isexists = false;

            lname = this.renameUserInfo.logonName;
            logonname = this.logonNametextBox.Text.Trim() + "@" + _dirnode.LdapContext.DomainName;

            if (string.Compare(lname, logonname) != 0)
            {
                while (i < logonname.Length)
                {
                    if (Char.IsControl(logonname[i]) || (logonname[i] == '/') || (logonname[i] == '\\') ||
                    (logonname[i] == '[') || (logonname[i] == ']') || (logonname[i] == ':') || (logonname[i] == ';') ||
                    (logonname[i] == '|') || (logonname[i] == '=') || (logonname[i] == ',') || (logonname[i] == '+') ||
                    (logonname[i] == '*') || (logonname[i] == '?') || (logonname[i] == '<') || (logonname[i] == '>') ||
                    (logonname[i] == '"'))
                    {
                        Isexists = true;
                        break;
                    }
                    else
                        i++;
                }
                if (Isexists)
                {
                    DialogResult dlg = MessageBox.Show(
                    this,
                    "The pre-Windows 2000 logon name " +
                    logonname +
                    " contains one or more of the following illegal characters " +
                    ":/\\n []:;|=,*+?<> \n If you continue LAC will replace these " +
                    "characters with underscore ('_').\n Do you want to continue?",
                    CommonResources.GetString("Caption_Console"),
                    MessageBoxButtons.YesNo,
                    MessageBoxIcon.Exclamation);
                    if (dlg == DialogResult.Yes)
                    {
                        i = 0;
                        while (i < logonname.Length)
                        {
                            if (Char.IsControl(logonname[i]) || (logonname[i] == '/') || (logonname[i] == '\\') ||
                            (logonname[i] == '[') || (logonname[i] == ']') || (logonname[i] == ':') || (logonname[i] == ';') ||
                            (logonname[i] == '|') || (logonname[i] == '=') || (logonname[i] == ',') || (logonname[i] == '+') ||
                            (logonname[i] == '*') || (logonname[i] == '?') || (logonname[i] == '<') || (logonname[i] == '>') ||
                            (logonname[i] == '"'))
                            {
                                logonname = logonname.Replace(logonname[i], '_');
                            }
                            else
                            {
                                i++;
                            }
                        }

                    }
                    else
                    {
                        return;
                    }
                }

                string filterquery = string.Format("(&(objectClass=user)(userPrincipalName={0}))", logonname);
                List<LdapEntry> ldapEntries = null;

                string[] attrList = new string[]
                {
                    "objectClass", null
                };

                ret = _dirnode.LdapContext.ListChildEntriesSynchronous(
                _dirnode.LdapContext.RootDN,
                Likewise.LMC.LDAP.Interop.LdapAPI.LDAPSCOPE.SUB_TREE,
                filterquery,
                attrList,
                false,
                out ldapEntries);
                if (ldapEntries != null && ldapEntries.Count != 0)
                {
                    string sMsg = "The user logon name you have choosen is already in use in this enterprise.\n" +
                    "Choose another logon name, and try again";
                    MessageBox.Show(this, sMsg, CommonResources.GetString("Caption_Console"),
                    MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                    return;
                }

                this.renameUserInfo.logonName = logonname;
            }

            if (string.Compare(this.renameUserInfo.fullName, this.FullNametextbox.Text.Trim()) != 0)
            {
                LACTreeNode parentnode = (LACTreeNode)_dirnode.Parent;

                ADUCDirectoryNode parentdirnode = parentnode as ADUCDirectoryNode;

                string filterqry = string.Format("(&(objectClass=user)(name={0}))", this.FullNametextbox.Text.Trim());
                List<LdapEntry> ldapEntry = null;

                string[] attrLists = new string[]
                {
                    "objectClass", null
                };

                ret = _dirnode.LdapContext.ListChildEntriesSynchronous(
                parentdirnode.DistinguishedName,
                Likewise.LMC.LDAP.Interop.LdapAPI.LDAPSCOPE.ONE_LEVEL,
                filterqry,
                attrLists,
                false,
                out ldapEntry);
                if (ldapEntry != null && ldapEntry.Count != 0)
                {
                    string sMsg = string.Format("LAC cannot complete the rename operation on {0} because:" +
                        "\nAn attempt was made to add an object to the directory with a name that is already in use." +
                        "\n\nName-related properties on this object might now be out of sync. Contact your system administrator.",
                        this.renameUserInfo.fullName);

                    MessageBox.Show(this, sMsg, CommonResources.GetString("Caption_Console"),
                    MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                    return;
                }

                this.renameUserInfo.fullName = this.FullNametextbox.Text.Trim();
            }

            this.renameUserInfo.commit = true;
            this.Close();
        }

        private void btkcancel_Click(object sender, EventArgs e)
        {
            this.renameUserInfo.commit = false;
            this.Close();
        }

        #endregion

    }

    public class RenameUserInfo
    {
        #region Class Data

        public string fName = string.Empty;
        public string lName = string.Empty;
        public string initials = string.Empty;
        public string fullName = string.Empty;
        public string displayName = string.Empty;
        public string logonName = string.Empty;
        public string userPrelogonname = string.Empty;
        public bool commit = false;

        #endregion
    }
}