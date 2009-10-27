/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;
using Likewise.LMC.Registry;

namespace Likewise.LMC.Plugins.RegistryViewerPlugin
{
    public class RegResourceItem
    {
        public enum InterfaceType
        {
            InterfaceTypeUndefined = -1,
            Internal,
            Isa,
            Eisa,
            MicroChannel,
            TurboChannel,
            PCIBus,
            VMEBus,
            NuBus,
            PCMCIABus,
            CBus,
            MPIBus,
            MPSABus,
            ProcessorInternal,
            InternalPowerBus,
            PNPISABus,
            PNPBus,
            MaximumInterfaceType,
        }

        public enum ResourceType
        {
            Null,
            Port,
            Interrupt,
            Memory,
            Dma,
            DeviceSpecific,
            BusNumber,
            Maximum,
            AssignedResource,
            SubAllocateFrom,
            NonArbitrated = 128,
            ConfigData = 128,
            DevicePrivate = 129,
            PcCardConfig = 130,
            MfCardConfig = 131,
        }

        public static string GetInterfaceString(InterfaceType type)
        {
            string sType = string.Empty;
            switch (type)
            {
                case InterfaceType.CBus:
                    sType = "CBus";
                    break;
                case InterfaceType.Internal:
                    sType = "Internal";
                    break;
                case InterfaceType.Isa:
                    sType = "Isa";
                    break;
                default:
                    sType = "Invalid";
                    break;
            }
            return sType;
        }

        [StructLayout(LayoutKind.Sequential)]
        public class RESOURCE_LIST_COUNT
        {
            public uint Count;
        }

        [StructLayout(LayoutKind.Sequential)]
        public class PARTIAL_RESOURCE_LIST
        {
            public ushort Version;
            public ushort Revision;
            public uint Count;
        }

        public class RESOURCE_DESCRIPTOR
        {
            public InterfaceType InterfaceType;
            public uint BusNumber;
            public PARTIAL_RESOURCE_LIST PartialResourceList;
        }

        public class FULL_RESOURCE_DESCRIPTOR
        {
            public InterfaceType InterfaceType;
            public uint BusNumber;

            public PARTIAL_RESOURCE_LIST PartialResourceList;
        }
      
        public class PARTIAL_RESOURCE_DESCRIPTOR
        {          
            public class GenericResource
            {
                public ulong Start;
                public uint Length;
            }

            [StructLayout(LayoutKind.Sequential)]
            public class DmaResource
            {
                public uint Channel;
                public uint Port;
                public uint Reserved1;
            }

            public class InterruptResource
            {
                public uint Level;
                public uint Vector;
                public uint Affinity;
            }
         
            public class PortResource
            {
                public ulong Start;
                public uint Length;
            }
          
            public class MemoryResource
            {
                public ulong Start;
                public uint Length;
            }
         
            public class DevicePrivateResource
            {
                // DevicePrivate;
                [MarshalAs(UnmanagedType.ByValArray, SizeConst = 3)]
                public uint[] Data;
            }
          
            public class BusNumberResource
            {
                public uint Start;
                public uint Length;
                public uint Reserved;
            }
       
            public class DeviceSpecificDataResource
            {
                public uint DataSize;
                public uint Reserved1;
                public uint Reserved2;
            }
          
            public byte Type;
            public byte ShareDisposition;
            public ushort Flags;           
        
            public GenericResource Generic;        
            public PortResource Port;     
            public MemoryResource Memory;     
            public DmaResource Dma;   
            public InterruptResource Interrupt;      
            public DevicePrivateResource DevicePrivate; 
            public BusNumberResource BusNumber;       
            public DeviceSpecificDataResource DeviceSpecificData;           
        }

        public class ResourceList
        {
            private List<FullResourceDescriptor> fullResourceDescriptors = new List<FullResourceDescriptor>();

            public List<FullResourceDescriptor> FullResourceDescriptors
            {
                get
                {
                    return this.fullResourceDescriptors;
                }
            }
        }

        public class HardwareResource
        {
            protected ResourceType type;

            protected HardwareResource(ResourceType type)
            {
                this.type = type;
            }

            public ResourceType Type
            {
                get { return this.type; }
            }

            #region Nested Classes

            public class Generic : HardwareResource
            {
                ulong start;
                uint length;

                internal Generic(ulong start, uint length)
                    : base(ResourceType.Null)
                {
                    this.start = start;
                    this.length = length;
                }

                public ulong Start
                {
                    get { return this.start; }
                }

                public uint Length
                {
                    get { return this.length; }
                }
            }

            public class Memory : HardwareResource
            {
                ulong start;
                uint length;

                internal Memory(ulong start, uint length)
                    : base(ResourceType.Memory)
                {
                    this.start = start;
                    this.length = length;
                }

                public ulong Start
                {
                    get { return this.start; }
                }

                public uint Length
                {
                    get { return this.length; }
                }
            }

            public class Port : HardwareResource
            {
                ulong start;
                uint length;

                internal Port(ulong start, uint length)
                    : base(ResourceType.Port)
                {
                    this.start = start;
                    this.length = length;
                }

                public ulong Start
                {
                    get { return this.start; }
                }

                public uint Length
                {
                    get { return this.length; }
                }
            }

            public class Dma : HardwareResource
            {
                uint channel;
                uint port;
                uint reserved1;

                internal Dma(uint channel, uint port, uint reserved1)
                    : base(ResourceType.Dma)
                {
                    this.channel = channel;
                    this.port = port;
                    this.reserved1 = reserved1;
                }

                public uint Channel
                {
                    get { return this.channel; }
                }

                public new uint Port
                {
                    get { return this.port; }
                }

                public uint Reserved1
                {
                    get { return this.reserved1; }
                }
            }

            public class Interrupt : HardwareResource
            {
                uint level;
                uint vector;
                uint affinity;

                internal Interrupt(uint level, uint vector, uint affinity)
                    : base(ResourceType.Interrupt)
                {
                    this.level = level;
                    this.vector = vector;
                    this.affinity = affinity;
                }

                public uint Level
                {
                    get { return this.level; }
                }

                public uint Vector
                {
                    get { return this.vector; }
                }

                public uint Affinity
                {
                    get { return this.affinity; }
                }
            }

            public class DevicePrivate : HardwareResource
            {
                uint[] data;

                internal DevicePrivate(uint[] data)
                    : base(ResourceType.DevicePrivate)
                {
                    this.data = data;
                }

                public uint[] Data
                {
                    get { return this.data; }
                }
            }

            public class BusNumber : HardwareResource
            {
                uint start;
                uint length;
                uint reserved;

                internal BusNumber(uint start, uint length, uint reserved)
                    : base(ResourceType.BusNumber)
                {
                    this.start = start;
                    this.length = length;
                    this.reserved = reserved;
                }

                public uint Start
                {
                    get { return this.start; }
                }

                public uint Length
                {
                    get { return this.length; }
                }

                public uint Reserved
                {
                    get { return this.reserved; }
                }
            }

            public class DeviceSpecificData : HardwareResource
            {
                uint dataSize;
                uint reserved1;
                uint reserved2;

                internal DeviceSpecificData(uint dataSize, uint reserved1, uint reserved2)
                    : base(ResourceType.DeviceSpecific)
                {
                    this.dataSize = dataSize;
                    this.reserved1 = reserved1;
                    this.reserved2 = reserved2;
                }

                public uint DataSize
                {
                    get { return this.dataSize; }
                }

                public uint Reserved1
                {
                    get { return this.reserved1; }
                }

                public uint Reserved2
                {
                    get { return this.reserved2; }
                }
            }

            #endregion
        }

        public class PartialResourceDescriptor
        {
            ResourceType type;
            int shareDisposition;
            int flags;
            HardwareResource resource;

            internal PartialResourceDescriptor(ResourceType type, int shareDisposition, int flags)
            {
                this.type = type;
                this.shareDisposition = shareDisposition;
                this.flags = flags;
            }

            public int ShareDisposition
            {
                get { return this.shareDisposition; }
            }

            public ResourceType Type
            {
                get { return this.type; }
            }

            public int Flags
            {
                get { return this.flags; }
            }

            public HardwareResource Resource
            {
                get { return this.resource; }
                internal set { this.resource = value; }
            }
        }

        public class FullResourceDescriptor
        {
            private List<PartialResourceDescriptor> partialResourceDescriptors = new List<PartialResourceDescriptor>();
            private InterfaceType interfaceType;
            private int busNumber;

            internal FullResourceDescriptor(InterfaceType interfaceType, int busNumber)
            {
                this.interfaceType = interfaceType;
                this.busNumber = busNumber;
            }

            public InterfaceType InterfaceType
            {
                get { return this.interfaceType; }
            }

            public int BusNumber
            {
                get { return this.busNumber; }
            }

            public List<PartialResourceDescriptor> PartialResourceDescriptors
            {
                get { return this.partialResourceDescriptors; }
            }
        }
    }
}
