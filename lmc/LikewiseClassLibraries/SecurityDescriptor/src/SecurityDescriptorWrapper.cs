using System;
using System.Collections.Generic;
using System.Text;
using System.Diagnostics;
using System.Runtime.InteropServices;
using Likewise.LMC.Utilities;

namespace Likewise.LMC.SecurityDesriptor
{
    public class SecurityDescriptorWrapper
    {
        #region Security Apis wrapper implementations

        public static uint ApiGetNamedSecurityInfo(
                                    string objectName,
                                    SecurityDescriptorApi.SE_OBJECT_TYPE object_type,
                                    out SecurityDescriptorApi.SECURITY_DESCRIPTOR pSECURITY_DESCRIPTOR)
        {
            Logger.Log(string.Format("SecurityDescriptorWrapper.ApiGetNamedSecurityInfo(objectName={0}", objectName), Logger.SecurityDescriptorLogLevel);

            IntPtr pZero = IntPtr.Zero;
            IntPtr pSidOwner = pZero;
            IntPtr pSidGroup = pZero;
            IntPtr pDacl = pZero;
            IntPtr pSacl = pZero;
            IntPtr pSecurityDescriptor = pZero;
            uint errorReturn = 0;
            pSECURITY_DESCRIPTOR = null;

            try
            {
                errorReturn = SecurityDescriptorApi.GetNamedSecurityInfo(
                                    objectName,
                                    object_type,
                                    SecurityDescriptorApi.SECURITY_INFORMATION.OWNER_SECURITY_INFORMATION |
                                    SecurityDescriptorApi.SECURITY_INFORMATION.GROUP_SECURITY_INFORMATION |
                                    SecurityDescriptorApi.SECURITY_INFORMATION.DACL_SECURITY_INFORMATION,
                                    out pSidOwner,
                                    out pSidGroup,
                                    out pDacl,
                                    out pSacl,
                                    out pSecurityDescriptor);

                if (errorReturn != 0)
                {
                    Console.WriteLine("SecurityDescriptorWrapper.ApiGetNamedSecurityInfo() errorReturn: {0} ", errorReturn);
                    return errorReturn;
                }

                if (pSecurityDescriptor != IntPtr.Zero)
                {
                    pSECURITY_DESCRIPTOR = new SecurityDescriptorApi.SECURITY_DESCRIPTOR();
                    Marshal.PtrToStructure(pSecurityDescriptor, pSECURITY_DESCRIPTOR);
                }
            }
            catch (Exception ex)
            {
                Logger.LogException(string.Format("SecurityDescriptorWrapper.ApiGetNamedSecurityInfo(objectName={0}", objectName), ex);
            }

            return errorReturn;
        }

        public static uint ReadSecurityDescriptor(
                                SecurityDescriptorApi.SECURITY_DESCRIPTOR pSECURITY_DESCRIPTOR,
                                SecurityDescriptor ObjSecurityDescriptor)
        {
            Logger.Log(string.Format("SecurityDescriptorWrapper.ReadSecurityDescriptor()"), Logger.SecurityDescriptorLogLevel);

            Dictionary<string, Dictionary<string, string>> SdDacls = new Dictionary<string, Dictionary<string, string>>();
            uint errorReturn = 0;
            ObjSecurityDescriptor = null;

            if (pSECURITY_DESCRIPTOR == null)
                return errorReturn;

            try
            {
                ObjSecurityDescriptor = new SecurityDescriptor();
                ObjSecurityDescriptor.Control = pSECURITY_DESCRIPTOR.control;
                ObjSecurityDescriptor.Owner = Marshal.PtrToStringAuto(pSECURITY_DESCRIPTOR.owner);
                ObjSecurityDescriptor.Revision = pSECURITY_DESCRIPTOR.revision;
                ObjSecurityDescriptor.PrimaryGroupID = Marshal.PtrToStringAuto(pSECURITY_DESCRIPTOR.group);
                ObjSecurityDescriptor.Size = pSECURITY_DESCRIPTOR.size;

                if (pSECURITY_DESCRIPTOR.dacl != IntPtr.Zero)
                {
                    SecurityDescriptorApi.ACL_SIZE_INFORMATION AclSize = new SecurityDescriptorApi.ACL_SIZE_INFORMATION();
                    SecurityDescriptorApi.GetAclInformation(pSECURITY_DESCRIPTOR.dacl, ref AclSize,
                                    ((uint)Marshal.SizeOf(typeof(SecurityDescriptorApi.ACL_SIZE_INFORMATION))),
                                    SecurityDescriptorApi.ACL_INFORMATION_CLASS.AclSizeInformation);

                    for (int idx = 0; idx < AclSize.AceCount; idx++)
                    {
                        IntPtr pAce;
                        IntPtr ptrSid;
                        IntPtr pTrustee;

                        int err = SecurityDescriptorApi.GetAce(pSECURITY_DESCRIPTOR.dacl, idx, out pAce);
                        SecurityDescriptorApi.ACCESS_ALLOWED_ACE ace = (SecurityDescriptorApi.ACCESS_ALLOWED_ACE)Marshal.PtrToStructure(pAce, typeof(SecurityDescriptorApi.ACCESS_ALLOWED_ACE));

                        IntPtr iter = (IntPtr)((int)pAce + (int)Marshal.OffsetOf(typeof(SecurityDescriptorApi.ACCESS_ALLOWED_ACE), "SidStart"));
                        byte[] bSID = null;
                        int size = (int)SecurityDescriptorApi.GetLengthSid(iter);
                        bSID = new byte[size];
                        Marshal.Copy(iter, bSID, 0, size);

                        SecurityDescriptorApi.ConvertSidToStringSid(bSID, out ptrSid);
                        SecurityDescriptorApi.BuildTrusteeWithSid(out pTrustee, ptrSid);
                        SecurityDescriptorApi.TRUSTEE trustee = new SecurityDescriptorApi.TRUSTEE();
                        Marshal.PtrToStructure(pTrustee, trustee);
                        string strSID = Marshal.PtrToStringAuto(ptrSid);

                        Logger.Log("Trustee = " + trustee.ptstrName, Logger.SecurityDescriptorLogLevel);
                        Logger.Log(string.Format("SID={0} : AceType={1}/ AceMask={2}/ AceFlags={3}",
                                            strSID,
                                            ace.Header.AceType.ToString(),
                                            ace.Mask.ToString(),
                                            ace.Header.AceFlags.ToString()), Logger.SecurityDescriptorLogLevel);

                        Dictionary<string, string> AceProperties = new Dictionary<string, string>();
                        AceProperties.Add("Sid", strSID);
                        AceProperties.Add("AceType", ace.Header.AceType.ToString());
                        AceProperties.Add("AceMask", ace.Mask.ToString());
                        AceProperties.Add("AceFlags", ace.Header.AceFlags.ToString());
                        AceProperties.Add("AceSize", ace.Header.AceSize.ToString());

                        SdDacls.Add(trustee.ptstrName, AceProperties);

                        ObjSecurityDescriptor.Descretionary_Access_Control_List = SdDacls;
                    }
                }
            }
            catch(Exception ex)
            {
                Logger.LogException("SecurityDescriptorWrapper.ReadSecurityDescriptor()", ex);
            }

            return errorReturn;
        }

        public static bool ApiAdjustTokenPrivileges(IntPtr pProcessTokenHandle)
        {
            Logger.Log(string.Format("SecurityDescriptorWrapper.ApiAdjustTokenPrivileges()"), Logger.SecurityDescriptorLogLevel);

            bool bIsSuccess = false;
            uint returnLength = 0, IreturnLength = 0;

            try
            {
                SecurityDescriptorApi.TOKEN_PRIVILEGES pPreviousTpStruct = new SecurityDescriptorApi.TOKEN_PRIVILEGES();
                IntPtr pPreviousToken = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(SecurityDescriptorApi.TOKEN_PRIVILEGES)));

                //Call the GetTokenInformation at the first time to get the length of the TOKEN_PRIVILEGES structure.
                bIsSuccess = SecurityDescriptorApi.GetTokenInformation(
                                    pProcessTokenHandle,
                                    SecurityDescriptorApi.TOKEN_INFORMATION_CLASS.TokenPrivileges,
                                    null,
                                    0,
                                    out returnLength);
                Logger.Log("Error at SecurityDescriptorApi.GetTokenInformation: " + Marshal.GetLastWin32Error(), Logger.SecurityDescriptorLogLevel);

                bIsSuccess = SecurityDescriptorApi.GetTokenInformation(
                                    pProcessTokenHandle,
                                    SecurityDescriptorApi.TOKEN_INFORMATION_CLASS.TokenPrivileges,
                                    pPreviousTpStruct,
                                    returnLength,
                                    out IreturnLength);
                Logger.Log("Error at SecurityDescriptorApi.GetTokenInformation: " + Marshal.GetLastWin32Error(), Logger.SecurityDescriptorLogLevel);

                uint privilige = SecurityDescriptorApi.SE_PRIVILEGE_USED_FOR_ACCESS;
                pPreviousTpStruct.Attributes = (int)privilige;
                pPreviousTpStruct.PrivilegeCount = 1;

                bIsSuccess = SecurityDescriptorApi.AdjustTokenPrivileges(
                                    pProcessTokenHandle,
                                    false,
                                    pPreviousTpStruct,
                                    0,
                                    null,
                                    0);
                Logger.Log("Error at SecurityDescriptorApi.AdjustTokenPrivileges: " + Marshal.GetLastWin32Error(), Logger.SecurityDescriptorLogLevel);
                if (!bIsSuccess)
                {
                    Logger.Log(string.Format("SecurityDescriptorWrapper.ApiAdjustTokenPrivileges:bIsSuccess()" + bIsSuccess), Logger.SecurityDescriptorLogLevel);
                    Logger.Log("Error code: " + Marshal.GetLastWin32Error());
                }
            }
            catch(Exception ex)
            {
                Logger.LogException(string.Format("SecurityDescriptorWrapper.ApiAdjustTokenPrivileges()"), ex);
            }
            finally
            {
                if (pProcessTokenHandle != IntPtr.Zero)
                    SecurityDescriptorApi.CloseHandle(pProcessTokenHandle);
            }

            return bIsSuccess;
        }

        public static uint ApiGetCurrentProcessHandle(uint DesiredAccess, out IntPtr pTokenHandle)
        {
            uint errorReturn = 0;
            IntPtr pProcessHandle = IntPtr.Zero;
            pTokenHandle = IntPtr.Zero;

            Logger.Log("SecurityDescriptorWrapper.ApiGetCurrentProcessHandle()", Logger.SecurityDescriptorLogLevel);

            try
            {
                pProcessHandle = Process.GetCurrentProcess().Handle;
                bool bSuccess = SecurityDescriptorApi.OpenProcessToken(pProcessHandle, DesiredAccess, out pTokenHandle); ;
                if (!bSuccess)
                {
                    errorReturn = (uint)Marshal.GetLastWin32Error();
                    Logger.Log("SecurityDescriptorWrapper.ApiGetCurrentProcessHandle() unsuccess with ReturnCode = " + errorReturn, Logger.SecurityDescriptorLogLevel);
                }
            }
            catch (Exception ex)
            {
                errorReturn = (uint)Marshal.GetLastWin32Error();
                Logger.LogException("SecurityDescriptorWrapper.ApiGetCurrentProcessHandle()", ex);
            }

            return errorReturn;
        }

        public static uint ApiGetLogOnUserHandle(
                                string username,
                                string password,
                                string domain,
                                uint LononType,
                                out IntPtr pTokenHandle)
        {
            uint errorReturn = 0;
            IntPtr phToken = IntPtr.Zero;
            pTokenHandle = IntPtr.Zero;

            Logger.Log("SecurityDescriptorWrapper.ApiGetLogOnUserHandle()", Logger.SecurityDescriptorLogLevel);

            try
            {
                bool bResult = SecurityDescriptorApi.LogonUser(
                               username,
                               domain,
                               password,
                               (int)LononType,
                               (int)SecurityDescriptorApi.LogonProvider.LOGON32_PROVIDER_DEFAULT,
                               ref phToken);

                if (!bResult)
                {
                    errorReturn = (uint)Marshal.GetLastWin32Error();
                    Logger.Log("SecurityDescriptorWrapper.ApiGetLogOnUserHandle() unsuccess with ReturnCode = " + errorReturn, Logger.SecurityDescriptorLogLevel);
                }
            }
            catch (Exception ex)
            {
                errorReturn = (uint)Marshal.GetLastWin32Error();
                Logger.LogException("SecurityDescriptorWrapper.ApiGetLogOnUserHandle()", ex);
            }

            return errorReturn;
        }

        #endregion
    }
}
