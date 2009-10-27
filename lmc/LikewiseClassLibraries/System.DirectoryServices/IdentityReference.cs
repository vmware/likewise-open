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
    public abstract class IdentityReference
    {
        // Summary:
        //     Compares two System.Security.Principal.IdentityReference objects to determine
        //     whether they are not equal. They are considered not equal if they have different
        //     canonical name representations than the one returned by the System.Security.Principal.IdentityReference.Value
        //     property or if one of the objects is null and the other is not.
        //
        // Parameters:
        //   right:
        //     The right System.Security.Principal.IdentityReference operand to use for
        //     the inequality comparison. This parameter can be null.
        //
        //   left:
        //     The left System.Security.Principal.IdentityReference operand to use for the
        //     inequality comparison. This parameter can be null.
        //
        // Returns:
        //     true if left and right are not equal; otherwise, false.
        public static bool operator !=(IdentityReference left, IdentityReference right)
        {
            throw new Exception("The method or operation is not implemented.");
        }
        //
        // Summary:
        //     Compares two System.Security.Principal.IdentityReference objects to determine
        //     whether they are equal. They are considered equal if they have the same canonical
        //     name representation as the one returned by the System.Security.Principal.IdentityReference.Value
        //     property or if they are both null.
        //
        // Parameters:
        //   right:
        //     The right System.Security.Principal.IdentityReference operand to use for
        //     the equality comparison. This parameter can be null.
        //
        //   left:
        //     The left System.Security.Principal.IdentityReference operand to use for the
        //     equality comparison. This parameter can be null.
        //
        // Returns:
        //     true if left and right are equal; otherwise, false.
        public static bool operator ==(IdentityReference left, IdentityReference right)
        {
            throw new Exception("The method or operation is not implemented.");
        }

        // Summary:
        //     Gets the string value of the identity represented by the System.Security.Principal.IdentityReference
        //     object.
        //
        // Returns:
        //     The string value of the identity represented by the System.Security.Principal.IdentityReference
        //     object.
        public abstract string Value { get; }

        // Summary:
        //     Returns a value that indicates whether the specified object equals this instance
        //     of the System.Security.Principal.IdentityReference class.
        //
        // Parameters:
        //   o:
        //     An object to compare with this System.Security.Principal.IdentityReference
        //     instance, or a null reference.
        //
        // Returns:
        //     true if o is an object with the same underlying type and value as this System.Security.Principal.IdentityReference
        //     instance; otherwise, false.
        public abstract override bool Equals(object o);
        //
        // Summary:
        //     Serves as a hash function for System.Security.Principal.IdentityReference.
        //     System.Security.Principal.IdentityReference.GetHashCode() is suitable for
        //     use in hashing algorithms and data structures like a hash table.
        //
        // Returns:
        //     The hash code for this System.Security.Principal.IdentityReference object.
        public abstract override int GetHashCode();
        //
        // Summary:
        //     Returns a value that indicates whether the specified type is a valid translation
        //     type for the System.Security.Principal.IdentityReference class.
        //
        // Parameters:
        //   targetType:
        //     The type being queried for validity to serve as a conversion from System.Security.Principal.IdentityReference.
        //     The following target types are valid:System.Security.Principal.NTAccountSystem.Security.Principal.SecurityIdentifier
        //
        // Returns:
        //     true if targetType is a valid translation type for the System.Security.Principal.IdentityReference
        //     class; otherwise, false.
        public abstract bool IsValidTargetType(Type targetType);
        //
        // Summary:
        //     Returns the string representation of the identity represented by the System.Security.Principal.IdentityReference
        //     object.
        //
        // Returns:
        //     The identity in string format.
        public abstract override string ToString();
        //
        // Summary:
        //     Translates the account name represented by the System.Security.Principal.IdentityReference
        //     object into another System.Security.Principal.IdentityReference-derived type.
        //
        // Parameters:
        //   targetType:
        //     The target type for the conversion from System.Security.Principal.IdentityReference.
        //
        // Returns:
        //     The converted identity.
        public abstract IdentityReference Translate(Type targetType);
    }
}
