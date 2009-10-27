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
    partial class UserMultiselectGeneralEditPage
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(UserMultiselectGeneralEditPage));
            this.pictureBox1 = new System.Windows.Forms.PictureBox();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.lbldisplayprop = new System.Windows.Forms.Label();
            this.lbldisplay = new System.Windows.Forms.Label();
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.chkDescription = new System.Windows.Forms.CheckBox();
            this.chkOffice = new System.Windows.Forms.CheckBox();
            this.txtDescription = new System.Windows.Forms.TextBox();
            this.txtOffice = new System.Windows.Forms.TextBox();
            this.chkTelephone = new System.Windows.Forms.CheckBox();
            this.chkWebpage = new System.Windows.Forms.CheckBox();
            this.chkEmail = new System.Windows.Forms.CheckBox();
            this.txtTelephone = new System.Windows.Forms.TextBox();
            this.txtWebpage = new System.Windows.Forms.TextBox();
            this.txtEmail = new System.Windows.Forms.TextBox();
            this.labelText = new System.Windows.Forms.Label();
            this.txtFax = new System.Windows.Forms.TextBox();
            this.chkFax = new System.Windows.Forms.CheckBox();
            this.pnlData.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).BeginInit();
            this.SuspendLayout();
            // 
            // pnlData
            // 
            this.pnlData.Controls.Add(this.labelText);
            this.pnlData.Controls.Add(this.txtTelephone);
            this.pnlData.Controls.Add(this.txtEmail);
            this.pnlData.Controls.Add(this.groupBox2);
            this.pnlData.Controls.Add(this.txtWebpage);
            this.pnlData.Size = new System.Drawing.Size(385, 371);
            // 
            // pictureBox1
            // 
            this.pictureBox1.Image = ((System.Drawing.Image)(resources.GetObject("pictureBox1.Image")));
            this.pictureBox1.Location = new System.Drawing.Point(15, 16);
            this.pictureBox1.Name = "pictureBox1";
            this.pictureBox1.Size = new System.Drawing.Size(24, 24);
            this.pictureBox1.TabIndex = 22;
            this.pictureBox1.TabStop = false;
            // 
            // groupBox1
            // 
            this.groupBox1.Location = new System.Drawing.Point(15, 55);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(361, 2);
            this.groupBox1.TabIndex = 23;
            this.groupBox1.TabStop = false; 
            // 
            // lbldisplayprop
            // 
            this.lbldisplayprop.Location = new System.Drawing.Point(18, 62);
            this.lbldisplayprop.Name = "lbldisplayprop";
            this.lbldisplayprop.Size = new System.Drawing.Size(333, 39);
            this.lbldisplayprop.TabIndex = 25;
            this.lbldisplayprop.Text = "To change a property for multiple objects, first select the checkbox to enable th" +
                "e change, and then type the change.";
            // 
            // lbldisplay
            // 
            this.lbldisplay.Location = new System.Drawing.Point(18, 107);
            this.lbldisplay.Name = "lbldisplay";
            this.lbldisplay.Size = new System.Drawing.Size(333, 36);
            this.lbldisplay.TabIndex = 26;
            this.lbldisplay.Text = "Depending on the number of objects selected, you might have to wait while changes" +
                " are applied.";
            // 
            // groupBox2
            // 
            this.groupBox2.Location = new System.Drawing.Point(15, 224);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new System.Drawing.Size(361, 2);
            this.groupBox2.TabIndex = 27;
            this.groupBox2.TabStop = false;
            // 
            // chkDescription
            // 
            this.chkDescription.Location = new System.Drawing.Point(15, 148);
            this.chkDescription.Name = "chkDescription";
            this.chkDescription.Size = new System.Drawing.Size(84, 29);
            this.chkDescription.TabIndex = 28;
            this.chkDescription.Text = "&Description:";
            this.chkDescription.UseVisualStyleBackColor = true;
            this.chkDescription.CheckedChanged += new System.EventHandler(this.chkDescription_CheckedChanged);
            // 
            // chkOffice
            // 
            this.chkOffice.Location = new System.Drawing.Point(15, 183);
            this.chkOffice.Name = "chkOffice";
            this.chkOffice.Size = new System.Drawing.Size(84, 20);
            this.chkOffice.TabIndex = 29;
            this.chkOffice.Text = "Offi&ce:";
            this.chkOffice.UseVisualStyleBackColor = true;
            this.chkOffice.CheckedChanged += new System.EventHandler(this.chkOffice_CheckedChanged);
            // 
            // txtDescription
            // 
            this.txtDescription.Enabled = false;
            this.txtDescription.Location = new System.Drawing.Point(105, 155);
            this.txtDescription.Name = "txtDescription";
            this.txtDescription.Size = new System.Drawing.Size(271, 20);
            this.txtDescription.TabIndex = 30;
            // 
            // txtOffice
            // 
            this.txtOffice.Enabled = false;
            this.txtOffice.Location = new System.Drawing.Point(105, 183);
            this.txtOffice.MaxLength = 128;
            this.txtOffice.Name = "txtOffice";
            this.txtOffice.Size = new System.Drawing.Size(271, 20);
            this.txtOffice.TabIndex = 31;
            // 
            // chkTelephone
            // 
            this.chkTelephone.Location = new System.Drawing.Point(15, 241);
            this.chkTelephone.Name = "chkTelephone";
            this.chkTelephone.Size = new System.Drawing.Size(136, 17);
            this.chkTelephone.TabIndex = 32;
            this.chkTelephone.Text = "&Telephone number:";
            this.chkTelephone.UseVisualStyleBackColor = true;
            this.chkTelephone.CheckedChanged += new System.EventHandler(this.chkTelephone_CheckedChanged);
            // 
            // chkWebpage
            // 
            this.chkWebpage.Location = new System.Drawing.Point(15, 304);
            this.chkWebpage.Name = "chkWebpage";
            this.chkWebpage.Size = new System.Drawing.Size(118, 18);
            this.chkWebpage.TabIndex = 33;
            this.chkWebpage.Text = "&Web page:";
            this.chkWebpage.UseVisualStyleBackColor = true;
            this.chkWebpage.CheckedChanged += new System.EventHandler(this.chkWebpage_CheckedChanged);
            // 
            // chkEmail
            // 
            this.chkEmail.Location = new System.Drawing.Point(15, 336);
            this.chkEmail.Name = "chkEmail";
            this.chkEmail.Size = new System.Drawing.Size(118, 17);
            this.chkEmail.TabIndex = 34;
            this.chkEmail.Text = "E-&mail:";
            this.chkEmail.UseVisualStyleBackColor = true;
            this.chkEmail.CheckedChanged += new System.EventHandler(this.chkEmail_CheckedChanged);
            // 
            // txtTelephone
            // 
            this.txtTelephone.Enabled = false;
            this.txtTelephone.Location = new System.Drawing.Point(157, 241);
            this.txtTelephone.MaxLength = 64;
            this.txtTelephone.Name = "txtTelephone";
            this.txtTelephone.Size = new System.Drawing.Size(219, 20);
            this.txtTelephone.TabIndex = 35;
            // 
            // txtWebpage
            // 
            this.txtWebpage.Enabled = false;
            this.txtWebpage.Location = new System.Drawing.Point(139, 303);
            this.txtWebpage.Name = "txtWebpage";
            this.txtWebpage.Size = new System.Drawing.Size(237, 20);
            this.txtWebpage.TabIndex = 36;
            // 
            // txtEmail
            // 
            this.txtEmail.Enabled = false;
            this.txtEmail.Location = new System.Drawing.Point(139, 333);
            this.txtEmail.MaxLength = 258;
            this.txtEmail.Name = "txtEmail";
            this.txtEmail.Size = new System.Drawing.Size(237, 20);
            this.txtEmail.TabIndex = 37;
            // 
            // labelText
            // 
            this.labelText.AutoSize = true;
            this.labelText.Location = new System.Drawing.Point(48, 24);
            this.labelText.Name = "labelText";
            this.labelText.Size = new System.Drawing.Size(114, 13);
            this.labelText.TabIndex = 38;
            this.labelText.Text = "Multiple users selected";
            // 
            // txtFax
            // 
            this.txtFax.Enabled = false;
            this.txtFax.Location = new System.Drawing.Point(139, 271);
            this.txtFax.MaxLength = 64;
            this.txtFax.Name = "txtFax";
            this.txtFax.Size = new System.Drawing.Size(237, 20);
            this.txtFax.TabIndex = 39;
            // 
            // chkFax
            // 
            this.chkFax.Location = new System.Drawing.Point(14, 274);
            this.chkFax.Name = "chkFax";
            this.chkFax.Size = new System.Drawing.Size(119, 17);
            this.chkFax.TabIndex = 38;
            this.chkFax.Text = "&Fax:";
            this.chkFax.UseVisualStyleBackColor = true;
            this.chkFax.CheckedChanged += new System.EventHandler(this.chkFax_CheckedChanged);
            // 
            // UserMultiselectGeneralEditPage
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.Transparent;
            this.Controls.Add(this.txtFax);
            this.Controls.Add(this.chkFax);
            this.Controls.Add(this.chkEmail);
            this.Controls.Add(this.chkWebpage);
            this.Controls.Add(this.chkTelephone);
            this.Controls.Add(this.txtOffice);
            this.Controls.Add(this.txtDescription);
            this.Controls.Add(this.chkOffice);
            this.Controls.Add(this.chkDescription);
            this.Controls.Add(this.lbldisplay);
            this.Controls.Add(this.lbldisplayprop);            
            this.Controls.Add(this.groupBox1);
            this.Controls.Add(this.pictureBox1);
            this.Name = "UserMultiselectGeneralEditPage";
            this.Size = new System.Drawing.Size(385, 371);
            this.Controls.SetChildIndex(this.pnlData, 0);
            this.Controls.SetChildIndex(this.pictureBox1, 0);
            this.Controls.SetChildIndex(this.groupBox1, 0);            
            this.Controls.SetChildIndex(this.lbldisplayprop, 0);
            this.Controls.SetChildIndex(this.lbldisplay, 0);
            this.Controls.SetChildIndex(this.chkDescription, 0);
            this.Controls.SetChildIndex(this.chkOffice, 0);
            this.Controls.SetChildIndex(this.txtDescription, 0);
            this.Controls.SetChildIndex(this.txtOffice, 0);
            this.Controls.SetChildIndex(this.chkTelephone, 0);
            this.Controls.SetChildIndex(this.chkWebpage, 0);
            this.Controls.SetChildIndex(this.chkEmail, 0);
            this.Controls.SetChildIndex(this.chkFax, 0);
            this.Controls.SetChildIndex(this.txtFax, 0);
            this.pnlData.ResumeLayout(false);
            this.pnlData.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.PictureBox pictureBox1;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.Label lbldisplayprop;
        private System.Windows.Forms.Label lbldisplay;
        private System.Windows.Forms.GroupBox groupBox2;
        private System.Windows.Forms.CheckBox chkDescription;
        private System.Windows.Forms.CheckBox chkOffice;
        private System.Windows.Forms.TextBox txtDescription;
        private System.Windows.Forms.TextBox txtOffice;
        private System.Windows.Forms.CheckBox chkTelephone;
        private System.Windows.Forms.CheckBox chkWebpage;
        private System.Windows.Forms.CheckBox chkEmail;
        private System.Windows.Forms.TextBox txtTelephone;
        private System.Windows.Forms.TextBox txtWebpage;
        private System.Windows.Forms.TextBox txtEmail;
        private System.Windows.Forms.Label labelText;
        private System.Windows.Forms.TextBox txtFax;
        private System.Windows.Forms.CheckBox chkFax;
    }
}
