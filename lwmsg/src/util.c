/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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

/*
 * Module Name:
 *
 *        util.c
 *
 * Abstract:
 *
 *        Utility functions
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#include <config.h>
#include "util-private.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#if HAVE_SYS_VARARGS_H
#include <sys/varargs.h>
#endif
#include <iconv.h>
#include <errno.h>

char* lwmsg_formatv(const char* fmt, va_list ap)
{
    int len;
    va_list my_ap;
    char* str;

    /* Some versions of vsnprintf won't accept
       a null or zero-length buffer */
    str = malloc(1);
    
    va_copy(my_ap, ap);
    
    len = vsnprintf(str, 1, fmt, my_ap);
    
    /* Some versions of vsnprintf return -1 when
       the buffer was too small rather than the
       number of characters that would be written,
       so we have loop in search of a large enough
       buffer */
    if (len == -1)
    {
        int capacity = 16;
        do
        {
            capacity *= 2;
            va_copy(my_ap, ap);
            str = realloc(str, capacity);
        } while ((len = vsnprintf(str, capacity-1, fmt, my_ap)) == -1 || capacity <= len);
        str[len] = '\0';
        
        return str;
    }
    else
    {
        va_copy(my_ap, ap);
        
        str = realloc(str, len+1);
        
        if (vsnprintf(str, len+1, fmt, my_ap) < len)
            return NULL;
        else
            return str;
    }
}

char* lwmsg_format(const char* fmt, ...)
{
    va_list ap;
    char* str;
    va_start(ap, fmt);
    str = lwmsg_formatv(fmt, ap);
    va_end(ap);
    return str;
}

ssize_t
lwmsg_convert_string_alloc(
    void* input,
    size_t input_len,
    void** output,
    const char* input_type,
    const char* output_type
    )
{
    size_t cblen;
    char *buffer;
    if (input == NULL)
        return -1;
    cblen = lwmsg_convert_string_buffer(input, input_len, NULL, 0, input_type, output_type);
    buffer = malloc(cblen);
    if (buffer == NULL)
        return -1;
    if (lwmsg_convert_string_buffer(input, input_len, buffer, cblen, input_type, output_type) != cblen)
    {
        free(buffer);
        return -1;
    }
    else
    {
        *output = buffer;
        return cblen;
    }
}

ssize_t
lwmsg_convert_string_buffer(
    void* input,
    size_t input_len,
    void* output,
    size_t output_len,
    const char* input_type,
    const char* output_type
    )
{
    iconv_t handle = iconv_open(output_type, input_type);
    char *inbuf = (char *)input;
    char *outbuf = (char *)output;
    size_t cbin = input_len;
    size_t cbout = output_len;
    size_t converted;

    if(outbuf == NULL)
    {
        char buffer[100];
        size_t cblen = 0;
        while(cbin > 0)
        {
            outbuf = buffer;
            cbout = sizeof(buffer);
            converted = iconv(handle, (ICONV_IN_TYPE) &inbuf, &cbin, &outbuf, &cbout);
            if(converted == (size_t)-1)
            {
                if(errno != E2BIG)
                {
                    cblen = -1;
                    fprintf(stderr, "error: %s\n", strerror(errno));
                    break;
                }
            }
            cblen += sizeof(buffer) - cbout;
        }
        iconv_close(handle);
        return cblen;
    }
    converted = iconv(handle, (ICONV_IN_TYPE) &inbuf, &cbin, &outbuf, &cbout);

    iconv_close(handle);
    if(converted == (size_t)-1 && cbout != 0)
        return -1;
    else
        return output_len - cbout;
}
