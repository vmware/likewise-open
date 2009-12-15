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
using Likewise.LMC.LDAP;
using System.Text.RegularExpressions;

namespace Likewise.LMC.Plugins.ADUCPlugin
{
    public partial class UserAddWelcomePage : WizardPage
    {
        #region Class Data
        private ADUserAddDlg _userAddDlg;
        private ADUCDirectoryNode _dirnode;
        public string logonname = "";
        #endregion

        #region Constructors
        public UserAddWelcomePage()
        {
            InitializeComponent();
        }

        public UserAddWelcomePage(ADUserAddDlg userAddDlg, ADUCDirectoryNode dirnode)
            : this()
        {

            this._userAddDlg = userAddDlg;
            this._dirnode = dirnode;
            string[] prefixes = this._userAddDlg.userInfo.domainName.Split('.');
            string prefix = string.Concat(prefixes[0].ToUpper(), "\\");
            this.prelogontextBox.Text = prefix;
            this.domainNamecomboBox.Text = this._userAddDlg.userInfo.domainName;
            this.txtcreatein.Text = "Create in: " + this._userAddDlg.userInfo.OUPath;
        }
        #endregion

        #region Override Methods
        public override string OnWizardNext()
        {
            int ret = -1;
            int i = 0;
            string prename = "";
            bool Isexists = false;

            prename = this._userAddDlg.userInfo.userPrelogonname;
            logonname = this.logonNametextBox.Text.Trim() + "@" + this._userAddDlg.userInfo.domainName;

            while (i < prename.Length)
            {
                if (Char.IsControl(prename[i]) || (prename[i] == '/') || (prename[i] == '\\') ||
                (prename[i] == '[') || (prename[i] == ']') || (prename[i] == ':') || (prename[i] == ';') ||
                (prename[i] == '|') || (prename[i] == '=') || (prename[i] == ',') || (prename[i] == '+') ||
                (prename[i] == '*') || (prename[i] == '?') || (prename[i] == '<') || (prename[i] == '>') ||
                (prename[i] == '"'))
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
                prename +
                " contains one or more of the following illegal characters " +
                ":/\\n []:;|=,*+?<> \n If you continue LAC will replace these " +
                "characters with underscore ('_').\n Do you want to continue?",
                CommonResources.GetString("Caption_Console"),
                MessageBoxButtons.YesNo,
                MessageBoxIcon.Exclamation);
                if (dlg == DialogResult.Yes)
                {
                    i = 0;
                    while (i < prename.Length)
                    {
                        if (Char.IsControl(prename[i]) || (prename[i] == '/') || (prename[i] == '\\') ||
                        (prename[i] == '[') || (prename[i] == ']') || (prename[i] == ':') || (prename[i] == ';') ||
                        (prename[i] == '|') || (prename[i] == '=') || (prename[i] == ',') || (prename[i] == '+') ||
                        (prename[i] == '*') || (prename[i] == '?') || (prename[i] == '<') || (prename[i] == '>') ||
                        (prename[i] == '"'))
                        {
                            prename = prename.Replace(prename[i], '_');
                        }
                        else
                        {
                            i++;
                        }
                    }

                }
                else
                {
                    return WizardDialog.NoPageChange;
                }
            }
            this._userAddDlg.userInfo.logonName = logonname;
            this._userAddDlg.userInfo.userPrelogonname = prename;
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
                return WizardDialog.NoPageChange;
            }

            return base.OnWizardNext();
        }

        public override bool OnSetActive()
        {
            if (this._userAddDlg.userInfo.domainName.EndsWith(@"/"))
            {
                this._userAddDlg.userInfo.domainName =
                    this._userAddDlg.userInfo.domainName.Substring(0, this._userAddDlg.userInfo.domainName.Length - 1);
            }

            domainNamecomboBox.Items.Add(string.Concat("@", _userAddDlg.userInfo.domainName));
            this.domainNamecomboBox.SelectedIndex = 0;
            Wizard.enableButton(WizardDialog.WizardButton.Cancel);
            Wizard.disableButton(WizardDialog.WizardButton.Back);
            Wizard.showButton(WizardDialog.WizardButton.Next);
            Wizard.disableButton(WizardDialog.WizardButton.Start);
            Wizard.hideButton(WizardDialog.WizardButton.Start);

            if (this._userAddDlg.userInfo.fullName != "" &&
            this._userAddDlg.userInfo.logonName != "" &&
            this._userAddDlg.userInfo.userPrelogonname != "")
            {
                Wizard.enableButton(WizardDialog.WizardButton.Next);
            }
            else
            {
                Wizard.disableButton(WizardDialog.WizardButton.Next);
            }

            Wizard.disableButton(WizardDialog.WizardButton.Finish);
            Wizard.hideButton(WizardDialog.WizardButton.Finish);

            return true;
        }
        #endregion

        #region Methods

        /// <summary>
        /// Sets the "Next" button state
        /// </summary>
        private void setButtonState()
        {
            if (this._userAddDlg.userInfo.fullName != "" &&
            this._userAddDlg.userInfo.logonName != "" &&
            this._userAddDlg.userInfo.userPrelogonname != "")
            {
                Wizard.enableButton(WizardDialog.WizardButton.Next);
            }
            else
            {
                Wizard.disableButton(WizardDialog.WizardButton.Next);
            }
        }

        private void GetUserFullName()
        {
            this._userAddDlg.userInfo.fullName = "";

            if (this._userAddDlg.userInfo.fName != "")
            {
                this._userAddDlg.userInfo.fullName =
                    this._userAddDlg.userInfo.fullName +
                    this._userAddDlg.userInfo.fName;
            }

            if (this._userAddDlg.userInfo.initials != "")
            {
                if (this._userAddDlg.userInfo.fullName != "")
                {
                    this._userAddDlg.userInfo.fullName =
                        this._userAddDlg.userInfo.fullName + " " +
                        this._userAddDlg.userInfo.initials + ".";
                }
                else
                {
                    this._userAddDlg.userInfo.fullName = this._userAddDlg.userInfo.fullName + this._userAddDlg.userInfo.initials + ".";
                }
            }

            if (this._userAddDlg.userInfo.lName != "")
            {
                if (this._userAddDlg.userInfo.fullName != "")
                {
                    this._userAddDlg.userInfo.fullName =
                        this._userAddDlg.userInfo.fullName + " " +
                        this._userAddDlg.userInfo.lName;
                }
                else
                {
                    this._userAddDlg.userInfo.fullName = this._userAddDlg.userInfo.fullName + this._userAddDlg.userInfo.lName;
                }
            }
        }

        #endregion

        #region Events
        private void FnametextBox_TextChanged(object sender, EventArgs e)
        {
            this._userAddDlg.userInfo.fName = this.FnametextBox.Text.Trim();

            GetUserFullName();

            this.fullnametextBox.Text = this._userAddDlg.userInfo.fullName;
        }

        private void InitialtextBox_TextChanged(object sender, EventArgs e)
        {
            this._userAddDlg.userInfo.initials = this.InitialtextBox.Text.Trim();

            GetUserFullName();

            if (this._userAddDlg.userInfo.fullName != "")
            {
                this.fullnametextBox.Text = this._userAddDlg.userInfo.fullName;
            }
        }

        private void LnametextBox_TextChanged(object sender, EventArgs e)
        {
            this._userAddDlg.userInfo.lName = this.LnametextBox.Text.Trim();

            GetUserFullName();

            if (this._userAddDlg.userInfo.fullName != "")
            {
                this.fullnametextBox.Text = this._userAddDlg.userInfo.fullName;
            }
        }

        private void logonNametextBox_TextChanged(object sender, EventArgs e)
        {
            if (this.logonNametextBox.Text.Trim().Length <= 20)
            {
                this.userlogonPretextBox.Text = this.logonNametextBox.Text.Trim();
            }

            this._userAddDlg.userInfo.logonName = this.logonNametextBox.Text.Trim();

            setButtonState();
        }

        private void fullnametextBox_TextChanged(object sender, EventArgs e)
        {
            this._userAddDlg.userInfo.fullName = this.fullnametextBox.Text.Trim();

            setButtonState();
        }

        private void userlogonPretextBox_TextChanged(object sender, EventArgs e)
        {
            this._userAddDlg.userInfo.userPrelogonname = this.userlogonPretextBox.Text.Trim();

            setButtonState();
        }
        #endregion

    }

}

