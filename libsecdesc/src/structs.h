

typedef struct _ACE_HEADER {
    UCHAR AceType;
    UCHAR AceFlags;
    USHORT AceSize;
}ACE_HEADER, *PACE_HEADER;

typedef _ACLtruct _ACCESS_ALLOWED_ACE {
  ACE_HEADER  Header;
  ACCESS_MASK  Mask;
  ULONG  SidStart;
} ACCESS_ALLOWED_ACE;
typedef ACCESS_ALLOWED_ACE *PACCESS_ALLOWED_ACE;





{
  UCHAR  AclRevision;
  UCHAR  Sbz1;
  USHORT  AclSize;
  USHORT  AceCount;
  USHORT  Sbz2;
} ACL;

typedef ACL *PACL;


typedef struct _SECURITY_DESCRIPTOR
{
  UCHAR  Revision;
  UCHAR  Sbz1;
  SECURITY_DESCRIPTOR_CONTROL  Control;
  PSID  Owner;
  PSID  Group;
  PACL  Sacl;
  PACL  Dacl;
} SECURITY_DESCRIPTOR, *PISECURITY_DESCRIPTOR;
    

} ACCESS_DENIED_ACE, *PACCESS_DENIED_ACE;


