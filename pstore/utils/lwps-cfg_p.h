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
 *        lwps-cfg_p.h
 *
 * Abstract:
 *
 *        Likewise Password Storage (LWPS)
 * 
 *        Config (INF Style) parser
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 *
 */
#ifndef __LWPS_CFG_P_H__
#define __LWPS_CFG_P_H__

#define LWPS_CFG_TOKEN_DEFAULT_LENGTH 128



typedef struct __LWPS_CONFIG_PARSE_STATE
{
    PSTR  pszFilePath;
    
    PVOID pData;
    
    DWORD dwOptions;
    
    FILE* fp;
    
    DWORD dwLine;
    DWORD dwCol;

    BOOLEAN bSkipSection;
    
    PSTR pszSectionName;
    
    PLWPS_STACK pLexerTokenStack; //only for lexer
    
    PFNLWPS_CONFIG_START_SECTION   pfnStartSectionHandler;
    PFNLWPS_CONFIG_COMMENT         pfnCommentHandler;
    PFNLWPS_CONFIG_NAME_VALUE_PAIR pfnNameValuePairHandler;
    PFNLWPS_CONFIG_END_SECTION     pfnEndSectionHandler;
    
} LWPS_CONFIG_PARSE_STATE, *PLWPS_CONFIG_PARSE_STATE;

typedef enum
{
    Consume,
    Pushback,
    Skip
} LwpsCfgLexAction;

typedef enum
{
    LwpsLexBegin = 0,
    LwpsLexChar,
    LwpsLexLSqBrace,
    LwpsLexRSqBrace,
    LwpsLexEquals,
    LwpsLexHash,
    LwpsLexNewline,
    LwpsLexOther,
    LwpsLexEOF,
    LwpsLexEnd
} LwpsCfgLexState;

typedef enum
{
    LwpsCfgNone = 0,
    LwpsCfgString,
    LwpsCfgHash,
    LwpsCfgNewline,
    LwpsCfgEquals,
    LwpsCfgRightSquareBrace,
    LwpsCfgLeftSquareBrace,
    LwpsCfgOther,
    LwpsCfgEOF
} LwpsCfgTokenType;

typedef struct __LWPS_CFG_TOKEN
{
    LwpsCfgTokenType tokenType;
    PSTR            pszToken;
    DWORD           dwMaxLen;
    DWORD           dwLen;
} LWPS_CFG_TOKEN, *PLWPS_CFG_TOKEN;

typedef struct __LWPS_CFG_LEXER_STATE
{
    LwpsCfgLexState  nextState;
    LwpsCfgLexAction action;
    LwpsCfgTokenType tokenId;
} LWPS_CFG_LEXER_STATE, *PLWPS_CFG_LEXER_STATE;

DWORD
LwpsCfgInitParseState(
    PCSTR                          pszFilePath,
    DWORD                          dwOptions,
    PFNLWPS_CONFIG_START_SECTION   pfnStartSectionHandler,
    PFNLWPS_CONFIG_COMMENT         pfnCommentHandler,
    PFNLWPS_CONFIG_NAME_VALUE_PAIR pfnNameValuePairHandler,
    PFNLWPS_CONFIG_END_SECTION     pfnEndSectionHandler,
    PVOID                          pData,
    PLWPS_CONFIG_PARSE_STATE*      ppParseState
    );

VOID
LwpsCfgFreeParseState(
    PLWPS_CONFIG_PARSE_STATE pParseState
    );

DWORD
LwpsCfgParse(
    PLWPS_CONFIG_PARSE_STATE pParseState
    );

DWORD
LwpsCfgParseSections(
    PLWPS_CONFIG_PARSE_STATE pParseState
    );

DWORD
LwpsCfgParseComment(
    PLWPS_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN    pbContinue
    );

DWORD
LwpsCfgParseSectionHeader(
    PLWPS_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN pbContinue
    );

DWORD
LwpsAssertWhitespaceOnly(
    PLWPS_CONFIG_PARSE_STATE pParseState
    );

DWORD
LwpsCfgParseNameValuePair(
    PLWPS_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN pbContinue
    );

DWORD
LwpsCfgProcessComment(
    PLWPS_CONFIG_PARSE_STATE pParseState,
    PLWPS_STACK*             ppTokenStack,
    PBOOLEAN                pbContinue
    );

DWORD
LwpsCfgProcessBeginSection(
    PLWPS_CONFIG_PARSE_STATE pParseState,
    PLWPS_STACK*             ppTokenStack,
    PBOOLEAN                pbContinue
    );

DWORD
LwpsCfgProcessNameValuePair(
    PLWPS_CONFIG_PARSE_STATE pParseState,
    PLWPS_STACK*             ppTokenStack,
    PBOOLEAN                pbContinue
    );

DWORD
LwpsCfgProcessEndSection(
    PLWPS_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN                pbContinue
    );

DWORD
LwpsCfgDetermineTokenLength(
    PLWPS_STACK pStack
    );

//this will consume the token stack
DWORD
LwpsCfgProcessTokenStackIntoString(
    PLWPS_STACK* ppTokenStack,
    PSTR* ppszConcatenated
    );

DWORD
LwpsCfgAllocateToken(
    DWORD           dwSize,
    PLWPS_CFG_TOKEN* ppToken
    );

DWORD
LwpsCfgReallocToken(
    PLWPS_CFG_TOKEN pToken,
    DWORD          dwNewSize
    );

VOID
LwpsCfgResetToken(
    PLWPS_CFG_TOKEN pToken
    );

DWORD
LwpsCfgCopyToken(
    PLWPS_CFG_TOKEN pTokenSrc,
    PLWPS_CFG_TOKEN pTokenDst
    );

DWORD
LwpsCfgFreeTokenStack(
    PLWPS_STACK* ppTokenStack
    );

DWORD
LwpsCfgFreeTokenInStack(
    PVOID pToken,
    PVOID pUserData
    );

VOID
LwpsCfgFreeToken(
    PLWPS_CFG_TOKEN pToken
    );

DWORD
LwpsCfgGetNextToken(
    PLWPS_CONFIG_PARSE_STATE pParseState,
    PLWPS_CFG_TOKEN*         ppToken
    );

DWORD
LwpsCfgGetCharacter(
    PLWPS_CONFIG_PARSE_STATE pParseState
    );

LwpsCfgLexState
LwpsCfgGetLexClass(
    DWORD ch
    );

DWORD
LwpsCfgPushBackCharacter(
    PLWPS_CONFIG_PARSE_STATE pParseState,
    BYTE ch
    );

LwpsCfgLexState
LwpsCfgGetNextLexState(
    LwpsCfgLexState currentState,
    DWORD chId
    );

LwpsCfgLexAction
LwpsCfgGetLexAction(
    LwpsCfgLexState currentState,
    DWORD chId
    );

LwpsCfgTokenType
LwpsCfgGetTokenType(
    LwpsCfgLexState currentState,
    DWORD chId
    );

#endif /* __LWPS_CFG_P_H__ */
