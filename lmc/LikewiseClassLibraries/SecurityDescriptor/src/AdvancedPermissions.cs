using System;
using System.Collections.Generic;
using System.Text;

namespace Likewise.LMC.SecurityDesriptor
{
    public class AdvancedPermissions
    {
        #region Advanced permissions for Files, Directories, Registry Keys and Ads objects

        public static string[] DirectoryPermissionSet = new string[]{
                            "Full Control",
                            "Traverse Folder / Execute File",
                            "List Folder / Read Data",
                            "Read Attributes",
                            "Read Extended Attributes",
                            "Create Files / Write Data",
                            "Create Folders / Append Data",
                            "Write Attributes",
                            "Write Extended Attributes",
                            "Delect Subfolders and Files",
                            "Delete",
                            "Read Permissions",
                            "Change Permissions",
                            "Take Ownership"};

        public static string[] FilePermissionSet = new string[]{
                            "Full Control",
                            "Traverse Folder / Execute File",
                            "List Folder / Read Data",
                            "Read Attributes",
                            "Read Extended Attributes",
                            "Create Files / Write Data",
                            "Create Folders / Append Data",
                            "Write Attributes",
                            "Write Extended Attributes",
                            "Delete",
                            "Read Permissions",
                            "Change Permissions",
                            "Take Ownership"};

        public static string[] RegistryPermissionSet = new string[]{
                            "Full Control",
                            "Query Value",
                            "Set Value",
                            "Create Subkey",
                            "Enumerate Subkeys",
                            "Notify",
                            "Create Link",
                            "Delete",
                            "Write DAC",
                            "Write Owner",
                            "Read Control"};

        public static  string[] AdsPermissionSet = new string[]{
                            "Full Control",
                            "List Contents",
                            "Read All Properties", "Write All Properties",
                            "Delete", "Delete Subtree",
                            "Read Permissions", "Modify Permissions",
                            "Modify Owner",
                            "All Validated Writes", "All Extended Rights",
                            "Create All Child Objects", "Delete All Child Objects",
                            "Create Account Objects", "Delete Account Objects",
                            "Create ApplicationEntry Objects", "Delete ApplicationEntry Objects",
                            "Create ApplicationVersion Objects", "Delete ApplicationVersion Objects",
                            "Create classStore Objects", "Delete classStore Objects",
                            "Create Computer Objects", "Delete Computer Objects",
                            "Create Contact Objects", "Delete Contact Objects",
                            "Create Container Objects", "Delete Container Objects",
                            "Create device Objects", "Delete device Objects",
                            "Create document Objects", "Delete document Objects",
                            "Create documentSeries Objects", "Delete documentSeries Objects",
                            "Create domain Policy Objects", "Delect domain Policy Objects" };

        #endregion
    }
}
