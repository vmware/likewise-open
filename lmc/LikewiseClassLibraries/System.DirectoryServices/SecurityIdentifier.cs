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

namespace System.DirectoryServices
{
    public class SecurityIdentifier : IdentityReference
    {
        #region Class Data
        private byte[] _sidbytes;
        private int startIndex;
        private int _bytelength;
        private string _sidStr;

        private string _AccountDomainSid;
        #endregion

        #region Methods
        public SecurityIdentifier(byte[] sidbytes, int index)
        {
            _sidbytes = sidbytes;
            startIndex = index;
            _bytelength = sidbytes.Length;
            _sidStr = this.BytesToString();
            _AccountDomainSid = _sidStr;
        }

        public SecurityIdentifier(string sddlForm)
        {
            _sidStr = sddlForm;
            _AccountDomainSid = _sidStr;
            _sidbytes = this.StringToBytes();
            _bytelength = _sidbytes.Length;
        }

        public SecurityIdentifier(WellKnownSidType sidType, SecurityIdentifier domainSid)
        {                 
            switch (sidType)
            {
                case WellKnownSidType.AccountComputersSid: //append 515
                    _sidStr = string.Concat(domainSid.Value, "-515");                  

                    break;

                case WellKnownSidType.AccountDomainUsersSid: //append 513
                    _sidStr = string.Concat(domainSid.Value, "-513");

                    break;

                default:
                    _sidStr = domainSid.Value;
                    // TODO
                    break;
            }
            _AccountDomainSid = domainSid.Value;
            _sidbytes = this.StringToBytes();
            _bytelength = _sidbytes.Length;            
        }

        /// <summary>
        /// Returns the SecurityID for the specified "ObjectSid" attribute value
        /// </summary>
        /// <returns></returns>
        public override string ToString()
        {
            return _sidStr;            
        }
        #endregion


        public override string Value
        {
            get 
            {
                return _sidStr;
            }
        }

        public override bool Equals(object o)
        {
            throw new Exception("The method or operation is not implemented.");
        }

        public override int GetHashCode()
        {
            throw new Exception("The method or operation is not implemented.");
        }

        public override bool IsValidTargetType(Type targetType)
        {
            throw new Exception("The method or operation is not implemented.");
        }

        public override IdentityReference Translate(Type targetType)
        {
            throw new Exception("The method or operation is not implemented.");
        }


        //The following functions needs be implemented
        public SecurityIdentifier AccountDomainSid
        {
            get
            {
                return new SecurityIdentifier(_AccountDomainSid);
            }
        }

        public byte[] BinaryForm
        {
            get
            {
                return _sidbytes;
            }
        }

        //before call this function, memory is allocated for binaryForm 
        public void GetBinaryForm(byte[] binaryForm, int offset)
        {
            if (_sidbytes != null && _sidbytes.Length > 0)
            {                   
                for (int i = offset; i < _sidbytes.Length + offset; i++)
                    binaryForm[i] = _sidbytes[i - offset];
            }
        }

        public int BinaryLength
        {
            get
            {
                return _bytelength;
            }
        }

        private string BytesToString()
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
                    for (int i = 0;i<strAuth.Length;i++)
                    {
                        if (strAuth[i] == '0') continue;
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
            catch (Exception)
            {
                //Console.WriteLine("catch exception: " + ex.Message);
                return "";
            }

            return stringSid.ToString();
        }


        /*So for example, if your SID is S-1-5-21-2127521184-1604012920-1887927527-72713, then your raw hex SID is
                                         010500000000000515000000A065CF7E784B9B5FE77C8770091C0100
          This breaks down as follows:
         * 01	S-1
         * 05	(seven dashes, seven minus two = 5)
         * 000000000005	(5 = 0x000000000005, big-endian) 6 bytes
         * 15000000	(21 = 0x00000015, big-endian)  4 bytes
         * A065CF7E	(2127521184 = 0x7ECF65A0, big-endian)  4 bytes
         * 784B9B5F	(1604012920 = 0x5F9B4B78, big-endian)  4 bytes
         * E77C8770	(1887927527 = 0X70877CE7, big-endian)  4 bytes
         * 091C0100	(72713 = 0x00011c09, big-endian)  ...  */
         private byte[] StringToBytes()
        {
            string[] splits = _sidStr.Split('-');
            int NumberDashes = splits.Length - 1;

            List<byte> sidByte = new List<byte>();
            
            //revision
            byte revision = Convert.ToByte(uint.Parse(splits[1]));
            sidByte.Add(revision);
            
            //number of dashes minus two
            byte dashes = Convert.ToByte(Convert.ToString(NumberDashes-2));
            sidByte.Add(dashes);

            //six bytes (48-bit)  in big-endian format            
            uint big_edian_number = uint.Parse(splits[2]);
            big_edian_number = endian_swap(big_edian_number);

            byte[] big_endianBytes = BitConverter.GetBytes(big_edian_number);
            if (big_endianBytes.Length < 6)
                for (int i = 0; i< 6 - big_endianBytes.Length; i++)
                    sidByte.Add(new byte());

            foreach (byte b in big_endianBytes)
                sidByte.Add(b);

            //four bytes treated as 32-bit number in little-endian format until we finish splits str array
            for (int i = 3; i < splits.Length; i++)
            {   
                //uint big_edians = uint.Parse(splits[i]);
                uint little_edian_number = uint.Parse(splits[i]);                                                              

                byte[] little_endianBytes = BitConverter.GetBytes(little_edian_number);
                if (little_endianBytes.Length < 4)
                    for (int j = 0; j < 4 - little_endianBytes.Length; j++)
                        sidByte.Add(new byte());

                foreach (byte b in little_endianBytes)
                    sidByte.Add(b); 
            }

            byte[] results = new byte[sidByte.Count];
            sidByte.CopyTo(results);

            return results;
        }   


        // C# to convert a string to a byte array.
        public static byte[] DecimalStrToByteArray(string str)
        {
            System.Text.ASCIIEncoding encoding = new System.Text.ASCIIEncoding();
            return encoding.GetBytes(str);
        }

        //swap big endian to little endian rep
        public static uint endian_swap(uint x)
        {
            return 
                (x >> 24) |
                ((x << 8) & 0x00FF0000) |
                ((x >> 8) & 0x0000FF00) |
                (x << 24);
        } 

        //convert a hex string to byte array
        public static byte[] HexStrToByteArray(string HexString)
        {

            int NumberChars = HexString.Length;

            byte[] bytes = new byte[NumberChars / 2];

            for (int i = 0; i < NumberChars; i += 2)           
                bytes[i / 2] = Convert.ToByte(HexString.Substring(i, 2), 16);

            return bytes;

        }
    }

    
}
