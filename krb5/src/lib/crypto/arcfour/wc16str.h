/*
 * COPYRIGHT (c) 2009
 * The Regents of the University of Michigan
 * ALL RIGHTS RESERVED
 *
 * Permission is granted to use, copy, create derivative works
 * and redistribute this software and such derivative works
 * for any purpose, so long as the name of The University of
 * Michigan is not used in any advertising or publicity
 * pertaining to the use of distribution of this software
 * without specific, written prior authorization.  If the
 * above copyright notice or any other identification of the
 * University of Michigan is included in any copy of any
 * portion of this software, then the disclaimer below must
 * also be included.
 *
 * THIS SOFTWARE IS PROVIDED AS IS, WITHOUT REPRESENTATION
 * FROM THE UNIVERSITY OF MICHIGAN AS TO ITS FITNESS FOR ANY
 * PURPOSE, AND WITHOUT WARRANTY BY THE UNIVERSITY OF
 * MICHIGAN OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING
 * WITHOUT LIMITATION THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE
 * REGENTS OF THE UNIVERSITY OF MICHIGAN SHALL NOT BE LIABLE
 * FOR ANY DAMAGES, INCLUDING SPECIAL, INDIRECT, INCIDENTAL, OR
 * CONSEQUENTIAL DAMAGES, WITH RESPECT TO ANY CLAIM ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OF THE SOFTWARE, EVEN
 * IF IT HAS BEEN OR IS HEREAFTER ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGES.
 */

#ifndef WCSTR16_H

#include <stddef.h>

#ifndef WCHAR16_T_DEFINED
#define WCHAR16_T_DEFINED 1

#ifdef __GNUC__
typedef unsigned short int  wchar16_t;  /* 16-bit unsigned */

#elif _WIN32
typedef wchar_t             wchar16_t;
#define WCHAR16_IS_WCHAR
#endif

#endif /* WCHAR16_T_DEFINED */

#ifdef __GNUC__
#define TEXT(str)  str
#endif

/*Convert a multibyte character string to a little endian wchar16_t string and return the number of characters converted.
 *
 * cchn is the maximum number of characters to store in dest (including null).
 */
size_t krb5_mbstowc16les(wchar16_t *dest, const char *src, size_t cchn);

#endif /* WCSTR16_H */
