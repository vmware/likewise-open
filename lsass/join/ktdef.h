/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        ktdef.h
 *
 * Abstract:
 *
 *        Kerberos 5 keytab functions
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Rafal Szczesniak (rafal@likewisesoftware.com)
 *
 */

#ifndef __KTDEF_H__
#define __KTDEF_H__


#define BAIL_ON_KRB5_ERROR(ctx, krb5_err, winerr)         \
    if ((krb5_err)) {                                     \
        switch ((krb5_err))                               \
        {                                                 \
        case ENOENT:                                      \
            winerr = krb5_err;                            \
            break;                                        \
                                                          \
        case KRB5_LIBOS_BADPWDMATCH:                      \
            winerr = ERROR_WRONG_PASSWORD;                \
            break;                                        \
                                                          \
        case KRB5KDC_ERR_KEY_EXP:                         \
            winerr = ERROR_PASSWORD_EXPIRED;              \
            break;                                        \
                                                          \
        case KRB5KRB_AP_ERR_SKEW:                         \
            winerr = ERROR_TIME_SKEW;                     \
            break;                                        \
                                                          \
        default:                                          \
            winerr = LwTranslateKrb5Error(                \
                        (ctx),                            \
                        (krb5_err),                       \
                        __FUNCTION__,                     \
                        __FILE__,                         \
                        __LINE__);                        \
            break;                                        \
        }                                                 \
        goto error;                                       \
    }

#define BAIL_ON_KRB_ERROR(ctx, ret) \
    do { \
        if (ret) \
        { \
           (dwError) = LwTranslateKrb5Error(ctx, ret, __FUNCTION__, __FILE__, __LINE__); \
           goto error; \
        } \
    } while (0)

#endif /* __KTDEF_H__ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
