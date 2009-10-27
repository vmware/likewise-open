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
    public partial class LogOnWorkStationsEditor : Form
    {
        #region Control Variables

        private Button btnEdit;
        private LWListView listViewAttrValues;
        private ColumnHeader chAttributeValue;
        private TextBox txtAttrValue;
        private Button btnRemove;
        private Button btnAdd;
        private GroupBox groupBox1;
        private Label lblAttrValue;
        private RadioButton rbSelectedComp;
        private RadioButton rbAllComp;
        private Label lblselectedAttr;
        private Button buttonCancel;
        private Button buttonOk;
        private Label AttributeSelectionLabel;
        private Label lblAttr;

        #endregion

        #region Class Data

        public string sMultiValuedStringAttrValue = "";
        public string origsMultiValuedStringAttrValue = "";
        private char SplitChar = ',';
        private string strOldName = "";

        #endregion Variables

        #region Constructors
        public LogOnWorkStationsEditor()
        {
            InitializeComponent();
        }

        public LogOnWorkStationsEditor(string sValues)
            : this()
        {
            origsMultiValuedStringAttrValue = sValues;
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
            listViewAttrValues.Enabled = true;
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
                btnRemove.Enabled = btnEdit.Enabled = false;
            }
            else
            {
                for (int i = 0; i < listViewAttrValues.Items.Count; i++)
                {
                    if (listViewAttrValues.Items[i].Text.Trim().Equals(string.Empty))
                    {
                        listViewAttrValues.Items[i].Text = strOldName;
                    }
                }
                btnRemove.Enabled = btnEdit.Enabled = true;
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

                txtAttrValue.Focus();
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
            }
            else
            {
                if (rbAllComp.Checked)
                    sMultiValuedStringAttrValue = "";
                else
                {
                    sMultiValuedStringAttrValue = "";
                    for (int i = 0; i < listViewAttrValues.Items.Count; i++)
                    {
                        sMultiValuedStringAttrValue += (SplitChar.ToString() + listViewAttrValues.Items[i].Text.ToString());
                    }
                    if (sMultiValuedStringAttrValue.StartsWith(SplitChar.ToString()))
                    {
                        sMultiValuedStringAttrValue = sMultiValuedStringAttrValue.Substring(1);
                    }
                }
            }

            this.DialogResult = DialogResult.OK;
            this.Close();
        }

        /// <summary>
        /// Loads the listbox with the attribute value list
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void LogOnWorkStationsEditor_Load(object sender, EventArgs e)
        {
            if (!String.IsNullOrEmpty(origsMultiValuedStringAttrValue))
            {
                rbSelectedComp.Checked = true;
                groupBox1.Enabled = true;
                listViewAttrValues.Enabled = true;

                string[] sTempArr = origsMultiValuedStringAttrValue.Split(SplitChar);
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
                }
                txtAttrValue.Focus();
            }
            else
                rbAllComp.Checked = true;
        }

        private void btnEdit_Click(object sender, EventArgs e)
        {
            if (listViewAttrValues.SelectedIndices.Count == 0)
                return;

            btnEdit.Enabled = false;
            strOldName = listViewAttrValues.SelectedItems[0].Text;
            if (listViewAttrValues.SelectedItems.Count == 0)
            {
                return;
            }
            listViewAttrValues.LabelEdit = true;
            if (listViewAttrValues.SelectedItems[0].Text != null)
            {
                listViewAttrValues.SelectedItems[0].BeginEdit();
            }
        }

        private void rbSelectedComp_CheckedChanged(object sender, EventArgs e)
        {
            groupBox1.Enabled = rbSelectedComp.Checked;
            listViewAttrValues.Enabled = rbSelectedComp.Checked;
        }

        private void buttonCancel_Click(object sender, EventArgs e)
        {
            this.DialogResult = DialogResult.Cancel;
            this.Close();
        }

        private void rbAllComp_CheckedChanged(object sender, EventArgs e)
        {

        }

        #endregion Events

        #region InitializeComponent

        private void InitializeComponent()
        {
            this.btnEdit = new System.Windows.Forms.Button();
            this.txtAttrValue = new System.Windows.Forms.TextBox();
            this.btnRemove = new System.Windows.Forms.Button();
            this.btnAdd = new System.Windows.Forms.Button();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.listViewAttrValues = new Likewise.LMC.ServerControl.LWListView();
            this.chAttributeValue = new System.Windows.Forms.ColumnHeader();
            this.lblAttrValue = new System.Windows.Forms.Label();
            this.rbSelectedComp = new System.Windows.Forms.RadioButton();
            this.rbAllComp = new System.Windows.Forms.RadioButton();
            this.lblselectedAttr = new System.Windows.Forms.Label();
            this.buttonCancel = new System.Windows.Forms.Button();
            this.buttonOk = new System.Windows.Forms.Button();
            this.AttributeSelectionLabel = new System.Windows.Forms.Label();
            this.lblAttr = new System.Windows.Forms.Label();
            this.groupBox1.SuspendLayout();
            this.SuspendLayout();
            // 
            // btnEdit
            // 
            this.btnEdit.Enabled = false;
            this.btnEdit.Location = new System.Drawing.Point(276, 52);
            this.btnEdit.Name = "btnEdit";
            this.btnEdit.Size = new System.Drawing.Size(75, 22);
            this.btnEdit.TabIndex = 15;
            this.btnEdit.Text = "&Edit";
            this.btnEdit.UseVisualStyleBackColor = true;
            this.btnEdit.Click += new System.EventHandler(this.btnEdit_Click);
            // 
            // txtAttrValue
            // 
            this.txtAttrValue.Location = new System.Drawing.Point(12, 26);
            this.txtAttrValue.Name = "txtAttrValue";
            this.txtAttrValue.Size = new System.Drawing.Size(254, 20);
            this.txtAttrValue.TabIndex = 10;
            // 
            // btnRemove
            // 
            this.btnRemove.Enabled = false;
            this.btnRemove.Location = new System.Drawing.Point(276, 80);
            this.btnRemove.Name = "btnRemove";
            this.btnRemove.Size = new System.Drawing.Size(75, 22);
            this.btnRemove.TabIndex = 14;
            this.btnRemove.Text = "&Remove";
            this.btnRemove.UseVisualStyleBackColor = true;
            this.btnRemove.Click += new System.EventHandler(this.btnRemove_Click);
            // 
            // btnAdd
            // 
            this.btnAdd.Location = new System.Drawing.Point(276, 24);
            this.btnAdd.Name = "btnAdd";
            this.btnAdd.Size = new System.Drawing.Size(75, 22);
            this.btnAdd.TabIndex = 11;
            this.btnAdd.Text = "A&dd";
            this.btnAdd.UseVisualStyleBackColor = true;
            this.btnAdd.Click += new System.EventHandler(this.btnAdd_Click);
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.btnEdit);
            this.groupBox1.Controls.Add(this.listViewAttrValues);
            this.groupBox1.Controls.Add(this.btnRemove);
            this.groupBox1.Controls.Add(this.btnAdd);
            this.groupBox1.Controls.Add(this.txtAttrValue);
            this.groupBox1.Controls.Add(this.lblAttrValue);
            this.groupBox1.Enabled = false;
            this.groupBox1.Location = new System.Drawing.Point(10, 113);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(365, 263);
            this.groupBox1.TabIndex = 21;
            this.groupBox1.TabStop = false;
            // 
            // listViewAttrValues
            // 
            this.listViewAttrValues.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.chAttributeValue});
            this.listViewAttrValues.Enabled = false;
            this.listViewAttrValues.HeaderStyle = System.Windows.Forms.ColumnHeaderStyle.None;
            this.listViewAttrValues.HideSelection = false;
            this.listViewAttrValues.Location = new System.Drawing.Point(12, 52);
            this.listViewAttrValues.MultiSelect = false;
            this.listViewAttrValues.Name = "listViewAttrValues";
            this.listViewAttrValues.Size = new System.Drawing.Size(254, 203);
            this.listViewAttrValues.TabIndex = 13;
            this.listViewAttrValues.UseCompatibleStateImageBehavior = false;
            this.listViewAttrValues.View = System.Windows.Forms.View.Details;
            this.listViewAttrValues.SelectedIndexChanged += new System.EventHandler(this.listViewAttrValues_SelectedIndexChanged);
            // 
            // chAttributeValue
            // 
            this.chAttributeValue.Text = "AttributeValue";
            this.chAttributeValue.Width = 250;
            // 
            // lblAttrValue
            // 
            this.lblAttrValue.AutoEllipsis = true;
            this.lblAttrValue.AutoSize = true;
            this.lblAttrValue.Location = new System.Drawing.Point(9, 10);
            this.lblAttrValue.Name = "lblAttrValue";
            this.lblAttrValue.Size = new System.Drawing.Size(84, 13);
            this.lblAttrValue.TabIndex = 9;
            this.lblAttrValue.Text = "Computer name:";
            // 
            // rbSelectedComp
            // 
            this.rbSelectedComp.AutoEllipsis = true;
            this.rbSelectedComp.Location = new System.Drawing.Point(14, 92);
            this.rbSelectedComp.Name = "rbSelectedComp";
            this.rbSelectedComp.Size = new System.Drawing.Size(199, 17);
            this.rbSelectedComp.TabIndex = 20;
            this.rbSelectedComp.TabStop = true;
            this.rbSelectedComp.Text = "&The following computers";
            this.rbSelectedComp.UseVisualStyleBackColor = true;
            this.rbSelectedComp.CheckedChanged += new System.EventHandler(this.rbSelectedComp_CheckedChanged);
            // 
            // rbAllComp
            // 
            this.rbAllComp.Location = new System.Drawing.Point(14, 69);
            this.rbAllComp.Name = "rbAllComp";
            this.rbAllComp.Size = new System.Drawing.Size(140, 17);
            this.rbAllComp.TabIndex = 19;
            this.rbAllComp.TabStop = true;
            this.rbAllComp.Text = "All &computers";
            this.rbAllComp.UseVisualStyleBackColor = true;
            this.rbAllComp.CheckedChanged += new System.EventHandler(this.rbAllComp_CheckedChanged);
            // 
            // lblselectedAttr
            // 
            this.lblselectedAttr.AutoEllipsis = true;
            this.lblselectedAttr.AutoSize = true;
            this.lblselectedAttr.Location = new System.Drawing.Point(10, 47);
            this.lblselectedAttr.Name = "lblselectedAttr";
            this.lblselectedAttr.Size = new System.Drawing.Size(118, 13);
            this.lblselectedAttr.TabIndex = 16;
            this.lblselectedAttr.Text = "This user can log on to:";
            // 
            // buttonCancel
            // 
            this.buttonCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.buttonCancel.Location = new System.Drawing.Point(300, 386);
            this.buttonCancel.Name = "buttonCancel";
            this.buttonCancel.Size = new System.Drawing.Size(75, 22);
            this.buttonCancel.TabIndex = 18;
            this.buttonCancel.Text = "Cancel";
            this.buttonCancel.UseVisualStyleBackColor = true;
            this.buttonCancel.Click += new System.EventHandler(this.buttonCancel_Click);
            // 
            // buttonOk
            // 
            this.buttonOk.Location = new System.Drawing.Point(216, 386);
            this.buttonOk.Name = "buttonOk";
            this.buttonOk.Size = new System.Drawing.Size(75, 22);
            this.buttonOk.TabIndex = 17;
            this.buttonOk.Text = "OK";
            this.buttonOk.UseVisualStyleBackColor = true;
            this.buttonOk.Click += new System.EventHandler(this.buttonOk_Click);
            // 
            // AttributeSelectionLabel
            // 
            this.AttributeSelectionLabel.AutoSize = true;
            this.AttributeSelectionLabel.Location = new System.Drawing.Point(65, 12);
            this.AttributeSelectionLabel.Name = "AttributeSelectionLabel";
            this.AttributeSelectionLabel.Size = new System.Drawing.Size(0, 13);
            this.AttributeSelectionLabel.TabIndex = 15;
            // 
            // lblAttr
            // 
            this.lblAttr.AutoEllipsis = true;
            this.lblAttr.AutoSize = true;
            this.lblAttr.Location = new System.Drawing.Point(9, 8);
            this.lblAttr.Name = "lblAttr";
            this.lblAttr.Size = new System.Drawing.Size(346, 26);
            this.lblAttr.TabIndex = 14;
            this.lblAttr.Text = "This feature requires the NetBIOS protocol. In Computer name, type the \r\npre-Wind" +
                "ows 2000 computer name.";
            // 
            // LogOnWorkStationsEditor
            // 
            this.ClientSize = new System.Drawing.Size(386, 419);
            this.Controls.Add(this.groupBox1);
            this.Controls.Add(this.rbSelectedComp);
            this.Controls.Add(this.rbAllComp);
            this.Controls.Add(this.lblselectedAttr);
            this.Controls.Add(this.buttonCancel);
            this.Controls.Add(this.buttonOk);
            this.Controls.Add(this.AttributeSelectionLabel);
            this.Controls.Add(this.lblAttr);
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "LogOnWorkStationsEditor";
            this.ShowIcon = false;
            this.Text = "Logon Workstations";
            this.Load += new System.EventHandler(this.LogOnWorkStationsEditor_Load);
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion
    }
}
