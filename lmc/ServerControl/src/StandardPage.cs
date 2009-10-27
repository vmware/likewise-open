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
using System.ComponentModel;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Windows.Forms;
using Likewise.LMC.LMConsoleUtils;
using Likewise.LMC.AuthUtils;
using Likewise.LMC.ServerControl.Properties;

namespace Likewise.LMC.ServerControl
{
    /// <summary>
    /// This class is mostly a template to be used by
    /// leaf Tab implementors as a base class. It combines
    /// a caption area, an icon and a couple of display areas.
    /// </summary>
    public partial class StandardPage : UserControl, IPlugInPage
    {
        #region Class data

        public enum ViewStyle
        {
            LARGE_ICONS,
            SMALL_ICONS,
            LIST_VIEW,
            DETAIL_VIEW
        }

        protected IPlugInContainer container = null;
        protected IPlugIn pi = null;
        private IContext context = null;
        protected LACTreeNode treeNode = null;
        protected string sHelpKeyword;
        protected string sLDAPPath = null;

        protected bool bLinuxOnly = false;

        protected ViewStyle currentViewStyle = ViewStyle.DETAIL_VIEW;
        protected bool currentViewStyleChanged = false;

        public static bool bshowActionPane = true;
        public static bool bEnableActionMenu = false;

        protected LWTreeView lmctreeview = null;

        protected bool validHandle = false;

        //For GPOE Editor
        public static bool bshowHeaderPane = true;         
        

        #endregion

        public LWTreeView LMCTree
        {
            get
            {
                return lmctreeview;
            }
        }

        protected IContext ctx
        {
            get
            {
                context = null;

                if (pi == null)
                    return context;

                if (pi != null)
                    context = pi.GetContext();

                if(context == null)
                {
                    if (pi.GetContextType() == IContextType.DbConninfo)
                    {
                        pi.SetContext(new DbConnInfo());
                    }
                    else if (pi.GetContextType() == IContextType.Hostinfo)
                    {
                        pi.SetContext(new Hostinfo());
                    }
                    context = pi.GetContext();
                }
                return context;
            }
            set
            {
                context = value;
            }
        }

        #region Constructor
        /// <summary>
        /// Default constructor
        /// </summary>
        public StandardPage()
        {
            InitializeComponent();

            this.HandleCreated += new System.EventHandler(this.handler_handleCreated);
            this.HandleDestroyed += new System.EventHandler(this.handler_handleDestroyed);

            Icon ic = new Icon(Resources.GenericServer, 48, 48);
            this.picture.Image = ic.ToBitmap();

#if !QUARTZ
            this.pbHelp.Image = (new Icon(SystemIcons.Question, 16, 16)).ToBitmap();
#else
            this.pbHelp.Visible = false;
            this.lnkHelp.Visible = false;
#endif
            
            ShowActionPane(bshowActionPane);
            ShowHeaderPane(bshowHeaderPane);

            SetStyle(ControlStyles.AllPaintingInWmPaint | ControlStyles.UserPaint, true);
        }
        #endregion

        #region Property Accessors

        [Category("Behavior")]
        [Browsable(true)]
        public string HelpKeyword
        {
            get
            {
                return sHelpKeyword;
            }
            set
            {
                sHelpKeyword = value;
            }
        }

        [Category("Behavior")]
        [Browsable(true)]
        public bool LinuxOnly
        {
            get
            {
                return bLinuxOnly;
            }
            set
            {
                bLinuxOnly = value;
            }
        }

        public bool ShowInDisabledState
        {
            get
            {
                return false;
            }
        }


        #endregion

        #region Overrides

        /// <summary>
        /// Handles resizing by keeping the Help link on the right
        /// </summary>
        /// <param name="e"></param>
        protected override void OnResize(EventArgs e)
        {
            // do the standard thing
            base.OnResize(e);

        }

        #endregion

        #region Other methods

        /// <summary>
        /// This methods simplifies the display of pages that
        /// are to be shown in "disabled" form when running under windows
        /// </summary>
        public void DisableForWindows()
        {
            // move the caption up a tad!
            lblCaption.Location = new Point(lblCaption.Location.X, lblCaption.Location.Y - 5);

            // display the linux-only label
            lblSubCaption.Visible = true;

            // Carefully disable controls in the page

            // disable every top level control except for the header panel
            foreach(Control c in Controls)
                if (c!=pnlHeader)
                    c.Enabled = false;

            // disable everything in the panel header except for the stuff we care about!
            foreach(Control c in pnlHeader.Controls)
            {
                if (c!=lblSubCaption)
                    c.Enabled = false;
            }

        }

        public void SetViewStyle(ViewStyle view)
        {
            if (currentViewStyle != view)
            {
                currentViewStyle = view;
                currentViewStyleChanged = true;
                Refresh();
            }

        }
    

        #endregion

        #region PlugInPage Members

        protected void SetCaption(string value)
        {
            Logger.Log(String.Format("StandardPage.SetCaption({0})", value), Logger.manageLogLevel);

            if (lblCaption.Text.IndexOf("{0}") >= 0)
            {
                lblCaption.Text = string.Format(lblCaption.Text, value);
            }
            else
            {
                lblCaption.Text = value;
            }
        }

        public virtual void SetPlugInInfo(IPlugInContainer container, IPlugIn pi, LACTreeNode treeNode, LWTreeView lmcTreeview, CServerControl sc)
        {
            this.container = container;
            this.pi = pi;            
            this.treeNode = treeNode;
            this.lmctreeview = lmcTreeview;         

            if (treeNode != null)
            {
                treeNode.PluginPage = this;
            }

            // disable if necessary
            if (ShowInDisabledState)
            {
                DisableForWindows();
            }

        }

        public virtual void SetPlugInInfo(IPlugInContainer container, IPlugIn pi)
        {
            this.container = container;
            this.pi = pi;
            
            // disable if necessary
            if (ShowInDisabledState)
            {
                DisableForWindows();
            }

        }

        public virtual void OnClose()
        {
            if (treeNode != null)
                treeNode.PluginPage = null;
        }

        public virtual void Clear()
        {
        }

        public virtual void RefreshPluginPage()
        {
            if (treeNode.IsModified)
            {
                treeNode.sc.ShowControl(treeNode);
                treeNode.IsModified = false;
            }
        }

        public virtual void RefreshData()
        {
        }

        public virtual ContextMenu GetPlugInContextMenu()
        {
            return null;
        }

        public virtual ContextMenu GetListViewWhitespaceMenu()
        {
            ContextMenu cm = new ContextMenu();

            MenuItem m_item = new MenuItem("Refresh", new EventHandler(cm_OnRefresh));
            cm.MenuItems.Add(m_item);

            return cm;
        }

        public virtual void cm_OnRefresh(object sender, EventArgs e)
        {
            Refresh();
        }

        public virtual void PropertyPageProxy(object o)
        {
            
        }

        public void ShowActionPane(bool bShowActionPane)
        {
            if (pnlActions != null)
            {
                if (bShowActionPane && bEnableActionMenu)
                {
                    pnlActions.Show();
                }
                else
                {
                    pnlActions.Hide();
                }
            }
        }

        public void ShowHeaderPane(bool bShowheaderPane)
        {
            if (pnlHeader != null)
            {
                if (bShowheaderPane)
                {
                    pnlHeader.Show();
                }
                else
                {
                    pnlHeader.Hide();
                }
            }
        }

        public LACTreeNode TreeNode
        {
            get
            {
                return treeNode;
            }
        }

        public virtual void SetContext(IContext ctx)
        {
            if (ctx == null)
            {
                this.ctx = null;
            }
            else
            {
                //this.ctx = ctx.Clone();
            }
        }

        public IContext GetContext()
        {
            return ctx;
        }

        #endregion

        #region event handlers
        void handler_handleCreated(object sender, EventArgs e)
        {
            validHandle = true;
            //container.SetCursor(Cursors.Default);
        }

        void handler_handleDestroyed(object sender, EventArgs e)
        {
            validHandle = false;
            //container.SetCursor(Cursors.Default);
        }

        private void lnkHelp_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
#if !QUARTZ
            // Launch help
            string sPath = string.Format(@"{0}\documentation\lwidentity.chm", Application.StartupPath);
            Help.ShowHelp(this, sPath, sHelpKeyword);
#endif
        }

        private void pbHelp_Click(object sender, EventArgs e)
        {
            lnkHelp_LinkClicked(sender, null);
        }

        #endregion


    }

    /// <summary>
    /// Implements the linear gradient stripe below the caption
    /// area. This class should really be moved out to a separate
    /// file.
    /// </summary>
    public class Border : Panel
    {
        #region Overrides

        /// <summary>
        /// Handles the Resize event
        /// </summary>
        protected override void OnResize(EventArgs eventargs)
        {
            base.OnResize(eventargs);

            // invalidate the entire rectangle in order to
            // repaint the whole thing
            Invalidate();
        }

        /// <summary>
        /// Handles background painting
        /// </summary>
        protected override void OnPaintBackground(PaintEventArgs e)
        {
            // create and use a linear gradient brush to paint the
            // background

            // don't do nothin if rectangle has any zero dimension
            if (Size.Width == 0 || Size.Height == 0)
                return;

            Rectangle r = new Rectangle(0, 0, Size.Width, Size.Height);
            LinearGradientBrush br = new LinearGradientBrush(r, this.BackColor, SystemColors.Window, 0.0F);
            e.Graphics.FillRectangle(br, r);
        }

        #endregion
    }
}
