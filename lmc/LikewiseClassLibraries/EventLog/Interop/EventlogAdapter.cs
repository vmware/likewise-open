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
using System.Runtime.InteropServices;
using Likewise.LMC.Utilities;

namespace Likewise.LMC.Eventlog
{
public class EventlogAdapter
{
    public static EventlogHandle OpenEventlog(string hostname)
    {
        IntPtr pEventLogHandle;
        UInt32 dwError = EventAPI.OpenEventLog(hostname, out pEventLogHandle);

        if (dwError == 0)
        {
            return new EventlogHandle(pEventLogHandle);
        }
        else
        {

            throw new Exception(String.Format(
            "Error: OpenEventLog [Code:{0}]", dwError));
        }
    }

    public static EventLogRecord[] ReadEventLog(EventlogHandle handle,
    UInt32 dwLastRecordId,
    UInt32 nMaxRecords,
    string sqlQuery)
    {

        EventLogRecord[] result = null;
        EventAPI.EventLogRecord[] records = null; // new EventAPI.EventLogRecord[nMaxRecords];

        UInt32 nRecordsReturned = 0;
        UInt32 dwError =
        EventAPI.ReadEventLog(handle.Handle, dwLastRecordId, nMaxRecords, sqlQuery,
        out nRecordsReturned,  out records);
        if (dwError != 0)
        {
            Logger.Log(String.Format("Error: ReadEventLog [Code:{0}]", dwError), Logger.eventLogLogLevel);
        }
        if (nRecordsReturned > 0)
        {
            result = new EventLogRecord[nRecordsReturned];
            int iRecord = 0;
            foreach (EventAPI.EventLogRecord record in records)
            {
                result[iRecord++] = new EventLogRecord(record);
            }
        }
        return result;

    }


    public static UInt32 CountLogs(EventlogHandle handle,
    string sqlQuery)
    {

        UInt32 nRecordsMatched = 0;

        UInt32 dwError =
        EventAPI.CountEventLog(handle.Handle, sqlQuery, out nRecordsMatched);
        if (dwError != 0)
        {
            Logger.Log(String.Format("Error: CountEventLog [Code:{0}]", dwError), Logger.eventLogLogLevel);
        }
        return nRecordsMatched;

    }

    public static UInt32 GetCategoryCount(EventlogHandle handle)
    {

        UInt32 pdwNumMatched = 0;

        UInt32 dwError =
               EventAPI.GetCategoryCount(handle.Handle, out pdwNumMatched);
        if (dwError != 0)
        {
            Logger.Log(String.Format("Error: GetCategoryCount [Code:{0}]", dwError), Logger.eventLogLogLevel);
        }
        return pdwNumMatched;

    }

    public static string[] GetDistinctCategories(EventlogHandle handle,
                                                 UInt32 pdwNumMatched)
    {
        string[] EventCategories = null;

        UInt32 dwError =
               EventAPI.GetDistinctCategories(handle.Handle, pdwNumMatched, out EventCategories);
        if (dwError != 0)
        {
            Logger.Log(String.Format("Error: GetDistinctCategories [Code:{0}]", dwError), Logger.eventLogLogLevel);
        }

        return EventCategories;
    }

    public static UInt32 DeleteFromEventLog(EventlogHandle handle,
                                                string sqlfilter)
    {
        UInt32 dwError =
               EventAPI.DeleteFromEventLog(handle.Handle, sqlfilter);
        if (dwError != 0)
        {
            Logger.Log(String.Format("Error: GetDistinctCategories [Code:{0}]", dwError), Logger.eventLogLogLevel);
        }

        return dwError;
    }


}
}
