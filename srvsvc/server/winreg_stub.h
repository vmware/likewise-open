/* This file is a copy of excerpt from generated winreg_h.h header.
   It has to be in sync with original idl file.
 */

#ifndef _WINREG_STUB_H
#define _WINREG_STUB_H

typedef idl_void_p_t POLICY_HANDLE;

#ifndef __WINREG_IDL__

#define __WINREG_IDL__

#ifdef _WIN32

#ifndef TARGET_IS_NT50_OR_LATER

#define TARGET_IS_NT50_OR_LATER TRUE

#else

#if !(TARGET_IS_NT50_OR_LATER)

#undef TARGET_IS_NT50_OR_LATER

#define TARGET_IS_NT50_OR_LATER TRUE

#endif

#endif

#endif

#endif /* DCERPC_STUB_BUILD */
extern void _winreg_Function0x0(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _winreg_Function0x1(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern WINERR _RegOpenHKLM(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *system_name,
    /* [in] */ uint32 access_mask,
    /* [out] */ POLICY_HANDLE *handle
#endif
);
extern void _winreg_Function0x3(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _winreg_Function0x4(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern WINERR _RegCloseKey(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in, out] */ POLICY_HANDLE *handle
#endif
);
extern void _winreg_Function0x6(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _winreg_Function0x7(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _winreg_Function0x8(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _winreg_Function0x9(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _winreg_Function0xa(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _winreg_Function0xb(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _winreg_Function0xc(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _winreg_Function0xd(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _winreg_Function0xe(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern WINERR _RegOpenKey(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ POLICY_HANDLE *parent_handle,
    /* [in] */ wchar16_t *key_name,
    /* [in] */ uint32 unknown,
    /* [in] */ uint32 access_mask,
    /* [out] */ POLICY_HANDLE *handle
#endif
);
extern void _winreg_Function0x10(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern WINERR _RegQueryValue(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ POLICY_HANDLE *handle,
    /* [in] */ wchar16_t *value_name,
    /* [in, out] */ uint32 *type,
    /* [in, out] */ uint8 *buffer,
    /* [in, out] */ uint32 *buffer_size,
    /* [in, out] */ uint32 *buffer_len
#endif
);
extern void _winreg_Function0x12(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _winreg_Function0x13(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _winreg_Function0x14(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _winreg_Function0x15(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _winreg_Function0x16(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _winreg_Function0x17(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _winreg_Function0x18(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _winreg_Function0x19(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern WINERR _RegGetVersion(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ POLICY_HANDLE *handle,
    /* [out] */ uint32 *version
#endif
);
extern void _winreg_Function0x1b(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _winreg_Function0x1c(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _winreg_Function0x1d(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _winreg_Function0x1e(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _winreg_Function0x1f(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _winreg_Function0x20(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _winreg_Function0x21(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _winreg_Function0x22(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);

#endif /* __WINREG_IDL__ */
void POLICY_HANDLE_rundown(
#ifdef IDL_PROTOTYPES
    rpc_ss_context_t context_handle
#endif
);
typedef struct winreg_v1_0_epv_t {
	void (*_winreg_Function0x0)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	void (*_winreg_Function0x1)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	WINERR (*_RegOpenHKLM)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *system_name,
    /* [in] */ uint32 access_mask,
    /* [out] */ POLICY_HANDLE *handle
#endif
);
	void (*_winreg_Function0x3)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	void (*_winreg_Function0x4)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	WINERR (*_RegCloseKey)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in, out] */ POLICY_HANDLE *handle
#endif
);
	void (*_winreg_Function0x6)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	void (*_winreg_Function0x7)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	void (*_winreg_Function0x8)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	void (*_winreg_Function0x9)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	void (*_winreg_Function0xa)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	void (*_winreg_Function0xb)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	void (*_winreg_Function0xc)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	void (*_winreg_Function0xd)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	void (*_winreg_Function0xe)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	WINERR (*_RegOpenKey)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ POLICY_HANDLE *parent_handle,
    /* [in] */ wchar16_t *key_name,
    /* [in] */ uint32 unknown,
    /* [in] */ uint32 access_mask,
    /* [out] */ POLICY_HANDLE *handle
#endif
);
	void (*_winreg_Function0x10)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	WINERR (*_RegQueryValue)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ POLICY_HANDLE *handle,
    /* [in] */ wchar16_t *value_name,
    /* [in, out] */ uint32 *type,
    /* [in, out] */ uint8 *buffer,
    /* [in, out] */ uint32 *buffer_size,
    /* [in, out] */ uint32 *buffer_len
#endif
);
	void (*_winreg_Function0x12)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	void (*_winreg_Function0x13)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	void (*_winreg_Function0x14)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	void (*_winreg_Function0x15)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	void (*_winreg_Function0x16)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	void (*_winreg_Function0x17)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	void (*_winreg_Function0x18)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	void (*_winreg_Function0x19)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	WINERR (*_RegGetVersion)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ POLICY_HANDLE *handle,
    /* [out] */ uint32 *version
#endif
);
	void (*_winreg_Function0x1b)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	void (*_winreg_Function0x1c)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	void (*_winreg_Function0x1d)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	void (*_winreg_Function0x1e)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	void (*_winreg_Function0x1f)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	void (*_winreg_Function0x20)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	void (*_winreg_Function0x21)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	void (*_winreg_Function0x22)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
} winreg_v1_0_epv_t;
extern rpc_if_handle_t winreg_v1_0_c_ifspec;
extern rpc_if_handle_t winreg_v1_0_s_ifspec;

#endif   /* _WINREG_STUB_H_ */
