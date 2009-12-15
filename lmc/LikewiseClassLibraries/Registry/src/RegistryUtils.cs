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

namespace Likewise.LMC.Registry
{
    public class RegistryUtils
    {
        private static char[] cHexa = new char[] { 'A', 'B', 'C', 'D', 'E', 'F' };
        private static int[] iHexaNumeric = new int[] { 10, 11, 12, 13, 14, 15 };
        private static int[] iHexaIndices = new int[] { 0, 1, 2, 3, 4, 5 };
        private const int asciiDiff = 48;
        private const int base10 = 10;

        public static string StringToBase(string sInputStr, int iBase)
        {
            string converted = string.Empty;
            int rem, quoe;
            int num = Convert.ToInt32(sInputStr);

            do
            {
                quoe = num % iBase;
                rem = num / iBase;
                if (rem != 0)
                    num = rem;
                else
                    num = 0;
                if (iHexaNumeric.ToString().Contains(rem.ToString()))
                {
                    int index = iHexaNumeric.ToString().IndexOf(rem.ToString());
                    quoe = Convert.ToInt32(iHexaNumeric[index]);
                }

                converted += quoe.ToString();

            } while (num != 0);

            char[] chars = converted.ToCharArray();

            converted = string.Empty;

            for (int idx = chars.Length - 1; idx >= 0; idx--)
            {
                converted += chars[idx].ToString();
            }

            return converted;
        }

        public static string DecimalToBase(uint iDec, uint numbase)
        {
            string strBin = "";
            uint[] result = new uint[32];
            int MaxBit = 32;
            for (; iDec > 0; iDec /= numbase)
            {
                uint rem = iDec % numbase;
                result[--MaxBit] = rem;
            }
            for (int i = 0; i < result.Length; i++)
            {
                if ((uint)result.GetValue(i) >= base10)
                {
                    strBin += cHexa[(uint)result.GetValue(i) % base10];
                }
                else
                {
                    strBin += result.GetValue(i);
                }
            }
            strBin = strBin.TrimStart(new char[]
                        {
                            '0'
                        }
            );
            return strBin;
        }

        public static string DecimalToBase(int iDec, int numbase)
        {
            string strBin = "";
            int[] result = new int[32];
            int MaxBit = 32;
            for (; iDec > 0; iDec /= numbase)
            {
                int rem = iDec % numbase;
                result[--MaxBit] = rem;
            }
            for (int i = 0; i < result.Length; i++)
            {
                if ((int)result.GetValue(i) >= base10)
                {
                    strBin += cHexa[(int)result.GetValue(i) % base10];
                }
                else
                {
                    strBin += result.GetValue(i);
                }
            }
            strBin = strBin.TrimStart(new char[]
                        {
                            '0'
                        }
            );
            return strBin;
        }

        public static int BaseToDecimal(string sBase, int numbase)
        {
            int dec = 0;
            int b;
            int iProduct = 1;
            string sHexa = "";
            if (numbase > base10)
                for (int i = 0; i < cHexa.Length; i++)
                    sHexa += cHexa.GetValue(i).ToString();
            for (int i = sBase.Length - 1; i >= 0; i--, iProduct *= numbase)
            {
                string sValue = sBase[i].ToString();
                if (sValue.IndexOfAny(cHexa) >= 0)
                    b = iHexaNumeric[sHexa.IndexOf(sBase[i])];
                else
                    b = (int)sBase[i] - asciiDiff;
                dec += (b * iProduct);
            }
            return dec;
        }

        public static string DecimalToBase(ulong lDec, uint numbase)
        {
            string strBin = "";
            ulong[] result = new ulong[64];
            int MaxBit = 64;
            for (; lDec > 0; lDec /= numbase)
            {
                ulong rem = lDec % numbase;
                result[--MaxBit] = rem;
            }
            for (int i = 0; i < result.Length; i++)
            {
                if ((ulong)result.GetValue(i) >= base10)
                {
                    strBin += cHexa[(ulong)result.GetValue(i) % base10];
                }
                else
                {
                    strBin += result.GetValue(i);
                }
            }
            strBin = strBin.TrimStart(new char[]
                        {
                            '0'
                        }
            );
            return strBin;
        }
    }
}
