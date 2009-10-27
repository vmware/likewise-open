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
using System.Windows.Forms;
using System.Net;
using Likewise.LMC.LMConsoleUtils;

namespace Likewise.LMC.ServerControl
{
    public partial class StringRequestDialog : Form
    {
        #region class data

        const int MAX_DESCRIPTION_LENGTH = 45;
        const int MAX_CAPTION_LENGTH = 48;
        const int MAX_GROUPBOX_CAPTION_LENGTH = 128;
        const int MAX_HINT_LENGTH = 32;

        public delegate bool stringArrContextDelegate(string[] s, Object context);

        private stringArrContextDelegate resultReportArrContextDelegate = null;

        private string caption;
        private string groupBoxCaption;
        private string[] descriptions;
        private string[] hints;
        private string[] oldFieldValues;

        private int numTextBoxes = 0;

        public bool allowOKWithoutModify = false;
        public bool bDialogResult = true;

        //this is used to keep track of whatever data the calling method needs to be passed along to a delegate.
        private Object context = null;

        #endregion

        #region constructors

        public StringRequestDialog(
            stringArrContextDelegate result, 
            string caption, 
            string groupBoxCaption,
            string[] descriptions, 
            string[] hints,  
            string[] fieldContents,
            Object context)
        {
            this.resultReportArrContextDelegate = result;
            this.caption = caption;
            this.descriptions = descriptions;
            this.hints = hints;
            constructorFinish();
            this.FieldContents = fieldContents;
            this.context = context;
        }


        private void SetGroupBoxCaption(string groupBoxCaption)
        {
            this.groupBoxCaption = groupBoxCaption;
            this.groupBox1.Text = groupBoxCaption;
            if (groupBoxCaption != null && groupBoxCaption.Length > MAX_GROUPBOX_CAPTION_LENGTH)
            {
                throw new ArgumentException(
                    String.Format("StringRequestDialog() called with groupBoxCaption.Length={0}, max={1}",
                    caption.Length, MAX_GROUPBOX_CAPTION_LENGTH));

            }
        }

        private void constructorFinish()
        {

            numTextBoxes = descriptions.Length;

            oldFieldValues = new string[numTextBoxes];

            if (descriptions == null)
            {
                throw new ArgumentException("StringRequestDialog() called with descriptions == null");
            }

            if (hints == null)
            {
                throw new ArgumentException("StringRequestDialog() called with hints == null");
            }

            if (descriptions.Length != hints.Length)
            {
                throw new ArgumentException("StringRequestDialog() called with descriptions.Length != hints.Length");
            }
            if (descriptions.Length < 1)
            {
                throw new ArgumentException("StringRequestDialog() called with descriptions.Length < 1");
            }

            for (int i = 0; i < numTextBoxes; i++)
            {
                if (descriptions[i] != null && descriptions[i].Length > MAX_DESCRIPTION_LENGTH)
                {
                    throw new ArgumentException(
                        String.Format("StringRequestDialog() called with descriptions[{0}].Length={1}, max={2}",
                        i, descriptions[i].Length, MAX_DESCRIPTION_LENGTH));
                }
                if (hints[i] != null && hints[i].Length > MAX_HINT_LENGTH)
                {
                    throw new ArgumentException(
                        String.Format("StringRequestDialog() called with hints[{0}].Length={1}, max={2}",
                        i, hints[i].Length, MAX_HINT_LENGTH));
                }
                oldFieldValues[i] = "";
            }

            if (caption != null && caption.Length > MAX_CAPTION_LENGTH)
            {
                throw new ArgumentException(
                    String.Format("StringRequestDialog() called with caption.Length={0}, max={1}",
                    caption.Length, MAX_CAPTION_LENGTH));

            }

            InitializeComponent();

            InitializeCustomComponents();

            this.Text = caption;


            if (textBoxes != null && textBoxes.Length >= 1)
            {
                textBoxes[0].Select();
            }
            
        }

        private void InitializeCustomComponents()
        {

            int hintsBaseY = 54;
            int textBoxBaseY = 30;
            int descriptionBaseY = 33;

            int offsetY = 58;

            int totalOffset = offsetY * (numTextBoxes-1);

            int windowSizeY = 140 + totalOffset;

            int buttonY = windowSizeY - 35;
            int groupBoxY = windowSizeY - 50;

            int sizeX = 455; //430
            int hintLocationX = 165; //145
            int textBoxLocationX = 180; //160

            this.lblHints = new Label[numTextBoxes];
            this.textBoxes = new TextBox[numTextBoxes];
            this.lblDescriptions = new Label[numTextBoxes];

            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(StringRequestDialog));

            this.SuspendLayout();

            this.ClientSize = new System.Drawing.Size(sizeX, windowSizeY);
            this.MinimumSize = new System.Drawing.Size(sizeX, windowSizeY);

            // 
            // groupBox1
            // 
            this.groupBox1.Location = new System.Drawing.Point(9, 8);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(sizeX - 22, groupBoxY);
            this.groupBox1.TabIndex = ((numTextBoxes) * 3) + 3;
            this.groupBox1.TabStop = false;


            for (int i = 0; i < numTextBoxes; i++)
            {
                this.lblHints[i] = new Label();
                this.textBoxes[i] = new TextBox();
                this.lblDescriptions[i] = new Label();

                // 
                // lblHints
                // 
                this.lblHints[i].AutoSize = true;
                this.lblHints[i].BackColor = System.Drawing.SystemColors.MenuBar;
                this.lblHints[i].Font = new System.Drawing.Font("Arial", 8.25F, System.Drawing.FontStyle.Italic, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
                this.lblHints[i].ForeColor = System.Drawing.Color.Black;
                this.lblHints[i].Location = new System.Drawing.Point(hintLocationX, hintsBaseY + (i*offsetY));
                this.lblHints[i].Name = String.Format("lblHint{0}", i);
                this.lblHints[i].Size = new System.Drawing.Size(25, 14);
                this.lblHints[i].TabIndex = (3*i)+2;
                this.lblHints[i].Text = hints[i];

                // 
                // textBoxes
                // 
                this.textBoxes[i].Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                            | System.Windows.Forms.AnchorStyles.Right)));
                this.textBoxes[i].Location = new System.Drawing.Point(textBoxLocationX, textBoxBaseY + (i*offsetY));
                this.textBoxes[i].Name = String.Format("textBox{0}", i);
                this.textBoxes[i].Size = new System.Drawing.Size(240, 25);
                this.textBoxes[i].TabIndex = (3*i)+1;
                this.textBoxes[i].KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.textBox_KeyPress);

                // 
                // lblDescriptions
                // 
                this.lblDescriptions[i].AutoSize = true;
                this.lblDescriptions[i].Location = new System.Drawing.Point(1, descriptionBaseY + (i * offsetY));
                this.lblDescriptions[i].Name = String.Format("lblDescription{0}", i);
                this.lblDescriptions[i].Size = new System.Drawing.Size(65, 23);
                this.lblDescriptions[i].TabIndex = (3*i);
                this.lblDescriptions[i].Text = descriptions[i];

                this.groupBox1.Controls.Add(this.lblDescriptions[i]);
                this.groupBox1.Controls.Add(this.lblHints[i]);
                this.groupBox1.Controls.Add(this.textBoxes[i]);
                
            }

            // 
            // okButton
            // 
            this.okButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.okButton.Location = new System.Drawing.Point(sizeX - 250, buttonY);
            this.okButton.Name = "okButton";
            this.okButton.Size = new System.Drawing.Size(92, 26);
            this.okButton.TabIndex = ((numTextBoxes) * 3) + 1;
            this.okButton.Text = "OK";
            this.okButton.UseVisualStyleBackColor = true;
            this.okButton.Click += new System.EventHandler(this.connectButton_Click);
            

            // 
            // cancelButton
            // 
            this.cancelButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.cancelButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.cancelButton.Location = new System.Drawing.Point(sizeX - 150, buttonY);
            this.cancelButton.Name = "cancelButton";
            this.cancelButton.Size = new System.Drawing.Size(92, 26);
            this.cancelButton.TabIndex = ((numTextBoxes) * 3) + 2;
            this.cancelButton.Text = "Cancel";
            this.cancelButton.UseVisualStyleBackColor = true;
            this.cancelButton.Click += new System.EventHandler(this.cancelButton_Click);            

            this.Controls.Add(this.groupBox1);
            this.Controls.Add(this.cancelButton);
            this.Controls.Add(this.okButton);          

            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion constructors

        #region member functions

        /// <summary>
        /// Toggles whether a given field should be shown to the user as ****, e.g. for password entry
        /// </summary>
        /// <param name="fieldIndex">Field index</param>
        /// <param name="secure">TRUE if the field should be displayed as asteriks</param>
        public void SetSecurityStatus(int fieldIndex, bool secure)
        {
            if (fieldIndex < 0 || fieldIndex > numTextBoxes-1)
            {
                Logger.LogMsgBox(String.Format("StringRequestDialog.SetSecurityStatus: invalid fieldIndex ({0})", fieldIndex));
                return;
            }

            if (secure)
            {
                this.textBoxes[fieldIndex].PasswordChar = '*';
            }
            else
            {
                this.textBoxes[fieldIndex].PasswordChar = (char) 0;
            }
            

        }

        #endregion

        #region accessors

        public string[] FieldContents
        {
            set
            {
                string[] input = (string[])value;
                if (input != null && input.Length == numTextBoxes)
                {
                    for (int i = 0; i < numTextBoxes; i++)
                    {
                        textBoxes[i].Text = input[i];
                        oldFieldValues[i] = input[i];
                    }
                }
                else
                {
                    throw new ArgumentException("StringRequestDialog.text: invalid argument for use of 'set' accessor");
                }
            }
            get
            {
                string[] result = new string[numTextBoxes];
                for (int i = 0; i < numTextBoxes; i++)
                {
                    result[i] = textBoxes[i].Text;
                }
                return result;
            }
        }

        #endregion

        #region event handlers

        private void connectButton_Click(object sender, EventArgs e)
        {
            bDialogResult = true;
            finish();
        }

        private void cancelButton_Click(object sender, EventArgs e)
        {
            bDialogResult = false;
            Close();
        }

        private void textBox_KeyPress(object sender, KeyPressEventArgs e)
        {
            if (e.KeyChar == (char)Keys.Escape)
                Close();
            else if (e.KeyChar == (char)Keys.Enter)
            {
                finish();
            }
        }

        #endregion

        #region helper functions

        private void finish()
        {
            if (!okButton.Enabled)
            {
                return;
            }
            else
            {
                okButton.Enabled = false;
            }

            string[] result = new string[numTextBoxes];
            bool validData = false;
            int i = 0;

            foreach (TextBox textBox in textBoxes)
            {
                if (i == 3)
                    result[i] = textBox.Text;
                else
                result[i] = textBox.Text.Trim();
                if (result[i] != oldFieldValues[i])
                {
                    validData = true;
                }
                i++;
            }

            if (allowOKWithoutModify)
            {
                validData = true;
            }

            if (!validData)
            {
                MessageBox.Show(
                    "Please modify one of the fields before clicking OK.", "Likewise Management Console", 
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Exclamation);
                okButton.Enabled = true;
                return;
            }

            i = 0;
            foreach (TextBox textBox in textBoxes)
            {
                if (String.IsNullOrEmpty(result[i]))
                {
                    validData = false;
                    textBox.Clear();
                }
                i++;
            }

            if (!validData)
            {
                MessageBox.Show(
                    "Please enter data in each of the fields before clicking OK.", 
                    "Likewise Management Console", 
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Exclamation);
                okButton.Enabled = true;
                return;
            }

            //for ADUC only need provide hostname and domain name, using GSS binding
            if (result.Length == 2) 
            {
                string[] tempRes = new string[result.Length + 2];
                tempRes[0] = result[0];
                tempRes[1] = result[1];
                tempRes[2] = "username";
                tempRes[3] = "password";

                result = tempRes;
            }

            if (validData && reportResult(result))
            {
                Close();           
            }
            else
            {
                okButton.Enabled = true;
            }
        }

        private bool reportResult(string[] result)
        {
            return resultReportArrContextDelegate(result, context); 
        }

        #endregion

    }
}