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
using Likewise.LMC.ServerControl;

namespace Likewise.LMC.Plugins.ADUCPlugin
{
    public partial class MultiValuedBooleanEditor : Form
    {
        #region Variables
        ListViewItem lvSelectedItem;
        public string sMultiValuedBooleanAttrValue = "";
        public string origsMultiValuedBooleanAttrValue = "";
        #endregion Variables

        #region Constructors
        public MultiValuedBooleanEditor()
        {
            InitializeComponent();
        }
        public MultiValuedBooleanEditor(ListViewItem lvItem)
            : this()
        {
            this.lvSelectedItem = lvItem;
        }
        #endregion Constructors

        #region Events
        private void btnAdd_Click(object sender, EventArgs e)
        {
            ListViewItem lviArr = null;          
            
            if (rbtnTrue.Checked)
            {
                string[] values = { "TRUE" };
                lviArr = new ListViewItem(values);               
            }
            else if (rbtnFalse.Checked)
            {
                string[] values = { "FALSE" };
                lviArr = new ListViewItem(values);      
            }

            this.listViewAttrValues.Items.Add(lviArr);   
        }

        private void listViewAttrValues_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (listViewAttrValues.Items.Count == 0)
            {
                btnRemove.Enabled = false;
            }
            else
            {
                btnRemove.Enabled = true;
            }
        }

        private void btnRemove_Click(object sender, EventArgs e)
        {
            if (listViewAttrValues.SelectedIndices.Count == 0)
                return;

            int idx = listViewAttrValues.SelectedItems[0].Index;
            if (idx >= 0)
            {
                List<Object> ItemList = new List<Object>();
                int index = 0;

                for (index = 0; index < listViewAttrValues.Items.Count; index++)
                {
                    if (index != idx)
                    {
                        ItemList.Add(listViewAttrValues.Items[index]);
                    }
                }
                ListViewItem[] ItemsArr = new ListViewItem[ItemList.Count];
                index = 0;
                foreach (ListViewItem item in ItemList)
                {
                    ItemsArr[index] = item;
                    index++;
                }

                listViewAttrValues.Items.Clear();
                listViewAttrValues.Items.AddRange(ItemsArr);

                btnRemove.Enabled = false;
            }
        }

        private void buttonOk_Click(object sender, EventArgs e)
        {
            if (listViewAttrValues.Items.Count <= 0)
            {
                sMultiValuedBooleanAttrValue = "<Not Set>";
                this.DialogResult = DialogResult.OK;
                this.Close();
                return;
            }
            else
            {
                sMultiValuedBooleanAttrValue = "";
                for (int i = 0; i < listViewAttrValues.Items.Count; i++)
                {
                    sMultiValuedBooleanAttrValue += (";" + listViewAttrValues.Items[i].Text.ToString());
                }
                if (sMultiValuedBooleanAttrValue.StartsWith(";"))
                {
                    sMultiValuedBooleanAttrValue = sMultiValuedBooleanAttrValue.Substring(1);
                }
                this.DialogResult = DialogResult.OK;
                this.Close();
            }
        }

        private void buttonCancel_Click(object sender, EventArgs e)
        {
            this.Close();
        }

        private void MultiValuedBooleanEditor_Load(object sender, EventArgs e)
        {
            if (lvSelectedItem != null)
            {
                lblselectedAttr.Text = lvSelectedItem.SubItems[0].Text;
                string[] sTempArr = lvSelectedItem.SubItems[2].Text.ToString().Split(';');
                listViewAttrValues.Items.Clear();
                ListViewItem lviArr = null;
                for (int i = 0; i < sTempArr.Length; i++)
                {
                    if (!sTempArr[i].Equals("<Not Set>"))
                    {                        
                        string[] values = { sTempArr[i].ToString() };
                        lviArr = new ListViewItem(values);
                        this.listViewAttrValues.Items.Add(lviArr);  
                    }
                    origsMultiValuedBooleanAttrValue += (";" + sTempArr[i].ToString());
                }
                if (origsMultiValuedBooleanAttrValue.StartsWith(";"))
                {
                    origsMultiValuedBooleanAttrValue = origsMultiValuedBooleanAttrValue.Substring(1);
                }

                rbtnTrue.Focus();
            }
        }
        #endregion Events
    }
}
