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
    public partial class MultiValuedOctetEditor : Form
    {
        #region Variables
        ListViewItem lvSelectedItem;
        public string sMultiValuedOctetAttrValue = "";
        public string origsMultiValuedOctetAttrValue = "";
        #endregion Variables

        #region Constructors
        public MultiValuedOctetEditor()
        {
            InitializeComponent();
        }
        public MultiValuedOctetEditor(ListViewItem lvItem)
            : this()
        {
            this.lvSelectedItem = lvItem;
        }
        #endregion Constructors

        #region Events

        /// <summary>
        /// Adds the new entry in the listbox if it is not exists
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void btnAdd_Click(object sender, EventArgs e)
        {
            OctetStringAttributeEditor _octStringAttrForm = new OctetStringAttributeEditor();
            if (_octStringAttrForm.ShowDialog(this) == DialogResult.OK)
            {
                if (_octStringAttrForm.sOctextStringAttrValue != "")
                {
                    ListViewItem lviArr = null;
                    string[] values = { _octStringAttrForm.sOctextStringAttrValue };
                    lviArr = new ListViewItem(values);
                    this.listViewAttrValues.Items.Add(lviArr);
                }
            }
        }

        private void listViewAttrValues_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (listViewAttrValues.SelectedItems.Count == 0)
            {
                btnRemove.Enabled = false;
                btnEdit.Enabled = false;
            }
            else
            {
                btnRemove.Enabled = true;
                btnEdit.Enabled = true;
            }
        }


        /// <summary>
        /// Removes the selected entry from the listbox
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
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
                btnEdit.Enabled = false;
            }
        }

        /// <summary>
        /// Allows us to edit the selected listbox item
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void btnEdit_Click(object sender, EventArgs e)
        {
            if (listViewAttrValues.SelectedIndices.Count == 0)
                return;

            string sListBoxValue = listViewAttrValues.Items[listViewAttrValues.SelectedItems[0].Index].Text.ToString();

            OctetStringAttributeEditor _octStringAttrForm = new OctetStringAttributeEditor(lvSelectedItem.SubItems[0].Text, sListBoxValue);
            if (_octStringAttrForm.ShowDialog(this) == DialogResult.OK)
            {
                if (_octStringAttrForm.sOctextStringAttrValue != "")
                {
                    listViewAttrValues.Items.RemoveAt(listViewAttrValues.SelectedItems[0].Index);
                    listViewAttrValues.Items.Add(_octStringAttrForm.sOctextStringAttrValue);
                }
                btnEdit.Enabled = false;
            }
        }

        /// <summary>
        /// Gives the comma seperated string with all listbox items
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void buttonOk_Click(object sender, EventArgs e)
        {
            if (listViewAttrValues.Items.Count <= 0)
            {
                sMultiValuedOctetAttrValue = "<Not Set>";
                this.DialogResult = DialogResult.OK;
                this.Close();
                return;
            }
            else
            {
                sMultiValuedOctetAttrValue = "";
                for (int i = 0; i < listViewAttrValues.Items.Count; i++)
                {
                    sMultiValuedOctetAttrValue += (";" + listViewAttrValues.Items[i].Text.ToString());
                }
                if (sMultiValuedOctetAttrValue.StartsWith(";"))
                {
                    sMultiValuedOctetAttrValue = sMultiValuedOctetAttrValue.Substring(1);
                }
                this.DialogResult = DialogResult.OK;
                this.Close();
            }
        }

        private void buttonCancel_Click(object sender, EventArgs e)
        {
            this.Close();
        }

        /// <summary>
        /// Loads the listbox with the attribute value list
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void MultiValuedOctetEditor_Load(object sender, EventArgs e)
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
                    origsMultiValuedOctetAttrValue += (";" + sTempArr[i].ToString());
                }
                if (origsMultiValuedOctetAttrValue.StartsWith(";"))
                {
                    origsMultiValuedOctetAttrValue = origsMultiValuedOctetAttrValue.Substring(1);
                }
                listViewAttrValues.Focus();
            }
        }
        #endregion Events
    }
}
