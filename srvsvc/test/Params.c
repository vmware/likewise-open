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

#include <stdlib.h>
#include <string.h>

#include <config.h>
#include <types.h>
#include <wc16str.h>
#include <security.h>
//#include <lwrpc/ntstatus.h>

#include "Params.h"


#if !defined(HAVE_STRNDUP)
char* strndup(const char *s, size_t maxlen)
{
    char *ret;
    size_t len, size;

    len = strlen(s);
    len = (len > maxlen) ? maxlen : len;
    size = (len + 1) * sizeof(char);
    ret = (char*) malloc(size);
    strncpy(ret, s, len);

    /* ensure null termination */
    ret[len] = '\0';

    return ret;
}
#endif


static char* cleanup_sep(char *s, char sep)
{
    char *seppos;
    char sepstr[3] = {0};
    int i = 0;

    if (s == NULL) return s;

    sepstr[0] = '\\';
    sepstr[1] = sep;

    seppos = strstr(s, sepstr);
    while (seppos) {
        while (*seppos) seppos[0] = (seppos++)[1];
        seppos = strstr(s, sepstr);
    }

    return s;
}


char** get_string_list(char *list, const char sep)
{
    char **ret;
    char *start, *end = NULL;
    int i, count = 1;

    /* count the elements */
    start = list;
    end = strchr(list, sep);
    while (end++) {
        end = strchr(end, sep);

        /* skip any "escaped" separator */
        if (end > start && *(end-1) == '\\') continue;
        count++;
    }

    ret = (char**) malloc(sizeof(char*) * (count + 1));
    if (!ret) return NULL;

    memset((void*)ret, 0, sizeof(char*) * (count + 1));

    /* copy elements to the array */
    start = list;
    for (i = 0; i < count; i++) {
        end = strchr(start, sep);

        /* skip any "escaped" separator */
        while (start && end > start && *(end-1) == '\\') {
            char *pos = (char*)(end+1);
            end = strchr(pos,sep);
        }

        if (end) {
            ret[i] = strndup(start, (size_t)(end - start));
            start = &end[1];

        } else if (strlen(start)) {
            ret[i] = strdup(start);
        }

        ret[i] = cleanup_sep(ret[i], sep);
    }

    return ret;
}


struct parameter* get_optional_params(char *opt, int *count)
{
    const char separator = ',';
    const char equal = '=';
    struct parameter *params;
    char **opts;
    int i;

    if (!opt) {
        if (count) *count = 0;
        return NULL;
    }

    if (!count) return NULL;

    *count = 0;
    i = 0;

    opts = get_string_list(opt, separator);
    while (opts[(*count)]) (*count)++;

    params = (struct parameter*) malloc(sizeof(struct parameter) * (*count));

    while (opts[i]) {
        size_t key_size, val_size;
        char *param = opts[i];
        char *value = strchr(param, equal);

        if (param) {
            key_size = (size_t)(value - param);
            params[i].key = strndup(param, key_size);

            if (value) {
                val_size = strlen(param) - key_size - 1; /* equal char doesn't count */
                params[i].val = strndup((char*)(value + sizeof(char)), val_size);

            } else {
                /* if param is specified but does not equal to anything we can
                   assume it's a boolean flag set */
                params[i].val = strdup("1");
            }

            i++;
        }
    }

    return params;
}


const char* find_value(struct parameter *params, int count, const char *key)
{
    int i;

    for (i = 0; i < count; i++) {
        if (strstr(params[i].key, key)) return params[i].val;
    }

    return NULL;
}


enum param_err fetch_value(struct parameter *params, int count,
                           const char *key, enum param_type type,
                           void *val, const void *def)
{
    const char *value;
    NTSTATUS status;
    char **valstr, **defstr;
    char *valchar, *defchar;
    wchar16_t **valw16str, **defw16str;
    int *valint, *defint;
    unsigned int *valuint, *defuint;
    DomSid **valsid;
    enum param_err ret = perr_success;

    if (params && !key) return perr_nullptr_passed;
    if (!val) return perr_invalid_out_param;

    value = find_value(params, count, key);
    if (!value && !def) return perr_not_found;

    switch (type) {
    case pt_string:
        valstr = (char**)val;
        defstr = (char**)def;
        *valstr = (value) ? strdup(value) : strdup(*defstr);
        break;
    case pt_w16string:
        valw16str = (wchar16_t**)val;
        defstr = (char**)def;
        *valw16str = (value) ? ambstowc16s(value) : ambstowc16s(*defstr);
        break;
    case pt_char:
        valchar = (char*)val;
        defchar = (char*)def;
        *valchar = (value) ? value[0] : *defchar;
        break;
    case pt_int32:
        valint = (int*)val;
        defint = (int*)def;
        *valint = (value) ? atoi(value) : *defint;
        break;
    case pt_uint32:
        valuint = (unsigned int*)val;
        defuint = (unsigned int*)def;
        *valuint = (unsigned int)((value) ? atol(value) : *defuint);
        break;
#if 0
    case pt_sid:
        valsid = (DomSid**)val;
        /* default SID is passed as string here for convenience */
        defstr = (char**)def;
        status = ParseSidString(valsid,
                                ((value) ? (const char*)value : *defstr));
        if (status != STATUS_SUCCESS) return perr_invalid_out_param;
        break;
#endif
    default:
        return perr_unknown_type;
        break;
    }

    return ret;
}


const char *param_errstr(enum param_err perr)
{
    const errcount = sizeof(param_errstr_maps)/sizeof(struct param_errstr_map);
    int i = 0;

    while (i < errcount && perr != param_errstr_maps[i].perr) i++;
    return param_errstr_maps[i].desc;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
