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
using System.Windows.Forms;
using System.Collections;
using Likewise.LMC.LMConsoleUtils;
using Likewise.LMC.ServerControl;
using Likewise.LMC.NETAPI;
using Likewise.LMC.AuthUtils;
using Likewise.LMC.Plugins.FileBrowser;

namespace Likewise.LMC.Plugins.FileBrowser
{
    public partial class SharePropertiesDlg : MPContainer
    {
        #region Class Data

        Hostinfo _hn = null;
        FileBrowserIPlugIn _plugin;
        IPlugInContainer _container;

        public static bool Applied = false;

        #endregion

        #region Constructors
        public SharePropertiesDlg(IPlugInContainer container, StandardPage parentPage, FileBrowserIPlugIn plugin, Hostinfo hn)
            : base(container, parentPage)
        {
            InitializeComponent();

            this.Text = "{0} Properties";
            this._hn = hn;
            _plugin = plugin;
            _container = container;
            InitializePages();
        }
        #endregion

        #region Initialization Methods

        /// <summary>
        /// Method to initailize the tab pages for the property sheet
        /// </summary>
        private void InitializePages()
        {
            MPPage page = null;

            page = new ShareGeneralEditPage();
            this.AddPage(page,
                     new MPMenuItem(page.PageID, "General", "General"),
                     MPMenu.POSITION_END);
        }

        /// <summary>
        /// Method to load data to the tab pages while loading
        /// Gets all the tab pages that are of type MPage and gets calls the SetData()
        /// Queries the ldap message to the selected node
        /// </summary>
        /// <param name="ce"></param>
        /// <param name="servername"></param>
        /// <param name="computer"></param>
        /// <param name="dirnode"></param>
        /// <param name="ldapSchemaCache"></param>
        public void SetData(CredentialEntry ce, string sharename, object shareInfo)
        {
            Applied = false;

            this.Text = String.Format(Text, sharename);

            ICollection pages = this.GetPages();

            foreach (MPPage page in pages)
            {
                if (page != null)
                {
                    IDirectoryPropertiesPage ipp = page as IDirectoryPropertiesPage;
                    if (ipp != null)
                    {
                        ipp.SetData(ce, sharename, shareInfo);
                    }
                }
            }
        }
        #endregion

        #region Event Handlers

        /// <summary>
        /// Method to call the Apply functionality for each of tab pages of type MPage.
        /// </summary>
        /// <param name="actionCause"></param>
        /// <returns></returns>
        protected override bool Apply(EditDialogAction actionCause)
        {
            if (Applied && !bDataWasChanged)
            {
                return true;
            }

            foreach (MPPage page in this.GetPages())
            {
                if (page != null)
                {
                    IDirectoryPropertiesPage ipp = page as IDirectoryPropertiesPage;
                    if (page.PageID.Trim().Equals("ShareGeneralEditPage"))
                    {
                        ShareGeneralEditPage _editPage = (ShareGeneralEditPage)page;
                        if (!_editPage.OnApply())
                        {
                            return false;
                        }
                    }
                }
            }

            Applied = true;
            return true;
        }

        private void btnCancel_Click(object sender, EventArgs e)
        {
            base.OnCancel(sender, e);

            this.Close();
        }

        private void btnOK_Click(object sender, EventArgs e)
        {
            if (Applied)
            {
                this.Close();
            }
        }

        #endregion
    }
}

