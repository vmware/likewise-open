/* This file is a copy of excerpt from generated wkssvc_h.h header.
   It has to be in sync with original idl file.
 */

#ifndef _WKSSVC_STUB_H
#define _WKSSVC_STUB_H

typedef struct _WKSTA_INFO_100 {
uint32 wksta100_platform_id;
wchar16_t *wksta100_name;
wchar16_t *wksta100_domain;
uint32 wksta100_version_major;
uint32 wksta100_version_minor;
} WKSTA_INFO_100;
typedef WKSTA_INFO_100 *PWKSTA_INFO_100;

typedef union  {
/* case(s): 100 */
WKSTA_INFO_100 *info100;
/* case(s): default */
/* Empty arm */
} wkssvc_NetWkstaInfo;

extern NET_API_STATUS _NetWkstaGetInfo(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ uint32 level,
    /* [out] */ wkssvc_NetWkstaInfo *info
#endif
);
extern void _wkssvc_Function0x1(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _wkssvc_Function0x2(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _wkssvc_Function0x3(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _wkssvc_Function0x4(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _wkssvc_Function0x5(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _wkssvc_Function0x6(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _wkssvc_Function0x7(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _wkssvc_Function0x8(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _wkssvc_Function0x9(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _wkssvc_Function0xa(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _wkssvc_Function0xb(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _wkssvc_Function0xc(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _wkssvc_Function0xd(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _wkssvc_Function0xe(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _wkssvc_Function0xf(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _wkssvc_Function0x10(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _wkssvc_Function0x11(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _wkssvc_Function0x12(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _wkssvc_Function0x13(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _wkssvc_Function0x14(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _wkssvc_Function0x15(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _wkssvc_Function0x16(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _wkssvc_Function0x17(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _wkssvc_Function0x18(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _wkssvc_Function0x19(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _wkssvc_Function0x1a(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _wkssvc_Function0x1b(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _wkssvc_Function0x1c(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _wkssvc_Function0x1d(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _wkssvc_Function0x1e(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
typedef struct wkssvc_v1_0_epv_t {
	NET_API_STATUS (*_NetWkstaGetInfo)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ uint32 level,
    /* [out] */ wkssvc_NetWkstaInfo *info
#endif
);
	void (*_wkssvc_Function0x0)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	void (*_wkssvc_Function0x1)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	void (*_wkssvc_Function0x2)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	void (*_wkssvc_Function0x3)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	void (*_wkssvc_Function0x4)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	void (*_wkssvc_Function0x5)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	void (*_wkssvc_Function0x6)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	void (*_wkssvc_Function0x7)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	void (*_wkssvc_Function0x8)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	void (*_wkssvc_Function0x9)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	void (*_wkssvc_Function0xa)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	void (*_wkssvc_Function0xb)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	void (*_wkssvc_Function0xc)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	void (*_wkssvc_Function0xd)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	void (*_wkssvc_Function0xe)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	void (*_wkssvc_Function0xf)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	void (*_wkssvc_Function0x10)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	void (*_wkssvc_Function0x11)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	void (*_wkssvc_Function0x12)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	void (*_wkssvc_Function0x13)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	void (*_wkssvc_Function0x14)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	void (*_wkssvc_Function0x15)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	void (*_wkssvc_Function0x16)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	void (*_wkssvc_Function0x17)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	void (*_wkssvc_Function0x18)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	void (*_wkssvc_Function0x19)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	void (*_wkssvc_Function0x1a)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	void (*_wkssvc_Function0x1b)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	void (*_wkssvc_Function0x1c)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	void (*_wkssvc_Function0x1d)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
	void (*_wkssvc_Function0x1e)(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
} wkssvc_v1_0_epv_t;
extern rpc_if_handle_t wkssvc_v1_0_c_ifspec;
extern rpc_if_handle_t wkssvc_v1_0_s_ifspec;

#endif   /* _WKSSVC_STUB_H_ */
