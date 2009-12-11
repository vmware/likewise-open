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

        #endregion

        #region Helper functions

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

                if ((iAccessMask & (uint)accesskmask) == 0)
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

                if ((iAccessMask & (uint)accesskmask) == 0)
                {
                    sPermissionname = PermissionsSet.PermissionSet[accesskmask];
                    AccessType = true;

                    permissions.Add(new object[] { sPermissionname, AccessType, !AccessType });
                }
            }

            return permissions;
        }

        public bool CheckAccessMaskExists(
                        string inputMask,
                        uint AccessMaskType)
        {
            if ((Convert.ToUInt32(inputMask) & AccessMaskType) == 0)
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
                    FillPermissionSet();

                return permissionSet;
            }
        }

        public static void FillPermissionSet()
        {
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.DELETE, "Delete");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.READ_CONTROL, "Read");
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
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.GENERIC_ALL, "Full Control");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.GENERIC_EXECUTE, "Generic Execute");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.GENERIC_READ, "Generic Read");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.GENERIC_WRITE, "Generic Write");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.MAXIMUM_ALLOWED, "Maximum allowed");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.ACCESS_SYSTEM_SECURITY, "Special Permissions");
            //permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.STANDARD_RIGHTS_ALL, "Standard Full Control");
            //permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.STANDARD_RIGHTS_EXECUTE, "Standard Rights Execute");
            //permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.STANDARD_RIGHTS_READ, "Standard Rights Read");
            //permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.STANDARD_RIGHTS_REQUIRED, "Standard Rights Required");
            //permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.STANDARD_RIGHTS_WRITE, "Standard Rights Write");
            //permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.SYNCHRONIZE, "Synchronize");
            //permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.WINSTA_ACCESSCLIPBOARD, "Access Clipboard");
            //permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.WINSTA_ACCESSGLOBALATOMS, "Access Global Atoms");
            //permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.WINSTA_ALL_ACCESS, "All Access");
            //permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.WINSTA_CREATEDESKTOP, "Create Desktop");
            //permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.WINSTA_ENUMDESKTOPS, "Enum Desktops");
            //permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.WINSTA_ENUMERATE, "Enumerate");
            //permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.WINSTA_EXITWINDOWS, "Exit Windows");
            //permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.WINSTA_READATTRIBUTES, "Read Attributes");
            //permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.WINSTA_READSCREEN, "Read Screen");
            //permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.WINSTA_WRITEATTRIBUTES, "Write Attributes");
        }
    }
}
