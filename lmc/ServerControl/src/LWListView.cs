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
using System.Drawing;
using System.Windows.Forms;
using Likewise.LMC.LMConsoleUtils;

namespace Likewise.LMC.ServerControl
{
    public class LWListView : ListView
    {
        #region class data

        public Font originalFont = null;

        public FontStyle originalFontStyle = FontStyle.Regular;

        #endregion


        #region Constructors

        public LWListView() : base()
        {
            this.originalFont = (Font) this.Font.Clone();
            this.originalFontStyle = this.originalFont.Style;

            this.Scrollable = Configurations.useListScrolling;

            if (Configurations.resetListViewFonts) {
                this.SelectedIndexChanged += new EventHandler(LWListView_SelectedIndexChanged);
            }
        }
        

        #endregion

        #region event handlers
        private void LWListView_SelectedIndexChanged(object sender, EventArgs e)
        {
            foreach (ListViewItem item in Items)
            {
                item.Font = new Font(this.originalFont, this.originalFontStyle);
            }
        }

        #endregion

    }

}
