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
    char* str, *str_new;

    /* Some versions of vsnprintf won't accept
       a null or zero-length buffer */
    str = malloc(1);

    if (!str)
    {
        return NULL;
    }
    
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
            str_new = realloc(str, capacity);
            if (!str_new)
            {
                free(str);
                return NULL;
            }
            str = str_new;
        } while ((len = vsnprintf(str, capacity-1, fmt, my_ap)) == -1 || capacity <= len);
        str[len] = '\0';
        
        return str;
    }
    else
    {
        va_copy(my_ap, ap);
        
        str_new = realloc(str, len+1);
        
        if (!str_new)
        {
            free(str);
            return NULL;
        }

        str = str_new;

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

LWMsgStatus
lwmsg_add_unsigned(
    size_t operand_a,
    size_t operand_b,
    size_t* result
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    size_t sum = operand_a + operand_b;

    if (sum < operand_a)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_OVERFLOW);
    }

    *result = sum;

error:

    return status;
}

#define SIZE_T_BITS (sizeof(size_t) * 8)
#define SIZE_T_HALF_BITS (SIZE_T_BITS / 2)
#define SIZE_T_LOWER_MASK (((size_t) (ssize_t) -1) >> SIZE_T_HALF_BITS)
#define SIZE_T_UPPER_MASK (~SIZE_T_LOWER_MASK)

LWMsgStatus
lwmsg_multiply_unsigned(
    size_t operand_a,
    size_t operand_b,
    size_t* result
    )
{
    /* Multiplication with overflow checking:

       Operands: x and y

       Let        r = 2^((8 * sizeof size_t) / 2)
       Let        x = ra + b
       Let        y = rc + d
       Therefore  xy = (ra + b) * (rc + d)
       Therefore  xy = (r^2)ac + rbc + rad + bd
       Assume     y <= x
       Therefore  c <= a
       If         c != 0
       Then       (r^2)ac >= r^2
       Therefore  xy >= r^2
       Therefore  raise overflow
       Otherwise  c = 0
       Therefore  xy = rad + bd
       If         ad >= r
       Then       rad >= r^2
       Therefore  xy >= r^2
       Therefore  raise overflow
       Perform    rad + bd with add overflow check
    */

    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    size_t x;
    size_t y;
    size_t a;
    size_t b;
    size_t c;
    size_t d;
    size_t ad;
    size_t bd;
    size_t rad;
    size_t product;

    /* Ensure y is the smaller operand */
    if (operand_b <= operand_a)
    {
        x = operand_a;
        y = operand_b;
    }
    else
    {
        x = operand_b;
        y = operand_a;
    }

    /* Calculate decomposition of operands */
    c = y >> SIZE_T_HALF_BITS;
    if (c != 0)
    {
        /* If c is not 0, the first term will overflow */
        BAIL_ON_ERROR(status = LWMSG_STATUS_OVERFLOW);
    }

    a = x >> SIZE_T_HALF_BITS;
    b = x & SIZE_T_LOWER_MASK;
    d = y & SIZE_T_LOWER_MASK;

    ad = a * d;

    if ((ad & SIZE_T_UPPER_MASK) != 0)
    {
        /* ad >= r, so rad will overflow */
        BAIL_ON_ERROR(status = LWMSG_STATUS_OVERFLOW);
    }

    rad = ad << SIZE_T_HALF_BITS;

    /* Never overflows */
    bd = b * d;

    product = rad + bd;

    if (product < rad)
    {
        /* Addition overflowed */
        BAIL_ON_ERROR(status = LWMSG_STATUS_OVERFLOW);
    }

    *result = product;

error:

    return status;
}
