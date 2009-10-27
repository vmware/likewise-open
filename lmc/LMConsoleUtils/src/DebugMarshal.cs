using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Runtime.InteropServices;

namespace Likewise.LMC.LMConsoleUtils
{
#if DEBUG
    enum AllocationType
    {
        HGlobal,
        CoTask,
        External,
    }
    class AllocationInfo
    {
        public AllocationInfo(AllocationType type, int size)
        {
            this.type = type;
            this.size = size;
        }
        public AllocationType type;
        public int size;
        public Type dotNetType; //This gets set when StructureToPtr is called
    }
#endif

    /// <summary>
    /// The TYMED enumeration values indicate the type of storage medium being used in a data transfer. 
    /// </summary>
    public enum TYMED
    {
        TYMED_HGLOBAL = 1,
        TYMED_FILE = 2,
        TYMED_ISTREAM = 4,
        TYMED_ISTORAGE = 8,
        TYMED_GDI = 16,
        TYMED_MFPICT = 32,
        TYMED_ENHMF = 64,
        TYMED_NULL = 0
    }

    public class STGWrapper
    {
        [StructLayout(LayoutKind.Sequential)]
        public struct STGMEDIUM
        {
            public uint tymed;
            public int hGlobal;
            public Object pUnkForRelease;
        }
        public STGMEDIUM stg = new STGMEDIUM();
        bool locked=false;

        public STGWrapper()
        {
            stg.tymed = (uint)TYMED.TYMED_HGLOBAL;
            stg.hGlobal = 0;
            stg.pUnkForRelease = null;
        }

        ~STGWrapper()
        {
            //If this fails, you should have called the Release function
            Debug.Assert(stg.hGlobal == 0);
            //If this fails, you locked this and didn't unlock it
            Debug.Assert(!locked);
        }

        //[DllImport("Ole32.dll")]
       // private extern static void ReleaseStgMedium(ref STGMEDIUM stg);

        public void Release()
        {
            Debug.Assert(!locked);
           // ReleaseStgMedium(ref stg);
            stg.hGlobal = 0;
        }

        /// <summary>
        /// The GlobalLock function locks a global memory object and returns a pointer to the first byte of the object's memory block.
        /// GlobalLock function increments the lock count by one.
        /// Needed for the clipboard functions when getting the data from IDataObject
        /// </summary>
        /// <param name="hMem"></param>
        /// <returns></returns>
        [DllImport("Kernel32.dll")]
        private static extern IntPtr GlobalLock(int hMem);

        /// <summary>
        /// The GlobalUnlock function decrements the lock count associated with a memory object.
        /// </summary>
        /// <param name="hMem"></param>
        /// <returns></returns>
        [DllImport("Kernel32.dll")]
        private static extern bool GlobalUnlock(int hMem);

        public IntPtr Lock()
        {
            Debug.Assert(!locked);
            IntPtr ret = GlobalLock(stg.hGlobal);
            locked = true;

            return ret;
        }

        public void Unlock()
        {
            Debug.Assert(locked);
            Debug.Assert(stg.hGlobal != 0);
            GlobalUnlock(stg.hGlobal);
            locked = false;
        }
    }

    /// <summary>
    /// This is a replacement for System.Runtime.InteropServices.Marshal. It is only used in
    /// Debug builds. This class does not have all the functions from Marshal. If you need
    /// more, add them in the unchecked section.
    /// </summary>
    public class DebugMarshal
    {
        [Conditional("DEBUG")]
        public static void CheckAllFreed()
        {
#if DEBUG
            Debug.Assert(allocated.Count == 0);
#endif
        }

#if DEBUG
        static Dictionary<IntPtr, AllocationInfo> allocated = new Dictionary<IntPtr, AllocationInfo>();

        public static IntPtr AllocHGlobal(int cb)
        {
            IntPtr buffer = Marshal.AllocHGlobal(cb);
            allocated.Add(buffer, new AllocationInfo(AllocationType.HGlobal, cb));
            return buffer;
        }

        public static void FreeHGlobal(IntPtr hglobal)
        {
            AllocationInfo info;
            if (allocated.TryGetValue(hglobal, out info))
            {
                Debug.Assert(info.type == AllocationType.HGlobal);
                Debug.Assert(info.dotNetType == null);
            }
            else
                Debug.Assert(false, "Memory Management Error: HGlobal not allocated with .NET");
            Marshal.FreeHGlobal(hglobal);
            allocated.Remove(hglobal);
        }

        public static IntPtr AllocCoTaskMem(int cb)
        {
            IntPtr buffer = Marshal.AllocHGlobal(cb);
            allocated.Add(buffer, new AllocationInfo(AllocationType.CoTask, cb));
            return buffer;
        }

        public static void FreeCoTaskMem(IntPtr ptr)
        {
            AllocationInfo info;
            if (allocated.TryGetValue(ptr, out info))
            {
                Debug.Assert(info.type == AllocationType.CoTask);
                Debug.Assert(info.dotNetType == null);
            }
            else
                Debug.Assert(false, "Memory Management Error: CoMemory not allocated with .NET");
            Marshal.FreeCoTaskMem(ptr);
            allocated.Remove(ptr);
        }

        public static void StructureToPtr(object structure, IntPtr ptr, bool fDeleteOld)
        {
            AllocationInfo info;
            if (allocated.TryGetValue(ptr, out info))
            {
                Debug.Assert(fDeleteOld == (info.dotNetType != null));
            }
            else
            {
                info = new AllocationInfo(AllocationType.External, 0);
                allocated.Add(ptr, info);
            }

            //
            // ISSUE-What is the postcondition of StructureToPtr if it throws
            // when fDeleteOld is true?
            //
            Marshal.StructureToPtr(structure, ptr, fDeleteOld);
            info.dotNetType = structure.GetType();
        }

        public static void DestroyStructure(IntPtr ptr, Type structuretype)
        {
            AllocationInfo info;
            if (allocated.TryGetValue(ptr, out info))
            {
                Debug.Assert(info.dotNetType == structuretype);
            }
            else
                Debug.Assert(false, "Memory Management Error: Memory not allocated with .NET");
            Marshal.DestroyStructure(ptr, structuretype);
            info.dotNetType = null;
            if (info.type == AllocationType.External)
            {
                allocated.Remove(ptr);
            }
        }

        static void CheckPtrToStructure(IntPtr ptr, Type structureType)
        {
            AllocationInfo info;
            if (allocated.TryGetValue(ptr, out info))
            {
                //
                // dotNetType is null if some other code fills it up.
                // dotNetType needs to match the type if our code filled it up.
                //

                Debug.Assert(info.dotNetType == null || info.dotNetType == structureType);
            }
        }

        public static void PtrToStructure(IntPtr ptr, object structure)
        {
            CheckPtrToStructure(ptr, structure.GetType());
            Marshal.PtrToStructure(ptr, structure);
        }

        public static object PtrToStructure(IntPtr ptr, Type structureType)
        {
            CheckPtrToStructure(ptr, structureType);
            return Marshal.PtrToStructure(ptr, structureType);
        }

        #region Unchecked
        public static int ReadInt32(IntPtr ptr)
        {
            return Marshal.ReadInt32(ptr);
        }
        public static int SizeOf(object structure)
        {
            return Marshal.SizeOf(structure);
        }
        public static int SizeOf(Type t)
        {
            return Marshal.SizeOf(t);
        }
        public static int GetLastWin32Error()
        {
            return Marshal.GetLastWin32Error();
        }
        public static string PtrToStringUni(IntPtr ptr)
        {
            return Marshal.PtrToStringUni(ptr);
        }

        public static Exception GetExceptionForHR(int errorCode)
        {
            return Marshal.GetExceptionForHR(errorCode);
        }

        public static int GetHRForLastWin32Error()
        {
            return Marshal.GetHRForLastWin32Error();
        }

        #endregion
#endif
    }
}
