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
 *        Likewise Eventlog
 *
 *        Config (INF Style) parser
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
 */
#ifndef __EVTCFG_P_H__
#define __EVTCFG_P_H__

#define EVT_CFG_TOKEN_DEFAULT_LENGTH 128

typedef struct __EVT_CONFIG_PARSE_STATE
{
    PSTR  pszFilePath;

    FILE* fp;

    DWORD dwLine;
    DWORD dwCol;

    BOOLEAN bSkipSection;

    PSTR pszSectionName;

    PEVT_STACK pLexerTokenStack; //only for lexer

    PFNCONFIG_START_SECTION   pfnStartSectionHandler;
    PFNCONFIG_COMMENT         pfnCommentHandler;
    PFNCONFIG_NAME_VALUE_PAIR pfnNameValuePairHandler;
    PFNCONFIG_END_SECTION     pfnEndSectionHandler;

} EVT_CONFIG_PARSE_STATE, *PEVT_CONFIG_PARSE_STATE;

typedef enum
{
    Consume,
    Pushback,
    Skip
} EVTLexAction;

typedef enum
{
    EVTLexBegin = 0,
    EVTLexChar,
    EVTLexLSqBrace,
    EVTLexRSqBrace,
    EVTLexEquals,
    EVTLexHash,
    EVTLexNewline,
    EVTLexOther,
    EVTLexEOF,
    EVTLexEnd
} EVTCfgLexState;

typedef enum
{
    EVTCfgNone = 0,
    EVTCfgString,
    EVTCfgHash,
    EVTCfgNewline,
    EVTCfgEquals,
    EVTCfgRightSquareBrace,
    EVTCfgLeftSquareBrace,
    EVTCfgOther,
    EVTCfgEOF
} EVTCfgTokenType;

typedef enum {
    PARSE_STATE_COMMENT       = 0,
    PARSE_STATE_START_SECTION,
    PARSE_STATE_SECTION_NAME,
    PARSE_STATE_SECTION,
    PARSE_STATE_NAME,
    PARSE_STATE_EQUALS,
    PARSE_STATE_VALUE
} EVTCfgParseState;

typedef struct __EVT_CFG_ELEMENT
{
    EVTCfgParseState parseState;
    PSTR             pszToken;
    DWORD            dwMaxLen;
    DWORD            dwLen;
} EVT_CFG_ELEMENT, *PEVT_CFG_ELEMENT;

typedef struct __EVT_CFG_TOKEN
{
    EVTCfgTokenType tokenType;
    PSTR            pszToken;
    DWORD           dwMaxLen;
    DWORD           dwLen;
} EVT_CFG_TOKEN, *PEVT_CFG_TOKEN;

typedef struct __EVT_CFG_LEXER_STATE
{
    EVTCfgLexState  nextState;
    EVTLexAction action;
    EVTCfgTokenType tokenId;
} EVT_CFG_LEXER_STATE, *PEVT_CFG_LEXER_STATE;

DWORD
EVTCfgInitParseState(
    PCSTR                     pszFilePath,
    PFNCONFIG_START_SECTION   pfnStartSectionHandler,
    PFNCONFIG_COMMENT         pfnCommentHandler,
    PFNCONFIG_NAME_VALUE_PAIR pfnNameValuePairHandler,
    PFNCONFIG_END_SECTION     pfnEndSectionHandler,
    PEVT_CONFIG_PARSE_STATE*  ppParseState
    );

VOID
EVTCfgFreeParseState(
    PEVT_CONFIG_PARSE_STATE pParseState
    );

DWORD
EVTCfgParse(
    PEVT_CONFIG_PARSE_STATE pParseState
    );

DWORD
EVTCfgParseSections(
    PEVT_CONFIG_PARSE_STATE pParseState
    );

DWORD
EVTCfgParseComment(
    PEVT_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN    pbContinue
    );

DWORD
EVTCfgParseSectionHeader(
    PEVT_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN pbContinue
    );

DWORD
EVTAssertWhitespaceOnly(
    PEVT_CONFIG_PARSE_STATE pParseState
    );

DWORD
EVTCfgParseNameValuePair(
    PEVT_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN pbContinue
    );

DWORD
EVTCfgProcessComment(
    PEVT_CONFIG_PARSE_STATE pParseState,
    PEVT_STACK*             ppTokenStack,
    PBOOLEAN                pbContinue
    );

DWORD
EVTCfgProcessBeginSection(
    PEVT_CONFIG_PARSE_STATE pParseState,
    PEVT_STACK*             ppTokenStack,
    PBOOLEAN                pbContinue
    );

DWORD
EVTCfgProcessNameValuePair(
    PEVT_CONFIG_PARSE_STATE pParseState,
    PEVT_STACK*             ppTokenStack,
    PBOOLEAN                pbContinue
    );

DWORD
EVTCfgProcessEndSection(
    PEVT_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN                pbContinue
    );

DWORD
EVTCfgDetermineTokenLength(
    PEVT_STACK pStack
    );

//this will consume the token stack
DWORD
EVTCfgProcessTokenStackIntoString(
    PEVT_STACK* ppTokenStack,
    PSTR* ppszConcatenated
    );

DWORD
EVTCfgAllocateToken(
    DWORD           dwSize,
    PEVT_CFG_TOKEN* ppToken
    );

DWORD
EVTCfgReallocToken(
    PEVT_CFG_TOKEN pToken,
    DWORD          dwNewSize
    );

VOID
EVTCfgResetToken(
    PEVT_CFG_TOKEN pToken
    );

DWORD
EVTCfgCopyToken(
    PEVT_CFG_TOKEN pTokenSrc,
    PEVT_CFG_TOKEN pTokenDst
    );

DWORD
EVTCfgFreeTokenStack(
    PEVT_STACK* ppTokenStack
    );

DWORD
EVTCfgFreeTokenInStack(
    PVOID pToken,
    PVOID pUserData
    );

VOID
EVTCfgFreeToken(
    PEVT_CFG_TOKEN pToken
    );

DWORD
EVTCfgGetNextToken(
    PEVT_CONFIG_PARSE_STATE pParseState,
    PEVT_CFG_TOKEN*         ppToken
    );

DWORD
EVTCfgGetCharacter(
    PEVT_CONFIG_PARSE_STATE pParseState
    );

EVTCfgLexState
EVTCfgGetLexClass(
    DWORD ch
    );

DWORD
EVTCfgPushBackCharacter(
    PEVT_CONFIG_PARSE_STATE pParseState,
    BYTE ch
    );

EVTCfgLexState
EVTCfgGetNextLexState(
    EVTCfgLexState currentState,
    DWORD chId
    );

EVTLexAction
EVTCfgGetLexAction(
    EVTCfgLexState currentState,
    DWORD chId
    );

EVTCfgTokenType
EVTCfgGetTokenType(
    EVTCfgLexState currentState,
    DWORD chId
    );



#endif /* __EVTCFG_P_H__ */
