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

        //Initialize the SecurityDescriptor out param to edit in the corresponding Apis list
        public IntPtr pSecurityDescriptorOut = IntPtr.Zero;

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
            if (pSecurityDescriptor != IntPtr.Zero)
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
        /// Function to read the mapped strings for the given AccessMask value
        /// </summary>
        /// <param name="accessMask"></param>
        /// <param name="sPermissionname"></param>
        public List<string> GetUserOrGroupPermissions(
                        string accessMask)
        {
            //TO DO: Still needs to find the AccessMask enum mapping with string values
            long iAccessMask = Convert.ToInt64(accessMask);
            long specialPermissionMask = 0;

            List<string> permissions = new List<string>();

            foreach (object accessmask in PermissionsSet.PermissionSet.Keys)
            {
                string sPermissionname = string.Empty;

                if ((iAccessMask & Convert.ToInt64(accessmask)) == Convert.ToInt64(accessmask))
                {
                    sPermissionname = PermissionsSet.PermissionSet[accessmask];
                    if (!sPermissionname.Equals("Special Permissions"))
                        permissions.Add(sPermissionname);
                    else
                        specialPermissionMask = Convert.ToInt64(accessmask);
                }
            }

            if (permissions.Count == 0)
                permissions.Add(PermissionsSet.PermissionSet[specialPermissionMask]);

            return permissions;
        }

        /// <summary>
        /// Function to read the mapped strings for the given AccessMask value
        /// </summary>
        /// <param name="accessMask"></param>
        /// <param name="sPermissionname"></param>
        public List<string> GetObjectPermissionSet()
        {
            List<string> permissions = new List<string>();

            foreach (object accesskmask in PermissionsSet.PermissionSet.Keys)
            {
                permissions.Add(PermissionsSet.PermissionSet[accesskmask]);
            }

            return permissions;
        }

        public List<object[]> GetPermissionsFromAccessMask(string accessMask)
        {
            //TO DO: Still needs to find the AccessMask enum mapping with string values
            long iAccessMask = Convert.ToInt64(accessMask);

            List<object[]> permissions = new List<object[]>();

            foreach (object accesskmask in PermissionsSet.PermissionSet.Keys)
            {
                string sPermissionname = string.Empty;
                bool AccessType = false;

                if ((iAccessMask & Convert.ToInt64(accesskmask)) == Convert.ToInt64(accesskmask))
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
            long iAccessMask = Convert.ToInt64(accessMask);

            foreach (object accesskmask in PermissionsSet.PermissionSet.Keys)
            {
                if (iAccessMask == Convert.ToInt64(accesskmask))
                {
                    return PermissionsSet.PermissionSet[accesskmask];
                }
            }

            return "";
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

        public void GetIntAccessMaskFromStringAceMask(string sAcePermission, ref long oAceMask)
        {
            foreach (object accesskmask in PermissionsSet.PermissionSet.Keys)
            {
                if (PermissionsSet.PermissionSet[accesskmask].Equals(sAcePermission))
                {
                    oAceMask = oAceMask | Convert.ToInt64(accesskmask);
                }
            }
        }

        public uint EditAce(object editAceslist,
                            object addedAceslist,
                            object deletedAceslist,
                            ref IntPtr pSecurityDescriptorOut)
        {
            return SecurityDescriptorWrapper.ApiSetSecurityDescriptorDacl(
                                    editAceslist, addedAceslist, deletedAceslist, pSecurityDescriptor, out pSecurityDescriptorOut);
        }

        public string CovertStringSidToLookupName(string sSID)
        {
            string sUsername = string.Empty;
            string sDomain = string.Empty;
            IntPtr pSid = IntPtr.Zero;

            SecurityDescriptorApi.ConvertStringSidToSid(sSID, out pSid);
            if (pSid != IntPtr.Zero)
            {
                SecurityDescriptorWrapper.GetObjectLookUpName(pSid, out sUsername, out sDomain);
                sUsername = string.Concat(sUsername, "(", sUsername, "@", sDomain, ")");
            }

            return sUsername;
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
}
