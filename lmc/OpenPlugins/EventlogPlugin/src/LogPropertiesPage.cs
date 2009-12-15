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
using System.Text.RegularExpressions;
using System.IO;
using Likewise.LMC.ServerControl;
using Likewise.LMC.Utilities;
using Likewise.LMC.Eventlog;


namespace Likewise.LMC.Plugins.EventlogPlugin
{
/// <summary>
/// This class encapsulates the form that is used to display information about a log file
/// as a whole
/// </summary>
    public partial class LogPropertiesPage : MPPage
    {
        #region Class data

        // back reference to the event log being viewed and the plugin container
        private EventLogRecord el;
        private IPlugInContainer _container;
        private EventlogPlugin _plugin;
        private StandardPage _parentPage;

        #endregion

        #region Constructors

        public LogPropertiesPage()
        {
            InitializeComponent();
        }

        /// <summary>
        /// The preferred constructor
        /// </summary>
        /// <param name="container">Back reference to the PlugInContainer</param>
        /// <param name="el">Reference to the log being viewed</param>
        public LogPropertiesPage(IPlugInContainer container, EventLogRecord el, IPlugIn plgin, StandardPage parentPage)
            : this()
        {
            this.pageID = "LogProperities";
            this._container = container;
            this.el = el;
            this._plugin = plgin as EventlogPlugin;
            this._parentPage = parentPage;
        }

        #endregion

        #region Event Handlers

        /// <summary>
        /// Handles the form load event
        /// </summary>
        private void LogPropertiesPage_Load(object sender, EventArgs e)
        {
            this.ParentContainer.btnApply.Enabled = false;
            this.ParentContainer.DataChanged = false;

            // set title
            Text = string.Format(Text, "<unset>");

            // load up the controls
            //LoadData();
        }

        /// <summary>
        /// Handles the OK button
        /// </summary>
        public bool OnOkApply()
        {
            return true;
        }

        private void btnClearEvtlog_Click(object sender, EventArgs e)
        {
            if (_plugin == null)
                return;

            UInt32 ret;

            string sMsg = string.Format("Are you sure do you want to clear the EventLog database?\n" +
                          "\ni. Select on 'Yes' will clear the entire EventLog database." +
                          "\nii. Select on 'No' clears all the records of the Log {0}", _parentPage.TreeNode.Text);

            DialogResult dlg = MessageBox.Show(this,
                                             sMsg,
                                             CommonResources.GetString("Caption_Console"),
                                             MessageBoxButtons.YesNoCancel,
                                             MessageBoxIcon.Question,
                                             MessageBoxDefaultButton.Button3);
            if (dlg == DialogResult.Yes)
            {
                ret = EventAPI.ClearEventLog(this._plugin.eventLogHandle.Handle);
                if (ret != 0)
                {
                    Logger.Log(string.Format("Unable to clear the events log database"));
                    return;
                }
                ClearListView();
            }
            else if (dlg == DialogResult.No)
            {
                ret = EventAPI.DeleteFromEventLog(this._plugin.eventLogHandle.Handle, string.Format("(EventTableCategoryId = '{0}')", _parentPage.TreeNode.Text));
                if (ret != 0)
                {
                    Logger.Log(string.Format("Unable to delete the events for the current log {0}", _parentPage.TreeNode.Text));
                    return;
                }
                ClearListView();
            }
            else
            {
                return;
            }
        }

        #endregion

        #region Helpers Methods

        private void ClearListView()
        {
            if (_parentPage is EventViewerControl)
            {
                EventViewerControl eventViewerControl = _parentPage as EventViewerControl;
                if (eventViewerControl != null)
                {
                    eventViewerControl.EventViewerListView.Items.Clear();
                }
            }
        }

        /// <summary>
        /// Loads up controls with data from the current log
        /// </summary>
        private void LoadData()
        {
            // update the button state
            SetButtonState();
        }

        /// <summary>
        /// Updates the state of any controls that are enabled/disabled according to context
        /// </summary>
        private void SetButtonState()
        {

        }

        /*private void ReadEventLogConfEntries()
        {
            string eventlogfilePath = Path.Combine(folderPath, eventlogdName);
            string lsassdFilePath=Path.Combine(folderPath,lsassdName);

            try
            {
                if (File.Exists(eventlogfilePath))
                {
                    string text = File.ReadAllText(eventlogfilePath); // read file

                    foreach (string key in logentries.Keys)
                    {
                        string strSource = string.Format("^{0} = (.*)$", key); // exp to find 1st arg

                        // look for 2nd arg first
                        bool found = Regex.IsMatch(text, strSource, RegexOptions.IgnoreCase);
                        if (found)
                        {
                            // now look for 1st arg and replace with 3rd arg
                            //text = Regex.Replace(text, strSource, strDest, RegexOptions.IgnoreCase);
                            Match match = Regex.Match(text, strSource, RegexOptions.IgnoreCase);

                            if (match != null)
                            {
                                string fileValue = match.Value;
                                if (!String.IsNullOrEmpty(fileValue) && fileValue.IndexOf("=") >= 0)
                                {
                                    string[] splits = fileValue.Split('=');
                                    if (splits != null && splits.Length >= 2)
                                    {
                                        logentries[key] = splits[1];
                                    }
                                }
                            }

                            File.WriteAllText(eventlogfilePath, text); // write back to file
                        }
                    }
                }
                if (File.Exists(lsassdFilePath))
                {
                    string text = File.ReadAllText(lsassdFilePath); // read file

                    string strSource = string.Format("^enable-eventlog = (.*)$"); // exp to find 1st arg

                    // look for 2nd arg first
                    bool found = Regex.IsMatch(text, strSource, RegexOptions.IgnoreCase);
                    if (found)
                    {
                        // now look for 1st arg and replace with 3rd arg
                        //text = Regex.Replace(text, strSource, strDest, RegexOptions.IgnoreCase);
                        Match match = Regex.Match(text, strSource, RegexOptions.IgnoreCase);

                        if (match != null)
                        {
                            string fileValue = match.Value;
                            if (!String.IsNullOrEmpty(fileValue) && fileValue.IndexOf("=") >= 0)
                            {
                                string[] splits = fileValue.Split('=');
                                if (splits != null && splits.Length >= 2)
                                {
                                    logentries["enable-eventlog"] = splits[1];
                                }
                            }
                        }

                        File.WriteAllText(lsassdFilePath, text); // write back to file
                    }
                }
                txtDeleteEvents.Text = logentries["allow-delete-to"];
                txtReadEvents.Text = logentries["allow-read-to"];
                txtWriteEvents.Text = logentries["allow-write-to"];
                nudEventsLifeSpan.Value = Convert.ToDecimal(logentries["max-event-lifespan"]);
                nudMaxDiskSize.Value = Convert.ToDecimal(logentries["max-disk-usage"]);
                nudNumEvents.Value = Convert.ToDecimal(logentries["max-num-events"]);

                if (logentries["enable-eventlog"] == "yes")
                {
                    rbRemoveEventsTrue.Checked = true;
                }
                else
                    rbRemoveEventsFalse.Checked = false;

                if (logentries["enable-eventlog"] == "yes")
                {
                    radioButtonEventlogTrue.Checked = true;
                }
                else
                    radioButtonEvtLogEnaledfalse.Checked = false;
            }
            catch(Exception ex)
            {
                Logger.LogException("LogPropertiesPage.ReadEventLogConfEntries()", ex);
            }
        }

        private void UpdateEventLogConfEntries()
        {
            string eventlogfilePath = Path.Combine(folderPath, eventlogdName);
            string lsassdFilePath=Path.Combine(folderPath,lsassdName);

            try
            {
                if (File.Exists(eventlogfilePath))
                {
                    string text = File.ReadAllText(eventlogfilePath); // read file
                    foreach (string key in logentries.Keys)
                    {
                        string strSource = string.Format("^{0} = (.*)$", key); // exp to find 1st arg
                        string strDest = string.Format("    {0} = {1}", key, logentries[key]);

                        // look for 2nd arg first
                        bool found = Regex.IsMatch(text, strSource, RegexOptions.IgnoreCase);
                        if (found)
                        {
                            // now look for 1st arg and replace with 3rd arg
                            text = Regex.Replace(text, strSource, strDest, RegexOptions.IgnoreCase);

                            File.WriteAllText(eventlogfilePath, text); // write back to file
                        }
                    }
                }
            }
            catch(Exception ex)
            {
                Logger.LogException("LogPropertiesPage.UpdateEventLogConfEntries()", ex);
            }
        }*/

        #endregion
}
}
