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
 *        Likewise Security and Authentication Subsystem (EVTSS)
 *
 *        Config (INF Style) parser
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
 */
#include "includes.h"


static EVT_CFG_LEXER_STATE gEVTLexStateTable[][9] =
{
    /* EVTLexBegin    := 0 */
    {
    {   EVTLexBegin,  Consume,             EVTCfgNone }, /* EVTLexBegin    */
    {    EVTLexChar,  Consume,             EVTCfgNone }, /* EVTLexChar     */
    {     EVTLexEnd,  Consume,  EVTCfgLeftSquareBrace }, /* EVTLexLSqBrace */
    {     EVTLexEnd,  Consume, EVTCfgRightSquareBrace }, /* EVTLexRSqBrace */
    {     EVTLexEnd,  Consume,           EVTCfgEquals }, /* EVTLexEquals   */
    {     EVTLexEnd,  Consume,             EVTCfgHash }, /* EVTLexHash     */
    {     EVTLexEnd,  Consume,          EVTCfgNewline }, /* EVTLexNewline  */
    {   EVTLexBegin,     Skip,             EVTCfgNone }, /* EVTLexOther    */
    {     EVTLexEnd,  Consume,              EVTCfgEOF }  /* EVTLexEOF      */
    },
    /* EVTLexChar     := 1 */
    {
    {   EVTLexBegin,  Consume,             EVTCfgNone }, /* EVTLexBegin    */
    {    EVTLexChar,  Consume,             EVTCfgNone }, /* EVTLexChar     */
    {     EVTLexEnd, Pushback,           EVTCfgString }, /* EVTLexLSqBrace */
    {     EVTLexEnd, Pushback,           EVTCfgString }, /* EVTLexRSqBrace */
    {     EVTLexEnd, Pushback,           EVTCfgString }, /* EVTLexEquals   */
    {     EVTLexEnd, Pushback,           EVTCfgString }, /* EVTLexHash     */
    {     EVTLexEnd, Pushback,           EVTCfgString }, /* EVTLexNewline  */
    {     EVTLexEnd,     Skip,           EVTCfgString }, /* EVTLexOther    */
    {     EVTLexEnd,     Skip,           EVTCfgString }  /* EVTLexEOF      */
    },
    /* EVTLexLSqBrace := 2 */
    {
    {   EVTLexBegin,  Consume,             EVTCfgNone }, /* EVTLexBegin    */
    {     EVTLexEnd, Pushback,  EVTCfgLeftSquareBrace }, /* EVTLexChar     */
    {     EVTLexEnd, Pushback,  EVTCfgLeftSquareBrace }, /* EVTLexLSqBrace */
    {     EVTLexEnd, Pushback,  EVTCfgLeftSquareBrace }, /* EVTLexRSqBrace */
    {     EVTLexEnd, Pushback,  EVTCfgLeftSquareBrace }, /* EVTLexEquals   */
    {     EVTLexEnd, Pushback,  EVTCfgLeftSquareBrace }, /* EVTLexHash     */
    {     EVTLexEnd, Pushback,  EVTCfgLeftSquareBrace }, /* EVTLexNewline  */
    {     EVTLexEnd,     Skip,  EVTCfgLeftSquareBrace }, /* EVTLexOther    */
    {     EVTLexEnd,     Skip,  EVTCfgLeftSquareBrace }  /* EVTLexEOF      */
    },
    /* EVTLexRSqBrace := 3 */
    {
    {   EVTLexBegin,  Consume,             EVTCfgNone }, /* EVTLexBegin    */
    {     EVTLexEnd, Pushback, EVTCfgRightSquareBrace }, /* EVTLexChar     */
    {     EVTLexEnd, Pushback, EVTCfgRightSquareBrace }, /* EVTLexLSqBrace */
    {     EVTLexEnd, Pushback, EVTCfgRightSquareBrace }, /* EVTLexRSqBrace */
    {     EVTLexEnd, Pushback, EVTCfgRightSquareBrace }, /* EVTLexEquals   */
    {     EVTLexEnd, Pushback, EVTCfgRightSquareBrace }, /* EVTLexHash     */
    {     EVTLexEnd, Pushback, EVTCfgRightSquareBrace }, /* EVTLexNewline  */
    {     EVTLexEnd,     Skip, EVTCfgRightSquareBrace }, /* EVTLexOther    */
    {     EVTLexEnd,     Skip, EVTCfgRightSquareBrace }  /* EVTLexEOF      */
    },
    /* EVTLexEquals   := 4 */
    {
    {   EVTLexBegin,  Consume,             EVTCfgNone }, /* EVTLexBegin    */
    {     EVTLexEnd, Pushback,           EVTCfgEquals }, /* EVTLexChar     */
    {     EVTLexEnd, Pushback,           EVTCfgEquals }, /* EVTLexLSqBrace */
    {     EVTLexEnd, Pushback,           EVTCfgEquals }, /* EVTLexRSqBrace */
    {     EVTLexEnd, Pushback,           EVTCfgEquals }, /* EVTLexEquals   */
    {     EVTLexEnd, Pushback,           EVTCfgEquals }, /* EVTLexHash     */
    {     EVTLexEnd, Pushback,           EVTCfgEquals }, /* EVTLexNewline  */
    {     EVTLexEnd,     Skip,           EVTCfgEquals }, /* EVTLexOther    */
    {     EVTLexEnd,     Skip,           EVTCfgEquals }  /* EVTLexEOF      */
    },
    /* EVTLexHash     := 5 */
    {
    {   EVTLexBegin,  Consume,             EVTCfgNone }, /* EVTLexBegin    */
    {     EVTLexEnd, Pushback,             EVTCfgHash }, /* EVTLexChar     */
    {     EVTLexEnd, Pushback,             EVTCfgHash }, /* EVTLexLSqBrace */
    {     EVTLexEnd, Pushback,             EVTCfgHash }, /* EVTLexRSqBrace */
    {     EVTLexEnd, Pushback,             EVTCfgHash }, /* EVTLexEquals   */
    {     EVTLexEnd, Pushback,             EVTCfgHash }, /* EVTLexHash     */
    {     EVTLexEnd, Pushback,             EVTCfgHash }, /* EVTLexNewline  */
    {     EVTLexEnd,     Skip,             EVTCfgHash }, /* EVTLexOther    */
    {     EVTLexEnd,     Skip,             EVTCfgHash }  /* EVTLexEOF      */
    },
    /* EVTLexNewline  := 6 */
    {
    {   EVTLexBegin,  Consume,             EVTCfgNone }, /* EVTLexBegin    */
    {     EVTLexEnd, Pushback,          EVTCfgNewline }, /* EVTLexChar     */
    {     EVTLexEnd, Pushback,          EVTCfgNewline }, /* EVTLexLSqBrace */
    {     EVTLexEnd, Pushback,          EVTCfgNewline }, /* EVTLexRSqBrace */
    {     EVTLexEnd, Pushback,          EVTCfgNewline }, /* EVTLexEquals   */
    {     EVTLexEnd, Pushback,          EVTCfgNewline }, /* EVTLexHash     */
    {     EVTLexEnd, Pushback,          EVTCfgNewline }, /* EVTLexNewline  */
    {     EVTLexEnd,     Skip,          EVTCfgNewline }, /* EVTLexOther    */
    {     EVTLexEnd,     Skip,          EVTCfgNewline }  /* EVTLexEOF      */
    },
    /* EVTLexOther    := 7 */
    {
    {   EVTLexBegin,  Consume,             EVTCfgNone }, /* EVTLexBegin    */
    {    EVTLexChar,  Consume,             EVTCfgNone }, /* EVTLexChar     */
    {EVTLexLSqBrace,  Consume,             EVTCfgNone }, /* EVTLexLSqBrace */
    {EVTLexRSqBrace,  Consume,             EVTCfgNone }, /* EVTLexRSqBrace */
    {  EVTLexEquals,  Consume,             EVTCfgNone }, /* EVTLexEquals   */
    {    EVTLexHash,  Consume,             EVTCfgNone }, /* EVTLexHash     */
    { EVTLexNewline,  Consume,             EVTCfgNone }, /* EVTLexNewline  */
    {   EVTLexOther,  Consume,             EVTCfgNone }, /* EVTLexOther    */
    {     EVTLexEOF,  Consume,             EVTCfgNone }  /* EVTLexEOF      */
    },
    /* EVTLexEOF      := 8 */
    {
    {   EVTLexBegin,  Consume,             EVTCfgNone }, /* EVTLexBegin    */
    {     EVTLexEnd,  Consume,             EVTCfgNone }, /* EVTLexChar     */
    {     EVTLexEnd,  Consume,             EVTCfgNone }, /* EVTLexLSqBrace */
    {     EVTLexEnd,  Consume,             EVTCfgNone }, /* EVTLexRSqBrace */
    {     EVTLexEnd,  Consume,             EVTCfgNone }, /* EVTLexEquals   */
    {     EVTLexEnd,  Consume,             EVTCfgNone }, /* EVTLexHash     */
    { EVTLexNewline,  Consume,             EVTCfgNone }, /* EVTLexNewline  */
    {     EVTLexEnd,  Consume,             EVTCfgNone }, /* EVTLexOther    */
    {     EVTLexEnd,  Consume,             EVTCfgNone }  /* EVTLexEOF      */
    }
};

DWORD
EVTParseConfigFile(
    PCSTR                     pszFilePath,
    PFNCONFIG_START_SECTION   pfnStartSectionHandler,
    PFNCONFIG_COMMENT         pfnCommentHandler,
    PFNCONFIG_NAME_VALUE_PAIR pfnNameValuePairHandler,
    PFNCONFIG_END_SECTION     pfnEndSectionHandler
    )
{
    DWORD dwError = 0;
    PEVT_CONFIG_PARSE_STATE pParseState = NULL;

    dwError = EVTCfgInitParseState(
                    pszFilePath,
                    pfnStartSectionHandler,
                    pfnCommentHandler,
                    pfnNameValuePairHandler,
                    pfnEndSectionHandler,
                    &pParseState);
    BAIL_ON_EVT_ERROR(dwError);

    dwError = EVTCfgParse(pParseState);
    BAIL_ON_EVT_ERROR(dwError);

cleanup:

    if (pParseState) {
        EVTCfgFreeParseState(pParseState);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
EVTCfgInitParseState(
    PCSTR                     pszFilePath,
    PFNCONFIG_START_SECTION   pfnStartSectionHandler,
    PFNCONFIG_COMMENT         pfnCommentHandler,
    PFNCONFIG_NAME_VALUE_PAIR pfnNameValuePairHandler,
    PFNCONFIG_END_SECTION     pfnEndSectionHandler,
    PEVT_CONFIG_PARSE_STATE*  ppParseState
    )
{
    DWORD dwError = 0;
    PEVT_CONFIG_PARSE_STATE pParseState = NULL;
    PEVT_STACK pTokenStack = NULL;
    FILE* fp = NULL;

    if ((fp = fopen(pszFilePath, "r")) == NULL) {
        dwError = errno;
        BAIL_ON_EVT_ERROR(dwError);
    }


    dwError = EVTAllocateMemory(
                    sizeof(EVT_CONFIG_PARSE_STATE),
                    (PVOID*)&pParseState);
    BAIL_ON_EVT_ERROR(dwError);

    dwError = EVTAllocateMemory(
                    sizeof(EVT_STACK),
                    (PVOID*)&pTokenStack);
    BAIL_ON_EVT_ERROR(dwError);

    pParseState->pLexerTokenStack = pTokenStack;

    dwError = EVTAllocateString(
                    pszFilePath,
                    &(pParseState->pszFilePath));
    BAIL_ON_EVT_ERROR(dwError);

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
        EVTCfgFreeParseState(pParseState);
    }

    if (fp) {
        fclose(fp);
    }

    goto cleanup;
}

VOID
EVTCfgFreeParseState(
    PEVT_CONFIG_PARSE_STATE pParseState
    )
{
    EVT_SAFE_FREE_STRING(pParseState->pszFilePath);
    if (pParseState->fp) {
        fclose(pParseState->fp);
    }
    EVTFreeMemory(pParseState);
}


DWORD
EVTCfgParse(
    PEVT_CONFIG_PARSE_STATE pParseState
    )
{

    DWORD dwError = 0;
    PEVT_CFG_TOKEN pToken = NULL;
    BOOLEAN bContinue = TRUE;
    PEVT_STACK pTokenStack = NULL;

    do
    {

        dwError = EVTCfgGetNextToken(
                        pParseState,
                        &pToken);
        BAIL_ON_EVT_ERROR(dwError);

        switch (pToken->tokenType)
        {
            case EVTCfgHash:
            {
                dwError = EVTCfgParseComment(
                                pParseState,
                                &bContinue);
                BAIL_ON_EVT_ERROR(dwError);

                break;
            }
            case EVTCfgNewline:
            {
                //in case of a blank line (newline),
                //interpret this as an empty comment.
                dwError = EVTCfgProcessComment(
                                pParseState,
                                &pTokenStack,
                                &bContinue);
                BAIL_ON_EVT_ERROR(dwError);

                break;
            }
            case EVTCfgLeftSquareBrace:
            {

                dwError = EVTCfgParseSections(
                                pParseState);
                BAIL_ON_EVT_ERROR(dwError);

                break;
            }
            case EVTCfgEOF:
            {
                bContinue = FALSE;
                break;
            }
            default:
            {
                dwError = EVT_ERROR_INVALID_CONFIG;
                BAIL_ON_EVT_ERROR(dwError);
            }
        }

    } while (bContinue);

cleanup:

    if (pToken) {
        EVTCfgFreeToken(pToken);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
EVTCfgParseSections(
    PEVT_CONFIG_PARSE_STATE pParseState
    )
{
    DWORD dwError = 0;
    PEVT_CFG_TOKEN pToken = NULL;
    BOOLEAN bContinue = TRUE;
    PEVT_STACK pTokenStack = NULL;

    dwError = EVTCfgParseSectionHeader(
                    pParseState,
                    &bContinue);
    BAIL_ON_EVT_ERROR(dwError);

    while (bContinue)
    {
        dwError = EVTCfgGetNextToken(
                        pParseState,
                        &pToken);
        BAIL_ON_EVT_ERROR(dwError);

        switch (pToken->tokenType)
        {
            case EVTCfgString:
            {

	      EVTStripWhitespace(pToken->pszToken, TRUE, TRUE);

                if(!IsNullOrEmptyString(pToken->pszToken))
		{

                    dwError = EVTStackPush(pToken, &(pParseState->pLexerTokenStack));
                    BAIL_ON_EVT_ERROR(dwError);

                    pToken = NULL;

                    dwError = EVTCfgParseNameValuePair(
                                    pParseState,
                                    &bContinue);
                    BAIL_ON_EVT_ERROR(dwError);
		}
                break;
            }

            case EVTCfgHash:
            {
                dwError = EVTCfgParseComment(
                                pParseState,
                                &bContinue);
                BAIL_ON_EVT_ERROR(dwError);
                break;
            }
            case EVTCfgNewline:
            {
                //in case of a blank line (newline),
                //interpret this as an empty comment.
                dwError = EVTCfgProcessComment(
                                pParseState,
                                &pTokenStack,
                                &bContinue);
                BAIL_ON_EVT_ERROR(dwError);
                break;
            }
            case EVTCfgLeftSquareBrace:
            {
                dwError = EVTCfgParseSectionHeader(
                                pParseState,
                                &bContinue);
                BAIL_ON_EVT_ERROR(dwError);

                break;
            }
            case EVTCfgEOF:
            {
                bContinue = FALSE;
                break;
            }
            default:
            {
                dwError = EVT_ERROR_INVALID_CONFIG;
                BAIL_ON_EVT_ERROR(dwError);
            }
        }
    }

    if (bContinue && !IsNullOrEmptyString(pParseState->pszSectionName))
    {
        dwError = EVTCfgProcessEndSection(
                        pParseState,
                        &bContinue);
        BAIL_ON_EVT_ERROR(dwError);
    }

cleanup:

    if (pToken) {
        EVTCfgFreeToken(pToken);
    }

    return dwError;

error:

    goto cleanup;
}



DWORD
EVTCfgParseComment(
    PEVT_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN    pbContinue
    )
{
    DWORD dwError = 0;
    BOOLEAN bContinue = TRUE;
    BOOLEAN bKeepParsing = TRUE;
    PEVT_CFG_TOKEN pToken = NULL;
    PEVT_STACK pTokenStack = NULL;

    do
    {
        dwError = EVTCfgGetNextToken(
                    pParseState,
                    &pToken);
        BAIL_ON_EVT_ERROR(dwError);

        switch (pToken->tokenType)
        {
            case EVTCfgEOF:
            {
                dwError = EVTCfgProcessComment(
                                pParseState,
                                &pTokenStack,
                                &bContinue);
                BAIL_ON_EVT_ERROR(dwError);

                bContinue = FALSE;

                break;
            }
            case EVTCfgNewline:
            {
                dwError = EVTCfgProcessComment(
                                pParseState,
                                &pTokenStack,
                                &bContinue);
                BAIL_ON_EVT_ERROR(dwError);

                bKeepParsing = FALSE;

                break;
            }
            default:
            {
                dwError = EVTStackPush(pToken, &pTokenStack);
                BAIL_ON_EVT_ERROR(dwError);

                pToken = NULL;

            }
        }

    } while (bContinue && bKeepParsing);

    *pbContinue = bContinue;

cleanup:

    if (pToken) {
        EVTCfgFreeToken(pToken);
    }

    return dwError;

error:

    *pbContinue = FALSE;

    goto cleanup;
}

DWORD
EVTCfgParseSectionHeader(
    PEVT_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    BOOLEAN bKeepParsing = TRUE;
    BOOLEAN bContinue = TRUE;
    PEVT_CFG_TOKEN pToken = NULL;
    PEVT_STACK pTokenStack = NULL;

    if (!IsNullOrEmptyString(pParseState->pszSectionName))
    {
        dwError = EVTCfgProcessEndSection(
                        pParseState,
                        &bContinue);
        BAIL_ON_EVT_ERROR(dwError);

    }

    if (!bContinue)
    {
        goto done;
    }

    pParseState->bSkipSection = FALSE;

    do
    {
        dwError = EVTCfgGetNextToken(
                        pParseState,
                        &pToken);
        BAIL_ON_EVT_ERROR(dwError);

        switch(pToken->tokenType)
        {
            case EVTCfgString:
            case EVTCfgEquals:
            case EVTCfgOther:
            {
                dwError = EVTStackPush(pToken, &pTokenStack);
                BAIL_ON_EVT_ERROR(dwError);

                pToken = NULL;
                break;
            }
            case EVTCfgRightSquareBrace:
            {
                dwError = EVTAssertWhitespaceOnly(pParseState);
                BAIL_ON_EVT_ERROR(dwError);

                bKeepParsing = FALSE;

                break;
            }
            default:
            {
                dwError = EVT_ERROR_INVALID_CONFIG;
                BAIL_ON_EVT_ERROR(dwError);
            }
        }

    } while (bKeepParsing);

    dwError = EVTCfgGetNextToken(
                    pParseState,
                    &pToken);
    BAIL_ON_EVT_ERROR(dwError);

    switch(pToken->tokenType)
    {

        case EVTCfgNewline:
        {
            dwError = EVTCfgProcessBeginSection(
                            pParseState,
                            &pTokenStack,
                            &bContinue);
            BAIL_ON_EVT_ERROR(dwError);

            break;
        }
        case EVTCfgEOF:
        {
            dwError = EVTCfgProcessBeginSection(
                            pParseState,
                            &pTokenStack,
                            &bContinue);
            BAIL_ON_EVT_ERROR(dwError);

            if (bContinue) {

                dwError = EVTCfgProcessEndSection(
                                pParseState,
                                &bContinue);
                BAIL_ON_EVT_ERROR(dwError);
            }

            bContinue = FALSE;

            break;
        }
        default:
        {
            dwError = EVT_ERROR_INVALID_CONFIG;
            BAIL_ON_EVT_ERROR(dwError);
        }
    }

done:

    *pbContinue = bContinue;

cleanup:

    if (pToken) {
        EVTCfgFreeToken(pToken);
    }

    return dwError;

error:

    *pbContinue = FALSE;

    goto cleanup;
}

DWORD
EVTAssertWhitespaceOnly(
    PEVT_CONFIG_PARSE_STATE pParseState
    )
{
    DWORD dwError = 0;
    PEVT_CFG_TOKEN pToken = NULL;
    BOOLEAN bKeepParsing = TRUE;

    do
    {
        dwError = EVTCfgGetNextToken(
                        pParseState,
                        &pToken);
        BAIL_ON_EVT_ERROR(dwError);

        switch(pToken->tokenType)
        {
            case EVTCfgString:
            case EVTCfgOther:
            {
                DWORD i = 0;
                for (; i < pToken->dwLen; i++) {
                    if (!isspace((int)pToken->pszToken[i])) {
                        dwError = EVT_ERROR_INVALID_CONFIG;
                        BAIL_ON_EVT_ERROR(dwError);
                    }
                }
                break;
            }
            case EVTCfgEOF:
            case EVTCfgNewline:
            {
                dwError = EVTStackPush(pToken, &pParseState->pLexerTokenStack);
                BAIL_ON_EVT_ERROR(dwError);

                pToken = NULL;

                bKeepParsing = FALSE;

                break;
            }
            default:
            {
                dwError = EVT_ERROR_INVALID_CONFIG;
                BAIL_ON_EVT_ERROR(dwError);
            }
        }
    } while (bKeepParsing);

cleanup:

    if (pToken) {
        EVTCfgFreeToken(pToken);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
EVTCfgParseNameValuePair(
    PEVT_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    BOOLEAN bContinue = TRUE;
    BOOLEAN bKeepParsing = TRUE;
    PEVT_CFG_TOKEN pToken = NULL;
    PEVT_STACK pTokenStack = NULL;

    //format is <str><equals><token1><token2>...<newline>

    //get initial <str>
    dwError = EVTCfgGetNextToken(
                    pParseState,
                    &pToken);
    BAIL_ON_EVT_ERROR(dwError);

    if (pToken->tokenType == EVTCfgString)
    {
        dwError = EVTStackPush(pToken, &pTokenStack);
        BAIL_ON_EVT_ERROR(dwError);
        pToken = NULL;
    }
    else
    {
        dwError = EVT_ERROR_INVALID_CONFIG;
        BAIL_ON_EVT_ERROR(dwError);
    }

    //get <equals>
    dwError = EVTCfgGetNextToken(
                    pParseState,
                    &pToken);
    BAIL_ON_EVT_ERROR(dwError);

    if (pToken->tokenType == EVTCfgEquals)
    {
        dwError = EVTStackPush(pToken, &pTokenStack);
        BAIL_ON_EVT_ERROR(dwError);
        pToken = NULL;
    }
    else
    {
        dwError = EVT_ERROR_INVALID_CONFIG;
        BAIL_ON_EVT_ERROR(dwError);
    }


    do
    {
        dwError = EVTCfgGetNextToken(
                        pParseState,
                        &pToken);
        BAIL_ON_EVT_ERROR(dwError);

        switch(pToken->tokenType)
        {
            case EVTCfgString:
            case EVTCfgEquals:
            case EVTCfgOther:
            {

                dwError = EVTStackPush(pToken, &pTokenStack);
                BAIL_ON_EVT_ERROR(dwError);
                pToken = NULL;

                break;

            }
            case EVTCfgNewline:
            {
                dwError = EVTCfgProcessNameValuePair(
                    pParseState,
                    &pTokenStack,
                    &bContinue);
                BAIL_ON_EVT_ERROR(dwError);
                bKeepParsing = FALSE;
                break;
            }
            case EVTCfgEOF:
            {
                dwError = EVTCfgProcessNameValuePair(
                    pParseState,
                    &pTokenStack,
                    &bContinue);
                BAIL_ON_EVT_ERROR(dwError);
                bContinue = FALSE;
                break;
            }
            default:
            {
                dwError = EVT_ERROR_INVALID_CONFIG;
                BAIL_ON_EVT_ERROR(dwError);
            }
        }
    } while (bContinue && bKeepParsing);


    *pbContinue = bContinue;

cleanup:

    if (pToken) {
        EVTCfgFreeToken(pToken);
    }

    return dwError;

error:

    *pbContinue = FALSE;

    goto cleanup;
}

DWORD
EVTCfgProcessComment(
    PEVT_CONFIG_PARSE_STATE pParseState,
    PEVT_STACK* ppTokenStack,
    PBOOLEAN    pbContinue
    )
{
    DWORD   dwError = 0;
    BOOLEAN bContinue = TRUE;
    PSTR    pszComment = NULL;

    dwError = EVTCfgProcessTokenStackIntoString(
        ppTokenStack,
        &pszComment);
    BAIL_ON_EVT_ERROR(dwError);

    if (pParseState->pfnCommentHandler &&
        !pParseState->bSkipSection) {

        dwError = pParseState->pfnCommentHandler(
                        pszComment,
                        &bContinue);
        BAIL_ON_EVT_ERROR(dwError);
    }

    *pbContinue = bContinue;

cleanup:

    EVT_SAFE_FREE_STRING(pszComment);

    return dwError;

error:

    *pbContinue = TRUE;

    goto cleanup;
}

DWORD
EVTCfgProcessBeginSection(
    PEVT_CONFIG_PARSE_STATE pParseState,
    PEVT_STACK* ppTokenStack,
    PBOOLEAN    pbContinue)
{
    DWORD   dwError = 0;
    PSTR    pszSectionName = NULL;
    BOOLEAN bSkipSection = FALSE;
    BOOLEAN bContinue = TRUE;

    dwError = EVTCfgProcessTokenStackIntoString(
        ppTokenStack,
        &pszSectionName);
    BAIL_ON_EVT_ERROR(dwError);

    if (IsNullOrEmptyString(pszSectionName)) {
        dwError = EVT_ERROR_INVALID_CONFIG;
        BAIL_ON_EVT_ERROR(dwError);
    }

    if (pParseState->pfnStartSectionHandler) {
        dwError = pParseState->pfnStartSectionHandler(
                        pszSectionName,
                        &bSkipSection,
                        &bContinue);
        BAIL_ON_EVT_ERROR(dwError);
    }

    pParseState->pszSectionName = pszSectionName;
    pParseState->bSkipSection = bSkipSection;
    *pbContinue = bContinue;

cleanup:

    return dwError;

error:

    EVT_SAFE_FREE_STRING(pszSectionName);
    pParseState->pszSectionName = NULL;
    pParseState->bSkipSection = FALSE;
    *pbContinue = FALSE;

    goto cleanup;
}

DWORD
EVTCfgProcessNameValuePair(
    PEVT_CONFIG_PARSE_STATE pParseState,
    PEVT_STACK*             ppTokenStack,
    PBOOLEAN                pbContinue
    )
{
    DWORD      dwError = 0;
    BOOLEAN    bContinue = TRUE;
    PSTR       pszName = NULL;
    PSTR       pszValue = NULL;
    PEVT_CFG_TOKEN pToken = NULL;

    *ppTokenStack = EVTStackReverse(*ppTokenStack);
    pToken = (PEVT_CFG_TOKEN)EVTStackPop(ppTokenStack);
    if (pToken && pToken->dwLen) {
        dwError = EVTStrndup(
                    pToken->pszToken,
                    pToken->dwLen,
                    &pszName);
        BAIL_ON_EVT_ERROR(dwError);
    }

    if (IsNullOrEmptyString(pszName)) {
        dwError = EVT_ERROR_INVALID_CONFIG;
        BAIL_ON_EVT_ERROR(dwError);
    }

    EVTCfgFreeToken(pToken);
    pToken = NULL;

    pToken = (PEVT_CFG_TOKEN)EVTStackPop(ppTokenStack);
    if (!pToken || pToken->tokenType != EVTCfgEquals)
    {
        dwError = EVT_ERROR_INVALID_CONFIG;
        BAIL_ON_EVT_ERROR(dwError);
    }

    EVTCfgFreeToken(pToken);
    pToken = NULL;

    //this will consume the token stack
    dwError = EVTCfgProcessTokenStackIntoString(
        ppTokenStack,
        &pszValue);
    BAIL_ON_EVT_ERROR(dwError);

    if (pParseState->pfnNameValuePairHandler  &&
        !pParseState->bSkipSection) {

        dwError = pParseState->pfnNameValuePairHandler(
                        pszName,
                        pszValue,
                        &bContinue);
        BAIL_ON_EVT_ERROR(dwError);

    }

    *pbContinue = bContinue;

cleanup:

    if (pToken) {
        EVTCfgFreeToken(pToken);
        pToken = NULL;
    }

    if (ppTokenStack && *ppTokenStack)
    {
        dwError = EVTCfgFreeTokenStack(ppTokenStack);
    }

    EVT_SAFE_FREE_STRING(pszName);
    EVT_SAFE_FREE_STRING(pszValue);

    return dwError;

error:

    goto cleanup;
}

DWORD
EVTCfgProcessEndSection(
    PEVT_CONFIG_PARSE_STATE pParseState,
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
        BAIL_ON_EVT_ERROR(dwError);
    }

    *pbContinue = bContinue;

cleanup:

    EVT_SAFE_FREE_STRING(pParseState->pszSectionName);

    return dwError;

error:

    *pbContinue = TRUE;

    goto cleanup;
}

DWORD
EVTCfgDetermineTokenLength(
    PEVT_STACK pStack
    )
{
    DWORD dwLen = 0;
    PEVT_STACK pIter = pStack;

    for (; pIter; pIter = pIter->pNext)
    {
        PEVT_CFG_TOKEN pToken = (PEVT_CFG_TOKEN)pIter->pItem;
        if (pToken) {
            dwLen += pToken->dwLen;
        }
    }

    return dwLen;
}

//this will consume the token stack
DWORD
EVTCfgProcessTokenStackIntoString(
    PEVT_STACK* ppTokenStack,
    PSTR* ppszConcatenated
    )
{
    DWORD   dwError = 0;
    DWORD   dwRequiredTokenLen = 0;
    PSTR    pszConcatenated = NULL;

    dwRequiredTokenLen = EVTCfgDetermineTokenLength(*ppTokenStack);

    if (dwRequiredTokenLen) {

        PSTR pszPos = NULL;
        PEVT_CFG_TOKEN pToken = NULL;

        *ppTokenStack = EVTStackReverse(*ppTokenStack);


        dwError = EVTAllocateMemory(
                        (dwRequiredTokenLen + 1) * sizeof(CHAR),
                        (PVOID*)&pszConcatenated);
        BAIL_ON_EVT_ERROR(dwError);


        pszPos = pszConcatenated;
        while (*ppTokenStack)
        {
            pToken = EVTStackPop(ppTokenStack);
            if (pToken && pToken->dwLen) {

                strncpy(pszPos, pToken->pszToken, pToken->dwLen);
                pszPos += pToken->dwLen;

                EVTCfgFreeToken(pToken);
                pToken = NULL;
            }
        }
    }

    *ppszConcatenated = pszConcatenated;

cleanup:

    return dwError;

error:

    EVT_SAFE_FREE_STRING(pszConcatenated);

    *ppszConcatenated = NULL;

    goto cleanup;
}


DWORD
EVTCfgAllocateToken(
    DWORD           dwSize,
    PEVT_CFG_TOKEN* ppToken
    )
{
    DWORD dwError = 0;
    PEVT_CFG_TOKEN pToken = NULL;
    DWORD dwMaxLen = (dwSize > 0 ? dwSize : EVT_CFG_TOKEN_DEFAULT_LENGTH);


    dwError = EVTAllocateMemory(
                    sizeof(EVT_CFG_TOKEN),
                    (PVOID*)&pToken);
    BAIL_ON_EVT_ERROR(dwError);

    dwError = EVTAllocateMemory(
                    dwMaxLen * sizeof(CHAR),
                    (PVOID*)&pToken->pszToken);
    BAIL_ON_EVT_ERROR(dwError);


    pToken->tokenType = EVTCfgNone;
    pToken->dwMaxLen = dwMaxLen;

    *ppToken = pToken;

cleanup:

    return dwError;

error:

    *ppToken = NULL;

    if (pToken) {
        EVTCfgFreeToken(pToken);
    }

    goto cleanup;
}

DWORD
EVTCfgReallocToken(
    PEVT_CFG_TOKEN pToken,
    DWORD          dwNewSize
    )
{
    DWORD dwError = 0;

    dwError = EVTReallocMemory(
                    pToken->pszToken,
                    (PVOID*)&pToken->pszToken,
                    dwNewSize);
    BAIL_ON_EVT_ERROR(dwError);

    pToken->dwMaxLen = dwNewSize;

cleanup:

    return dwError;

error:

    goto cleanup;
}

VOID
EVTCfgResetToken(
    PEVT_CFG_TOKEN pToken
    )
{
    if (pToken && pToken->pszToken && pToken->dwMaxLen)
    {
        memset(pToken->pszToken, 0, pToken->dwMaxLen);
        pToken->dwLen = 0;
    }
}

DWORD
EVTCfgCopyToken(
    PEVT_CFG_TOKEN pTokenSrc,
    PEVT_CFG_TOKEN pTokenDst
    )
{
    DWORD dwError = 0;

    pTokenDst->tokenType = pTokenSrc->tokenType;

    if (pTokenSrc->dwLen > pTokenDst->dwLen) {
        dwError = EVTReallocMemory(
                        (PVOID)  pTokenDst->pszToken,
                        (PVOID*) &pTokenDst->pszToken,
                        (DWORD)  pTokenSrc->dwLen);
        BAIL_ON_EVT_ERROR(dwError);

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
EVTCfgFreeTokenStack(
    PEVT_STACK* ppTokenStack
    )
{

    DWORD dwError = 0;

    PEVT_STACK pTokenStack = *ppTokenStack;

    dwError = EVTStackForeach(
            pTokenStack,
            &EVTCfgFreeTokenInStack,
            NULL);
    BAIL_ON_EVT_ERROR(dwError);

    EVTStackFree(pTokenStack);

    *ppTokenStack = NULL;

error:

    return dwError;
}

DWORD
EVTCfgFreeTokenInStack(
    PVOID pToken,
    PVOID pUserData
    )
{
    if (pToken)
    {
        EVTCfgFreeToken((PEVT_CFG_TOKEN)pToken);
    }

    return 0;
}

VOID
EVTCfgFreeToken(
    PEVT_CFG_TOKEN pToken
    )
{
    EVT_SAFE_FREE_MEMORY(pToken->pszToken);
    EVTFreeMemory(pToken);
}

DWORD
EVTCfgGetNextToken(
    PEVT_CONFIG_PARSE_STATE pParseState,
    PEVT_CFG_TOKEN*         ppToken
    )
{
    DWORD dwError = 0;
    EVTCfgTokenType tokenType = EVTCfgNone;
    EVTCfgLexState  curLexState = EVTLexBegin;
    PEVT_CFG_TOKEN  pToken = NULL;
    BOOLEAN         bOwnToken = FALSE;

    if (EVTStackPeek(pParseState->pLexerTokenStack) != NULL)
    {
        PEVT_CFG_TOKEN pToken_input = *ppToken;

        pToken = (PEVT_CFG_TOKEN)EVTStackPop(&pParseState->pLexerTokenStack);
        bOwnToken = TRUE;

        if (pToken_input) {

            dwError = EVTCfgCopyToken(pToken, pToken_input);
            BAIL_ON_EVT_ERROR(dwError);

            EVTCfgFreeToken(pToken);
            pToken = NULL;
            bOwnToken = FALSE;

        }

        goto done;
    }

    pToken = *ppToken;

    if (!pToken) {
        dwError = EVTCfgAllocateToken(
                        0,
                        &pToken);
        BAIL_ON_EVT_ERROR(dwError);

        bOwnToken = TRUE;
    }
    else
    {
        EVTCfgResetToken(pToken);
    }

    while (curLexState != EVTLexEnd)
    {
        DWORD ch = EVTCfgGetCharacter(pParseState);
        EVTCfgLexState lexClass = EVTCfgGetLexClass(ch);

        if (lexClass != EVTLexEOF) {
            pParseState->dwCol++;
        }

        if (ch == (DWORD)'\n') {
            pParseState->dwLine++;
            pParseState->dwCol = 0;
        }

        tokenType = EVTCfgGetTokenType(curLexState, lexClass);

        switch(EVTCfgGetLexAction(curLexState, lexClass))
        {
            case Skip:

                break;

            case Consume:

                if (pToken->dwLen >= pToken->dwMaxLen) {
                    dwError = EVTCfgReallocToken(
                                    pToken,
                                    pToken->dwMaxLen + EVT_CFG_TOKEN_DEFAULT_LENGTH);
                    BAIL_ON_EVT_ERROR(dwError);
                }

                pToken->pszToken[pToken->dwLen++] = (BYTE)ch;

                break;

            case Pushback:

                pParseState->dwCol--;
                dwError = EVTCfgPushBackCharacter(
                                pParseState,
                                (BYTE)ch);
                BAIL_ON_EVT_ERROR(dwError);

                break;
        }

        curLexState = EVTCfgGetNextLexState(curLexState, lexClass);
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
        EVTCfgFreeToken(pToken);
        *ppToken = NULL;
    }

    goto cleanup;
}

DWORD
EVTCfgGetCharacter(
    PEVT_CONFIG_PARSE_STATE pParseState
    )
{
    return getc(pParseState->fp);
}

EVTCfgLexState
EVTCfgGetLexClass(
    DWORD ch
    )
{

    if (ch == EOF) {
        return EVTLexEOF;
    }

    if (ch == '\n') {
        return EVTLexNewline;
    }

    if (ch == '[') {
        return EVTLexLSqBrace;
    }

    if (ch == ']') {
        return EVTLexRSqBrace;
    }

    if (ch == '=') {
        return EVTLexEquals;
    }

    if (ch == '#') {
        return EVTLexHash;
    }

    if (isalnum(ch) ||
        ispunct(ch) ||
        isblank(ch)) {
        return EVTLexChar;
    }

    return EVTLexOther;
}

DWORD
EVTCfgPushBackCharacter(
    PEVT_CONFIG_PARSE_STATE pParseState,
    BYTE ch
    )
{
    return ((EOF == ungetc(ch, pParseState->fp)) ? errno : 0);
}

EVTCfgLexState
EVTCfgGetNextLexState(
    EVTCfgLexState currentState,
    DWORD chId
    )
{
    return (gEVTLexStateTable[currentState][chId].nextState);
}

EVTLexAction
EVTCfgGetLexAction(
    EVTCfgLexState currentState,
    DWORD chId
    )
{
    return (gEVTLexStateTable[currentState][chId].action);
}

EVTCfgTokenType
EVTCfgGetTokenType(
    EVTCfgLexState currentState,
    DWORD chId
    )
{
    return (gEVTLexStateTable[currentState][chId].tokenId);
}
