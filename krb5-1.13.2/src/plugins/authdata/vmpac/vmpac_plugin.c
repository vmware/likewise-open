/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* lib/krb5/krb/pac.c */
/*
 * Copyright 2008 by the Massachusetts Institute of Technology.
 * All Rights Reserved.
 *
 * Export of this software from the United States of America may
 *   require a specific license from the United States Government.
 *   It is the responsibility of any person or organization contemplating
 *   export to obtain such a license before exporting.
 *
 * WITHIN THAT CONSTRAINT, permission to use, copy, modify, and
 * distribute this software and its documentation for any purpose and
 * without fee is hereby granted, provided that the above copyright
 * notice appear in all copies and that both that copyright notice and
 * this permission notice appear in supporting documentation, and that
 * the name of M.I.T. not be used in advertising or publicity pertaining
 * to distribution of the software without specific, written prior
 * permission.  Furthermore if you modify this software you must label
 * your software as modified software and not distribute it in such a
 * fashion that it might be confused with the original M.I.T. software.
 * M.I.T. makes no representations about the suitability of
 * this software for any purpose.  It is provided "as is" without express
 * or implied warranty.
 */

#include "k5-int.h"
#include <krb5/authdata_plugin.h>
#include "../../../lib/krb5/krb/authdata.h"
#include <assert.h>

#define KRB5_AUTHDATA_VM_PAC -121
#define KRB5_PAC_VMDIR_ACCESS_INFO 1

/*
 * VMPAC auth data attribute backend
 */
struct vmpac_context {
    krb5_pac pac;
};

static krb5_error_code
vmpac_init(krb5_context kcontext, void **plugin_context)
{
    *plugin_context = NULL;
    return 0;
}

static void
vmpac_flags(krb5_context kcontext,
            void *plugin_context,
            krb5_authdatatype ad_type,
            krb5_flags *flags)
{
    *flags = AD_USAGE_TGS_REQ;
}

static void
vmpac_fini(krb5_context kcontext, void *plugin_context)
{
    return;
}

static krb5_error_code
vmpac_request_init(krb5_context kcontext,
                   krb5_authdata_context context,
                   void *plugin_context,
                   void **request_context)
{
    struct vmpac_context *pacctx;

    pacctx = (struct vmpac_context *)malloc(sizeof(*pacctx));
    if (pacctx == NULL)
        return ENOMEM;

    pacctx->pac = NULL;

    *request_context = pacctx;

    return 0;
}

static krb5_error_code
vmpac_import_authdata(krb5_context kcontext,
                      krb5_authdata_context context,
                      void *plugin_context,
                      void *request_context,
                      krb5_authdata **authdata,
                      krb5_boolean kdc_issued,
                      krb5_const_principal kdc_issuer)
{
    krb5_error_code code;
    struct vmpac_context *pacctx = (struct vmpac_context *)request_context;

    if (kdc_issued)
        return EINVAL;

    if (pacctx->pac != NULL) {
        krb5_pac_free(kcontext, pacctx->pac);
        pacctx->pac = NULL;
    }

    assert(authdata[0] != NULL);
#if 0
    assert((authdata[0]->ad_type & AD_TYPE_FIELD_TYPE_MASK) ==
           KRB5_AUTHDATA_VM_PAC);
#endif

    code = krb5_pac_parse(kcontext, authdata[0]->contents,
                          authdata[0]->length, &pacctx->pac);

    return code;
}

static krb5_error_code
vmpac_export_authdata(krb5_context kcontext,
                      krb5_authdata_context context,
                      void *plugin_context,
                      void *request_context,
                      krb5_flags usage,
                      krb5_authdata ***out_authdata)
{
    struct vmpac_context *pacctx = (struct vmpac_context *)request_context;
    krb5_error_code code;
    krb5_authdata **authdata;
    krb5_data data;

    if (pacctx->pac == NULL)
        return 0;

    authdata = calloc(2, sizeof(krb5_authdata *));
    if (authdata == NULL)
        return ENOMEM;

    authdata[0] = calloc(1, sizeof(krb5_authdata));
    if (authdata[0] == NULL) {
        free(authdata);
        return ENOMEM;
    }
    authdata[1] = NULL;

    code = krb5int_copy_data_contents(kcontext, &pacctx->pac->data, &data);
    if (code != 0) {
        krb5_free_authdata(kcontext, authdata);
        return code;
    }

    authdata[0]->magic = KV5M_AUTHDATA;
    authdata[0]->ad_type = KRB5_AUTHDATA_VM_PAC;
    authdata[0]->length = data.length;
    authdata[0]->contents = (krb5_octet *)data.data;

    authdata[1] = NULL;

    *out_authdata = authdata;

    return 0;
}

static krb5_error_code
vmpac_verify(krb5_context kcontext,
             krb5_authdata_context context,
             void *plugin_context,
             void *request_context,
             const krb5_auth_context *auth_context,
             const krb5_keyblock *key,
             const krb5_ap_req *req)
{
    krb5_error_code code;
    struct vmpac_context *pacctx = (struct vmpac_context *)request_context;

    if (pacctx->pac == NULL)
        return EINVAL;

    code = krb5_pac_verify(kcontext, pacctx->pac,
                           req->ticket->enc_part2->times.authtime,
                           req->ticket->enc_part2->client, key, NULL);
    if (code != 0)
        TRACE_MSPAC_VERIFY_FAIL(kcontext, code);

    /*
     * If the above verification failed, don't fail the whole authentication,
     * just don't mark the PAC as verified.  A checksum mismatch can occur if
     * the PAC was copied from a cross-realm TGT by an ignorant KDC, and Apple
     * Mac OS X Server Open Directory (as of 10.6) generates PACs with no
     * server checksum at all.
     */
    return 0;
}

static void
vmpac_request_fini(krb5_context kcontext,
                   krb5_authdata_context context,
                   void *plugin_context,
                   void *request_context)
{
    struct vmpac_context *pacctx = (struct vmpac_context *)request_context;

    if (pacctx != NULL) {
        if (pacctx->pac != NULL)
            krb5_pac_free(kcontext, pacctx->pac);

        free(pacctx);
    }
}

#define STRLENOF(x) (sizeof((x)) - 1)

static struct {
    krb5_ui_4 type;
    krb5_data attribute;
} vmpac_attribute_types[] = {
    { (krb5_ui_4)-1,            { KV5M_DATA, STRLENOF("urn:vmpac:"),
                                  "urn:vmpac:" } },
    { KRB5_PAC_VMDIR_ACCESS_INFO,   { KV5M_DATA,
                                   STRLENOF("urn:vmpac:access-info"),
                                   "urn:vmpac:access-info" } },
    { KRB5_PAC_SERVER_CHECKSUM,  { KV5M_DATA,
                                   STRLENOF("urn:vmpac:server-checksum"),
                                   "urn:vmpac:server-checksum" } },
    { KRB5_PAC_PRIVSVR_CHECKSUM, { KV5M_DATA,
                                   STRLENOF("urn:vmpac:privsvr-checksum"),
                                   "urn:vmpac:privsvr-checksum" } },
};

#define VMPAC_ATTRIBUTE_COUNT   (sizeof(vmpac_attribute_types)/sizeof(vmpac_attribute_types[0]))

static krb5_error_code
vmpac_type2attr(krb5_ui_4 type, krb5_data *attr)
{
    unsigned int i;

    for (i = 0; i < VMPAC_ATTRIBUTE_COUNT; i++) {
        if (vmpac_attribute_types[i].type == type) {
            *attr = vmpac_attribute_types[i].attribute;
            return 0;
        }
    }

    return ENOENT;
}

static krb5_error_code
vmpac_attr2type(const krb5_data *attr, krb5_ui_4 *type)
{
    unsigned int i;

    for (i = 0; i < VMPAC_ATTRIBUTE_COUNT; i++) {
        if (attr->length == vmpac_attribute_types[i].attribute.length &&
            strncasecmp(attr->data, vmpac_attribute_types[i].attribute.data, attr->length) == 0) {
            *type = vmpac_attribute_types[i].type;
            return 0;
        }
    }

    if (attr->length > STRLENOF("urn:vmpac:") &&
        strncasecmp(attr->data, "urn:vmpac:", STRLENOF("urn:vmpac:")) == 0)
    {
        char *p = &attr->data[STRLENOF("urn:vmpac:")];
        char *endptr;

        *type = strtoul(p, &endptr, 10);
        if (*type != 0 && *endptr == '\0')
            return 0;
    }

    return ENOENT;
}

static krb5_error_code
vmpac_get_attribute_types(krb5_context kcontext,
                          krb5_authdata_context context,
                          void *plugin_context,
                          void *request_context,
                          krb5_data **out_attrs)
{
    struct vmpac_context *pacctx = (struct vmpac_context *)request_context;
    unsigned int i, j;
    krb5_data *attrs;
    krb5_error_code code;

    if (pacctx->pac == NULL)
        return ENOENT;

    attrs = calloc(1 + pacctx->pac->pac->cBuffers + 1, sizeof(krb5_data));
    if (attrs == NULL)
        return ENOMEM;

    j = 0;

    /* The entire PAC */
    code = krb5int_copy_data_contents(kcontext,
                                      &vmpac_attribute_types[0].attribute,
                                      &attrs[j++]);
    if (code != 0) {
        free(attrs);
        return code;
    }

    /* PAC buffers */
    for (i = 0; i < pacctx->pac->pac->cBuffers; i++) {
        krb5_data attr;

        code = vmpac_type2attr(pacctx->pac->pac->Buffers[i].ulType, &attr);
        if (code == 0) {
            code = krb5int_copy_data_contents(kcontext, &attr, &attrs[j++]);
            if (code != 0) {
                krb5int_free_data_list(kcontext, attrs);
                return code;
            }
        } else {
            int length;

            length = asprintf(&attrs[j].data, "urn:vmpac:%d",
                              pacctx->pac->pac->Buffers[i].ulType);
            if (length < 0) {
                krb5int_free_data_list(kcontext, attrs);
                return ENOMEM;
            }
            attrs[j++].length = length;
        }
    }
    attrs[j].data = NULL;
    attrs[j].length = 0;

    *out_attrs = attrs;

    return 0;
}

krb5_error_code
k5_pac_locate_buffer(krb5_context context,
                     const krb5_pac pac,
                     krb5_ui_4 type,
                     krb5_data *data)
{
    PAC_INFO_BUFFER *buffer = NULL;
    size_t i;

    if (pac == NULL)
        return EINVAL;

    for (i = 0; i < pac->pac->cBuffers; i++) {
        if (pac->pac->Buffers[i].ulType == type) {
            if (buffer == NULL)
                buffer = &pac->pac->Buffers[i];
            else
                return EINVAL;
        }
    }

    if (buffer == NULL)
        return ENOENT;

    assert(buffer->Offset + buffer->cbBufferSize <= pac->data.length);

    if (data != NULL) {
        data->length = buffer->cbBufferSize;
        data->data = pac->data.data + buffer->Offset;
    }

    return 0;
}

static krb5_error_code
vmpac_get_attribute(krb5_context kcontext,
                    krb5_authdata_context context,
                    void *plugin_context,
                    void *request_context,
                    const krb5_data *attribute,
                    krb5_boolean *authenticated,
                    krb5_boolean *complete,
                    krb5_data *value,
                    krb5_data *display_value,
                    int *more)
{
    struct vmpac_context *pacctx = (struct vmpac_context *)request_context;
    krb5_error_code code;
    krb5_ui_4 type;

    if (display_value != NULL) {
        display_value->data = NULL;
        display_value->length = 0;
    }

    if (*more != -1 || pacctx->pac == NULL)
        return ENOENT;

    /* If it didn't verify, pretend it didn't exist. */
    if (!pacctx->pac->verified) {
        TRACE_MSPAC_DISCARD_UNVERF(kcontext);
        return ENOENT;
    }

    code = vmpac_attr2type(attribute, &type);
    if (code != 0)
        return code;

    /* -1 is a magic type that refers to the entire PAC */
    if (type == (krb5_ui_4)-1) {
        if (value != NULL)
            code = krb5int_copy_data_contents(kcontext,
                                              &pacctx->pac->data,
                                              value);
        else
            code = 0;
    } else {
        if (value != NULL)
            code = krb5_pac_get_buffer(kcontext, pacctx->pac, type, value);
        else
            code = k5_pac_locate_buffer(kcontext, pacctx->pac, type, NULL);
    }
    if (code == 0) {
        *authenticated = pacctx->pac->verified;
        *complete = TRUE;
    }

    *more = 0;

    return code;
}

static krb5_error_code
vmpac_set_attribute(krb5_context kcontext,
                    krb5_authdata_context context,
                    void *plugin_context,
                    void *request_context,
                    krb5_boolean complete,
                    const krb5_data *attribute,
                    const krb5_data *value)
{
    struct vmpac_context *pacctx = (struct vmpac_context *)request_context;
    krb5_error_code code;
    krb5_ui_4 type;

    if (pacctx->pac == NULL)
        return ENOENT;

    code = vmpac_attr2type(attribute, &type);
    if (code != 0)
        return code;

    /* -1 is a magic type that refers to the entire PAC */
    if (type == (krb5_ui_4)-1) {
        krb5_pac newpac;

        code = krb5_pac_parse(kcontext, value->data, value->length, &newpac);
        if (code != 0)
            return code;

        krb5_pac_free(kcontext, pacctx->pac);
        pacctx->pac = newpac;
    } else {
        code = krb5_pac_add_buffer(kcontext, pacctx->pac, type, value);
    }

    return code;
}

static krb5_error_code
vmpac_export_internal(krb5_context kcontext,
                      krb5_authdata_context context,
                      void *plugin_context,
                      void *request_context,
                      krb5_boolean restrict_authenticated,
                      void **ptr)
{
    struct vmpac_context *pacctx = (struct vmpac_context *)request_context;
    krb5_error_code code;
    krb5_pac pac;

    *ptr = NULL;

    if (pacctx->pac == NULL)
        return ENOENT;

    if (restrict_authenticated && (pacctx->pac->verified) == FALSE)
        return ENOENT;

    code = krb5_pac_parse(kcontext, pacctx->pac->data.data,
                          pacctx->pac->data.length, &pac);
    if (code == 0) {
        pac->verified = pacctx->pac->verified;
        *ptr = pac;
    }

    return code;
}

static void
vmpac_free_internal(krb5_context kcontext,
                    krb5_authdata_context context,
                    void *plugin_context,
                    void *request_context,
                    void *ptr)
{
    if (ptr != NULL)
        krb5_pac_free(kcontext, (krb5_pac)ptr);

    return;
}

static krb5_error_code
vmpac_size(krb5_context kcontext,
           krb5_authdata_context context,
           void *plugin_context,
           void *request_context,
           size_t *sizep)
{
    struct vmpac_context *pacctx = (struct vmpac_context *)request_context;

    *sizep += sizeof(krb5_int32);

    if (pacctx->pac != NULL)
        *sizep += pacctx->pac->data.length;

    *sizep += sizeof(krb5_int32);

    return 0;
}

static krb5_error_code
vmpac_externalize(krb5_context kcontext,
                  krb5_authdata_context context,
                  void *plugin_context,
                  void *request_context,
                  krb5_octet **buffer,
                  size_t *lenremain)
{
    krb5_error_code code = 0;
    struct vmpac_context *pacctx = (struct vmpac_context *)request_context;
    size_t required = 0;
    krb5_octet *bp;
    size_t remain;

    bp = *buffer;
    remain = *lenremain;

    if (pacctx->pac != NULL) {
        vmpac_size(kcontext, context, plugin_context,
                   request_context, &required);

        if (required <= remain) {
            krb5_ser_pack_int32((krb5_int32)pacctx->pac->data.length,
                                &bp, &remain);
            krb5_ser_pack_bytes((krb5_octet *)pacctx->pac->data.data,
                                (size_t)pacctx->pac->data.length,
                                &bp, &remain);
            krb5_ser_pack_int32((krb5_int32)pacctx->pac->verified,
                                &bp, &remain);
        } else {
            code = ENOMEM;
        }
    } else {
        krb5_ser_pack_int32(0, &bp, &remain); /* length */
        krb5_ser_pack_int32(0, &bp, &remain); /* verified */
    }

    *buffer = bp;
    *lenremain = remain;

    return code;
}

static krb5_error_code
vmpac_internalize(krb5_context kcontext,
                  krb5_authdata_context context,
                  void *plugin_context,
                  void *request_context,
                  krb5_octet **buffer,
                  size_t *lenremain)
{
    struct vmpac_context *pacctx = (struct vmpac_context *)request_context;
    krb5_error_code code;
    krb5_int32 ibuf;
    krb5_octet *bp;
    size_t remain;
    krb5_pac pac = NULL;

    bp = *buffer;
    remain = *lenremain;

    /* length */
    code = krb5_ser_unpack_int32(&ibuf, &bp, &remain);
    if (code != 0)
        return code;

    if (ibuf != 0) {
        code = krb5_pac_parse(kcontext, bp, ibuf, &pac);
        if (code != 0)
            return code;

        bp += ibuf;
        remain -= ibuf;
    }

    /* verified */
    code = krb5_ser_unpack_int32(&ibuf, &bp, &remain);
    if (code != 0) {
        krb5_pac_free(kcontext, pac);
        return code;
    }

    if (pac != NULL) {
        pac->verified = (ibuf != 0);
    }

    if (pacctx->pac != NULL) {
        krb5_pac_free(kcontext, pacctx->pac);
    }

    pacctx->pac = pac;

    *buffer = bp;
    *lenremain = remain;

    return 0;
}

static krb5_error_code
k5_pac_copy(krb5_context context,
            krb5_pac src,
            krb5_pac *dst)
{
    size_t header_len;
    krb5_ui_4 cbuffers;
    krb5_error_code code;
    krb5_pac pac;

    cbuffers = src->pac->cBuffers;
    if (cbuffers != 0)
        cbuffers--;

    header_len = sizeof(PACTYPE) + cbuffers * sizeof(PAC_INFO_BUFFER);

    pac = (krb5_pac)malloc(sizeof(*pac));
    if (pac == NULL)
        return ENOMEM;

    pac->pac = k5memdup(src->pac, header_len, &code);
    if (pac->pac == NULL) {
        free(pac);
        return code;
    }

    code = krb5int_copy_data_contents(context, &src->data, &pac->data);
    if (code != 0) {
        free(pac->pac);
        free(pac);
        return ENOMEM;
    }

    pac->verified = src->verified;
    *dst = pac;

    return 0;
}

static krb5_error_code
vmpac_copy(krb5_context kcontext,
           krb5_authdata_context context,
           void *plugin_context,
           void *request_context,
           void *dst_plugin_context,
           void *dst_request_context)
{
    struct vmpac_context *srcctx = (struct vmpac_context *)request_context;
    struct vmpac_context *dstctx = (struct vmpac_context *)dst_request_context;
    krb5_error_code code = 0;

    assert(dstctx != NULL);
    assert(dstctx->pac == NULL);

    if (srcctx->pac != NULL)
        code = k5_pac_copy(kcontext, srcctx->pac, &dstctx->pac);

    return code;
}

static krb5_authdatatype vmpac_ad_types[] = {
    KRB5_AUTHDATA_VM_PAC,
    0
};

krb5plugin_authdata_client_ftable_v0 authdata_client_0 = {
    "vmpac",
    vmpac_ad_types,
    vmpac_init,
    vmpac_fini,
    vmpac_flags,
    vmpac_request_init,
    vmpac_request_fini,
    vmpac_get_attribute_types,
    vmpac_get_attribute,
    vmpac_set_attribute,
    NULL, /* delete_attribute_proc */
    vmpac_export_authdata,
    vmpac_import_authdata,
    vmpac_export_internal,
    vmpac_free_internal,
    vmpac_verify,
    vmpac_size,
    vmpac_externalize,
    vmpac_internalize,
    vmpac_copy
};
