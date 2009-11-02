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
using Likewise.LMC.Utilities;

namespace Likewise.LMC.ServerControl
{
    public partial class LMCCustomView : MPPage
    {
        #region Class data

        public static bool bDefaults = true;

        #endregion

        #region Constructor

        public LMCCustomView()
        {
            InitializeComponent();
            bDefaults = true;
        }
        #endregion

        #region Property Accessors

        // These are all straightforward get/set pairs

        public bool ConsoleTree
        {
            get
            {
                return this.cbConsoleTree.Checked;
            }
            set
            {
                this.cbConsoleTree.Checked = value;
            }
        }

        public bool StandardMenus
        {
            get
            {
                return this.cbStandardMenus.Checked;
            }
            set
            {
                this.cbStandardMenus.Checked = value;
            }
        }

        public bool StandardToolbar
        {
            get
            {
                return this.cbToolBar.Checked;
            }
            set
            {
                this.cbToolBar.Checked = value;
            }
        }

        public bool StatusBar
        {
            get
            {
                return this.cbStatusbar.Checked;
            }
            set
            {
                this.cbStatusbar.Checked = value;
            }
        }      

        public bool TaskpadNavigationpads
        {
            get
            {
                return this.cbTaskPad.Checked;
            }
            set
            {
                this.cbTaskPad.Checked = value;
            }
        }

        public bool ActionPane
        {
            get
            {
                return this.cbActionPane.Checked;
            }
            set
            {
                this.cbActionPane.Checked = value;
            }
        }
        #endregion

        private void SettingsChanged(object sender, EventArgs e)
        { 
            bDefaults = false;
        }       
    }
}
