using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace Likewise.LMC.ServerControl
{
    /// <summary>
    /// This class derives from LinkLabel in order
    /// to address deficiencies in how it handles
    /// the coloring of disabled links. LinkLabel
    /// doesn't seem to pay any attention to the
    /// DisabledLinkColor property. This class
    /// fixes that problem
    /// </summary>
    public partial class Action : LinkLabel
    {
        #region Constants

        // major and minor OS version for windows 2000
        private const int nMajorWin2k = 5;
        private const int nMinorWin2k = 0;

        #endregion

        #region Constructor
        public Action()
        {
            InitializeComponent();
        }
        #endregion

        public void DoAction(EventArgs e)
        {
            OnLinkClicked(null);
        }

        [DefaultValue(typeof(Color), "DarkGray")]
        public new Color DisabledLinkColor
        {
            get
            {
                return base.DisabledLinkColor;
            }
            set
            {
                base.DisabledLinkColor = value;
            }
        }

        private bool Win2K()
        {
            OperatingSystem os = Environment.OSVersion;
            return os.Version.Major == nMajorWin2k && os.Version.Minor == nMinorWin2k;
        }

        #region Method Overrides

        /// <summary>
        /// Simulate LinkClicked event when shortcut key is pressed.
        /// </summary>
        /// <param name="charCode"></param>
        /// <returns>true if shortcut kew was handled</returns>
        protected override bool ProcessMnemonic(char charCode)
        {
            // Only process the shortcuts if the ALT key is down
            if ((KeyboardInfo.GetKeyState(KeyboardInfo.VirtualKeyStates.VK_MENU) & 0x8000) == 0x8000)
            {
                if (CanSelect && IsMnemonic(charCode, Text) && this.Links.Count > 0 )
                {
                    this.OnLinkClicked(new LinkLabelLinkClickedEventArgs(this.Links[0]));
                    return true;
                }
            }

            return false;
        }

        /// <summary>
        /// Override paint code since LinkLabel doesn't
        /// seem to respect DisabledLinkColor
        /// </summary>
        /// <param name="e"></param>
        protected override void OnPaint(PaintEventArgs e)
        {
            // if we're running on Windows 2000 or the link is enabled, just do the
            // standard thing. Win2k seems to have problems with the TextRenderer.

            if (Win2K() || Enabled)
                base.OnPaint(e);
            else
            {
                // not enabled. Do the regular background
                // painting, then draw our own text using
                // the correct background color
                base.OnPaintBackground(e);

                Brush br = new SolidBrush(this.DisabledLinkColor);
                StringFormat f = new StringFormat();
                if (this.ShowKeyboardCues)
                    f.HotkeyPrefix = System.Drawing.Text.HotkeyPrefix.Show;
                else
                    f.HotkeyPrefix = System.Drawing.Text.HotkeyPrefix.Hide;
                e.Graphics.DrawString(this.Text, this.Font, br, new PointF(Padding.Left,Padding.Top),f);
            }
        }
        #endregion

    }
}
