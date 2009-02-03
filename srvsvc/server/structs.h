#ifndef __STRUCTS_H__
#define __STRUCTS_H__

typedef struct _NET_SHARE_INFO0
{
    PSTR pszName;
    PSTR pszPath;
    PSTR pszComment;
    PSTR pszSID;
} NET_SHARE_INFO0, *PNET_SHARE_INFO0;

typedef struct _NET_SHARE_INFO1
{
    PSTR pszName;
    PSTR pszPath;
    PSTR pszComment;
    PSTR pszSID;
} NET_SHARE_INFO1, *PNET_SHARE_INFO1;

typedef struct _NET_SHARE_INFO2
{
    PSTR pszName;
    PSTR pszPath;
    PSTR pszComment;
    PSTR pszSID;
} NET_SHARE_INFO2, *PNET_SHARE_INFO2;

typedef struct _NET_SHARE_INFO501
{
    PSTR pszName;
    PSTR pszPath;
    PSTR pszComment;
    PSTR pszSID;
} NET_SHARE_INFO501, *PNET_SHARE_INFO501;

typedef struct _NET_SHARE_INFO502
{
    PSTR pszName;
    PSTR pszPath;
    PSTR pszComment;
    PSTR pszSID;
} NET_SHARE_INFO502, *PNET_SHARE_INFO502;

typedef struct _NET_SHARE_INFO503
{
    PSTR pszName;
    PSTR pszPath;
    PSTR pszComment;
    PSTR pszSID;
} NET_SHARE_INFO503, *PNET_SHARE_INFO503;

#endif /* __STRUCTS_H__ */
