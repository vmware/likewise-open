/*
 * lwldap-error.h
 *
 *  Created on: Nov 6, 2009
 *      Author: wfu
 */

#ifndef LWLDAPERROR_H_
#define LWLDAPERROR_H_

DWORD
LwLdapErrToWin32Error(
    int lderr
    );

int
LwErrnoToLdapErr(
    int uerror
    );

#ifndef LW_STRICT_NAMESPACE
#define ErrnoToLdapErr(uerror)               LwErrnoToLdapErr(uerror)
#define LdapErrToWin32Error(lderr)           LwLdapErrToWin32Error(lderr)
#endif /* LW_STRICT_NAMESPACE */

#endif /* LWLDAPERROR_H_ */
