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
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows.Forms;
using Likewise.LMC.Utilities;
using Likewise.LMC.Registry;

namespace Likewise.LMC.Plugins.RegistryViewerPlugin
{
    public partial class ResourceListDialog : Form
    {
        #region Class Data

        private SubKeyValueInfo ValueInfo = null;
        private RegistryValueInfo regValueInfo = null;

        const int TYPE_RESOURCE_LIST = 8;
        const int TYPE_FULL_RESOURCE_DESCRIPTOR = 9;
        const int ErrorMoreDataIsAvailable = 234;
        int LargestStructSizeInUnion = sizeof(ulong) + sizeof(uint);
        private Hashtable ResourceListTable;

        #endregion

        #region Constructors

        public ResourceListDialog()
        {
            InitializeComponent();
        }

        public ResourceListDialog(object valueInfo, bool IsAdd)
            : this()
        {
            Logger.Log("ResourceListDialog, Inside Constructor", Logger.RegistryViewerLoglevel);

            if (valueInfo is SubKeyValueInfo)
                this.ValueInfo = valueInfo as SubKeyValueInfo;
            else
                this.regValueInfo = valueInfo as RegistryValueInfo;

            SetInputData();
        }

        #endregion

        #region Helper functions

        unsafe private void SetInputData()
        {
            Logger.Log("ResourceListDialog.SetInputData", Logger.RegistryViewerLoglevel);

            string[] sKey = null;
            object data = null;

            if (ValueInfo != null)
            {
                sKey = ValueInfo.sParentKey.ToString().Split(new char[] { '\\' } , 2);
                data = RegistryInteropWrapperWindows.RegGetValue(RegistryInteropWrapperWindows.GetRegistryHive(ValueInfo.hKey), sKey[1], ValueInfo.sValue, out ValueInfo.intDataType);
            }
            else
            {
                sKey = regValueInfo.sKeyname.ToString().Split(new char[] { '\\' } , 2);
                data = regValueInfo.bDataBuf;
            }

            if (data == null)
            {
                Logger.Log("ResourceListDialog.SetInputData - RegistryKey.GetValue returns null", Logger.RegistryViewerLoglevel);
                return;
            }

            try
            {
                int RESOURCE_LIST = 8;
                int FULL_RESOURCE_DESCRIPTOR = 9;
                byte[] buffer = data as byte[];
                RegResourceItem.ResourceList resourceList = null;
                fixed (byte* pBuffer = buffer)
                {
                    IntPtr p = (IntPtr)pBuffer;
                    resourceList = new RegResourceItem.ResourceList();
                    if ((ValueInfo != null && ValueInfo.intDataType == RESOURCE_LIST) ||
                        (regValueInfo != null && (int)regValueInfo.pType == RESOURCE_LIST))
                    {
                        RegResourceItem.RESOURCE_LIST_COUNT rCount = new RegResourceItem.RESOURCE_LIST_COUNT();
                        Marshal.PtrToStructure(p, rCount);
                        p = (IntPtr)(p.ToInt32() + Marshal.SizeOf(rCount));
                        for (int i = 0; i < rCount.Count; i++)
                        {
                            ExtractResourceDescriptor(resourceList, ref p);
                        }
                    }
                    else if ((ValueInfo != null && ValueInfo.intDataType == FULL_RESOURCE_DESCRIPTOR) ||
                        (regValueInfo != null && (int)regValueInfo.pType == FULL_RESOURCE_DESCRIPTOR))
                    {
                        ExtractResourceDescriptor(resourceList, ref p);
                    }
                }

                if (LWlvResourceList.Items.Count > 0)
                    btnDisplay.Enabled = true;
            }
            finally
            {
            }
        }

        unsafe private void ExtractResourceDescriptor(RegResourceItem.ResourceList resourceList, ref IntPtr p)
        {
            try
            {
                RegResourceItem.FULL_RESOURCE_DESCRIPTOR frd = new RegResourceItem.FULL_RESOURCE_DESCRIPTOR();
                Marshal.PtrToStructure(p, frd);
                p = (IntPtr)(p.ToInt32() + Marshal.SizeOf(frd));
                if (frd != null)
                {
                    RegResourceItem.FullResourceDescriptor fullResourceDescriptor = new RegResourceItem.FullResourceDescriptor(frd.InterfaceType, (int)frd.BusNumber);

                    for (int j = 0; j < frd.PartialResourceList.Count; j++)
                    {
                        RegResourceItem.PARTIAL_RESOURCE_DESCRIPTOR prd = new RegResourceItem.PARTIAL_RESOURCE_DESCRIPTOR();
                        Marshal.PtrToStructure(p, prd);

                        p = (IntPtr)(p.ToInt32() + (sizeof(byte) + sizeof(byte) + sizeof(ushort) + LargestStructSizeInUnion));

                        RegResourceItem.PartialResourceDescriptor partialResourceDescriptor = new RegResourceItem.PartialResourceDescriptor((RegResourceItem.ResourceType)prd.Type, prd.ShareDisposition, prd.Flags);

                        #region Parse Resource
                        switch ((RegResourceItem.ResourceType)prd.Type)
                        {
                            case RegResourceItem.ResourceType.Interrupt:
                                {
                                    // Interrupt resource type
                                    partialResourceDescriptor.Resource =
                                        new RegResourceItem.HardwareResource.Interrupt(prd.Interrupt.Level, prd.Interrupt.Vector, prd.Interrupt.Affinity);
                                    break;
                                }

                            case RegResourceItem.ResourceType.Memory:
                                {
                                    // Memory resource type
                                    partialResourceDescriptor.Resource =
                                        new RegResourceItem.HardwareResource.Memory(prd.Memory.Start, prd.Memory.Length);
                                    break;
                                }

                            case RegResourceItem.ResourceType.BusNumber:
                                {
                                    // BusNumber resource type
                                    partialResourceDescriptor.Resource =
                                        new RegResourceItem.HardwareResource.BusNumber(prd.BusNumber.Start, prd.BusNumber.Length, prd.BusNumber.Reserved);
                                    break;
                                }

                            case RegResourceItem.ResourceType.Dma:
                                {
                                    // Dma resource type
                                    partialResourceDescriptor.Resource =
                                        new RegResourceItem.HardwareResource.Dma(prd.Dma.Channel, prd.Dma.Port, prd.Dma.Reserved1);
                                    break;
                                }

                            case RegResourceItem.ResourceType.Null:
                                {
                                    // Generic resource type
                                    partialResourceDescriptor.Resource =
                                        new RegResourceItem.HardwareResource.Generic(prd.Generic.Start, prd.Generic.Length);
                                    break;
                                }

                            case RegResourceItem.ResourceType.Port:
                                {
                                    // Port resource type
                                    partialResourceDescriptor.Resource =
                                        new RegResourceItem.HardwareResource.Port(prd.Port.Start, prd.Port.Length);
                                    break;
                                }

                            case RegResourceItem.ResourceType.DevicePrivate:
                                {
                                    // DevicePrivate resource type
                                    partialResourceDescriptor.Resource =
                                        new RegResourceItem.HardwareResource.DevicePrivate(prd.DevicePrivate.Data);
                                    break;
                                }

                            case RegResourceItem.ResourceType.DeviceSpecific:
                                {
                                    // DeviceSpecific resource type
                                    partialResourceDescriptor.Resource =
                                        new RegResourceItem.HardwareResource.DeviceSpecificData(prd.DeviceSpecificData.DataSize, prd.DeviceSpecificData.Reserved1, prd.DeviceSpecificData.Reserved2);
                                    break;
                                }
                        }
                        #endregion

                        fullResourceDescriptor.PartialResourceDescriptors.Add(partialResourceDescriptor);
                    }

                    resourceList.FullResourceDescriptors.Add(fullResourceDescriptor);

                    string[] listOfItems = new string[3];
                    listOfItems[0] = ((int)frd.BusNumber).ToString();
                    string sInterface = RegResourceItem.GetInterfaceString(frd.InterfaceType);
                    listOfItems[1] = sInterface;
                    listOfItems[2] = frd.BusNumber.ToString() + sInterface;

                    if (ResourceListTable == null)
                    {
                        ResourceListTable = new Hashtable();
                        ResourceListTable.Add(listOfItems[2], fullResourceDescriptor);
                    }
                    else
                        ResourceListTable.Add(listOfItems[2], fullResourceDescriptor);

                    ListViewItem lvItem = new ListViewItem(listOfItems);
                    LWlvResourceList.Items.Add(lvItem);
                }
            }
            finally
            {
            }
        }

        #endregion

        #region Events

        private void btnOK_Click(object sender, EventArgs e)
        {
            this.Close();
        }

        private void btnDisplay_Click(object sender, EventArgs e)
        {
            // TODO : Need to check list box is selected or not, if selected enable Display button
            if (LWlvResourceList.SelectedItems.Count == 1)
            {
                foreach (ListViewItem item in LWlvResourceList.SelectedItems)
                {
                    string key = item.SubItems[2].Text;
                    if (ResourceListTable != null && ResourceListTable.ContainsKey(key))
                    {
                        RegResourceItem.FullResourceDescriptor frd = (RegResourceItem.FullResourceDescriptor)ResourceListTable[key];
                        if (frd != null)
                        {
                            DialogResult dlg;
                            ResourceDescriptorDialog ResListDialog = new ResourceDescriptorDialog(frd);
                            dlg = ResListDialog.ShowDialog(this);
                        }
                    }
                }
            }
            else
            {
                // None of the item is selected, do nothing.
                return;
            }
        }

        private void ResourceListDialog_Load(object sender, EventArgs e)
        {
            //Thrid column contains the resoure list details
            //This will be given to ResourceDescriptionDialog form as an input
            LWlvResourceList.Columns[2].Width = 0;
        }

        #endregion
    }
}