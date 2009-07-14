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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "wc16str.h"
#include <stdlib.h>
#include <ctype.h>
#ifdef HAVE_WCTYPE_H
#    include <wctype.h>
#endif
#include <iconv.h>
#include <errno.h>
#include <wchar.h>
#include <string.h>
#include <autoconf.h>

#if SIZEOF_WCHAR_T == 2
#define WCHAR16_IS_WCHAR 1
#endif

size_t krb5_mbstowc16les(wchar16_t *dest, const char *src, size_t cchcopy)
{
    iconv_t handle = iconv_open("UCS-2LE", "");
    char *inbuf;
    char *outbuf;
    size_t cbin;
    size_t cbout;
    size_t converted;
    if(handle == (iconv_t)-1)
        return (size_t)-1;
    inbuf = (char *)src;
    outbuf = (char *)dest;
    cbin = strlen(src) * sizeof(src[0]);
    cbout = cchcopy * sizeof(dest[0]);
    converted = iconv(handle, (ICONV_IN_TYPE) &inbuf, &cbin, &outbuf, &cbout);

    if(cbout >= sizeof(dest[0]))
        *(wchar16_t *)outbuf = 0;
    iconv_close(handle);
    if(converted == (size_t)-1 && cbout != 0)
        return (size_t)-1;
    else
        return cchcopy - cbout/sizeof(dest[0]);
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
