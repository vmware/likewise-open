/* This file is a copy of excerpt from generated netlogon_h.h header.
   It has to be in sync with original idl file.
 */

#ifndef _NETLOGON_STUB_H_
#define _NETLOGON_STUB_H_

extern NTSTATUS netr_Function00(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS netr_Function01(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS _NetrLogonSamLogon(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ wchar16_t *computer_name,
    /* [in] */ NetrAuth *creds,
    /* [in, out] */ NetrAuth *ret_creds,
    /* [in] */ uint16 logon_level,
    /* [in] */ NetrLogonInfo *logon,
    /* [in] */ uint16 validation_level,
    /* [out] */ NetrValidationInfo *validation,
    /* [out] */ uint8 *authoritative
#endif
);
extern NTSTATUS _NetrLogonSamLogoff(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ wchar16_t *computer_name,
    /* [in] */ NetrAuth *creds,
    /* [in, out] */ NetrAuth *ret_creds,
    /* [in] */ uint16 logon_level,
    /* [in] */ NetrLogonInfo *logon
#endif
);
extern NTSTATUS _NetrServerReqChallenge(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ wchar16_t *computer_name,
    /* [in, out] */ NetrCred *credentials
#endif
);
extern NTSTATUS _NetrServerAuthenticate(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ wchar16_t account_name[],
    /* [in] */ uint16 secure_channel_type,
    /* [in] */ wchar16_t computer_name[],
    /* [in, out] */ NetrCred *credentials
#endif
);
extern NTSTATUS netr_Function06(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS netr_Function07(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS netr_Function08(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS netr_Function09(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS netr_Function0a(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS netr_Function0b(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS netr_Function0c(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS netr_Function0d(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS netr_Function0e(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS _NetrServerAuthenticate2(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ wchar16_t account_name[],
    /* [in] */ uint16 secure_channel_type,
    /* [in] */ wchar16_t computer_name[],
    /* [in, out] */ NetrCred *credentials,
    /* [in, out] */ uint32 *negotiate_flags
#endif
);
extern NTSTATUS netr_Function10(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS netr_Function11(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS netr_Function12(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS netr_Function13(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS netr_Function14(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS netr_Function15(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS netr_Function16(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS netr_Function17(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS netr_Function18(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS netr_Function19(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS _NetrServerAuthenticate3(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ wchar16_t account_name[],
    /* [in] */ uint16 secure_channel_type,
    /* [in] */ wchar16_t computer_name[],
    /* [in, out] */ NetrCred *credentials,
    /* [in, out] */ uint32 *negotiate_flags,
    /* [out] */ uint32 *rid
#endif
);
extern NTSTATUS netr_Function1b(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS netr_Function1c(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS netr_Function1d(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS netr_Function1e(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS netr_Function1f(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS netr_Function20(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS netr_Function21(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS netr_Function22(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS netr_Function23(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS _NetrEnumerateTrustedDomainsEx(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [out] */ NetrDomainTrustList *domain_trusts
#endif
);
extern NTSTATUS netr_Function25(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS netr_Function26(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS _NetrLogonSamLogonEx(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ wchar16_t *computer_name,
    /* [in] */ uint16 logon_level,
    /* [in] */ NetrLogonInfo *logon,
    /* [in] */ uint16 validation_level,
    /* [out] */ NetrValidationInfo *validation,
    /* [out] */ uint8 *authoritative,
    /* [in, out] */ uint32 *flags
#endif
);
extern WINERR _DsrEnumerateDomainTrusts(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ uint32 trust_flags,
    /* [out] */ NetrDomainTrustList *trusts
#endif
);
extern NTSTATUS netr_Function29(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS netr_Function2a(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS netr_Function2b(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS netr_Function2c(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS netr_Function2d(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS netr_Function2e(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS netr_Function2f(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS netr_Function30(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS netr_Function31(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS netr_Function32(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS netr_Function33(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS netr_Function34(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS netr_Function35(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS netr_Function36(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS netr_Function37(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS netr_Function38(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS netr_Function39(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS netr_Function3a(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS netr_Function3b(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);


#endif /* _NETLOGON_STUB_H_ */
