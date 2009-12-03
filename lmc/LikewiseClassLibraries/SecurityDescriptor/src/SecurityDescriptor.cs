using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;

namespace Likewise.LMC.SecurityDesriptor
{
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
                if (permissionSet != null && permissionSet.Count != 0)
                    return permissionSet;
                else
                {
                    permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.DELETE, "Delete");
                    permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.READ_CONTROL, "Read");
                    permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.WRITE_DAC, "Modify user attributes");
                    permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.WRITE_OWNER, "Full Control");
                    permissionSet.Add(SecurityDescriptorApi.ACCESS_MASK.ACCESS_SYSTEM_SECURITY, "Special Permissions");

                    return permissionSet;
                }
            }
        }
    }
}
