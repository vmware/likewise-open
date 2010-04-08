/*
 * lib/krb5/krb/pac.c
 *
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
 *
 */

#include "k5-int.h"
#include "k5-utf8.h"
#include "authdata.h"

/* draft-brezak-win2k-krb-authz-00 */

/*
 * A PAC consists of a sequence of PAC_INFO_BUFFERs, preceeded by
 * a PACTYPE header. Decoding the contents of the buffers is left
 * to the application (notwithstanding signature verification).
 */

typedef struct _PAC_INFO_BUFFER {
    krb5_ui_4 ulType;
    krb5_ui_4 cbBufferSize;
    krb5_ui_8 Offset;
} PAC_INFO_BUFFER;

#define PAC_INFO_BUFFER_LENGTH	16

/* ulType */
#define PAC_LOGON_INFO		1
#define PAC_CREDENTIALS_INFO	2
#define PAC_SERVER_CHECKSUM	6
#define PAC_PRIVSVR_CHECKSUM	7
#define PAC_CLIENT_INFO		10
#define PAC_DELEGATION_INFO	11
#define PAC_UPN_DNS_INFO	12

typedef struct _PACTYPE {
    krb5_ui_4 cBuffers;
    krb5_ui_4 Version;
    PAC_INFO_BUFFER Buffers[1];
} PACTYPE;

#define PAC_ALIGNMENT		    8
#define PACTYPE_LENGTH		    8U
#define PAC_SIGNATURE_DATA_LENGTH   4U
#define PAC_CLIENT_INFO_LENGTH	    10U

#define NT_TIME_EPOCH		    11644473600LL

struct krb5_pac_data {
    PACTYPE *pac;	/* PAC header + info buffer array */
    krb5_data data;	/* PAC data (including uninitialised header) */
    krb5_boolean verified;
};

static krb5_error_code
k5_pac_locate_buffer(krb5_context context,
		     const krb5_pac pac,
		     krb5_ui_4 type,
		     krb5_data *data);

/*
 * Add a buffer to the provided PAC and update header.
 */
static krb5_error_code
k5_pac_add_buffer(krb5_context context,
		  krb5_pac pac,
		  krb5_ui_4 type,
		  const krb5_data *data,
		  krb5_boolean zerofill,
		  krb5_data *out_data)
{
    PACTYPE *header;
    size_t header_len, i, pad = 0;
    char *pac_data;

    assert((data->data == NULL) == zerofill);

    /* Check there isn't already a buffer of this type */
    if (k5_pac_locate_buffer(context, pac, type, NULL) == 0) {
	return EEXIST;
    }

    header = (PACTYPE *)realloc(pac->pac,
				sizeof(PACTYPE) +
				(pac->pac->cBuffers * sizeof(PAC_INFO_BUFFER)));
    if (header == NULL) {
	return ENOMEM;
    }
    pac->pac = header;

    header_len = PACTYPE_LENGTH + (pac->pac->cBuffers * PAC_INFO_BUFFER_LENGTH);

    if (data->length % PAC_ALIGNMENT)
	pad = PAC_ALIGNMENT - (data->length % PAC_ALIGNMENT);

    pac_data = realloc(pac->data.data,
		       pac->data.length + PAC_INFO_BUFFER_LENGTH + data->length + pad);
    if (pac_data == NULL) {
	return ENOMEM;
    }
    pac->data.data = pac_data;

    /* Update offsets of existing buffers */
    for (i = 0; i < pac->pac->cBuffers; i++)
	pac->pac->Buffers[i].Offset += PAC_INFO_BUFFER_LENGTH;

    /* Make room for new PAC_INFO_BUFFER */
    memmove(pac->data.data + header_len + PAC_INFO_BUFFER_LENGTH,
	    pac->data.data + header_len,
	    pac->data.length - header_len);
    memset(pac->data.data + header_len, 0, PAC_INFO_BUFFER_LENGTH);

    /* Initialise new PAC_INFO_BUFFER */
    pac->pac->Buffers[i].ulType = type;
    pac->pac->Buffers[i].cbBufferSize = data->length;
    pac->pac->Buffers[i].Offset = pac->data.length + PAC_INFO_BUFFER_LENGTH;
    assert((pac->pac->Buffers[i].Offset % PAC_ALIGNMENT) == 0);

    /* Copy in new PAC data and zero padding bytes */
    if (zerofill)
	memset(pac->data.data + pac->pac->Buffers[i].Offset, 0, data->length);
    else
	memcpy(pac->data.data + pac->pac->Buffers[i].Offset, data->data, data->length);

    memset(pac->data.data + pac->pac->Buffers[i].Offset + data->length, 0, pad);

    pac->pac->cBuffers++;
    pac->data.length += PAC_INFO_BUFFER_LENGTH + data->length + pad;

    if (out_data != NULL) {
	out_data->data = pac->data.data + pac->pac->Buffers[i].Offset;
	out_data->length = data->length;
    }

    pac->verified = FALSE;

    return 0;
}

krb5_error_code KRB5_CALLCONV
krb5_pac_add_buffer(krb5_context context,
		    krb5_pac pac,
		    krb5_ui_4 type,
		    const krb5_data *data)
{
    return k5_pac_add_buffer(context, pac, type, data, FALSE, NULL);
}

/*
 * Free a PAC
 */
void KRB5_CALLCONV
krb5_pac_free(krb5_context context,
	      krb5_pac pac)
{
    if (pac != NULL) {
	if (pac->data.data != NULL) {
	    memset(pac->data.data, 0, pac->data.length);
	    free(pac->data.data);
	}
	if (pac->pac != NULL)
	    free(pac->pac);
	memset(pac, 0, sizeof(*pac));
	free(pac);
    }
}

static krb5_error_code
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

/*
 * Find a buffer and copy data into output
 */
krb5_error_code KRB5_CALLCONV
krb5_pac_get_buffer(krb5_context context,
		    krb5_pac pac,
		    krb5_ui_4 type,
		    krb5_data *data)
{
    krb5_data d;
    krb5_error_code ret;

    ret = k5_pac_locate_buffer(context, pac, type, &d);
    if (ret != 0)
	return ret;

    data->data = malloc(d.length);
    if (data->data == NULL)
	return ENOMEM;

    data->length = d.length;
    memcpy(data->data, d.data, d.length);

    return 0;
}

/*
 * Return an array of the types of data in the PAC
 */
krb5_error_code KRB5_CALLCONV
krb5_pac_get_types(krb5_context context,
		   krb5_pac pac,
		   size_t *len,
		   krb5_ui_4 **types)
{
    size_t i;

    *types = (krb5_ui_4 *)malloc(pac->pac->cBuffers * sizeof(krb5_ui_4));
    if (*types == NULL)
	return ENOMEM;

    *len = pac->pac->cBuffers;

    for (i = 0; i < pac->pac->cBuffers; i++)
	(*types)[i] = pac->pac->Buffers[i].ulType;

    return 0;
}

/*
 * Initialize PAC
 */
krb5_error_code KRB5_CALLCONV
krb5_pac_init(krb5_context context,
	      krb5_pac *ppac)
{
    krb5_pac pac;

    pac = (krb5_pac)malloc(sizeof(*pac));
    if (pac == NULL)
	return ENOMEM;

    pac->pac = (PACTYPE *)malloc(sizeof(PACTYPE));
    if (pac->pac == NULL) {
	free(pac);
	return ENOMEM;
    }

    pac->pac->cBuffers = 0;
    pac->pac->Version = 0;

    pac->data.length = PACTYPE_LENGTH;
    pac->data.data = calloc(1, pac->data.length);
    if (pac->data.data == NULL) {
	krb5_pac_free(context, pac);
	return ENOMEM;
    }

    pac->verified = FALSE;

    *ppac = pac;

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

    pac->pac = (PACTYPE *)malloc(header_len);
    if (pac->pac == NULL) {
	free(pac);
	return ENOMEM;
    }

    memcpy(pac->pac, src->pac, header_len);

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

/*
 * Parse the supplied data into the PAC allocated by this function
 */
krb5_error_code KRB5_CALLCONV
krb5_pac_parse(krb5_context context,
	       const void *ptr,
	       size_t len,
	       krb5_pac *ppac)
{
    krb5_error_code ret;
    size_t i;
    const unsigned char *p = (const unsigned char *)ptr;
    krb5_pac pac;
    size_t header_len;
    krb5_ui_4 cbuffers, version;

    *ppac = NULL;

    if (len < PACTYPE_LENGTH)
	return ERANGE;

    cbuffers = load_32_le(p);
    p += 4;
    version = load_32_le(p);
    p += 4;

    if (version != 0)
	return EINVAL;

    header_len = PACTYPE_LENGTH + (cbuffers * PAC_INFO_BUFFER_LENGTH);
    if (len < header_len)
	return ERANGE;

    ret = krb5_pac_init(context, &pac);
    if (ret != 0)
	return ret;

    pac->pac = (PACTYPE *)realloc(pac->pac,
	sizeof(PACTYPE) + ((cbuffers - 1) * sizeof(PAC_INFO_BUFFER)));
    if (pac->pac == NULL) {
	krb5_pac_free(context, pac);
	return ENOMEM;
    }

    pac->pac->cBuffers = cbuffers;
    pac->pac->Version = version;

    for (i = 0; i < pac->pac->cBuffers; i++) {
	PAC_INFO_BUFFER *buffer = &pac->pac->Buffers[i];

	buffer->ulType = load_32_le(p);
	p += 4;
	buffer->cbBufferSize = load_32_le(p);
	p += 4;
	buffer->Offset = load_64_le(p);
	p += 8;

	if (buffer->Offset % PAC_ALIGNMENT) {
	    krb5_pac_free(context, pac);
	    return EINVAL;
	}
	if (buffer->Offset < header_len ||
	    buffer->Offset + buffer->cbBufferSize > len) {
	    krb5_pac_free(context, pac);
	    return ERANGE;
	}
    }

    pac->data.data = realloc(pac->data.data, len);
    if (pac->data.data == NULL) {
	krb5_pac_free(context, pac);
	return ENOMEM;
    }
    memcpy(pac->data.data, ptr, len);

    pac->data.length = len;

    *ppac = pac;

    return 0;
}

static krb5_error_code
k5_time_to_seconds_since_1970(krb5_int64 ntTime,
			      krb5_timestamp *elapsedSeconds)
{
    krb5_ui_8 abstime;

    ntTime /= 10000000;

    abstime = ntTime > 0 ? ntTime - NT_TIME_EPOCH : -ntTime;

    if (abstime > KRB5_INT32_MAX)
	return ERANGE;

    *elapsedSeconds = abstime;

    return 0;
}

static krb5_error_code
k5_seconds_since_1970_to_time(krb5_timestamp elapsedSeconds,
			      krb5_ui_8 *ntTime)
{
    *ntTime = elapsedSeconds;

    if (elapsedSeconds > 0)
	*ntTime += NT_TIME_EPOCH;

    *ntTime *= 10000000;

    return 0;
}

static krb5_error_code
k5_pac_validate_client(krb5_context context,
		       const krb5_pac pac,
		       krb5_timestamp authtime,
		       krb5_const_principal principal)
{
    krb5_error_code ret;
    krb5_data client_info;
    char *decoded_pac_princname;
    char *appended_pac_princname;
    char *pac_princname;
    unsigned char *p;
    krb5_timestamp pac_authtime;
    krb5_ui_2 pac_princname_length;
    krb5_int64 pac_nt_authtime;
    krb5_principal pac_principal;

    ret = k5_pac_locate_buffer(context, pac, PAC_CLIENT_INFO, &client_info);
    if (ret != 0)
	return ret;

    if (client_info.length < PAC_CLIENT_INFO_LENGTH)
	return ERANGE;

    p = (unsigned char *)client_info.data;
    pac_nt_authtime = load_64_le(p);
    p += 8;
    pac_princname_length = load_16_le(p);
    p += 2;

    ret = k5_time_to_seconds_since_1970(pac_nt_authtime, &pac_authtime);
    if (ret != 0)
	return ret;

    if (client_info.length < PAC_CLIENT_INFO_LENGTH + pac_princname_length ||
	pac_princname_length % 2)
	return ERANGE;

    ret = krb5int_ucs2lecs_to_utf8s(p, (size_t)pac_princname_length / 2,
				    &decoded_pac_princname, NULL);
    if (ret != 0)
	return ret;

    if (!strchr(decoded_pac_princname, '@')) {
	/* Append a realm so the default realm in the conf file is is avoided */
	appended_pac_princname = malloc(pac_princname_length / 2 +
					principal->realm.length + 2);
	if (appended_pac_princname == NULL) {
	    free(decoded_pac_princname);
	    return(ENOMEM);
	}

	pac_princname = appended_pac_princname;

	memcpy(pac_princname, decoded_pac_princname,
		pac_princname_length / 2);
	pac_princname += pac_princname_length / 2;
	pac_princname[0] = '@';
	pac_princname++;
	memcpy(pac_princname, principal->realm.data, principal->realm.length);
	pac_princname += principal->realm.length;
	pac_princname[0] = 0;

	pac_princname = appended_pac_princname;
	free(decoded_pac_princname);
    } else {
	pac_princname = decoded_pac_princname;
    }

    ret = krb5_parse_name_flags(context, pac_princname, 0, &pac_principal);
    if (ret != 0) {
	free(pac_princname);
	return ret;
    }

    free(pac_princname);

    if (pac_authtime != authtime ||
	!krb5_principal_compare_flags(context,
				      pac_principal,
				      principal,
				      KRB5_PRINCIPAL_COMPARE_IGNORE_REALM))
	ret = KRB5KRB_AP_WRONG_PRINC;

    krb5_free_principal(context, pac_principal);

    return ret;
}

static krb5_error_code
k5_pac_zero_signature(krb5_context context,
		      const krb5_pac pac,
		      krb5_ui_4 type,
		      krb5_data *data)
{
    PAC_INFO_BUFFER *buffer = NULL;
    size_t i;

    assert(type == PAC_SERVER_CHECKSUM || type == PAC_PRIVSVR_CHECKSUM);
    assert(data->length >= pac->data.length);

    for (i = 0; i < pac->pac->cBuffers; i++) {
	if (pac->pac->Buffers[i].ulType == type) {
	    buffer = &pac->pac->Buffers[i];
	    break;
	}
    }

    if (buffer == NULL)
	return ENOENT;

    if (buffer->Offset + buffer->cbBufferSize > pac->data.length)
	return ERANGE;

    if (buffer->cbBufferSize < PAC_SIGNATURE_DATA_LENGTH)
	return KRB5_BAD_MSIZE;

    /* Zero out the data portion of the checksum only */
    memset(data->data + buffer->Offset + PAC_SIGNATURE_DATA_LENGTH,
	   0,
	   buffer->cbBufferSize - PAC_SIGNATURE_DATA_LENGTH);

    return 0;
}

static krb5_error_code
k5_pac_verify_server_checksum(krb5_context context,
			      const krb5_pac pac,
			      const krb5_keyblock *server)
{
    krb5_error_code ret;
    krb5_data pac_data; /* PAC with zeroed checksums */
    krb5_checksum checksum;
    krb5_data checksum_data;
    krb5_boolean valid;
    krb5_octet *p;

    ret = k5_pac_locate_buffer(context, pac,
			       PAC_SERVER_CHECKSUM, &checksum_data);
    if (ret != 0)
	return ret;

    if (checksum_data.length < PAC_SIGNATURE_DATA_LENGTH)
	return KRB5_BAD_MSIZE;

    p = (krb5_octet *)checksum_data.data;
    checksum.checksum_type = load_32_le(p);
    checksum.length = checksum_data.length - PAC_SIGNATURE_DATA_LENGTH;
    checksum.contents = p + PAC_SIGNATURE_DATA_LENGTH;

    pac_data.length = pac->data.length;
    pac_data.data = malloc(pac->data.length);
    if (pac_data.data == NULL)
	return ENOMEM;

    memcpy(pac_data.data, pac->data.data, pac->data.length);

    /* Zero out both checksum buffers */
    ret = k5_pac_zero_signature(context, pac,
				PAC_SERVER_CHECKSUM, &pac_data);
    if (ret != 0) {
	free(pac_data.data);
	return ret;
    }

    ret = k5_pac_zero_signature(context, pac,
				PAC_PRIVSVR_CHECKSUM, &pac_data);
    if (ret != 0) {
	free(pac_data.data);
	return ret;
    }

    ret = krb5_c_verify_checksum(context, server,
				 KRB5_KEYUSAGE_APP_DATA_CKSUM,
				 &pac_data, &checksum, &valid);

    free(pac_data.data);

    if (ret != 0) {
	return ret;
    }

    if (valid == FALSE)
	ret = KRB5KRB_AP_ERR_BAD_INTEGRITY;

    return ret;
}

static krb5_error_code
k5_pac_verify_kdc_checksum(krb5_context context,
			   const krb5_pac pac,
			   const krb5_keyblock *privsvr)
{
    krb5_error_code ret;
    krb5_data server_checksum, privsvr_checksum;
    krb5_checksum checksum;
    krb5_boolean valid;
    krb5_octet *p;

    ret = k5_pac_locate_buffer(context, pac,
			       PAC_PRIVSVR_CHECKSUM, &privsvr_checksum);
    if (ret != 0)
	return ret;

    if (privsvr_checksum.length < PAC_SIGNATURE_DATA_LENGTH)
	return KRB5_BAD_MSIZE;

    ret = k5_pac_locate_buffer(context, pac,
			       PAC_SERVER_CHECKSUM, &server_checksum);
    if (ret != 0)
	return ret;

    if (server_checksum.length < PAC_SIGNATURE_DATA_LENGTH)
	return KRB5_BAD_MSIZE;

    p = (krb5_octet *)privsvr_checksum.data;
    checksum.checksum_type = load_32_le(p);
    checksum.length = privsvr_checksum.length - PAC_SIGNATURE_DATA_LENGTH;
    checksum.contents = p + PAC_SIGNATURE_DATA_LENGTH;

    server_checksum.data += PAC_SIGNATURE_DATA_LENGTH;
    server_checksum.length -= PAC_SIGNATURE_DATA_LENGTH;

    ret = krb5_c_verify_checksum(context, privsvr,
				 KRB5_KEYUSAGE_APP_DATA_CKSUM,
				 &server_checksum, &checksum, &valid);
    if (ret != 0)
	return ret;

    if (valid == FALSE)
	ret = KRB5KRB_AP_ERR_BAD_INTEGRITY;

    return ret;
}

krb5_error_code KRB5_CALLCONV
krb5_pac_verify(krb5_context context,
		const krb5_pac pac,
		krb5_timestamp authtime,
		krb5_const_principal principal,
		const krb5_keyblock *server,
		const krb5_keyblock *privsvr)
{
    krb5_error_code ret;

    if (server == NULL)
	return EINVAL;

    ret = k5_pac_verify_server_checksum(context, pac, server);
    if (ret != 0)
	return ret;

    if (privsvr != NULL) {
	ret = k5_pac_verify_kdc_checksum(context, pac, privsvr);
	if (ret != 0)
	    return ret;
    }

    if (principal != NULL) {
	ret = k5_pac_validate_client(context, pac, authtime, principal);
	if (ret != 0)
	    return ret;
    }

    pac->verified = TRUE;

    return 0;
}

static krb5_error_code
k5_insert_client_info(krb5_context context,
		      krb5_pac pac,
		      krb5_timestamp authtime,
		      krb5_const_principal principal)
{
    krb5_error_code ret;
    krb5_data client_info;
    char *princ_name_utf8 = NULL;
    unsigned char *princ_name_ucs2 = NULL, *p;
    size_t princ_name_ucs2_len = 0;
    krb5_ui_8 nt_authtime;

    /* If we already have a CLIENT_INFO buffer, then just validate it */
    if (k5_pac_locate_buffer(context, pac,
			     PAC_CLIENT_INFO, &client_info) == 0) {
	return k5_pac_validate_client(context, pac, authtime, principal);
    }

    ret = krb5_unparse_name_flags(context, principal,
				  KRB5_PRINCIPAL_UNPARSE_NO_REALM,
				  &princ_name_utf8);
    if (ret != 0)
	goto cleanup;

    ret = krb5int_utf8s_to_ucs2les(princ_name_utf8,
				   &princ_name_ucs2,
				   &princ_name_ucs2_len);
    if (ret != 0)
	goto cleanup;

    client_info.length = PAC_CLIENT_INFO_LENGTH + princ_name_ucs2_len;
    client_info.data = NULL;

    ret = k5_pac_add_buffer(context, pac, PAC_CLIENT_INFO,
			    &client_info, TRUE, &client_info);
    if (ret != 0)
	goto cleanup;

    p = (unsigned char *)client_info.data;

    /* copy in authtime converted to a 64-bit NT time */
    k5_seconds_since_1970_to_time(authtime, &nt_authtime);
    store_64_le(nt_authtime, p);
    p += 8;

    /* copy in number of UCS-2 characters in principal name */
    store_16_le(princ_name_ucs2_len, p);
    p += 2;

    /* copy in principal name */
    memcpy(p, princ_name_ucs2, princ_name_ucs2_len);

cleanup:
    if (princ_name_ucs2 != NULL)
	free(princ_name_ucs2);
    krb5_free_unparsed_name(context, princ_name_utf8);

    return ret;
}

static krb5_error_code
k5_insert_checksum(krb5_context context,
		   krb5_pac pac,
		   krb5_ui_4 type,
		   const krb5_keyblock *key,
		   krb5_cksumtype *cksumtype)
{
    krb5_error_code ret;
    size_t len;
    krb5_data cksumdata;

    ret = krb5int_c_mandatory_cksumtype(context, key->enctype, cksumtype);
    if (ret != 0)
	return ret;

    ret = krb5_c_checksum_length(context, *cksumtype, &len);
    if (ret != 0)
	return ret;

    ret = k5_pac_locate_buffer(context, pac, type, &cksumdata);
    if (ret == 0) {
	/*
	 * If we're resigning PAC, make sure we can fit checksum
	 * into existing buffer
	 */
	if (cksumdata.length != PAC_SIGNATURE_DATA_LENGTH + len)
	    return ERANGE;

	memset(cksumdata.data, 0, cksumdata.length);
    } else {
	/* Add a zero filled buffer */
	cksumdata.length = PAC_SIGNATURE_DATA_LENGTH + len;
	cksumdata.data = NULL;

	ret = k5_pac_add_buffer(context, pac,
				type, &cksumdata,
				TRUE, &cksumdata);
	if (ret != 0)
	    return ret;
    }

    /* Encode checksum type into buffer */
    store_32_le((krb5_ui_4)*cksumtype, cksumdata.data);

    return 0;
}

/* in-place encoding of PAC header */
static krb5_error_code
k5_pac_encode_header(krb5_context context, krb5_pac pac)
{
    size_t i;
    unsigned char *p;
    size_t header_len;

    header_len = PACTYPE_LENGTH +
	(pac->pac->cBuffers * PAC_INFO_BUFFER_LENGTH);
    assert(pac->data.length >= header_len);

    p = (unsigned char *)pac->data.data;

    store_32_le(pac->pac->cBuffers, p);
    p += 4;
    store_32_le(pac->pac->Version, p);
    p += 4;

    for (i = 0; i < pac->pac->cBuffers; i++) {
	PAC_INFO_BUFFER *buffer = &pac->pac->Buffers[i];

	store_32_le(buffer->ulType, p);
	p += 4;
	store_32_le(buffer->cbBufferSize, p);
	p += 4;
	store_64_le(buffer->Offset, p);
	p += 8;

	assert((buffer->Offset % PAC_ALIGNMENT) == 0);
	assert(buffer->Offset + buffer->cbBufferSize <= pac->data.length);
	assert(buffer->Offset >= header_len);

	if (buffer->Offset % PAC_ALIGNMENT ||
	    buffer->Offset + buffer->cbBufferSize > pac->data.length ||
	    buffer->Offset < header_len)
	    return ERANGE;
    }

    return 0;
}

krb5_error_code KRB5_CALLCONV
krb5int_pac_sign(krb5_context context,
		 krb5_pac pac,
		 krb5_timestamp authtime,
		 krb5_const_principal principal,
		 const krb5_keyblock *server_key,
		 const krb5_keyblock *privsvr_key,
		 krb5_data *data)
{
    krb5_error_code ret;
    krb5_data server_cksum, privsvr_cksum;
    krb5_cksumtype server_cksumtype, privsvr_cksumtype;
    krb5_crypto_iov iov[2];

    data->length = 0;
    data->data = NULL;

    if (principal != NULL) {
	ret = k5_insert_client_info(context, pac, authtime, principal);
	if (ret != 0)
	    return ret;
    }

    /* Create zeroed buffers for both checksums */
    ret = k5_insert_checksum(context, pac, PAC_SERVER_CHECKSUM,
			     server_key, &server_cksumtype);
    if (ret != 0)
	return ret;

    ret = k5_insert_checksum(context, pac, PAC_PRIVSVR_CHECKSUM,
			     privsvr_key, &privsvr_cksumtype);
    if (ret != 0)
	return ret;

    /* Now, encode the PAC header so that the checksums will include it */
    ret = k5_pac_encode_header(context, pac);
    if (ret != 0)
	return ret;

    /* Generate the server checksum over the entire PAC */
    ret = k5_pac_locate_buffer(context, pac,
			       PAC_SERVER_CHECKSUM, &server_cksum);
    if (ret != 0)
	return ret;

    assert(server_cksum.length > PAC_SIGNATURE_DATA_LENGTH);

    iov[0].flags = KRB5_CRYPTO_TYPE_DATA;
    iov[0].data = pac->data;

    iov[1].flags = KRB5_CRYPTO_TYPE_CHECKSUM;
    iov[1].data.data = server_cksum.data + PAC_SIGNATURE_DATA_LENGTH;
    iov[1].data.length = server_cksum.length - PAC_SIGNATURE_DATA_LENGTH;

    ret = krb5_c_make_checksum_iov(context, server_cksumtype,
				   server_key, KRB5_KEYUSAGE_APP_DATA_CKSUM,
				   iov, sizeof(iov)/sizeof(iov[0]));
    if (ret != 0)
	return ret;

    /* Generate the privsvr checksum over the server checksum buffer */
    ret = k5_pac_locate_buffer(context, pac,
			       PAC_PRIVSVR_CHECKSUM, &privsvr_cksum);
    if (ret != 0)
	return ret;

    assert(privsvr_cksum.length > PAC_SIGNATURE_DATA_LENGTH);

    iov[0].flags = KRB5_CRYPTO_TYPE_DATA;
    iov[0].data.data = server_cksum.data + PAC_SIGNATURE_DATA_LENGTH;
    iov[0].data.length = server_cksum.length - PAC_SIGNATURE_DATA_LENGTH;

    iov[1].flags = KRB5_CRYPTO_TYPE_CHECKSUM;
    iov[1].data.data = privsvr_cksum.data + PAC_SIGNATURE_DATA_LENGTH;
    iov[1].data.length = privsvr_cksum.length - PAC_SIGNATURE_DATA_LENGTH;

    ret = krb5_c_make_checksum_iov(context, privsvr_cksumtype,
				   privsvr_key, KRB5_KEYUSAGE_APP_DATA_CKSUM,
				   iov, sizeof(iov)/sizeof(iov[0]));
    if (ret != 0)
	return ret;

    data->data = malloc(pac->data.length);
    if (data->data == NULL)
	return ENOMEM;

    data->length = pac->data.length;

    memcpy(data->data, pac->data.data, pac->data.length);
    memset(pac->data.data, 0,
	   PACTYPE_LENGTH + (pac->pac->cBuffers * PAC_INFO_BUFFER_LENGTH));

    return 0;
}

/*
 * PAC auth data attribute backend
 */
struct mspac_context {
    krb5_pac pac;
};

static krb5_error_code
mspac_init(krb5_context kcontext, void **plugin_context)
{
    *plugin_context = NULL;
    return 0;
}

static void
mspac_flags(krb5_context kcontext,
	    void *plugin_context,
	    krb5_authdatatype ad_type,
	    krb5_flags *flags)
{
    *flags = AD_USAGE_KDC_ISSUED;
}

static void
mspac_fini(krb5_context kcontext, void *plugin_context)
{
    return;
}

static krb5_error_code
mspac_request_init(krb5_context kcontext,
		   krb5_authdata_context context,
		   void *plugin_context,
		   void **request_context)
{
    struct mspac_context *pacctx;

    pacctx = (struct mspac_context *)malloc(sizeof(*pacctx));
    if (pacctx == NULL)
	return ENOMEM;

    pacctx->pac = NULL;

    *request_context = pacctx;

    return 0;
}

static krb5_error_code
mspac_import_authdata(krb5_context kcontext,
		      krb5_authdata_context context,
		      void *plugin_context,
		      void *request_context,
		      krb5_authdata **authdata,
		      krb5_boolean kdc_issued,
		      krb5_const_principal kdc_issuer)
{
    krb5_error_code code;
    struct mspac_context *pacctx = (struct mspac_context *)request_context;

    if (kdc_issued)
	return EINVAL;

    if (pacctx->pac != NULL) {
	krb5_pac_free(kcontext, pacctx->pac);
	pacctx->pac = NULL;
    }

    assert(authdata[0] != NULL);
    assert((authdata[0]->ad_type & AD_TYPE_FIELD_TYPE_MASK) ==
	KRB5_AUTHDATA_WIN2K_PAC);

    code = krb5_pac_parse(kcontext, authdata[0]->contents,
			  authdata[0]->length, &pacctx->pac);

    return code;
}

static krb5_error_code
mspac_export_authdata(krb5_context kcontext,
		      krb5_authdata_context context,
		      void *plugin_context,
		      void *request_context,
		      krb5_flags usage,
		      krb5_authdata ***out_authdata)
{
    struct mspac_context *pacctx = (struct mspac_context *)request_context;
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
    authdata[0]->ad_type = KRB5_AUTHDATA_WIN2K_PAC;
    authdata[0]->length = data.length;
    authdata[0]->contents = (krb5_octet *)data.data;

    authdata[1] = NULL;

    *out_authdata = authdata;

    return 0;
}

static krb5_error_code
mspac_verify(krb5_context kcontext,
	     krb5_authdata_context context,
	     void *plugin_context,
	     void *request_context,
	     const krb5_auth_context *auth_context,
	     const krb5_keyblock *key,
	     const krb5_ap_req *req)
{
    krb5_error_code code;
    struct mspac_context *pacctx = (struct mspac_context *)request_context;

    if (pacctx->pac == NULL)
	return EINVAL;

    code = krb5_pac_verify(kcontext,
			   pacctx->pac,
			   req->ticket->enc_part2->times.authtime,
			   req->ticket->enc_part2->client,
			   key,
			   NULL);

#if 0
    /*
     * Now, we could return 0 and just set pac->verified to FALSE.
     * Thoughts?
     */
    if (code == KRB5KRB_AP_ERR_BAD_INTEGRITY) {
	assert(pacctx->pac->verified == FALSE);
	code = 0;
    }
#endif

    return code;
}

static void
mspac_request_fini(krb5_context kcontext,
		   krb5_authdata_context context,
		   void *plugin_context,
		   void *request_context)
{
    struct mspac_context *pacctx = (struct mspac_context *)request_context;

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
} mspac_attribute_types[] = {
    { (krb5_ui_4)-1,		{ KV5M_DATA, STRLENOF("urn:mspac:"), "urn:mspac:" } },
    { PAC_LOGON_INFO,		{ KV5M_DATA, STRLENOF("urn:mspac:logon-info"), "urn:mspac:logon-info" } },
    { PAC_CREDENTIALS_INFO,	{ KV5M_DATA, STRLENOF("urn:mspac:credentials-info"), "urn:mspac:credentials-info" } },
    { PAC_SERVER_CHECKSUM,	{ KV5M_DATA, STRLENOF("urn:mspac:server-checksum"), "urn:mspac:server-checksum" } },
    { PAC_PRIVSVR_CHECKSUM,	{ KV5M_DATA, STRLENOF("urn:mspac:privsvr-checksum"), "urn:mspac:privsvr-checksum" } },
    { PAC_CLIENT_INFO,		{ KV5M_DATA, STRLENOF("urn:mspac:client-info"), "urn:mspac:client-info" } },
    { PAC_DELEGATION_INFO,	{ KV5M_DATA, STRLENOF("urn:mspac:delegation-info"), "urn:mspac:delegation-info" } },
    { PAC_UPN_DNS_INFO,		{ KV5M_DATA, STRLENOF("urn:mspac:upn-dns-info"), "urn:mspac:upn-dns-info" } },
};

#define MSPAC_ATTRIBUTE_COUNT	(sizeof(mspac_attribute_types)/sizeof(mspac_attribute_types[0]))

static krb5_error_code
mspac_type2attr(krb5_ui_4 type, krb5_data *attr)
{
    unsigned int i;

    for (i = 0; i < MSPAC_ATTRIBUTE_COUNT; i++) {
	if (mspac_attribute_types[i].type == type) {
	    *attr = mspac_attribute_types[i].attribute;
	    return 0;
	}
    }

    return ENOENT;
}

static krb5_error_code
mspac_attr2type(const krb5_data *attr, krb5_ui_4 *type)
{
    unsigned int i;

    for (i = 0; i < MSPAC_ATTRIBUTE_COUNT; i++) {
	if (attr->length == mspac_attribute_types[i].attribute.length &&
	    strncasecmp(attr->data, mspac_attribute_types[i].attribute.data, attr->length) == 0) {
	    *type = mspac_attribute_types[i].type;
	    return 0;
	}
    }

    if (attr->length > STRLENOF("urn:mspac:") &&
	strncasecmp(attr->data, "urn:mspac:", STRLENOF("urn:mspac:")) == 0)
    {
	char *p = &attr->data[STRLENOF("urn:mspac:")];
	char *endptr;

	*type = strtoul(p, &endptr, 10);
	if (*type != 0 && *endptr == '\0')
	    return 0;
    }

    return ENOENT;
}

static krb5_error_code
mspac_get_attribute_types(krb5_context kcontext,
			  krb5_authdata_context context,
			  void *plugin_context,
			  void *request_context,
			  krb5_data **out_attrs)
{
    struct mspac_context *pacctx = (struct mspac_context *)request_context;
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
				      &mspac_attribute_types[0].attribute,
				      &attrs[j++]);
    if (code != 0) {
	free(attrs);
	return code;
    }

    /* PAC buffers */
    for (i = 0; i < pacctx->pac->pac->cBuffers; i++) {
	krb5_data attr;

	code = mspac_type2attr(pacctx->pac->pac->Buffers[i].ulType, &attr);
	if (code == 0) {
	    code = krb5int_copy_data_contents(kcontext, &attr, &attrs[j++]);
	    if (code != 0) {
		krb5int_free_data_list(kcontext, attrs);
		return code;
	    }
	} else {
	    int length;

	    length = asprintf(&attrs[j].data, "urn:mspac:%d",
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

static krb5_error_code
mspac_get_attribute(krb5_context kcontext,
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
    struct mspac_context *pacctx = (struct mspac_context *)request_context;
    krb5_error_code code;
    krb5_ui_4 type;

    if (display_value != NULL) {
	display_value->data = NULL;
	display_value->length = 0;
    }

    if (*more != -1 || pacctx->pac == NULL)
	return ENOENT;

    code = mspac_attr2type(attribute, &type);
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
mspac_set_attribute(krb5_context kcontext,
		    krb5_authdata_context context,
		    void *plugin_context,
		    void *request_context,
		    krb5_boolean complete,
		    const krb5_data *attribute,
		    const krb5_data *value)
{
    struct mspac_context *pacctx = (struct mspac_context *)request_context;
    krb5_error_code code;
    krb5_ui_4 type;

    if (pacctx->pac == NULL)
	return ENOENT;

    code = mspac_attr2type(attribute, &type);
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
mspac_export_internal(krb5_context kcontext,
		      krb5_authdata_context context,
		      void *plugin_context,
		      void *request_context,
		      krb5_boolean restrict_authenticated,
		      void **ptr)
{
    struct mspac_context *pacctx = (struct mspac_context *)request_context;
    krb5_error_code code;
    krb5_pac pac;

    *ptr = NULL;

    if (pacctx->pac == NULL)
	return 0;

    if (restrict_authenticated && (pacctx->pac->verified) == FALSE)
	return 0;

    code = krb5_pac_parse(kcontext, pacctx->pac->data.data,
			  pacctx->pac->data.length, &pac);
    if (code == 0) {
	pac->verified = pacctx->pac->verified;
	*ptr = pac;
    }

    return code;
}

static void
mspac_free_internal(krb5_context kcontext,
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
mspac_size(krb5_context kcontext,
	   krb5_authdata_context context,
	   void *plugin_context,
	   void *request_context,
	   size_t *sizep)
{
    struct mspac_context *pacctx = (struct mspac_context *)request_context;

    *sizep += sizeof(krb5_int32);

    if (pacctx->pac != NULL)
	*sizep += pacctx->pac->data.length;

    *sizep += sizeof(krb5_int32);

    return 0;
}

static krb5_error_code
mspac_externalize(krb5_context kcontext,
		  krb5_authdata_context context,
		  void *plugin_context,
		  void *request_context,
		  krb5_octet **buffer,
		  size_t *lenremain)
{
    krb5_error_code code = 0;
    struct mspac_context *pacctx = (struct mspac_context *)request_context;
    size_t required = 0;
    krb5_octet *bp;
    size_t remain;

    bp = *buffer;
    remain = *lenremain;

    if (pacctx->pac != NULL) {
	mspac_size(kcontext, context, plugin_context,
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
mspac_internalize(krb5_context kcontext,
		  krb5_authdata_context context,
		  void *plugin_context,
		  void *request_context,
		  krb5_octet **buffer,
		  size_t *lenremain)
{
    struct mspac_context *pacctx = (struct mspac_context *)request_context;
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
mspac_copy(krb5_context kcontext,
	   krb5_authdata_context context,
	   void *plugin_context,
	   void *request_context,
	   void *dst_plugin_context,
	   void *dst_request_context)
{
    struct mspac_context *srcctx = (struct mspac_context *)request_context;
    struct mspac_context *dstctx = (struct mspac_context *)dst_request_context;
    krb5_error_code code = 0;

    assert(dstctx != NULL);
    assert(dstctx->pac == NULL);

    if (srcctx->pac != NULL)
	code = k5_pac_copy(kcontext, srcctx->pac, &dstctx->pac);

    return code;
}

static krb5_authdatatype mspac_ad_types[] = { KRB5_AUTHDATA_WIN2K_PAC, 0 };

krb5plugin_authdata_client_ftable_v0 krb5int_mspac_authdata_client_ftable = {
    "mspac",
    mspac_ad_types,
    mspac_init,
    mspac_fini,
    mspac_flags,
    mspac_request_init,
    mspac_request_fini,
    mspac_get_attribute_types,
    mspac_get_attribute,
    mspac_set_attribute,
    NULL, /* delete_attribute_proc */
    mspac_export_authdata,
    mspac_import_authdata,
    mspac_export_internal,
    mspac_free_internal,
    mspac_verify,
    mspac_size,
    mspac_externalize,
    mspac_internalize,
    mspac_copy
};

