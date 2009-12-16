using System;
using System.Collections.Generic;
using System.Text;

namespace Likewise.LMC.SecurityDesriptor
{
    public class LwAccessControlEntry
    {
        #region Class Data

        private string sUsername = string.Empty;
        private string sSid = string.Empty;
        private int iAceType = -1;
        private string sAccessMask = string.Empty;
        private int sAceFlags = -1;
        private int sAceSize = -1;

        #endregion

        #region Access Specifiers

        public string Username
        {
            get
            {
                return sUsername;
            }
            set
            {
                sUsername = value;
            }
        }

        public string SID
        {
            get
            {
                return sSid;
            }
            set
            {
                sSid = value;
            }
        }

        public int AceType
        {
            get
            {
                return iAceType;
            }
            set
            {
                iAceType = value;
            }
        }

        public string AccessMask
        {
            get
            {
                return sAccessMask;
            }
            set
            {
                sAccessMask = value;
            }
        }

        public int AceFlags
        {
            get
            {
                return sAceFlags;
            }
            set
            {
                sAceFlags = value;
            }
        }

        public int AceSize
        {
            get
            {
                return sAceSize;
            }
            set
            {
                sAceSize = value;
            }
        }

        #endregion
    }
}
