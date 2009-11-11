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
using System.Text;
using System.Management;
using Likewise.LMC.Utilities;
using System.ServiceProcess;
using System.Runtime.InteropServices;

namespace Likewise.LMC.Services
{
    public class ServiceInfo
    {
        public string serviceName = string.Empty;
        public bool IsDisabled = false;
        public bool IsRunning = false;
        public bool IsPaused = false;
        public bool IsAcceptStop = false;
        public bool IsAcceptPause = false;
    }    
    
    public class ServiceManagerWindowsWrapper
    {
        #region WMI Api wrapper implementations

        public static IntPtr phSCManager = IntPtr.Zero;
        public static IntPtr phService = IntPtr.Zero;

        public static Dictionary<string, string[]> EnumManagementServices()
        {
            string sHostname = System.Environment.MachineName;

            string QueryString = "SELECT * FROM Win32_Service";
            string[] serviceInfo = null;
            Dictionary<string, string[]> services = new Dictionary<string, string[]>();
            ManagementScope managementScope = null;

            Logger.Log(string.Format("ServiceManagerWindowsWrapper.EnumManagementServices(sHostname={0})", sHostname), Logger.ServiceManagerLoglevel);

            try
            {
                //Point to machine
                managementScope = new ManagementScope("\\\\" + sHostname + "\\root\\cimv2", new ConnectionOptions());

                //Query remote computer across the connection
                ObjectQuery objectQuery = new ObjectQuery(QueryString);
                ManagementObjectSearcher mgtQueryCollection = new ManagementObjectSearcher(managementScope, objectQuery);

                foreach (ManagementObject MObj in mgtQueryCollection.Get())
                {
                    serviceInfo = new string[5];
                    try
                    {
                        serviceInfo[0] = MObj["Caption"].ToString();
                        serviceInfo[1] = MObj["Description"].ToString();

                        if (MObj["State"].Equals("Paused"))
                            serviceInfo[2] = "Paused";
                        else if (MObj["Started"].Equals(true))
                            serviceInfo[2] = "Started";
                        else
                            serviceInfo[2] = string.Empty;

                        if (MObj["StartMode"].ToString().Equals("Auto"))
                            serviceInfo[3] = "Automatic";
                        else
                            serviceInfo[3] = MObj["StartMode"].ToString();

                        string startName = MObj["StartName"].ToString();
                        serviceInfo[4] = startName.IndexOf("\\") >= 0 ?
                                         startName.Substring(startName.IndexOf("\\") + 1) : startName;

                        services.Add(MObj["Name"].ToString(), serviceInfo);
                    }
                    catch { }
                }
            }
            catch (Exception ex)
            {
                services = null;
                Logger.LogException("ServiceManagerWindowsWrapper.EnumManagementServices()", ex);
            }

            return services;
        }

        public static ManagementObjectCollection GetServiceCollection(string sServiceName)
        {
            Logger.Log(string.Format("ServiceManagerWindowsWrapper.GetServiceCollection(sServiceName={0})", sServiceName), Logger.ServiceManagerLoglevel);

            ManagementScope managementScope = null;
            ManagementObjectCollection queryCollection = null;
            string QueryString = "SELECT * FROM Win32_Service Where Name = '" + sServiceName + "'";

            try
            {
                //Point to machine
                managementScope = new ManagementScope("\\\\" + System.Environment.MachineName + "\\root\\cimv2", new ConnectionOptions());

                //Query remote computer across the connection
                ObjectQuery objectQuery = new ObjectQuery(QueryString);
                ManagementObjectSearcher query = new ManagementObjectSearcher(managementScope, objectQuery);

                queryCollection = query.Get();
            }
            catch (Exception ex)
            {
                Logger.LogException("ServiceManagerWindowsWrapper.GetServiceCollection()", ex);
            }

            return queryCollection;
        }

        public static ServiceInfo GetServiceStateInfo(string sServiceName)
        {
            Logger.Log(string.Format("ServiceManagerWindowsWrapper.GetServiceStateInfo(sServiceName={0})", sServiceName), Logger.ServiceManagerLoglevel);
            ServiceInfo serviceInfo = new ServiceInfo();

            try
            {
                ManagementObjectCollection queryCollection = GetServiceCollection(sServiceName);

                if (queryCollection != null)
                {
                    serviceInfo.serviceName = sServiceName;
                    foreach (ManagementObject mo in queryCollection)
                    {
                        serviceInfo.IsRunning = (bool)mo["Started"];
                        serviceInfo.IsAcceptStop = (bool)mo["AcceptStop"];
                        serviceInfo.IsAcceptPause = (bool)mo["AcceptPause"];
                        serviceInfo.IsDisabled = string.Equals(mo["StartMode"].ToString(), "Disabled");
                        serviceInfo.IsPaused = string.Equals(mo["State"].ToString(), "Paused");
                        break;
                    }
                }
            }
            catch (Exception ex)
            {
                Logger.LogException("ServiceManagerWindowsWrapper.GetServiceCollection()", ex);
            }

            return serviceInfo;
        }

        public static bool InvokeWMIServiceMethod(string serviceAction, string sServiceName)
        {
            bool bInvokeMethodStatus = false;

            try
            {
                //Set up a handler for the asynchronous callback
                ManagementOperationObserver observer = new ManagementOperationObserver();
                ServiceHandler completionHandlerObj = new ServiceHandler();
                observer.ObjectReady += new ObjectReadyEventHandler(completionHandlerObj.Done);

                //get specific service object
                ManagementObjectCollection queryCollection = GetServiceCollection(sServiceName);

                foreach (ManagementObject mo in queryCollection)
                {
                    //start or stop service
                    mo.InvokeMethod(observer, serviceAction, null);
                }

                //wait until invoke method is complete or 5 sec timeout
                int intCount = 0;
                while (!completionHandlerObj.IsComplete)
                {
                    if (intCount > 10)
                    {
                        Logger.Log("InvokeWMIServiceMethod():Terminate process timed out.", Logger.ServiceManagerLoglevel);
                        break;
                    }
                    //wait 1/2 sec.
                    System.Threading.Thread.Sleep(500);

                    //increment counter
                    intCount++;
                }

                //see if call was successful.
                if (completionHandlerObj.ReturnObject.Properties["returnValue"].Value.ToString() == "0")
                    bInvokeMethodStatus = true;
            }
            catch (Exception ex)
            {
                Logger.LogException("ServiceManagerWindowsWrapper.InvokeWMIServiceMethod()", ex);
            }

            return bInvokeMethodStatus;
        }

        public static void WMIServiceRestart(string sServiceName)
        {
            try
            {
                ServiceController sc = new ServiceController(sServiceName);
                if (sc != null)
                {
                    sc.Start();
                    sc.WaitForStatus(ServiceControllerStatus.Running);
                }
            }
            catch (Exception ex)
            {
                Logger.LogException("ServiceManagerWindowsWrapper.WMIServiceRestart()", ex);
            }
        }

        #endregion

        #region ADVAPI Wrapper implementations

        public static int ApiOpnSCManager()
        {
            string sHostname = System.Environment.MachineName;

            Logger.Log(string.Format("ServiceManagerWindowsWrapper.ApiOpnSCManager(sHostname={0})", sHostname), Logger.ServiceManagerLoglevel);

            try
            {
                if (phSCManager == IntPtr.Zero)
                {
                    phSCManager = ServiceManagerInteropWindows.OpenSCManager(sHostname, null, (uint)ServiceManagerInteropWindows.SC_MANAGER_ALL_ACCESS);
                }

                if (phSCManager == IntPtr.Zero)
                {
                    Logger.Log("ServiceManagerWindowsWrapper.ApiOpnSCManager returns invalid handle", Logger.ServiceManagerLoglevel);
                }
                Logger.Log("ServiceManagerWindowsWrapper.ApiOpnSCManager is Success", Logger.ServiceManagerLoglevel);
            }
            catch (Exception ex)
            {
                phSCManager = IntPtr.Zero;
                Logger.LogException("ServiceManagerWindowsWrapper.ApiOpnSCManager()", ex);
            }

            return phSCManager != IntPtr.Zero ? 0 : -1;
        }

        public static IntPtr ApiOpenService(string sServicename, uint dwDesiredAccess)
        {
            try
            {
                if (phSCManager == IntPtr.Zero)
                    ApiOpnSCManager();

                Logger.Log(string.Format("ServiceManagerWindowsWrapper.ApiOpenService(sHostname={0}, sServicename={1})", System.Environment.MachineName, sServicename), Logger.ServiceManagerLoglevel);

                if (phService != IntPtr.Zero)
                    ApiCloseServiceHandle(phService);

                phService = ServiceManagerInteropWindows.OpenService(phSCManager, sServicename, dwDesiredAccess);

                if (phService == IntPtr.Zero)
                {
                    Logger.Log("ServiceManagerWindowsWrapper.ApiOpenService returns invalid handle", Logger.ServiceManagerLoglevel);
                }
                Logger.Log("ServiceManagerWindowsWrapper.ApiOpenService is Success", Logger.ServiceManagerLoglevel);
            }
            catch (Exception ex)
            {
                phService = IntPtr.Zero;
                Logger.LogException("ServiceManagerWindowsWrapper.ApiOpenService()", ex);
            }

            return phService;
        }

        public static bool ApiCloseServiceHandle(IntPtr pServiceHandle)
        {
            bool bRet = false;

            try
            {
                Logger.Log(string.Format("ServiceManagerWindowsWrapper.ApiCloseServiceHandle()"), Logger.ServiceManagerLoglevel);

                bRet = ServiceManagerInteropWindows.CloseServiceHandle(pServiceHandle);

                if (pServiceHandle != IntPtr.Zero)
                {
                    Logger.Log("ServiceManagerWindowsWrapper.ApiCloseServiceHandle returns invalid handle", Logger.ServiceManagerLoglevel);
                }

                phService = IntPtr.Zero;
                Logger.Log("ServiceManagerWindowsWrapper.ApiCloseServiceHandle is Success", Logger.ServiceManagerLoglevel);               
            }
            catch (Exception ex)
            {
                Logger.LogException("ServiceManagerWindowsWrapper.ApiCloseServiceHandle()", ex);
            }

            return bRet;
        }

        public static ServiceManagerApi.SERVICE_FAILURE_ACTIONS ApiQueryServiceConfig2(
                                                                string sServicename)
        {
            UInt32 dwBytesNeeded;
            //UInt32 INFINITE = 0xFFFFFFFF;
            ServiceManagerApi.SERVICE_FAILURE_ACTIONS failureActions = new ServiceManagerApi.SERVICE_FAILURE_ACTIONS();
            try
            {
                ApiOpenService(sServicename, ServiceManagerInteropWindows.SERVICE_QUERY_CONFIG);

                Logger.Log(string.Format("ServiceManagerWindowsWrapper.ApiQueryServiceConfig2(sHostname={0})", sServicename), Logger.ServiceManagerLoglevel);

                bool bSuccess = ServiceManagerInteropWindows.QueryServiceConfig2(phService,
                                                    ServiceManagerInteropWindows.SERVICE_CONFIG_FAILURE_ACTIONS,
                                                    IntPtr.Zero, 0, out dwBytesNeeded);
                if (!bSuccess)
                {
                    Logger.Log("ServiceManagerWindowsWrapper.ApiOpenService returns false", Logger.ServiceManagerLoglevel);
                }

                IntPtr ptr = Marshal.AllocHGlobal((int)dwBytesNeeded);
                bSuccess = ServiceManagerInteropWindows.QueryServiceConfig2(phService,
                                                  ServiceManagerInteropWindows.SERVICE_CONFIG_FAILURE_ACTIONS,
                                                  ptr, dwBytesNeeded, out dwBytesNeeded);

                Marshal.PtrToStructure(ptr, failureActions);

                ApiCloseServiceHandle(phService);

                Logger.Log("ServiceManagerWindowsWrapper.ApiOpnSCManager is Success", Logger.ServiceManagerLoglevel);
            }
            catch (Exception ex)
            {
                Logger.LogException("ServiceManagerWindowsWrapper.ApiOpnSCManager()", ex);
            }

            return failureActions;
        }

        public static bool ApiChangeServiceConfig2(string sServicename,
                                           ServiceManagerApi.SERVICE_FAILURE_ACTIONS failureActions)
        {
            try
            {
                ApiOpenService(sServicename, ServiceManagerInteropWindows.SERVICE_CHANGE_CONFIG);

                IntPtr lpInfo = Marshal.AllocHGlobal(Marshal.SizeOf(failureActions));
                if (lpInfo == IntPtr.Zero)
                {                    
                    throw new Exception(String.Format("Unable to allocate memory, error was: 0x{0:X}", Marshal.GetLastWin32Error()));
                }

                Marshal.StructureToPtr(failureActions, lpInfo, false);

                if (!ServiceManagerInteropWindows.ChangeServiceConfig2(phService, (int)ServiceManagerInteropWindows.SERVICE_CONFIG_FAILURE_ACTIONS, lpInfo))
                {
                    Marshal.FreeHGlobal(lpInfo);                    
                    Logger.Log(String.Format("Error setting service config, error was: 0x{0:X}", Marshal.GetLastWin32Error()));
                    return false;
                }

                Marshal.FreeHGlobal(lpInfo);               

                Logger.Log("MyService: Service modification completed");

                ApiCloseServiceHandle(phService);
            }
            catch (Exception ex)
            {               
                Logger.LogException("ApiChangeServiceConfig2()", ex);
                return false;
            }

            return true;
        }

        #endregion
    }

    internal class ServiceHandler
    {
        private bool isComplete = false;
        private ManagementBaseObject returnObject;

        /// <summary>
        /// Trigger Done event when InvokeMethod is complete
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        public void Done(object sender, ObjectReadyEventArgs e)
        {
            isComplete = true;
            returnObject = e.NewObject;
        }


        /// <summary>
        /// Get property IsComplete
        /// </summary>
        public bool IsComplete
        {
            get
            {
                return isComplete;
            }
        }

        /// <summary>
        /// Property allows accessing the result object in the main function
        /// </summary>
        public ManagementBaseObject ReturnObject
        {
            get
            {
                return returnObject;
            }
        }
    }
}
