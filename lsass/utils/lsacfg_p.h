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
 *        lsacfg.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Config (INF Style) parser
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 */
#ifndef __LSACFG_P_H__
#define __LSACFG_P_H__

#define LSA_CFG_TOKEN_DEFAULT_LENGTH 256



typedef struct __LSA_CONFIG_PARSE_STATE
{
    PSTR  pszFilePath;

    PVOID pData;

    DWORD dwOptions;

    FILE* fp;

    DWORD dwLine;
    DWORD dwCol;

    BOOLEAN bSkipSection;

    PSTR pszSectionName;

    PLSA_STACK pLexerTokenStack; //only for lexer

    PFNCONFIG_START_SECTION   pfnStartSectionHandler;
    PFNCONFIG_COMMENT         pfnCommentHandler;
    PFNCONFIG_NAME_VALUE_PAIR pfnNameValuePairHandler;
    PFNCONFIG_END_SECTION     pfnEndSectionHandler;

} LSA_CONFIG_PARSE_STATE, *PLSA_CONFIG_PARSE_STATE;

typedef enum
{
    Consume,
    Pushback,
    Skip
} LsaCfgLexAction;

typedef enum
{
    LsaLexBegin = 0,
    LsaLexChar,
    LsaLexLSqBrace,
    LsaLexRSqBrace,
    LsaLexEquals,
    LsaLexHash,
    LsaLexNewline,
    LsaLexOther,
    LsaLexEOF,
    LsaLexEnd
} LsaCfgLexState;

typedef enum
{
    LsaCfgNone = 0,
    LsaCfgString,
    LsaCfgHash,
    LsaCfgNewline,
    LsaCfgEquals,
    LsaCfgRightSquareBrace,
    LsaCfgLeftSquareBrace,
    LsaCfgOther,
    LsaCfgEOF
} LsaCfgTokenType;

typedef struct __LSA_CFG_TOKEN
{
    LsaCfgTokenType tokenType;
    PSTR            pszToken;
    DWORD           dwMaxLen;
    DWORD           dwLen;
} LSA_CFG_TOKEN, *PLSA_CFG_TOKEN;

typedef struct __LSA_CFG_LEXER_STATE
{
    LsaCfgLexState  nextState;
    LsaCfgLexAction action;
    LsaCfgTokenType tokenId;
} LSA_CFG_LEXER_STATE, *PLSA_CFG_LEXER_STATE;

DWORD
LsaCfgInitParseState(
    PCSTR                     pszFilePath,
    DWORD                     dwOptions,
    PFNCONFIG_START_SECTION   pfnStartSectionHandler,
    PFNCONFIG_COMMENT         pfnCommentHandler,
    PFNCONFIG_NAME_VALUE_PAIR pfnNameValuePairHandler,
    PFNCONFIG_END_SECTION     pfnEndSectionHandler,
    PVOID                     pData,
    PLSA_CONFIG_PARSE_STATE*  ppParseState
    );

VOID
LsaCfgFreeParseState(
    PLSA_CONFIG_PARSE_STATE pParseState
    );

DWORD
LsaCfgParse(
    PLSA_CONFIG_PARSE_STATE pParseState
    );

DWORD
LsaCfgParseSections(
    PLSA_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN pbContinue
    );

DWORD
LsaCfgParseComment(
    PLSA_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN    pbContinue
    );

DWORD
LsaCfgParseSectionHeader(
    PLSA_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN pbContinue
    );

DWORD
LsaAssertWhitespaceOnly(
    PLSA_CONFIG_PARSE_STATE pParseState
    );

DWORD
LsaCfgParseNameValuePair(
    PLSA_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN pbContinue
    );

DWORD
LsaCfgProcessComment(
    PLSA_CONFIG_PARSE_STATE pParseState,
    PLSA_STACK*             ppTokenStack,
    PBOOLEAN                pbContinue
    );

DWORD
LsaCfgProcessBeginSection(
    PLSA_CONFIG_PARSE_STATE pParseState,
    PLSA_STACK*             ppTokenStack,
    PBOOLEAN                pbContinue
    );

DWORD
LsaCfgProcessNameValuePair(
    PLSA_CONFIG_PARSE_STATE pParseState,
    PLSA_STACK*             ppTokenStack,
    PBOOLEAN                pbContinue
    );

DWORD
LsaCfgProcessEndSection(
    PLSA_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN                pbContinue
    );

DWORD
LsaCfgDetermineTokenLength(
    PLSA_STACK pStack
    );

//this will consume the token stack
DWORD
LsaCfgProcessTokenStackIntoString(
    PLSA_STACK* ppTokenStack,
    PSTR* ppszConcatenated
    );

DWORD
LsaCfgAllocateToken(
    DWORD           dwSize,
    PLSA_CFG_TOKEN* ppToken
    );

DWORD
LsaCfgReallocToken(
    PLSA_CFG_TOKEN pToken,
    DWORD          dwNewSize
    );

VOID
LsaCfgResetToken(
    PLSA_CFG_TOKEN pToken
    );

DWORD
LsaCfgCopyToken(
    PLSA_CFG_TOKEN pTokenSrc,
    PLSA_CFG_TOKEN pTokenDst
    );

DWORD
LsaCfgFreeTokenStack(
    PLSA_STACK* ppTokenStack
    );

DWORD
LsaCfgFreeTokenInStack(
    PVOID pToken,
    PVOID pUserData
    );

VOID
LsaCfgFreeToken(
    PLSA_CFG_TOKEN pToken
    );

DWORD
LsaCfgGetNextToken(
    PLSA_CONFIG_PARSE_STATE pParseState,
    PLSA_CFG_TOKEN*         ppToken
    );

DWORD
LsaCfgGetCharacter(
    PLSA_CONFIG_PARSE_STATE pParseState
    );

LsaCfgLexState
LsaCfgGetLexClass(
    DWORD ch
    );

DWORD
LsaCfgPushBackCharacter(
    PLSA_CONFIG_PARSE_STATE pParseState,
    BYTE ch
    );

LsaCfgLexState
LsaCfgGetNextLexState(
    LsaCfgLexState currentState,
    DWORD chId
    );

LsaCfgLexAction
LsaCfgGetLexAction(
    LsaCfgLexState currentState,
    DWORD chId
    );

LsaCfgTokenType
LsaCfgGetTokenType(
    LsaCfgLexState currentState,
    DWORD chId
    );



#endif /* __LSACFG_P_H__ */
