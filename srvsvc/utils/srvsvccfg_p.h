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
 *        evtcfg_p.h
 *
 * Abstract:
 *
 *        Likewise Server Service
 *
 *        Config (INF Style) parser
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
 */
#ifndef __SRVSVCCFG_P_H__
#define __SRVSVCCFG_P_H__

#define SRVSVC_CFG_TOKEN_DEFAULT_LENGTH 128

typedef struct __SRVSVC_CONFIG_PARSE_STATE
{
    PSTR  pszFilePath;

    FILE* fp;

    DWORD dwLine;
    DWORD dwCol;

    BOOLEAN bSkipSection;

    PSTR pszSectionName;

    PSRVSVC_STACK pLexerTokenStack; //only for lexer

    PFNCONFIG_START_SECTION   pfnStartSectionHandler;
    PFNCONFIG_COMMENT         pfnCommentHandler;
    PFNCONFIG_NAME_VALUE_PAIR pfnNameValuePairHandler;
    PFNCONFIG_END_SECTION     pfnEndSectionHandler;

} SRVSVC_CONFIG_PARSE_STATE, *PSRVSVC_CONFIG_PARSE_STATE;

typedef enum
{
    Consume,
    Pushback,
    Skip
} SRVSVCLexAction;

typedef enum
{
    SRVSVCLexBegin = 0,
    SRVSVCLexChar,
    SRVSVCLexLSqBrace,
    SRVSVCLexRSqBrace,
    SRVSVCLexEquals,
    SRVSVCLexHash,
    SRVSVCLexNewline,
    SRVSVCLexOther,
    SRVSVCLexEOF,
    SRVSVCLexEnd
} SRVSVCCfgLexState;

typedef enum
{
    SRVSVCCfgNone = 0,
    SRVSVCCfgString,
    SRVSVCCfgHash,
    SRVSVCCfgNewline,
    SRVSVCCfgEquals,
    SRVSVCCfgRightSquareBrace,
    SRVSVCCfgLeftSquareBrace,
    SRVSVCCfgOther,
    SRVSVCCfgEOF
} SRVSVCCfgTokenType;

typedef enum {
    PARSE_STATE_COMMENT       = 0,
    PARSE_STATE_START_SECTION,
    PARSE_STATE_SECTION_NAME,
    PARSE_STATE_SECTION,
    PARSE_STATE_NAME,
    PARSE_STATE_EQUALS,
    PARSE_STATE_VALUE
} SRVSVCCfgParseState;

typedef struct __SRVSVC_CFG_ELEMENT
{
    SRVSVCCfgParseState parseState;
    PSTR             pszToken;
    DWORD            dwMaxLen;
    DWORD            dwLen;
} SRVSVC_CFG_ELEMENT, *PSRVSVC_CFG_ELEMENT;

typedef struct __SRVSVC_CFG_TOKEN
{
    SRVSVCCfgTokenType tokenType;
    PSTR            pszToken;
    DWORD           dwMaxLen;
    DWORD           dwLen;
} SRVSVC_CFG_TOKEN, *PSRVSVC_CFG_TOKEN;

typedef struct __SRVSVC_CFG_LEXER_STATE
{
    SRVSVCCfgLexState  nextState;
    SRVSVCLexAction action;
    SRVSVCCfgTokenType tokenId;
} SRVSVC_CFG_LEXER_STATE, *PSRVSVC_CFG_LEXER_STATE;

DWORD
SRVSVCCfgInitParseState(
    PCSTR                     pszFilePath,
    PFNCONFIG_START_SECTION   pfnStartSectionHandler,
    PFNCONFIG_COMMENT         pfnCommentHandler,
    PFNCONFIG_NAME_VALUE_PAIR pfnNameValuePairHandler,
    PFNCONFIG_END_SECTION     pfnEndSectionHandler,
    PSRVSVC_CONFIG_PARSE_STATE*  ppParseState
    );

VOID
SRVSVCCfgFreeParseState(
    PSRVSVC_CONFIG_PARSE_STATE pParseState
    );

DWORD
SRVSVCCfgParse(
    PSRVSVC_CONFIG_PARSE_STATE pParseState
    );

DWORD
SRVSVCCfgParseSections(
    PSRVSVC_CONFIG_PARSE_STATE pParseState
    );

DWORD
SRVSVCCfgParseComment(
    PSRVSVC_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN    pbContinue
    );

DWORD
SRVSVCCfgParseSectionHeader(
    PSRVSVC_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN pbContinue
    );

DWORD
SRVSVCAssertWhitespaceOnly(
    PSRVSVC_CONFIG_PARSE_STATE pParseState
    );

DWORD
SRVSVCCfgParseNameValuePair(
    PSRVSVC_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN pbContinue
    );

DWORD
SRVSVCCfgProcessComment(
    PSRVSVC_CONFIG_PARSE_STATE pParseState,
    PSRVSVC_STACK*             ppTokenStack,
    PBOOLEAN                pbContinue
    );

DWORD
SRVSVCCfgProcessBeginSection(
    PSRVSVC_CONFIG_PARSE_STATE pParseState,
    PSRVSVC_STACK*             ppTokenStack,
    PBOOLEAN                pbContinue
    );

DWORD
SRVSVCCfgProcessNameValuePair(
    PSRVSVC_CONFIG_PARSE_STATE pParseState,
    PSRVSVC_STACK*             ppTokenStack,
    PBOOLEAN                pbContinue
    );

DWORD
SRVSVCCfgProcessEndSection(
    PSRVSVC_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN                pbContinue
    );

DWORD
SRVSVCCfgDetermineTokenLength(
    PSRVSVC_STACK pStack
    );

//this will consume the token stack
DWORD
SRVSVCCfgProcessTokenStackIntoString(
    PSRVSVC_STACK* ppTokenStack,
    PSTR* ppszConcatenated
    );

DWORD
SRVSVCCfgAllocateToken(
    DWORD           dwSize,
    PSRVSVC_CFG_TOKEN* ppToken
    );

DWORD
SRVSVCCfgReallocToken(
    PSRVSVC_CFG_TOKEN pToken,
    DWORD          dwNewSize
    );

VOID
SRVSVCCfgResetToken(
    PSRVSVC_CFG_TOKEN pToken
    );

DWORD
SRVSVCCfgCopyToken(
    PSRVSVC_CFG_TOKEN pTokenSrc,
    PSRVSVC_CFG_TOKEN pTokenDst
    );

DWORD
SRVSVCCfgFreeTokenStack(
    PSRVSVC_STACK* ppTokenStack
    );

DWORD
SRVSVCCfgFreeTokenInStack(
    PVOID pToken,
    PVOID pUserData
    );

VOID
SRVSVCCfgFreeToken(
    PSRVSVC_CFG_TOKEN pToken
    );

DWORD
SRVSVCCfgGetNextToken(
    PSRVSVC_CONFIG_PARSE_STATE pParseState,
    PSRVSVC_CFG_TOKEN*         ppToken
    );

DWORD
SRVSVCCfgGetCharacter(
    PSRVSVC_CONFIG_PARSE_STATE pParseState
    );

SRVSVCCfgLexState
SRVSVCCfgGetLexClass(
    DWORD ch
    );

DWORD
SRVSVCCfgPushBackCharacter(
    PSRVSVC_CONFIG_PARSE_STATE pParseState,
    BYTE ch
    );

SRVSVCCfgLexState
SRVSVCCfgGetNextLexState(
    SRVSVCCfgLexState currentState,
    DWORD chId
    );

SRVSVCLexAction
SRVSVCCfgGetLexAction(
    SRVSVCCfgLexState currentState,
    DWORD chId
    );

SRVSVCCfgTokenType
SRVSVCCfgGetTokenType(
    SRVSVCCfgLexState currentState,
    DWORD chId
    );



#endif /* __SRVSVCCFG_P_H__ */
