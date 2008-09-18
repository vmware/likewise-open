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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        debug.c
 *
 * Abstract:
 *
 *        Debug routines
 *
 * Author: Todd Stecher (2007)
 *
 */
#include "includes.h"

DWORD db_level = D_TRACE | D_ERROR | D_WARN | D_BYTES; /* @todo for now */


void 
DBGDumpBin(
    PBYTE buffer,
    ULONG length
    )
{

    ULONG i,index;
    CHAR hex[]="0123456789abcdef";
    CHAR lines[100];
    ULONG lineNum, count;

    if ((db_level & D_BYTES) == 0)
        return;


    for(index = 0; length; 
        length -= count, buffer += count, index += count) 
    {
        count = (length > 16) ? 16:length;
        sprintf(lines,  "%4.4x  ",index);
        lineNum = 6;

        for(i=0;i<count;i++) 
        {
            lines[lineNum++] = hex[buffer[i] >> 4]; 
            lines[lineNum++] = hex[buffer[i] & 0x0f];
            if(i == 7) 
                lines[lineNum++] = ':';
            else 
                lines[lineNum++] = '.';

        } 

        /* fill end w/ spaces */
        for(; i < 16; i++) 
        {
            lines[lineNum++] = ' ';
            lines[lineNum++] = ' ';
            lines[lineNum++] = ' ';
        }

        lines[lineNum++] = ' ';

        for(i = 0; i < count; i++) 
        {

            if(buffer[i] < 32 || buffer[i] > 126) 
                lines[lineNum++] = '.';
            else 
                lines[lineNum++] = buffer[i];

        }

        lines[lineNum++] = 0;
        DBG(D_ERROR,("%s\n", lines));

    }

}



#if 0

void
DumpSecBufferType(SecBuffer *sb)
{

    switch (sb->BufferType & ~SECBUFFER_ATTRMASK)

    {

        case SECBUFFER_EMPTY:          

            printf(" empty ");

            break;

        case SECBUFFER_DATA:

            printf(" data ");

            break;

        case SECBUFFER_TOKEN:

            printf(" token ");

            break;

        case SECBUFFER_PKG_PARAMS:

            printf(" params ");

            break;

        case SECBUFFER_MISSING:

            printf(" missing ");

            break;

        case SECBUFFER_EXTRA:

            printf(" extra ");

            break;

        case SECBUFFER_STREAM_TRAILER:

            printf(" trailer ");

            break;

        case SECBUFFER_STREAM_HEADER:

            printf(" header ");

            break;

        case SECBUFFER_NEGOTIATION_INFO:

            printf(" neghint ");

            break;

        case SECBUFFER_PADDING:

            printf(" header ");

            break;

        case SECBUFFER_STREAM:

            printf(" stream / msg ");

            break;

        case SECBUFFER_MECHLIST:

            printf(" mechlist ");

            break;

        case SECBUFFER_MECHLIST_SIGNATURE:

            printf(" mechlist-mic ");

            break;

        case SECBUFFER_TARGET:

            printf(" target - obs ");

            break;

        case SECBUFFER_CHANNEL_BINDINGS:

            printf(" channel-bind ");

            break;

        case SECBUFFER_CHANGE_PASS_RESPONSE:

            printf(" chpwd ");

            break;

        default:

            printf(" ? ");

            break;

    }



    switch (sb->BufferType & SECBUFFER_ATTRMASK)

    {

	case SECBUFFER_READONLY:

		printf("RO ");

		break;

	case SECBUFFER_READONLY_WITH_CHECKSUM:

		printf("RO CHKSUM ");

		break;

	}

	printf("\n");

}


#endif  /* if 0 */

/*
 * @brief DBGDumpSecBuffer
 *
 * Useful routine for dumping a sec buffer
 * 
 * @param lvl - debug level
 * @param optional msg - msg to print with dump
 * @param secBuf - sec buf to dump
 * @param dumpBytes - dump contents of the buffer
 *
 */ 
void
DBGDumpSecBuffer(
    DWORD lvl,
    char *msg,
    PSEC_BUFFER secBuf
)
{
    if (msg)
        DBG(lvl,("%s\n", msg));

    DBG(lvl,("len:%d max:%d buf:0x%p\n", 
            secBuf->length, 
            secBuf->maxLength,
            secBuf->buffer
    ));
    
    DBGDumpBin(secBuf->buffer, secBuf->length);
}







