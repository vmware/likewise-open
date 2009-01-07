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
 *        smbconfig.h
 *
 * Abstract:
 *
 *        Likewise Server Message Block (LSMB)
 *
 *        Config (INF Style) parser
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 */
#ifndef __SMBCFG_P_H__
#define __SMBCFG_P_H__

#define SMB_CFG_TOKEN_DEFAULT_LENGTH 256

typedef struct __SMB_CONFIG_PARSE_STATE
{
    PSTR  pszFilePath;

    PVOID pData;

    DWORD dwOptions;

    FILE* fp;

    DWORD dwLine;
    DWORD dwCol;

    BOOLEAN bSkipSection;

    PSTR pszSectionName;

    PSMB_STACK pLexerTokenStack; //only for lexer

    PFNSMB_CONFIG_START_SECTION   pfnStartSectionHandler;
    PFNSMB_CONFIG_COMMENT         pfnCommentHandler;
    PFNSMB_CONFIG_NAME_VALUE_PAIR pfnNameValuePairHandler;
    PFNSMB_CONFIG_END_SECTION     pfnEndSectionHandler;

} SMB_CONFIG_PARSE_STATE, *PSMB_CONFIG_PARSE_STATE;

typedef enum
{
    Consume,
    Pushback,
    Skip
} SMBCfgLexAction;

typedef enum
{
    SMBLexBegin = 0,
    SMBLexChar,
    SMBLexLSqBrace,
    SMBLexRSqBrace,
    SMBLexEquals,
    SMBLexHash,
    SMBLexNewline,
    SMBLexOther,
    SMBLexEOF,
    SMBLexEnd
} SMBCfgLexState;

typedef enum
{
    SMBCfgNone = 0,
    SMBCfgString,
    SMBCfgHash,
    SMBCfgNewline,
    SMBCfgEquals,
    SMBCfgRightSquareBrace,
    SMBCfgLeftSquareBrace,
    SMBCfgOther,
    SMBCfgEOF
} SMBCfgTokenType;

typedef struct __SMB_CFG_TOKEN
{
    SMBCfgTokenType tokenType;
    PSTR            pszToken;
    DWORD           dwMaxLen;
    DWORD           dwLen;
} SMB_CFG_TOKEN, *PSMB_CFG_TOKEN;

typedef struct __SMB_CFG_LEXER_STATE
{
    SMBCfgLexState  nextState;
    SMBCfgLexAction action;
    SMBCfgTokenType tokenId;
} SMB_CFG_LEXER_STATE, *PSMB_CFG_LEXER_STATE;

DWORD
SMBCfgInitParseState(
    PCSTR                         pszFilePath,
    DWORD                         dwOptions,
    PFNSMB_CONFIG_START_SECTION   pfnStartSectionHandler,
    PFNSMB_CONFIG_COMMENT         pfnCommentHandler,
    PFNSMB_CONFIG_NAME_VALUE_PAIR pfnNameValuePairHandler,
    PFNSMB_CONFIG_END_SECTION     pfnEndSectionHandler,
    PVOID                         pData,
    PSMB_CONFIG_PARSE_STATE*      ppParseState
    );

VOID
SMBCfgFreeParseState(
    PSMB_CONFIG_PARSE_STATE pParseState
    );

DWORD
SMBCfgParse(
    PSMB_CONFIG_PARSE_STATE pParseState
    );

DWORD
SMBCfgParseSections(
    PSMB_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN pbContinue
    );

DWORD
SMBCfgParseComment(
    PSMB_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN    pbContinue
    );

DWORD
SMBCfgParseSectionHeader(
    PSMB_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN pbContinue
    );

DWORD
SMBAssertWhitespaceOnly(
    PSMB_CONFIG_PARSE_STATE pParseState
    );

DWORD
SMBCfgParseNameValuePair(
    PSMB_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN pbContinue
    );

DWORD
SMBCfgProcessComment(
    PSMB_CONFIG_PARSE_STATE pParseState,
    PSMB_STACK*             ppTokenStack,
    PBOOLEAN                pbContinue
    );

DWORD
SMBCfgProcessBeginSection(
    PSMB_CONFIG_PARSE_STATE pParseState,
    PSMB_STACK*             ppTokenStack,
    PBOOLEAN                pbContinue
    );

DWORD
SMBCfgProcessNameValuePair(
    PSMB_CONFIG_PARSE_STATE pParseState,
    PSMB_STACK*             ppTokenStack,
    PBOOLEAN                pbContinue
    );

DWORD
SMBCfgProcessEndSection(
    PSMB_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN                pbContinue
    );

DWORD
SMBCfgDetermineTokenLength(
    PSMB_STACK pStack
    );

//this will consume the token stack
DWORD
SMBCfgProcessTokenStackIntoString(
    PSMB_STACK* ppTokenStack,
    PSTR* ppszConcatenated
    );

DWORD
SMBCfgAllocateToken(
    DWORD           dwSize,
    PSMB_CFG_TOKEN* ppToken
    );

DWORD
SMBCfgReallocToken(
    PSMB_CFG_TOKEN pToken,
    DWORD          dwNewSize
    );

VOID
SMBCfgResetToken(
    PSMB_CFG_TOKEN pToken
    );

DWORD
SMBCfgCopyToken(
    PSMB_CFG_TOKEN pTokenSrc,
    PSMB_CFG_TOKEN pTokenDst
    );

DWORD
SMBCfgFreeTokenStack(
    PSMB_STACK* ppTokenStack
    );

DWORD
SMBCfgFreeTokenInStack(
    PVOID pToken,
    PVOID pUserData
    );

VOID
SMBCfgFreeToken(
    PSMB_CFG_TOKEN pToken
    );

DWORD
SMBCfgGetNextToken(
    PSMB_CONFIG_PARSE_STATE pParseState,
    PSMB_CFG_TOKEN*         ppToken
    );

DWORD
SMBCfgGetCharacter(
    PSMB_CONFIG_PARSE_STATE pParseState
    );

SMBCfgLexState
SMBCfgGetLexClass(
    DWORD ch
    );

DWORD
SMBCfgPushBackCharacter(
    PSMB_CONFIG_PARSE_STATE pParseState,
    BYTE ch
    );

SMBCfgLexState
SMBCfgGetNextLexState(
    SMBCfgLexState currentState,
    DWORD chId
    );

SMBCfgLexAction
SMBCfgGetLexAction(
    SMBCfgLexState currentState,
    DWORD chId
    );

SMBCfgTokenType
SMBCfgGetTokenType(
    SMBCfgLexState currentState,
    DWORD chId
    );



#endif /* __SMBCFG_P_H__ */
