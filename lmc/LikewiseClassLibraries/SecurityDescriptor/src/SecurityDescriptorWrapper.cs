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

            Dictionary<string, List<LwAccessControlEntry>> SdDacls = new Dictionary<string, List<LwAccessControlEntry>>();
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

                Marshal.PtrToStructure(pSECURITY_DESCRIPTOR, sSECURITY_DESCRIPTOR);

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

                IntPtr pLuid = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(SecurityDescriptorApi.LwLUID)));
                SecurityDescriptorApi.LookupPrivilegeValueW("", "SeSecurityPrivilege", out pLuid);

                uint privilige = SecurityDescriptorApi.SE_PRIVILEGE_ENABLED ;//| SecurityDescriptorApi.SE_SECURITY_NAME;
                pPreviousTpStruct.Attributes = (int)privilige;
                pPreviousTpStruct.PrivilegeCount = 1;
                pPreviousTpStruct.Luid = pLuid;

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

        public static uint ApiSetSecurityDescriptorDacl(
                    object editedObjects,
                    object addedObjects,
                    object deletedObjects,
                    IntPtr pSecurityDesriptor)
        {
            uint errorReturn = 0;

            Dictionary<string, object> editAces = editedObjects as Dictionary<string, object>;
            Dictionary<string, object> newAces = addedObjects as Dictionary<string, object>;
            Dictionary<string, object> deleteAces = deletedObjects as Dictionary<string, object>;

            //List<IntPtr> explicitAccesslist = new List<IntPtr>();

            try
            {
                Logger.Log("SecurityDescriptorWrapper.ApiSetSecurityDescriptorDacl", Logger.SecurityDescriptorLogLevel);

                IntPtr pDaclOffset;
                bool lpbDaclPresent = false;
                bool lpbDaclDefaulted = false;
                SecurityDescriptorApi.LwACL acl = new SecurityDescriptorApi.LwACL();

                pDaclOffset = Marshal.AllocHGlobal(Marshal.SizeOf(acl));
                bool bRet = SecurityDescriptorApi.GetSecurityDescriptorDacl(pSecurityDesriptor, out lpbDaclPresent, out pDaclOffset, out lpbDaclDefaulted);
                Logger.Log("SecurityDescriptorApi.GetSecurityDescriptorDacl iRet value", Logger.SecurityDescriptorLogLevel);

                if (pDaclOffset != IntPtr.Zero)
                {
                    Marshal.PtrToStructure(pDaclOffset, acl);
                }

                SecurityDescriptorApi.ACL_SIZE_INFORMATION AclSize = new SecurityDescriptorApi.ACL_SIZE_INFORMATION();
                SecurityDescriptorApi.GetAclInformation(pDaclOffset, AclSize,
                                ((uint)Marshal.SizeOf(typeof(SecurityDescriptorApi.ACL_SIZE_INFORMATION))),
                                SecurityDescriptorApi.ACL_INFORMATION_CLASS.AclSizeInformation);

                SecurityDescriptorApi.SECURITY_DESCRIPTOR sSECURITY_DESCRIPTOR = new SecurityDescriptorApi.SECURITY_DESCRIPTOR();
                Marshal.PtrToStructure(pSecurityDesriptor, sSECURITY_DESCRIPTOR);

                //Delete/Editing the Ace entry from the ACl
                for (int idx = 0; idx < AclSize.AceCount; idx++)
                {
                    IntPtr pAce; IntPtr ptrSid;
                    string sUsername, sDomain;
                    SecurityDescriptorApi.TRUSTEE trustee = new SecurityDescriptorApi.TRUSTEE();

                    int err = SecurityDescriptorApi.GetAce(pDaclOffset, idx, out pAce);
                    SecurityDescriptorApi.ACCESS_ALLOWED_ACE ace = (SecurityDescriptorApi.ACCESS_ALLOWED_ACE)Marshal.PtrToStructure(pAce, typeof(SecurityDescriptorApi.ACCESS_ALLOWED_ACE));

                    IntPtr iter = (IntPtr)((int)pAce + (int)Marshal.OffsetOf(typeof(SecurityDescriptorApi.ACCESS_ALLOWED_ACE), "SidStart"));

                    int size = (int)SecurityDescriptorApi.GetLengthSid(iter);
                    byte[] bSID = new byte[size];
                    Marshal.Copy(iter, bSID, 0, size);
                    SecurityDescriptorApi.ConvertSidToStringSid(bSID, out ptrSid);

                    GetObjectLookUpName(iter, out sUsername, out sDomain);

                    if (deleteAces != null && deleteAces.Count != 0)
                    {
                        if (deleteAces.ContainsKey(sUsername))
                        {
                            bRet = SecurityDescriptorApi.DeleteAce(pDaclOffset, (uint)idx);
                            Logger.Log("SecurityDescriptorWrapper.ApiGetLogOnUserHandle() : SecurityDescriptorApi.DeleteAce return code :" + Marshal.GetLastWin32Error());
                            continue;
                        }
                    }

                    if (editAces != null && editAces.Count != 0)
                    {
                        if (editAces.ContainsKey(sUsername))
                        {
                            List<LwAccessControlEntry> attributes = editAces[sUsername] as List<LwAccessControlEntry>;
                            if (attributes != null)
                            {
                                foreach (LwAccessControlEntry lwAce in attributes)
                                {
                                    if ((int)ace.Header.AceType == lwAce.AceType)
                                    {
                                        IntPtr pNewDacl = IntPtr.Zero;
                                        IntPtr pNewSD = IntPtr.Zero;

                                        IntPtr pExplicitAccess = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(SecurityDescriptorApi.EXPLICIT_ACCESS)));
                                        SecurityDescriptorApi.BuildExplicitAccessWithName(ref pExplicitAccess,
                                                                    sUsername,
                                                                    Convert.ToUInt32(lwAce.AccessMask),
                                                                    SecurityDescriptorApi.ACCESS_MODE.GRANT_ACCESS,
                                                                    (uint)SecurityDescriptorApi.ACEFlags.CONTAINER_INHERIT_ACE);

                                        //SecurityDescriptorApi.EXPLICIT_ACCESS[] explicitAccess = new SecurityDescriptorApi.EXPLICIT_ACCESS[1];
                                        //explicitAccess[0] = new SecurityDescriptorApi.EXPLICIT_ACCESS();
                                        //explicitAccess[0].grfAccessMode = SecurityDescriptorApi.ACCESS_MODE.SET_ACCESS;
                                        //explicitAccess[0].grfAccessPermissions = Convert.ToUInt32(Convert.ToUInt32(lwAce.AccessMask) | (uint)LwAccessMask.ACCESS_MASK.ACCESS_SYSTEM_SECURITY);
                                        //explicitAccess[0].grfInheritance = 0;
                                        //explicitAccess[0].Trustee = new SecurityDescriptorApi.TRUSTEE();
                                        //explicitAccess[0].Trustee.ptstrName = lwAce.Username.Substring(0, lwAce.Username.IndexOf("("));
                                        //explicitAccess[0].Trustee.TrusteeForm = SecurityDescriptorApi.TRUSTEE_FORM.TRUSTEE_IS_NAME;
                                        //explicitAccess[0].Trustee.TrusteeType = SecurityDescriptorApi.TRUSTEE_TYPE.TRUSTEE_IS_USER;

                                        //IntPtr pexplicitAccess = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(SecurityDescriptorApi.EXPLICIT_ACCESS)));
                                        //Marshal.StructureToPtr(explicitAccess[0], pexplicitAccess, true);

                                        int iSdSize = Marshal.SizeOf(typeof(SecurityDescriptorApi.SECURITY_DESCRIPTOR));
                                        pNewDacl = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(SecurityDescriptorApi.LwACL)));
                                        pNewSD = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(SecurityDescriptorApi.SECURITY_DESCRIPTOR)));

                                        errorReturn = SecurityDescriptorApi.SetEntriesInAcl(
                                                            1,
                                                            pExplicitAccess,
                                                            pDaclOffset,
                                                            ref pNewDacl);
                                        Logger.Log("SecurityDescriptorApi.SetEntriesInAcl returns errorcode: " + errorReturn);

                                        errorReturn = SecurityDescriptorApi.InitializeSecurityDescriptor(ref pNewSD, (uint)iSdSize);
                                        Logger.Log("SecurityDescriptorApi.InitializeSecurityDescriptor returns errorcode: " + errorReturn);

                                        SecurityDescriptorApi.SECURITY_DESCRIPTOR newSd = new SecurityDescriptorApi.SECURITY_DESCRIPTOR();
                                        newSd.revision = sSECURITY_DESCRIPTOR.revision;
                                        Marshal.StructureToPtr(newSd, pNewSD, true);
                                        bRet = SecurityDescriptorApi.SetSecurityDescriptorDacl(
                                                        pNewSD,
                                                        true,
                                                        pNewDacl,
                                                        false);
                                        Logger.Log("SecurityDescriptorApi.InitializeSecurityDescriptor returns errorcode: " + Marshal.GetLastWin32Error());

                                        if (pNewDacl != IntPtr.Zero)
                                        {
                                            SecurityDescriptorApi.LocalFree(pNewDacl);
                                        }

                                        if (pNewSD != IntPtr.Zero)
                                        {
                                            SecurityDescriptorApi.LocalFree(pNewSD);
                                        }

                                    }
                                }
                            }
                        }
                    }
                }


                //Adding new Aces to the Security Descriptor
                int indx = 0;
                uint AceIndex = AclSize.AceCount;
                SecurityDescriptorApi.ACE[] Aces = new SecurityDescriptorApi.ACE[newAces.Count];

                foreach (string key in newAces.Keys)
                {
                    List<LwAccessControlEntry> attributes = newAces[key] as List<LwAccessControlEntry>;

                    foreach (LwAccessControlEntry lwAce in attributes)
                    {
                        SecurityDescriptorApi.ACE ace = new SecurityDescriptorApi.ACE();
                        ace.AccessMask = Convert.ToUInt32(lwAce.AccessMask);
                        ace.AceFlags = Convert.ToUInt32(lwAce.AceFlags);
                        ace.AceType = Convert.ToUInt32(lwAce.AceType);
                        ace.Trustee = lwAce.Username;

                        Aces[indx] = ace;
                        indx++;
                    }
                }

                bRet = SecurityDescriptorApi.AddAce(
                                pDaclOffset,
                                sSECURITY_DESCRIPTOR.revision,
                                AceIndex,
                                Aces,
                                (uint)Aces.Length);
                Logger.Log("SecurityDescriptorWrapper.ApiGetLogOnUserHandle() : SecurityDescriptorApi.AddAce return code :" + Marshal.GetLastWin32Error());
            }
            catch (Exception ex)
            {
                errorReturn = (uint)Marshal.GetLastWin32Error();
                Logger.LogException("SecurityDescriptorWrapper.ApiGetLogOnUserHandle()", ex);
            }

            return errorReturn;
        }

        public static uint ApiBuildTrusteeWithSid(IntPtr pSid, out SecurityDescriptorApi.TRUSTEE Trustee)
        {
            uint errorReturn = 0;
            IntPtr pTrustee = IntPtr.Zero;
            Trustee = null;

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
                    Marshal.PtrToStructure(pTrustee, Trustee);
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

            ApiAdjustTokenPrivileges(pToken);

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
