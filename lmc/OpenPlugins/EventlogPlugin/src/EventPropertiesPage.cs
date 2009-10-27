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
using System.Collections;
using Likewise.LMC.ServerControl;
using Likewise.LMC.LMConsoleUtils;
using Likewise.LMC.Eventlog;


namespace Likewise.LMC.Plugins.EventlogPlugin
{
public partial class EventPropertiesPage : MPPage, IDirectoryPropertiesPage
{
    #region Class data
    
    // back reference to the log being viewed and the
    // datagrid that is viewing it (so that we can
    // navigate up and down on it)
    private ListView _eventsListView;
    
    #endregion
    
    #region Constructors
    public EventPropertiesPage()
    {
        InitializeComponent();
    }
    
    /// <summary>
    /// This is the preferred constructor.
    /// </summary>
    public EventPropertiesPage(ListView lvEvents)
    : this()
    {
        this.pageID = "EventProperities";
        _eventsListView = lvEvents;
    }
    #endregion
    
    #region IDirectoryPropertiesPage Members
    
    void IDirectoryPropertiesPage.SetData()
    {
        // load the data for the current event
        LoadData();
        
        // update the next/prev button state
        SetButtonState();
    }
    
    #endregion
    
    /// <summary>
    /// Handles the prev button
    /// </summary>
    private void btnPrev_Click(object sender, EventArgs e)
    {  
        if (_eventsListView == null ||
        _eventsListView.Items.Count == 0 ||
        _eventsListView.SelectedItems.Count != 1 ||
        !btnPrev.Enabled)
        {
            return;
        }
        
        int iEntry = _eventsListView.SelectedItems[0].Index;

        if (iEntry == 0)
        {
            DialogResult dlg = MessageBox.Show(this, "You have reached the beginning of the Event log. Do you want to continue from the end?",
                             CommonResources.GetString("Caption_Console"), MessageBoxButtons.YesNo, MessageBoxIcon.Information);
            if (dlg == DialogResult.Yes)
            {
                iEntry = _eventsListView.Items.Count;
                btnPrev.Focus();
            }
            else
            {
                btnPrev.Focus();
                return;
            }
        }

        if (iEntry != 0)
            // bump the count
            iEntry--;
        
        // change the selection
        _eventsListView.SelectedItems[0].Selected = false;
        _eventsListView.Items[iEntry].Selected = true;
        
        // scroll into view if necessary
        ScrollIntoView(iEntry);
        
        // update the button state
        SetButtonState();
        
        LoadData();
    }
    
    /// <summary>
    /// Handles the next button
    /// </summary>
    private void btnNext_Click(object sender, EventArgs e)
    {
        if (_eventsListView == null ||
        _eventsListView.Items.Count == 0 ||
        _eventsListView.SelectedItems.Count != 1 ||
        !btnNext.Enabled)
        {
            return;
        }
        
        int iEntry = _eventsListView.SelectedItems[0].Index;

        if (iEntry == _eventsListView.Items.Count - 1)
        {
            DialogResult dlg = MessageBox.Show(this, "You have reached the end of the Event log. Do you want to continue from the beginning?",
                             CommonResources.GetString("Caption_Console"), MessageBoxButtons.YesNo, MessageBoxIcon.Information);
            if (dlg == DialogResult.Yes)
            {
                iEntry = -1;
                btnNext.Focus();
            }
            else
            {
                btnNext.Focus();
                return;
            }
        }
        
        iEntry++;
        
        // change the selection
        _eventsListView.SelectedItems[0].Selected = false;
        _eventsListView.Items[iEntry].Selected = true;
        
        // scroll into view if necessary
        ScrollIntoView(iEntry);
        
        // update the button state
        SetButtonState();
        
        LoadData();
    }
    
    private void btnCopy_Click(object sender, EventArgs e)
    {
        try
        {
            string eventData = "Event Type:    {0}" + "\n" + "Event Source:    {1}" + "\n" + "Event Category:    {2}" + "\n" + "Event ID:    {3}" + "\n" +
            "Date:        {4}" + "\n" + "Time:        {5}" + "\n" + "User:        {6}" + "\n" + "Computer:    {7}" + "\n" +
            "Description:" + "\n" + "{8}";
            
            string clipBoardData = string.Empty;
            // load the selected row
            if (_eventsListView.SelectedItems.Count == 0)
            {
                return;
            }
            
            EventLogRecord el = _eventsListView.SelectedItems[0].Tag as EventLogRecord;
            if (el != null)
            {
                EventAPI.EventLogRecord eventRecord = el.Record;
                DateTime eventTime = EventUtils.Time_T2DateTime(eventRecord.dwEventDateTime);
                eventTime = eventTime.ToLocalTime();
                
                // copy fields to clipboard
                clipBoardData = string.Format(eventData, eventRecord.pszEventType, eventRecord.pszEventSource, eventRecord.pszEventCategory, eventRecord.dwEventSourceId.ToString(),
                eventTime.ToString("MM/dd/yyyy"), eventTime.ToString("hh:mm:ss"), eventRecord.pszUser, eventRecord.pszComputer, eventRecord.pszDescription);
            }
            if (clipBoardData != "")
            {
                if (Clipboard.ContainsText())
                {
                    Clipboard.Clear();
                }
                Clipboard.SetText(clipBoardData);
            }
        }
        catch (Exception ex)
        {
            Logger.Log(ex.ToString(), Logger.LogLevel.Error);
        }
    }
    
    /// <summary>
    /// Assures that the indicated row is visible
    /// </summary>
    /// <param name="iEntry">The row whose visibility is to be assured</param>
    private void ScrollIntoView(int iEntry)
    {
        _eventsListView.EnsureVisible(iEntry);
    }
    
    #region Helper functions
    
    /// <summary>
    /// Updates the prev/next buttons as per the current selection in the datagridview
    /// </summary>
    private void SetButtonState()
    {
        // get the current selection
        if (_eventsListView.SelectedItems.Count == 1)
        {
            int iEntry = _eventsListView.SelectedItems[0].Index;
            
            // enable disable as needed
            btnPrev.Enabled = iEntry >= 0;
            btnNext.Enabled = (iEntry <= (_eventsListView.Items.Count - 1));
        }
        else
        {
            btnPrev.Enabled = false;
            btnNext.Enabled = false;
        }
    }
    
    /// <summary>
    /// Loads data into the properties controls
    /// </summary>
    private void LoadData()
    {
        try
        {
            // load the selected row
            if (_eventsListView.SelectedItems.Count == 0)
            {
                return;
            }
            
            EventLogRecord el = _eventsListView.SelectedItems[0].Tag as EventLogRecord;
            if (el != null)
            {
                EventAPI.EventLogRecord eventRecord = el.Record;
                DateTime eventTime = EventUtils.Time_T2DateTime(eventRecord.dwEventDateTime);
                eventTime = eventTime.ToLocalTime();
                // setup fields
                tbDate.Text = eventTime.ToString("MM/dd/yyyy");
                tbTime.Text = eventTime.ToLongTimeString();// ToString("hh:mm:ss");
                tbType.Text = eventRecord.pszEventType;
                tbUser.Text = eventRecord.pszUser;
                tbComputer.Text = eventRecord.pszComputer;
                tbSource.Text = eventRecord.pszEventSource;
                tbCategory.Text = eventRecord.pszEventCategory;
                tbEventId.Text = eventRecord.dwEventSourceId.ToString();
                tbDescription.Text = eventRecord.pszDescription;
            }
        }
        catch (Exception ex)
        {
            Logger.Log(ex.ToString(), Logger.LogLevel.Error);
        }
    }
    #endregion
}
}
