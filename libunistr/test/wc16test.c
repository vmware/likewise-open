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

/* ex: set tabstop=4 expandtab shiftwidth=4: */
#include <wc16str.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <iconv.h>
#include <string.h>
#include <wchar.h>

#define ASSERT(x) do { if(!(x)) { fprintf(stderr, "Test [%s] on line %d of file %s failed.\n", #x, __LINE__, __FILE__); exit(1); } } while(0)

void CheckCharToWchar16(const char *input)
{
    size_t inputlen = mbstrlen(input);
    if(input[0] != '\0')
        ASSERT(inputlen != 0);
    wchar16_t *allocated = malloc((inputlen + 1) * sizeof(wchar16_t));
    ASSERT(allocated != NULL);
    size_t converted = mbstowc16s(allocated, input, inputlen + 1);
    ASSERT(converted == inputlen);
    ASSERT(wc16slen(allocated) == inputlen);
    free(allocated);

    allocated = ambstowc16s(input);
    ASSERT(allocated != NULL);
    ASSERT(wc16slen(allocated) == inputlen);

    char *convertedback = awc16stombs(allocated);
    ASSERT(convertedback != NULL);
    ASSERT(!strcmp(input, convertedback));

    free(convertedback);
    free(allocated);
}

void CheckWcharToWchar16(const wchar_t *input)
{
    size_t inputlen = wcslen(input);
    wchar16_t *allocated = malloc((inputlen + 1) * sizeof(wchar16_t));
    ASSERT(allocated != NULL);
    size_t converted = wcstowc16s(allocated, input, inputlen + 1);
    ASSERT(converted == inputlen);
    ASSERT(wc16slen(allocated) == inputlen);
    free(allocated);

    int free_allocated;
    allocated = awcstowc16s(input, &free_allocated);
    ASSERT(allocated != NULL);
    ASSERT(wc16slen(allocated) == inputlen);

    int free_convertedback;
    wchar_t *convertedback = awc16stowcs(allocated, &free_convertedback);
    ASSERT(convertedback != NULL);
    ASSERT(!wcscmp(input, convertedback));

    if(free_convertedback)
        free(convertedback);
    if(free_allocated)
        free(allocated);
}

int main()
{
    setlocale(LC_ALL, "en_US.UTF-8");
   
    CheckCharToWchar16("a simple test string");
    char buffer[1024];
    int i;
    for(i = 0; i < sizeof(buffer); i++)
    {
        buffer[i] = '\0';
        if(i > 0)
            buffer[i - 1] = 'a';
        CheckCharToWchar16(buffer);
    }

    CheckWcharToWchar16(L"a simple test string");
    wchar_t wbuffer[1024];
    for(i = 0; i < sizeof(buffer); i++)
    {
        wbuffer[i] = '\0';
        if(i > 0)
            wbuffer[i - 1] = L'a';
        CheckWcharToWchar16(wbuffer);
    }

    wchar_t *japanese = L"日本語";
    CheckWcharToWchar16(japanese);

    int free_wc16s_japanese;
    wchar16_t *wc16s_japanese = awcstowc16s(japanese, &free_wc16s_japanese);
    char *utf8_japanese = awc16stombs(wc16s_japanese);
    ASSERT(strlen(utf8_japanese) > wcslen(japanese));
    ASSERT(mbstrlen(utf8_japanese) == wcslen(japanese));
    wchar16_t *convertedback = ambstowc16s(utf8_japanese);
    ASSERT(!memcmp(wc16s_japanese, convertedback, (wcslen(japanese) + 1) * sizeof(wchar16_t)));

    if(free_wc16s_japanese)
        free(wc16s_japanese);
    free(utf8_japanese);
    free(convertedback);

    return 0;
}
