using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;

using Likewise.LMC.Netlogon;

namespace Likewise.LMC.SecurityDesriptor
{
    /// <summary>
    /// Class to describe the Security descriptor marshed values
    /// Includes functions to get the user or group list and permissions sets
    /// </summary>
    public class SecurityDescriptor
    {
        #region Class Data

        //Read the security Descriptor object for editing the Aces list and for adding the new Aces to the selected object.
        public IntPtr pSecurityDescriptor = IntPtr.Zero;
        public static SecurityDescriptorApi.SE_OBJECT_TYPE objectType = SecurityDescriptorApi.SE_OBJECT_TYPE.SE_FILE_OBJECT;

        private bool _disposed = false;

        private uint _revision;
        private byte _size;
        private uint _control;
        private string _owner = string.Empty;
        private string _primaryGroupID = string.Empty;
        private object _descretionary_access_control_list = null;
        private object _system_access_control_list = null;

        #endregion

        #region Constructors

        public SecurityDescriptor()
        {
            InitailizeToNull();
        }

        // Use C# destructor syntax for finalization code.
        // This destructor will run only if the Dispose method
        // does not get called.
        // It gives your base class the opportunity to finalize.
        // Do not provide destructors in types derived from this class.
        ~SecurityDescriptor()
        {
            // Do not re-create Dispose clean-up code here.
            // Calling Dispose(false) is optimal in terms of
            // readability and maintainability.
            Dispose(false);
        }

        #endregion

        #region Helper functions

        // Dispose(bool disposing) executes in two distinct scenarios.
        // If disposing equals true, the method has been called directly
        // or indirectly by a user's code. Managed and unmanaged resources
        // can be disposed.
        // If disposing equals false, the method has been called by the
        // runtime from inside the finalizer and you should not reference
        // other objects. Only unmanaged resources can be disposed.
        protected virtual void Dispose(bool disposing)
        {
            // Check to see if Dispose has already been called.
            if (!_disposed)
            {
                // If disposing equals true, dispose all managed
                // and unmanaged resources.
                if (disposing)
                {
                    // Dispose managed resources.
                    // Components.Dispose();
                }
                // Release unmanaged resources. If disposing is false,
                // only the following code is executed.
                CloseHandle();

                // Note that this is not thread safe.
                // Another thread could start disposing the object
                // after the managed resources are disposed,
                // but before the disposed flag is set to true.
                // If thread safety is necessary, it must be
                // implemented by the client.
            }
            _disposed = true;
        }

        public void Dispose()
        {
            Dispose(true);
            // Take yourself off the Finalization queue
            // to prevent finalization code for this object
            // from executing a second time.
            GC.SuppressFinalize(this);
        }

        /// <summary>
        /// Close the Security Descriptor pointer before the object got disposed.
        /// </summary>
        public void CloseHandle()
        {
            if(pSecurityDescriptor!=IntPtr.Zero)
                SecurityDescriptorApi.CloseHandle(pSecurityDescriptor);
        }

        /// <summary>
        /// Function to reset the all the data
        /// </summary>
        public void InitailizeToNull()
        {
            this._control = 0;
            this._revision = 0;
            this._size = 0;
            this._owner = string.Empty;
            this._primaryGroupID = string.Empty;
            this._descretionary_access_control_list = null;
            this._system_access_control_list = null;
        }

        /// <summary>
        /// Function to read the DACL data with the given key value
        /// </summary>
        /// <param name="daclInfo"></param>
        /// <param name="sKey"></param>
        /// <returns>Object: Value for the given key</returns>
        public object GetUserOrGroupSecurityInfo(
                            Dictionary<string, string> daclInfo,
                            string sKey)
        {
            object value = null;
            switch (sKey)
            {
                case "Sid":
                    //Can use this sid for editing the permission set
                    break;

                case "AceType":
                    if (Convert.ToUInt32(daclInfo[sKey]) == 0)
                        value = true;
                    else if (Convert.ToUInt32(daclInfo[sKey]) == 1)
                        value = true;
                    else if (Convert.ToUInt32(daclInfo[sKey]) == 2)
                        value = true;
                    break;

                case "AceMask":
                    value = daclInfo[sKey];
                    break;

                case "AceFlags":
                    //Can use these flags for editing the permission set
                    break;

                case "AceSize":
                    //Can use for editing the security descriptor value back
                    break;
            }
            return value;
        }

        /// <summary>
        /// Function to read the mapped strings for the given AccessMask value
        /// </summary>
        /// <param name="accessMask"></param>
        /// <param name="sPermissionname"></param>
        public List<string> GetUserOrGroupPermissions(
                        string accessMask)
        {
            //TO DO: Still needs to find the AccessMask enum mapping with string values
            uint iAccessMask = Convert.ToUInt32(accessMask);

            List<string> permissions = new List<string>();

            foreach (SecurityDescriptorApi.ACCESS_MASK accesskmask in PermissionsSet.PermissionSet.Keys)
            {
                string sPermissionname = string.Empty;

                if ((iAccessMask & (uint)accesskmask) > 0)
                {
                    sPermissionname = PermissionsSet.PermissionSet[accesskmask];

                    permissions.Add(sPermissionname);
                }
            }

            return permissions;
        }

        public List<object[]> GetPermissionsFromAccessMask(string accessMask)
        {
            //TO DO: Still needs to find the AccessMask enum mapping with string values
            uint iAccessMask = Convert.ToUInt32(accessMask);

            List<object[]> permissions = new List<object[]>();

            foreach (SecurityDescriptorApi.ACCESS_MASK accesskmask in PermissionsSet.PermissionSet.Keys)
            {
                string sPermissionname = string.Empty;
                bool AccessType = false;

                if ((iAccessMask & (uint)accesskmask) > 0)
                {
                    sPermissionname = PermissionsSet.PermissionSet[accesskmask];
                    AccessType = true;

                    permissions.Add(new object[] { sPermissionname, AccessType, !AccessType });
                }
            }

            return permissions;
        }

        public string GetKeyPermissionName(string accessMask)
        {
            //Still needs to find the AccessMask enum mapping with string values
            uint iAccessMask = Convert.ToUInt32(accessMask);

            foreach (SecurityDescriptorApi.ACCESS_MASK accesskmask in PermissionsSet.PermissionSet.Keys)
            {
                if (iAccessMask == (uint)accesskmask)
                {
                    return PermissionsSet.PermissionSet[accesskmask];
                }
            }

            return "";
        }

        public bool CheckAccessMaskExists(
                        string inputMask,
                        uint AccessMaskType)
        {
            if ((Convert.ToUInt32(inputMask) & AccessMaskType) > 0)
                return true;

            return false;
        }

        public string GetDCInfo(string domain)
        {
            string sDomain = string.Empty;
            Likewise.LMC.Netlogon.CNetlogon.LWNET_DC_INFO DCInfo;

            if (String.IsNullOrEmpty(domain))
                Likewise.LMC.Netlogon.CNetlogon.GetCurrentDomain(out sDomain);
            else
                sDomain = domain;

            uint netlogonError = Likewise.LMC.Netlogon.CNetlogon.GetDCName(sDomain, 0, out DCInfo);

            if (netlogonError == 0 && !String.IsNullOrEmpty(DCInfo.FullyQualifiedDomainName))
            {
                sDomain = DCInfo.FullyQualifiedDomainName;
            }

            return sDomain;
        }

        public string ConvetByteSidToStringSid(byte[] bSid)
        {
            return SecurityDescriptorWrapper.ConvetByteSidToStringSid(bSid);
        }

        public void GetAceType(uint AceType, ref uint iAceType)
        {
            switch (AceType)
            {
                case (uint)SecurityDescriptorApi.AccessTypes.Allow:
                    iAceType = iAceType | (uint)SecurityDescriptorApi.AccessTypes.Allow;
                    break;

                case (uint)SecurityDescriptorApi.AccessTypes.Allow_Object:
                    iAceType = iAceType | (uint)SecurityDescriptorApi.AccessTypes.Allow_Object;
                    break;

                case (uint)SecurityDescriptorApi.AccessTypes.Audit:
                    iAceType = iAceType | (uint)SecurityDescriptorApi.AccessTypes.Audit;
                    break;

                case (uint)SecurityDescriptorApi.AccessTypes.Audit_Object:
                    iAceType = iAceType | (uint)SecurityDescriptorApi.AccessTypes.Audit_Object;
                    break;

                case (uint)SecurityDescriptorApi.AccessTypes.Deny:
                    iAceType = iAceType | (uint)SecurityDescriptorApi.AccessTypes.Deny;
                    break;

                case (uint)SecurityDescriptorApi.AccessTypes.Deny_Object:
                    iAceType = iAceType | (uint)SecurityDescriptorApi.AccessTypes.Deny_Object;
                    break;
            }
        }

        public uint EditAce(object editAceslist,
                            object addedAceslist,
                            object deletedAceslist)
        {
            return SecurityDescriptorWrapper.ApiSetSecurityDescriptorDacl(
                                    editAceslist, addedAceslist, deletedAceslist, pSecurityDescriptor);
        }

        #endregion

        #region Access Specifiers

        public byte Size
        {
            set
            {
                _size = value;
            }
            get
            {
                return _size;
            }
        }

        public uint Control
        {
            set
            {
                _control = value;
            }
            get
            {
                return _control;
            }
        }

        public uint Revision
        {
            set
            {
                _revision = value;
            }
            get
            {
                return _revision;
            }
        }

        public string Owner
        {
            set
            {
                _owner = value;
            }
            get
            {
                return _owner;
            }
        }

        public string PrimaryGroupID
        {
            set
            {
                _primaryGroupID = value;
            }
            get
            {
                return _primaryGroupID;
            }
        }

        public object Descretionary_Access_Control_List
        {
            set
            {
                _descretionary_access_control_list = value;
            }
            get
            {
                return _descretionary_access_control_list;
            }
        }

        public object System_Access_Control_List
        {
            set
            {
                _system_access_control_list = value;
            }
            get
            {
                return _system_access_control_list;
            }
        }


        #endregion
    }

    public class PermissionsSet
    {
        public static Dictionary<SecurityDescriptorApi.ACCESS_MASK, string> permissionSet
                            = new Dictionary<SecurityDescriptorApi.ACCESS_MASK, string>();

        public string[] permissionsSet = new string[]{
                        "Delete",
                        "Read",
                        "Modify user attributes",
                        "Modify",
                        "Full Control",
                        "Read & Execute",
                        "Write",
                        "Special Permissions",
                        "Create All Child Objects",
                        "Delete All Child Objects",
                        "Add GUID",
                        "Add Replica In Domain",
                        "Change PDC",
                        "Create Inbound Forest Trust",
                        "Enable Per User Reversibly Encrypted Password",
                        "Genetate Resultant Set of Policy(Logging)",
                        "Genetate Resultant Set of Policy(Planning)",
                        "Manage Replication Topology",
                        "Migrate SID History",
                        "Monitor Active Directory Replication",
                        "Reanimate Tombstones",
                        "Replicate Directory Changes",
                        "Replicate Directory Changes All",
                        "Replication Synchronization",
                        "Unexpire Password",
                        "Update Password Not Required Bit",
                        "Read Domain Password & Lockout Policies",
                        "Write Domain Password & Lockout Policies",
                        "Read Other Domain Parameters (for use bt SAMS)",
                        "Write Other Domain Parameters (for use bt SAMS)"
                        };

        public static Dictionary<SecurityDescriptorApi.ACCESS_MASK, string> PermissionSet
        {
            get
            {
                if (permissionSet == null || permissionSet.Count == 0)
                {
                    switch (SecurityDescriptor.objectType)
                    {
                        case SecurityDescriptorApi.SE_OBJECT_TYPE.SE_FILE_OBJECT:
                            FillFilePermissionSet();
                            break;

                        case SecurityDescriptorApi.SE_OBJECT_TYPE.SE_REGISTRY_KEY:
                        case SecurityDescriptorApi.SE_OBJECT_TYPE.SE_REGISTRY_WOW64_32KEY:
                            FillRegistryPermissionSet();
                            break;

                        case SecurityDescriptorApi.SE_OBJECT_TYPE.SE_DS_OBJECT:
                        case SecurityDescriptorApi.SE_OBJECT_TYPE.SE_DS_OBJECT_ALL:
                            FillAdsPermissionSet();
                            break;

                        default:
                            FillPermissionSet();
                            break;
                    }
                }
                return permissionSet;
            }
        }

        public static void FillPermissionSet()
        {
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.DELETE, "Delete");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.READ_CONTROL, "Full Control");//Read
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.WRITE_DAC, "Modify");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.WRITE_OWNER, "Write");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.SPECIFIC_RIGHTS_ALL, "Specific Rights");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.DESKTOP_CREATEMENU, "Create Menu");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.DESKTOP_CREATEWINDOW, "Create Window");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.DESKTOP_ENUMERATE, "Desktop Enumerate");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.DESKTOP_HOOKCONTROL, "Hook Control");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.DESKTOP_JOURNALPLAYBACK, "Journal Playback");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.DESKTOP_JOURNALRECORD, "Journal Record");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.DESKTOP_READOBJECTS, "Read Objects");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.DESKTOP_SWITCHDESKTOP, "Switch Desktop");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.DESKTOP_WRITEOBJECTS, "Write Objects");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.GENERIC_ALL, "Generic All");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.GENERIC_EXECUTE, "Generic Execute");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.GENERIC_READ, "Generic Read");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.GENERIC_WRITE, "Generic Write");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.MAXIMUM_ALLOWED, "Maximum allowed");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.ACCESS_SYSTEM_SECURITY, "Special Permissions");
        }

        public static void FillFilePermissionSet()
        {
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.Full_Control, "Full Control");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.Modify, "Modify");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.Read_And_Execute, "Read & Execute");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.Read, "Read");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.WRITE_OWNER, "Write");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.Special_Permissions, "Special Permissions");
        }

        public static void FillDirectiryPermissionSet()
        {
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.Full_Control, "Full Control");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.Modify, "Modify");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.Read_And_Execute, "Read & Execute");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.Ds_Delete_Child, "List Folder Contents");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.Read, "Read");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.WRITE_OWNER, "Write");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.Special_Permissions, "Special Permissions");
        }

        public static void FillRegistryPermissionSet()
        {
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.Full_Control, "Full Control");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.Read, "Read");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.Special_Permissions, "Special Permissions");
        }

        public static void FillAdsPermissionSet()
        {
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.Full_Control, "Full Control");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.Read, "Read");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.WRITE_OWNER, "Write");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.Ds_Create_Child, "Create All Child Objects");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.Ds_Delete_Child, "Delete All Child Objects");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.Ds_List_Object, "Write");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.Special_Permissions, "Special Permissions");
        }
    }
}
