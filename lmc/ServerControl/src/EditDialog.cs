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
using System.Windows.Forms;
using Likewise.LMC.Utilities;

namespace Likewise.LMC.ServerControl
{
    /// <summary>
    /// This is a base class for all editable dialogs.
    /// This class handles the enabling and disabling of the buttons depending
    /// on the state of the child controls.
    /// </summary>
    ///
    public partial class EditDialog : Form
    {
        #region Class data
        protected IPlugInContainer container = null;
        private StandardPage parentPage = null;
        public bool bDataWasChanged = false;
        private bool bHasErrors = false;
        private bool bShowApplyButton = true;
        private bool bAllowCancelDuringApply = false;
        private bool bApplyInProgress = false;
        private string sHelpKey = "";
        #endregion

        #region constructors

        // dialog actions
        public enum EditDialogAction { ACTION_OK, ACTION_APPLY, ACTION_CANCEL };

        #endregion

        #region Public interface

        public EditDialog(IPlugInContainer container, StandardPage parentPage)
        {
            this.container = container;
            this.parentPage = parentPage;
            InitializeComponent();
        }

        public EditDialog(StandardPage parentPage)
        {
            this.parentPage = parentPage;
            InitializeComponent();
        }

        public EditDialog()
        {
            InitializeComponent();
        }

        /// <summary>
        /// Set or get the parent page that shows this dialog.
        /// </summary>
        public StandardPage ParentPage
        {
            get
            {
                return this.parentPage;
            }
            set
            {
                this.parentPage = value;
            }
        }

        /// <summary>
        /// This property reflects a state of the data displayed in the dialog.
        /// It is set to true if any data was changed.
        ///
        /// Some controls (ListBox) do not provide an event handler that can be used to detect
        /// if a control data was changed. For these controls set this value directly
        /// when the control data is modified programatically.
        /// </summary>
        public bool DataChanged
        {
            get
            {
                return this.bDataWasChanged;
            }
            set
            {
                this.bDataWasChanged = value;
            }
        }

        /// <summary>
        /// Set or get the error flag indicating that there are errors in the dialog.
        /// </summary>
        public bool HasErrors
        {
            get
            {
                return this.bHasErrors;
            }
            set
            {
                this.bHasErrors = value;
                SetContainerState();
            }
        }

        public bool AllowCancelDuringApply
        {
            get
            {
                return bAllowCancelDuringApply;
            }
            set
            {
                bAllowCancelDuringApply = value;
            }
        }

        /// <summary>
        /// Override this in derived class to check if any controls are in error.
        /// Set the HasErrors flag accordingly.
        /// </summary>
        public virtual void RefreshErrorState()
        {
        }

        /// <summary>
        /// Set or get the ShowApplyButton property.
        /// If set to true the Apply button will be shown on the dialog.
        /// </summary>
        [Browsable(true)]
        public bool ShowApplyButton
        {
            get
            {
                return bShowApplyButton;
            }
            set
            {
                this.bShowApplyButton = value;
            }
        }

        [Browsable(true)]
        public string HelpKeyword
        {
            get
            {
                return sHelpKey;
            }
            set
            {
                sHelpKey = value;
            }
        }

        #region Access to buttons

        // The buttons are not simply just made public
        // because the VS designer keeps messing up their positions
        // in the inherited classes

        public Button ButtonOK
        {
            get
            {
                return btnOK;
            }
        }

        public Button ButtonCancel
        {
            get
            {
                return btnCancel;
            }
        }

        public Button ButtonApply
        {
            get
            {
                return btnApply;
            }
        }

        #endregion

        #endregion

        #region Helper methods

        /// <summary>
        /// Override this in subclass to validate all the dialog data.
        /// </summary>
        /// <param name="actionCause">indicates the action causing data to be validated</param>
        protected virtual bool ValidateAllData(EditDialogAction actionCause)
        {
            // do nothing
            return true;
        }

        /// <summary>
        /// Override this in subclass to write dialog data to persistent storage.
        /// NOTE: there is no separate method for OK. Clicking either OK or Apply buttons
        /// will cause this method to be called, but only if dialog is free of errors and
        /// data was modified. The actionCause parameter will be set to
        /// ACTION_OK or ACTION_APPLY accordingly.
        /// </summary>
        /// <param name="actionCause">indicates the action causing data to be validated</param>
        /// <returns></returns>
        protected virtual bool Apply(EditDialogAction actionCause)
        {
            // do nothing
            return true;
        }

        /// <summary>
        /// Called when the cancel buton is clicked.
        /// Override if necessary.
        /// </summary>
        protected virtual void OnCancel(object sender, EventArgs e)
        {

        }

        /// <summary>
        /// Sets the apperance of the entire dialog based on the error state.
        /// </summary>
        /// <param name="bErrors">pass true if errors exist, false otherwise</param>
        protected virtual void SetContainerState()
        {
            // don't allow changing the state of the buttons during apply operation
            if (bApplyInProgress)
            {
                this.btnCancel.Enabled = bAllowCancelDuringApply;
                this.btnOK.Enabled = false;
                this.btnApply.Enabled = false;
                return;
            }

            if (!this.btnCancel.Enabled)
                this.btnCancel.Enabled = true;

            if (bHasErrors)
            {
                btnApply.Enabled = false;
                btnOK.Enabled = false;
                bHasErrors = false;
            }
            else
            {
                btnApply.Enabled = bDataWasChanged;
                btnOK.Enabled = true;
            }
        }

        private void SetValueChangedHandler(Control ctrl)
        {
            if (ctrl.GetType().Equals(typeof(TextBox)))
            {
                ((TextBox)ctrl).TextChanged += new EventHandler(ValueChangedHandler);
            }
            else if (ctrl.GetType().Equals(typeof(ComboBox)))
            {
                ((ComboBox)ctrl).SelectedIndexChanged += new EventHandler(ValueChangedHandler);
            }
            else if (ctrl.GetType().Equals(typeof(RadioButton)))
            {
                ((RadioButton)ctrl).CheckedChanged += new EventHandler(ValueChangedHandler);
            }
            else if (ctrl.GetType().Equals(typeof(DateTimePicker)))
            {
                ((DateTimePicker)ctrl).ValueChanged += new EventHandler(ValueChangedHandler);
            }
            else if (ctrl.GetType().Equals(typeof(ListBox)))
            {
                ((ListBox)ctrl).Click += new EventHandler(ValueChangedHandler);
            }
            else if (ctrl.GetType().Equals(typeof(ListView)))
            {
                ((ListView)ctrl).Click += new EventHandler(ValueChangedHandler);
            }
            else if (ctrl.GetType().Equals(typeof(ListView)))
            {
                ((ListView)ctrl).Enter += new EventHandler(ValueChangedHandler);
            }
            else if (ctrl.GetType().Equals(typeof(DataGridView)))
            {
                ((DataGridView)ctrl).Click += new EventHandler(ValueChangedHandler);
            }
            else if (ctrl.GetType().Equals(typeof(CheckBox)))
            {
                ((CheckBox)ctrl).Click += new EventHandler(ValueChangedHandler);
            }
            else
            {
                // last resort .. see if the object implements the DataModifiedNotify interface
                IDataModifiedNotify notify = ctrl as IDataModifiedNotify;
                if (notify != null)
                {
                    notify.AddDataModifiedHandler(new EventHandler(ValueChangedHandler));
                }
            }
        }

        /// <summary>
        /// Find all editable controls and add value modified handlers
        /// </summary>
        /// <param name="ctrl"></param>
        protected void SetAllValueChangedHandlers(Control ctrl)
        {
            SetValueChangedHandler(ctrl);

            // recusive traversal of all child controls
            foreach (Control child in ctrl.Controls)
                SetAllValueChangedHandlers(child);
        }

        protected virtual void ValueChangedHandler(object sender, EventArgs e)
        {
            //bDataWasChanged = true;
            SetContainerState();
        }

        #endregion

        #region Event handlers

        private void StartApply()
        {
            bApplyInProgress = true;
            SetContainerState();
        }

        private void EndApply()
        {
            bApplyInProgress = false;
            SetContainerState();
        }

        private void btnOK_Click(object sender, EventArgs e)
        {
            if (bDataWasChanged)
            {
                StartApply();

                if (!ValidateAllData(EditDialogAction.ACTION_OK))
                {
                    this.DialogResult = DialogResult.None;
                    return;
                }
                if (Apply(EditDialogAction.ACTION_OK))
                {
                    bDataWasChanged = false;
                    Close();
                }
                else
                {
                    this.DialogResult = DialogResult.None;
                }
            }

            EndApply();
        }

        private void btnApply_Click(object sender, EventArgs e)
        {
            StartApply();

            // validate data, persist it and clear the DataWasChanged flag if all is OK
            if (!ValidateAllData(EditDialogAction.ACTION_APPLY))
            {
                EndApply();
                return;
            }

            if (Apply(EditDialogAction.ACTION_APPLY))
            {
                bDataWasChanged = false;
            }

            EndApply();
        }

        private void EditDialog_Load(object sender, EventArgs e)
        {
            if (!this.bShowApplyButton)
            {
                this.btnApply.Visible = false;
                this.btnOK.Location = btnCancel.Location;
                this.btnCancel.Location = btnApply.Location;
            }

            SetContainerState();
        }

        private void btnCancel_Click(object sender, EventArgs e)
        {
            OnCancel(sender, e);
        }

        private void btnOK_EnabledChanged(object sender, EventArgs e)
        {
            // Set the focus to the OK button.
            // Needed when last field on a form is validated.

            if (btnOK.Enabled && this.ActiveControl == btnCancel)
                btnOK.Focus();

        }

        #endregion
    }
}