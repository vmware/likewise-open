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
using Likewise.LMC.LMConsoleUtils;

namespace Likewise.LMC.ServerControl
{
    public partial class WizardDialog : Form
    {
        # region Public Constants
        /// <summary>
        /// Used to identify the various buttons that may appear within the wizard dialog.  
        /// </summary>
        [Flags]
        public enum WizardButton
        {
            Cancel = 0x00000001,
            Back = 0x00000002,
            Next = 0x00000004,
            Start = 0x00000008,
            Finish = 0x00000010
        }

        /// <summary>
        /// Used by a page to indicate to this wizard that the next page
        /// should be activated when either the Back or Next buttons are
        /// pressed.
        /// </summary>
        public const string NextPage = "";

        /// <summary>
        /// Used by a page to indicate to this wizard that the selected page
        /// should go back to the first wizard when either the Back, Finish or Next buttons are
        /// pressed.
        /// </summary>
        public static string FirstPage = "";

        /// <summary>
        /// Used by a page to indicate to this wizard that the selected page
        /// should remain active when either the Back or Next buttons are
        /// pressed.
        /// </summary>
        public const string NoPageChange = null;
        
        #endregion

        #region Class data
        private IPlugInContainer _IPlugInContainer = null;
        
        /// <summary>
        /// Array of wizard pages.
        /// </summary>
        private ArrayList _wizardPages = null;
        private int _curPageIndex = -1;
        private Stack pageHistory = new Stack();
        #endregion

        #region Constructor
        public WizardDialog()
        {
            InitializeComponent();
            _wizardPages = new ArrayList();
        }
        
        public WizardDialog(IPlugInContainer container)
        : this()
        {
            _IPlugInContainer = container;
        }
        #endregion

        #region accessor function
        public IPlugInContainer IPlugInContainer
        {
            get
            {
                return _IPlugInContainer;
            }
            set
            {
                _IPlugInContainer = value;
            }
        }
        #endregion

        protected void SetPlugInContainer(IPlugInContainer container)
        {
            _IPlugInContainer = container;
        }

        public void AddPage(WizardPage pg)
        {
            pg.Wizard = this;
            pg.Visible = false;
            pg.Dock = DockStyle.Fill;
            _wizardPages.Add(pg);
            panel_content.Controls.Add(pg);
        }

        /// <summary>
        /// Enables or disables the Back, Next, or Finish buttons in the
        /// wizard.
        /// </summary>
        /// <param name="flags">
        /// A set of flags that customize the function and appearance of the
        /// wizard buttons.  This parameter can be a combination of any
        /// value in the <c>DnsWizardButton</c> enumeration.
        /// </param>
        /// <remarks>
        /// Typically, you should call <c>SetWizardButtons</c> from
        /// <c>WizardPage.OnSetActive</c>.  You can display a Finish or a
        /// Next button at one time, but not both.
        /// </remarks>
        public void SetWizardButtons(WizardButton flags)
        {
            // Enable/disable and show/hide buttons appropriately
            m_buttonBack.Enabled =
                (flags & WizardButton.Back) == WizardButton.Back;
            m_buttonNext.Enabled =
                (flags & WizardButton.Next) == WizardButton.Next;
            m_buttonStart.Enabled =
              (flags & WizardButton.Start) == WizardButton.Start;
            m_buttonNext.Enabled =
                (flags & WizardButton.Finish) == WizardButton.Finish;
            
            SetAcceptButton();
        }

        public void disableButton(WizardButton flags)
        {
            if (WizardButton.Back == (flags & WizardButton.Back))
                m_buttonBack.Enabled = false;
            if (WizardButton.Next == (flags & WizardButton.Next))
                m_buttonNext.Enabled = false;
            if (WizardButton.Start == (flags & WizardButton.Start))
                m_buttonStart.Enabled = false;
            if (WizardButton.Finish == (flags & WizardButton.Finish))
                m_buttonFinish.Enabled = false;
            if (WizardButton.Cancel == (flags & WizardButton.Cancel))
                m_buttonCancel.Enabled = false;  
            SetAcceptButton();
        }

        public void enableButton(WizardButton flags)
        {
            if (WizardButton.Back == (flags & WizardButton.Back))
                m_buttonBack.Enabled = true;
            if (WizardButton.Next == (flags & WizardButton.Next))
                m_buttonNext.Enabled = true;
            if (WizardButton.Start == (flags & WizardButton.Start))
                m_buttonStart.Enabled = true;
            if (WizardButton.Finish == (flags & WizardButton.Finish))
                m_buttonFinish.Enabled = true;
            if (WizardButton.Cancel == (flags & WizardButton.Cancel))
                m_buttonCancel.Enabled = true;
            SetAcceptButton();
        }

        public void hideButton(WizardButton flags)
        {
            if (WizardButton.Back == (flags & WizardButton.Back))
                m_buttonBack.Hide();
            if (WizardButton.Next == (flags & WizardButton.Next))
                m_buttonNext.Hide();
            if (WizardButton.Start == (flags & WizardButton.Start))
                m_buttonStart.Hide();
            if (WizardButton.Finish == (flags & WizardButton.Finish))
                m_buttonFinish.Hide();
            if (WizardButton.Cancel == (flags & WizardButton.Cancel))
                m_buttonCancel.Hide();
            SetAcceptButton();
        }

        public void showButton(WizardButton flags)
        {
            if (WizardButton.Back == (flags & WizardButton.Back))
                m_buttonBack.Show();
            if (WizardButton.Next == (flags & WizardButton.Next))
                m_buttonNext.Show();
            if (WizardButton.Start == (flags & WizardButton.Start))
                m_buttonStart.Show();
            if (WizardButton.Finish == (flags & WizardButton.Finish))
                m_buttonFinish.Show();
            if (WizardButton.Cancel == (flags & WizardButton.Cancel))
                m_buttonCancel.Show();
            SetAcceptButton();
        }

        /// <summary>
        /// Retrieves the position of a wizard button
        /// </summary>
        /// <param name="flag">Button specifier</param>
        /// <returns>Current position of the button</returns>
        public Point GetButtonPosition(WizardButton flag)
        {
            if (WizardButton.Back == (flag & WizardButton.Back))
                return m_buttonBack.Location;
            if (WizardButton.Next == (flag & WizardButton.Next))
                return m_buttonNext.Location;
            if (WizardButton.Start == (flag & WizardButton.Start))
                return m_buttonStart.Location;
            if (WizardButton.Finish == (flag & WizardButton.Finish))
                return m_buttonFinish.Location;
            if (WizardButton.Cancel == (flag & WizardButton.Cancel))
                return m_buttonCancel.Location;
            throw new ApplicationException("Internal Error: Unrecognized wizard button.");
        }

        /// <summary>
        /// Sets the position of a wizard button
        /// </summary>
        /// <param name="flag">Button specifier</param>
        /// <param name="newLocation">New button location</param>
        /// <returns>Original position of the button</returns>
        public Point SetButtonPosition(WizardButton flag, Point newLocation)
        {
            Point originalLocation;

            if (WizardButton.Back == (flag & WizardButton.Back))
            {
                originalLocation = m_buttonBack.Location;
                m_buttonBack.Location = newLocation;
            }
            else if (WizardButton.Next == (flag & WizardButton.Next))
            {
                originalLocation = m_buttonNext.Location;
                m_buttonNext.Location = newLocation;
            }
            else if (WizardButton.Start == (flag & WizardButton.Start))
            {
                originalLocation = m_buttonStart.Location;
                m_buttonStart.Location = newLocation;
            }
            else if (WizardButton.Finish == (flag & WizardButton.Finish))
            {
                originalLocation = m_buttonFinish.Location;
                m_buttonFinish.Location = newLocation;
            }
            else if (WizardButton.Cancel == (flag & WizardButton.Cancel))
            {
                originalLocation = m_buttonCancel.Location;
                m_buttonCancel.Location = newLocation;
            }
            else
            {
                throw new ApplicationException("Internal Error: Unrecognized wizard button.");
            }
            return originalLocation;
        }
        
        protected void SetAcceptButton()
        {
            // Set the AcceptButton depending on whether or not the Finish
            // button is visible or not
            AcceptButton = m_buttonFinish.Visible && m_buttonFinish.Enabled ? m_buttonFinish :
                (m_buttonNext.Visible && m_buttonNext.Enabled ? m_buttonNext : null);
        }

        /// <summary>
        /// Activates the page at the specified index in the page array.
        /// </summary>
        /// <param name="newIndex">
        /// Index of new page to be selected.
        /// </param>
        protected void ActivatePage(int newIndex)
        {
            // Ensure the index is valid
            if (newIndex < 0 || newIndex >= _wizardPages.Count)
                throw new ArgumentOutOfRangeException();

            // Deactivate the current page if applicable
            WizardPage currentPage = null;
            if (_curPageIndex != -1)
            {
                currentPage = _wizardPages[_curPageIndex] as WizardPage;
                if (!currentPage.OnKillActive())
                {
                    return;
                }
            }

            // Activate the new page
            WizardPage newPage = _wizardPages[newIndex] as WizardPage;
            if (!newPage.OnSetActive())
            {
                return;
            }

            // Update state
            _curPageIndex = newIndex;
            if (currentPage != null)
                currentPage.Visible = false;
            newPage.Visible = true;
            newPage.Focus();
        }

        /// <summary>
        /// Handles the Click event for the Back button.
        /// </summary>
        private void OnClickBack(object sender, EventArgs e)
        {
            // Ensure a page is currently selected
            if (_curPageIndex != -1)
            {
                // Inform selected page that the Back button was clicked
                WizardPage curPage = _wizardPages[_curPageIndex] as WizardPage;
                string pageName = curPage.OnWizardBack();
                switch (pageName)
                {
                    // Do nothing
                    case NoPageChange:
                        break;

                    // Activate the next appropriate page
                    case NextPage:
                        if (_curPageIndex - 1 >= 0)
                            ActivatePage(_curPageIndex - 1);
                        break;

                    // Activate the specified page if it exists
                    default:
                        foreach (WizardPage page in _wizardPages)
                        {
                            if (page.Name == pageName)
                                ActivatePage(_wizardPages.IndexOf(page));
                        }
                        break;
                }
            }
        }

        public bool ConfirmCancel(string caption, string message)
        {
            DialogResult result = MessageBox.Show(this, 
                                    message, 
                                    caption,
                                    MessageBoxButtons.YesNo, 
                                    MessageBoxIcon.Question);

            if (result == DialogResult.Yes)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
        
        /// <summary>
        /// Handles the Click event for the Cancel button.
        /// </summary>
        private void OnClickCancel(object sender, EventArgs e)
        {
            if (_curPageIndex != -1)
            {
                // Inform selected page that the Cancel button was clicked
                WizardPage page = _wizardPages[_curPageIndex] as WizardPage;
                if (page.OnWizardCancel())
                {
                    DialogResult = System.Windows.Forms.DialogResult.Cancel;
                }
                else
                {
                    DialogResult = System.Windows.Forms.DialogResult.None;
                }
            }
        }

        /// <summary>
        /// Handles the Click event for the Finish button.
        /// </summary>
        private void OnClickFinish(object sender, EventArgs e)
        {
            // Ensure a page is currently selected
            if (_curPageIndex != -1)
            {
                // Inform selected page that the Finish button was clicked
                WizardPage page = _wizardPages[_curPageIndex] as WizardPage;
                if (page.OnWizardFinish())
                {
                    // Deactivate page and close wizard
                    if (page.OnKillActive())
                        DialogResult = DialogResult.OK;
                }
                else if (!String.IsNullOrEmpty(FirstPage))
                {
                    ActivatePage(0);
                }
            }
        }

        /// <summary>
        /// Handles the Click event for the Next button.
        /// </summary>
        private void OnClickNext(object sender, EventArgs e)
        {
            // Ensure a page is currently selected
            if (_curPageIndex != -1)
            {
                // Inform selected page that the Next button was clicked
                WizardPage curPage = _wizardPages[_curPageIndex] as WizardPage;
                string pageName = curPage.OnWizardNext();
                switch (pageName)
                {
                    // Do nothing
                    case NoPageChange:
                        break;

                    // Activate the next appropriate page
                    case NextPage:
                        if (_curPageIndex + 1 < _wizardPages.Count)
                            ActivatePage(_curPageIndex + 1);
                        break;

                    // Activate the specified page if it exists
                    default:
                        foreach (WizardPage page in _wizardPages)
                        {
                            if (page.Name == pageName)
                                ActivatePage(_wizardPages.IndexOf(page));
                        }
                        break;
                }
            }
        }

        private void m_buttonStart_Click(object sender, EventArgs e)
        {
            // Ensure a page is currently selected
            if (_curPageIndex != -1)
            {
                // Inform selected page that the Start button was clicked
                WizardPage curPage = _wizardPages[_curPageIndex] as WizardPage;
                string pageName = curPage.OnWizardStart();
                switch (pageName)
                {
                    // Do nothing
                    case NoPageChange:
                        break;

                    // Activate the next appropriate page
                    case NextPage:
                        if (_curPageIndex + 1 < _wizardPages.Count)
                        {
                            //pageHistory.Push(curPage.Name);
                            ActivatePage(_curPageIndex + 1);
                        }
                        break;

                    // Activate the specified page if it exists
                    default:
                        foreach (WizardPage page in _wizardPages)
                        {
                            if (page.Name == pageName)
                            {
                                //pageHistory.Push(curPage.Name);
                                ActivatePage(_wizardPages.IndexOf(page));
                            }
                        }
                        break;
                }
            }
        }

        private void WizardForm_Load(object sender, EventArgs e)
        {
           if (_wizardPages.Count > 0)
            {
                ActivatePage(0);
            }
        }       
    }
}