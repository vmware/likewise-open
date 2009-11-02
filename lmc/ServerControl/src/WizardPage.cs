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
using System.Windows.Forms;
using Likewise.LMC.Utilities;

namespace Likewise.LMC.ServerControl
{
    public partial class WizardPage : UserControl
    {
        #region Class data
        private WizardDialog _wizard = null;
        private IPlugInContainer _container = null;
        #endregion

        #region Constructor
        public WizardPage()
        {
            InitializeComponent();
        }
        
        public WizardPage(WizardDialog wizard)
        : this()
        {
            _wizard = wizard;
        }
        #endregion

        #region accessor function
        protected IPlugInContainer PlugInContainer
        {
            get
            {
                return _container;
            }
            set
            {
                _container = value;
            }
        }
        
        public WizardDialog Wizard
        {
            get
            {
                return _wizard;
            }
            set
            {
                _wizard = value;
            }
        }
        #endregion

        /// <summary>
        /// Called when the page is no longer the active page.
        /// </summary>
        /// <returns>
        /// <c>true</c> if the page was successfully deactivated; otherwise
        /// <c>false</c>.
        /// </returns>
        /// <remarks>
        /// Override this method to perform special data validation tasks.
        /// </remarks>
        public virtual bool OnKillActive()
        {
            // Deactivate if validation successful
            return Validate();
        }

        /// <summary>
        /// Called when the page becomes the active page.
        /// </summary>
        /// <returns>
        /// <c>true</c> if the page was successfully set active; otherwise
        /// <c>false</c>.
        /// </returns>
        /// <remarks>
        /// Override this method to performs tasks when a page is activated.
        /// Your override of this method should call the default version
        /// before any other processing is done.
        /// </remarks>
        public virtual bool OnSetActive()
        {
            return true;
        }

        /// <summary>
        /// Called when the user clicks the Back button in a wizard.
        /// </summary>
        /// <returns>
        /// <c>WizardForm.DefaultPage</c> to automatically advance to the
        /// next page; <c>WizardForm.NoPageChange</c> to prevent the page
        /// changing.  To jump to a page other than the next one, return
        /// the <c>Name</c> of the page to be displayed.
        /// </returns>
        /// <remarks>
        /// Override this method to specify some action the user must take
        /// when the Back button is pressed.
        /// </remarks>
        public virtual string OnWizardBack()
        {
            // Move to the default previous page in the wizard
            return WizardDialog.NextPage;
        }

        /// <summary>
        /// Called when the user clicks the Finish button in a wizard.
        /// </summary>
        /// <returns>
        /// <c>true</c> if the wizard finishes successfully; otherwise
        /// <c>false</c>.
        /// </returns>
        /// <remarks>
        /// Override this method to specify some action the user must take
        /// when the Finish button is pressed.  Return <c>false</c> to
        /// prevent the wizard from finishing.
        /// </remarks>
        public virtual bool OnWizardFinish()
        {
            // Finish the wizard
            return true;
        }

        /// <summary>
        /// Called when the user clicks the Next button in a wizard.
        /// </summary>
        /// <returns>
        /// <c>WizardForm.DefaultPage</c> to automatically advance to the
        /// next page; <c>WizardForm.NoPageChange</c> to prevent the page
        /// changing.  To jump to a page other than the next one, return
        /// the <c>Name</c> of the page to be displayed.
        /// </returns>
        /// <remarks>
        /// Override this method to specify some action the user must take
        /// when the Next button is pressed.
        /// </remarks>
        public virtual string OnWizardNext()
        {
            // Move to the default next page in the wizard
            return WizardDialog.NextPage;
        }

        /// <summary>
        /// Called when the user clicks the Start button in a wizard.
        /// </summary>        
        /// <remarks>
        /// Override this method to specify some action the user must take
        /// when the Start button is pressed. 
        /// </remarks>
        public virtual string OnWizardStart()
        {
            // Move to the default next page in the wizard
            return WizardDialog.NextPage;
        }

        /// <summary>
        /// Called when the user clicks the Cancel button in a wizard.
        /// </summary>
        /// <returns>true if the form should be canceled.</returns>
        /// <remarks>
        /// Override this method to specify some action the user must take
        /// when the Cancel button is pressed.
        /// </remarks>
        public virtual bool OnWizardCancel()
        {
            // Cancel the form
            return true;
        }

        /// <summary>
        /// Shows the EULA.
        /// </summary>
        public virtual void OnWizardEula()
        {
            throw new NotImplementedException();
        }

        /// <summary>
        /// Activates the Skip functionality.
        /// </summary>
        public virtual void OnWizardSkip()
        {
            throw new NotImplementedException();
        }
    }
}
