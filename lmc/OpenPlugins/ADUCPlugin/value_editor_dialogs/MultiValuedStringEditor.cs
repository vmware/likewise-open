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
using System.Text.RegularExpressions;

namespace Likewise.LMC.Plugins.ADUCPlugin
{
    public partial class MultiValuedStringEditor : Form
    {
        #region Variables
        ListViewItem lvSelectedItem;
        public string sMultiValuedStringAttrValue = "";
        public string origsMultiValuedStringAttrValue = "";
        private bool bIsInt = false;
        Regex regNum = new Regex("^[-]?([0-9])+$");
        #endregion Variables

        #region Constructors
        public MultiValuedStringEditor()
        {
            InitializeComponent();
        }
        public MultiValuedStringEditor(ListViewItem lvItem, bool IsInt)
            : this()
        {
            this.lvSelectedItem = lvItem;
            this.bIsInt = IsInt;
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
            if (bIsInt)
            {
                string value = txtAttrValue.Text.Trim();
                if ((value.Contains("-") && !value.StartsWith("-")) || !regNum.IsMatch(value))
                {
                    MessageBox.Show(this,
                    "The value entered can contain only digits between 0 and 9.\nIf a negative value is desired a minus sign can be placed \nas the first character.",
                    CommonResources.GetString("Caption_Console"), MessageBoxButtons.OK, MessageBoxIcon.Warning);
                    return;
                }
            }

            ListViewItem lviArr = null;
            if (!txtAttrValue.Text.ToString().Trim().Equals(string.Empty))
            {
                for (int i = 0; i < listViewAttrValues.Items.Count; i++)
                {
                    if (listViewAttrValues.Items[i].Text.Trim().Equals(txtAttrValue.Text.ToString().Trim()))
                    {
                        DialogResult dlg = MessageBox.Show(
                        this,
                        "This value already exists in the list. Do you want to add it anyway?",
                        CommonResources.GetString("Caption_Console"),
                        MessageBoxButtons.OKCancel,
                        MessageBoxIcon.Asterisk);
                        if (dlg == DialogResult.OK)
                        {
                            break;
                        }
                        else
                        {
                            return;
                        }
                    }
                }
                string[] values = { txtAttrValue.Text.ToString().Trim() };
                lviArr = new ListViewItem(values);
                this.listViewAttrValues.Items.Add(lviArr);
            }
            txtAttrValue.Text = "";
        }

        private void listViewAttrValues_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (listViewAttrValues.SelectedItems.Count == 0)
            {
                btnRemove.Enabled = false;
            }
            else
            {
                btnRemove.Enabled = true;
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
                txtAttrValue.Text = listViewAttrValues.Items[idx].Text.ToString();
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

        /// <summary>
        /// Gives the comma seperated string with all listbox items
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void buttonOk_Click(object sender, EventArgs e)
        {
            if (listViewAttrValues.Items.Count <= 0)
            {
                this.sMultiValuedStringAttrValue = "<Not Set>";
                this.DialogResult = DialogResult.OK;
                this.Close();
                return;
            }
            else
            {
                sMultiValuedStringAttrValue = "";
                for (int i = 0; i < listViewAttrValues.Items.Count; i++)
                {
                    sMultiValuedStringAttrValue += (";" + listViewAttrValues.Items[i].Text.ToString());
                }
                if (sMultiValuedStringAttrValue.StartsWith(";"))
                {
                    sMultiValuedStringAttrValue = sMultiValuedStringAttrValue.Substring(1);
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
        private void MultiValuedStringEditor_Load(object sender, EventArgs e)
        {
            if (lvSelectedItem != null)
            {
                lblselectedAttr.Text = lvSelectedItem.SubItems[0].Text;
                string[] sTempArr = lvSelectedItem.SubItems[2].Text.ToString().Split(';');
                this.listViewAttrValues.Items.Clear();
                ListViewItem lviArr = null;
                for (int i = 0; i < sTempArr.Length; i++)
                {
                    if (!sTempArr[i].Equals("<Not Set>"))
                    {
                        string[] values = { sTempArr[i].ToString() };
                        lviArr = new ListViewItem(values);
                        this.listViewAttrValues.Items.Add(lviArr);
                    }
                    origsMultiValuedStringAttrValue += (";" + sTempArr[i].ToString());
                }
                if (origsMultiValuedStringAttrValue.StartsWith(";"))
                {
                    origsMultiValuedStringAttrValue = origsMultiValuedStringAttrValue.Substring(1);
                }
                txtAttrValue.Focus();
            }
        }

        #endregion Events
    }
}
