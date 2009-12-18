using System;
using System.Collections.Generic;
using System.Text;

namespace Likewise.LMC.SecurityDesriptor
{
    public class PermissionsSet
    {
        public static Dictionary<object, string> permissionSet
                            = new Dictionary<object, string>();

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

        public static Dictionary<object, string> PermissionSet
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

                        case SecurityDescriptorApi.SE_OBJECT_TYPE.SE_DIR_OBJECT:
                            FillDirectoryPermissionSet();
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
            permissionSet.Add(LwAccessMask.ACCESS_MASK.DELETE, "Delete");
            permissionSet.Add(LwAccessMask.ACCESS_MASK.READ_CONTROL, "Read Control");//Read
            permissionSet.Add(LwAccessMask.ACCESS_MASK.WRITE_DAC, "Write access to the DACL");
            permissionSet.Add(LwAccessMask.ACCESS_MASK.WRITE_OWNER, "Write access to owner");
            permissionSet.Add(LwAccessMask.ACCESS_MASK.SPECIFIC_RIGHTS_ALL, "Specific Rights");
            permissionSet.Add(LwAccessMask.ACCESS_MASK.DESKTOP_CREATEMENU, "Create Menu");
            permissionSet.Add(LwAccessMask.ACCESS_MASK.DESKTOP_CREATEWINDOW, "Create Window");
            permissionSet.Add(LwAccessMask.ACCESS_MASK.DESKTOP_ENUMERATE, "Desktop Enumerate");
            permissionSet.Add(LwAccessMask.ACCESS_MASK.DESKTOP_HOOKCONTROL, "Hook Control");
            permissionSet.Add(LwAccessMask.ACCESS_MASK.DESKTOP_JOURNALPLAYBACK, "Journal Playback");
            permissionSet.Add(LwAccessMask.ACCESS_MASK.DESKTOP_JOURNALRECORD, "Journal Record");
            permissionSet.Add(LwAccessMask.ACCESS_MASK.DESKTOP_READOBJECTS, "Read Objects");
            permissionSet.Add(LwAccessMask.ACCESS_MASK.DESKTOP_SWITCHDESKTOP, "Switch Desktop");
            permissionSet.Add(LwAccessMask.ACCESS_MASK.DESKTOP_WRITEOBJECTS, "Write Objects");
            permissionSet.Add(LwAccessMask.ACCESS_MASK.GENERIC_ALL, "Read & Execute");
            permissionSet.Add(LwAccessMask.ACCESS_MASK.GENERIC_EXECUTE, "Execute");
            permissionSet.Add(LwAccessMask.ACCESS_MASK.GENERIC_READ, "Read");
            permissionSet.Add(LwAccessMask.ACCESS_MASK.GENERIC_WRITE, "Write");
            permissionSet.Add(LwAccessMask.ACCESS_MASK.MAXIMUM_ALLOWED, "Maximum allowed");
            permissionSet.Add(LwAccessMask.ACCESS_MASK.ACCESS_SYSTEM_SECURITY, "Special Permissions");
        }

        public static void FillFilePermissionSet()
        {
            permissionSet.Add(LwAccessMask.FileSystemAccesssMask.FullControl, "Full Control");
            permissionSet.Add(LwAccessMask.FileSystemAccesssMask.Modify, "Modify");
            permissionSet.Add(LwAccessMask.FileSystemAccesssMask.ReadAndExecute, "Read & Execute");
            permissionSet.Add(LwAccessMask.FileSystemAccesssMask.Read, "Read");
            permissionSet.Add(LwAccessMask.FileSystemAccesssMask.Write, "Write");
            permissionSet.Add(LwAccessMask.ACCESS_MASK.Special_Permissions, "Special Permissions");
        }

        public static void FillDirectoryPermissionSet()
        {
            permissionSet.Add(LwAccessMask.FileSystemAccesssMask.FullControl, "Full Control");
            permissionSet.Add(LwAccessMask.FileSystemAccesssMask.Modify, "Modify");
            permissionSet.Add(LwAccessMask.FileSystemAccesssMask.ReadAndExecute, "Read & Execute");
            permissionSet.Add(LwAccessMask.FileSystemAccesssMask.List_Directory, "List Folder Contents");
            permissionSet.Add(LwAccessMask.FileSystemAccesssMask.Read, "Read");
            permissionSet.Add(LwAccessMask.FileSystemAccesssMask.Write, "Write");
            permissionSet.Add(LwAccessMask.ACCESS_MASK.Special_Permissions, "Special Permissions");
        }

        public static void FillRegistryPermissionSet()
        {
            permissionSet.Add(LwAccessMask.RegistryAccesssMask.FULL_CONTROL, "Full Control");
            permissionSet.Add(LwAccessMask.RegistryAccesssMask.READ_KEY, "Read");
            permissionSet.Add(LwAccessMask.ACCESS_MASK.Special_Permissions, "Special Permissions");
        }

        public static void FillAdsPermissionSet()
        {
            permissionSet.Add(LwAccessMask.DirectoryServiceAccesssMask.Generic_All, "Full Control");
            permissionSet.Add(LwAccessMask.DirectoryServiceAccesssMask.Generic_Read, "Read");
            permissionSet.Add(LwAccessMask.DirectoryServiceAccesssMask.Generic_Read, "Write");
            permissionSet.Add(LwAccessMask.DirectoryServiceAccesssMask.Ds_Create_Child, "Create All Child Objects");
            permissionSet.Add(LwAccessMask.DirectoryServiceAccesssMask.Ds_Delete_Child, "Delete All Child Objects");
            permissionSet.Add(LwAccessMask.ACCESS_MASK.Special_Permissions, "Special Permissions");
        }
    }
}
