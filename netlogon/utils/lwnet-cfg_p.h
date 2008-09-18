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
 *        lwnet-cfg_p.h
 *
 * Abstract:
 *
 *        Likewise Site Manager
 * 
 *        Config (INF Style) parser
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 * 
 */
#ifndef __LWNETCFG_P_H__
#define __LWNETCFG_P_H__

#define LWNET_CFG_TOKEN_DEFAULT_LENGTH 128



typedef struct __LWNET_CONFIG_PARSE_STATE
{
    PSTR  pszFilePath;
    
    PVOID pData;
    
    DWORD dwOptions;
    
    FILE* fp;
    
    DWORD dwLine;
    DWORD dwCol;

    BOOLEAN bSkipSection;
    
    PSTR pszSectionName;
    
    PLWNET_STACK pLexerTokenStack; //only for lexer
    
    PFNLWNET_CONFIG_START_SECTION   pfnStartSectionHandler;
    PFNLWNET_CONFIG_COMMENT         pfnCommentHandler;
    PFNLWNET_CONFIG_NAME_VALUE_PAIR pfnNameValuePairHandler;
    PFNLWNET_CONFIG_END_SECTION     pfnEndSectionHandler;
    
} LWNET_CONFIG_PARSE_STATE, *PLWNET_CONFIG_PARSE_STATE;

typedef enum
{
    Consume,
    Pushback,
    Skip
} LWNetCfgLexAction;

typedef enum
{
    LWNetLexBegin = 0,
    LWNetLexChar,
    LWNetLexLSqBrace,
    LWNetLexRSqBrace,
    LWNetLexEquals,
    LWNetLexHash,
    LWNetLexNewline,
    LWNetLexOther,
    LWNetLexEOF,
    LWNetLexEnd
} LWNetCfgLexState;

typedef enum
{
    LWNetCfgNone = 0,
    LWNetCfgString,
    LWNetCfgHash,
    LWNetCfgNewline,
    LWNetCfgEquals,
    LWNetCfgRightSquareBrace,
    LWNetCfgLeftSquareBrace,
    LWNetCfgOther,
    LWNetCfgEOF
} LWNetCfgTokenType;

typedef struct __LWNET_CFG_TOKEN
{
    LWNetCfgTokenType tokenType;
    PSTR            pszToken;
    DWORD           dwMaxLen;
    DWORD           dwLen;
} LWNET_CFG_TOKEN, *PLWNET_CFG_TOKEN;

typedef struct __LWNET_CFG_LEXER_STATE
{
    LWNetCfgLexState  nextState;
    LWNetCfgLexAction action;
    LWNetCfgTokenType tokenId;
} LWNET_CFG_LEXER_STATE, *PLWNET_CFG_LEXER_STATE;

DWORD
LWNetCfgInitParseState(
    PCSTR                            pszFilePath,
    DWORD                            dwOptions,
    PFNLWNET_CONFIG_START_SECTION   pfnStartSectionHandler,
    PFNLWNET_CONFIG_COMMENT         pfnCommentHandler,
    PFNLWNET_CONFIG_NAME_VALUE_PAIR pfnNameValuePairHandler,
    PFNLWNET_CONFIG_END_SECTION     pfnEndSectionHandler,
    PVOID                            pData,
    PLWNET_CONFIG_PARSE_STATE*      ppParseState
    );

VOID
LWNetCfgFreeParseState(
    PLWNET_CONFIG_PARSE_STATE pParseState
    );

DWORD
LWNetCfgParse(
    PLWNET_CONFIG_PARSE_STATE pParseState
    );

DWORD
LWNetCfgParseSections(
    PLWNET_CONFIG_PARSE_STATE pParseState
    );

DWORD
LWNetCfgParseComment(
    PLWNET_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN    pbContinue
    );

DWORD
LWNetCfgParseSectionHeader(
    PLWNET_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN pbContinue
    );

DWORD
LWNetAssertWhitespaceOnly(
    PLWNET_CONFIG_PARSE_STATE pParseState
    );

DWORD
LWNetCfgParseNameValuePair(
    PLWNET_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN pbContinue
    );

DWORD
LWNetCfgProcessComment(
    PLWNET_CONFIG_PARSE_STATE pParseState,
    PLWNET_STACK*             ppTokenStack,
    PBOOLEAN                pbContinue
    );

DWORD
LWNetCfgProcessBeginSection(
    PLWNET_CONFIG_PARSE_STATE pParseState,
    PLWNET_STACK*             ppTokenStack,
    PBOOLEAN                pbContinue
    );

DWORD
LWNetCfgProcessNameValuePair(
    PLWNET_CONFIG_PARSE_STATE pParseState,
    PLWNET_STACK*             ppTokenStack,
    PBOOLEAN                pbContinue
    );

DWORD
LWNetCfgProcessEndSection(
    PLWNET_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN                pbContinue
    );

DWORD
LWNetCfgDetermineTokenLength(
    PLWNET_STACK pStack
    );

//this will consume the token stack
DWORD
LWNetCfgProcessTokenStackIntoString(
    PLWNET_STACK* ppTokenStack,
    PSTR* ppszConcatenated
    );

DWORD
LWNetCfgAllocateToken(
    DWORD           dwSize,
    PLWNET_CFG_TOKEN* ppToken
    );

DWORD
LWNetCfgReallocToken(
    PLWNET_CFG_TOKEN pToken,
    DWORD          dwNewSize
    );

VOID
LWNetCfgResetToken(
    PLWNET_CFG_TOKEN pToken
    );

DWORD
LWNetCfgCopyToken(
    PLWNET_CFG_TOKEN pTokenSrc,
    PLWNET_CFG_TOKEN pTokenDst
    );

DWORD
LWNetCfgFreeTokenStack(
    PLWNET_STACK* ppTokenStack
    );

DWORD
LWNetCfgFreeTokenInStack(
    PVOID pToken,
    PVOID pUserData
    );

VOID
LWNetCfgFreeToken(
    PLWNET_CFG_TOKEN pToken
    );

DWORD
LWNetCfgGetNextToken(
    PLWNET_CONFIG_PARSE_STATE pParseState,
    PLWNET_CFG_TOKEN*         ppToken
    );

DWORD
LWNetCfgGetCharacter(
    PLWNET_CONFIG_PARSE_STATE pParseState
    );

LWNetCfgLexState
LWNetCfgGetLexClass(
    DWORD ch
    );

DWORD
LWNetCfgPushBackCharacter(
    PLWNET_CONFIG_PARSE_STATE pParseState,
    BYTE ch
    );

LWNetCfgLexState
LWNetCfgGetNextLexState(
    LWNetCfgLexState currentState,
    DWORD chId
    );

LWNetCfgLexAction
LWNetCfgGetLexAction(
    LWNetCfgLexState currentState,
    DWORD chId
    );

LWNetCfgTokenType
LWNetCfgGetTokenType(
    LWNetCfgLexState currentState,
    DWORD chId
    );



#endif /* __LWNETCFG_P_H__ */
