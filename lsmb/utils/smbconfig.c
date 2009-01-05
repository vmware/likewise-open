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
 *        smbconfig.c
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
#include "includes.h"

static SMB_CFG_LEXER_STATE gSMBLexStateTable[][9] =
{
  /* SMBLexBegin    := 0 */
  {
    {   SMBLexBegin,  Consume,             SMBCfgNone }, /* SMBLexBegin    */
    {    SMBLexChar,  Consume,             SMBCfgNone }, /* SMBLexChar     */
    {     SMBLexEnd,  Consume,  SMBCfgLeftSquareBrace }, /* SMBLexLSqBrace */
    {     SMBLexEnd,  Consume, SMBCfgRightSquareBrace }, /* SMBLexRSqBrace */
    {     SMBLexEnd,  Consume,           SMBCfgEquals }, /* SMBLexEquals   */
    {     SMBLexEnd,  Consume,             SMBCfgHash }, /* SMBLexHash     */
    {     SMBLexEnd,  Consume,          SMBCfgNewline }, /* SMBLexNewline  */
    {   SMBLexBegin,     Skip,             SMBCfgNone }, /* SMBLexOther    */
    {     SMBLexEnd,  Consume,              SMBCfgEOF }  /* SMBLexEOF      */
  },
  /* SMBLexChar     := 1 */
  {
    {   SMBLexBegin,  Consume,             SMBCfgNone }, /* SMBLexBegin    */
    {    SMBLexChar,  Consume,             SMBCfgNone }, /* SMBLexChar     */
    {     SMBLexEnd, Pushback,           SMBCfgString }, /* SMBLexLSqBrace */
    {     SMBLexEnd, Pushback,           SMBCfgString }, /* SMBLexRSqBrace */
    {     SMBLexEnd, Pushback,           SMBCfgString }, /* SMBLexEquals   */
    {     SMBLexEnd, Pushback,           SMBCfgString }, /* SMBLexHash     */
    {     SMBLexEnd, Pushback,           SMBCfgString }, /* SMBLexNewline  */
    {     SMBLexEnd,     Skip,           SMBCfgString }, /* SMBLexOther    */
    {     SMBLexEnd,     Skip,           SMBCfgString }  /* SMBLexEOF      */
  },
  /* SMBLexLSqBrace := 2 */
  {
    {   SMBLexBegin,  Consume,             SMBCfgNone }, /* SMBLexBegin    */
    {     SMBLexEnd, Pushback,  SMBCfgLeftSquareBrace }, /* SMBLexChar     */
    {     SMBLexEnd, Pushback,  SMBCfgLeftSquareBrace }, /* SMBLexLSqBrace */
    {     SMBLexEnd, Pushback,  SMBCfgLeftSquareBrace }, /* SMBLexRSqBrace */
    {     SMBLexEnd, Pushback,  SMBCfgLeftSquareBrace }, /* SMBLexEquals   */
    {     SMBLexEnd, Pushback,  SMBCfgLeftSquareBrace }, /* SMBLexHash     */
    {     SMBLexEnd, Pushback,  SMBCfgLeftSquareBrace }, /* SMBLexNewline  */
    {     SMBLexEnd,     Skip,  SMBCfgLeftSquareBrace }, /* SMBLexOther    */
    {     SMBLexEnd,     Skip,  SMBCfgLeftSquareBrace }  /* SMBLexEOF      */
  },
  /* SMBLexRSqBrace := 3 */
  {
    {   SMBLexBegin,  Consume,             SMBCfgNone }, /* SMBLexBegin    */
    {     SMBLexEnd, Pushback, SMBCfgRightSquareBrace }, /* SMBLexChar     */
    {     SMBLexEnd, Pushback, SMBCfgRightSquareBrace }, /* SMBLexLSqBrace */
    {     SMBLexEnd, Pushback, SMBCfgRightSquareBrace }, /* SMBLexRSqBrace */
    {     SMBLexEnd, Pushback, SMBCfgRightSquareBrace }, /* SMBLexEquals   */
    {     SMBLexEnd, Pushback, SMBCfgRightSquareBrace }, /* SMBLexHash     */
    {     SMBLexEnd, Pushback, SMBCfgRightSquareBrace }, /* SMBLexNewline  */
    {     SMBLexEnd,     Skip, SMBCfgRightSquareBrace }, /* SMBLexOther    */
    {     SMBLexEnd,     Skip, SMBCfgRightSquareBrace }  /* SMBLexEOF      */
  },
  /* SMBLexEquals   := 4 */
  {
    {   SMBLexBegin,  Consume,             SMBCfgNone }, /* SMBLexBegin    */
    {     SMBLexEnd, Pushback,           SMBCfgEquals }, /* SMBLexChar     */
    {     SMBLexEnd, Pushback,           SMBCfgEquals }, /* SMBLexLSqBrace */
    {     SMBLexEnd, Pushback,           SMBCfgEquals }, /* SMBLexRSqBrace */
    {     SMBLexEnd, Pushback,           SMBCfgEquals }, /* SMBLexEquals   */
    {     SMBLexEnd, Pushback,           SMBCfgEquals }, /* SMBLexHash     */
    {     SMBLexEnd, Pushback,           SMBCfgEquals }, /* SMBLexNewline  */
    {     SMBLexEnd,     Skip,           SMBCfgEquals }, /* SMBLexOther    */
    {     SMBLexEnd,     Skip,           SMBCfgEquals }  /* SMBLexEOF      */
  },
  /* SMBLexHash     := 5 */
  {
    {   SMBLexBegin,  Consume,             SMBCfgNone }, /* SMBLexBegin    */
    {     SMBLexEnd, Pushback,             SMBCfgHash }, /* SMBLexChar     */
    {     SMBLexEnd, Pushback,             SMBCfgHash }, /* SMBLexLSqBrace */
    {     SMBLexEnd, Pushback,             SMBCfgHash }, /* SMBLexRSqBrace */
    {     SMBLexEnd, Pushback,             SMBCfgHash }, /* SMBLexEquals   */
    {     SMBLexEnd, Pushback,             SMBCfgHash }, /* SMBLexHash     */
    {     SMBLexEnd, Pushback,             SMBCfgHash }, /* SMBLexNewline  */
    {     SMBLexEnd,     Skip,             SMBCfgHash }, /* SMBLexOther    */
    {     SMBLexEnd,     Skip,             SMBCfgHash }  /* SMBLexEOF      */
  },
  /* SMBLexNewline  := 6 */
  {
    {   SMBLexBegin,  Consume,             SMBCfgNone }, /* SMBLexBegin    */
    {     SMBLexEnd, Pushback,          SMBCfgNewline }, /* SMBLexChar     */
    {     SMBLexEnd, Pushback,          SMBCfgNewline }, /* SMBLexLSqBrace */
    {     SMBLexEnd, Pushback,          SMBCfgNewline }, /* SMBLexRSqBrace */
    {     SMBLexEnd, Pushback,          SMBCfgNewline }, /* SMBLexEquals   */
    {     SMBLexEnd, Pushback,          SMBCfgNewline }, /* SMBLexHash     */
    {     SMBLexEnd, Pushback,          SMBCfgNewline }, /* SMBLexNewline  */
    {     SMBLexEnd,     Skip,          SMBCfgNewline }, /* SMBLexOther    */
    {     SMBLexEnd,     Skip,          SMBCfgNewline }  /* SMBLexEOF      */
  },
  /* SMBLexOther    := 7 */
  {
    {   SMBLexBegin,  Consume,             SMBCfgNone }, /* SMBLexBegin    */
    {    SMBLexChar,  Consume,             SMBCfgNone }, /* SMBLexChar     */
    {SMBLexLSqBrace,  Consume,             SMBCfgNone }, /* SMBLexLSqBrace */
    {SMBLexRSqBrace,  Consume,             SMBCfgNone }, /* SMBLexRSqBrace */
    {  SMBLexEquals,  Consume,             SMBCfgNone }, /* SMBLexEquals   */
    {    SMBLexHash,  Consume,             SMBCfgNone }, /* SMBLexHash     */
    { SMBLexNewline,  Consume,             SMBCfgNone }, /* SMBLexNewline  */
    {   SMBLexOther,  Consume,             SMBCfgNone }, /* SMBLexOther    */
    {     SMBLexEOF,  Consume,             SMBCfgNone }  /* SMBLexEOF      */
  },
  /* SMBLexEOF      := 8 */
  {
    {   SMBLexBegin,  Consume,             SMBCfgNone }, /* SMBLexBegin    */
    {     SMBLexEnd,  Consume,             SMBCfgNone }, /* SMBLexChar     */
    {     SMBLexEnd,  Consume,             SMBCfgNone }, /* SMBLexLSqBrace */
    {     SMBLexEnd,  Consume,             SMBCfgNone }, /* SMBLexRSqBrace */
    {     SMBLexEnd,  Consume,             SMBCfgNone }, /* SMBLexEquals   */
    {     SMBLexEnd,  Consume,             SMBCfgNone }, /* SMBLexHash     */
    { SMBLexNewline,  Consume,             SMBCfgNone }, /* SMBLexNewline  */
    {     SMBLexEnd,  Consume,             SMBCfgNone }, /* SMBLexOther    */
    {     SMBLexEnd,  Consume,             SMBCfgNone }  /* SMBLexEOF      */
  }
};

DWORD
SMBParseConfigFile(
    PCSTR                     pszFilePath,
    DWORD                     dwOptions,
    PFNSMB_CONFIG_START_SECTION   pfnStartSectionHandler,
    PFNSMB_CONFIG_COMMENT         pfnCommentHandler,
    PFNSMB_CONFIG_NAME_VALUE_PAIR pfnNameValuePairHandler,
    PFNSMB_CONFIG_END_SECTION     pfnEndSectionHandler,
    PVOID                     pData
    )
{
    DWORD dwError = 0;
    PSMB_CONFIG_PARSE_STATE pParseState = NULL;

    dwError = SMBCfgInitParseState(
                    pszFilePath,
                    dwOptions,
                    pfnStartSectionHandler,
                    pfnCommentHandler,
                    pfnNameValuePairHandler,
                    pfnEndSectionHandler,
                    pData,
                    &pParseState);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBCfgParse(pParseState);
    BAIL_ON_SMB_ERROR(dwError);

cleanup:

    if (pParseState) {
        SMBCfgFreeParseState(pParseState);
    }

    return dwError;

error:

    goto cleanup;
}

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
    )
{
    DWORD dwError = 0;
    PSMB_CONFIG_PARSE_STATE pParseState = NULL;
    PSMB_STACK pTokenStack = NULL;
    FILE* fp = NULL;

    if ((fp = fopen(pszFilePath, "r")) == NULL) {
        dwError = errno;
        BAIL_ON_SMB_ERROR(dwError);
    }


    dwError = SMBAllocateMemory(
                    sizeof(SMB_CONFIG_PARSE_STATE),
                    (PVOID*)&pParseState);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBAllocateMemory(
                    sizeof(SMB_STACK),
                    (PVOID*)&pTokenStack);
    BAIL_ON_SMB_ERROR(dwError);

    pParseState->pLexerTokenStack = pTokenStack;

    dwError = SMBAllocateString(
                    pszFilePath,
                    &(pParseState->pszFilePath));
    BAIL_ON_SMB_ERROR(dwError);

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
        SMBCfgFreeParseState(pParseState);
    }

    if (fp) {
        fclose(fp);
    }

    goto cleanup;
}

VOID
SMBCfgFreeParseState(
    PSMB_CONFIG_PARSE_STATE pParseState
    )
{
    SMB_SAFE_FREE_STRING(pParseState->pszFilePath);
    SMB_SAFE_FREE_STRING(pParseState->pszSectionName);
    if (pParseState->pLexerTokenStack) {
        SMBCfgFreeTokenStack(&pParseState->pLexerTokenStack);
    }

    if (pParseState->fp) {
        fclose(pParseState->fp);
    }
    SMBFreeMemory(pParseState);
}


DWORD
SMBCfgParse(
    PSMB_CONFIG_PARSE_STATE pParseState
    )
{

    DWORD dwError = 0;
    PSMB_CFG_TOKEN pToken = NULL;
    BOOLEAN bContinue = TRUE;
    PSMB_STACK pTokenStack = NULL;

    do
    {

        dwError = SMBCfgGetNextToken(
                        pParseState,
                        &pToken);
        BAIL_ON_SMB_ERROR(dwError);

        switch (pToken->tokenType)
        {
            case SMBCfgHash:
            {
                dwError = SMBCfgParseComment(
                                pParseState,
                                &bContinue);
                BAIL_ON_SMB_ERROR(dwError);

                break;
            }
            case SMBCfgNewline:
            {
                //in case of a blank line (newline),
                //interpret this as an empty comment.
                dwError = SMBCfgProcessComment(
                                pParseState,
                                &pTokenStack,
                                &bContinue);
                BAIL_ON_SMB_ERROR(dwError);

                break;
            }
            case SMBCfgLeftSquareBrace:
            {

                dwError = SMBCfgParseSections(
                                pParseState,
                                &bContinue);
                BAIL_ON_SMB_ERROR(dwError);

                break;
            }
            case SMBCfgEOF:
            {
                bContinue = FALSE;
                break;
            }
            default:
            {
                dwError = SMB_ERROR_INVALID_CONFIG;
                BAIL_ON_SMB_ERROR(dwError);
            }
        }

    } while (bContinue);

cleanup:

    if (pToken) {
        SMBCfgFreeToken(pToken);
    }

    return dwError;

error:

    if (dwError == SMB_ERROR_INVALID_CONFIG)
    {
        if (pParseState) {
            SMB_LOG_ERROR ("Parse error at line=%d, column=%d of file [%s]",
                          pParseState->dwLine,
                          pParseState->dwCol,
                          IsNullOrEmptyString(pParseState->pszFilePath) ?
                              "" : pParseState->pszFilePath);
        }
    }

    goto cleanup;
}

DWORD
SMBCfgParseSections(
    PSMB_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    PSMB_CFG_TOKEN pToken = NULL;
    BOOLEAN bContinue = TRUE;
    PSMB_STACK pTokenStack = NULL;

    dwError = SMBCfgParseSectionHeader(
                    pParseState,
                    &bContinue);
    BAIL_ON_SMB_ERROR(dwError);

    while (bContinue)
    {
        dwError = SMBCfgGetNextToken(
                        pParseState,
                        &pToken
                        );
        BAIL_ON_SMB_ERROR(dwError);

        switch (pToken->tokenType)
        {
            case SMBCfgString:
            {
                BOOLEAN bIsAllSpace = FALSE;

                dwError = SMBStrIsAllSpace(
                                pToken->pszToken,
                                &bIsAllSpace
                                );
                BAIL_ON_SMB_ERROR(dwError);

                if (bIsAllSpace)
                {
                    continue;
                }

                dwError = SMBStackPush(pToken, &(pParseState->pLexerTokenStack));
                BAIL_ON_SMB_ERROR(dwError);

                pToken = NULL;

                dwError = SMBCfgParseNameValuePair(
                                pParseState,
                                &bContinue
                                );
                BAIL_ON_SMB_ERROR(dwError);
                break;
            }

            case SMBCfgHash:
            {
                dwError = SMBCfgParseComment(
                                pParseState,
                                &bContinue
                                );
                BAIL_ON_SMB_ERROR(dwError);
                break;
            }
            case SMBCfgNewline:
            {
                //in case of a blank line (newline),
                //interpret this as an empty comment.
                dwError = SMBCfgProcessComment(
                                pParseState,
                                &pTokenStack,
                                &bContinue);
                BAIL_ON_SMB_ERROR(dwError);
                break;
            }
            case SMBCfgLeftSquareBrace:
            {
                dwError = SMBCfgParseSectionHeader(
                                pParseState,
                                &bContinue);
                BAIL_ON_SMB_ERROR(dwError);

                break;
            }
            case SMBCfgEOF:
            {
                bContinue = FALSE;
                break;
            }
            default:
            {
                dwError = SMB_ERROR_INVALID_CONFIG;
                BAIL_ON_SMB_ERROR(dwError);
            }
        }
    }

    if (bContinue && !IsNullOrEmptyString(pParseState->pszSectionName))
    {
        dwError = SMBCfgProcessEndSection(
                        pParseState,
                        &bContinue);
        BAIL_ON_SMB_ERROR(dwError);
    }

    *pbContinue = bContinue;

cleanup:

    if (pToken) {
        SMBCfgFreeToken(pToken);
    }

    return dwError;

error:

    *pbContinue = FALSE;

    goto cleanup;
}



DWORD
SMBCfgParseComment(
    PSMB_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN    pbContinue
    )
{
    DWORD dwError = 0;
    BOOLEAN bContinue = TRUE;
    BOOLEAN bKeepParsing = TRUE;
    PSMB_CFG_TOKEN pToken = NULL;
    PSMB_STACK pTokenStack = NULL;

    do
    {
        dwError = SMBCfgGetNextToken(
                    pParseState,
                    &pToken);
        BAIL_ON_SMB_ERROR(dwError);

        switch (pToken->tokenType)
        {
            case SMBCfgEOF:
            {
                dwError = SMBCfgProcessComment(
                                pParseState,
                                &pTokenStack,
                                &bContinue);
                BAIL_ON_SMB_ERROR(dwError);

                bContinue = FALSE;

                break;
            }
            case SMBCfgNewline:
            {
                dwError = SMBCfgProcessComment(
                                pParseState,
                                &pTokenStack,
                                &bContinue);
                BAIL_ON_SMB_ERROR(dwError);

                bKeepParsing = FALSE;

                break;
            }
            default:
            {
                dwError = SMBStackPush(pToken, &pTokenStack);
                BAIL_ON_SMB_ERROR(dwError);

                pToken = NULL;

            }
        }

    } while (bContinue && bKeepParsing);

    *pbContinue = bContinue;

cleanup:

    if (pToken) {
        SMBCfgFreeToken(pToken);
    }

    return dwError;

error:

    *pbContinue = FALSE;

    goto cleanup;
}

DWORD
SMBCfgParseSectionHeader(
    PSMB_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    BOOLEAN bKeepParsing = TRUE;
    BOOLEAN bContinue = TRUE;
    PSMB_CFG_TOKEN pToken = NULL;
    PSMB_STACK pTokenStack = NULL;

    if(!IsNullOrEmptyString(pParseState->pszSectionName))
    {
        dwError = SMBCfgProcessEndSection(
                        pParseState,
                        &bContinue);
        BAIL_ON_SMB_ERROR(dwError);

    }

    if (!bContinue)
    {
        goto done;
    }

    pParseState->bSkipSection = FALSE;

    do
    {
        dwError = SMBCfgGetNextToken(
                        pParseState,
                        &pToken);
        BAIL_ON_SMB_ERROR(dwError);

        switch(pToken->tokenType)
        {
            case SMBCfgString:
            case SMBCfgEquals:
            case SMBCfgOther:
            {
                dwError = SMBStackPush(pToken, &pTokenStack);
                BAIL_ON_SMB_ERROR(dwError);

                pToken = NULL;
                break;
            }
            case SMBCfgRightSquareBrace:
            {
                dwError = SMBAssertWhitespaceOnly(pParseState);
                BAIL_ON_SMB_ERROR(dwError);

                bKeepParsing = FALSE;

                break;
            }
            default:
            {
                dwError = SMB_ERROR_INVALID_CONFIG;
                BAIL_ON_SMB_ERROR(dwError);
            }
        }

    } while (bKeepParsing);

    dwError = SMBCfgGetNextToken(
                    pParseState,
                    &pToken);
    BAIL_ON_SMB_ERROR(dwError);

    switch(pToken->tokenType)
    {

        case SMBCfgNewline:
        {
            dwError = SMBCfgProcessBeginSection(
                            pParseState,
                            &pTokenStack,
                            &bContinue);
            BAIL_ON_SMB_ERROR(dwError);

            break;
        }
        case SMBCfgEOF:
        {
            dwError = SMBCfgProcessBeginSection(
                            pParseState,
                            &pTokenStack,
                            &bContinue);
            BAIL_ON_SMB_ERROR(dwError);

            if (bContinue) {

                dwError = SMBCfgProcessEndSection(
                                pParseState,
                                &bContinue);
                BAIL_ON_SMB_ERROR(dwError);
            }

            bContinue = FALSE;

            break;
        }
        default:
        {
            dwError = SMB_ERROR_INVALID_CONFIG;
            BAIL_ON_SMB_ERROR(dwError);
        }
    }

done:

    *pbContinue = bContinue;

cleanup:

    if (pToken) {
        SMBCfgFreeToken(pToken);
    }

    return dwError;

error:

    *pbContinue = FALSE;

    goto cleanup;
}

DWORD
SMBAssertWhitespaceOnly(
    PSMB_CONFIG_PARSE_STATE pParseState
    )
{
    DWORD dwError = 0;
    PSMB_CFG_TOKEN pToken = NULL;
    BOOLEAN bKeepParsing = TRUE;

    do
    {
        dwError = SMBCfgGetNextToken(
                        pParseState,
                        &pToken);
        BAIL_ON_SMB_ERROR(dwError);

        switch(pToken->tokenType)
        {
            case SMBCfgString:
            case SMBCfgOther:
            {
                DWORD i = 0;
                for (; i < pToken->dwLen; i++) {
                    if (!isspace((int)pToken->pszToken[i])) {
                        dwError = SMB_ERROR_INVALID_CONFIG;
                        BAIL_ON_SMB_ERROR(dwError);
                    }
                }
                break;
            }
            case SMBCfgEOF:
            case SMBCfgNewline:
            {
                dwError = SMBStackPush(pToken, &pParseState->pLexerTokenStack);
                BAIL_ON_SMB_ERROR(dwError);

                pToken = NULL;

                bKeepParsing = FALSE;

                break;
            }
            default:
            {
                dwError = SMB_ERROR_INVALID_CONFIG;
                BAIL_ON_SMB_ERROR(dwError);
            }
        }
    } while (bKeepParsing);

cleanup:

    if (pToken) {
        SMBCfgFreeToken(pToken);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
SMBCfgParseNameValuePair(
    PSMB_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    BOOLEAN bContinue = TRUE;
    BOOLEAN bKeepParsing = TRUE;
    PSMB_CFG_TOKEN pToken = NULL;
    PSMB_STACK pTokenStack = NULL;

    //format is <str><equals><token1><token2>...<newline>

    //get initial <str>
    dwError = SMBCfgGetNextToken(
                    pParseState,
                    &pToken);
    BAIL_ON_SMB_ERROR(dwError);

    if(pToken->tokenType == SMBCfgString)
    {
        dwError = SMBStackPush(pToken, &pTokenStack);
        BAIL_ON_SMB_ERROR(dwError);
        pToken = NULL;
    }
    else
    {
        dwError = SMB_ERROR_INVALID_CONFIG;
        BAIL_ON_SMB_ERROR(dwError);
    }

    //get <equals>
    dwError = SMBCfgGetNextToken(
                    pParseState,
                    &pToken);
    BAIL_ON_SMB_ERROR(dwError);

    if(pToken->tokenType == SMBCfgEquals)
    {
        dwError = SMBStackPush(pToken, &pTokenStack);
        BAIL_ON_SMB_ERROR(dwError);
        pToken = NULL;
    }
    else
    {
        dwError = SMB_ERROR_INVALID_CONFIG;
        BAIL_ON_SMB_ERROR(dwError);
    }


    do
    {
        dwError = SMBCfgGetNextToken(
                        pParseState,
                        &pToken);
        BAIL_ON_SMB_ERROR(dwError);

        switch(pToken->tokenType)
        {
            case SMBCfgString:
            case SMBCfgEquals:
            case SMBCfgOther:
            {

                dwError = SMBStackPush(pToken, &pTokenStack);
                BAIL_ON_SMB_ERROR(dwError);
                pToken = NULL;

                break;

            }
            case SMBCfgNewline:
            {
                dwError = SMBCfgProcessNameValuePair(
                    pParseState,
                    &pTokenStack,
                    &bContinue);
                BAIL_ON_SMB_ERROR(dwError);
                bKeepParsing = FALSE;
                break;
            }
            case SMBCfgEOF:
            {
                dwError = SMBCfgProcessNameValuePair(
                    pParseState,
                    &pTokenStack,
                    &bContinue);
                BAIL_ON_SMB_ERROR(dwError);
                bContinue = FALSE;
                break;
            }
            default:
            {
                dwError = SMB_ERROR_INVALID_CONFIG;
                BAIL_ON_SMB_ERROR(dwError);
            }
        }
    } while(bContinue && bKeepParsing);


    *pbContinue = bContinue;

cleanup:

    if (pToken) {
        SMBCfgFreeToken(pToken);
    }

    return dwError;

error:

    *pbContinue = FALSE;

    goto cleanup;
}

DWORD
SMBCfgProcessComment(
    PSMB_CONFIG_PARSE_STATE pParseState,
    PSMB_STACK* ppTokenStack,
    PBOOLEAN    pbContinue
    )
{
    DWORD   dwError = 0;
    BOOLEAN bContinue = TRUE;
    PSTR    pszComment = NULL;

    dwError = SMBCfgProcessTokenStackIntoString(
        ppTokenStack,
        &pszComment);
    BAIL_ON_SMB_ERROR(dwError);

    if (pParseState->pfnCommentHandler &&
        !pParseState->bSkipSection) {

        dwError = pParseState->pfnCommentHandler(
                        pszComment,
                        pParseState->pData,
                        &bContinue);
        BAIL_ON_SMB_ERROR(dwError);
    }

    *pbContinue = bContinue;

cleanup:

    SMB_SAFE_FREE_STRING(pszComment);

    return dwError;

error:

    *pbContinue = TRUE;

    goto cleanup;
}

DWORD
SMBCfgProcessBeginSection(
    PSMB_CONFIG_PARSE_STATE pParseState,
    PSMB_STACK* ppTokenStack,
    PBOOLEAN    pbContinue)
{
    DWORD   dwError = 0;
    PSTR    pszSectionName = NULL;
    BOOLEAN bSkipSection = FALSE;
    BOOLEAN bContinue = TRUE;

    dwError = SMBCfgProcessTokenStackIntoString(
        ppTokenStack,
        &pszSectionName);
    BAIL_ON_SMB_ERROR(dwError);

    if (IsNullOrEmptyString(pszSectionName)) {
        dwError = SMB_ERROR_INVALID_CONFIG;
        BAIL_ON_SMB_ERROR(dwError);
    }

    if (pParseState->pfnStartSectionHandler) {

        if(pParseState->dwOptions & SMB_CFG_OPTION_STRIP_SECTION)
        {
            SMBStripWhitespace(pszSectionName, TRUE, TRUE);
        }

        dwError = pParseState->pfnStartSectionHandler(
                        pszSectionName,
                        pParseState->pData,
                        &bSkipSection,
                        &bContinue);
        BAIL_ON_SMB_ERROR(dwError);
    }

    pParseState->pszSectionName = pszSectionName;
    pParseState->bSkipSection = bSkipSection;
    *pbContinue = bContinue;

cleanup:

    return dwError;

error:

    SMB_SAFE_FREE_STRING(pszSectionName);
    pParseState->pszSectionName = NULL;
    pParseState->bSkipSection = FALSE;
    *pbContinue = FALSE;

    goto cleanup;
}

DWORD
SMBCfgProcessNameValuePair(
    PSMB_CONFIG_PARSE_STATE pParseState,
    PSMB_STACK*             ppTokenStack,
    PBOOLEAN                pbContinue
    )
{
    DWORD      dwError = 0;
    BOOLEAN    bContinue = TRUE;
    PSTR       pszName = NULL;
    PSTR       pszValue = NULL;
    PSMB_CFG_TOKEN pToken = NULL;

    *ppTokenStack = SMBStackReverse(*ppTokenStack);
    pToken = (PSMB_CFG_TOKEN)SMBStackPop(ppTokenStack);
    if (pToken && pToken->dwLen) {
        dwError = SMBStrndup(
                    pToken->pszToken,
                    pToken->dwLen,
                    &pszName);
        BAIL_ON_SMB_ERROR(dwError);
    }

    if (IsNullOrEmptyString(pszName)) {
        dwError = SMB_ERROR_INVALID_CONFIG;
        BAIL_ON_SMB_ERROR(dwError);
    }

    SMBCfgFreeToken(pToken);
    pToken = NULL;

    pToken = (PSMB_CFG_TOKEN)SMBStackPop(ppTokenStack);
    if (!pToken || pToken->tokenType != SMBCfgEquals)
    {
        dwError = SMB_ERROR_INVALID_CONFIG;
        BAIL_ON_SMB_ERROR(dwError);
    }

    SMBCfgFreeToken(pToken);
    pToken = NULL;

    //this will consume the token stack
    dwError = SMBCfgProcessTokenStackIntoString(
        ppTokenStack,
        &pszValue);
    BAIL_ON_SMB_ERROR(dwError);

    if (pParseState->pfnNameValuePairHandler  &&
        !pParseState->bSkipSection) {

        if(pParseState->dwOptions & SMB_CFG_OPTION_STRIP_NAME_VALUE_PAIR)
        {
            SMBStripWhitespace(pszName, TRUE, TRUE);
            SMBStripWhitespace(pszValue, TRUE, TRUE);
        }

        dwError = pParseState->pfnNameValuePairHandler(
                        pszName,
                        pszValue,
                        pParseState->pData,
                        &bContinue);
        BAIL_ON_SMB_ERROR(dwError);

    }

    *pbContinue = bContinue;

cleanup:

    if (pToken) {
        SMBCfgFreeToken(pToken);
        pToken = NULL;
    }

    if(ppTokenStack && *ppTokenStack)
    {
        dwError = SMBCfgFreeTokenStack(ppTokenStack);
    }

    SMB_SAFE_FREE_STRING(pszName);
    SMB_SAFE_FREE_STRING(pszValue);

    return dwError;

error:

    goto cleanup;
}

DWORD
SMBCfgProcessEndSection(
    PSMB_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN                pbContinue
    )
{
    DWORD dwError = 0;
    BOOLEAN bContinue = TRUE;

    if (pParseState->pfnEndSectionHandler &&
        !pParseState->bSkipSection) {

        if(pParseState->dwOptions & SMB_CFG_OPTION_STRIP_SECTION)
        {
            SMBStripWhitespace(pParseState->pszSectionName, TRUE, TRUE);
        }


        dwError = pParseState->pfnEndSectionHandler(
                        pParseState->pszSectionName,
                        pParseState->pData,
                        &bContinue);
        BAIL_ON_SMB_ERROR(dwError);
    }

    *pbContinue = bContinue;

cleanup:

    SMB_SAFE_FREE_STRING(pParseState->pszSectionName);

    return dwError;

error:

    *pbContinue = TRUE;

    goto cleanup;
}

DWORD
SMBCfgDetermineTokenLength(
    PSMB_STACK pStack
    )
{
    DWORD dwLen = 0;
    PSMB_STACK pIter = pStack;

    for (; pIter; pIter = pIter->pNext)
    {
        PSMB_CFG_TOKEN pToken = (PSMB_CFG_TOKEN)pIter->pItem;
        if (pToken) {
            dwLen += pToken->dwLen;
        }
    }

    return dwLen;
}

//this will consume the token stack
DWORD
SMBCfgProcessTokenStackIntoString(
    PSMB_STACK* ppTokenStack,
    PSTR* ppszConcatenated
    )
{
    DWORD   dwError = 0;
    DWORD   dwRequiredTokenLen = 0;
    PSTR    pszConcatenated = NULL;

    dwRequiredTokenLen = SMBCfgDetermineTokenLength(*ppTokenStack);

    if (dwRequiredTokenLen) {

        PSTR pszPos = NULL;
        PSMB_CFG_TOKEN pToken = NULL;

        *ppTokenStack = SMBStackReverse(*ppTokenStack);


        dwError = SMBAllocateMemory(
                        (dwRequiredTokenLen + 1) * sizeof(CHAR),
                        (PVOID*)&pszConcatenated);
        BAIL_ON_SMB_ERROR(dwError);


        pszPos = pszConcatenated;
        while (*ppTokenStack)
        {
            pToken = SMBStackPop(ppTokenStack);
            if (pToken && pToken->dwLen) {

                strncpy(pszPos, pToken->pszToken, pToken->dwLen);
                pszPos += pToken->dwLen;

                SMBCfgFreeToken(pToken);
                pToken = NULL;
            }
        }
    }

    *ppszConcatenated = pszConcatenated;

cleanup:

    return dwError;

error:

    SMB_SAFE_FREE_STRING(pszConcatenated);

    *ppszConcatenated = NULL;

    goto cleanup;
}


DWORD
SMBCfgAllocateToken(
    DWORD           dwSize,
    PSMB_CFG_TOKEN* ppToken
    )
{
    DWORD dwError = 0;
    PSMB_CFG_TOKEN pToken = NULL;
    DWORD dwMaxLen = (dwSize > 0 ? dwSize : SMB_CFG_TOKEN_DEFAULT_LENGTH);


    dwError = SMBAllocateMemory(
                    sizeof(SMB_CFG_TOKEN),
                    (PVOID*)&pToken);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBAllocateMemory(
                    dwMaxLen * sizeof(CHAR),
                    (PVOID*)&pToken->pszToken);
    BAIL_ON_SMB_ERROR(dwError);


    pToken->tokenType = SMBCfgNone;
    pToken->dwMaxLen = dwMaxLen;

    *ppToken = pToken;

cleanup:

    return dwError;

error:

    *ppToken = NULL;

    if (pToken) {
        SMBCfgFreeToken(pToken);
    }

    goto cleanup;
}

DWORD
SMBCfgReallocToken(
    PSMB_CFG_TOKEN pToken,
    DWORD          dwNewSize
    )
{
    DWORD dwError = 0;

    dwError = SMBReallocMemory(
                    pToken->pszToken,
                    (PVOID*)&pToken->pszToken,
                    dwNewSize);
    BAIL_ON_SMB_ERROR(dwError);

    pToken->dwMaxLen = dwNewSize;

cleanup:

    return dwError;

error:

    goto cleanup;
}

VOID
SMBCfgResetToken(
    PSMB_CFG_TOKEN pToken
    )
{
    if (pToken && pToken->pszToken && pToken->dwMaxLen)
    {
        memset(pToken->pszToken, 0, pToken->dwMaxLen);
        pToken->dwLen = 0;
    }
}

DWORD
SMBCfgCopyToken(
    PSMB_CFG_TOKEN pTokenSrc,
    PSMB_CFG_TOKEN pTokenDst
    )
{
    DWORD dwError = 0;

    pTokenDst->tokenType = pTokenSrc->tokenType;

    if (pTokenSrc->dwLen > pTokenDst->dwLen) {
        dwError = SMBReallocMemory(
                        (PVOID)  pTokenDst->pszToken,
                        (PVOID*) &pTokenDst->pszToken,
                        (DWORD)  pTokenSrc->dwLen);
        BAIL_ON_SMB_ERROR(dwError);

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
SMBCfgFreeTokenStack(
    PSMB_STACK* ppTokenStack
    )
{

    DWORD dwError = 0;

    PSMB_STACK pTokenStack = *ppTokenStack;

    dwError = SMBStackForeach(
            pTokenStack,
            &SMBCfgFreeTokenInStack,
            NULL);
    BAIL_ON_SMB_ERROR(dwError);

    SMBStackFree(pTokenStack);

    *ppTokenStack = NULL;

error:

    return dwError;
}

DWORD
SMBCfgFreeTokenInStack(
    PVOID pToken,
    PVOID pUserData
    )
{
    if (pToken)
    {
        SMBCfgFreeToken((PSMB_CFG_TOKEN)pToken);
    }

    return 0;
}

VOID
SMBCfgFreeToken(
    PSMB_CFG_TOKEN pToken
    )
{
    SMB_SAFE_FREE_MEMORY(pToken->pszToken);
    SMBFreeMemory(pToken);
}

DWORD
SMBCfgGetNextToken(
    PSMB_CONFIG_PARSE_STATE pParseState,
    PSMB_CFG_TOKEN*         ppToken
    )
{
    DWORD dwError = 0;
    SMBCfgTokenType tokenType = SMBCfgNone;
    SMBCfgLexState  curLexState = SMBLexBegin;
    PSMB_CFG_TOKEN  pToken = NULL;
    BOOLEAN         bOwnToken = FALSE;

    if (SMBStackPeek(pParseState->pLexerTokenStack) != NULL)
    {
        PSMB_CFG_TOKEN pToken_input = *ppToken;

        pToken = (PSMB_CFG_TOKEN)SMBStackPop(&pParseState->pLexerTokenStack);
        bOwnToken = TRUE;

        if (pToken_input) {

            dwError = SMBCfgCopyToken(pToken, pToken_input);
            BAIL_ON_SMB_ERROR(dwError);

            SMBCfgFreeToken(pToken);
            pToken = NULL;
            bOwnToken = FALSE;

        }

        goto done;
    }

    pToken = *ppToken;

    if (!pToken) {
        dwError = SMBCfgAllocateToken(
                        0,
                        &pToken);
        BAIL_ON_SMB_ERROR(dwError);

        bOwnToken = TRUE;
    }
    else
    {
        SMBCfgResetToken(pToken);
    }

    while (curLexState != SMBLexEnd)
    {
        DWORD ch = SMBCfgGetCharacter(pParseState);
        SMBCfgLexState lexClass = SMBCfgGetLexClass(ch);

        if (lexClass != SMBLexEOF) {
            pParseState->dwCol++;
        }

        if (lexClass == SMBLexNewline) {
            pParseState->dwLine++;
            pParseState->dwCol = 0;
        }

        tokenType = SMBCfgGetTokenType(curLexState, lexClass);

        switch(SMBCfgGetLexAction(curLexState, lexClass))
        {
            case Skip:

                break;

            case Consume:

                if (pToken->dwLen >= pToken->dwMaxLen) {
                    dwError = SMBCfgReallocToken(
                                    pToken,
                                    pToken->dwMaxLen + SMB_CFG_TOKEN_DEFAULT_LENGTH);
                    BAIL_ON_SMB_ERROR(dwError);
                }

                pToken->pszToken[pToken->dwLen++] = (BYTE)ch;

                break;

            case Pushback:

	        if (lexClass == SMBLexNewline)
	        {
                    pParseState->dwLine--;
	        }
                pParseState->dwCol--;
                dwError = SMBCfgPushBackCharacter(
                                pParseState,
                                (BYTE)ch);
                BAIL_ON_SMB_ERROR(dwError);

                break;
        }

        curLexState = SMBCfgGetNextLexState(curLexState, lexClass);
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
        SMBCfgFreeToken(pToken);
        *ppToken = NULL;
    }

    goto cleanup;
}

DWORD
SMBCfgGetCharacter(
    PSMB_CONFIG_PARSE_STATE pParseState
    )
{
    return getc(pParseState->fp);
}

SMBCfgLexState
SMBCfgGetLexClass(
    DWORD ch
    )
{

    if (ch == EOF) {
        return SMBLexEOF;
    }

    if (ch == '\n') {
        return SMBLexNewline;
    }

    if (ch == '[') {
        return SMBLexLSqBrace;
    }

    if (ch == ']') {
        return SMBLexRSqBrace;
    }

    if (ch == '=') {
        return SMBLexEquals;
    }

    if (ch == '#') {
        return SMBLexHash;
    }

    if (isalnum(ch) ||
        ispunct(ch) ||
        isblank(ch)) {
        return SMBLexChar;
    }

    return SMBLexOther;
}

DWORD
SMBCfgPushBackCharacter(
    PSMB_CONFIG_PARSE_STATE pParseState,
    BYTE ch
    )
{
    return ((EOF == ungetc(ch, pParseState->fp)) ? errno : 0);
}

SMBCfgLexState
SMBCfgGetNextLexState(
    SMBCfgLexState currentState,
    DWORD chId
    )
{
    return (gSMBLexStateTable[currentState][chId].nextState);
}

SMBCfgLexAction
SMBCfgGetLexAction(
    SMBCfgLexState currentState,
    DWORD chId
    )
{
    return (gSMBLexStateTable[currentState][chId].action);
}

SMBCfgTokenType
SMBCfgGetTokenType(
    SMBCfgLexState currentState,
    DWORD chId
    )
{
    return (gSMBLexStateTable[currentState][chId].tokenId);
}
