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

namespace Likewise.LMC.Plugins.ADUCPlugin
{
    partial class MultiItemsAddressEditPage
    {
        /// <summary> 
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary> 
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Component Designer generated code

        /// <summary> 
        /// Required method for Designer support - do not modify 
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.lblDisplay = new System.Windows.Forms.Label();
            this.chkStreet = new System.Windows.Forms.CheckBox();
            this.chkPO = new System.Windows.Forms.CheckBox();
            this.chkCity = new System.Windows.Forms.CheckBox();
            this.chkState = new System.Windows.Forms.CheckBox();
            this.chkZip = new System.Windows.Forms.CheckBox();
            this.chkCountry = new System.Windows.Forms.CheckBox();
            this.txtPOBox = new System.Windows.Forms.TextBox();
            this.txtCity = new System.Windows.Forms.TextBox();
            this.txtState = new System.Windows.Forms.TextBox();
            this.txtZip = new System.Windows.Forms.TextBox();
            this.cbCountry = new System.Windows.Forms.ComboBox();
            this.lbStreet = new System.Windows.Forms.Label();
            this.lbPOBox = new System.Windows.Forms.Label();
            this.lbCity = new System.Windows.Forms.Label();
            this.lbState = new System.Windows.Forms.Label();
            this.lbZip = new System.Windows.Forms.Label();
            this.lbCountry = new System.Windows.Forms.Label();
            this.txtStreet = new System.Windows.Forms.RichTextBox();
            this.pnlData.SuspendLayout();
            this.SuspendLayout();
            // 
            // pnlData
            // 
            this.pnlData.Controls.Add(this.txtStreet);
            this.pnlData.Controls.Add(this.lbCountry);
            this.pnlData.Controls.Add(this.lbZip);
            this.pnlData.Controls.Add(this.lbState);
            this.pnlData.Controls.Add(this.lbCity);
            this.pnlData.Controls.Add(this.lbPOBox);
            this.pnlData.Controls.Add(this.lbStreet);
            this.pnlData.Controls.Add(this.cbCountry);
            this.pnlData.Size = new System.Drawing.Size(392, 345);
            // 
            // lblDisplay
            // 
            this.lblDisplay.Location = new System.Drawing.Point(20, 16);
            this.lblDisplay.Name = "lblDisplay";
            this.lblDisplay.Size = new System.Drawing.Size(363, 31);
            this.lblDisplay.TabIndex = 0;
            this.lblDisplay.Text = "To change a property for multiple objects, first select the checkbox to enable th" +
                "e change, and then type the change.";
            // 
            // chkStreet
            // 
            this.chkStreet.Location = new System.Drawing.Point(7, 86);
            this.chkStreet.Name = "chkStreet";
            this.chkStreet.Size = new System.Drawing.Size(84, 16);
            this.chkStreet.TabIndex = 1;
            this.chkStreet.Text = "&Street:";
            this.chkStreet.UseVisualStyleBackColor = true;
            this.chkStreet.CheckedChanged += new System.EventHandler(this.chkStreet_CheckedChanged);
            // 
            // chkPO
            // 
            this.chkPO.Location = new System.Drawing.Point(7, 170);
            this.chkPO.Name = "chkPO";
            this.chkPO.Size = new System.Drawing.Size(84, 17);
            this.chkPO.TabIndex = 3;
            this.chkPO.Text = "P.O. &Box:";
            this.chkPO.UseVisualStyleBackColor = true;
            this.chkPO.CheckedChanged += new System.EventHandler(this.chkPO_CheckedChanged);
            // 
            // chkCity
            // 
            this.chkCity.Location = new System.Drawing.Point(7, 202);
            this.chkCity.Name = "chkCity";
            this.chkCity.Size = new System.Drawing.Size(78, 19);
            this.chkCity.TabIndex = 4;
            this.chkCity.Text = "&City:";
            this.chkCity.UseVisualStyleBackColor = true;
            this.chkCity.CheckedChanged += new System.EventHandler(this.chkCity_CheckedChanged);
            // 
            // chkState
            // 
            this.chkState.Location = new System.Drawing.Point(7, 230);
            this.chkState.Name = "chkState";
            this.chkState.Size = new System.Drawing.Size(104, 30);
            this.chkState.TabIndex = 5;
            this.chkState.Text = "State/pro&vince:";
            this.chkState.UseVisualStyleBackColor = true;
            this.chkState.CheckedChanged += new System.EventHandler(this.chkState_CheckedChanged);
            // 
            // chkZip
            // 
            this.chkZip.Location = new System.Drawing.Point(7, 263);
            this.chkZip.Name = "chkZip";
            this.chkZip.Size = new System.Drawing.Size(106, 27);
            this.chkZip.TabIndex = 6;
            this.chkZip.Text = "&Zip/Postal Code:";
            this.chkZip.UseVisualStyleBackColor = true;
            this.chkZip.CheckedChanged += new System.EventHandler(this.chkZip_CheckedChanged);
            // 
            // chkCountry
            // 
            this.chkCountry.Location = new System.Drawing.Point(7, 296);
            this.chkCountry.Name = "chkCountry";
            this.chkCountry.Size = new System.Drawing.Size(104, 28);
            this.chkCountry.TabIndex = 7;
            this.chkCountry.Text = "C&ountry/region:";
            this.chkCountry.UseVisualStyleBackColor = true;
            this.chkCountry.CheckedChanged += new System.EventHandler(this.chkCountry_CheckedChanged);
            // 
            // txtPOBox
            // 
            this.txtPOBox.Enabled = false;
            this.txtPOBox.Location = new System.Drawing.Point(112, 170);
            this.txtPOBox.MaxLength = 40;
            this.txtPOBox.Name = "txtPOBox";
            this.txtPOBox.Size = new System.Drawing.Size(269, 20);
            this.txtPOBox.TabIndex = 8;
            this.txtPOBox.TextChanged += new System.EventHandler(this.txtPOBox_TextChanged);
            // 
            // txtCity
            // 
            this.txtCity.Enabled = false;
            this.txtCity.Location = new System.Drawing.Point(112, 202);
            this.txtCity.MaxLength = 128;
            this.txtCity.Name = "txtCity";
            this.txtCity.Size = new System.Drawing.Size(269, 20);
            this.txtCity.TabIndex = 9;
            this.txtCity.TextChanged += new System.EventHandler(this.txtCity_TextChanged);
            // 
            // txtState
            // 
            this.txtState.Enabled = false;
            this.txtState.Location = new System.Drawing.Point(112, 237);
            this.txtState.MaxLength = 128;
            this.txtState.Name = "txtState";
            this.txtState.Size = new System.Drawing.Size(269, 20);
            this.txtState.TabIndex = 10;
            this.txtState.TextChanged += new System.EventHandler(this.txtState_TextChanged);
            // 
            // txtZip
            // 
            this.txtZip.Enabled = false;
            this.txtZip.Location = new System.Drawing.Point(112, 270);
            this.txtZip.MaxLength = 40;
            this.txtZip.Name = "txtZip";
            this.txtZip.Size = new System.Drawing.Size(269, 20);
            this.txtZip.TabIndex = 11;
            this.txtZip.TextChanged += new System.EventHandler(this.txtZip_TextChanged);
            // 
            // cbCountry
            // 
            this.cbCountry.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cbCountry.Enabled = false;
            this.cbCountry.FormattingEnabled = true;
            this.cbCountry.Location = new System.Drawing.Point(114, 304);
            this.cbCountry.Name = "cbCountry";
            this.cbCountry.Size = new System.Drawing.Size(266, 21);
            this.cbCountry.Sorted = true;
            this.cbCountry.TabIndex = 0;
            this.cbCountry.SelectedIndexChanged += new System.EventHandler(this.cbCountry_SelectedIndexChanged);
            // 
            // lbStreet
            // 
            this.lbStreet.AutoSize = true;
            this.lbStreet.Location = new System.Drawing.Point(4, 84);
            this.lbStreet.Name = "lbStreet";
            this.lbStreet.Size = new System.Drawing.Size(38, 13);
            this.lbStreet.TabIndex = 12;
            this.lbStreet.Text = "&Street:";
            this.lbStreet.Visible = false;
            // 
            // lbPOBox
            // 
            this.lbPOBox.AutoSize = true;
            this.lbPOBox.Location = new System.Drawing.Point(4, 171);
            this.lbPOBox.Name = "lbPOBox";
            this.lbPOBox.Size = new System.Drawing.Size(52, 13);
            this.lbPOBox.TabIndex = 13;
            this.lbPOBox.Text = "P.O. &Box:";
            this.lbPOBox.Visible = false;
            // 
            // lbCity
            // 
            this.lbCity.AutoSize = true;
            this.lbCity.Location = new System.Drawing.Point(4, 205);
            this.lbCity.Name = "lbCity";
            this.lbCity.Size = new System.Drawing.Size(27, 13);
            this.lbCity.TabIndex = 14;
            this.lbCity.Text = "&City:";
            this.lbCity.Visible = false;
            // 
            // lbState
            // 
            this.lbState.AutoSize = true;
            this.lbState.Location = new System.Drawing.Point(4, 240);
            this.lbState.Name = "lbState";
            this.lbState.Size = new System.Drawing.Size(81, 13);
            this.lbState.TabIndex = 12;
            this.lbState.Text = "State/pro&vince:";
            this.lbState.Visible = false;
            // 
            // lbZip
            // 
            this.lbZip.AutoSize = true;
            this.lbZip.Location = new System.Drawing.Point(4, 274);
            this.lbZip.Name = "lbZip";
            this.lbZip.Size = new System.Drawing.Size(87, 13);
            this.lbZip.TabIndex = 15;
            this.lbZip.Text = "&Zip/Postal Code:";
            this.lbZip.Visible = false;
            // 
            // lbCountry
            // 
            this.lbCountry.AutoSize = true;
            this.lbCountry.Location = new System.Drawing.Point(5, 308);
            this.lbCountry.Name = "lbCountry";
            this.lbCountry.Size = new System.Drawing.Size(80, 13);
            this.lbCountry.TabIndex = 16;
            this.lbCountry.Text = "C&ountry/region:";
            this.lbCountry.Visible = false;
            // 
            // txtStreet
            // 
            this.txtStreet.AcceptsTab = true;
            this.txtStreet.Enabled = false;
            this.txtStreet.Location = new System.Drawing.Point(112, 84);
            this.txtStreet.Name = "txtStreet";
            this.txtStreet.ScrollBars = System.Windows.Forms.RichTextBoxScrollBars.ForcedVertical;
            this.txtStreet.Size = new System.Drawing.Size(270, 72);
            this.txtStreet.TabIndex = 17;
            this.txtStreet.Text = "";
            this.txtStreet.TextChanged += new System.EventHandler(this.txtStreet_TextChanged);
            // 
            // MultiItemsAddressEditPage
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.txtZip);
            this.Controls.Add(this.txtState);
            this.Controls.Add(this.txtCity);
            this.Controls.Add(this.txtPOBox);
            this.Controls.Add(this.chkCountry);
            this.Controls.Add(this.chkZip);
            this.Controls.Add(this.chkState);
            this.Controls.Add(this.chkCity);
            this.Controls.Add(this.chkPO);
            this.Controls.Add(this.chkStreet);
            this.Controls.Add(this.lblDisplay);
            this.Name = "MultiItemsAddressEditPage";
            this.Size = new System.Drawing.Size(392, 345);
            this.Controls.SetChildIndex(this.pnlData, 0);
            this.Controls.SetChildIndex(this.lblDisplay, 0);
            this.Controls.SetChildIndex(this.chkStreet, 0);
            this.Controls.SetChildIndex(this.chkPO, 0);
            this.Controls.SetChildIndex(this.chkCity, 0);
            this.Controls.SetChildIndex(this.chkState, 0);
            this.Controls.SetChildIndex(this.chkZip, 0);
            this.Controls.SetChildIndex(this.chkCountry, 0);
            this.Controls.SetChildIndex(this.txtPOBox, 0);
            this.Controls.SetChildIndex(this.txtCity, 0);
            this.Controls.SetChildIndex(this.txtState, 0);
            this.Controls.SetChildIndex(this.txtZip, 0);
            this.pnlData.ResumeLayout(false);
            this.pnlData.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label lblDisplay;
        private System.Windows.Forms.CheckBox chkStreet;
        private System.Windows.Forms.CheckBox chkPO;
        private System.Windows.Forms.CheckBox chkCity;
        private System.Windows.Forms.CheckBox chkState;
        private System.Windows.Forms.CheckBox chkZip;
        private System.Windows.Forms.CheckBox chkCountry;
        private System.Windows.Forms.TextBox txtPOBox;
        private System.Windows.Forms.TextBox txtCity;
        private System.Windows.Forms.TextBox txtState;
        private System.Windows.Forms.TextBox txtZip;
        private System.Windows.Forms.ComboBox cbCountry;
        private System.Windows.Forms.Label lbCity;
        private System.Windows.Forms.Label lbPOBox;
        private System.Windows.Forms.Label lbStreet;
        private System.Windows.Forms.Label lbCountry;
        private System.Windows.Forms.Label lbZip;
        private System.Windows.Forms.Label lbState;
        private System.Windows.Forms.RichTextBox txtStreet;
    }
}
