using System;
using System.Collections.Generic;
using System.Text;

namespace Likewise.LMC.SecurityDesriptor
{
    public class LwAccessMask
    {
        [Flags]
        public enum ACCESS_MASK : long
        {
            DELETE = 0x00010000,
            READ_CONTROL = 0x00020000,
            WRITE_DAC = 0x00040000,
            WRITE_OWNER = 0x00080000,
            SYNCHRONIZE = 0x00100000,

            STANDARD_RIGHTS_REQUIRED = 0x000f0000,//=DELETE|READ_CONTROL|WRITE_DAC|WRITE_OWNER
            STANDARD_RIGHTS_READ = 0x00020000,//=READ_CONTROL
            STANDARD_RIGHTS_WRITE = 0x00020000,//=READ_CONTROL
            STANDARD_RIGHTS_EXECUTE = 0x00020000,//=READ_CONTROL
            STANDARD_RIGHTS_ALL = 0x001f0000, //=DELETE|READ_CONTROL|WRITE_DAC|WRITE_OWNER|SYNCHRONIZE

            SPECIFIC_RIGHTS_ALL = 0x0000ffff,

            ACCESS_SYSTEM_SECURITY = 0x01000000,

            MAXIMUM_ALLOWED = 0x02000000,

            //Generic rights
            GENERIC_READ = 0x80000000,
            GENERIC_WRITE = 0x40000000,
            GENERIC_EXECUTE = 0x20000000,
            GENERIC_ALL = 0x10000000,

            DESKTOP_READOBJECTS = 0x00000001,
            DESKTOP_CREATEWINDOW = 0x00000002,
            DESKTOP_CREATEMENU = 0x00000004,
            DESKTOP_HOOKCONTROL = 0x00000008,
            DESKTOP_JOURNALRECORD = 0x00000010,
            DESKTOP_JOURNALPLAYBACK = 0x00000020,
            DESKTOP_ENUMERATE = 0x00000040,
            DESKTOP_WRITEOBJECTS = 0x00000080,
            DESKTOP_SWITCHDESKTOP = 0x00000100,

            WINSTA_ENUMDESKTOPS = 0x00000001,
            WINSTA_READATTRIBUTES = 0x00000002,
            WINSTA_ACCESSCLIPBOARD = 0x00000004,
            WINSTA_CREATEDESKTOP = 0x00000008,
            WINSTA_WRITEATTRIBUTES = 0x00000010,
            WINSTA_ACCESSGLOBALATOMS = 0x00000020,
            WINSTA_EXITWINDOWS = 0x00000040,
            WINSTA_ENUMERATE = 0x00000100,
            WINSTA_READSCREEN = 0x00000200,
            WINSTA_ALL_ACCESS = 0x0000037f,

            Full_Control = 983103,
            Read = 131097,
            Special_Permissions = 131072,
            Read_And_Execute = 1179817,
            Read_And_Write = 1180095,
            Modify = 1245631,

            Generic_All = 268435456,
            Generic_Execute = 536870912,
            Generic_Read = 2147483648,
            Generic_write = 1073741824
        }

        [Flags]
        public enum FileSystemAccesssMask : long
        {
            List_Directory = 1, //For Directory
            Read_Data = 1,
            Create_Files = 2, //For Directory
            Write_Data = 2,
            Create_Directories = 4, //For Directory
            Append_Data = 4,
            Read_Extended_Attributes = 8,
            WriteExtendedAttributes = 16,
            ExecuteFile = 32,
            Traverse = 32, //For Directory
            DeleteSubdirectoriesAndFiles = 64,
            ReadAttributes = 128,
            WriteAttributes = 256,
            Read = 131209,
            Write = 278,
            Modify = 197055,
            Delete = 65536,
            ReadAndExecute = 131241,
            ReadPermissions = 131072,
            ChangePermissions = 262144,
            TakeOwnership = 524288,
            Synchronize = 1048576,
            FullControl = 2032127
        }

        [Flags]
        public enum DirectoryAccesssMask : long
        {
            LIST = 0x00000001,
            ADD_FILE = 0x00000002,
            ADD_SUBDIR = 0x00000004,
            READ_EXTENDED_ATTR = 0x00000008,
            WRITE_EXTENDED_ATTR = 0x00000010,
            TRAVERSE = 0x00000020, //Execute
            DELETE_CHILD = 0x00000040,
            READ_ATTR = 0x00000080,
            WRITE_ATTR = 0x00000100,
            READ_EXECUTE = ACCESS_MASK.GENERIC_READ | TRAVERSE,
            FULL_CONTROL = ACCESS_MASK.GENERIC_READ | ACCESS_MASK.GENERIC_WRITE | /*APPEND |*/ READ_EXTENDED_ATTR |
                           WRITE_EXTENDED_ATTR | ACCESS_MASK.GENERIC_EXECUTE | ACCESS_MASK.DELETE | READ_ATTR |
                           WRITE_ATTR
        }

        [Flags]
        public enum RegistryAccesssMask : long
        {
            QUERY = 1,
            SET_VALUE = 2,
            CREATE_SUBKEY = 4,
            ENUM_SUBKEY = 8,
            NOTIFY = 16,
            CREATE_LINK = 32,
            DELETE = 65536,
            READ_PERMISSIONS = 131072,
            CHANGE_PERMISSIONS = 262144,
            WRITE_KEY = 131078,
            EXECUTE_KEY = 131097,
            READ_KEY = 131097,
            TAKE_OWNERSHIP = 524288,
            FULL_CONTROL = 983103
        }

        [Flags]
        public enum ServicesAccesssMask : long
        {
            QueryConf = 0x00000001,
            ChangeConf = 0x00000002,
            QueryState = 0x00000004,
            EnumDeps = 0x00000008,
            Start = 0x00000010,
            Stop = 0x00000020,
            Pause = 0x00000040,
            Interrogate = 0x00000080,
            UserDefined = 0x00000100
        }

        [Flags]
        public enum DirectoryServiceAccesssMask : long
        {
            Ds_Create_Child = 1,
            Ds_Delete_Child = 2,
            Actrl_Ds_list = 4, //List Children
            Self = 8,
            Ds_Read_Properties = 16,
            Ds_Write_Properties = 32,
            Delete_Tree = 64,
            Ds_List_Object = 128,//List_Object
            Entended_Right = 256, //Ds_Control_Access = 256
            Delete = 65536,
            Read_Control = 131072,
            Generic_Execute = 131076,
            Generic_Write = 131112,
            Generic_Read = 131220,
            Generic_All = 983551,
            Write_Dacl = 262144,
            Write_Owner = 524288,
            Synchronize = 1048576,
            AccessSystemSecurity = 16777216
        }
    }
}
