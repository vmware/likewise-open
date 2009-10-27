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
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using Likewise.LMC.LMConsoleUtils;
using Likewise.LMC.Registry;

namespace Likewise.LMC.Plugins.RegistryViewerPlugin
{
    public partial class ResourceDescriptorDialog : Form
    {
        #region Constructors
        public ResourceDescriptorDialog()
        {
            InitializeComponent();
        }

        public ResourceDescriptorDialog(RegResourceItem.FullResourceDescriptor frd)
            : this()
        {
            Logger.Log("ResourceListDialog, Inside Constructor", Logger.RegistryViewerLoglevel);
            SetInputData(frd);
        }
        #endregion

        #region Helper Functions
        private void SetInputData(RegResourceItem.FullResourceDescriptor frd)
        {
            IntTypeAnsLabel.Text = RegResourceItem.GetInterfaceString(frd.InterfaceType);
            BusNumAnsLabel.Text = frd.BusNumber.ToString();

            foreach (RegResourceItem.PartialResourceDescriptor prd in frd.PartialResourceDescriptors)
            {
                string[] listOfItems = null;
                ListViewItem lvItem = null;
                switch (prd.Type)
                {
                    case RegResourceItem.ResourceType.Dma:
                        break;
                    case RegResourceItem.ResourceType.Interrupt:
                        listOfItems = new string[4];
                        RegResourceItem.HardwareResource.Interrupt hri = (RegResourceItem.HardwareResource.Interrupt)prd.Resource;
                        //Vector
                        listOfItems[0] = hri.Vector.ToString();
                        //Level
                        listOfItems[1] = hri.Level.ToString();
                        //Affinity
                        listOfItems[2] = "0x" + RegistryUtils.DecimalToBase(hri.Affinity, 16);
                        //Type
                        listOfItems[3] = "Level Sensitive";     // Need to check this can be hardcorded or not

                        lvItem = new ListViewItem(listOfItems);
                        LWlvInterrupt.Items.Add(lvItem);
                        break;
                    case RegResourceItem.ResourceType.Memory:
                        listOfItems = new string[3];
                        RegResourceItem.HardwareResource.Memory hrm = (RegResourceItem.HardwareResource.Memory)prd.Resource;
                        //Physical Address
                        listOfItems[0] = "0x" + RegistryUtils.DecimalToBase(hrm.Start,16);
                        //Length
                        listOfItems[1] = "0x" + RegistryUtils.DecimalToBase(hrm.Length, 16);
                        //Access
                        listOfItems[2] = "Unknown";     // How to get this ?

                        lvItem = new ListViewItem(listOfItems);
                        LWlvMemory.Items.Add(lvItem);
                        break;

                    case RegResourceItem.ResourceType.Port:
                        listOfItems = new string[3];
                        RegResourceItem.HardwareResource.Port hrp = (RegResourceItem.HardwareResource.Port)prd.Resource;
                        //Physical Address
                        listOfItems[0] = "0x" + RegistryUtils.DecimalToBase(hrp.Start, 16);
                        //Length
                        listOfItems[1] = "0x" + RegistryUtils.DecimalToBase(hrp.Length, 16);
                        //Type
                        listOfItems[2] = "Port";     // How to get this ?

                        lvItem = new ListViewItem(listOfItems);
                        LWlvPort.Items.Add(lvItem);
                        break;
                    case RegResourceItem.ResourceType.DeviceSpecific:
                        break;
                }
            }
        }

        private void btnOk_Click(object sender, EventArgs e)
        {
            this.Close();
        }
        #endregion

        #region Event Handlers
        private void LWlv_SelectedIndexChanged(object sender, EventArgs e)
        {   
            ListView lv = (ListView)sender;
            switch (lv.Name)
            {
                case "LWlvMemory":
                    DeviceLabel.Enabled = true;
                    DriverLabel.Enabled = false;
                    break;
                case "LWlvInterrupt":
                case "LWlvPort":
                    DeviceLabel.Enabled = false;
                    DriverLabel.Enabled = true;
                    break;
                default:
                    DeviceLabel.Enabled = DriverLabel.Enabled = false;
                    break;
            }
        }

        private void LWlv_Resize(object sender, EventArgs e)
        {
        }
        #endregion
    }
}