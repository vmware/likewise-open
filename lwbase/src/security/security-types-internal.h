#ifndef __LW_SECURITY_TYPES_INTERNAL_H__
#define __LW_SECURITY_TYPES_INTERNAL_H__

#include <lw/security-types.h>

//
// ACL - Access Control List
//
// See the documentation with the PACL type for a description.
//

typedef struct _ACL {
    UCHAR AclRevision;
    UCHAR Sbz1; // Padding (should be 0)
    USHORT AclSize;
    USHORT AceCount;
    USHORT Sbz2; // Padding (should be 0)
} ACL;

typedef char _LW_C_ASSERT_CHECK_ACL_SIZE[(sizeof(ACL) == ACL_HEADER_SIZE)?1:-1];

//
// SD - Security Descriptor
//
// See the documentation with the PSECURITY_DESCRIPTOR_ABSOLUTE and
// PSECURITY_DESCRIPTOR_RELATIVE types for a description.
//

typedef struct _SECURITY_DESCRIPTOR_ABSOLUTE {
   UCHAR Revision;
   UCHAR Sbz1; // Padding (should be 0 unless SE_RM_CONTROL_VALID)
   SECURITY_DESCRIPTOR_CONTROL Control;
   PSID Owner;
   PSID Group; /// Can be NULL.
   PACL Sacl;
   PACL Dacl;
} SECURITY_DESCRIPTOR_ABSOLUTE;

typedef char _LW_C_ASSERT_CHECK_SECURITY_DESCRIPTOR_ABSOLUTE_SIZE[(sizeof(SECURITY_DESCRIPTOR_ABSOLUTE) == SECURITY_DESCRIPTOR_ABSOLUTE_MIN_SIZE)?1:-1];

typedef struct _SECURITY_DESCRIPTOR_RELATIVE {
    UCHAR Revision;
    UCHAR Sbz1; // Padding (should be 0 unless SE_RM_CONTROL_VALID)
    SECURITY_DESCRIPTOR_CONTROL Control;
    ULONG Owner; // offset to Owner SID
    ULONG Group; // offset to Group SID
    ULONG Sacl; // offset to system ACL
    ULONG Dacl; // offset to discretional ACL
    // Owner, Group, Sacl, and Dacl data follows
} SECURITY_DESCRIPTOR_RELATIVE;

typedef char _LW_C_ASSERT_CHECK_SECURITY_DESCRIPTOR_RELATIVE_SIZE[(sizeof(SECURITY_DESCRIPTOR_RELATIVE) == SECURITY_DESCRIPTOR_RELATIVE_MIN_SIZE)?1:-1];

//
// Access Token
//
// This is an opaque type.
//

typedef ULONG ACCESS_TOKEN_FLAGS, *PACCESS_TOKEN_FLAGS;

#define ACCESS_TOKEN_FLAG_UNIX_PRESENT 0x00000001

typedef struct _ACCESS_TOKEN {
    LONG ReferenceCount;
    ACCESS_TOKEN_FLAGS Flags;
    // TOKEN_USER:
    SID_AND_ATTRIBUTES User;
    // TOKEN_GROUPS:
    ULONG GroupCount;
    PSID_AND_ATTRIBUTES Groups;
#if 0
    TOKEN_PRIVILEGES Privileges;
#endif
    // TOKEN_OWNER:
    PSID Owner;
    // TOKEN_PRIMARY_GROUP:
    PSID PrimaryGroup;
    // TOKEN_DEFAULT_DACL:
    PACL DefaultDacl;
#if 0
    TOKEN_SOURCE Source;
#endif
    // TOKEN_UNIX:
    ULONG Uid;
    ULONG Gid;
    ULONG Umask;
} ACCESS_TOKEN;

#endif /* __LW_SECURITY_TYPES_INTERNAL_H__ */
