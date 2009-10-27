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
namespace Likewise.LMC.Plugins.ServiceManagerPlugin
{
    public partial class ServicePropertiesDlg : MPContainer
    {
        #region Class Data

        private ServiceManagerPlugin _plugin;
        private IPlugInContainer _container;

        #endregion

        #region Constructors

        public ServicePropertiesDlg(IPlugInContainer container, StandardPage parentPage, 
                                    ServiceManagerPlugin plugin, string serviceName)
                                    : base(container, parentPage)
        {
            InitializeComponent();

            this.Text = serviceName;
            _container = container;
            _plugin = plugin;
           
            InitializePages(serviceName);
        }

        #endregion

        #region Helper functions

        private void InitializePages(string serviceName)
        {
            MPPage page = null;

            page = new GeneralPropertyPage(this._container, this._plugin, serviceName);
            this.AddPage(page,
                           new MPMenuItem(page.PageID, "General", "General"),
                           MPMenu.POSITION_BEGINING);

            page = new ServiceRecoveryPage(this._container, this._plugin, serviceName);
            this.AddPage(page,
                           new MPMenuItem(page.PageID, "Recovery", "Recovery"),
                           MPMenu.POSITION_BEGINING);  

            foreach (IDirectoryPropertiesPage ppage in this.GetPages())
            {
                if (ppage != null)
                    ppage.SetData();
            }
        } 

        #endregion

        #region Event Handlers

        protected override bool Apply(EditDialog.EditDialogAction actionCause)
        {
            base.Apply(actionCause);

            foreach (IDirectoryPropertiesPage ppage in this.GetPages())
            {
                if (ppage != null && ppage is ServiceRecoveryPage)
                {
                    ServiceRecoveryPage recoveryPage = (ServiceRecoveryPage)ppage;
                    if (recoveryPage != null)
                    {
                        if (!recoveryPage.OnApply())
                            return false;
                    }
                }
            }

            return true;
        }

        private void btnOK_Click(object sender, EventArgs e)
        {
            this.Close();
        }
        private void btnCancel_Click(object sender, EventArgs e)
        {
            this.Close();
        }

        #endregion
    }
}