using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;

namespace Likewise.LMC.SecurityDesriptor
{
    /// <summary>
    /// Class to describe the Security descriptor marshed values
    /// Includes functions to get the user or group list and permissions sets
    /// </summary>
    public class SecurityDescriptor
    {
        #region Class Data

        private byte _revision;
        private byte _size;
        private short _control;
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
                    GetUserOrGroupAccessMask(daclInfo[sKey], ref value);
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
        public void GetUserOrGroupAccessMask(string accessMask, ref object sPermissionname)
        {
            //TO DO: Still needs to find the AccessMask enum mapping with string values
            uint iAccessMask = Convert.ToUInt32(accessMask);

            if ((iAccessMask & (uint)SecurityDescriptorApi.ACCESS_MASK.DELETE) == 0)
            {
                sPermissionname = PermissionsSet.PermissionSet[SecurityDescriptorApi.ACCESS_MASK.DELETE];
            }
            if ((iAccessMask & (uint)SecurityDescriptorApi.ACCESS_MASK.GENERIC_ALL) == 0)
            {
                sPermissionname = PermissionsSet.PermissionSet[SecurityDescriptorApi.ACCESS_MASK.GENERIC_ALL];
            }
            if ((iAccessMask & (uint)SecurityDescriptorApi.ACCESS_MASK.GENERIC_EXECUTE) == 0)
            {
                sPermissionname = PermissionsSet.PermissionSet[SecurityDescriptorApi.ACCESS_MASK.GENERIC_EXECUTE];
            }
            if ((iAccessMask & (uint)SecurityDescriptorApi.ACCESS_MASK.GENERIC_READ) == 0)
            {
                sPermissionname = PermissionsSet.PermissionSet[SecurityDescriptorApi.ACCESS_MASK.GENERIC_READ];
            }
            if ((iAccessMask & (uint)SecurityDescriptorApi.ACCESS_MASK.GENERIC_WRITE) == 0)
            {
                sPermissionname = PermissionsSet.PermissionSet[SecurityDescriptorApi.ACCESS_MASK.GENERIC_WRITE];
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

        public short Control
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

        public byte Revision
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
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.SPECIFIC_RIGHTS_ALL, "");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.DESKTOP_CREATEMENU, "");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.DESKTOP_CREATEWINDOW, "");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.DESKTOP_ENUMERATE, "");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.DESKTOP_HOOKCONTROL, "");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.DESKTOP_JOURNALPLAYBACK, "");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.DESKTOP_JOURNALRECORD, "");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.DESKTOP_READOBJECTS, "");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.DESKTOP_SWITCHDESKTOP, "");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.DESKTOP_WRITEOBJECTS, "");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.GENERIC_ALL, "Full Control");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.GENERIC_EXECUTE, "Execute");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.GENERIC_READ, "Read");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.GENERIC_WRITE, "Write");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.MAXIMUM_ALLOWED, "");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.ACCESS_SYSTEM_SECURITY, "");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.STANDARD_RIGHTS_ALL, "");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.STANDARD_RIGHTS_EXECUTE, "");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.STANDARD_RIGHTS_READ, "");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.STANDARD_RIGHTS_REQUIRED, "");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.STANDARD_RIGHTS_WRITE, "");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.SYNCHRONIZE, "");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.WINSTA_ACCESSCLIPBOARD, "");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.WINSTA_ACCESSGLOBALATOMS, "");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.WINSTA_ALL_ACCESS, "");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.WINSTA_CREATEDESKTOP, "");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.WINSTA_ENUMDESKTOPS, "");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.WINSTA_ENUMERATE, "");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.WINSTA_EXITWINDOWS, "");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.WINSTA_READATTRIBUTES, "");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.WINSTA_READSCREEN, "");
            permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.WINSTA_WRITEATTRIBUTES, "");
        }
    }
}
