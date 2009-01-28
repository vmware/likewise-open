#ifndef _SDFLAGS_H_
#define _SDFLAGS_H_


/* Security ACE types */
#define SEC_ACE_TYPE_ACCESS_ALLOWED         0
#define SEC_ACE_TYPE_ACCESS_DENIED          1
#define SEC_ACE_TYPE_SYSTEM_AUDIT           2
#define SEC_ACE_TYPE_SYSTEM_ALARM           3
#define SEC_ACE_TYPE_ALLOWED_COMPOUND       4
#define SEC_ACE_TYPE_ACCESS_ALLOWED_OBJECT  5
#define SEC_ACE_TYPE_ACCESS_DENIED_OBJECT   6
#define SEC_ACE_TYPE_SYSTEM_AUDIT_OBJECT    7
#define SEC_ACE_TYPE_SYSTEM_ALARM_OBJECT    8

/* Security ACE object flags */
#define SEC_ACE_OBJECT_TYPE_PRESENT             0x00000001
#define SEC_ACE_INHERITED_OBJECT_TYPE_PRESENT   0x00000002

/* Standard access rights */
#define SEC_STD_DELETE                        0x00010000
#define SEC_STD_READ_CONTROL                  0x00020000
#define SEC_STD_WRITE_DAC                     0x00040000
#define SEC_STD_WRITE_OWNER                   0x00080000
#define SEC_STD_SYNCHRONIZE                   0x00100000
#define SEC_STD_REQUIRED                      0x000f0000
#define SEC_STD_ALL                           0x001f0000


#endif /* _SDFLAGS_H_ */
