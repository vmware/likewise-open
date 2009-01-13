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
 *        evtcfg.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (SRVSVCSS)
 *
 *        Config (INF Style) parser
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
 */
#include "includes.h"


static SRVSVC_CFG_LEXER_STATE gSRVSVCLexStateTable[][9] =
{
    /* SRVSVCLexBegin    := 0 */
    {
    {   SRVSVCLexBegin,  Consume,             SRVSVCCfgNone }, /* SRVSVCLexBegin    */
    {    SRVSVCLexChar,  Consume,             SRVSVCCfgNone }, /* SRVSVCLexChar     */
    {     SRVSVCLexEnd,  Consume,  SRVSVCCfgLeftSquareBrace }, /* SRVSVCLexLSqBrace */
    {     SRVSVCLexEnd,  Consume, SRVSVCCfgRightSquareBrace }, /* SRVSVCLexRSqBrace */
    {     SRVSVCLexEnd,  Consume,           SRVSVCCfgEquals }, /* SRVSVCLexEquals   */
    {     SRVSVCLexEnd,  Consume,             SRVSVCCfgHash }, /* SRVSVCLexHash     */
    {     SRVSVCLexEnd,  Consume,          SRVSVCCfgNewline }, /* SRVSVCLexNewline  */
    {   SRVSVCLexBegin,     Skip,             SRVSVCCfgNone }, /* SRVSVCLexOther    */
    {     SRVSVCLexEnd,  Consume,              SRVSVCCfgEOF }  /* SRVSVCLexEOF      */
    },
    /* SRVSVCLexChar     := 1 */
    {
    {   SRVSVCLexBegin,  Consume,             SRVSVCCfgNone }, /* SRVSVCLexBegin    */
    {    SRVSVCLexChar,  Consume,             SRVSVCCfgNone }, /* SRVSVCLexChar     */
    {     SRVSVCLexEnd, Pushback,           SRVSVCCfgString }, /* SRVSVCLexLSqBrace */
    {     SRVSVCLexEnd, Pushback,           SRVSVCCfgString }, /* SRVSVCLexRSqBrace */
    {     SRVSVCLexEnd, Pushback,           SRVSVCCfgString }, /* SRVSVCLexEquals   */
    {     SRVSVCLexEnd, Pushback,           SRVSVCCfgString }, /* SRVSVCLexHash     */
    {     SRVSVCLexEnd, Pushback,           SRVSVCCfgString }, /* SRVSVCLexNewline  */
    {     SRVSVCLexEnd,     Skip,           SRVSVCCfgString }, /* SRVSVCLexOther    */
    {     SRVSVCLexEnd,     Skip,           SRVSVCCfgString }  /* SRVSVCLexEOF      */
    },
    /* SRVSVCLexLSqBrace := 2 */
    {
    {   SRVSVCLexBegin,  Consume,             SRVSVCCfgNone }, /* SRVSVCLexBegin    */
    {     SRVSVCLexEnd, Pushback,  SRVSVCCfgLeftSquareBrace }, /* SRVSVCLexChar     */
    {     SRVSVCLexEnd, Pushback,  SRVSVCCfgLeftSquareBrace }, /* SRVSVCLexLSqBrace */
    {     SRVSVCLexEnd, Pushback,  SRVSVCCfgLeftSquareBrace }, /* SRVSVCLexRSqBrace */
    {     SRVSVCLexEnd, Pushback,  SRVSVCCfgLeftSquareBrace }, /* SRVSVCLexEquals   */
    {     SRVSVCLexEnd, Pushback,  SRVSVCCfgLeftSquareBrace }, /* SRVSVCLexHash     */
    {     SRVSVCLexEnd, Pushback,  SRVSVCCfgLeftSquareBrace }, /* SRVSVCLexNewline  */
    {     SRVSVCLexEnd,     Skip,  SRVSVCCfgLeftSquareBrace }, /* SRVSVCLexOther    */
    {     SRVSVCLexEnd,     Skip,  SRVSVCCfgLeftSquareBrace }  /* SRVSVCLexEOF      */
    },
    /* SRVSVCLexRSqBrace := 3 */
    {
    {   SRVSVCLexBegin,  Consume,             SRVSVCCfgNone }, /* SRVSVCLexBegin    */
    {     SRVSVCLexEnd, Pushback, SRVSVCCfgRightSquareBrace }, /* SRVSVCLexChar     */
    {     SRVSVCLexEnd, Pushback, SRVSVCCfgRightSquareBrace }, /* SRVSVCLexLSqBrace */
    {     SRVSVCLexEnd, Pushback, SRVSVCCfgRightSquareBrace }, /* SRVSVCLexRSqBrace */
    {     SRVSVCLexEnd, Pushback, SRVSVCCfgRightSquareBrace }, /* SRVSVCLexEquals   */
    {     SRVSVCLexEnd, Pushback, SRVSVCCfgRightSquareBrace }, /* SRVSVCLexHash     */
    {     SRVSVCLexEnd, Pushback, SRVSVCCfgRightSquareBrace }, /* SRVSVCLexNewline  */
    {     SRVSVCLexEnd,     Skip, SRVSVCCfgRightSquareBrace }, /* SRVSVCLexOther    */
    {     SRVSVCLexEnd,     Skip, SRVSVCCfgRightSquareBrace }  /* SRVSVCLexEOF      */
    },
    /* SRVSVCLexEquals   := 4 */
    {
    {   SRVSVCLexBegin,  Consume,             SRVSVCCfgNone }, /* SRVSVCLexBegin    */
    {     SRVSVCLexEnd, Pushback,           SRVSVCCfgEquals }, /* SRVSVCLexChar     */
    {     SRVSVCLexEnd, Pushback,           SRVSVCCfgEquals }, /* SRVSVCLexLSqBrace */
    {     SRVSVCLexEnd, Pushback,           SRVSVCCfgEquals }, /* SRVSVCLexRSqBrace */
    {     SRVSVCLexEnd, Pushback,           SRVSVCCfgEquals }, /* SRVSVCLexEquals   */
    {     SRVSVCLexEnd, Pushback,           SRVSVCCfgEquals }, /* SRVSVCLexHash     */
    {     SRVSVCLexEnd, Pushback,           SRVSVCCfgEquals }, /* SRVSVCLexNewline  */
    {     SRVSVCLexEnd,     Skip,           SRVSVCCfgEquals }, /* SRVSVCLexOther    */
    {     SRVSVCLexEnd,     Skip,           SRVSVCCfgEquals }  /* SRVSVCLexEOF      */
    },
    /* SRVSVCLexHash     := 5 */
    {
    {   SRVSVCLexBegin,  Consume,             SRVSVCCfgNone }, /* SRVSVCLexBegin    */
    {     SRVSVCLexEnd, Pushback,             SRVSVCCfgHash }, /* SRVSVCLexChar     */
    {     SRVSVCLexEnd, Pushback,             SRVSVCCfgHash }, /* SRVSVCLexLSqBrace */
    {     SRVSVCLexEnd, Pushback,             SRVSVCCfgHash }, /* SRVSVCLexRSqBrace */
    {     SRVSVCLexEnd, Pushback,             SRVSVCCfgHash }, /* SRVSVCLexEquals   */
    {     SRVSVCLexEnd, Pushback,             SRVSVCCfgHash }, /* SRVSVCLexHash     */
    {     SRVSVCLexEnd, Pushback,             SRVSVCCfgHash }, /* SRVSVCLexNewline  */
    {     SRVSVCLexEnd,     Skip,             SRVSVCCfgHash }, /* SRVSVCLexOther    */
    {     SRVSVCLexEnd,     Skip,             SRVSVCCfgHash }  /* SRVSVCLexEOF      */
    },
    /* SRVSVCLexNewline  := 6 */
    {
    {   SRVSVCLexBegin,  Consume,             SRVSVCCfgNone }, /* SRVSVCLexBegin    */
    {     SRVSVCLexEnd, Pushback,          SRVSVCCfgNewline }, /* SRVSVCLexChar     */
    {     SRVSVCLexEnd, Pushback,          SRVSVCCfgNewline }, /* SRVSVCLexLSqBrace */
    {     SRVSVCLexEnd, Pushback,          SRVSVCCfgNewline }, /* SRVSVCLexRSqBrace */
    {     SRVSVCLexEnd, Pushback,          SRVSVCCfgNewline }, /* SRVSVCLexEquals   */
    {     SRVSVCLexEnd, Pushback,          SRVSVCCfgNewline }, /* SRVSVCLexHash     */
    {     SRVSVCLexEnd, Pushback,          SRVSVCCfgNewline }, /* SRVSVCLexNewline  */
    {     SRVSVCLexEnd,     Skip,          SRVSVCCfgNewline }, /* SRVSVCLexOther    */
    {     SRVSVCLexEnd,     Skip,          SRVSVCCfgNewline }  /* SRVSVCLexEOF      */
    },
    /* SRVSVCLexOther    := 7 */
    {
    {   SRVSVCLexBegin,  Consume,             SRVSVCCfgNone }, /* SRVSVCLexBegin    */
    {    SRVSVCLexChar,  Consume,             SRVSVCCfgNone }, /* SRVSVCLexChar     */
    {SRVSVCLexLSqBrace,  Consume,             SRVSVCCfgNone }, /* SRVSVCLexLSqBrace */
    {SRVSVCLexRSqBrace,  Consume,             SRVSVCCfgNone }, /* SRVSVCLexRSqBrace */
    {  SRVSVCLexEquals,  Consume,             SRVSVCCfgNone }, /* SRVSVCLexEquals   */
    {    SRVSVCLexHash,  Consume,             SRVSVCCfgNone }, /* SRVSVCLexHash     */
    { SRVSVCLexNewline,  Consume,             SRVSVCCfgNone }, /* SRVSVCLexNewline  */
    {   SRVSVCLexOther,  Consume,             SRVSVCCfgNone }, /* SRVSVCLexOther    */
    {     SRVSVCLexEOF,  Consume,             SRVSVCCfgNone }  /* SRVSVCLexEOF      */
    },
    /* SRVSVCLexEOF      := 8 */
    {
    {   SRVSVCLexBegin,  Consume,             SRVSVCCfgNone }, /* SRVSVCLexBegin    */
    {     SRVSVCLexEnd,  Consume,             SRVSVCCfgNone }, /* SRVSVCLexChar     */
    {     SRVSVCLexEnd,  Consume,             SRVSVCCfgNone }, /* SRVSVCLexLSqBrace */
    {     SRVSVCLexEnd,  Consume,             SRVSVCCfgNone }, /* SRVSVCLexRSqBrace */
    {     SRVSVCLexEnd,  Consume,             SRVSVCCfgNone }, /* SRVSVCLexEquals   */
    {     SRVSVCLexEnd,  Consume,             SRVSVCCfgNone }, /* SRVSVCLexHash     */
    { SRVSVCLexNewline,  Consume,             SRVSVCCfgNone }, /* SRVSVCLexNewline  */
    {     SRVSVCLexEnd,  Consume,             SRVSVCCfgNone }, /* SRVSVCLexOther    */
    {     SRVSVCLexEnd,  Consume,             SRVSVCCfgNone }  /* SRVSVCLexEOF      */
    }
};

DWORD
SRVSVCParseConfigFile(
    PCSTR                     pszFilePath,
    PFNCONFIG_START_SECTION   pfnStartSectionHandler,
    PFNCONFIG_COMMENT         pfnCommentHandler,
    PFNCONFIG_NAME_VALUE_PAIR pfnNameValuePairHandler,
    PFNCONFIG_END_SECTION     pfnEndSectionHandler
    )
{
    DWORD dwError = 0;
    PSRVSVC_CONFIG_PARSE_STATE pParseState = NULL;

    dwError = SRVSVCCfgInitParseState(
                    pszFilePath,
                    pfnStartSectionHandler,
                    pfnCommentHandler,
                    pfnNameValuePairHandler,
                    pfnEndSectionHandler,
                    &pParseState);
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwError = SRVSVCCfgParse(pParseState);
    BAIL_ON_SRVSVC_ERROR(dwError);

cleanup:

    if (pParseState) {
        SRVSVCCfgFreeParseState(pParseState);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
SRVSVCCfgInitParseState(
    PCSTR                     pszFilePath,
    PFNCONFIG_START_SECTION   pfnStartSectionHandler,
    PFNCONFIG_COMMENT         pfnCommentHandler,
    PFNCONFIG_NAME_VALUE_PAIR pfnNameValuePairHandler,
    PFNCONFIG_END_SECTION     pfnEndSectionHandler,
    PSRVSVC_CONFIG_PARSE_STATE*  ppParseState
    )
{
    DWORD dwError = 0;
    PSRVSVC_CONFIG_PARSE_STATE pParseState = NULL;
    PSRVSVC_STACK pTokenStack = NULL;
    FILE* fp = NULL;

    if ((fp = fopen(pszFilePath, "r")) == NULL) {
        dwError = errno;
        BAIL_ON_SRVSVC_ERROR(dwError);
    }


    dwError = SRVSVCAllocateMemory(
                    sizeof(SRVSVC_CONFIG_PARSE_STATE),
                    (PVOID*)&pParseState);
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwError = SRVSVCAllocateMemory(
                    sizeof(SRVSVC_STACK),
                    (PVOID*)&pTokenStack);
    BAIL_ON_SRVSVC_ERROR(dwError);

    pParseState->pLexerTokenStack = pTokenStack;

    dwError = SRVSVCAllocateString(
                    pszFilePath,
                    &(pParseState->pszFilePath));
    BAIL_ON_SRVSVC_ERROR(dwError);

    pParseState->fp = fp;
    fp = NULL;

    pParseState->pfnStartSectionHandler =
        pfnStartSectionHandler;
    pParseState->pfnCommentHandler =
        pfnCommentHandler;
    pParseState->pfnNameValuePairHandler =
        pfnNameValuePairHandler;
    pParseState->pfnEndSectionHandler =
        pfnEndSectionHandler;

    pParseState->dwLine = 1;

    *ppParseState = pParseState;

cleanup:

    return dwError;

error:

    *ppParseState = NULL;

    if (pParseState) {
        SRVSVCCfgFreeParseState(pParseState);
    }

    if (fp) {
        fclose(fp);
    }

    goto cleanup;
}

VOID
SRVSVCCfgFreeParseState(
    PSRVSVC_CONFIG_PARSE_STATE pParseState
    )
{
    SRVSVC_SAFE_FREE_STRING(pParseState->pszFilePath);
    if (pParseState->fp) {
        fclose(pParseState->fp);
    }
    SRVSVCFreeMemory(pParseState);
}


DWORD
SRVSVCCfgParse(
    PSRVSVC_CONFIG_PARSE_STATE pParseState
    )
{

    DWORD dwError = 0;
    PSRVSVC_CFG_TOKEN pToken = NULL;
    BOOLEAN bContinue = TRUE;
    PSRVSVC_STACK pTokenStack = NULL;

    do
    {

        dwError = SRVSVCCfgGetNextToken(
                        pParseState,
                        &pToken);
        BAIL_ON_SRVSVC_ERROR(dwError);

        switch (pToken->tokenType)
        {
            case SRVSVCCfgHash:
            {
                dwError = SRVSVCCfgParseComment(
                                pParseState,
                                &bContinue);
                BAIL_ON_SRVSVC_ERROR(dwError);

                break;
            }
            case SRVSVCCfgNewline:
            {
                //in case of a blank line (newline),
                //interpret this as an empty comment.
                dwError = SRVSVCCfgProcessComment(
                                pParseState,
                                &pTokenStack,
                                &bContinue);
                BAIL_ON_SRVSVC_ERROR(dwError);

                break;
            }
            case SRVSVCCfgLeftSquareBrace:
            {

                dwError = SRVSVCCfgParseSections(
                                pParseState);
                BAIL_ON_SRVSVC_ERROR(dwError);

                break;
            }
            case SRVSVCCfgEOF:
            {
                bContinue = FALSE;
                break;
            }
            default:
            {
                dwError = SRVSVC_ERROR_INVALID_CONFIG;
                BAIL_ON_SRVSVC_ERROR(dwError);
            }
        }

    } while (bContinue);

cleanup:

    if (pToken) {
        SRVSVCCfgFreeToken(pToken);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
SRVSVCCfgParseSections(
    PSRVSVC_CONFIG_PARSE_STATE pParseState
    )
{
    DWORD dwError = 0;
    PSRVSVC_CFG_TOKEN pToken = NULL;
    BOOLEAN bContinue = TRUE;
    PSRVSVC_STACK pTokenStack = NULL;

    dwError = SRVSVCCfgParseSectionHeader(
                    pParseState,
                    &bContinue);
    BAIL_ON_SRVSVC_ERROR(dwError);

    while (bContinue)
    {
        dwError = SRVSVCCfgGetNextToken(
                        pParseState,
                        &pToken);
        BAIL_ON_SRVSVC_ERROR(dwError);

        switch (pToken->tokenType)
        {
            case SRVSVCCfgString:
            {

	      SRVSVCStripWhitespace(pToken->pszToken, TRUE, TRUE);

                if(!IsNullOrEmptyString(pToken->pszToken))
		{

                    dwError = SRVSVCStackPush(pToken, &(pParseState->pLexerTokenStack));
                    BAIL_ON_SRVSVC_ERROR(dwError);

                    pToken = NULL;

                    dwError = SRVSVCCfgParseNameValuePair(
                                    pParseState,
                                    &bContinue);
                    BAIL_ON_SRVSVC_ERROR(dwError);
		}
                break;
            }

            case SRVSVCCfgHash:
            {
                dwError = SRVSVCCfgParseComment(
                                pParseState,
                                &bContinue);
                BAIL_ON_SRVSVC_ERROR(dwError);
                break;
            }
            case SRVSVCCfgNewline:
            {
                //in case of a blank line (newline),
                //interpret this as an empty comment.
                dwError = SRVSVCCfgProcessComment(
                                pParseState,
                                &pTokenStack,
                                &bContinue);
                BAIL_ON_SRVSVC_ERROR(dwError);
                break;
            }
            case SRVSVCCfgLeftSquareBrace:
            {
                dwError = SRVSVCCfgParseSectionHeader(
                                pParseState,
                                &bContinue);
                BAIL_ON_SRVSVC_ERROR(dwError);

                break;
            }
            case SRVSVCCfgEOF:
            {
                bContinue = FALSE;
                break;
            }
            default:
            {
                dwError = SRVSVC_ERROR_INVALID_CONFIG;
                BAIL_ON_SRVSVC_ERROR(dwError);
            }
        }
    }

    if (bContinue && !IsNullOrEmptyString(pParseState->pszSectionName))
    {
        dwError = SRVSVCCfgProcessEndSection(
                        pParseState,
                        &bContinue);
        BAIL_ON_SRVSVC_ERROR(dwError);
    }

cleanup:

    if (pToken) {
        SRVSVCCfgFreeToken(pToken);
    }

    return dwError;

error:

    goto cleanup;
}



DWORD
SRVSVCCfgParseComment(
    PSRVSVC_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN    pbContinue
    )
{
    DWORD dwError = 0;
    BOOLEAN bContinue = TRUE;
    BOOLEAN bKeepParsing = TRUE;
    PSRVSVC_CFG_TOKEN pToken = NULL;
    PSRVSVC_STACK pTokenStack = NULL;

    do
    {
        dwError = SRVSVCCfgGetNextToken(
                    pParseState,
                    &pToken);
        BAIL_ON_SRVSVC_ERROR(dwError);

        switch (pToken->tokenType)
        {
            case SRVSVCCfgEOF:
            {
                dwError = SRVSVCCfgProcessComment(
                                pParseState,
                                &pTokenStack,
                                &bContinue);
                BAIL_ON_SRVSVC_ERROR(dwError);

                bContinue = FALSE;

                break;
            }
            case SRVSVCCfgNewline:
            {
                dwError = SRVSVCCfgProcessComment(
                                pParseState,
                                &pTokenStack,
                                &bContinue);
                BAIL_ON_SRVSVC_ERROR(dwError);

                bKeepParsing = FALSE;

                break;
            }
            default:
            {
                dwError = SRVSVCStackPush(pToken, &pTokenStack);
                BAIL_ON_SRVSVC_ERROR(dwError);

                pToken = NULL;

            }
        }

    } while (bContinue && bKeepParsing);

    *pbContinue = bContinue;

cleanup:

    if (pToken) {
        SRVSVCCfgFreeToken(pToken);
    }

    return dwError;

error:

    *pbContinue = FALSE;

    goto cleanup;
}

DWORD
SRVSVCCfgParseSectionHeader(
    PSRVSVC_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    BOOLEAN bKeepParsing = TRUE;
    BOOLEAN bContinue = TRUE;
    PSRVSVC_CFG_TOKEN pToken = NULL;
    PSRVSVC_STACK pTokenStack = NULL;

    if (!IsNullOrEmptyString(pParseState->pszSectionName))
    {
        dwError = SRVSVCCfgProcessEndSection(
                        pParseState,
                        &bContinue);
        BAIL_ON_SRVSVC_ERROR(dwError);

    }

    if (!bContinue)
    {
        goto done;
    }

    pParseState->bSkipSection = FALSE;

    do
    {
        dwError = SRVSVCCfgGetNextToken(
                        pParseState,
                        &pToken);
        BAIL_ON_SRVSVC_ERROR(dwError);

        switch(pToken->tokenType)
        {
            case SRVSVCCfgString:
            case SRVSVCCfgEquals:
            case SRVSVCCfgOther:
            {
                dwError = SRVSVCStackPush(pToken, &pTokenStack);
                BAIL_ON_SRVSVC_ERROR(dwError);

                pToken = NULL;
                break;
            }
            case SRVSVCCfgRightSquareBrace:
            {
                dwError = SRVSVCAssertWhitespaceOnly(pParseState);
                BAIL_ON_SRVSVC_ERROR(dwError);

                bKeepParsing = FALSE;

                break;
            }
            default:
            {
                dwError = SRVSVC_ERROR_INVALID_CONFIG;
                BAIL_ON_SRVSVC_ERROR(dwError);
            }
        }

    } while (bKeepParsing);

    dwError = SRVSVCCfgGetNextToken(
                    pParseState,
                    &pToken);
    BAIL_ON_SRVSVC_ERROR(dwError);

    switch(pToken->tokenType)
    {

        case SRVSVCCfgNewline:
        {
            dwError = SRVSVCCfgProcessBeginSection(
                            pParseState,
                            &pTokenStack,
                            &bContinue);
            BAIL_ON_SRVSVC_ERROR(dwError);

            break;
        }
        case SRVSVCCfgEOF:
        {
            dwError = SRVSVCCfgProcessBeginSection(
                            pParseState,
                            &pTokenStack,
                            &bContinue);
            BAIL_ON_SRVSVC_ERROR(dwError);

            if (bContinue) {

                dwError = SRVSVCCfgProcessEndSection(
                                pParseState,
                                &bContinue);
                BAIL_ON_SRVSVC_ERROR(dwError);
            }

            bContinue = FALSE;

            break;
        }
        default:
        {
            dwError = SRVSVC_ERROR_INVALID_CONFIG;
            BAIL_ON_SRVSVC_ERROR(dwError);
        }
    }

done:

    *pbContinue = bContinue;

cleanup:

    if (pToken) {
        SRVSVCCfgFreeToken(pToken);
    }

    return dwError;

error:

    *pbContinue = FALSE;

    goto cleanup;
}

DWORD
SRVSVCAssertWhitespaceOnly(
    PSRVSVC_CONFIG_PARSE_STATE pParseState
    )
{
    DWORD dwError = 0;
    PSRVSVC_CFG_TOKEN pToken = NULL;
    BOOLEAN bKeepParsing = TRUE;

    do
    {
        dwError = SRVSVCCfgGetNextToken(
                        pParseState,
                        &pToken);
        BAIL_ON_SRVSVC_ERROR(dwError);

        switch(pToken->tokenType)
        {
            case SRVSVCCfgString:
            case SRVSVCCfgOther:
            {
                DWORD i = 0;
                for (; i < pToken->dwLen; i++) {
                    if (!isspace((int)pToken->pszToken[i])) {
                        dwError = SRVSVC_ERROR_INVALID_CONFIG;
                        BAIL_ON_SRVSVC_ERROR(dwError);
                    }
                }
                break;
            }
            case SRVSVCCfgEOF:
            case SRVSVCCfgNewline:
            {
                dwError = SRVSVCStackPush(pToken, &pParseState->pLexerTokenStack);
                BAIL_ON_SRVSVC_ERROR(dwError);

                pToken = NULL;

                bKeepParsing = FALSE;

                break;
            }
            default:
            {
                dwError = SRVSVC_ERROR_INVALID_CONFIG;
                BAIL_ON_SRVSVC_ERROR(dwError);
            }
        }
    } while (bKeepParsing);

cleanup:

    if (pToken) {
        SRVSVCCfgFreeToken(pToken);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
SRVSVCCfgParseNameValuePair(
    PSRVSVC_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    BOOLEAN bContinue = TRUE;
    BOOLEAN bKeepParsing = TRUE;
    PSRVSVC_CFG_TOKEN pToken = NULL;
    PSRVSVC_STACK pTokenStack = NULL;

    //format is <str><equals><token1><token2>...<newline>

    //get initial <str>
    dwError = SRVSVCCfgGetNextToken(
                    pParseState,
                    &pToken);
    BAIL_ON_SRVSVC_ERROR(dwError);

    if (pToken->tokenType == SRVSVCCfgString)
    {
        dwError = SRVSVCStackPush(pToken, &pTokenStack);
        BAIL_ON_SRVSVC_ERROR(dwError);
        pToken = NULL;
    }
    else
    {
        dwError = SRVSVC_ERROR_INVALID_CONFIG;
        BAIL_ON_SRVSVC_ERROR(dwError);
    }

    //get <equals>
    dwError = SRVSVCCfgGetNextToken(
                    pParseState,
                    &pToken);
    BAIL_ON_SRVSVC_ERROR(dwError);

    if (pToken->tokenType == SRVSVCCfgEquals)
    {
        dwError = SRVSVCStackPush(pToken, &pTokenStack);
        BAIL_ON_SRVSVC_ERROR(dwError);
        pToken = NULL;
    }
    else
    {
        dwError = SRVSVC_ERROR_INVALID_CONFIG;
        BAIL_ON_SRVSVC_ERROR(dwError);
    }


    do
    {
        dwError = SRVSVCCfgGetNextToken(
                        pParseState,
                        &pToken);
        BAIL_ON_SRVSVC_ERROR(dwError);

        switch(pToken->tokenType)
        {
            case SRVSVCCfgString:
            case SRVSVCCfgEquals:
            case SRVSVCCfgOther:
            {

                dwError = SRVSVCStackPush(pToken, &pTokenStack);
                BAIL_ON_SRVSVC_ERROR(dwError);
                pToken = NULL;

                break;

            }
            case SRVSVCCfgNewline:
            {
                dwError = SRVSVCCfgProcessNameValuePair(
                    pParseState,
                    &pTokenStack,
                    &bContinue);
                BAIL_ON_SRVSVC_ERROR(dwError);
                bKeepParsing = FALSE;
                break;
            }
            case SRVSVCCfgEOF:
            {
                dwError = SRVSVCCfgProcessNameValuePair(
                    pParseState,
                    &pTokenStack,
                    &bContinue);
                BAIL_ON_SRVSVC_ERROR(dwError);
                bContinue = FALSE;
                break;
            }
            default:
            {
                dwError = SRVSVC_ERROR_INVALID_CONFIG;
                BAIL_ON_SRVSVC_ERROR(dwError);
            }
        }
    } while (bContinue && bKeepParsing);


    *pbContinue = bContinue;

cleanup:

    if (pToken) {
        SRVSVCCfgFreeToken(pToken);
    }

    return dwError;

error:

    *pbContinue = FALSE;

    goto cleanup;
}

DWORD
SRVSVCCfgProcessComment(
    PSRVSVC_CONFIG_PARSE_STATE pParseState,
    PSRVSVC_STACK* ppTokenStack,
    PBOOLEAN    pbContinue
    )
{
    DWORD   dwError = 0;
    BOOLEAN bContinue = TRUE;
    PSTR    pszComment = NULL;

    dwError = SRVSVCCfgProcessTokenStackIntoString(
        ppTokenStack,
        &pszComment);
    BAIL_ON_SRVSVC_ERROR(dwError);

    if (pParseState->pfnCommentHandler &&
        !pParseState->bSkipSection) {

        dwError = pParseState->pfnCommentHandler(
                        pszComment,
                        &bContinue);
        BAIL_ON_SRVSVC_ERROR(dwError);
    }

    *pbContinue = bContinue;

cleanup:

    SRVSVC_SAFE_FREE_STRING(pszComment);

    return dwError;

error:

    *pbContinue = TRUE;

    goto cleanup;
}

DWORD
SRVSVCCfgProcessBeginSection(
    PSRVSVC_CONFIG_PARSE_STATE pParseState,
    PSRVSVC_STACK* ppTokenStack,
    PBOOLEAN    pbContinue)
{
    DWORD   dwError = 0;
    PSTR    pszSectionName = NULL;
    BOOLEAN bSkipSection = FALSE;
    BOOLEAN bContinue = TRUE;

    dwError = SRVSVCCfgProcessTokenStackIntoString(
        ppTokenStack,
        &pszSectionName);
    BAIL_ON_SRVSVC_ERROR(dwError);

    if (IsNullOrEmptyString(pszSectionName)) {
        dwError = SRVSVC_ERROR_INVALID_CONFIG;
        BAIL_ON_SRVSVC_ERROR(dwError);
    }

    if (pParseState->pfnStartSectionHandler) {
        dwError = pParseState->pfnStartSectionHandler(
                        pszSectionName,
                        &bSkipSection,
                        &bContinue);
        BAIL_ON_SRVSVC_ERROR(dwError);
    }

    pParseState->pszSectionName = pszSectionName;
    pParseState->bSkipSection = bSkipSection;
    *pbContinue = bContinue;

cleanup:

    return dwError;

error:

    SRVSVC_SAFE_FREE_STRING(pszSectionName);
    pParseState->pszSectionName = NULL;
    pParseState->bSkipSection = FALSE;
    *pbContinue = FALSE;

    goto cleanup;
}

DWORD
SRVSVCCfgProcessNameValuePair(
    PSRVSVC_CONFIG_PARSE_STATE pParseState,
    PSRVSVC_STACK*             ppTokenStack,
    PBOOLEAN                pbContinue
    )
{
    DWORD      dwError = 0;
    BOOLEAN    bContinue = TRUE;
    PSTR       pszName = NULL;
    PSTR       pszValue = NULL;
    PSRVSVC_CFG_TOKEN pToken = NULL;

    *ppTokenStack = SRVSVCStackReverse(*ppTokenStack);
    pToken = (PSRVSVC_CFG_TOKEN)SRVSVCStackPop(ppTokenStack);
    if (pToken && pToken->dwLen) {
        dwError = SRVSVCStrndup(
                    pToken->pszToken,
                    pToken->dwLen,
                    &pszName);
        BAIL_ON_SRVSVC_ERROR(dwError);
    }

    if (IsNullOrEmptyString(pszName)) {
        dwError = SRVSVC_ERROR_INVALID_CONFIG;
        BAIL_ON_SRVSVC_ERROR(dwError);
    }

    SRVSVCCfgFreeToken(pToken);
    pToken = NULL;

    pToken = (PSRVSVC_CFG_TOKEN)SRVSVCStackPop(ppTokenStack);
    if (!pToken || pToken->tokenType != SRVSVCCfgEquals)
    {
        dwError = SRVSVC_ERROR_INVALID_CONFIG;
        BAIL_ON_SRVSVC_ERROR(dwError);
    }

    SRVSVCCfgFreeToken(pToken);
    pToken = NULL;

    //this will consume the token stack
    dwError = SRVSVCCfgProcessTokenStackIntoString(
        ppTokenStack,
        &pszValue);
    BAIL_ON_SRVSVC_ERROR(dwError);

    if (pParseState->pfnNameValuePairHandler  &&
        !pParseState->bSkipSection) {

        dwError = pParseState->pfnNameValuePairHandler(
                        pszName,
                        pszValue,
                        &bContinue);
        BAIL_ON_SRVSVC_ERROR(dwError);

    }

    *pbContinue = bContinue;

cleanup:

    if (pToken) {
        SRVSVCCfgFreeToken(pToken);
        pToken = NULL;
    }

    if (ppTokenStack && *ppTokenStack)
    {
        dwError = SRVSVCCfgFreeTokenStack(ppTokenStack);
    }

    SRVSVC_SAFE_FREE_STRING(pszName);
    SRVSVC_SAFE_FREE_STRING(pszValue);

    return dwError;

error:

    goto cleanup;
}

DWORD
SRVSVCCfgProcessEndSection(
    PSRVSVC_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN                pbContinue
    )
{
    DWORD dwError = 0;
    BOOLEAN bContinue = TRUE;

    if (pParseState->pfnEndSectionHandler &&
        !pParseState->bSkipSection) {
        dwError = pParseState->pfnEndSectionHandler(
                        pParseState->pszSectionName,
                        &bContinue);
        BAIL_ON_SRVSVC_ERROR(dwError);
    }

    *pbContinue = bContinue;

cleanup:

    SRVSVC_SAFE_FREE_STRING(pParseState->pszSectionName);

    return dwError;

error:

    *pbContinue = TRUE;

    goto cleanup;
}

DWORD
SRVSVCCfgDetermineTokenLength(
    PSRVSVC_STACK pStack
    )
{
    DWORD dwLen = 0;
    PSRVSVC_STACK pIter = pStack;

    for (; pIter; pIter = pIter->pNext)
    {
        PSRVSVC_CFG_TOKEN pToken = (PSRVSVC_CFG_TOKEN)pIter->pItem;
        if (pToken) {
            dwLen += pToken->dwLen;
        }
    }

    return dwLen;
}

//this will consume the token stack
DWORD
SRVSVCCfgProcessTokenStackIntoString(
    PSRVSVC_STACK* ppTokenStack,
    PSTR* ppszConcatenated
    )
{
    DWORD   dwError = 0;
    DWORD   dwRequiredTokenLen = 0;
    PSTR    pszConcatenated = NULL;

    dwRequiredTokenLen = SRVSVCCfgDetermineTokenLength(*ppTokenStack);

    if (dwRequiredTokenLen) {

        PSTR pszPos = NULL;
        PSRVSVC_CFG_TOKEN pToken = NULL;

        *ppTokenStack = SRVSVCStackReverse(*ppTokenStack);


        dwError = SRVSVCAllocateMemory(
                        (dwRequiredTokenLen + 1) * sizeof(CHAR),
                        (PVOID*)&pszConcatenated);
        BAIL_ON_SRVSVC_ERROR(dwError);


        pszPos = pszConcatenated;
        while (*ppTokenStack)
        {
            pToken = SRVSVCStackPop(ppTokenStack);
            if (pToken && pToken->dwLen) {

                strncpy(pszPos, pToken->pszToken, pToken->dwLen);
                pszPos += pToken->dwLen;

                SRVSVCCfgFreeToken(pToken);
                pToken = NULL;
            }
        }
    }

    *ppszConcatenated = pszConcatenated;

cleanup:

    return dwError;

error:

    SRVSVC_SAFE_FREE_STRING(pszConcatenated);

    *ppszConcatenated = NULL;

    goto cleanup;
}


DWORD
SRVSVCCfgAllocateToken(
    DWORD           dwSize,
    PSRVSVC_CFG_TOKEN* ppToken
    )
{
    DWORD dwError = 0;
    PSRVSVC_CFG_TOKEN pToken = NULL;
    DWORD dwMaxLen = (dwSize > 0 ? dwSize : SRVSVC_CFG_TOKEN_DEFAULT_LENGTH);


    dwError = SRVSVCAllocateMemory(
                    sizeof(SRVSVC_CFG_TOKEN),
                    (PVOID*)&pToken);
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwError = SRVSVCAllocateMemory(
                    dwMaxLen * sizeof(CHAR),
                    (PVOID*)&pToken->pszToken);
    BAIL_ON_SRVSVC_ERROR(dwError);


    pToken->tokenType = SRVSVCCfgNone;
    pToken->dwMaxLen = dwMaxLen;

    *ppToken = pToken;

cleanup:

    return dwError;

error:

    *ppToken = NULL;

    if (pToken) {
        SRVSVCCfgFreeToken(pToken);
    }

    goto cleanup;
}

DWORD
SRVSVCCfgReallocToken(
    PSRVSVC_CFG_TOKEN pToken,
    DWORD          dwNewSize
    )
{
    DWORD dwError = 0;

    dwError = SRVSVCReallocMemory(
                    pToken->pszToken,
                    (PVOID*)&pToken->pszToken,
                    dwNewSize);
    BAIL_ON_SRVSVC_ERROR(dwError);

    pToken->dwMaxLen = dwNewSize;

cleanup:

    return dwError;

error:

    goto cleanup;
}

VOID
SRVSVCCfgResetToken(
    PSRVSVC_CFG_TOKEN pToken
    )
{
    if (pToken && pToken->pszToken && pToken->dwMaxLen)
    {
        memset(pToken->pszToken, 0, pToken->dwMaxLen);
        pToken->dwLen = 0;
    }
}

DWORD
SRVSVCCfgCopyToken(
    PSRVSVC_CFG_TOKEN pTokenSrc,
    PSRVSVC_CFG_TOKEN pTokenDst
    )
{
    DWORD dwError = 0;

    pTokenDst->tokenType = pTokenSrc->tokenType;

    if (pTokenSrc->dwLen > pTokenDst->dwLen) {
        dwError = SRVSVCReallocMemory(
                        (PVOID)  pTokenDst->pszToken,
                        (PVOID*) &pTokenDst->pszToken,
                        (DWORD)  pTokenSrc->dwLen);
        BAIL_ON_SRVSVC_ERROR(dwError);

        pTokenDst->dwLen = pTokenSrc->dwLen;
        pTokenDst->dwMaxLen = pTokenDst->dwLen;
    }

    memset(pTokenDst->pszToken, 0, pTokenDst->dwLen);
    memcpy(pTokenDst->pszToken, pTokenSrc->pszToken, pTokenSrc->dwLen);

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
SRVSVCCfgFreeTokenStack(
    PSRVSVC_STACK* ppTokenStack
    )
{

    DWORD dwError = 0;

    PSRVSVC_STACK pTokenStack = *ppTokenStack;

    dwError = SRVSVCStackForeach(
            pTokenStack,
            &SRVSVCCfgFreeTokenInStack,
            NULL);
    BAIL_ON_SRVSVC_ERROR(dwError);

    SRVSVCStackFree(pTokenStack);

    *ppTokenStack = NULL;

error:

    return dwError;
}

DWORD
SRVSVCCfgFreeTokenInStack(
    PVOID pToken,
    PVOID pUserData
    )
{
    if (pToken)
    {
        SRVSVCCfgFreeToken((PSRVSVC_CFG_TOKEN)pToken);
    }

    return 0;
}

VOID
SRVSVCCfgFreeToken(
    PSRVSVC_CFG_TOKEN pToken
    )
{
    SRVSVC_SAFE_FREE_MEMORY(pToken->pszToken);
    SRVSVCFreeMemory(pToken);
}

DWORD
SRVSVCCfgGetNextToken(
    PSRVSVC_CONFIG_PARSE_STATE pParseState,
    PSRVSVC_CFG_TOKEN*         ppToken
    )
{
    DWORD dwError = 0;
    SRVSVCCfgTokenType tokenType = SRVSVCCfgNone;
    SRVSVCCfgLexState  curLexState = SRVSVCLexBegin;
    PSRVSVC_CFG_TOKEN  pToken = NULL;
    BOOLEAN         bOwnToken = FALSE;

    if (SRVSVCStackPeek(pParseState->pLexerTokenStack) != NULL)
    {
        PSRVSVC_CFG_TOKEN pToken_input = *ppToken;

        pToken = (PSRVSVC_CFG_TOKEN)SRVSVCStackPop(&pParseState->pLexerTokenStack);
        bOwnToken = TRUE;

        if (pToken_input) {

            dwError = SRVSVCCfgCopyToken(pToken, pToken_input);
            BAIL_ON_SRVSVC_ERROR(dwError);

            SRVSVCCfgFreeToken(pToken);
            pToken = NULL;
            bOwnToken = FALSE;

        }

        goto done;
    }

    pToken = *ppToken;

    if (!pToken) {
        dwError = SRVSVCCfgAllocateToken(
                        0,
                        &pToken);
        BAIL_ON_SRVSVC_ERROR(dwError);

        bOwnToken = TRUE;
    }
    else
    {
        SRVSVCCfgResetToken(pToken);
    }

    while (curLexState != SRVSVCLexEnd)
    {
        DWORD ch = SRVSVCCfgGetCharacter(pParseState);
        SRVSVCCfgLexState lexClass = SRVSVCCfgGetLexClass(ch);

        if (lexClass != SRVSVCLexEOF) {
            pParseState->dwCol++;
        }

        if (ch == (DWORD)'\n') {
            pParseState->dwLine++;
            pParseState->dwCol = 0;
        }

        tokenType = SRVSVCCfgGetTokenType(curLexState, lexClass);

        switch(SRVSVCCfgGetLexAction(curLexState, lexClass))
        {
            case Skip:

                break;

            case Consume:

                if (pToken->dwLen >= pToken->dwMaxLen) {
                    dwError = SRVSVCCfgReallocToken(
                                    pToken,
                                    pToken->dwMaxLen + SRVSVC_CFG_TOKEN_DEFAULT_LENGTH);
                    BAIL_ON_SRVSVC_ERROR(dwError);
                }

                pToken->pszToken[pToken->dwLen++] = (BYTE)ch;

                break;

            case Pushback:

                pParseState->dwCol--;
                dwError = SRVSVCCfgPushBackCharacter(
                                pParseState,
                                (BYTE)ch);
                BAIL_ON_SRVSVC_ERROR(dwError);

                break;
        }

        curLexState = SRVSVCCfgGetNextLexState(curLexState, lexClass);
    }

    pToken->tokenType = tokenType;

done:

    if (bOwnToken) {
        *ppToken = pToken;
    }

cleanup:

    return dwError;

error:

    if (bOwnToken && pToken) {
        SRVSVCCfgFreeToken(pToken);
        *ppToken = NULL;
    }

    goto cleanup;
}

DWORD
SRVSVCCfgGetCharacter(
    PSRVSVC_CONFIG_PARSE_STATE pParseState
    )
{
    return getc(pParseState->fp);
}

SRVSVCCfgLexState
SRVSVCCfgGetLexClass(
    DWORD ch
    )
{

    if (ch == EOF) {
        return SRVSVCLexEOF;
    }

    if (ch == '\n') {
        return SRVSVCLexNewline;
    }

    if (ch == '[') {
        return SRVSVCLexLSqBrace;
    }

    if (ch == ']') {
        return SRVSVCLexRSqBrace;
    }

    if (ch == '=') {
        return SRVSVCLexEquals;
    }

    if (ch == '#') {
        return SRVSVCLexHash;
    }

    if (isalnum(ch) ||
        ispunct(ch) ||
        isblank(ch)) {
        return SRVSVCLexChar;
    }

    return SRVSVCLexOther;
}

DWORD
SRVSVCCfgPushBackCharacter(
    PSRVSVC_CONFIG_PARSE_STATE pParseState,
    BYTE ch
    )
{
    return ((EOF == ungetc(ch, pParseState->fp)) ? errno : 0);
}

SRVSVCCfgLexState
SRVSVCCfgGetNextLexState(
    SRVSVCCfgLexState currentState,
    DWORD chId
    )
{
    return (gSRVSVCLexStateTable[currentState][chId].nextState);
}

SRVSVCLexAction
SRVSVCCfgGetLexAction(
    SRVSVCCfgLexState currentState,
    DWORD chId
    )
{
    return (gSRVSVCLexStateTable[currentState][chId].action);
}

SRVSVCCfgTokenType
SRVSVCCfgGetTokenType(
    SRVSVCCfgLexState currentState,
    DWORD chId
    )
{
    return (gSRVSVCLexStateTable[currentState][chId].tokenId);
}
