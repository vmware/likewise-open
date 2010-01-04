using System;
using System.Collections.Generic;
using System.Text;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Threading;
using Likewise.LMC.Utilities;

////Use the DebugMarshal for Debug builds, and the standard Marshal in release builds
//#if DEBUG
//using Marshal = Likewise.LMC.Utilities.DebugMarshal;
//#endif

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
            pSECURITY_DESCRIPTOR = new SecurityDescriptorApi.SECURITY_DESCRIPTOR();

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
                    pSECURITY_DESCRIPTOR = (SecurityDescriptorApi.SECURITY_DESCRIPTOR)Marshal.PtrToStructure(pSecurityDescriptor, typeof(SecurityDescriptorApi.SECURITY_DESCRIPTOR));
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

            Dictionary<string, List<LwAccessControlEntry>> SdDacls = null;
            IntPtr ptrSid;
            uint errorReturn = 0;
            bool bRet = false;
            ObjSecurityDescriptor = new SecurityDescriptor();
            ObjSecurityDescriptor.InitailizeToNull();

            SecurityDescriptorApi.SECURITY_DESCRIPTOR sSECURITY_DESCRIPTOR = new SecurityDescriptorApi.SECURITY_DESCRIPTOR();

            try
            {
                if (pSECURITY_DESCRIPTOR != IntPtr.Zero)
                {
                    SdDacls = new Dictionary<string, List<LwAccessControlEntry>>();
                    IntPtr pDaclOffset;
                    bool lpbDaclPresent = false;
                    bool lpbDaclDefaulted = false;

                    bRet = SecurityDescriptorApi.GetSecurityDescriptorDacl(pSECURITY_DESCRIPTOR, out lpbDaclPresent, out pDaclOffset, out lpbDaclDefaulted);
                    Logger.Log("SecurityDescriptorApi.GetSecurityDescriptorDacl iRet value", Logger.SecurityDescriptorLogLevel);

                    SecurityDescriptorApi.ACL_SIZE_INFORMATION AclSize = new SecurityDescriptorApi.ACL_SIZE_INFORMATION();
                    SecurityDescriptorApi.GetAclInformation(pDaclOffset, AclSize,
                                    ((uint)Marshal.SizeOf(typeof(SecurityDescriptorApi.ACL_SIZE_INFORMATION))),
                                    SecurityDescriptorApi.ACL_INFORMATION_CLASS.AclSizeInformation);

                    if (pDaclOffset != IntPtr.Zero)
                    {
                        SdDacls = new Dictionary<string, List<LwAccessControlEntry>>();
                        List<LwAccessControlEntry> daclInfo = new List<LwAccessControlEntry>();
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
                            if (String.IsNullOrEmpty(sUsername))
                                sUsername = strSID;

                            Logger.Log("Trustee = " + sUsername, Logger.SecurityDescriptorLogLevel);
                            Logger.Log(string.Format("SID={0} : AceType={1}/ AceMask={2}/ AceFlags={3}",
                                                strSID,
                                                ace.Header.AceType.ToString(),
                                                ace.Mask.ToString(),
                                                ace.Header.AceFlags.ToString()), Logger.SecurityDescriptorLogLevel);

                            LwAccessControlEntry Ace = new LwAccessControlEntry();
                            Ace.Username = sUsername + "(" + sUsername + "@" + sDomain + ")";
                            Ace.SID = strSID;
                            Ace.AceType = Convert.ToInt32(ace.Header.AceType);
                            Ace.AccessMask = ace.Mask.ToString();
                            Ace.AceFlags = Convert.ToInt32(ace.Header.AceFlags.ToString());
                            Ace.AceSize = Convert.ToInt32(ace.Header.AceSize.ToString());

                            daclInfo.Add(Ace);
                        }
                        if (daclInfo != null && daclInfo.Count != 0)
                        {
                            List<LwAccessControlEntry> objectDacl = new List<LwAccessControlEntry>();
                            foreach (LwAccessControlEntry Ace in daclInfo)
                            {
                                if (!SdDacls.ContainsKey(Ace.Username))
                                {
                                    objectDacl = new List<LwAccessControlEntry>();
                                    objectDacl.Add(Ace);
                                    SdDacls.Add(Ace.Username, objectDacl);
                                }
                                else
                                {
                                    objectDacl = SdDacls[Ace.Username];
                                    objectDacl.Add(Ace);
                                    SdDacls[Ace.Username] = objectDacl;
                                }
                            }
                        }
                        ObjSecurityDescriptor.Descretionary_Access_Control_List = SdDacls;
                    }
                    else
                    {
                        ObjSecurityDescriptor.Descretionary_Access_Control_List = null;
                        ObjSecurityDescriptor.IsAccessDenied = true;
                    }

                    sSECURITY_DESCRIPTOR = (SecurityDescriptorApi.SECURITY_DESCRIPTOR)Marshal.PtrToStructure(pSECURITY_DESCRIPTOR, typeof(SecurityDescriptorApi.SECURITY_DESCRIPTOR));

                    //Get Security Descriptor Control
                    uint dwRevision;
                    SecurityDescriptorApi.SECURITY_DESCRIPTOR_CONTROL pControl;
                    SecurityDescriptorApi.GetSecurityDescriptorControl(pSECURITY_DESCRIPTOR, out pControl, out dwRevision);
                    ObjSecurityDescriptor.Control = (uint)pControl;
                    ObjSecurityDescriptor.Revision = dwRevision;

                    //Get Security Descriptor Owner
                    bool lpbOwnerDefaulted = false;
                    ptrSid = IntPtr.Zero;
                    bRet = SecurityDescriptorApi.GetSecurityDescriptorOwner(pSECURITY_DESCRIPTOR, out ptrSid, out lpbOwnerDefaulted);
                    Logger.Log("SecurityDescriptorApi.GetSecurityDescriptorOwner iRet value: " + Marshal.GetLastWin32Error());
                    ObjSecurityDescriptor.Owner = GetObjectStringSid(ptrSid);
                    SecurityDescriptorApi.FreeSid(ptrSid);

                    //Get Security Descriptor Group
                    bool lpbGroupDefaulted = false;
                    ptrSid = IntPtr.Zero;
                    bRet = SecurityDescriptorApi.GetSecurityDescriptorGroup(pSECURITY_DESCRIPTOR, out ptrSid, out lpbGroupDefaulted);
                    Logger.Log("SecurityDescriptorApi.GetSecurityDescriptorGroup iRet value: " + Marshal.GetLastWin32Error());
                    ObjSecurityDescriptor.PrimaryGroupID = GetObjectStringSid(ptrSid);
                    SecurityDescriptorApi.FreeSid(ptrSid);

                    ObjSecurityDescriptor.Size = SecurityDescriptorApi.GetSecurityDescriptorLength(pSECURITY_DESCRIPTOR);

                    ObjSecurityDescriptor.pSecurityDescriptor = pSECURITY_DESCRIPTOR;
                }
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

        public static bool ApiAdjustTokenPrivileges(ref IntPtr pProcessTokenHandle, string sPrivilizeValue)
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
                                    pPreviousTpStruct,
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

                IntPtr pLuid = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(SecurityDescriptorApi.LwLUID)));
                SecurityDescriptorApi.LookupPrivilegeValueW("", sPrivilizeValue, out pLuid);

                uint privilige = SecurityDescriptorApi.SE_PRIVILEGE_ENABLED ;//| SecurityDescriptorApi.SE_SECURITY_NAME;
                pPreviousTpStruct.Attributes = (int)privilige;
                pPreviousTpStruct.PrivilegeCount = 1;
                pPreviousTpStruct.Luid = pLuid;

                bIsSuccess = SecurityDescriptorApi.AdjustTokenPrivileges(
                                    pProcessTokenHandle,
                                    false,
                                    pPreviousTpStruct,
                                    0,
                                    IntPtr.Zero,
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
            bool bSuccess = false;
            uint errorReturn = 0;
            IntPtr pProcessHandle = IntPtr.Zero;
            pTokenHandle = IntPtr.Zero;

            Logger.Log("SecurityDescriptorWrapper.ApiGetCurrentProcessHandle()", Logger.SecurityDescriptorLogLevel);

            try
            {
                uint iThreadId = SecurityDescriptorApi.GetCurrentThreadId();
                pProcessHandle = SecurityDescriptorApi.OpenThread(SecurityDescriptorApi.ThreadAccess.ALL_ACCESS, true, iThreadId);

                bSuccess = SecurityDescriptorApi.OpenThreadToken(pProcessHandle, DesiredAccess, false, out pTokenHandle);
                errorReturn = (uint)Marshal.GetLastWin32Error();
                if (errorReturn == (uint)ErrorCodes.WIN32Enum.ERROR_NO_TOKEN ||
                    errorReturn != 0)
                {
                    pProcessHandle = Process.GetCurrentProcess().Handle;
                    bSuccess = SecurityDescriptorApi.OpenProcessToken(pProcessHandle, DesiredAccess, out pTokenHandle);
                }
                if (pTokenHandle != null)
                {
                    SecurityDescriptorWrapper.ApiAdjustTokenPrivileges(ref pTokenHandle, "SeTakeOwnershipPrivilege");
                    SecurityDescriptorWrapper.ApiAdjustTokenPrivileges(ref pTokenHandle, "SeSecurityPrivilege");
                    SecurityDescriptorWrapper.ApiAdjustTokenPrivileges(ref pTokenHandle, "SeBackupPrivilege");
                    SecurityDescriptorWrapper.ApiAdjustTokenPrivileges(ref pTokenHandle, "SeRestorePrivilege");
                    //bSuccess = SecurityDescriptorApi.SetThreadToken(pProcessHandle, pTokenHandle);
                    //errorReturn = (uint)Marshal.GetLastWin32Error();
                    //Logger.Log("SecurityDescriptorApi.SetThreadToken()");
                }

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

        public static uint ApiSetSecurityDescriptorDacl(
                    object editedObjects,
                    object addedObjects,
                    object deletedObjects,
                    IntPtr pSecurityDesriptorIn,
                    out IntPtr pSecurityDescriptorOut)
        {
            uint errorReturn = 0;
            Dictionary<string, object> editAces = editedObjects as Dictionary<string, object>;
            Dictionary<string, object> newAces = addedObjects as Dictionary<string, object>;
            Dictionary<string, object> deleteAces = deletedObjects as Dictionary<string, object>;
            pSecurityDescriptorOut = IntPtr.Zero;

            try
            {
                Logger.Log("SecurityDescriptorWrapper.ApiSetSecurityDescriptorDacl", Logger.SecurityDescriptorLogLevel);

                IntPtr pNewSd; IntPtr pDaclOffset; IntPtr pNewDacl; IntPtr ptrSid;
                bool lpbDaclPresent = false;
                bool lpbDaclDefaulted = false;
                SecurityDescriptorApi.LwACL acl = new SecurityDescriptorApi.LwACL();

                pDaclOffset = Marshal.AllocHGlobal(Marshal.SizeOf(acl));
                bool bRet = SecurityDescriptorApi.GetSecurityDescriptorDacl(pSecurityDesriptorIn, out lpbDaclPresent, out pDaclOffset, out lpbDaclDefaulted);
                Logger.Log("SecurityDescriptorApi.ApiSetSecurityDescriptorDacl iRet value", Logger.SecurityDescriptorLogLevel);

                if (pDaclOffset != IntPtr.Zero)
                {
                    acl = (SecurityDescriptorApi.LwACL)Marshal.PtrToStructure(pDaclOffset, typeof(SecurityDescriptorApi.LwACL));
                }

                SecurityDescriptorApi.ACL_SIZE_INFORMATION AclSize = new SecurityDescriptorApi.ACL_SIZE_INFORMATION();
                SecurityDescriptorApi.GetAclInformation(pDaclOffset, AclSize,
                                ((uint)Marshal.SizeOf(typeof(SecurityDescriptorApi.ACL_SIZE_INFORMATION))),
                                SecurityDescriptorApi.ACL_INFORMATION_CLASS.AclSizeInformation);

                SecurityDescriptorApi.SECURITY_DESCRIPTOR sSECURITY_DESCRIPTOR = new SecurityDescriptorApi.SECURITY_DESCRIPTOR();
                sSECURITY_DESCRIPTOR = (SecurityDescriptorApi.SECURITY_DESCRIPTOR)Marshal.PtrToStructure(pSecurityDesriptorIn, typeof(SecurityDescriptorApi.SECURITY_DESCRIPTOR));

                uint totalCount = (uint)(newAces.Count - deleteAces.Count);
                int cbNewACL = (int)AclSize.AclBytesInUse + (int)((Marshal.SizeOf(typeof(SecurityDescriptorApi.ACCESS_ALLOWED_ACE)) - IntPtr.Size) * totalCount);

                // Create new ACL
                pNewDacl = Marshal.AllocHGlobal(cbNewACL);
                bRet = SecurityDescriptorApi.InitializeAcl(pNewDacl, cbNewACL, acl.AclRevision);
                Console.WriteLine(Marshal.GetLastWin32Error());

                bRet = SecurityDescriptorApi.IsValidAcl(pNewDacl);
                Console.WriteLine(Marshal.GetLastWin32Error());

                SecurityDescriptorApi.ACL_REVISION_INFORMATION aclRevision = new SecurityDescriptorApi.ACL_REVISION_INFORMATION();
                aclRevision.AclRevision = acl.AclRevision;
                IntPtr pAclRevision = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(SecurityDescriptorApi.ACL_REVISION_INFORMATION)));
                Marshal.StructureToPtr(aclRevision, pAclRevision, true);

                bRet = SecurityDescriptorApi.SetAclInformation(
                                      pNewDacl,
                                      pAclRevision,
                                      ((uint)Marshal.SizeOf(typeof(SecurityDescriptorApi.ACL_REVISION_INFORMATION))),
                                      SecurityDescriptorApi.ACL_INFORMATION_CLASS.AclRevisionInformation);
                Console.WriteLine(Marshal.GetLastWin32Error());

                //Delete/Editing the Ace entry from the ACl
                for (int idx = 0; idx < AclSize.AceCount; idx++)
                {
                    IntPtr pAce; ptrSid = IntPtr.Zero;
                    string sUsername, sDomain;

                    int err = SecurityDescriptorApi.GetAce(pDaclOffset, idx, out pAce);
                    SecurityDescriptorApi.ACCESS_ALLOWED_ACE ace = (SecurityDescriptorApi.ACCESS_ALLOWED_ACE)Marshal.PtrToStructure(pAce, typeof(SecurityDescriptorApi.ACCESS_ALLOWED_ACE));

                    IntPtr iter = (IntPtr)((int)pAce + (int)Marshal.OffsetOf(typeof(SecurityDescriptorApi.ACCESS_ALLOWED_ACE), "SidStart"));

                    int size = (int)SecurityDescriptorApi.GetLengthSid(iter);
                    byte[] bSID = new byte[size];
                    Marshal.Copy(iter, bSID, 0, size);
                    SecurityDescriptorApi.ConvertSidToStringSid(bSID, out ptrSid);
                    GetObjectLookUpName(iter, out sUsername, out sDomain);

                    if (deleteAces != null && deleteAces.Count != 0 && deleteAces.ContainsKey(sUsername))
                        continue;

                    if (editAces != null && editAces.Count != 0 && editAces.ContainsKey(sUsername))
                    {
                        List<LwAccessControlEntry> attributes = editAces[sUsername] as List<LwAccessControlEntry>;
                        if (attributes != null)
                        {
                            foreach (LwAccessControlEntry lwAce in attributes)
                            {
                                if ((int)ace.Header.AceType == lwAce.AceType)
                                {
                                    SecurityDescriptorApi.ConvertStringSidToSid(lwAce.SID, out ptrSid);
                                    bRet = SecurityDescriptorApi.AddAccessAllowedAceEx(
                                         pNewDacl,
                                         0,
                                         Convert.ToByte(ace.Header.AceFlags),
                                         Convert.ToInt32(lwAce.AccessMask),
                                         ptrSid);

                                    Console.WriteLine(Marshal.GetLastWin32Error());
                                    Logger.Log("SecurityDescriptorWrapper.ApiSetSecurityDescriptorDacl() : SecurityDescriptorApi.AddAccessAllowedAceEx return code :" + Marshal.GetLastWin32Error());
                                }
                            }
                        }
                    }
                    else
                    {
                        Logger.Log("Adding new Ace from existing one: " + sUsername, Logger.SecurityDescriptorLogLevel);
                        bRet = SecurityDescriptorApi.AddAccessAllowedAceEx(
                                       pNewDacl,
                                       0,
                                       ace.Header.AceFlags,
                                       ace.Mask,
                                       iter);
                        Console.WriteLine(Marshal.GetLastWin32Error());
                    }
                    SecurityDescriptorApi.FreeSid(ptrSid);
                }

                //Adding new Aces to the Security Descriptor
                foreach (string key in newAces.Keys)
                {
                    List<LwAccessControlEntry> attributes = newAces[key] as List<LwAccessControlEntry>;
                    foreach (LwAccessControlEntry lwAce in attributes)
                    {
                        SecurityDescriptorApi.ConvertStringSidToSid(lwAce.SID, out ptrSid);
                        bRet = SecurityDescriptorApi.AddAccessAllowedAceEx(
                             pNewDacl,
                             0,
                             Convert.ToByte(lwAce.AceFlags),
                             Convert.ToInt32(lwAce.AccessMask),
                             ptrSid);

                        Console.WriteLine(Marshal.GetLastWin32Error());
                        Logger.Log("Adding new Ace : " + lwAce.Username, Logger.SecurityDescriptorLogLevel);
                        Logger.Log("SecurityDescriptorWrapper.ApiSetSecurityDescriptorDacl() : SecurityDescriptorApi.AddAccessAllowedAceEx return code :" + Marshal.GetLastWin32Error());
                    }
                }

                pNewSd = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(SecurityDescriptorApi.SECURITY_DESCRIPTOR)));
                int iSdRevision = (int)SecurityDescriptorApi.SECURITY_DESCRIPTOR_REVISION;
                bRet = SecurityDescriptorApi.InitializeSecurityDescriptor(pNewSd, (uint)iSdRevision);
                Console.WriteLine(Marshal.GetLastWin32Error());
                Logger.Log("SecurityDescriptorApi.InitializeSecurityDescriptor returns errorcode: " + errorReturn);

                bRet = SecurityDescriptorApi.IsValidSecurityDescriptor(pNewSd);
                Console.WriteLine(Marshal.GetLastWin32Error());
                Logger.Log("SecurityDescriptorApi.IsValidSecurityDescriptor returns errorcode: " + Marshal.GetLastWin32Error());

                bRet = SecurityDescriptorApi.SetSecurityDescriptorDacl(
                                pNewSd,
                                lpbDaclPresent,
                                pNewDacl,
                                lpbDaclDefaulted);
                Console.WriteLine(Marshal.GetLastWin32Error());
                Logger.Log("SecurityDescriptorApi.SetSecurityDescriptorDacl returns errorcode: " + Marshal.GetLastWin32Error());

                bRet = SecurityDescriptorApi.IsValidSecurityDescriptor(pNewSd);
                Console.WriteLine(Marshal.GetLastWin32Error());
                Logger.Log("SecurityDescriptorApi.IsValidSecurityDescriptor returns errorcode: " + Marshal.GetLastWin32Error());

                pSecurityDescriptorOut = IntPtr.Zero;
                uint lpdwBufferLength = (uint)Marshal.SizeOf(typeof(SecurityDescriptorApi.SECURITY_DESCRIPTOR));

                bRet = SecurityDescriptorApi.MakeSelfRelativeSD(pNewSd, out pSecurityDescriptorOut, ref lpdwBufferLength);
                Console.WriteLine(Marshal.GetLastWin32Error());

                bRet = SecurityDescriptorApi.MakeSelfRelativeSD(pNewSd, out pSecurityDescriptorOut, ref lpdwBufferLength);
                Console.WriteLine(Marshal.GetLastWin32Error());

                bRet = SecurityDescriptorApi.IsValidSecurityDescriptor(pNewSd);
                Console.WriteLine(Marshal.GetLastWin32Error());
                Logger.Log("SecurityDescriptorApi.IsValidSecurityDescriptor returns errorcode: " + Marshal.GetLastWin32Error());

                /*bRet = SecurityDescriptorApi.SetSecurityDescriptorGroup(
                               pNewSd,
                               sSECURITY_DESCRIPTOR.group,
                               false);
                Console.WriteLine(Marshal.GetLastWin32Error());
                Logger.Log("SecurityDescriptorApi.SetSecurityDescriptorGroup returns errorcode: " + Marshal.GetLastWin32Error());

                bRet = SecurityDescriptorApi.IsValidSecurityDescriptor(pNewSd);
                Console.WriteLine(Marshal.GetLastWin32Error());
                Logger.Log("SecurityDescriptorApi.IsValidSecurityDescriptor returns errorcode: " + Marshal.GetLastWin32Error());

                bRet = SecurityDescriptorApi.SetSecurityDescriptorOwner(
                              pNewSd,
                              sSECURITY_DESCRIPTOR.owner,
                              false);
                Console.WriteLine(Marshal.GetLastWin32Error());
                Logger.Log("SecurityDescriptorApi.SetSecurityDescriptorOwner returns errorcode: " + Marshal.GetLastWin32Error());

                bRet = SecurityDescriptorApi.IsValidSecurityDescriptor(pNewSd);
                Console.WriteLine(Marshal.GetLastWin32Error());
                Logger.Log("SecurityDescriptorApi.IsValidSecurityDescriptor returns errorcode: " + Marshal.GetLastWin32Error());

                uint pControl; uint lpdwRevision;
                bRet = SecurityDescriptorApi.GetSecurityDescriptorControl(
                             pSecurityDesriptorIn,
                             out pControl,
                             out lpdwRevision);
                Console.WriteLine(Marshal.GetLastWin32Error());
                Logger.Log("SecurityDescriptorApi.SetSecurityDescriptorControl returns errorcode: " + Marshal.GetLastWin32Error());

                bRet = SecurityDescriptorApi.SetSecurityDescriptorControl(
                             pNewSd,
                             pControl,
                             pControl);
                Console.WriteLine(Marshal.GetLastWin32Error());
                Logger.Log("SecurityDescriptorApi.SetSecurityDescriptorControl returns errorcode: " + Marshal.GetLastWin32Error());

                bRet = SecurityDescriptorApi.IsValidSecurityDescriptor(pNewSd);
                Console.WriteLine(Marshal.GetLastWin32Error());
                Logger.Log("SecurityDescriptorApi.IsValidSecurityDescriptor returns errorcode: " + Marshal.GetLastWin32Error());

                if (pNewDacl != IntPtr.Zero)
                {
                    SecurityDescriptorApi.LocalFree(pNewDacl);
                }

                uint lpdwAbsoluteSDSize = 0, lpdwDaclSize = 0, lpdwSaclSize = 0, lpdwOwnerSize = 0, lpdwPrimaryGroupSize = 0;
                IntPtr pAbsoluteSD, pDacl, pSacl, pOwner, pPrimaryGroup;
                lpdwAbsoluteSDSize = (uint)Marshal.SizeOf(typeof(SecurityDescriptorApi.SECURITY_DESCRIPTOR));
                bRet = SecurityDescriptorApi.MakeAbsoluteSD(pNewSd, out pAbsoluteSD,
                                                ref lpdwAbsoluteSDSize, out pDacl, ref lpdwDaclSize,
                                                out pSacl, ref lpdwSaclSize,
                                                out pOwner, ref lpdwOwnerSize,
                                                out pPrimaryGroup, ref lpdwPrimaryGroupSize);
                Console.WriteLine(Marshal.GetLastWin32Error());
                Logger.Log("SecurityDescriptorApi.MakeAbsoluteSD returns errorcode: " + Marshal.GetLastWin32Error()); */

                if (!bRet)
                    pSecurityDescriptorOut = IntPtr.Zero;
            }
            catch (Exception ex)
            {
                pSecurityDescriptorOut = IntPtr.Zero;
                errorReturn = (uint)Marshal.GetLastWin32Error();
                Logger.LogException("SecurityDescriptorWrapper.ApiGetLogOnUserHandle()", ex);
            }

            return errorReturn;
        }

        public static uint ApiBuildTrusteeWithSid(IntPtr pSid, out SecurityDescriptorApi.TRUSTEE Trustee)
        {
            uint errorReturn = 0;
            IntPtr pTrustee = IntPtr.Zero;
            Trustee = new SecurityDescriptorApi.TRUSTEE();

            try
            {
                Logger.Log("SecurityDescriptorWrapper.ApiBuildTrusteeWithSid() called", Logger.SecurityDescriptorLogLevel);


                pTrustee = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(SecurityDescriptorApi.TRUSTEE)));
                bool bRet = SecurityDescriptorApi.BuildTrusteeWithSid(ref pTrustee, pSid);

                errorReturn = (uint)Marshal.GetLastWin32Error();

                Logger.Log("Build trustee returns" + errorReturn);
                if (pTrustee != IntPtr.Zero)
                {
                    Trustee = new SecurityDescriptorApi.TRUSTEE();
                    Trustee = (SecurityDescriptorApi.TRUSTEE)Marshal.PtrToStructure(pTrustee, typeof(SecurityDescriptorApi.TRUSTEE));
                }
            }
            catch (Exception ex)
            {
                errorReturn = (uint)Marshal.GetLastWin32Error();
                Logger.LogException("SecurityDescriptorWrapper.ApiBuildTrusteeWithSid()", ex);
                Logger.Log("Build trustee " + Marshal.GetLastWin32Error());
            }

            return errorReturn;
        }

        public static uint ApiAdjustTokenSecurity(IntPtr pToken)
        {
            Logger.Log("SecurityDescriptorWrapper.ApiAdjustTokenSecurity is called");

            IntPtr pProcessToken = IntPtr.Zero;
            uint errorReturn = ApiGetCurrentProcessHandle(SecurityDescriptorApi.TOKEN_ALL_ACCESS, out pProcessToken);
            Logger.Log("SecurityDescriptorWrapper.ApiGetCurrentProcessHandle() returns: " + errorReturn);

            ApiAdjustTokenPrivileges(ref pToken, "SeTakeOwnershipPrivilege");
            ApiAdjustTokenPrivileges(ref pToken, "SeSecurityPrivilege");
            ApiAdjustTokenPrivileges(ref pToken, "SeBackupPrivilege");
            ApiAdjustTokenPrivileges(ref pToken, "SeRestorePrivilege");

            return errorReturn;
        }

        #endregion

        #region CSP (cryptographic service provider) wrapper implementations

        public static int ApiGetHandleToCSP(string _objectPath, out IntPtr hProv)
        {
            int errorReturn = 0;
            bool bRet = false;
            IntPtr hSigKey = IntPtr.Zero;
            IntPtr hExchKey = IntPtr.Zero;
            hProv = IntPtr.Zero;

            Logger.Log("SecurityDescriptorWrapper.ApiGetHandleToCSP called", Logger.SecurityDescriptorLogLevel);

            try
            {
                //Acquire a handle to the cryptographic service provider.
                bRet = SecurityDescriptorApi.CryptAcquireContext(
                                    ref hProv,
                                    _objectPath,
                                    SecurityDescriptorApi.MS_STRONG_PROV,
                                    SecurityDescriptorApi.PROV_RSA_FULL,
                                    SecurityDescriptorApi.CRYPT_NEWKEYSET | SecurityDescriptorApi.CRYPT_MACHINE_KEYSET);
                if (!bRet)
                {
                    errorReturn = Marshal.GetLastWin32Error();
                    Logger.Log("SecurityDescriptorApi.CryptAcquireContext is failed to open OpenCtxHandle handle :errorReturn = " + errorReturn, Logger.SecurityDescriptorLogLevel);
                    return errorReturn;
                }
                Logger.Log("SecurityDescriptorApi.CryptAcquireContext success", Logger.SecurityDescriptorLogLevel);

                /*****Generates a signature and a key exchange key. *****/
                // Generate the signature key.
                bRet = SecurityDescriptorApi.CryptGenKey(hProv,
                                    SecurityDescriptorApi.AT_SIGNATURE,
                                    0,
                                    ref hSigKey);
                if (!bRet)
                {
                    errorReturn = Marshal.GetLastWin32Error();
                    Logger.Log("SecurityDescriptorApi.CryptGenKey is failed to get signature key :errorReturn = " + errorReturn, Logger.SecurityDescriptorLogLevel);
                    return errorReturn;
                }
                Logger.Log("SecurityDescriptorApi.CryptGenKey for signature key success", Logger.SecurityDescriptorLogLevel);

                // Generate the key exchange key.
                bRet = SecurityDescriptorApi.CryptGenKey(hProv,
                                    SecurityDescriptorApi.AT_KEYEXCHANGE,
                                    0,
                                    ref hExchKey);
                if (!bRet)
                {
                    errorReturn = Marshal.GetLastWin32Error();
                    Logger.Log("SecurityDescriptorApi.CryptGenKey is failed to get exchange key :errorReturn = " + errorReturn, Logger.SecurityDescriptorLogLevel);
                    return errorReturn;
                }
                Logger.Log("SecurityDescriptorApi.CryptGenKey for exchange key success", Logger.SecurityDescriptorLogLevel);
            }
            catch (Exception ex)
            {
                Logger.LogException("SecurityDescriptorWrapper.ApiGetHandleToCSP", ex);
            }
            finally
            {
                if (hSigKey != IntPtr.Zero)
                    SecurityDescriptorApi.CryptDestroyKey(hSigKey);

                if (hExchKey != IntPtr.Zero)
                    SecurityDescriptorApi.CryptDestroyKey(hExchKey);
            }
            return errorReturn;
        }

        #endregion

        #region Helper functions

        public static SecurityDescriptorApi.ACCESS_MODE GetAccessMode(object AccessType)
        {
            int iAccessType = Convert.ToInt32(AccessType);

            switch (iAccessType)
            {
                case 0:
                    return SecurityDescriptorApi.ACCESS_MODE.GRANT_ACCESS;

                case 1:
                    return SecurityDescriptorApi.ACCESS_MODE.DENY_ACCESS;

                case 2:
                    return SecurityDescriptorApi.ACCESS_MODE.SET_AUDIT_SUCCESS;
            }

            return SecurityDescriptorApi.ACCESS_MODE.NOT_USED_ACCESS;
        }

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
            finally
            {
                //Freeying sid pointer
                //if (ptrSid != IntPtr.Zero)
                    //SecurityDescriptorApi.FreeSid(ptrSid);
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
