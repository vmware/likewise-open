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
using System.Text;
using System.Diagnostics;
using Likewise.LMC.Utilities;

/*****
 * PSID_IDENTIFIER_AUTHORITY    - First 6 bytes
 * SubAuthorityCount            - Next 1 Byte
 * SubAuthority0                - Next 4 bytes
 * SubAuthority1                - Next 4 bytes
 * SubAuthority2                - Next 4 bytes
 * SubAuthority3                - Next 4 bytes
 * SubAuthority4                - Next 4 bytes
 * SubAuthority5                - Next 4 bytes
 * SubAuthority6                - Next 4 bytes
 * SubAuthority7                - Next 4 bytes
 * *****************************************************/

namespace Likewise.LMC.Plugins.ADUCPlugin
{
    public class SecurityID
    {
        #region Class Data
        private byte[] _sidbytes;
        private int startIndex;
        #endregion

        #region Methods
        public SecurityID(byte[] sidbytes, int index)
        {
            this._sidbytes = sidbytes;
            startIndex = index;
        }

        /// <summary>
        /// Returns the SecurityID for the specified "ObjectSid" attribute value
        /// </summary>
        /// <returns></returns>
        /*public override string ToString()
        {
            StringBuilder stringSid = new StringBuilder();
            stringSid.Append("S-");
            try
            {
                // Add SID revision.
                stringSid.Append(_sidbytes[0].ToString());
                // Next six bytes are SID authority value.
                if (_sidbytes[6] != 0 || _sidbytes[5] != 0)
                {
                    string strAuth = String.Format
                        ("0x{0:2x}{1:2x}{2:2x}{3:2x}{4:2x}{5:2x}",
                        (Int16)_sidbytes[1], (Int16)_sidbytes[2], (Int16)_sidbytes[3],
                        (Int16)_sidbytes[4], (Int16)_sidbytes[5], (Int16)_sidbytes[6]);
                    stringSid.Append("-");
                    stringSid.Append(strAuth);
                }
                else
                {
                    Int64 intValue = (Int32)(_sidbytes[1]) + (Int32)(_sidbytes[2] << 8) +
                        (Int32)(_sidbytes[3] << 16) + (Int32)(_sidbytes[4] << 24);
                    stringSid.Append("-");
                    stringSid.Append(intValue.ToString());
                }

                // Get sub authority count
                int iSubCount = Convert.ToInt32(_sidbytes[7]);
                int indexAuth = 0;
                for (int i = 0; i < iSubCount && indexAuth < iSubCount; i++)
                {
                    indexAuth = 8 + i * 4;
                    UInt32 iSubAuth = BitConverter.ToUInt32(_sidbytes, indexAuth);
                    stringSid.Append("-");
                    stringSid.Append(iSubAuth.ToString());
                }
            }
            catch (Exception ex)
            {
                Logger.Log(ex.Message);
                return "";
            }

            return stringSid.ToString();
        }*/

        public override string ToString()
        {
            StringBuilder stringSid = new StringBuilder();
            stringSid.Append("S-");
            try
            {
                // Add SID revision.
                stringSid.Append(_sidbytes[0].ToString());
                //the next byte is how many dashes

                // Next six bytes are SID authority value.
                if (_sidbytes[7] != 0 || _sidbytes[6] != 0)
                {
                    string strAuth = String.Format
                    ("{0:x2}{1:x2}{2:x2}{3:x2}{4:x2}{5:x2}",
                    (Int16)_sidbytes[2], (Int16)_sidbytes[3], (Int16)_sidbytes[4],
                    (Int16)_sidbytes[5], (Int16)_sidbytes[6], (Int16)_sidbytes[7]);

                    int firstIndex = 0;
                    for (int i = 0; i < strAuth.Length; i++)
                    {
                        if (strAuth[i] == '0')
                        {
                            continue;
                        }
                        else
                        {
                            firstIndex = i;
                            break;
                        }
                    }
                    strAuth = strAuth.Substring(firstIndex);

                    stringSid.Append("-");
                    stringSid.Append(strAuth);


                }
                else
                {
                    Int64 intValue = (Int32)(_sidbytes[2]) + (Int32)(_sidbytes[3] << 8) +
                    (Int32)(_sidbytes[4] << 16) + (Int32)(_sidbytes[5] << 24);
                    stringSid.Append("-");
                    stringSid.Append(intValue.ToString());
                }

                // Get sub authority count
                int iSubCount = Convert.ToInt32(_sidbytes[1]);
                int indexAuth = 0;
                for (int i = 0; i < iSubCount; i++)
                {
                    indexAuth = 8 + i * 4;
                    UInt32 iSubAuth = BitConverter.ToUInt32(_sidbytes, indexAuth);
                    stringSid.Append("-");
                    stringSid.Append(iSubAuth.ToString());
                }
            }
            catch (Exception e)
            {
                Logger.LogException("SecurityID.ToString()", e);
                return "";
            }

            return stringSid.ToString();
        }
        #endregion
    }

    public class ObjectGuid
    {
        #region Class Data
        private byte[] _guidbytes;
        private int startIndex;
        #endregion

        #region Methods
        public ObjectGuid(byte[] guidbytes, int index)
        {
            this._guidbytes = guidbytes;
            startIndex = index;
        }

        /// <summary>
        /// Returns the SecurityID for the specified "ObjectSid" attribute value
        /// </summary>
        /// <returns></returns>
        /// 4321-65-87-9 10-11 12 13 14 15 16
        public override string ToString()
        {
            StringBuilder stringGuid = new StringBuilder();

            try
            {
                stringGuid.Append(_guidbytes[3].ToString("X").PadLeft(2, '0'));
                stringGuid.Append(_guidbytes[2].ToString("X").PadLeft(2, '0'));
                stringGuid.Append(_guidbytes[1].ToString("X").PadLeft(2, '0'));
                stringGuid.Append(_guidbytes[0].ToString("X").PadLeft(2, '0'));

                stringGuid.Append("-");

                stringGuid.Append(_guidbytes[5].ToString("X").PadLeft(2, '0'));
                stringGuid.Append(_guidbytes[4].ToString("X").PadLeft(2, '0'));

                stringGuid.Append("-");

                stringGuid.Append(_guidbytes[7].ToString("X").PadLeft(2, '0'));
                stringGuid.Append(_guidbytes[6].ToString("X").PadLeft(2, '0'));

                stringGuid.Append("-");

                stringGuid.Append(_guidbytes[8].ToString("X").PadLeft(2, '0'));
                stringGuid.Append(_guidbytes[9].ToString("X").PadLeft(2, '0'));

                stringGuid.Append("-");

                stringGuid.Append(_guidbytes[10].ToString("X").PadLeft(2, '0'));
                stringGuid.Append(_guidbytes[11].ToString("X").PadLeft(2, '0'));
                stringGuid.Append(_guidbytes[12].ToString("X").PadLeft(2, '0'));
                stringGuid.Append(_guidbytes[13].ToString("X").PadLeft(2, '0'));
                stringGuid.Append(_guidbytes[14].ToString("X").PadLeft(2, '0'));
                stringGuid.Append(_guidbytes[15].ToString("X").PadLeft(2, '0'));
            }
            catch (Exception ex)
            {
                Logger.Log(ex.Message);
                return "";
            }

            return stringGuid.ToString();
        }

        #endregion

    }
}


