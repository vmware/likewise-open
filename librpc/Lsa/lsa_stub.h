/* This file is a copy of excerpt from generated lsa_h.h header.
   It has to be in sync with original idl file.
 */

#ifndef _LSA_STUB_H_
#define _LSA_STUB_H_

extern NTSTATUS _LsaClose(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in, out] */ PolicyHandle *handle
#endif
);
extern NTSTATUS lsa_Function01(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS lsa_Function02(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS lsa_Function03(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS lsa_Function04(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS lsa_Function05(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS lsa_Function06(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS _LsaQueryInfoPolicy(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ PolicyHandle *handle,
    /* [in] */ uint16 level,
    /* [out] */ LsaPolicyInformation **info
#endif
);
extern NTSTATUS lsa_Function08(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS lsa_Function09(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS lsa_Function0a(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS lsa_Function0b(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS lsa_Function0c(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS lsa_Function0d(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS _LsaLookupNames(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ PolicyHandle *handle,
    /* [in] */ uint32 num_names,
    /* [in] */ UnicodeString *names,
    /* [out] */ RefDomainList **domains,
    /* [in, out] */ TranslatedSidArray *sids,
    /* [in] */ uint16 level,
    /* [in, out] */ uint32 *count
#endif
);
extern NTSTATUS _LsaLookupSids(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ PolicyHandle *handle,
    /* [in] */ SidArray *sids,
    /* [out] */ RefDomainList **domains,
    /* [in, out] */ TranslatedNameArray *names,
    /* [in] */ uint16 level,
    /* [in, out] */ uint32 *count
#endif
);
extern NTSTATUS lsa_Function10(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS lsa_Function11(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS lsa_Function12(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS lsa_Function13(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS lsa_Function14(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS lsa_Function15(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS lsa_Function16(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS lsa_Function17(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS lsa_Function18(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS lsa_Function19(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS lsa_Function1a(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS lsa_Function1b(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS lsa_Function1c(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS lsa_Function1d(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS lsa_Function1e(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS lsa_Function1f(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS lsa_Function20(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS lsa_Function21(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS lsa_Function22(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS lsa_Function23(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS lsa_Function24(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS lsa_Function25(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS lsa_Function26(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS lsa_Function27(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS lsa_Function28(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS lsa_Function29(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS lsa_Function2a(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS lsa_Function2b(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS _LsaOpenPolicy2(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *system_name,
    /* [in] */ ObjectAttribute *attrib,
    /* [in] */ uint32 access_mask,
    /* [out] */ PolicyHandle *handle
#endif
);
extern NTSTATUS lsa_Function2d(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS _LsaQueryInfoPolicy2(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ PolicyHandle *handle,
    /* [in] */ uint16 level,
    /* [out] */ LsaPolicyInformation **info
#endif
);
extern NTSTATUS lsa_Function2f(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS lsa_Function30(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS lsa_Function31(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS lsa_Function32(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS lsa_Function33(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS lsa_Function34(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS lsa_Function35(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS lsa_Function36(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS lsa_Function37(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS lsa_Function38(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS lsa_Function39(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS _LsaLookupNames2(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ PolicyHandle *handle,
    /* [in] */ uint32 num_names,
    /* [in] */ UnicodeStringEx *names,
    /* [out] */ RefDomainList **domains,
    /* [in, out] */ TranslatedSidArray2 *sids,
    /* [in] */ uint16 level,
    /* [in, out] */ uint32 *count,
    /* [in] */ uint32 unknown1,
    /* [in] */ uint32 unknown2
#endif
);


#endif /* _LSA_STUB_H_ */
