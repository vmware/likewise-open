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
                                IntPtr pSECURITY_DESCRIPTOR,
                                ref SecurityDescriptor ObjSecurityDescriptor)
        {
            Logger.Log(string.Format("SecurityDescriptorWrapper.ReadSecurityDescriptor()"), Logger.SecurityDescriptorLogLevel);

            Dictionary<string, Dictionary<string, string>> SdDacls = new Dictionary<string, Dictionary<string, string>>();
            IntPtr ptrSid;
            uint errorReturn = 0;
            bool bRet = false;
            ObjSecurityDescriptor = new SecurityDescriptor();

            SecurityDescriptorApi.SECURITY_DESCRIPTOR sSECURITY_DESCRIPTOR = new SecurityDescriptorApi.SECURITY_DESCRIPTOR();

            try
            {
                if (pSECURITY_DESCRIPTOR != IntPtr.Zero)
                {
                    IntPtr pDaclOffset;
                    bool lpbDaclPresent = false;
                    bool lpbDaclDefaulted = false;

                    bRet = SecurityDescriptorApi.GetSecurityDescriptorDacl(pSECURITY_DESCRIPTOR, out lpbDaclPresent, out pDaclOffset, out lpbDaclDefaulted);
                    Logger.Log("SecurityDescriptorApi.GetSecurityDescriptorDacl iRet value", Logger.SecurityDescriptorLogLevel);

                    SecurityDescriptorApi.ACL_SIZE_INFORMATION AclSize = new SecurityDescriptorApi.ACL_SIZE_INFORMATION();
                    SecurityDescriptorApi.GetAclInformation(pDaclOffset, AclSize,
                                    ((uint)Marshal.SizeOf(typeof(SecurityDescriptorApi.ACL_SIZE_INFORMATION))),
                                    SecurityDescriptorApi.ACL_INFORMATION_CLASS.AclSizeInformation);

                    for (int idx = 0; idx < AclSize.AceCount; idx++)
                    {
                        IntPtr pAce;
                        string sUsername, sDomain;

                        int err = SecurityDescriptorApi.GetAce(pDaclOffset, idx, out pAce);
                        SecurityDescriptorApi.ACCESS_ALLOWED_ACE ace = (SecurityDescriptorApi.ACCESS_ALLOWED_ACE)Marshal.PtrToStructure(pAce, typeof(SecurityDescriptorApi.ACCESS_ALLOWED_ACE));

                        IntPtr iter = (IntPtr)((int)pAce + (int)Marshal.OffsetOf(typeof(SecurityDescriptorApi.ACCESS_ALLOWED_ACE), "SidStart"));
                        string strSID = GetObjectStringSid(iter);

                        //Commented this, to use it in feature
                        //IntPtr pTrustee = IntPtr.Zero;
                        //SecurityDescriptorApi.BuildTrusteeWithSid(out pTrustee, ptrSid);
                        //SecurityDescriptorApi.TRUSTEE trustee = new SecurityDescriptorApi.TRUSTEE();
                        //Marshal.PtrToStructure(pTrustee, trustee);

                        GetObjectLookUpName(iter, out sUsername, out sDomain);

                        Logger.Log("Trustee = " + sUsername, Logger.SecurityDescriptorLogLevel);
                        Logger.Log(string.Format("SID={0} : AceType={1}/ AceMask={2}/ AceFlags={3}",
                                            strSID,
                                            ace.Header.AceType.ToString(),
                                            ace.Mask.ToString(),
                                            ace.Header.AceFlags.ToString()), Logger.SecurityDescriptorLogLevel);

                        Dictionary<string, string> AceProperties = new Dictionary<string, string>();
                        AceProperties.Add("Username", sUsername + "(" + sUsername + "@" + sDomain + ")");
                        AceProperties.Add("Sid", strSID);
                        AceProperties.Add("AceType", ace.Header.AceType.ToString());
                        AceProperties.Add("AceMask", ace.Mask.ToString());
                        AceProperties.Add("AceFlags", ace.Header.AceFlags.ToString());
                        AceProperties.Add("AceSize", ace.Header.AceSize.ToString());

                        if (!SdDacls.ContainsKey(sUsername))
                            SdDacls.Add(sUsername, AceProperties);
                    }
                    ObjSecurityDescriptor.Descretionary_Access_Control_List = SdDacls;
                }

                Marshal.PtrToStructure(pSECURITY_DESCRIPTOR, sSECURITY_DESCRIPTOR);

                //Get Security Descriptor Control
                SecurityDescriptorApi.SECURITY_DESCRIPTOR_CONTROL pControl;
                uint dwRevision;
                SecurityDescriptorApi.GetSecurityDescriptorControl(pSECURITY_DESCRIPTOR, out pControl, out dwRevision);
                ObjSecurityDescriptor.Control = (uint)pControl;
                ObjSecurityDescriptor.Revision = dwRevision;

                //Get Security Descriptor Owner
                ptrSid = IntPtr.Zero;
                bool lpbOwnerDefaulted = false;
                bRet = SecurityDescriptorApi.GetSecurityDescriptorOwner(pSECURITY_DESCRIPTOR, out ptrSid, out lpbOwnerDefaulted);
                Logger.Log("SecurityDescriptorApi.GetSecurityDescriptorOwner iRet value: " + Marshal.GetLastWin32Error());
                ObjSecurityDescriptor.Owner = GetObjectStringSid(ptrSid);

                //Get Security Descriptor Group
                bool lpbGroupDefaulted = false;
                ptrSid = IntPtr.Zero;
                bRet = SecurityDescriptorApi.GetSecurityDescriptorGroup(pSECURITY_DESCRIPTOR, out ptrSid, out lpbGroupDefaulted);
                Logger.Log("SecurityDescriptorApi.GetSecurityDescriptorGroup iRet value: " + Marshal.GetLastWin32Error());
                ObjSecurityDescriptor.PrimaryGroupID = GetObjectStringSid(ptrSid);

                ObjSecurityDescriptor.Size = SecurityDescriptorApi.GetSecurityDescriptorLength(pSECURITY_DESCRIPTOR);
            }
            catch (Exception ex)
            {
                Logger.LogException("SecurityDescriptorWrapper.ReadSecurityDescriptor()", ex);
            }

            return errorReturn;
        }

        public static int ApiLookupAccountSid(
                                    byte[] sid,
                                    ref string sUsername,
                                    ref string sDomainname)
        {
            Logger.Log("SecurityDescriptorWrapper.ApiLookupAccountSid()");

            SecurityDescriptorApi.SID_NAME_USE sidUse;
            StringBuilder name = new StringBuilder();
            StringBuilder referencedDomainName = new StringBuilder();
            uint cchName = (uint)name.Capacity;
            uint cchReferencedDomainName = (uint)referencedDomainName.Capacity;
            int iReturn = 0;
            int ERROR_INSUFFICIENT_BUFFER = 122;

            try
            {
                bool bReturn = SecurityDescriptorApi.LookupAccountSid(
                                    null,
                                    sid,
                                    name,
                                    ref cchName,
                                    referencedDomainName,
                                    ref cchReferencedDomainName,
                                    out sidUse);

                iReturn = Marshal.GetLastWin32Error();
                if (iReturn == ERROR_INSUFFICIENT_BUFFER)
                {
                    bReturn = SecurityDescriptorApi.LookupAccountSid(
                                null,
                                sid,
                                name,
                                ref cchName,
                                referencedDomainName,
                                ref cchReferencedDomainName,
                                out sidUse);

                    iReturn = Marshal.GetLastWin32Error();
                }
                if (bReturn)
                {
                    sUsername = name.ToString();
                    sDomainname = referencedDomainName.ToString();
                }
            }
            catch (Exception ex)
            {
                Logger.LogException("SecurityDescriptorWrapper.ApiLookupAccountSid()", ex);
            }

            return iReturn;
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

                uint privilige = SecurityDescriptorApi.SE_PRIVILEGE_ENABLED;
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
                //if (pProcessTokenHandle != IntPtr.Zero)
                //    SecurityDescriptorApi.CloseHandle(pProcessTokenHandle);
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

        #region Helper functions

        public static string GetObjectStringSid(IntPtr pByteSid)
        {
            string strSID = string.Empty;
            try
            {
                IntPtr ptrSid = IntPtr.Zero;

                int size = (int)SecurityDescriptorApi.GetLengthSid(pByteSid);
                byte[] bSID = new byte[size];
                Marshal.Copy(pByteSid, bSID, 0, size);
                SecurityDescriptorApi.ConvertSidToStringSid(bSID, out ptrSid);
                strSID = Marshal.PtrToStringAuto(ptrSid);
            }
            catch (Exception ex)
            {
                Logger.LogException("SecurityDescriptorWrapper.GetObjectStringSid()", ex);
            }

            return strSID;
        }

        public static void GetObjectLookUpName(
                        IntPtr ptrSid,
                        out string Username,
                        out string Domain)
        {
            Username = string.Empty;
            Domain = string.Empty;

            try
            {
                int size = (int)SecurityDescriptorApi.GetLengthSid(ptrSid);
                byte[] bSID = new byte[size];
                Marshal.Copy(ptrSid, bSID, 0, size);

                ApiLookupAccountSid(bSID, ref Username, ref Domain);
            }
            catch (Exception ex)
            {
                Logger.LogException("SecurityDescriptorWrapper.GetObjectLookUpName()", ex);
            }
        }

        public static string ConvetByteSidToStringSid(byte[] bSid)
        {
            string strSID = string.Empty;

            try
            {
                IntPtr ptrSid;
                SecurityDescriptorApi.ConvertSidToStringSid(bSid, out ptrSid);
                strSID = Marshal.PtrToStringAuto(ptrSid);
            }
            catch (Exception ex)
            {
                Logger.LogException("SecurityDescriptorWrapper.ConvetByteSidToStringSid()", ex);
            }

            return strSID;
        }

        public static string GetAceTypes(byte[] bSid)
        {
            string strSID = string.Empty;

            try
            {
                IntPtr ptrSid;
                SecurityDescriptorApi.ConvertSidToStringSid(bSid, out ptrSid);
                strSID = Marshal.PtrToStringAuto(ptrSid);
            }
            catch (Exception ex)
            {
                Logger.LogException("SecurityDescriptorWrapper.ConvetByteSidToStringSid()", ex);
            }

            return strSID;
        }


        #endregion
    }
}
