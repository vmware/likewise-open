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
using System.Threading;
using Likewise.LMC.Utilities;

namespace Likewise.LMC.ServerControl
{
    public class LWTreeView : TreeView
    {
        #region class data

        public Font originalFont = null;

        public FontStyle originalFontStyle = FontStyle.Regular;

        #endregion

        #region Constructors

        public LWTreeView()
            : base()
        {
            this.originalFont = (Font)this.Font.Clone();
            this.originalFontStyle = this.originalFont.Style;

            this.Scrollable = Configurations.useListScrolling;

            if (Configurations.resetLWTreeViewFonts)  {
                this.MouseDown += new MouseEventHandler(LWTreeView_MouseDown);
            }

            if (Configurations.provideDummyMouseHandlers)  {
                this.MouseCaptureChanged += new EventHandler(LWTreeView_MouseCaptureChanged);
                this.MouseClick += new MouseEventHandler(LWTreeView_MouseClick);
                this.MouseDoubleClick += new MouseEventHandler(LWTreeView_MouseDoubleClick);
                this.MouseEnter += new EventHandler(LWTreeView_MouseEnter);
                this.MouseHover += new EventHandler(LWTreeView_MouseHover);
                this.MouseLeave += new EventHandler(LWTreeView_MouseMove);
                this.MouseMove += new MouseEventHandler(LWTreeView_MouseMove);
                this.MouseUp += new MouseEventHandler(LWTreeView_MouseUp);
                this.MouseWheel += new MouseEventHandler(LWTreeView_MouseWheel);

                this.NodeMouseClick += new TreeNodeMouseClickEventHandler(LWTreeView_NodeMouseClick);
                this.NodeMouseDoubleClick += new TreeNodeMouseClickEventHandler(LWTreeView_NodeMouseDoubleClick);
                this.NodeMouseHover += new TreeNodeMouseHoverEventHandler(LWTreeView_NodeMouseHover);
            }
        }

        #endregion

        #region event handlers

        private void LWTreeView_MouseDown(object sender, MouseEventArgs e)
        {
            Logger.Log("LWTreeView.TreeView_MouseDown", Logger.manageLogLevel);

            Refresh();

            foreach (TreeNode node in Nodes)
            {
                node.NodeFont = new Font(this.originalFont, this.originalFontStyle);
            }

        }

        private void LWTreeView_MouseCaptureChanged(object sender, EventArgs e)
        {
            Logger.Log("LWTreeView.TreeView_MouseCaptureChanged", Logger.manageLogLevel);
        }

        private void LWTreeView_MouseClick(object sender, EventArgs e)
        {
            Logger.Log("LWTreeView.TreeView_MouseClick", Logger.manageLogLevel);
        }
        private void LWTreeView_MouseDoubleClick(object sender, EventArgs e)
        {
            Logger.Log("LWTreeView.TreeView_MouseDoubleClick", Logger.manageLogLevel);
        }

        private void LWTreeView_MouseEnter(object sender, EventArgs e)
        {
            Logger.Log("LWTreeView.TreeView_MouseEnter", Logger.manageLogLevel);
        }

        private void LWTreeView_MouseHover(object sender, EventArgs e)
        {
            Logger.Log("LWTreeView.TreeView_MouseHover", Logger.manageLogLevel);
        }

        private void LWTreeView_MouseLeave(object sender, EventArgs e)
        {
            Logger.Log("LWTreeView.TreeView_MouseLeave", Logger.manageLogLevel);
        }

        private void LWTreeView_MouseMove(object sender, EventArgs e)
        {
            Logger.Log("LWTreeView.TreeView_MouseMove", Logger.manageLogLevel);
        }

        private void LWTreeView_MouseUp(object sender, EventArgs e)
        {
            Logger.Log("LWTreeView.TreeView_MouseUp", Logger.manageLogLevel);
        }
        private void LWTreeView_MouseWheel(object sender, EventArgs e)
        {
            Logger.Log("LWTreeView.TreeView_MouseWheel", Logger.manageLogLevel);
        }

        private void LWTreeView_NodeMouseClick(object sender, EventArgs e)
        {
            Logger.Log("LWTreeView.TreeView_NodeMouseClick", Logger.manageLogLevel);
        }

        private void LWTreeView_NodeMouseDoubleClick(object sender, EventArgs e)
        {
            Logger.Log("LWTreeView.TreeView_NodeMouseDoubleClick", Logger.manageLogLevel);
        }

        private void LWTreeView_NodeMouseHover(object sender, EventArgs e)
        {
            Logger.Log("LWTreeView.TreeView_NodeMouseHover", Logger.manageLogLevel);
        }

        #endregion
    }

}
