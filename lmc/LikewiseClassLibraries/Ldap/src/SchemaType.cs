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

namespace Likewise.LMC.LDAP
{
    public class SchemaType
    {
        #region Class data

        private readonly string _cName;

        private readonly ADSType _adsType;

        private bool _isSingleValued = false;

        private readonly string _attributeSyntax;

        private readonly string _attrDisplayName;

        private string _attributeType = "";

        #endregion

        #region Constructor

        public SchemaType(string cName, string attributeSyntax , string attrDisplayName ,string attributeType)
        {
            _cName = cName;
            if (attributeSyntax != null)
            {
                _adsType = ADTypeLookup.GetADSType(attributeSyntax);
            }
            _isSingleValued = false;
            _attributeSyntax = attributeSyntax;
            _attrDisplayName = attrDisplayName;
            _attributeType = attributeType;
        }

        #endregion

        #region accessor methods

        public string CName
        {
            get
            {
                return _cName;
            }
        }

        public ADSType DataType
        {
            get
            {
                return _adsType;
            }   
        }

        public bool IsSingleValued
        {
            set
            {
                _isSingleValued = value;
            }
            get
            {
                return _isSingleValued;
            }
        }

        public string AttributeSyntax
        {
            get
            {
                return _attributeSyntax;
            }
        }

        public string AttributeDisplayName
        {
            get
            {
                return _attrDisplayName;
            }
        }

        public string AttributeType
        {
            get
            {
                return _attributeType;
            }
            set
            {
                _attributeType = value;
            }
        }

        #endregion
    }
}
