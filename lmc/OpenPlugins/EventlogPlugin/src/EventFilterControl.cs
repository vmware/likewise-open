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
using Likewise.LMC.Utilities;
using Likewise.LMC.Eventlog;

namespace Likewise.LMC.Plugins.EventlogPlugin
{
    /// <summary>
    /// This class encapsulates the dialog box that allows the user to specify how events
    /// should be filtered
    /// </summary>
    public partial class EventFilterControl : MPPage, IDirectoryPropertiesPage
    {
        #region Class data

        // true if the default filter values are shown (i.e. no need to filter)
        private bool bDefaults;
        private EventFilter _ef = null;

        #endregion

        #region Constructors

        public EventFilterControl()
        {
            InitializeComponent();
        }

        /// <summary>
        /// This is the preferred constructor.
        /// </summary>
        /// <param name="el">Back reference to the event log being displayed</param>
        /// <param name="ef">Reference to current filter settings (can be null)</param>
        public EventFilterControl(EventFilter ef)
            : this()
        {
            this.pageID = "FilterProperities";
            _ef = ef;
        }

        #endregion

        #region Initialization methods

        /// <summary>
        /// Queries and fills the ldap message for the selected OU
        /// Gets the attribute list from AD for OU schema attribute.
        /// search for the attributes description, ou or name and displays them in a controls
        /// </summary>
        /// <param name="ce"></param>
        /// <param name="servername"></param>
        /// <param name="name"></param>
        /// <param name="dirnode"></param>
        public void SetData()
        {
            // either set the defaults or initialize as per the existing filter settings
            if (_ef == null)
            {
                SetDefaults();
            }
            else
            {
                Initialize(_ef);
            }

            this.ParentContainer.btnApply.Enabled = false;
            this.ParentContainer.DataChanged = false;
        }

        #endregion


        #region Property Accessors

        /// <summary>
        /// Returns a filter object that represents the user filter settings
        /// </summary>
        public EventFilter Filter
        {
            get
            {
                // if nothing's changed, return a null filter
                if (bDefaults)
                {
                    return null;
                }

                EventFilter ef = new EventFilter();

                ef.ShowError = cbError.Checked;
                ef.ShowInformation = cbInformation.Checked;
                ef.ShowWarning = cbWarning.Checked;
                ef.ShowSuccessAudit = cbSuccessAudit.Checked;
                ef.ShowFailureAudit = cbFailureAudit.Checked;

                ef.EventSource = tbEventSource.Text;

                ef.CustomFilterString = tbCustomFilterString.Text;

                if (cbCategory.Text != "(All)")
                {
                    ef.Category = cbCategory.Text;
                }
                else
                {
                    ef.Category = "";
                }

                try
                {
                    if (tbEventID.Text != null && tbEventID.Text != "")
                    {
                        ef.EventId = long.Parse(tbEventID.Text);
                    }
                }
                catch (Exception ex)
                {
                    Logger.Log(ex.Message, Logger.manageLogLevel);
                }

                ef.User = tbUser.Text;
                ef.Computer = tbComputer.Text;

                if (rbRestrictDates.Checked)
                {
                    DateTime dateTimeStart = new DateTime(dtStart.Value.Year, dtStart.Value.Month,
                                          dtStart.Value.Day, tmStart.Value.Hour, tmStart.Value.Minute,
                                          tmStart.Value.Second);
                    ef.StartDate = dateTimeStart;

                    DateTime dateTimeEnd = new DateTime(dtEnd.Value.Year, dtEnd.Value.Month,
                                          dtEnd.Value.Day, tmEnd.Value.Hour, tmEnd.Value.Minute,
                                          tmEnd.Value.Second);
                    ef.EndDate = dateTimeEnd;
                }
                return ef;
            }
        }

        #endregion

        #region Helper functions

        /// <summary>
        /// Sets the UI widgets to the default (no filtering) values
        /// </summary>
        private void SetDefaults()
        {
            cbError.Checked = true;
            cbInformation.Checked = true;
            cbWarning.Checked = true;
            cbSuccessAudit.Checked = true;
            cbFailureAudit.Checked = true;

            tbEventSource.Text = "";

            if (cbCategory.Items.Count != 0)
            {
                cbCategory.SelectedIndex = 0;
            }

            tbEventID.Text = "";
            tbUser.Text = "";
            tbComputer.Text = "";
            tbCustomFilterString.Text = "";
            rbShowAll.Checked = true;

            bDefaults = true;

            this.ParentContainer.btnApply.Enabled = true;
            this.ParentContainer.DataChanged = true;
        }

        /// <summary>
        /// Sets the UI widgets as per a given filter object
        /// </summary>
        /// <param name="ef">The given filter object</param>
        private void Initialize(EventFilter ef)
        {
            cbError.Checked = ef.ShowError;
            cbWarning.Checked = ef.ShowWarning;
            cbInformation.Checked = ef.ShowInformation;
            cbSuccessAudit.Checked = ef.ShowSuccessAudit;
            cbFailureAudit.Checked = ef.ShowFailureAudit;

            tbEventSource.Text = ef.EventSource;

            if (ef.Category != "")
            {
                cbCategory.Text = ef.Category;
            }
            else
            {
                cbCategory.Text = "(All)";
            }

            if (ef.EventId != -1)
            {
                tbEventID.Text = ef.EventId.ToString();
            }
            else
            {
                tbEventID.Text = "";
            }

            tbCustomFilterString.Text = ef.CustomFilterString;

            tbUser.Text = ef.User;
            tbComputer.Text = tbComputer.Text;

            if (ef.StartDate == DateTime.MinValue &&
            ef.EndDate == DateTime.MaxValue)
            {
                rbShowAll.Checked = true;
            }
            else
            {
                rbRestrictDates.Checked = true;
                if (ef.StartDate != null)
                {
                    dtStart.Value = ef.StartDate;
                    tmStart.Value = ef.StartDate;
                }
                if (ef.EndDate != null)
                {
                    dtEnd.Value = ef.EndDate;
                    tmEnd.Value = ef.EndDate;
                }
            }

            bDefaults = false;

        }

        /// <summary>
        /// Handles the OK button
        /// </summary>
        public bool OnOkApply()
        {
            if (!bDefaults)
            {
                if (Filter.StartDate.ToLocalTime() > Filter.EndDate.ToLocalTime())
                {
                    container.ShowError("The date/time in View Events From is later than the date/time in View Events To");
                    return false;
                }
            }

            return true;
        }

        #endregion

        #region Event handlers

        /// <summary>
        /// Handles the Restore Defaults button
        /// </summary>
        private void btnDefaults_Click(object sender, EventArgs e)
        {
            SetDefaults();
        }

        /// <summary>
        /// Handles changes to the radio buttons that are used to specify whether
        /// we should filter on dates
        /// </summary>
        private void rbRestrictDates_CheckedChanged(object sender, EventArgs e)
        {
            dtStart.Enabled = dtEnd.Enabled = rbRestrictDates.Checked;
            tmStart.Enabled = tmEnd.Enabled = rbRestrictDates.Checked;
            SettingChanged(sender, e);
        }

        /// <summary>
        /// Called in various places (event and internal) to indicate that
        /// some value has changed and that we no longer have default settings.
        /// </summary>
        private void SettingChanged(object sender, EventArgs e)
        {
            bDefaults = false;
            ParentContainer.DataChanged = true;
            ParentContainer.btnApply.Enabled = true;
        }

        private void tbEventID_KeyPress(object sender, KeyPressEventArgs e)
        {
            if (!Char.IsDigit(e.KeyChar) &&
            (!(e.KeyChar == 8)) &&
            (!(e.KeyChar == 22)) &&
            (!(e.KeyChar == 3)))
            {
                e.Handled = true;
            }
        }       

        private void dtStart_ValueChanged(object sender, EventArgs e)
        {
            SettingChanged(sender, e);
        }

        #endregion
    }
}
