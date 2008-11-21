/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        basic_types.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Marshall/Unmarshall API for Basic data types
 *
 * Authors: Gerald Carter <gcarter@likewisesoftware.com>
 *
 */

#include "ipc.h"
#include "lsadatablob.h"

/*********************************************************************
 */

DWORD
IpcParseDword(
	LsaMarshallType Action,
	PBYTE pBuffer,
	DWORD dwBufLen,
	PDWORD pdwValue,
	PDWORD pdwNeeded
	)
{
	DWORD dwError = LSA_ERROR_INTERNAL;
	DWORD dwNeeded = 0;

	BAIL_ON_INVALID_POINTER(pdwValue);
	
	/* Calc space */

	dwNeeded = sizeof(DWORD);	

	if (dwNeeded > dwBufLen) {
		dwError = (Action == LSA_MARSHALL_DATA) ? 
			LSA_ERROR_INSUFFICIENT_BUFFER : 
			LSA_ERROR_UNEXPECTED_MESSAGE;		
		BAIL_ON_LSA_ERROR(dwError);
	}
	
	switch (Action)
	{
	case LSA_MARSHALL_DATA:
		SETUINT32(pBuffer, 0, *pdwValue);
		break;
	
	case LSA_UNMARSHALL_DATA:
		*pdwValue = GETUINT32(pBuffer, 0);
		break;
	}
	
	dwError = LSA_ERROR_SUCCESS;	

cleanup:
	/* Always set this and letthe caller decide */

	*pdwNeeded = dwNeeded;

	return dwError;
error:
	goto cleanup;
}

/*********************************************************************
 */

DWORD
IpcParseBytes(
	LsaMarshallType Action,
	PBYTE pBuffer,
	DWORD dwBufLen,
	PBYTE *ppData,
	DWORD dwDataLen,
	PDWORD pdwNeeded
	)
{
	DWORD dwError = LSA_ERROR_INTERNAL;
	DWORD dwNeeded = 0;
	DWORD i;

	BAIL_ON_INVALID_POINTER(ppData);
	
	/* Check for remaining space */

	dwNeeded = dwDataLen;

	if (dwNeeded > dwBufLen) {
		dwError = (Action == LSA_MARSHALL_DATA) ? 
			LSA_ERROR_INSUFFICIENT_BUFFER : 
			LSA_ERROR_UNEXPECTED_MESSAGE;		
		BAIL_ON_LSA_ERROR(dwError);
	}

	switch (Action)
	{
	case LSA_MARSHALL_DATA:
	{
		for (i=0; i<dwDataLen; i++) {
			SETUINT8(pBuffer, i, (*ppData)[i]);
		}		
		break;
	}
	
	case LSA_UNMARSHALL_DATA:
	{
		dwError = LsaAllocateMemory(dwDataLen, (PVOID*)ppData);
		BAIL_ON_LSA_ERROR(dwError);		
		
		for (i=0; i<dwDataLen; i++) {
			(*ppData)[i] = GETUINT8(pBuffer, i);
		}		
		break;
	}
	}
	
	dwError = LSA_ERROR_SUCCESS;

cleanup:
	/* Always set this and letthe caller decide */

	*pdwNeeded = dwNeeded;

	return dwError;
error:
	goto cleanup;	
}


/*********************************************************************
 */

DWORD
IpcParseString(
	LsaMarshallType Action,
	PBYTE pBuffer,
	DWORD dwBufLen,
	PCSTR *ppszString,
	PDWORD pdwNeeded
	)
{
	DWORD dwError = LSA_ERROR_INTERNAL;
	DWORD dwNeeded = 0;
	DWORD dwOffset = 0;
	DWORD dwString = 0;
	DWORD dwStringLen = 0;	

	BAIL_ON_INVALID_POINTER(ppszString);	

	/* String pointer */

	if (Action == LSA_MARSHALL_DATA) {
		dwString = *ppszString ? 1 : 0;		
	}	

	dwError = IpcParseDword(Action,
				SAFE_BUFFER_OFFSET(pBuffer, dwOffset),
				SAFE_BUFFER_LENGTH(pBuffer, dwBufLen, dwOffset),
				&dwString,
				&dwNeeded);
	BAIL_ON_LSA_PARSE_ERROR(dwError);
	
	dwOffset += dwNeeded;

	/* See if we still have a string to parse */

	if (dwString == 0) {	
		goto cleanup;
	}	

	/* Length */

	if (Action == LSA_MARSHALL_DATA) {
		dwStringLen = strlen(*ppszString) + 1;
	}	

	dwError = IpcParseDword(Action,
				SAFE_BUFFER_OFFSET(pBuffer, dwOffset),
				SAFE_BUFFER_LENGTH(pBuffer, dwBufLen, dwOffset),
				&dwStringLen,
				&dwNeeded);
	BAIL_ON_LSA_PARSE_ERROR(dwError);

	dwOffset += dwNeeded;

	/* Character array */

	dwError = IpcParseBytes(Action,
				SAFE_BUFFER_OFFSET(pBuffer, dwOffset),
				SAFE_BUFFER_LENGTH(pBuffer, dwBufLen, dwOffset),
				(PBYTE*)ppszString,
				dwStringLen,
				&dwNeeded);
	BAIL_ON_LSA_PARSE_ERROR(dwError);
	
	dwOffset += dwNeeded;

cleanup:
	/* Always set this and letthe caller decide */

	*pdwNeeded = dwOffset;	

	return dwError;
error:
	goto cleanup;	
}

/*********************************************************************
 */

DWORD
IpcParseDataBlob(
	LsaMarshallType Action,
	PBYTE pBuffer,
	DWORD dwBufLen,
	PLSA_DATA_BLOB *ppBlob,
	PDWORD pdwNeeded
	)
{
	DWORD dwError = LSA_ERROR_INTERNAL;
	DWORD dwNeeded = 0;
	DWORD dwOffset = 0;
	DWORD dwBlobLen = 0;
	PBYTE pData = NULL;	

	BAIL_ON_INVALID_POINTER(ppBlob);

	/* String pointer */

	if (Action == LSA_MARSHALL_DATA) {
		dwBlobLen = LsaDataBlobLength(*ppBlob);		
	}	

	dwError = IpcParseDword(Action,
				SAFE_BUFFER_OFFSET(pBuffer, dwOffset),
				SAFE_BUFFER_LENGTH(pBuffer, dwBufLen, dwOffset),
				&dwBlobLen,
				&dwNeeded);
	BAIL_ON_LSA_PARSE_ERROR(dwError);
	
	dwOffset += dwNeeded;

	/* See if we still have a string to parse */

	if (dwBlobLen > 0) {
		/* Byte array */

		if (Action == LSA_MARSHALL_DATA) {
			pData = LsaDataBlobBuffer(*ppBlob);
		}	

		dwError = IpcParseBytes(Action,
					SAFE_BUFFER_OFFSET(pBuffer, dwOffset),
					SAFE_BUFFER_LENGTH(pBuffer, dwBufLen, dwOffset),
					&pData,
					dwBlobLen,
					&dwNeeded);
		BAIL_ON_LSA_PARSE_ERROR(dwError);
	
		dwOffset += dwNeeded;
	}	

	if (Action == LSA_UNMARSHALL_DATA) {		
		dwError = LsaDataBlobStore(ppBlob, dwBlobLen, pData);
		BAIL_ON_LSA_ERROR(dwError);
	}

cleanup:
	/* Always set this and letthe caller decide */

	*pdwNeeded = dwOffset;	

	return dwError;
error:
	goto cleanup;	
}
