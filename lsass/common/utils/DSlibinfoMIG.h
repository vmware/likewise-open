#ifndef	_DSlibinfoMIG_user_
#define	_DSlibinfoMIG_user_

/* Module DSlibinfoMIG */

#include <string.h>
#include <mach/ndr.h>
#include <mach/boolean.h>
#include <mach/kern_return.h>
#include <mach/notify.h>
#include <mach/mach_types.h>
#include <mach/message.h>
#include <mach/mig_errors.h>
#include <mach/port.h>

#ifdef AUTOTEST
#ifndef FUNCTION_PTR_T
#define FUNCTION_PTR_T
typedef void (*function_ptr_t)(mach_port_t, char *, mach_msg_type_number_t);
typedef struct {
        char            *name;
        function_ptr_t  function;
} function_table_entry;
typedef function_table_entry   *function_table_t;
#endif /* FUNCTION_PTR_T */
#endif /* AUTOTEST */

#ifndef	DSlibinfoMIG_MSG_COUNT
#define	DSlibinfoMIG_MSG_COUNT	3
#endif	/* DSlibinfoMIG_MSG_COUNT */

#include <sys/types.h>
#include <mach/std_types.h>
#include <mach/mig.h>
#include <mach/mig.h>
#include <mach/mach_types.h>
#include <DSlibinfoMIG_types.h>

#ifdef __BeforeMigUserHeader
__BeforeMigUserHeader
#endif /* __BeforeMigUserHeader */

#include <sys/cdefs.h>
__BEGIN_DECLS


/* Routine GetProcedureNumber */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t libinfoDSmig_GetProcedureNumber
(
	mach_port_t server,
	proc_name_t name,
	int32_t *procno,
	security_token_t *usertoken
);

/* Routine Query */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t libinfoDSmig_Query
(
	mach_port_t server,
	int32_t proc,
	inline_data_t request,
	mach_msg_type_number_t requestCnt,
	inline_data_t reply,
	mach_msg_type_number_t *replyCnt,
	vm_offset_t *ooreply,
	mach_msg_type_number_t *ooreplyCnt,
	security_token_t *usertoken
);

/* SimpleRoutine Query_async */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t libinfoDSmig_Query_async
(
	mach_port_t server,
	mach_port_t replyToPort,
	int32_t proc,
	inline_data_t request,
	mach_msg_type_number_t requestCnt,
	mach_vm_address_t callbackAddr
);

__END_DECLS

/********************** Caution **************************/
/* The following data types should be used to calculate  */
/* maximum message sizes only. The actual message may be */
/* smaller, and the position of the arguments within the */
/* message layout may vary from what is presented here.  */
/* For example, if any of the arguments are variable-    */
/* sized, and less than the maximum is sent, the data    */
/* will be packed tight in the actual message to reduce  */
/* the presence of holes.                                */
/********************** Caution **************************/

/* typedefs for all requests */

#ifndef __Request__DSlibinfoMIG_subsystem__defined
#define __Request__DSlibinfoMIG_subsystem__defined

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		mach_msg_type_number_t nameOffset; /* MiG doesn't use it */
		mach_msg_type_number_t nameCnt;
		char name[256];
	} __Request__GetProcedureNumber_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		int32_t proc;
		mach_msg_type_number_t requestCnt;
		char request[16384];
	} __Request__Query_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		/* start of the kernel processed data */
		mach_msg_body_t msgh_body;
		mach_msg_port_descriptor_t replyToPort;
		/* end of the kernel processed data */
		NDR_record_t NDR;
		int32_t proc;
		mach_msg_type_number_t requestCnt;
		char request[16384];
		mach_vm_address_t callbackAddr;
	} __Request__Query_async_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif
#endif /* !__Request__DSlibinfoMIG_subsystem__defined */

/* union of all requests */

#ifndef __RequestUnion__libinfoDSmig_DSlibinfoMIG_subsystem__defined
#define __RequestUnion__libinfoDSmig_DSlibinfoMIG_subsystem__defined
union __RequestUnion__libinfoDSmig_DSlibinfoMIG_subsystem {
	__Request__GetProcedureNumber_t Request_libinfoDSmig_GetProcedureNumber;
	__Request__Query_t Request_libinfoDSmig_Query;
	__Request__Query_async_t Request_libinfoDSmig_Query_async;
};
#endif /* !__RequestUnion__libinfoDSmig_DSlibinfoMIG_subsystem__defined */
/* typedefs for all replies */

#ifndef __Reply__DSlibinfoMIG_subsystem__defined
#define __Reply__DSlibinfoMIG_subsystem__defined

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		kern_return_t RetCode;
		int32_t procno;
	} __Reply__GetProcedureNumber_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		/* start of the kernel processed data */
		mach_msg_body_t msgh_body;
		mach_msg_ool_descriptor_t ooreply;
		/* end of the kernel processed data */
		NDR_record_t NDR;
		mach_msg_type_number_t replyCnt;
		char reply[16384];
		mach_msg_type_number_t ooreplyCnt;
	} __Reply__Query_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		kern_return_t RetCode;
	} __Reply__Query_async_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif
#endif /* !__Reply__DSlibinfoMIG_subsystem__defined */

/* union of all replies */

#ifndef __ReplyUnion__libinfoDSmig_DSlibinfoMIG_subsystem__defined
#define __ReplyUnion__libinfoDSmig_DSlibinfoMIG_subsystem__defined
union __ReplyUnion__libinfoDSmig_DSlibinfoMIG_subsystem {
	__Reply__GetProcedureNumber_t Reply_libinfoDSmig_GetProcedureNumber;
	__Reply__Query_t Reply_libinfoDSmig_Query;
	__Reply__Query_async_t Reply_libinfoDSmig_Query_async;
};
#endif /* !__RequestUnion__libinfoDSmig_DSlibinfoMIG_subsystem__defined */

#ifndef subsystem_to_name_map_DSlibinfoMIG
#define subsystem_to_name_map_DSlibinfoMIG \
    { "GetProcedureNumber", 50000 },\
    { "Query", 50001 },\
    { "Query_async", 50002 }
#endif

#ifdef __AfterMigUserHeader
__AfterMigUserHeader
#endif /* __AfterMigUserHeader */

#endif	 /* _DSlibinfoMIG_user_ */
