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
using Likewise.LMC.LDAP;

namespace Likewise.LMC
{
    public partial class ObjectSelectPage : WizardPage
    {

        #region Class Data

        public static string PAGENAME = "ObjectSelectPage";
        private IPlugIn plugin = null;
        private IPlugInContainer container = null;
        private ObjectSelectDlg objectSelectDlg = null;
        DirectoryContext dirContext = null;

        #endregion

        #region Constructors

        public ObjectSelectPage(WizardDialog wizardDialog, IPlugIn plugin, IPlugInContainer container)
        {
            InitializeComponent();
            this.objectSelectDlg = (ObjectSelectDlg)wizardDialog;
            this.plugin = plugin;
            this.container = container;
        }

        #endregion

        #region Override Methods


        public override string OnWizardBack()
        {
            //return DomainConnectPage.PAGENAME;
            return "";
        }


        public override bool OnWizardFinish()
        {
            objectSelectDlg.dirContext = this.dirContext;
            return base.OnWizardFinish();
        }

        public override bool OnWizardCancel()
        {
            objectSelectDlg.displayName = objectSelectDlg.distinguishedName = null;
            return base.OnWizardCancel();
        }

        /// <summary>
        /// When activating the control on a wizard dialog loads the specific data.
        /// </summary>
        /// <returns></returns>
        public override bool OnSetActive()
        {
            Wizard.enableButton(WizardDialog.WizardButton.Cancel);
            Wizard.disableButton(WizardDialog.WizardButton.Back);
            Wizard.showButton(WizardDialog.WizardButton.Finish);
            Wizard.disableButton(WizardDialog.WizardButton.Finish);
            Wizard.hideButton(WizardDialog.WizardButton.Next);
            Wizard.disableButton(WizardDialog.WizardButton.Next);
            Wizard.disableButton(WizardDialog.WizardButton.Start);
            Wizard.hideButton(WizardDialog.WizardButton.Start);

            return true;
        }

        #endregion

        #region Events

        private void btnBrowse_Click(object sender, EventArgs e)
        {
            LACTreeNode PluginNode = plugin.GetPlugInNode();
            LACTreeNode rootDNnode = PluginNode.Tag as LACTreeNode;
            dirContext = rootDNnode.Tag as DirectoryContext;

            ObjectsListPage objectsPage = new ObjectsListPage();
            objectsPage.SetData(dirContext, plugin, container);
            if (objectsPage.ShowDialog(this) == DialogResult.OK)
            {
                lblGPO.Text = objectsPage.GPODisplayName;
                Wizard.enableButton(WizardDialog.WizardButton.Finish);
                objectSelectDlg.displayName = objectsPage.GPODisplayName;
                objectSelectDlg.distinguishedName = objectsPage.DistinguishedName;
            }
        }

        #endregion
    }
}
