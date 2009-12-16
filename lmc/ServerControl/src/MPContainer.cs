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
using System.Collections;
using System.Drawing;
using System.Windows.Forms;
using System.Collections.Generic;
using Likewise.LMC.ServerControl.Properties;
using Likewise.LMC.Utilities;
using System.Threading;

namespace Likewise.LMC.ServerControl
{
    public partial class MPContainer : EditDialog
    {
        #region Class data
        private const String errorImageKey = "MPContainerErrorImage";
        private Hashtable htPages = new Hashtable();
        private Hashtable htTabPages = new Hashtable();
        // currently shown control
        private MPPage controlShown = null;
        private Size largestPageSize = new Size(0, 0);
        private bool bPagesSizable = true;
        private Size origClientSize;
        private Size origTabControlSize;

        //in order to retrieve information from inside tabs
        private bool _attrModified = false;
        private string _newDn = null;
        private bool _foundGid = false;
        private bool _foundUid = false;

        public byte[] objectSidBytes;
        public byte[] objectGUIDBytes;
        private List<object> objectsList = new List<object>();

        public bool foundAssociatedCell = false;
        public Thread threadMain;
        public List<Thread> ThreadsInner;
        public List<object> ObjectCounts
        {
            get
            {
                return objectsList;
            }
            set
            {
                objectsList = value;
            }
        }

        public bool AttrModified
        {
            get
            {
                return _attrModified;
            }
            set
            {
                _attrModified = value;
            }
        }

        public string newDn
        {
            get
            {
                return _newDn;
            }
            set
            {
                _newDn = value;
            }
        }

        public bool foundGid
        {
            get
            {
                return _foundGid;
            }
            set
            {
                _foundGid = value;
            }
        }

        public bool foundUid
        {
            get
            {
                return _foundUid;
            }
            set
            {
                _foundUid = value;
            }
        }

        #endregion

        #region Constructor

        public MPContainer() :
            base()
        {
            Initialize();
        }

        public MPContainer(IPlugInContainer container, StandardPage parentPage)
            : base(container, parentPage)
        {
            Initialize();
            this.Icon = Resources.likewise;
        }

        #endregion

        #region Class data access

        /// <summary>
        /// Gets the currently shown control
        /// </summary>
        public MPPage ControlShown
        {
            get
            {
                return this.controlShown;
            }
        }

        /// <summary>
        /// Set this to true to automatically make all pages
        /// the same size as the largest page.
        /// </summary>
        public bool PagesSizable
        {
            set
            {
                this.bPagesSizable = value;
            }
            get
            {
                return this.bPagesSizable;
            }
        }

        /// <summary>
        /// Set the selected tab page pageID
        /// </summary>
        public string SelectedTabPageID
        {
            get
            {
                return this.tabControl.SelectedTab.Name;
            }
        }

        #endregion

        #region Public interface

        /// <summary>
        /// Returns a collection of all MPPage objects in this container
        /// </summary>
        /// <returns>ICollection of MPPage objects</returns>
        public ICollection GetPages()
        {
            ICollection pages = htPages.Values;
            return pages;
        }

        /// <summary>
        /// Return the page identified by the given pageID
        /// </summary>
        /// <param name="pageID">page id</param>
        /// <returns>MPPage object or null if not found</returns>
        public MPPage GetPage(String pageID)
        {
            return (MPPage)htPages[pageID];
        }

        /// <summary>
        /// Programatic page selection.
        /// </summary>
        /// <param name="pageID">ID of the page to select</param>
        public void ShowPage(String pageID)
        {
            TabPage tabPage = (TabPage)htTabPages[pageID];
            if ( tabPage != null )
                tabControl.SelectedTab = tabPage;
        }

        /// <summary>
        /// Adds a new page to the container.
        /// </summary>
        /// <param name="page">The page </param>
        /// <param name="menuItem"></param>
        /// <param name="position">zero based position of the page</param>
        public void AddPage(MPPage page, MPMenuItem menuItem, int position)
        {
            htPages.Add(menuItem.pageID, page);

            if (page.Size.Width > largestPageSize.Width)
                largestPageSize.Width = page.Size.Width;
            if (page.Size.Height > largestPageSize.Height)
                largestPageSize.Height = page.Size.Height;

            TabPage tabPage = new TabPage(menuItem.menuText);
            tabPage.Name = page.PageID;
            tabPage.UseVisualStyleBackColor = true;
            tabPage.Size = page.Size;
            if (bPagesSizable)
                page.Dock = DockStyle.Fill;
            tabPage.Controls.Add(page);
            if (tabControl.ImageList != null)
            {
                tabPage.ImageIndex =
                    tabControl.ImageList.Images.IndexOfKey(page.PageID);
            }
            page.ParentContainer = this;
            page.IPlugInContainer = container;
            page.position = position;

            // Menu positioning:
            // The assumption is that the caller will either care about
            // positioning and give each page a unique position or
            // that he will use POSITION_END or POSITION_BEGINING for all pages.

            if (page.position == MPMenu.POSITION_END || tabControl.TabPages.Count == 0)
            {
                tabControl.TabPages.Add(tabPage);
            }
            else
            {
                TabPage[] arrPages = new TabPage[tabControl.TabPages.Count + 1];
                int i;
                int j = 0;
                for (i = 0; i < tabControl.TabPages.Count; i++)
                {
                    MPPage curPage = GetPage(tabControl.TabPages[i].Name);
                    if (i == j && page.position < curPage.position)
                        arrPages[j++] = tabPage;
                    arrPages[j++] = tabControl.TabPages[i];
                }

                // insert at the end
                if (i == j)
                    arrPages[j++] = tabPage;

                // the tabControl.TabPages.Insert( int index, TabPage page ) does not work for some reason
                tabControl.TabPages.Clear();
                tabControl.TabPages.AddRange(arrPages);
            }

            // we need to remember the index of the tab page so we can get it
            // later by page ID
            htTabPages.Add(page.PageID, tabPage);
        }

        /// <summary>
        /// Sets or clears the error flags next to each menu item based on the errors
        /// on each page.
        /// </summary>
        public override void RefreshErrorState()
        {
            bool bErrors = false;

            foreach (MPPage page in htPages.Values)
            {
                bool bPageError = page.HasError();
                bErrors = bErrors || bPageError;

                // set or clear the page error image
                TabPage tabPage = (TabPage)htTabPages[page.PageID];
                if (tabPage != null && tabControl.ImageList != null)
                {
                    String tabImageKey = bPageError ? errorImageKey : page.PageID;

                    tabPage.ImageIndex =
                        tabControl.ImageList.Images.IndexOfKey(tabImageKey);
                }
            }

            base.HasErrors = bErrors;
        }

        /// <summary>
        /// Selects the first tab that has an error.
        /// If the currently selected tab has an error this function will do nothing.
        /// </summary>
        public void ShowFirstPageWithError()
        {
            TabPage selectedTab = tabControl.SelectedTab;
            if (selectedTab != null && ((MPPage)htPages[selectedTab.Name]).HasError())
                return;

            foreach (MPPage page in htPages.Values)
            {
                if ( page.HasError() )
                {
                    selectedTab = ((TabPage)htTabPages[page.PageID]);
                    if (selectedTab != null)
                        //                        tabControl.SelectTab(page.PageID);
                        tabControl.SelectedTab = selectedTab;
                    return;
                }
            }
        }

        #endregion

        #region Private methods

        private void Initialize()
        {
            InitializeComponent();

            // This is to get around the problem of the inherited dialog changing the
            // client size in the Visual designer and the ancored and filled panels not getting
            // resized ???

            origClientSize = this.ClientSize;
            origTabControlSize = this.tabControl.Size;

            // create image list to show the error on the tab
            if (tabControl.ImageList == null)
            {
                tabControl.ImageList = new ImageList();
                tabControl.ImageList.ColorDepth = ColorDepth.Depth24Bit;
                tabControl.ImageList.Images.Add(errorImageKey, errorProvider.Icon);
            }
        }

        /// <summary>
        /// Set the small image list displayed next to each menu item.
        /// Each image in the list must have the same name as the pageID of the page
        /// to which it points.
        /// </summary>
        /// <param name="smallImageList">small image list</param>
        protected void SetMenuSmallImageList(ImageList smallImageList)
        {
            smallImageList.Images.Add(errorImageKey, errorProvider.Icon);
            tabControl.ImageList = smallImageList;
        }

        /// <summary>
        /// Resize the dialog to fit the largest page.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void MPContainer_Load(object sender, EventArgs e)
        {
            // restore the original size
            if (!this.ClientSize.Equals(origClientSize))
            {
                this.ClientSize = origClientSize;
                this.tabControl.Size = origTabControlSize;
            }

            // the dialog must be poplated with data by now so we can start
            // watching for any changes by adding ValueChanged handlers to all controls
            foreach (MPPage page in htPages.Values)
            {
                // make all pages the size of the biggest page if they are sizable
                if (bPagesSizable)
                    page.Size = largestPageSize;

                SetAllValueChangedHandlers(page);
            }


            // change the size of the container to fit the largest of the pages
            Size currentPageSize = tabControl.DisplayRectangle.Size;
            this.ClientSize = new Size(
                this.ClientSize.Width + largestPageSize.Width - currentPageSize.Width,
                this.ClientSize.Height + largestPageSize.Height - currentPageSize.Height);
            this.MinimumSize = this.Size;

            //Margin to use here
            int lwMarginW = 40;
            int lwMarginH = 85;

            int inner_w = tabControl.Size.Width;
            int inner_h = tabControl.Size.Height;

            Size = new Size(inner_w + lwMarginW, inner_h + lwMarginH);

            tabControl.Size = new Size(inner_w, inner_h);

            BackColor = tabControl.BackColor;
            //END HACK



            SetContainerState();
        }

        public virtual void tabControl_SelectedIndexChanged(object sender, EventArgs e)
        {

        }
        protected override void OnCancel(object sender, EventArgs e)
        {
            base.OnCancel(sender, e);
            StopThreads();
        }

        public void StopThreads()
        {
            if (ThreadsInner != null && ThreadsInner.Count != 0)
            {
                foreach (Thread inner in ThreadsInner)
                {
                    try
                    {
                        if (inner != null && inner.ThreadState == ThreadState.Running)
                        {
                            inner.Interrupt();
                        }
                    }
                    catch (Exception ex)
                    {
                        Logger.LogException("MPContainer.StopThreads", ex);
                    }
                }
            }

            try
            {
                if (threadMain != null && threadMain.ThreadState == ThreadState.Running)
                {
                    threadMain.Interrupt();
                }
            }
            catch(Exception ex)
            {
                Logger.LogException("MPContainer.StopThreads", ex);
            }
        }

        #endregion
    }
}