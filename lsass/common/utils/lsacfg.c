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
 *        lsacfg.c
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
#include "includes.h"

static LSA_CFG_LEXER_STATE gLsaLexStateTable[][9] =
{
  /* LsaLexBegin    := 0 */
  {
    {   LsaLexBegin,  Consume,             LsaCfgNone }, /* LsaLexBegin    */
    {    LsaLexChar,  Consume,             LsaCfgNone }, /* LsaLexChar     */
    {     LsaLexEnd,  Consume,  LsaCfgLeftSquareBrace }, /* LsaLexLSqBrace */
    {     LsaLexEnd,  Consume, LsaCfgRightSquareBrace }, /* LsaLexRSqBrace */
    {     LsaLexEnd,  Consume,           LsaCfgEquals }, /* LsaLexEquals   */
    {     LsaLexEnd,  Consume,             LsaCfgHash }, /* LsaLexHash     */
    {     LsaLexEnd,  Consume,          LsaCfgNewline }, /* LsaLexNewline  */
    {   LsaLexBegin,     Skip,             LsaCfgNone }, /* LsaLexOther    */
    {     LsaLexEnd,  Consume,              LsaCfgEOF }  /* LsaLexEOF      */
  },
  /* LsaLexChar     := 1 */
  {
    {   LsaLexBegin,  Consume,             LsaCfgNone }, /* LsaLexBegin    */
    {    LsaLexChar,  Consume,             LsaCfgNone }, /* LsaLexChar     */
    {     LsaLexEnd, Pushback,           LsaCfgString }, /* LsaLexLSqBrace */
    {     LsaLexEnd, Pushback,           LsaCfgString }, /* LsaLexRSqBrace */
    {     LsaLexEnd, Pushback,           LsaCfgString }, /* LsaLexEquals   */
    {     LsaLexEnd, Pushback,           LsaCfgString }, /* LsaLexHash     */
    {     LsaLexEnd, Pushback,           LsaCfgString }, /* LsaLexNewline  */
    {     LsaLexEnd,     Skip,           LsaCfgString }, /* LsaLexOther    */
    {     LsaLexEnd,     Skip,           LsaCfgString }  /* LsaLexEOF      */
  },
  /* LsaLexLSqBrace := 2 */
  {
    {   LsaLexBegin,  Consume,             LsaCfgNone }, /* LsaLexBegin    */
    {     LsaLexEnd, Pushback,  LsaCfgLeftSquareBrace }, /* LsaLexChar     */
    {     LsaLexEnd, Pushback,  LsaCfgLeftSquareBrace }, /* LsaLexLSqBrace */
    {     LsaLexEnd, Pushback,  LsaCfgLeftSquareBrace }, /* LsaLexRSqBrace */
    {     LsaLexEnd, Pushback,  LsaCfgLeftSquareBrace }, /* LsaLexEquals   */
    {     LsaLexEnd, Pushback,  LsaCfgLeftSquareBrace }, /* LsaLexHash     */
    {     LsaLexEnd, Pushback,  LsaCfgLeftSquareBrace }, /* LsaLexNewline  */
    {     LsaLexEnd,     Skip,  LsaCfgLeftSquareBrace }, /* LsaLexOther    */
    {     LsaLexEnd,     Skip,  LsaCfgLeftSquareBrace }  /* LsaLexEOF      */
  },
  /* LsaLexRSqBrace := 3 */
  {
    {   LsaLexBegin,  Consume,             LsaCfgNone }, /* LsaLexBegin    */
    {     LsaLexEnd, Pushback, LsaCfgRightSquareBrace }, /* LsaLexChar     */
    {     LsaLexEnd, Pushback, LsaCfgRightSquareBrace }, /* LsaLexLSqBrace */
    {     LsaLexEnd, Pushback, LsaCfgRightSquareBrace }, /* LsaLexRSqBrace */
    {     LsaLexEnd, Pushback, LsaCfgRightSquareBrace }, /* LsaLexEquals   */
    {     LsaLexEnd, Pushback, LsaCfgRightSquareBrace }, /* LsaLexHash     */
    {     LsaLexEnd, Pushback, LsaCfgRightSquareBrace }, /* LsaLexNewline  */
    {     LsaLexEnd,     Skip, LsaCfgRightSquareBrace }, /* LsaLexOther    */
    {     LsaLexEnd,     Skip, LsaCfgRightSquareBrace }  /* LsaLexEOF      */
  },
  /* LsaLexEquals   := 4 */
  {
    {   LsaLexBegin,  Consume,             LsaCfgNone }, /* LsaLexBegin    */
    {     LsaLexEnd, Pushback,           LsaCfgEquals }, /* LsaLexChar     */
    {     LsaLexEnd, Pushback,           LsaCfgEquals }, /* LsaLexLSqBrace */
    {     LsaLexEnd, Pushback,           LsaCfgEquals }, /* LsaLexRSqBrace */
    {     LsaLexEnd, Pushback,           LsaCfgEquals }, /* LsaLexEquals   */
    {     LsaLexEnd, Pushback,           LsaCfgEquals }, /* LsaLexHash     */
    {     LsaLexEnd, Pushback,           LsaCfgEquals }, /* LsaLexNewline  */
    {     LsaLexEnd,     Skip,           LsaCfgEquals }, /* LsaLexOther    */
    {     LsaLexEnd,     Skip,           LsaCfgEquals }  /* LsaLexEOF      */
  },
  /* LsaLexHash     := 5 */
  {
    {   LsaLexBegin,  Consume,             LsaCfgNone }, /* LsaLexBegin    */
    {     LsaLexEnd, Pushback,             LsaCfgHash }, /* LsaLexChar     */
    {     LsaLexEnd, Pushback,             LsaCfgHash }, /* LsaLexLSqBrace */
    {     LsaLexEnd, Pushback,             LsaCfgHash }, /* LsaLexRSqBrace */
    {     LsaLexEnd, Pushback,             LsaCfgHash }, /* LsaLexEquals   */
    {     LsaLexEnd, Pushback,             LsaCfgHash }, /* LsaLexHash     */
    {     LsaLexEnd, Pushback,             LsaCfgHash }, /* LsaLexNewline  */
    {     LsaLexEnd,     Skip,             LsaCfgHash }, /* LsaLexOther    */
    {     LsaLexEnd,     Skip,             LsaCfgHash }  /* LsaLexEOF      */
  },
  /* LsaLexNewline  := 6 */
  {
    {   LsaLexBegin,  Consume,             LsaCfgNone }, /* LsaLexBegin    */
    {     LsaLexEnd, Pushback,          LsaCfgNewline }, /* LsaLexChar     */
    {     LsaLexEnd, Pushback,          LsaCfgNewline }, /* LsaLexLSqBrace */
    {     LsaLexEnd, Pushback,          LsaCfgNewline }, /* LsaLexRSqBrace */
    {     LsaLexEnd, Pushback,          LsaCfgNewline }, /* LsaLexEquals   */
    {     LsaLexEnd, Pushback,          LsaCfgNewline }, /* LsaLexHash     */
    {     LsaLexEnd, Pushback,          LsaCfgNewline }, /* LsaLexNewline  */
    {     LsaLexEnd,     Skip,          LsaCfgNewline }, /* LsaLexOther    */
    {     LsaLexEnd,     Skip,          LsaCfgNewline }  /* LsaLexEOF      */
  },
  /* LsaLexOther    := 7 */
  {
    {   LsaLexBegin,  Consume,             LsaCfgNone }, /* LsaLexBegin    */
    {    LsaLexChar,  Consume,             LsaCfgNone }, /* LsaLexChar     */
    {LsaLexLSqBrace,  Consume,             LsaCfgNone }, /* LsaLexLSqBrace */
    {LsaLexRSqBrace,  Consume,             LsaCfgNone }, /* LsaLexRSqBrace */
    {  LsaLexEquals,  Consume,             LsaCfgNone }, /* LsaLexEquals   */
    {    LsaLexHash,  Consume,             LsaCfgNone }, /* LsaLexHash     */
    { LsaLexNewline,  Consume,             LsaCfgNone }, /* LsaLexNewline  */
    {   LsaLexOther,  Consume,             LsaCfgNone }, /* LsaLexOther    */
    {     LsaLexEOF,  Consume,             LsaCfgNone }  /* LsaLexEOF      */
  },
  /* LsaLexEOF      := 8 */
  {
    {   LsaLexBegin,  Consume,             LsaCfgNone }, /* LsaLexBegin    */
    {     LsaLexEnd,  Consume,             LsaCfgNone }, /* LsaLexChar     */
    {     LsaLexEnd,  Consume,             LsaCfgNone }, /* LsaLexLSqBrace */
    {     LsaLexEnd,  Consume,             LsaCfgNone }, /* LsaLexRSqBrace */
    {     LsaLexEnd,  Consume,             LsaCfgNone }, /* LsaLexEquals   */
    {     LsaLexEnd,  Consume,             LsaCfgNone }, /* LsaLexHash     */
    { LsaLexNewline,  Consume,             LsaCfgNone }, /* LsaLexNewline  */
    {     LsaLexEnd,  Consume,             LsaCfgNone }, /* LsaLexOther    */
    {     LsaLexEnd,  Consume,             LsaCfgNone }  /* LsaLexEOF      */
  }
};

DWORD
LsaParseConfigFile(
    PCSTR                     pszFilePath,
    DWORD                     dwOptions,
    PFNCONFIG_START_SECTION   pfnStartSectionHandler,
    PFNCONFIG_COMMENT         pfnCommentHandler,
    PFNCONFIG_NAME_VALUE_PAIR pfnNameValuePairHandler,
    PFNCONFIG_END_SECTION     pfnEndSectionHandler,
    PVOID                     pData
    )
{
    DWORD dwError = 0;
    PLSA_CONFIG_PARSE_STATE pParseState = NULL;

    dwError = LsaCfgInitParseState(
                    pszFilePath,
                    dwOptions,
                    pfnStartSectionHandler,
                    pfnCommentHandler,
                    pfnNameValuePairHandler,
                    pfnEndSectionHandler,
                    pData,
                    &pParseState);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaCfgParse(pParseState);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (pParseState) {
        LsaCfgFreeParseState(pParseState);
    }

    return dwError;

error:

    goto cleanup;
}

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
    )
{
    DWORD dwError = 0;
    PLSA_CONFIG_PARSE_STATE pParseState = NULL;
    PLSA_STACK pTokenStack = NULL;
    FILE* fp = NULL;

    if ((fp = fopen(pszFilePath, "r")) == NULL) {
        dwError = errno;
        BAIL_ON_LSA_ERROR(dwError);
    }


    dwError = LsaAllocateMemory(
                    sizeof(LSA_CONFIG_PARSE_STATE),
                    (PVOID*)&pParseState);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateMemory(
                    sizeof(LSA_STACK),
                    (PVOID*)&pTokenStack);
    BAIL_ON_LSA_ERROR(dwError);

    pParseState->pLexerTokenStack = pTokenStack;

    dwError = LsaAllocateString(
                    pszFilePath,
                    &(pParseState->pszFilePath));
    BAIL_ON_LSA_ERROR(dwError);

    pParseState->fp = fp;
    fp = NULL;

    pParseState->dwOptions = dwOptions;
    pParseState->pData = pData;

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
        LsaCfgFreeParseState(pParseState);
    }

    if (fp) {
        fclose(fp);
    }

    goto cleanup;
}

VOID
LsaCfgFreeParseState(
    PLSA_CONFIG_PARSE_STATE pParseState
    )
{
    LSA_SAFE_FREE_STRING(pParseState->pszFilePath);
    LSA_SAFE_FREE_STRING(pParseState->pszSectionName);
    if (pParseState->pLexerTokenStack) {
        LsaCfgFreeTokenStack(&pParseState->pLexerTokenStack);
    }

    if (pParseState->fp) {
        fclose(pParseState->fp);
    }
    LsaFreeMemory(pParseState);
}


DWORD
LsaCfgParse(
    PLSA_CONFIG_PARSE_STATE pParseState
    )
{

    DWORD dwError = 0;
    PLSA_CFG_TOKEN pToken = NULL;
    BOOLEAN bContinue = TRUE;
    PLSA_STACK pTokenStack = NULL;

    do
    {

        dwError = LsaCfgGetNextToken(
                        pParseState,
                        &pToken);
        BAIL_ON_LSA_ERROR(dwError);

        switch (pToken->tokenType)
        {
            case LsaCfgHash:
            {
                dwError = LsaCfgParseComment(
                                pParseState,
                                &bContinue);
                BAIL_ON_LSA_ERROR(dwError);

                break;
            }
            case LsaCfgNewline:
            {
                //in case of a blank line (newline),
                //interpret this as an empty comment.
                dwError = LsaCfgProcessComment(
                                pParseState,
                                &pTokenStack,
                                &bContinue);
                BAIL_ON_LSA_ERROR(dwError);

                break;
            }
            case LsaCfgLeftSquareBrace:
            {

                dwError = LsaCfgParseSections(
                                pParseState,
                                &bContinue);
                BAIL_ON_LSA_ERROR(dwError);

                break;
            }
            case LsaCfgEOF:
            {
                bContinue = FALSE;
                break;
            }
            default:
            {
                dwError = LW_ERROR_INVALID_CONFIG;
                BAIL_ON_LSA_ERROR(dwError);
            }
        }

    } while (bContinue);

cleanup:

    if (pToken) {
        LsaCfgFreeToken(pToken);
    }

    return dwError;

error:

    if (dwError == LW_ERROR_INVALID_CONFIG)
    {
        if (pParseState) {
            LSA_LOG_ERROR ("Parse error at line=%d, column=%d of file [%s]",
                          pParseState->dwLine,
                          pParseState->dwCol,
                          IsNullOrEmptyString(pParseState->pszFilePath) ?
                              "" : pParseState->pszFilePath);
        }
    }

    goto cleanup;
}

DWORD
LsaCfgParseSections(
    PLSA_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    PLSA_CFG_TOKEN pToken = NULL;
    BOOLEAN bContinue = TRUE;
    PLSA_STACK pTokenStack = NULL;

    dwError = LsaCfgParseSectionHeader(
                    pParseState,
                    &bContinue);
    BAIL_ON_LSA_ERROR(dwError);

    while (bContinue)
    {
        dwError = LsaCfgGetNextToken(
                        pParseState,
                        &pToken
                        );
        BAIL_ON_LSA_ERROR(dwError);

        switch (pToken->tokenType)
        {
            case LsaCfgString:
            {
                BOOLEAN bIsAllSpace = FALSE;

                dwError = LsaStrIsAllSpace(
                                pToken->pszToken,
                                &bIsAllSpace
                                );
                BAIL_ON_LSA_ERROR(dwError);

                if (bIsAllSpace)
                {
                    continue;
                }

                dwError = LsaStackPush(pToken, &(pParseState->pLexerTokenStack));
                BAIL_ON_LSA_ERROR(dwError);

                pToken = NULL;

                dwError = LsaCfgParseNameValuePair(
                                pParseState,
                                &bContinue
                                );
                BAIL_ON_LSA_ERROR(dwError);
                break;
            }

            case LsaCfgHash:
            {
                dwError = LsaCfgParseComment(
                                pParseState,
                                &bContinue
                                );
                BAIL_ON_LSA_ERROR(dwError);
                break;
            }
            case LsaCfgNewline:
            {
                //in case of a blank line (newline),
                //interpret this as an empty comment.
                dwError = LsaCfgProcessComment(
                                pParseState,
                                &pTokenStack,
                                &bContinue);
                BAIL_ON_LSA_ERROR(dwError);
                break;
            }
            case LsaCfgLeftSquareBrace:
            {
                dwError = LsaCfgParseSectionHeader(
                                pParseState,
                                &bContinue);
                BAIL_ON_LSA_ERROR(dwError);

                break;
            }
            case LsaCfgEOF:
            {
                bContinue = FALSE;
                break;
            }
            default:
            {
                dwError = LW_ERROR_INVALID_CONFIG;
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
    }

    if (bContinue && !IsNullOrEmptyString(pParseState->pszSectionName))
    {
        dwError = LsaCfgProcessEndSection(
                        pParseState,
                        &bContinue);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *pbContinue = bContinue;

cleanup:

    if (pToken) {
        LsaCfgFreeToken(pToken);
    }

    return dwError;

error:

    *pbContinue = FALSE;

    goto cleanup;
}



DWORD
LsaCfgParseComment(
    PLSA_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN    pbContinue
    )
{
    DWORD dwError = 0;
    BOOLEAN bContinue = TRUE;
    BOOLEAN bKeepParsing = TRUE;
    PLSA_CFG_TOKEN pToken = NULL;
    PLSA_STACK pTokenStack = NULL;

    do
    {
        dwError = LsaCfgGetNextToken(
                    pParseState,
                    &pToken);
        BAIL_ON_LSA_ERROR(dwError);

        switch (pToken->tokenType)
        {
            case LsaCfgEOF:
            {
                dwError = LsaCfgProcessComment(
                                pParseState,
                                &pTokenStack,
                                &bContinue);
                BAIL_ON_LSA_ERROR(dwError);

                bContinue = FALSE;

                break;
            }
            case LsaCfgNewline:
            {
                dwError = LsaCfgProcessComment(
                                pParseState,
                                &pTokenStack,
                                &bContinue);
                BAIL_ON_LSA_ERROR(dwError);

                bKeepParsing = FALSE;

                break;
            }
            default:
            {
                dwError = LsaStackPush(pToken, &pTokenStack);
                BAIL_ON_LSA_ERROR(dwError);

                pToken = NULL;

            }
        }

    } while (bContinue && bKeepParsing);

    *pbContinue = bContinue;

cleanup:

    if (pToken) {
        LsaCfgFreeToken(pToken);
    }

    return dwError;

error:

    *pbContinue = FALSE;

    goto cleanup;
}

DWORD
LsaCfgParseSectionHeader(
    PLSA_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    BOOLEAN bKeepParsing = TRUE;
    BOOLEAN bContinue = TRUE;
    PLSA_CFG_TOKEN pToken = NULL;
    PLSA_STACK pTokenStack = NULL;

    if(!IsNullOrEmptyString(pParseState->pszSectionName))
    {
        dwError = LsaCfgProcessEndSection(
                        pParseState,
                        &bContinue);
        BAIL_ON_LSA_ERROR(dwError);

    }

    if (!bContinue)
    {
        goto done;
    }

    pParseState->bSkipSection = FALSE;

    do
    {
        dwError = LsaCfgGetNextToken(
                        pParseState,
                        &pToken);
        BAIL_ON_LSA_ERROR(dwError);

        switch(pToken->tokenType)
        {
            case LsaCfgString:
            case LsaCfgEquals:
            case LsaCfgOther:
            {
                dwError = LsaStackPush(pToken, &pTokenStack);
                BAIL_ON_LSA_ERROR(dwError);

                pToken = NULL;
                break;
            }
            case LsaCfgRightSquareBrace:
            {
                dwError = LsaAssertWhitespaceOnly(pParseState);
                BAIL_ON_LSA_ERROR(dwError);

                bKeepParsing = FALSE;

                break;
            }
            default:
            {
                dwError = LW_ERROR_INVALID_CONFIG;
                BAIL_ON_LSA_ERROR(dwError);
            }
        }

    } while (bKeepParsing);

    dwError = LsaCfgGetNextToken(
                    pParseState,
                    &pToken);
    BAIL_ON_LSA_ERROR(dwError);

    switch(pToken->tokenType)
    {

        case LsaCfgNewline:
        {
            dwError = LsaCfgProcessBeginSection(
                            pParseState,
                            &pTokenStack,
                            &bContinue);
            BAIL_ON_LSA_ERROR(dwError);

            break;
        }
        case LsaCfgEOF:
        {
            dwError = LsaCfgProcessBeginSection(
                            pParseState,
                            &pTokenStack,
                            &bContinue);
            BAIL_ON_LSA_ERROR(dwError);

            if (bContinue) {

                dwError = LsaCfgProcessEndSection(
                                pParseState,
                                &bContinue);
                BAIL_ON_LSA_ERROR(dwError);
            }

            bContinue = FALSE;

            break;
        }
        default:
        {
            dwError = LW_ERROR_INVALID_CONFIG;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

done:

    *pbContinue = bContinue;

cleanup:

    if (pToken) {
        LsaCfgFreeToken(pToken);
    }

    return dwError;

error:

    *pbContinue = FALSE;

    goto cleanup;
}

DWORD
LsaAssertWhitespaceOnly(
    PLSA_CONFIG_PARSE_STATE pParseState
    )
{
    DWORD dwError = 0;
    PLSA_CFG_TOKEN pToken = NULL;
    BOOLEAN bKeepParsing = TRUE;

    do
    {
        dwError = LsaCfgGetNextToken(
                        pParseState,
                        &pToken);
        BAIL_ON_LSA_ERROR(dwError);

        switch(pToken->tokenType)
        {
            case LsaCfgString:
            case LsaCfgOther:
            {
                DWORD i = 0;
                for (; i < pToken->dwLen; i++) {
                    if (!isspace((int)pToken->pszToken[i])) {
                        dwError = LW_ERROR_INVALID_CONFIG;
                        BAIL_ON_LSA_ERROR(dwError);
                    }
                }
                break;
            }
            case LsaCfgEOF:
            case LsaCfgNewline:
            {
                dwError = LsaStackPush(pToken, &pParseState->pLexerTokenStack);
                BAIL_ON_LSA_ERROR(dwError);

                pToken = NULL;

                bKeepParsing = FALSE;

                break;
            }
            default:
            {
                dwError = LW_ERROR_INVALID_CONFIG;
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
    } while (bKeepParsing);

cleanup:

    if (pToken) {
        LsaCfgFreeToken(pToken);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaCfgParseNameValuePair(
    PLSA_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    BOOLEAN bContinue = TRUE;
    BOOLEAN bKeepParsing = TRUE;
    PLSA_CFG_TOKEN pToken = NULL;
    PLSA_STACK pTokenStack = NULL;

    //format is <str><equals><token1><token2>...<newline>

    //get initial <str>
    dwError = LsaCfgGetNextToken(
                    pParseState,
                    &pToken);
    BAIL_ON_LSA_ERROR(dwError);

    if(pToken->tokenType == LsaCfgString)
    {
        dwError = LsaStackPush(pToken, &pTokenStack);
        BAIL_ON_LSA_ERROR(dwError);
        pToken = NULL;
    }
    else
    {
        dwError = LW_ERROR_INVALID_CONFIG;
        BAIL_ON_LSA_ERROR(dwError);
    }

    //get <equals>
    dwError = LsaCfgGetNextToken(
                    pParseState,
                    &pToken);
    BAIL_ON_LSA_ERROR(dwError);

    if(pToken->tokenType == LsaCfgEquals)
    {
        dwError = LsaStackPush(pToken, &pTokenStack);
        BAIL_ON_LSA_ERROR(dwError);
        pToken = NULL;
    }
    else
    {
        dwError = LW_ERROR_INVALID_CONFIG;
        BAIL_ON_LSA_ERROR(dwError);
    }


    do
    {
        dwError = LsaCfgGetNextToken(
                        pParseState,
                        &pToken);
        BAIL_ON_LSA_ERROR(dwError);

        switch(pToken->tokenType)
        {
            case LsaCfgString:
            case LsaCfgEquals:
            case LsaCfgOther:
            {

                dwError = LsaStackPush(pToken, &pTokenStack);
                BAIL_ON_LSA_ERROR(dwError);
                pToken = NULL;

                break;

            }
            case LsaCfgNewline:
            {
                dwError = LsaCfgProcessNameValuePair(
                    pParseState,
                    &pTokenStack,
                    &bContinue);
                BAIL_ON_LSA_ERROR(dwError);
                bKeepParsing = FALSE;
                break;
            }
            case LsaCfgEOF:
            {
                dwError = LsaCfgProcessNameValuePair(
                    pParseState,
                    &pTokenStack,
                    &bContinue);
                BAIL_ON_LSA_ERROR(dwError);
                bContinue = FALSE;
                break;
            }
            default:
            {
                dwError = LW_ERROR_INVALID_CONFIG;
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
    } while(bContinue && bKeepParsing);


    *pbContinue = bContinue;

cleanup:

    if (pToken) {
        LsaCfgFreeToken(pToken);
    }

    return dwError;

error:

    *pbContinue = FALSE;

    goto cleanup;
}

DWORD
LsaCfgProcessComment(
    PLSA_CONFIG_PARSE_STATE pParseState,
    PLSA_STACK* ppTokenStack,
    PBOOLEAN    pbContinue
    )
{
    DWORD   dwError = 0;
    BOOLEAN bContinue = TRUE;
    PSTR    pszComment = NULL;

    dwError = LsaCfgProcessTokenStackIntoString(
        ppTokenStack,
        &pszComment);
    BAIL_ON_LSA_ERROR(dwError);

    if (pParseState->pfnCommentHandler &&
        !pParseState->bSkipSection) {

        dwError = pParseState->pfnCommentHandler(
                        pszComment,
                        pParseState->pData,
                        &bContinue);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *pbContinue = bContinue;

cleanup:

    LSA_SAFE_FREE_STRING(pszComment);

    return dwError;

error:

    *pbContinue = TRUE;

    goto cleanup;
}

DWORD
LsaCfgProcessBeginSection(
    PLSA_CONFIG_PARSE_STATE pParseState,
    PLSA_STACK* ppTokenStack,
    PBOOLEAN    pbContinue)
{
    DWORD   dwError = 0;
    PSTR    pszSectionName = NULL;
    BOOLEAN bSkipSection = FALSE;
    BOOLEAN bContinue = TRUE;

    dwError = LsaCfgProcessTokenStackIntoString(
        ppTokenStack,
        &pszSectionName);
    BAIL_ON_LSA_ERROR(dwError);

    if (IsNullOrEmptyString(pszSectionName)) {
        dwError = LW_ERROR_INVALID_CONFIG;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pParseState->pfnStartSectionHandler) {

        if(pParseState->dwOptions & LSA_CFG_OPTION_STRIP_SECTION)
        {
            LsaStripWhitespace(pszSectionName, TRUE, TRUE);
        }

        dwError = pParseState->pfnStartSectionHandler(
                        pszSectionName,
                        pParseState->pData,
                        &bSkipSection,
                        &bContinue);
        BAIL_ON_LSA_ERROR(dwError);
    }

    pParseState->pszSectionName = pszSectionName;
    pParseState->bSkipSection = bSkipSection;
    *pbContinue = bContinue;

cleanup:

    return dwError;

error:

    LSA_SAFE_FREE_STRING(pszSectionName);
    pParseState->pszSectionName = NULL;
    pParseState->bSkipSection = FALSE;
    *pbContinue = FALSE;

    goto cleanup;
}

DWORD
LsaCfgProcessNameValuePair(
    PLSA_CONFIG_PARSE_STATE pParseState,
    PLSA_STACK*             ppTokenStack,
    PBOOLEAN                pbContinue
    )
{
    DWORD      dwError = 0;
    BOOLEAN    bContinue = TRUE;
    PSTR       pszName = NULL;
    PSTR       pszValue = NULL;
    PLSA_CFG_TOKEN pToken = NULL;

    *ppTokenStack = LsaStackReverse(*ppTokenStack);
    pToken = (PLSA_CFG_TOKEN)LsaStackPop(ppTokenStack);
    if (pToken && pToken->dwLen) {
        dwError = LsaStrndup(
                    pToken->pszToken,
                    pToken->dwLen,
                    &pszName);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (IsNullOrEmptyString(pszName)) {
        dwError = LW_ERROR_INVALID_CONFIG;
        BAIL_ON_LSA_ERROR(dwError);
    }

    LsaCfgFreeToken(pToken);
    pToken = NULL;

    pToken = (PLSA_CFG_TOKEN)LsaStackPop(ppTokenStack);
    if (!pToken || pToken->tokenType != LsaCfgEquals)
    {
        dwError = LW_ERROR_INVALID_CONFIG;
        BAIL_ON_LSA_ERROR(dwError);
    }

    LsaCfgFreeToken(pToken);
    pToken = NULL;

    //this will consume the token stack
    dwError = LsaCfgProcessTokenStackIntoString(
        ppTokenStack,
        &pszValue);
    BAIL_ON_LSA_ERROR(dwError);

    if (pParseState->pfnNameValuePairHandler  &&
        !pParseState->bSkipSection) {

        if(pParseState->dwOptions & LSA_CFG_OPTION_STRIP_NAME_VALUE_PAIR)
        {
            LsaStripWhitespace(pszName, TRUE, TRUE);
            LsaStripWhitespace(pszValue, TRUE, TRUE);
        }

        dwError = pParseState->pfnNameValuePairHandler(
                        pszName,
                        pszValue,
                        pParseState->pData,
                        &bContinue);
        BAIL_ON_LSA_ERROR(dwError);

    }

    *pbContinue = bContinue;

cleanup:

    if (pToken) {
        LsaCfgFreeToken(pToken);
        pToken = NULL;
    }

    if(ppTokenStack && *ppTokenStack)
    {
        dwError = LsaCfgFreeTokenStack(ppTokenStack);
    }

    LSA_SAFE_FREE_STRING(pszName);
    LSA_SAFE_FREE_STRING(pszValue);

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaCfgProcessEndSection(
    PLSA_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN                pbContinue
    )
{
    DWORD dwError = 0;
    BOOLEAN bContinue = TRUE;

    if (pParseState->pfnEndSectionHandler &&
        !pParseState->bSkipSection) {

        if(pParseState->dwOptions & LSA_CFG_OPTION_STRIP_SECTION)
        {
            LsaStripWhitespace(pParseState->pszSectionName, TRUE, TRUE);
        }


        dwError = pParseState->pfnEndSectionHandler(
                        pParseState->pszSectionName,
                        pParseState->pData,
                        &bContinue);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *pbContinue = bContinue;

cleanup:

    LSA_SAFE_FREE_STRING(pParseState->pszSectionName);

    return dwError;

error:

    *pbContinue = TRUE;

    goto cleanup;
}

DWORD
LsaCfgDetermineTokenLength(
    PLSA_STACK pStack
    )
{
    DWORD dwLen = 0;
    PLSA_STACK pIter = pStack;

    for (; pIter; pIter = pIter->pNext)
    {
        PLSA_CFG_TOKEN pToken = (PLSA_CFG_TOKEN)pIter->pItem;
        if (pToken) {
            dwLen += pToken->dwLen;
        }
    }

    return dwLen;
}

//this will consume the token stack
DWORD
LsaCfgProcessTokenStackIntoString(
    PLSA_STACK* ppTokenStack,
    PSTR* ppszConcatenated
    )
{
    DWORD   dwError = 0;
    DWORD   dwRequiredTokenLen = 0;
    PSTR    pszConcatenated = NULL;

    dwRequiredTokenLen = LsaCfgDetermineTokenLength(*ppTokenStack);

    if (dwRequiredTokenLen) {

        PSTR pszPos = NULL;
        PLSA_CFG_TOKEN pToken = NULL;

        *ppTokenStack = LsaStackReverse(*ppTokenStack);


        dwError = LsaAllocateMemory(
                        (dwRequiredTokenLen + 1) * sizeof(CHAR),
                        (PVOID*)&pszConcatenated);
        BAIL_ON_LSA_ERROR(dwError);


        pszPos = pszConcatenated;
        while (*ppTokenStack)
        {
            pToken = LsaStackPop(ppTokenStack);
            if (pToken && pToken->dwLen) {

                strncpy(pszPos, pToken->pszToken, pToken->dwLen);
                pszPos += pToken->dwLen;

                LsaCfgFreeToken(pToken);
                pToken = NULL;
            }
        }
    }

    *ppszConcatenated = pszConcatenated;

cleanup:

    return dwError;

error:

    LSA_SAFE_FREE_STRING(pszConcatenated);

    *ppszConcatenated = NULL;

    goto cleanup;
}


DWORD
LsaCfgAllocateToken(
    DWORD           dwSize,
    PLSA_CFG_TOKEN* ppToken
    )
{
    DWORD dwError = 0;
    PLSA_CFG_TOKEN pToken = NULL;
    DWORD dwMaxLen = (dwSize > 0 ? dwSize : LSA_CFG_TOKEN_DEFAULT_LENGTH);


    dwError = LsaAllocateMemory(
                    sizeof(LSA_CFG_TOKEN),
                    (PVOID*)&pToken);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateMemory(
                    dwMaxLen * sizeof(CHAR),
                    (PVOID*)&pToken->pszToken);
    BAIL_ON_LSA_ERROR(dwError);


    pToken->tokenType = LsaCfgNone;
    pToken->dwMaxLen = dwMaxLen;

    *ppToken = pToken;

cleanup:

    return dwError;

error:

    *ppToken = NULL;

    if (pToken) {
        LsaCfgFreeToken(pToken);
    }

    goto cleanup;
}

DWORD
LsaCfgReallocToken(
    PLSA_CFG_TOKEN pToken,
    DWORD          dwNewSize
    )
{
    DWORD dwError = 0;

    dwError = LsaReallocMemory(
                    pToken->pszToken,
                    (PVOID*)&pToken->pszToken,
                    dwNewSize);
    BAIL_ON_LSA_ERROR(dwError);

    pToken->dwMaxLen = dwNewSize;

cleanup:

    return dwError;

error:

    goto cleanup;
}

VOID
LsaCfgResetToken(
    PLSA_CFG_TOKEN pToken
    )
{
    if (pToken && pToken->pszToken && pToken->dwMaxLen)
    {
        memset(pToken->pszToken, 0, pToken->dwMaxLen);
        pToken->dwLen = 0;
    }
}

DWORD
LsaCfgCopyToken(
    PLSA_CFG_TOKEN pTokenSrc,
    PLSA_CFG_TOKEN pTokenDst
    )
{
    DWORD dwError = 0;

    pTokenDst->tokenType = pTokenSrc->tokenType;

    if (pTokenSrc->dwLen > pTokenDst->dwLen) {
        dwError = LsaReallocMemory(
                        (PVOID)  pTokenDst->pszToken,
                        (PVOID*) &pTokenDst->pszToken,
                        (DWORD)  pTokenSrc->dwLen);
        BAIL_ON_LSA_ERROR(dwError);

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
LsaCfgFreeTokenStack(
    PLSA_STACK* ppTokenStack
    )
{

    DWORD dwError = 0;

    PLSA_STACK pTokenStack = *ppTokenStack;

    dwError = LsaStackForeach(
            pTokenStack,
            &LsaCfgFreeTokenInStack,
            NULL);
    BAIL_ON_LSA_ERROR(dwError);

    LsaStackFree(pTokenStack);

    *ppTokenStack = NULL;

error:

    return dwError;
}

DWORD
LsaCfgFreeTokenInStack(
    PVOID pToken,
    PVOID pUserData
    )
{
    if (pToken)
    {
        LsaCfgFreeToken((PLSA_CFG_TOKEN)pToken);
    }

    return 0;
}

VOID
LsaCfgFreeToken(
    PLSA_CFG_TOKEN pToken
    )
{
    LSA_SAFE_FREE_MEMORY(pToken->pszToken);
    LsaFreeMemory(pToken);
}

DWORD
LsaCfgGetNextToken(
    PLSA_CONFIG_PARSE_STATE pParseState,
    PLSA_CFG_TOKEN*         ppToken
    )
{
    DWORD dwError = 0;
    LsaCfgTokenType tokenType = LsaCfgNone;
    LsaCfgLexState  curLexState = LsaLexBegin;
    PLSA_CFG_TOKEN  pToken = NULL;
    BOOLEAN         bOwnToken = FALSE;

    if (LsaStackPeek(pParseState->pLexerTokenStack) != NULL)
    {
        PLSA_CFG_TOKEN pToken_input = *ppToken;

        pToken = (PLSA_CFG_TOKEN)LsaStackPop(&pParseState->pLexerTokenStack);
        bOwnToken = TRUE;

        if (pToken_input) {

            dwError = LsaCfgCopyToken(pToken, pToken_input);
            BAIL_ON_LSA_ERROR(dwError);

            LsaCfgFreeToken(pToken);
            pToken = NULL;
            bOwnToken = FALSE;

        }

        goto done;
    }

    pToken = *ppToken;

    if (!pToken) {
        dwError = LsaCfgAllocateToken(
                        0,
                        &pToken);
        BAIL_ON_LSA_ERROR(dwError);

        bOwnToken = TRUE;
    }
    else
    {
        LsaCfgResetToken(pToken);
    }

    while (curLexState != LsaLexEnd)
    {
        DWORD ch = LsaCfgGetCharacter(pParseState);
        LsaCfgLexState lexClass = LsaCfgGetLexClass(ch);

        if (lexClass != LsaLexEOF) {
            pParseState->dwCol++;
        }

        if (lexClass == LsaLexNewline) {
            pParseState->dwLine++;
            pParseState->dwCol = 0;
        }

        tokenType = LsaCfgGetTokenType(curLexState, lexClass);

        switch(LsaCfgGetLexAction(curLexState, lexClass))
        {
            case Skip:

                break;

            case Consume:

                if (pToken->dwLen >= pToken->dwMaxLen) {
                    dwError = LsaCfgReallocToken(
                                    pToken,
                                    pToken->dwMaxLen + LSA_CFG_TOKEN_DEFAULT_LENGTH);
                    BAIL_ON_LSA_ERROR(dwError);
                }

                pToken->pszToken[pToken->dwLen++] = (BYTE)ch;

                break;

            case Pushback:

	        if (lexClass == LsaLexNewline)
	        {
                    pParseState->dwLine--;
	        }
                pParseState->dwCol--;
                dwError = LsaCfgPushBackCharacter(
                                pParseState,
                                (BYTE)ch);
                BAIL_ON_LSA_ERROR(dwError);

                break;
        }

        curLexState = LsaCfgGetNextLexState(curLexState, lexClass);
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
        LsaCfgFreeToken(pToken);
        *ppToken = NULL;
    }

    goto cleanup;
}

DWORD
LsaCfgGetCharacter(
    PLSA_CONFIG_PARSE_STATE pParseState
    )
{
    return getc(pParseState->fp);
}

LsaCfgLexState
LsaCfgGetLexClass(
    DWORD ch
    )
{

    if (ch == EOF) {
        return LsaLexEOF;
    }

    if (ch == '\n') {
        return LsaLexNewline;
    }

    if (ch == '[') {
        return LsaLexLSqBrace;
    }

    if (ch == ']') {
        return LsaLexRSqBrace;
    }

    if (ch == '=') {
        return LsaLexEquals;
    }

    if (ch == '#') {
        return LsaLexHash;
    }

    if (isalnum(ch) ||
        ispunct(ch) ||
        isblank(ch)) {
        return LsaLexChar;
    }

    return LsaLexOther;
}

DWORD
LsaCfgPushBackCharacter(
    PLSA_CONFIG_PARSE_STATE pParseState,
    BYTE ch
    )
{
    return ((EOF == ungetc(ch, pParseState->fp)) ? errno : 0);
}

LsaCfgLexState
LsaCfgGetNextLexState(
    LsaCfgLexState currentState,
    DWORD chId
    )
{
    return (gLsaLexStateTable[currentState][chId].nextState);
}

LsaCfgLexAction
LsaCfgGetLexAction(
    LsaCfgLexState currentState,
    DWORD chId
    )
{
    return (gLsaLexStateTable[currentState][chId].action);
}

LsaCfgTokenType
LsaCfgGetTokenType(
    LsaCfgLexState currentState,
    DWORD chId
    )
{
    return (gLsaLexStateTable[currentState][chId].tokenId);
}
