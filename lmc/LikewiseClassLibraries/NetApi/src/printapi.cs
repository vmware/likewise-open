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
using System.Data;
using System.Diagnostics;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using Likewise.LMC.Utilities;


//Use the DebugMarshal for Debug builds, and the standard Marshal in release builds
#if DEBUG
using Marshal = Likewise.LMC.Utilities.DebugMarshal;
#endif

namespace Likewise.LMC.NETAPI
{
/// <summary>
/// Summary description for PrintAPI.
/// </summary>
    public class PrintAPI
    {
        // error handling constants
        protected const int nErrorBadRPC = 1722;
        protected const int nErrorAccessDenied = 5;
        protected const int nErrorRPC_S_CALL_FAILED = 1726;
        protected const string sErrorMsg = "Unable to retrieve printer list from {0}. Error {1}";
        protected const string sBadServerName = "An error occurred while reading the list of printers for this server. ";
    
        /// <summary>
        /// Column names for created tables
        /// TODO: Put these in a resource?
        /// </summary>
        protected const string sCol0 =  "ShareName";
        protected const string sCol1 =     "Name";
        protected const string sCol2 =     "Documents";
        protected const string sCol3 =     "Status";
        protected const string sCol4 =     "Comments";
        protected const string sCol5 =     "Location";
        protected const string sCol6 =  "Model";
      
        protected const string sReady = "Ready";
        protected const string sPaused = "Paused";
        protected const string sError = "Error";
        protected const string sOther = "Other";
    
    const int ERROR_INSUFFICIENT_BUFFER = 122;
    
    protected enum PEFlags
    {
        PRINTER_ENUM_DEFAULT     = 0x00000001,
        PRINTER_ENUM_LOCAL       = 0x00000002,
        PRINTER_ENUM_CONNECTIONS = 0x00000004,
        PRINTER_ENUM_FAVORITE    = 0x00000004,
        PRINTER_ENUM_NAME        = 0x00000008,
        PRINTER_ENUM_REMOTE      = 0x00000010,
        PRINTER_ENUM_SHARED      = 0x00000020,
        PRINTER_ENUM_NETWORK     = 0x00000040
        };
    
    protected enum PRINTER_STATUS
    {
        PRINTER_STATUS_PAUSED            = 0x00000001,
        PRINTER_STATUS_ERROR             = 0x00000002,
        PRINTER_STATUS_PENDING_DELETION  = 0x00000004,
        PRINTER_STATUS_PAPER_JAM         = 0x00000008,
        PRINTER_STATUS_PAPER_OUT         = 0x00000010,
        PRINTER_STATUS_MANUAL_FEED       = 0x00000020,
        PRINTER_STATUS_PAPER_PROBLEM     = 0x00000040,
        PRINTER_STATUS_OFFLINE           = 0x00000080,
        PRINTER_STATUS_IO_ACTIVE         = 0x00000100,
        PRINTER_STATUS_BUSY              = 0x00000200,
        PRINTER_STATUS_PRINTING          = 0x00000400,
        PRINTER_STATUS_OUTPUT_BIN_FULL   = 0x00000800,
        PRINTER_STATUS_NOT_AVAILABLE     = 0x00001000,
        PRINTER_STATUS_WAITING           = 0x00002000,
        PRINTER_STATUS_PROCESSING        = 0x00004000,
        PRINTER_STATUS_INITIALIZING      = 0x00008000,
        PRINTER_STATUS_WARMING_UP        = 0x00010000,
        PRINTER_STATUS_TONER_LOW         = 0x00020000,
        PRINTER_STATUS_NO_TONER          = 0x00040000,
        PRINTER_STATUS_PAGE_PUNT         = 0x00080000,
        PRINTER_STATUS_USER_INTERVENTION = 0x00100000,
        PRINTER_STATUS_OUT_OF_MEMORY     = 0x00200000,
        PRINTER_STATUS_DOOR_OPEN         = 0x00400000,
        PRINTER_STATUS_SERVER_UNKNOWN    = 0x00800000,
        PRINTER_STATUS_POWER_SAVE        = 0x01000000
        };
    
    public const int PRINTER_ALL_ACCESS = 0x000F0000 + 4 + 8;
    
    protected enum DSPRINT_OPTIONS
    {
        DSPRINT_PUBLISH         = 0x00000001,
        DSPRINT_UPDATE          = 0x00000002,
        DSPRINT_UNPUBLISH       = 0x00000004,
        DSPRINT_REPUBLISH       = 0x00000008
        // DSPRINT_PENDING         = 0x80000000
        };
    
    
    [ StructLayout( LayoutKind.Sequential, CharSet=CharSet.Ansi )]
    protected class PRINTER_DEFAULTS
    {
        public IntPtr pDatatype;
        public IntPtr pDevMode;
        public int DesiredAccess;
        };
    
    [ StructLayout( LayoutKind.Sequential, CharSet=CharSet.Ansi )]
    protected class PRINTER_INFO_2
    {
        public IntPtr  pServerName;
        public IntPtr  pPrinterName;
        public IntPtr  pShareName;
        public IntPtr  pPortName;
        public IntPtr  pDriverName;
        public IntPtr  pComment;
        public IntPtr  pLocation;
        public IntPtr  pDevMode;
        public IntPtr  pSepFile;
        public IntPtr  pPrintProcessor;
        public IntPtr  pDatatype;
        public IntPtr  pParameters;
        public IntPtr  pSecurityDescriptor;
        public int     Attributes;
        public int     Priority;
        public int     DefaultPriority;
        public int     StartTime;
        public int     UntilTime;
        public int     Status;
        public int     cJobs;
        public int     AveragePPM;
        };
    
    [ StructLayout( LayoutKind.Sequential, CharSet=CharSet.Ansi )]
    protected class PRINTER_INFO_7
    {
        public IntPtr  pszObjectGUID;
        public uint    dwAction;
        };
    
    [DllImport("winspool.drv", CharSet=CharSet.Unicode, SetLastError=true)]
    protected extern static bool EnumPrinters(    int dwFlags,
    string Name,
    int Level,
    IntPtr pPrinterEnum,
    int cBuf,
    ref int cBufNeeded,
    ref int cReturned);
    
    [DllImport("winspool.drv", CharSet=CharSet.Unicode, SetLastError=true)]
    protected extern static bool OpenPrinter(    string sPrinterName,
    out int hPrinter,
    IntPtr pDefault);
    
    
    [DllImport("winspool.drv", CharSet=CharSet.Unicode, SetLastError=true)]
    protected extern static bool DeletePrinter( int hPrinter );
    
    [DllImport("winspool.drv", CharSet=CharSet.Unicode, SetLastError=true)]
    protected extern static bool SetPrinter( int hPrinter, int nLevel, IntPtr pData, int nCommand );
    
    /// <summary>
    /// Fetches data about the printers available on the designated server
    /// </summary>
    /// <param name="sServer"></param>
    /// <param name="dt">The DataTable into which the data should be placed. dt MUST have been created properly with CreateDataTable</param>
    public static Dictionary<int, string[]> FetchPrinterData(CredentialEntry ce, string sServer)
    {
        int dwFlags = (int)PEFlags.PRINTER_ENUM_NAME;
        string Name = Canon(sServer);
        int Level = 2;
        IntPtr pPrinterEnum = IntPtr.Zero;
        int cBufNeeded = 0;
        int cReturned = 0;
        
        if (!Session.EnsureNullSession(sServer, ce))
        {
            return null;
        }
        
        Dictionary<int, string[]> printInfoList = new Dictionary<int, string[]>();
        
        ///
        /// TODO: revise. Samba 3.0.13 fails if we pass in a null buffer. To avoid this
        /// pass in a pointer to a zero length buffer!
        ///
        
        #if SAMBAFIXED
        if (EnumPrinters( dwFlags, Name, Level, pPrinterEnum, 0, ref cBufNeeded, ref cReturned))
        {
            // Can only happen if no printers available!
            return;
        }
        #else
        {
            
        }
        pPrinterEnum = Marshal.AllocHGlobal(0);
        if (EnumPrinters(dwFlags, Name, Level, pPrinterEnum, 0, ref cBufNeeded, ref cReturned))
        {
            // free the mem
            Marshal.FreeHGlobal(pPrinterEnum);
            
            // Can only happen if no printers available!
            return null;
        }
        // free the mem
        Marshal.FreeHGlobal(pPrinterEnum);
        #endif
        
        // Normally, we fail because we need a bigger buffer. However, if we fail
        // with RPC_S_CALL_FAILED, this means that we "succeeded" but that the remote
        // machine has nothing to tell us
        int nError = Marshal.GetLastWin32Error();
        
        if (nError == nErrorRPC_S_CALL_FAILED)
        {
            return null;
        }
        
        if (nError != ERROR_INSUFFICIENT_BUFFER)
        {
            // Any other error results in an exception
            string sErr;
            if (nError == nErrorBadRPC)
            {
                sErr = sBadServerName;
            }
            else if (nError == nErrorAccessDenied)
            {
                throw new AuthException(new Win32Exception(nError));
            }
            else
            {
                sErr = string.Format(sErrorMsg, sServer, nError);
            }
            throw new Win32Exception(sErr);
        }
        
        // allocate a big enough buffer
        pPrinterEnum = Marshal.AllocHGlobal(cBufNeeded);
        
        // retry
        if (!EnumPrinters(dwFlags, Name, Level, pPrinterEnum, cBufNeeded, ref cBufNeeded, ref cReturned))
        {
            nError = Marshal.GetLastWin32Error();
            Win32Exception we = new Win32Exception(nError);
            if (nError == nErrorAccessDenied)
            {
                throw new AuthException(we);
            }
            else
            {
                throw we;
            }
        }
        
        // iterate through the retrieved entries, copying printer entries
        // to the output table
        IntPtr pCur = pPrinterEnum;
        for (int i = 0; i < cReturned; i++)
        {
            // marshal the entry into
            PRINTER_INFO_2 pi2 = new PRINTER_INFO_2();
            Marshal.PtrToStructure(pCur, pi2);
            
            // trim off server name
            string sName = Marshal.PtrToStringUni(pi2.pPrinterName);
            if (sName.StartsWith(@"\\"))
            {
                // yep, trim off prefix
                int ibackwhack = sName.LastIndexOf(@"\");
                sName = sName.Substring(ibackwhack + 1);
            }
            
            string[] printInfo ={ Marshal.PtrToStringUni(pi2.pShareName), Marshal.PtrToStringUni(pi2.pDriverName), pi2.cJobs.ToString(), Marshal.PtrToStringUni(pi2.pComment) };
            
            // advance to the next entry
            pCur = (IntPtr)((int)pCur + Marshal.SizeOf(pi2));
            
            printInfoList.Add(i, printInfo);
        }
        
        // free the mem
        Marshal.FreeHGlobal(pPrinterEnum);
        
        return printInfoList;
    }
    
    public static Process RunAddPrinterWizard(CredentialEntry ce, string sServer)
    {
        Session.EnsureNullSession(sServer, ce);
        return ProcessUtil.Exec(Environment.SystemDirectory, "rundll32.exe", @"printui.dll,PrintUIEntry /c\\" + sServer + " /il");
    }
    
    public static Process RunAddDriverWizard(CredentialEntry ce, string sServer)
    {
        Session.EnsureNullSession(sServer, ce);
        return ProcessUtil.Exec(Environment.SystemDirectory, "rundll32.exe", @"printui.dll,PrintUIEntry /c\\" + sServer + " /id");
        
    }
    
    public static Process ShowProperties(CredentialEntry ce, string sServer, string sShare)
    {
        Session.EnsureNullSession(sServer, ce);
        return ProcessUtil.Exec(Environment.SystemDirectory, "rundll32.exe", string.Format(@"printui.dll,PrintUIEntry /p /n ""\\{0}\{1}""", sServer, sShare));
    }
    
    public static Process ShowManageUI(CredentialEntry ce, string sServer, string sShare)
    {
        Session.EnsureNullSession(sServer, ce);
        return ProcessUtil.Exec(Environment.SystemDirectory, "rundll32.exe", string.Format(@"printui.dll,PrintUIEntry /o /n ""\\{0}\{1}""", sServer, sShare));
    }
    
    protected static string MapPrinterStatus(int status)
    {
        if (status==0)
        {
            return sReady;
        }
        
        if ((status & (int)PRINTER_STATUS.PRINTER_STATUS_PAUSED)!=0)
        {
            return sPaused;
        }
        
        if ((status & (int)PRINTER_STATUS.PRINTER_STATUS_ERROR)!=0)
        {
            return sError;
        }
        
        return sOther;
    }
    
    /// <summary>
    /// Deletes a printer from the designated server. Must have administrator access to it.
    /// </summary>
    /// <param name="sServer"></param>
    /// <param name="sPrinter"></param>
    public static void DeletePrinter(CredentialEntry ce, string sServer, string sPrinter)
    {
        Session.EnsureNullSession(sServer, ce);
        
        // first, open the printer
        string s = Canon(sServer) + @"\" + sPrinter;
        int hPrinter;
        PRINTER_DEFAULTS pd = new PRINTER_DEFAULTS();
        pd.pDatatype = IntPtr.Zero;
        pd.pDevMode = IntPtr.Zero;
        pd.DesiredAccess = PRINTER_ALL_ACCESS;
        IntPtr p = Marshal.AllocHGlobal(Marshal.SizeOf(pd));
        Marshal.StructureToPtr(pd, p, false);
        if (!OpenPrinter(s, out hPrinter, p))
        {
            // free memory
            Marshal.DestroyStructure(p, typeof(PRINTER_DEFAULTS));
            Marshal.FreeHGlobal(p);
            
            int nError = Marshal.GetLastWin32Error();
            Win32Exception we = new Win32Exception(nError);
            if (nError == nErrorAccessDenied)
            {
                throw new AuthException(we);
            }
            else
            {
                throw we;
            }
        }
        
        Marshal.DestroyStructure(p, typeof(PRINTER_DEFAULTS));
        Marshal.FreeHGlobal(p);
        
        // now remove it from AD
        PRINTER_INFO_7 pi7 = new PRINTER_INFO_7();
        pi7.pszObjectGUID = IntPtr.Zero;
        pi7.dwAction = (uint) DSPRINT_OPTIONS.DSPRINT_UNPUBLISH;
        p = Marshal.AllocHGlobal(Marshal.SizeOf(pi7));
        Marshal.StructureToPtr(pi7, p, false);
        SetPrinter(hPrinter, 7, p, 0);
        
        // NOTE: we swallow any error since the operation may be left in a "pending" state rather
        // than returning success or failure right away.
        
        // free memory
        Marshal.DestroyStructure(p, typeof(PRINTER_INFO_7));
        Marshal.FreeHGlobal(p);
        
        // finally, remove the printer
        if (!DeletePrinter(hPrinter))
        {
            int nError = Marshal.GetLastWin32Error();
            Win32Exception we = new Win32Exception(nError);
            if (nError == nErrorAccessDenied)
            {
                throw new AuthException(we);
            }
            else
            {
                throw we;
            }
        }
    }
    
    private static string Canon(string sHostname)
    {
        return @"\\" + sHostname;
    }
}
}
