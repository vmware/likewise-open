using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Text;
using System.Windows.Forms;

namespace Likewise.LMC.ServerControl
{
    /// <summary>
    /// A convenience class that simplifies the creation
    /// of "Task" infopanels used on the left half of your
    /// typical Likewise page. There's little processing
    /// that goes on here; mostly, the class is about
    /// compositing a label along with the InfoPanel and
    /// setting some default visual properties.
    /// </summary>
    public partial class ActionBox : Panel, IComparer
    {
        #region Visual constants

        private const int nPad = 6;
        private int nCurveSize = 9;

        #endregion

        #region Object data

        private Color colTop = Color.FromArgb(132, 162, 195);
        private Color colBottom = Color.FromArgb(10, 42, 67);
        private string sCaption=Properties.Resources.Caption_Tasks;
        private Color colorCaptionForeColor=SystemColors.Window;
        private Font fontCaption = new Font("Verdana", 8, FontStyle.Bold);

        #endregion

        #region Constructor
        public ActionBox()
        {
            InitializeComponent();
        }
        #endregion

        #region Public methods

        public ContextMenu GetContextMenu()
        {
            ContextMenu cm = new ContextMenu();

            // add items in the same order that we encounter them, visually

            // iterate through children, looking for Action objects
            ArrayList al = new ArrayList();

            foreach (Control c in Controls)
                if ((c is Action) || (c is GroupBox))
                    al.Add(c);

            // now, sort these by y position
            al.Sort(this);

            // now, iterate through them, creating menu items for them
            foreach (Control c in al)
            {
                if (c is Action)
                {
                    Action ac = (Action)c;
                    MenuItem mi = new MenuItem(ac.Text, new EventHandler(cm_OnMenuClick));
                    mi.Enabled = ac.Enabled;
                    mi.Tag = ac;
                    cm.MenuItems.Add(mi);
                }
                else if (c is GroupBox)
                    cm.MenuItems.Add(new MenuItem("-"));
            }

            return cm;
        }

        /// <summary>
        /// Event handler for context menu. Passes action on to the Action
        /// object referenced in the MenuItem tag.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cm_OnMenuClick(object sender, EventArgs e)
        {
            // assure that the sender is a MenuItem
            MenuItem mi = sender as MenuItem;
            if (mi != null)
            {
                // see if we've got a tag
                Action ac = mi.Tag as Action;
                if (ac != null)
                {
                    ac.DoAction(e);
                }
            }
        }


        #endregion

        #region Properties

        public string Caption
        {
            get
            {
                return sCaption;
            }
            set
            {
                sCaption = value;
            }
        }

        public Color CaptionForeColor
        {
            get
            {
                return colorCaptionForeColor;
            }
            set
            {
                colorCaptionForeColor = value;
            }
        }

        public Color BottomColor
        {
            get
            {
                return colBottom;
            }
            set
            {
                colBottom = value;
            }
        }

        public Color TopColor
        {
            get
            {
                return colTop;
            }
            set
            {
                colTop = value;
            }
        }

        public int CaptionHeight
        {
            get
            {
                Graphics g = Graphics.FromHwnd(this.Handle);
                SizeF sf;
                if (Caption.Length!=0)
                    sf = g.MeasureString(Caption, this.Font);
                else
                    sf = g.MeasureString("M", this.Font);
                return (int) (sf.Height * 1.5);
            }
        }

        public Font CaptionFont
        {
            get
            {
                return fontCaption;
            }
            set
            {
                fontCaption = value;
            }
        }

        #endregion

        #region Helpers

        #region Paint-related code

        /// <summary>
        /// Creates the rounded outer border path.
        /// </summary>
        /// <param name="rc">The outer rectangle dimensions</param>
        /// <param name="bWholeWindow">TRUE if a path for the whole
        ///                            region is desired. FALSE if it
        ///                            should only include the "body"
        ///                            portion under the caption area.</param>
        /// <param name="bSettingUpClipRegion">TRUE if function is being called
        ///                            to set up the clip region for the window.
        ///                            In this case, we tweak the parameters to
        ///                            avoid interfering with actual drawing</param>
        /// <returns>The created GraphicsPath object.</returns>
        private GraphicsPath CreateBorderPath(Rectangle rc, bool bWholeWindow, bool bSettingUpClipRegion)
        {
            // start out with an empty path
            GraphicsPath gp = new GraphicsPath();

            // calculate a bunch of useful points
            int nWidth = rc.Width;
            int nHeight = rc.Height;
            int xleft = 0;
            int ytop = 0;
            int xright = xleft + nWidth;
            int ybottom = ytop + nHeight;
            if (!bSettingUpClipRegion)
            {
                xright--;
                ybottom--;
            }

            int radius = nCurveSize;
            int diameter = radius + radius;

            Rectangle rs;

            // start the region
            gp.StartFigure();

            // if we're doing the whole window, have to start out differently
            if (bWholeWindow)
            {
                // add the ulhc arc
                rs = new Rectangle(xleft, ytop, diameter, diameter);
                gp.AddArc(rs, 180.0F, 90.0F);

                // add the top line
                gp.AddLine(diameter, 0, xright - diameter, 0);

                // add the urhc arc
                rs = new Rectangle(xright - diameter, 0, diameter, diameter);
                gp.AddArc(rs, 270.0F, 90.0F);

                // add the right line
                gp.AddLine(xright, diameter, xright, ybottom - diameter);
            }
            else
            {
                // add the top line
                gp.AddLine(xleft, CaptionHeight+1, xright, CaptionHeight+1);

                // add the right line
                gp.AddLine(xright, CaptionHeight+1, xright, ybottom - diameter);
            }

            // add the lrhc arc
            rs = new Rectangle(xright - diameter, ybottom - diameter, diameter, diameter);
            gp.AddArc(rs, 0.0F, 90.0F);

            // add the bottom line
            gp.AddLine(xright - diameter, ybottom, xleft + diameter, ybottom);

            // add the llhc arc
            if (bWholeWindow)
                // HACK: when setting up the clip region, if we use "regular math" the
                // clip region ends up interfering with the arc drawing later on. We
                // "inflate" the size of the arc in order to avoid this.
                rs = new Rectangle(xleft, ybottom - diameter, diameter + 2, diameter + 2);
            else
                rs = new Rectangle(xleft, ybottom - diameter, diameter, diameter);
            gp.AddArc(rs, 90.0F, 90.0F);

            gp.CloseFigure();

            return gp;

        }

        /// <summary>
        /// Paints the caption area
        /// </summary>
        /// <param name="g">The Graphics object to be used</param>
        /// <param name="rc">The rectangle enclosing the entire window.</param>
        private void PaintCaption(Graphics g, Rectangle rc)
        {
            // calculate the fill rectangle
            Rectangle rcFill = new Rectangle(0, 0, rc.Width, CaptionHeight);

            // do the gradient fill
            LinearGradientBrush br = new LinearGradientBrush(   rcFill,
                                                                colTop,
                                                                colBottom,
                                                                90.0F);

            g.FillRectangle(br, rcFill);

            // now draw the text

            // calculate the xposition based on the curvature of the window region
            float x = (float) nCurveSize;

            // calculate the y position by splitting the 50% overhead top and bottom
            float y = CaptionHeight * .25F;

            g.DrawString(Caption, this.CaptionFont, new SolidBrush(CaptionForeColor), new PointF(x, y));

        }

        /// <summary>
        /// Handles control painting.
        /// </summary>
        /// <param name="e"></param>
        protected override void OnPaint(PaintEventArgs e)
        {
            // if the rounded region doesn't exist yet, create it now
            if (this.Region == null)
            {
                // create a region
                Rectangle rcc = this.ClientRectangle;
                this.Region = new Region(CreateBorderPath(rcc, true, true));
            }

            base.OnPaint(e);

            // peg the graphics object for later use
            Graphics g = e.Graphics;

            // get the client rectangle
            Rectangle rc = this.ClientRectangle;

            // if either dimension is 0, do nothing
            if (rc.Height == 0 || rc.Width == 0)
                return;

            // draw the background
            Brush br = new SolidBrush(this.BackColor);
            g.FillRectangle(br, rc);

            // now, paint the caption area
            PaintCaption(g, rc);

            // now, outline the rest of the window
            GraphicsPath gp = CreateBorderPath(rc, false, false);
            Pen p = new Pen(this.ForeColor);
            e.Graphics.DrawPath(p, gp);

        }
        #endregion

        /// <summary>
        /// Handles the layout event by positioning its children in a regular pattern
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void ActionBox_Layout(object sender, LayoutEventArgs e)
        {
            // iterate through children, looking for Action objects and snapping them to grid points

            // first, collect all the Action objects
            ArrayList al = new ArrayList();

            foreach (Control c in Controls)
                if ((c is Action) || (c is GroupBox))
                    al.Add(c);

            // now, sort these by y position
            al.Sort(this);

            // Calculate the initial positions
            int x = nCurveSize;
            int y = (int) (CaptionHeight + CaptionHeight * .25);

            // iterate, snapping things into position
            foreach (Control c in al)
            {
                c.Location = new Point(x, y);

                int cy = c.Height;
                if (cy < 3)
                    cy = 3;

                // advance
                y += cy;
            }
        }

        #endregion

        #region IComparer Members

        public int Compare(object a, object b)
        {
            if ((a is Control) && (b is Control))
            {
                int aY = ((Control)a).Location.Y;
                int bY = ((Control)b).Location.Y;

                if (aY < bY)
                    return -1;
                else if (aY == bY)
                    return 1;
                else
                    return 0;
            }
            else
                throw new Exception("Comparing these types of objects is not supported");
        }

        #endregion
    }


}

