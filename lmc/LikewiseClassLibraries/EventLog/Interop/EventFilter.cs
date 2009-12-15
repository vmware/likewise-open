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
using System.Diagnostics;
using Likewise.LMC.Utilities;

namespace Likewise.LMC.Eventlog
{
/// <summary>
/// This class encapsulates a filter setting. When the user fills out the
/// FilterForm, the form generates an EventFilter that reflects the user's
/// specifications.
/// </summary>
public class EventFilter
{
    #region Class data
    
    // booleans for the various event types
    private bool bShowInformation = true;
    private bool bShowWarning = true;
    private bool bShowError = true;
    private bool bShowSuccessAudit = true;
    private bool bShowFailureAudit = true;
    
    // text fields for the source, category, user and computer. An empty string
    // indicates that no filter should take place.
    private string sEventSource = "";
    private string sCategory = "";
    private string sUser = "";
    private string sComputer = "";
    private string sCustomFilterString = "";
    
    // the event id to look for (-1 means don't test)
    private long lEventId = -1;
    
    // the first and last datetimes to look at
    private DateTime dtStart = DateTime.MinValue;
    private DateTime dtEnd = DateTime.MaxValue;
    
    #endregion
    
    #region Constructor
    
    //public EventFilter()
    //{
    // do nothing - all filter settings are set through property accessors
    //}
    
    #endregion
    
    #region Property Accessors
    
    // These are all straightforward get/set pairs
    
    public bool ShowInformation
    {
        get
        {
            return bShowInformation;
        }
        set
        {
            bShowInformation = value;
        }
    }
    
    public bool ShowWarning
    {
        get
        {
            return bShowWarning;
        }
        set
        {
            bShowWarning = value;
        }
    }
    
    public bool ShowError
    {
        get
        {
            return bShowError;
        }
        set
        {
            bShowError = value;
        }
    }
    
    public bool ShowSuccessAudit
    {
        get
        {
            return bShowSuccessAudit;
        }
        set
        {
            bShowSuccessAudit = value;
        }
    }
    
    public bool ShowFailureAudit
    {
        get
        {
            return bShowFailureAudit;
        }
        set
        {
            bShowFailureAudit = value;
        }
    }
    
    public string EventSource
    {
        get
        {
            return sEventSource;
        }
        set
        {
            sEventSource = value;
        }
    }
    
    public string Category
    {
        get
        {
            return sCategory;
        }
        set
        {
            sCategory = value;
        }
    }
    
    public string CustomFilterString
    {
        get
        {
            return sCustomFilterString;
        }
        set
        {
            sCustomFilterString = value;
        }
    }
    
    public long EventId
    {
        get
        {
            return lEventId;
        }
        set
        {
            lEventId = value;
        }
    }
    
    public string User
    {
        get
        {
            return sUser;
        }
        set
        {
            sUser = value;
        }
    }
    
    public string Computer
    {
        get
        {
            return sComputer;
        }
        set
        {
            sComputer = value;
        }
    }
    
    public DateTime StartDate
    {
        get
        {
            return dtStart;
        }
        set
        {
            dtStart = value;
        }
    }
    
    public DateTime EndDate
    {
        get
        {
            return dtEnd;
        }
        set
        {
            dtEnd = value;
        }
    }
    
    #endregion
    
    #region Public methods
    
    /// <summary>
    /// This method tests whether a particular log entry is acceptable
    /// to the filter instance
    /// </summary>
    /// <param name="ele">The event log entry to test</param>
    /// <returns>True if it passes the filter criteria</returns>
    public bool Acceptable(EventLogEntry ele)
    {
        // test the entry type
        switch (ele.EntryType)
        {
            case EventLogEntryType.Error:
            if (!bShowError)
            {
                return false;
            }
            break;
            
            case EventLogEntryType.Information:
            if (!bShowInformation)
            {
                return false;
            }
            break;
            
            case EventLogEntryType.Warning:
            if (!bShowWarning)
            {
                return false;
            }
            break;
            
            case EventLogEntryType.SuccessAudit:
            if (!bShowSuccessAudit)
            {
                return false;
            }
            break;
            
            case EventLogEntryType.FailureAudit:
            if (!bShowFailureAudit)
            {
                return false;
            }
            break;
        }
        
        // test the source and category
        if (sEventSource != "" && ele.Source != sEventSource)
        {
            return false;
        }
        
        if (sCategory != "" && ele.Category != sCategory)
        {
            return false;
        }
        
        // test the event id
        if (lEventId != -1)
        {
            long lEleEventId = ele.InstanceId & 0x000000000000FFFF;
            if (lEventId != lEleEventId)
            {
                return false;
            }
        }
        
        // test the user and computer
        if (sUser != "" && ele.UserName != sUser)
        {
            return false;
        }
        
        if (sComputer != "" && ele.MachineName != sComputer)
        {
            return false;
        }
        
        // test the event datetime
        if (ele.TimeGenerated < dtStart || ele.TimeGenerated > dtEnd)
        {
            return false;
        }
        
        return true;
    }
    
    #endregion
}
}
