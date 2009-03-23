/* This file is a copy of excerpt from generated lsa_h.h header.
   It has to be in sync with original idl file.
 */

#ifndef _SAMR_STUB_H_
#define _SAMR_STUB_H_

extern NTSTATUS SamrConnect(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *system_name,
    /* [in] */ uint32 access_mask,
    /* [out] */ PolicyHandle *connect_handle
#endif
);
extern NTSTATUS _SamrClose(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in, out] */ PolicyHandle *handle
#endif
);
extern NTSTATUS samr_Function02(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS samr_Function03(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS samr_Function04(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS _SamrLookupDomain(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ PolicyHandle *handle,
    /* [in] */ UnicodeString *domain_name,
    /* [out] */ PSID* sid
#endif
);
extern NTSTATUS _SamrEnumDomains(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ PolicyHandle *handle,
    /* [in, out] */ uint32 *resume,
    /* [in] */ uint32 size,
    /* [out] */ EntryArray **domains,
    /* [out] */ uint32 *num_entries
#endif
);
extern NTSTATUS _SamrOpenDomain(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ PolicyHandle *conn_handle,
    /* [in] */ uint32 access_mask,
    /* [in] */ PSID sid,
    /* [out] */ PolicyHandle *domain_handle
#endif
);
extern NTSTATUS _SamrQueryDomainInfo(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ PolicyHandle *domain_handle,
    /* [in] */ uint16 level,
    /* [out] */ DomainInfo **info
#endif
);
extern NTSTATUS samr_Function09(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS samr_Function0a(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS samr_Function0b(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS _SamrCreateUser(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ PolicyHandle *domain_handle,
    /* [in] */ UnicodeString *account_name,
    /* [in] */ uint32 access_mask,
    /* [out] */ PolicyHandle *user_handle,
    /* [out] */ uint32 *rid
#endif
);
extern NTSTATUS _SamrEnumDomainUsers(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ PolicyHandle *domain_handle,
    /* [in, out] */ uint32 *resume,
    /* [in] */ uint32 account_flags,
    /* [in] */ uint32 max_size,
    /* [out] */ RidNameArray **names,
    /* [out] */ uint32 *num_entries
#endif
);
extern NTSTATUS _SamrCreateDomAlias(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ PolicyHandle *domain_handle,
    /* [in] */ UnicodeString *alias_name,
    /* [in] */ uint32 access_mask,
    /* [out] */ PolicyHandle *alias_handle,
    /* [out] */ uint32 *rid
#endif
);
extern NTSTATUS _SamrEnumDomainAliases(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ PolicyHandle *domain_handle,
    /* [in, out] */ uint32 *resume,
    /* [in] */ uint32 account_flags,
    /* [out] */ RidNameArray **names,
    /* [out] */ uint32 *num_entries
#endif
);
extern NTSTATUS _SamrGetAliasMembership(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ PolicyHandle *domain_handle,
    /* [in] */ SidArray *sids,
    /* [out] */ Ids *rids
#endif
);
extern NTSTATUS _SamrLookupNames(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ PolicyHandle *domain_handle,
    /* [in] */ uint32 num_names,
    /* [in] */ UnicodeString *names,
    /* [out] */ Ids *ids,
    /* [out] */ Ids *types
#endif
);
extern NTSTATUS _SamrLookupRids(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ PolicyHandle *domain_handle,
    /* [in] */ uint32 num_rids,
    /* [in] */ uint32 rids[],
    /* [out] */ UnicodeStringArray *names,
    /* [out] */ Ids *types
#endif
);
extern NTSTATUS samr_Function13(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS samr_Function14(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS samr_Function15(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS samr_Function16(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS samr_Function17(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS samr_Function18(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS samr_Function19(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS samr_Function1a(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS _SamrOpenAlias(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ PolicyHandle *domain_handle,
    /* [in] */ uint32 access_mask,
    /* [in] */ uint32 rid,
    /* [out] */ PolicyHandle *alias
#endif
);
extern NTSTATUS _SamrQueryAliasInfo(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ PolicyHandle *alias_handle,
    /* [in] */ uint16 level,
    /* [out] */ AliasInfo **info
#endif
);
extern NTSTATUS _SamrSetAliasInfo(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ PolicyHandle *alias_handle,
    /* [in] */ uint16 level,
    /* [in] */ AliasInfo *info
#endif
);
extern NTSTATUS _SamrDeleteDomAlias(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in, out] */ PolicyHandle *alias_handle
#endif
);
extern NTSTATUS _SamrAddAliasMember(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ PolicyHandle *alias_handle,
    /* [in] */ PSID sid
#endif
);
extern NTSTATUS _SamrDeleteAliasMember(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ PolicyHandle *alias_handle,
    /* [in] */ PSID sid
#endif
);
extern NTSTATUS _SamrGetMembersInAlias(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ PolicyHandle *alias_handle,
    /* [out] */ SidArray *sids
#endif
);
extern NTSTATUS _SamrOpenUser(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ PolicyHandle *domain_handle,
    /* [in] */ uint32 access_mask,
    /* [in] */ uint32 rid,
    /* [out] */ PolicyHandle *user_handle
#endif
);
extern NTSTATUS _SamrDeleteUser(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in, out] */ PolicyHandle *user_handle
#endif
);
extern NTSTATUS _SamrQueryUserInfo(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ PolicyHandle *user_handle,
    /* [in] */ uint16 level,
    /* [out] */ UserInfo **info
#endif
);
extern NTSTATUS _SamrSetUserInfo(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ PolicyHandle *user_handle,
    /* [in] */ uint16 level,
    /* [in] */ UserInfo *info
#endif
);
extern NTSTATUS samr_Function26(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS _SamrGetUserGroups(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ PolicyHandle *user_handle,
    /* [out] */ RidWithAttributeArray **rids
#endif
);
extern NTSTATUS samr_Function28(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS samr_Function29(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS samr_Function2a(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS samr_Function2b(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS _SamrGetUserPwInfo(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ PolicyHandle *user_handle,
    /* [out] */ PwInfo *info
#endif
);
extern NTSTATUS samr_Function2d(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS samr_Function2e(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS samr_Function2f(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS samr_Function30(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS samr_Function31(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS _SamrCreateUser2(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ PolicyHandle *domain_handle,
    /* [in] */ UnicodeStringEx *account_name,
    /* [in] */ uint32 account_flags,
    /* [in] */ uint32 access_mask,
    /* [out] */ PolicyHandle *user_handle,
    /* [out] */ uint32 *access_granted,
    /* [out] */ uint32 *rid
#endif
);
extern NTSTATUS samr_Function33(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS samr_Function34(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS samr_Function35(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS samr_Function36(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS _SamrChangePasswordUser2(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ UnicodeString *server,
    /* [in] */ UnicodeString *account_name,
    /* [in] */ CryptPassword *nt_password,
    /* [in] */ HashPassword *nt_verifier,
    /* [in] */ uint8 lm_change,
    /* [in] */ CryptPassword *lm_password,
    /* [in] */ HashPassword *lm_verifier
#endif
);
extern NTSTATUS samr_Function38(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS _SamrConnect2(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ uint32 size,
    /* [in] */ wchar16_t *system_name,
    /* [in] */ uint32 access_mask,
    /* [out] */ PolicyHandle *connect_handle
#endif
);
extern NTSTATUS samr_Function3a(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS samr_Function3b(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS samr_Function3c(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS samr_Function3d(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NTSTATUS _SamrConnect4(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *system_name,
    /* [in] */ uint32 unknown,
    /* [in] */ uint32 access_mask,
    /* [out] */ PolicyHandle *connect_handle
#endif
);


#endif /* _SAMR_STUB_H_ */
