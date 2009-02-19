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
 *        lwps-cfg.c
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
#include "lwps-utils.h"
#include "lwps-cfg_p.h"

#if !HAVE_DECL_ISBLANK && !defined(isblank)
static
int
isblank(int c);
#endif

static LWPS_CFG_LEXER_STATE gLwpsLexStateTable[][9] =
{
  /* LwpsLexBegin    := 0 */
  {
    {   LwpsLexBegin,  Consume,             LwpsCfgNone }, /* LwpsLexBegin    */
    {    LwpsLexChar,  Consume,             LwpsCfgNone }, /* LwpsLexChar     */
    {     LwpsLexEnd,  Consume,  LwpsCfgLeftSquareBrace }, /* LwpsLexLSqBrace */
    {     LwpsLexEnd,  Consume, LwpsCfgRightSquareBrace }, /* LwpsLexRSqBrace */
    {     LwpsLexEnd,  Consume,           LwpsCfgEquals }, /* LwpsLexEquals   */
    {     LwpsLexEnd,  Consume,             LwpsCfgHash }, /* LwpsLexHash     */
    {     LwpsLexEnd,  Consume,          LwpsCfgNewline }, /* LwpsLexNewline  */
    {   LwpsLexBegin,     Skip,             LwpsCfgNone }, /* LwpsLexOther    */
    {     LwpsLexEnd,  Consume,              LwpsCfgEOF }  /* LwpsLexEOF      */
  },
  /* LwpsLexChar     := 1 */
  {
    {   LwpsLexBegin,  Consume,             LwpsCfgNone }, /* LwpsLexBegin    */ 
    {    LwpsLexChar,  Consume,             LwpsCfgNone }, /* LwpsLexChar     */
    {     LwpsLexEnd, Pushback,           LwpsCfgString }, /* LwpsLexLSqBrace */
    {     LwpsLexEnd, Pushback,           LwpsCfgString }, /* LwpsLexRSqBrace */
    {     LwpsLexEnd, Pushback,           LwpsCfgString }, /* LwpsLexEquals   */
    {     LwpsLexEnd, Pushback,           LwpsCfgString }, /* LwpsLexHash     */
    {     LwpsLexEnd, Pushback,           LwpsCfgString }, /* LwpsLexNewline  */
    {     LwpsLexEnd,     Skip,           LwpsCfgString }, /* LwpsLexOther    */
    {     LwpsLexEnd,     Skip,           LwpsCfgString }  /* LwpsLexEOF      */
  },
  /* LwpsLexLSqBrace := 2 */
  {
    {   LwpsLexBegin,  Consume,             LwpsCfgNone }, /* LwpsLexBegin    */ 
    {     LwpsLexEnd, Pushback,  LwpsCfgLeftSquareBrace }, /* LwpsLexChar     */ 
    {     LwpsLexEnd, Pushback,  LwpsCfgLeftSquareBrace }, /* LwpsLexLSqBrace */
    {     LwpsLexEnd, Pushback,  LwpsCfgLeftSquareBrace }, /* LwpsLexRSqBrace */
    {     LwpsLexEnd, Pushback,  LwpsCfgLeftSquareBrace }, /* LwpsLexEquals   */
    {     LwpsLexEnd, Pushback,  LwpsCfgLeftSquareBrace }, /* LwpsLexHash     */
    {     LwpsLexEnd, Pushback,  LwpsCfgLeftSquareBrace }, /* LwpsLexNewline  */
    {     LwpsLexEnd,     Skip,  LwpsCfgLeftSquareBrace }, /* LwpsLexOther    */
    {     LwpsLexEnd,     Skip,  LwpsCfgLeftSquareBrace }  /* LwpsLexEOF      */
  },
  /* LwpsLexRSqBrace := 3 */
  {
    {   LwpsLexBegin,  Consume,             LwpsCfgNone }, /* LwpsLexBegin    */
    {     LwpsLexEnd, Pushback, LwpsCfgRightSquareBrace }, /* LwpsLexChar     */
    {     LwpsLexEnd, Pushback, LwpsCfgRightSquareBrace }, /* LwpsLexLSqBrace */
    {     LwpsLexEnd, Pushback, LwpsCfgRightSquareBrace }, /* LwpsLexRSqBrace */
    {     LwpsLexEnd, Pushback, LwpsCfgRightSquareBrace }, /* LwpsLexEquals   */
    {     LwpsLexEnd, Pushback, LwpsCfgRightSquareBrace }, /* LwpsLexHash     */
    {     LwpsLexEnd, Pushback, LwpsCfgRightSquareBrace }, /* LwpsLexNewline  */
    {     LwpsLexEnd,     Skip, LwpsCfgRightSquareBrace }, /* LwpsLexOther    */
    {     LwpsLexEnd,     Skip, LwpsCfgRightSquareBrace }  /* LwpsLexEOF      */
  },
  /* LwpsLexEquals   := 4 */
  {
    {   LwpsLexBegin,  Consume,             LwpsCfgNone }, /* LwpsLexBegin    */
    {     LwpsLexEnd, Pushback,           LwpsCfgEquals }, /* LwpsLexChar     */
    {     LwpsLexEnd, Pushback,           LwpsCfgEquals }, /* LwpsLexLSqBrace */
    {     LwpsLexEnd, Pushback,           LwpsCfgEquals }, /* LwpsLexRSqBrace */
    {     LwpsLexEnd, Pushback,           LwpsCfgEquals }, /* LwpsLexEquals   */
    {     LwpsLexEnd, Pushback,           LwpsCfgEquals }, /* LwpsLexHash     */
    {     LwpsLexEnd, Pushback,           LwpsCfgEquals }, /* LwpsLexNewline  */
    {     LwpsLexEnd,     Skip,           LwpsCfgEquals }, /* LwpsLexOther    */
    {     LwpsLexEnd,     Skip,           LwpsCfgEquals }  /* LwpsLexEOF      */
  },
  /* LwpsLexHash     := 5 */
  {
    {   LwpsLexBegin,  Consume,             LwpsCfgNone }, /* LwpsLexBegin    */
    {     LwpsLexEnd, Pushback,             LwpsCfgHash }, /* LwpsLexChar     */
    {     LwpsLexEnd, Pushback,             LwpsCfgHash }, /* LwpsLexLSqBrace */
    {     LwpsLexEnd, Pushback,             LwpsCfgHash }, /* LwpsLexRSqBrace */
    {     LwpsLexEnd, Pushback,             LwpsCfgHash }, /* LwpsLexEquals   */
    {     LwpsLexEnd, Pushback,             LwpsCfgHash }, /* LwpsLexHash     */
    {     LwpsLexEnd, Pushback,             LwpsCfgHash }, /* LwpsLexNewline  */
    {     LwpsLexEnd,     Skip,             LwpsCfgHash }, /* LwpsLexOther    */
    {     LwpsLexEnd,     Skip,             LwpsCfgHash }  /* LwpsLexEOF      */
  },
  /* LwpsLexNewline  := 6 */
  {
    {   LwpsLexBegin,  Consume,             LwpsCfgNone }, /* LwpsLexBegin    */
    {     LwpsLexEnd, Pushback,          LwpsCfgNewline }, /* LwpsLexChar     */
    {     LwpsLexEnd, Pushback,          LwpsCfgNewline }, /* LwpsLexLSqBrace */
    {     LwpsLexEnd, Pushback,          LwpsCfgNewline }, /* LwpsLexRSqBrace */
    {     LwpsLexEnd, Pushback,          LwpsCfgNewline }, /* LwpsLexEquals   */
    {     LwpsLexEnd, Pushback,          LwpsCfgNewline }, /* LwpsLexHash     */
    {     LwpsLexEnd, Pushback,          LwpsCfgNewline }, /* LwpsLexNewline  */
    {     LwpsLexEnd,     Skip,          LwpsCfgNewline }, /* LwpsLexOther    */
    {     LwpsLexEnd,     Skip,          LwpsCfgNewline }  /* LwpsLexEOF      */
  },
  /* LwpsLexOther    := 7 */
  {
    {   LwpsLexBegin,  Consume,             LwpsCfgNone }, /* LwpsLexBegin    */
    {    LwpsLexChar,  Consume,             LwpsCfgNone }, /* LwpsLexChar     */
    {LwpsLexLSqBrace,  Consume,             LwpsCfgNone }, /* LwpsLexLSqBrace */
    {LwpsLexRSqBrace,  Consume,             LwpsCfgNone }, /* LwpsLexRSqBrace */
    {  LwpsLexEquals,  Consume,             LwpsCfgNone }, /* LwpsLexEquals   */
    {    LwpsLexHash,  Consume,             LwpsCfgNone }, /* LwpsLexHash     */
    { LwpsLexNewline,  Consume,             LwpsCfgNone }, /* LwpsLexNewline  */
    {   LwpsLexOther,  Consume,             LwpsCfgNone }, /* LwpsLexOther    */
    {     LwpsLexEOF,  Consume,             LwpsCfgNone }  /* LwpsLexEOF      */
  },
  /* LwpsLexEOF      := 8 */
  {
    {   LwpsLexBegin,  Consume,             LwpsCfgNone }, /* LwpsLexBegin    */
    {     LwpsLexEnd,  Consume,             LwpsCfgNone }, /* LwpsLexChar     */
    {     LwpsLexEnd,  Consume,             LwpsCfgNone }, /* LwpsLexLSqBrace */
    {     LwpsLexEnd,  Consume,             LwpsCfgNone }, /* LwpsLexRSqBrace */
    {     LwpsLexEnd,  Consume,             LwpsCfgNone }, /* LwpsLexEquals   */
    {     LwpsLexEnd,  Consume,             LwpsCfgNone }, /* LwpsLexHash     */
    { LwpsLexNewline,  Consume,             LwpsCfgNone }, /* LwpsLexNewline  */
    {     LwpsLexEnd,  Consume,             LwpsCfgNone }, /* LwpsLexOther    */
    {     LwpsLexEnd,  Consume,             LwpsCfgNone }  /* LwpsLexEOF      */
  }
};

DWORD
LwpsParseConfigFile(
    PCSTR                     pszFilePath,
    DWORD                     dwOptions,
    PFNLWPS_CONFIG_START_SECTION   pfnStartSectionHandler,
    PFNLWPS_CONFIG_COMMENT         pfnCommentHandler,
    PFNLWPS_CONFIG_NAME_VALUE_PAIR pfnNameValuePairHandler,
    PFNLWPS_CONFIG_END_SECTION     pfnEndSectionHandler,
    PVOID                     pData
    )
{
    DWORD dwError = 0;
    PLWPS_CONFIG_PARSE_STATE pParseState = NULL;
    
    dwError = LwpsCfgInitParseState(
                    pszFilePath,
                    dwOptions, 
                    pfnStartSectionHandler,
                    pfnCommentHandler,
                    pfnNameValuePairHandler,
                    pfnEndSectionHandler,
                    pData,
                    &pParseState);
    BAIL_ON_LWPS_ERROR(dwError);
    
    dwError = LwpsCfgParse(pParseState);
    BAIL_ON_LWPS_ERROR(dwError);
    
cleanup:
    
    if (pParseState) {
        LwpsCfgFreeParseState(pParseState);
    }

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LwpsCfgInitParseState(
    PCSTR                     pszFilePath,
    DWORD                     dwOptions,
    PFNLWPS_CONFIG_START_SECTION   pfnStartSectionHandler,
    PFNLWPS_CONFIG_COMMENT         pfnCommentHandler,
    PFNLWPS_CONFIG_NAME_VALUE_PAIR pfnNameValuePairHandler,
    PFNLWPS_CONFIG_END_SECTION     pfnEndSectionHandler,
    PVOID                     pData,
    PLWPS_CONFIG_PARSE_STATE*  ppParseState
    )
{
    DWORD dwError = 0;
    PLWPS_CONFIG_PARSE_STATE pParseState = NULL;
    PLWPS_STACK pTokenStack = NULL;
    FILE* fp = NULL;

    if ((fp = fopen(pszFilePath, "r")) == NULL) {
        dwError = errno;
        BAIL_ON_LWPS_ERROR(dwError);
    }
    
    
    dwError = LwpsAllocateMemory(
                    sizeof(LWPS_CONFIG_PARSE_STATE),
                    (PVOID*)&pParseState);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = LwpsAllocateMemory(
                    sizeof(LWPS_STACK), 
                    (PVOID*)&pTokenStack);
    BAIL_ON_LWPS_ERROR(dwError);

    pParseState->pLexerTokenStack = pTokenStack;

    dwError = LwpsAllocateString(
                    pszFilePath,
                    &(pParseState->pszFilePath));
    BAIL_ON_LWPS_ERROR(dwError);
    
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
        LwpsCfgFreeParseState(pParseState);
    }
    
    if (fp) {
        fclose(fp);
    }

    goto cleanup;
}

VOID
LwpsCfgFreeParseState(
    PLWPS_CONFIG_PARSE_STATE pParseState
    )
{
    LWPS_SAFE_FREE_STRING(pParseState->pszFilePath);
    LWPS_SAFE_FREE_STRING(pParseState->pszSectionName);
    if (pParseState->pLexerTokenStack) {
        LwpsCfgFreeTokenStack(&pParseState->pLexerTokenStack);
    }
    
    if (pParseState->fp) {
        fclose(pParseState->fp);
    }
    LwpsFreeMemory(pParseState);
}
  

DWORD
LwpsCfgParse(
    PLWPS_CONFIG_PARSE_STATE pParseState
    )
{
    
    DWORD dwError = 0;
    PLWPS_CFG_TOKEN pToken = NULL;
    BOOLEAN bContinue = TRUE;
    PLWPS_STACK pTokenStack = NULL;
    
    do 
    {
    
        dwError = LwpsCfgGetNextToken(
                        pParseState,
                        &pToken);
        BAIL_ON_LWPS_ERROR(dwError);
        
        switch (pToken->tokenType)
        {
            case LwpsCfgHash:
            {
                dwError = LwpsCfgParseComment(
                                pParseState,
                                &bContinue);
                BAIL_ON_LWPS_ERROR(dwError);
                
                break;
            }
            case LwpsCfgNewline:
            {
                //in case of a blank line (newline),
                //interpret this as an empty comment.
                dwError = LwpsCfgProcessComment(
                                pParseState, 
                                &pTokenStack,
                                &bContinue);
                BAIL_ON_LWPS_ERROR(dwError);
                
                break;
            }
            case LwpsCfgLeftSquareBrace:
            {
                
                dwError = LwpsCfgParseSections(
                                pParseState);
                BAIL_ON_LWPS_ERROR(dwError);
                
                break;
            }
            case LwpsCfgEOF:
            {
                bContinue = FALSE;
                break;
            }
            default:
            {
                dwError = LWPS_ERROR_INVALID_CONFIG;
                BAIL_ON_LWPS_ERROR(dwError);
            }
        }
        
    } while (bContinue);
    
cleanup:
    
    if (pToken) {
        LwpsCfgFreeToken(pToken);
    }
    
    return dwError;
    
error: 

    if (dwError == LWPS_ERROR_INVALID_CONFIG)
    {
        if (pParseState) {                                                             
            LWPS_LOG_ERROR ("Parse error at line=%d, column=%d of file [%s]",  
                          pParseState->dwLine, 
                          pParseState->dwCol, 
                          IsNullOrEmptyString(pParseState->pszFilePath) ? 
                              "" : pParseState->pszFilePath);                  
        }
    }

    goto cleanup;
}

DWORD
LwpsCfgParseSections(
    PLWPS_CONFIG_PARSE_STATE pParseState
    )
{
    DWORD dwError = 0;
    PLWPS_CFG_TOKEN pToken = NULL;
    BOOLEAN bContinue = TRUE;
    PLWPS_STACK pTokenStack = NULL;
    
    dwError = LwpsCfgParseSectionHeader(
                    pParseState, 
                    &bContinue);
    BAIL_ON_LWPS_ERROR(dwError);
    
    while (bContinue)
    {
        dwError = LwpsCfgGetNextToken(
                        pParseState,
                        &pToken
                        );
        BAIL_ON_LWPS_ERROR(dwError);
        
        switch (pToken->tokenType)
        {
            case LwpsCfgString:
            {
                BOOLEAN bIsAllSpace = FALSE;
                
                dwError = LwpsStrIsAllSpace(
                                pToken->pszToken,
                                &bIsAllSpace
                                );
                BAIL_ON_LWPS_ERROR(dwError);
                                
                if (bIsAllSpace)
                {
                    continue;
                }
                            
                dwError = LwpsStackPush(pToken, &(pParseState->pLexerTokenStack));
                BAIL_ON_LWPS_ERROR(dwError);
                
                pToken = NULL;
                
                dwError = LwpsCfgParseNameValuePair(
                                pParseState,
                                &bContinue
                                );
                BAIL_ON_LWPS_ERROR(dwError);
                break;                   
            }
        
            case LwpsCfgHash:
            {
                dwError = LwpsCfgParseComment(
                                pParseState,
                                &bContinue
                                );
                BAIL_ON_LWPS_ERROR(dwError);
                break;
            }
            case LwpsCfgNewline:
            {
                //in case of a blank line (newline),
                //interpret this as an empty comment.
                dwError = LwpsCfgProcessComment(
                                pParseState,
                                &pTokenStack,
                                &bContinue);
                BAIL_ON_LWPS_ERROR(dwError);
                break;
            }
            case LwpsCfgLeftSquareBrace:
            {
                dwError = LwpsCfgParseSectionHeader(
                                pParseState, 
                                &bContinue);
                BAIL_ON_LWPS_ERROR(dwError);

                break;
            }
            case LwpsCfgEOF:
            {
                bContinue = FALSE;
                break;
            }
            default:
            {
                dwError = LWPS_ERROR_INVALID_CONFIG;
                BAIL_ON_LWPS_ERROR(dwError);
            }
        }
    } 
    
    if (bContinue && !IsNullOrEmptyString(pParseState->pszSectionName))
    {
        dwError = LwpsCfgProcessEndSection(
                        pParseState,
                        &bContinue);
        BAIL_ON_LWPS_ERROR(dwError);
    }
    
cleanup:
    
    if (pToken) {
        LwpsCfgFreeToken(pToken);
    }
    
    return dwError;
    
error:

    goto cleanup;
}



DWORD
LwpsCfgParseComment(
    PLWPS_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN    pbContinue
    )
{
    DWORD dwError = 0;
    BOOLEAN bContinue = TRUE;
    BOOLEAN bKeepParsing = TRUE;
    PLWPS_CFG_TOKEN pToken = NULL;
    PLWPS_STACK pTokenStack = NULL;
    
    do
    {
        dwError = LwpsCfgGetNextToken(
                    pParseState,
                    &pToken);
        BAIL_ON_LWPS_ERROR(dwError);
        
        switch (pToken->tokenType)
        {
            case LwpsCfgEOF:
            {
                dwError = LwpsCfgProcessComment(
                                pParseState,
                                &pTokenStack,
                                &bContinue);
                BAIL_ON_LWPS_ERROR(dwError);
                
                bContinue = FALSE;
                
                break;
            }
            case LwpsCfgNewline:
            {
                dwError = LwpsCfgProcessComment(
                                pParseState,
                                &pTokenStack,
                                &bContinue);
                BAIL_ON_LWPS_ERROR(dwError);
                
                bKeepParsing = FALSE;
                
                break;
            }
            default:
            {
                dwError = LwpsStackPush(pToken, &pTokenStack);
                BAIL_ON_LWPS_ERROR(dwError);
                
                pToken = NULL;
                
            }
        }
        
    } while (bContinue && bKeepParsing);
    
    *pbContinue = bContinue;
    
cleanup:

    if (pToken) {
        LwpsCfgFreeToken(pToken);
    }

    return dwError;
    
error:

    *pbContinue = FALSE;
    
    goto cleanup;
}

DWORD
LwpsCfgParseSectionHeader(
    PLWPS_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    BOOLEAN bKeepParsing = TRUE;
    BOOLEAN bContinue = TRUE;
    PLWPS_CFG_TOKEN pToken = NULL;
    PLWPS_STACK pTokenStack = NULL;
    
    if(!IsNullOrEmptyString(pParseState->pszSectionName))
    {
        dwError = LwpsCfgProcessEndSection(
                        pParseState,
                        &bContinue);
        BAIL_ON_LWPS_ERROR(dwError);
        
    }
    
    if (!bContinue)
    {
        goto done;
    }
    
    pParseState->bSkipSection = FALSE;
    
    do
    {
        dwError = LwpsCfgGetNextToken(
                        pParseState,
                        &pToken);
        BAIL_ON_LWPS_ERROR(dwError);
        
        switch(pToken->tokenType)
        {
            case LwpsCfgString:
            case LwpsCfgEquals:
            case LwpsCfgOther:
            { 
                dwError = LwpsStackPush(pToken, &pTokenStack);
                BAIL_ON_LWPS_ERROR(dwError);
                
                pToken = NULL;
                break;
            }
            case LwpsCfgRightSquareBrace:
            {
                dwError = LwpsAssertWhitespaceOnly(pParseState);
                BAIL_ON_LWPS_ERROR(dwError);
                
                bKeepParsing = FALSE;
                
                break;
            }
            default:
            {
                dwError = LWPS_ERROR_INVALID_CONFIG;
                BAIL_ON_LWPS_ERROR(dwError);
            }
        }
        
    } while (bKeepParsing);
    
    dwError = LwpsCfgGetNextToken(
                    pParseState,
                    &pToken);
    BAIL_ON_LWPS_ERROR(dwError);
    
    switch(pToken->tokenType)
    {
    
        case LwpsCfgNewline:
        {
            dwError = LwpsCfgProcessBeginSection(
                            pParseState,
                            &pTokenStack,
                            &bContinue);
            BAIL_ON_LWPS_ERROR(dwError);

            break;
        }
        case LwpsCfgEOF:
        {
            dwError = LwpsCfgProcessBeginSection(
                            pParseState,
                            &pTokenStack,
                            &bContinue);
            BAIL_ON_LWPS_ERROR(dwError);
            
            if (bContinue) {
                
                dwError = LwpsCfgProcessEndSection(
                                pParseState,
                                &bContinue);
                BAIL_ON_LWPS_ERROR(dwError);
            }
            
            bContinue = FALSE;
            
            break;     
        }
        default:
        {
            dwError = LWPS_ERROR_INVALID_CONFIG;
            BAIL_ON_LWPS_ERROR(dwError);
        }
    }
    
done:

    *pbContinue = bContinue;
    
cleanup:

    if (pToken) {
        LwpsCfgFreeToken(pToken);
    }

    return dwError;
    
error:

    *pbContinue = FALSE;

    goto cleanup;
}

DWORD
LwpsAssertWhitespaceOnly(
    PLWPS_CONFIG_PARSE_STATE pParseState
    )
{
    DWORD dwError = 0;
    PLWPS_CFG_TOKEN pToken = NULL;
    BOOLEAN bKeepParsing = TRUE;
    
    do
    {
        dwError = LwpsCfgGetNextToken(
                        pParseState,
                        &pToken);
        BAIL_ON_LWPS_ERROR(dwError);
        
        switch(pToken->tokenType)
        {
            case LwpsCfgString:
            case LwpsCfgOther:
            {
                DWORD i = 0;
                for (; i < pToken->dwLen; i++) {
                    if (!isspace((int)pToken->pszToken[i])) {
                        dwError = LWPS_ERROR_INVALID_CONFIG;
                        BAIL_ON_LWPS_ERROR(dwError);
                    }
                }
                break;
            }
            case LwpsCfgEOF:
            case LwpsCfgNewline:
            {
                dwError = LwpsStackPush(pToken, &pParseState->pLexerTokenStack);
                BAIL_ON_LWPS_ERROR(dwError);
                
                pToken = NULL;
                
                bKeepParsing = FALSE;
                
                break;
            }
            default:
            {
                dwError = LWPS_ERROR_INVALID_CONFIG;
                BAIL_ON_LWPS_ERROR(dwError);
            }
        }
    } while (bKeepParsing);
    
cleanup:

    if (pToken) {
        LwpsCfgFreeToken(pToken);
    }

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LwpsCfgParseNameValuePair(
    PLWPS_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    BOOLEAN bContinue = TRUE;
    BOOLEAN bKeepParsing = TRUE;
    PLWPS_CFG_TOKEN pToken = NULL;
    PLWPS_STACK pTokenStack = NULL;
    
    //format is <str><equals><token1><token2>...<newline>
    
    //get initial <str>
    dwError = LwpsCfgGetNextToken(
                    pParseState,
                    &pToken);
    BAIL_ON_LWPS_ERROR(dwError);
    
    if(pToken->tokenType == LwpsCfgString) 
    {
        dwError = LwpsStackPush(pToken, &pTokenStack);
        BAIL_ON_LWPS_ERROR(dwError);
        pToken = NULL;
    }
    else
    {
        dwError = LWPS_ERROR_INVALID_CONFIG;
        BAIL_ON_LWPS_ERROR(dwError);
    }

    //get <equals>
    dwError = LwpsCfgGetNextToken(
                    pParseState,
                    &pToken);
    BAIL_ON_LWPS_ERROR(dwError);
   
    if(pToken->tokenType == LwpsCfgEquals) 
    {
        dwError = LwpsStackPush(pToken, &pTokenStack);
        BAIL_ON_LWPS_ERROR(dwError);
        pToken = NULL;
    }
    else
    {
        dwError = LWPS_ERROR_INVALID_CONFIG;
        BAIL_ON_LWPS_ERROR(dwError);
    }


    do 
    {
        dwError = LwpsCfgGetNextToken(
                        pParseState,
                        &pToken);
        BAIL_ON_LWPS_ERROR(dwError);
        
        switch(pToken->tokenType)
        {
            case LwpsCfgString:
            case LwpsCfgEquals:
            case LwpsCfgOther:
            {
                
                dwError = LwpsStackPush(pToken, &pTokenStack);
                BAIL_ON_LWPS_ERROR(dwError);
                pToken = NULL;
                
                break;

            }
            case LwpsCfgNewline:
            {
                dwError = LwpsCfgProcessNameValuePair(
                    pParseState,
                    &pTokenStack,
                    &bContinue);
                BAIL_ON_LWPS_ERROR(dwError);
                bKeepParsing = FALSE; 
                break;
            }
            case LwpsCfgEOF: 
            {
                dwError = LwpsCfgProcessNameValuePair(
                    pParseState,
                    &pTokenStack,
                    &bContinue);
                BAIL_ON_LWPS_ERROR(dwError);
                bContinue = FALSE;
                break;
            }
            default:
            {
                dwError = LWPS_ERROR_INVALID_CONFIG;
                BAIL_ON_LWPS_ERROR(dwError);
            }
        }
    } while(bContinue && bKeepParsing);
    
    
    *pbContinue = bContinue;
    
cleanup:

    if (pToken) {
        LwpsCfgFreeToken(pToken);
    }

    return dwError;
    
error:

    *pbContinue = FALSE;

    goto cleanup;
}

DWORD
LwpsCfgProcessComment(
    PLWPS_CONFIG_PARSE_STATE pParseState,
    PLWPS_STACK* ppTokenStack,
    PBOOLEAN    pbContinue
    )
{
    DWORD   dwError = 0;
    BOOLEAN bContinue = TRUE;
    PSTR    pszComment = NULL;
    
    dwError = LwpsCfgProcessTokenStackIntoString(
        ppTokenStack,
        &pszComment);
    BAIL_ON_LWPS_ERROR(dwError);
    
    if (pParseState->pfnCommentHandler &&
        !pParseState->bSkipSection) {
            
        dwError = pParseState->pfnCommentHandler(
                        pszComment,
                        pParseState->pData,
                        &bContinue);
        BAIL_ON_LWPS_ERROR(dwError);
    }
    
    *pbContinue = bContinue;
    
cleanup:

    LWPS_SAFE_FREE_STRING(pszComment);

    return dwError;
    
error:

    *pbContinue = TRUE;

    goto cleanup;
}

DWORD
LwpsCfgProcessBeginSection(
    PLWPS_CONFIG_PARSE_STATE pParseState,
    PLWPS_STACK* ppTokenStack,
    PBOOLEAN    pbContinue)
{
    DWORD   dwError = 0;
    PSTR    pszSectionName = NULL;
    BOOLEAN bSkipSection = FALSE;
    BOOLEAN bContinue = TRUE;
    
    dwError = LwpsCfgProcessTokenStackIntoString(
        ppTokenStack,
        &pszSectionName);
    BAIL_ON_LWPS_ERROR(dwError);
    
    if (IsNullOrEmptyString(pszSectionName)) {
        dwError = LWPS_ERROR_INVALID_CONFIG;
        BAIL_ON_LWPS_ERROR(dwError);
    }
    
    if (pParseState->pfnStartSectionHandler) {
        
        if(pParseState->dwOptions & LWPS_CFG_OPTION_STRIP_SECTION) 
        {
            LwpsStripWhitespace(pszSectionName, TRUE, TRUE); 
        }
        
        dwError = pParseState->pfnStartSectionHandler(
                        pszSectionName,
                        pParseState->pData,
                        &bSkipSection,
                        &bContinue);
        BAIL_ON_LWPS_ERROR(dwError);
    }
    
    pParseState->pszSectionName = pszSectionName;
    pParseState->bSkipSection = bSkipSection;
    *pbContinue = bContinue;
    
cleanup:

    return dwError;
    
error:

    LWPS_SAFE_FREE_STRING(pszSectionName);
    pParseState->pszSectionName = NULL;
    pParseState->bSkipSection = FALSE;
    *pbContinue = FALSE;

    goto cleanup;
}

DWORD
LwpsCfgProcessNameValuePair(
    PLWPS_CONFIG_PARSE_STATE pParseState,
    PLWPS_STACK*             ppTokenStack,
    PBOOLEAN                pbContinue
    )
{
    DWORD      dwError = 0;
    BOOLEAN    bContinue = TRUE;
    PSTR       pszName = NULL;
    PSTR       pszValue = NULL;
    PLWPS_CFG_TOKEN pToken = NULL;
    
    *ppTokenStack = LwpsStackReverse(*ppTokenStack);
    pToken = (PLWPS_CFG_TOKEN)LwpsStackPop(ppTokenStack);
    if (pToken && pToken->dwLen) {
        dwError = LwpsStrndup(
                    pToken->pszToken,
                    pToken->dwLen,
                    &pszName);
        BAIL_ON_LWPS_ERROR(dwError);
    }
    
    if (IsNullOrEmptyString(pszName)) {
        dwError = LWPS_ERROR_INVALID_CONFIG;
        BAIL_ON_LWPS_ERROR(dwError);
    }
    
    LwpsCfgFreeToken(pToken);
    pToken = NULL;
    
    pToken = (PLWPS_CFG_TOKEN)LwpsStackPop(ppTokenStack);
    if (!pToken || pToken->tokenType != LwpsCfgEquals) 
    {
        dwError = LWPS_ERROR_INVALID_CONFIG;
        BAIL_ON_LWPS_ERROR(dwError);
    }
    
    LwpsCfgFreeToken(pToken);
    pToken = NULL;
    
    //this will consume the token stack
    dwError = LwpsCfgProcessTokenStackIntoString(
        ppTokenStack,
        &pszValue);
    BAIL_ON_LWPS_ERROR(dwError);
    
    if (pParseState->pfnNameValuePairHandler  &&
        !pParseState->bSkipSection) {
        
        if(pParseState->dwOptions & LWPS_CFG_OPTION_STRIP_NAME_VALUE_PAIR) 
        {
            LwpsStripWhitespace(pszName, TRUE, TRUE); 
            LwpsStripWhitespace(pszValue, TRUE, TRUE); 
        }
        
        dwError = pParseState->pfnNameValuePairHandler( 
                        pszName,
                        pszValue,
                        pParseState->pData,
                        &bContinue);
        BAIL_ON_LWPS_ERROR(dwError);
        
    }
    
    *pbContinue = bContinue;
    
cleanup:

    if (pToken) {
        LwpsCfgFreeToken(pToken);
        pToken = NULL;
    }
    
    if(ppTokenStack && *ppTokenStack)
    {
        dwError = LwpsCfgFreeTokenStack(ppTokenStack);
    }
    
    LWPS_SAFE_FREE_STRING(pszName);
    LWPS_SAFE_FREE_STRING(pszValue);

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LwpsCfgProcessEndSection(
    PLWPS_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN                pbContinue
    )
{
    DWORD dwError = 0;
    BOOLEAN bContinue = TRUE;
    
    if (pParseState->pfnEndSectionHandler &&
        !pParseState->bSkipSection) {
        
        if(pParseState->dwOptions & LWPS_CFG_OPTION_STRIP_SECTION) 
        {
            LwpsStripWhitespace(pParseState->pszSectionName, TRUE, TRUE); 
        }
        
        
        dwError = pParseState->pfnEndSectionHandler(
                        pParseState->pszSectionName,
                        pParseState->pData,
                        &bContinue);
        BAIL_ON_LWPS_ERROR(dwError);
    }
    
    *pbContinue = bContinue;
    
cleanup:

    LWPS_SAFE_FREE_STRING(pParseState->pszSectionName);

    return dwError;
    
error:

    *pbContinue = TRUE;

    goto cleanup;
}

DWORD
LwpsCfgDetermineTokenLength(
    PLWPS_STACK pStack
    )
{
    DWORD dwLen = 0;
    PLWPS_STACK pIter = pStack;
    
    for (; pIter; pIter = pIter->pNext)
    {
        PLWPS_CFG_TOKEN pToken = (PLWPS_CFG_TOKEN)pIter->pItem;
        if (pToken) {
            dwLen += pToken->dwLen;
        }
    }
    
    return dwLen;
}

//this will consume the token stack
DWORD
LwpsCfgProcessTokenStackIntoString(
    PLWPS_STACK* ppTokenStack,
    PSTR* ppszConcatenated
    )
{
    DWORD   dwError = 0;
    DWORD   dwRequiredTokenLen = 0;
    PSTR    pszConcatenated = NULL;
    
    dwRequiredTokenLen = LwpsCfgDetermineTokenLength(*ppTokenStack);
    
    if (dwRequiredTokenLen) {
        
        PSTR pszPos = NULL;
        PLWPS_CFG_TOKEN pToken = NULL;
        
        *ppTokenStack = LwpsStackReverse(*ppTokenStack);
        
	
        dwError = LwpsAllocateMemory(
                        (dwRequiredTokenLen + 1) * sizeof(CHAR),
                        (PVOID*)&pszConcatenated);
        BAIL_ON_LWPS_ERROR(dwError);
	        

        pszPos = pszConcatenated;
        while (*ppTokenStack)
        {
            pToken = LwpsStackPop(ppTokenStack);
            if (pToken && pToken->dwLen) {
                    
                strncpy(pszPos, pToken->pszToken, pToken->dwLen);
                pszPos += pToken->dwLen;
                    
                LwpsCfgFreeToken(pToken);
                pToken = NULL;
            }
        }
    }
    
    *ppszConcatenated = pszConcatenated;
    
cleanup:

    return dwError;
    
error:

    LWPS_SAFE_FREE_STRING(pszConcatenated);
    
    *ppszConcatenated = NULL;

    goto cleanup;
}


DWORD
LwpsCfgAllocateToken(
    DWORD           dwSize,
    PLWPS_CFG_TOKEN* ppToken
    )
{
    DWORD dwError = 0;
    PLWPS_CFG_TOKEN pToken = NULL;
    DWORD dwMaxLen = (dwSize > 0 ? dwSize : LWPS_CFG_TOKEN_DEFAULT_LENGTH);
    
    
    dwError = LwpsAllocateMemory(
                    sizeof(LWPS_CFG_TOKEN),
                    (PVOID*)&pToken);
    BAIL_ON_LWPS_ERROR(dwError);
    
    dwError = LwpsAllocateMemory(
                    dwMaxLen * sizeof(CHAR),
                    (PVOID*)&pToken->pszToken);
    BAIL_ON_LWPS_ERROR(dwError);
        

    pToken->tokenType = LwpsCfgNone;
    pToken->dwMaxLen = dwMaxLen;
    
    *ppToken = pToken;
    
cleanup:
    
    return dwError;
    
error:

    *ppToken = NULL;
    
    if (pToken) {
        LwpsCfgFreeToken(pToken);
    }

    goto cleanup;
}

DWORD
LwpsCfgReallocToken(
    PLWPS_CFG_TOKEN pToken,
    DWORD          dwNewSize
    )
{
    DWORD dwError = 0;
    
    dwError = LwpsReallocMemory(
                    pToken->pszToken,
                    (PVOID*)&pToken->pszToken,
                    dwNewSize);
    BAIL_ON_LWPS_ERROR(dwError);
    
    pToken->dwMaxLen = dwNewSize;
    
cleanup:

    return dwError;
    
error:

    goto cleanup;
}

VOID
LwpsCfgResetToken(
    PLWPS_CFG_TOKEN pToken
    )
{
    if (pToken && pToken->pszToken && pToken->dwMaxLen)
    {
        memset(pToken->pszToken, 0, pToken->dwMaxLen);
        pToken->dwLen = 0;
    }
}

DWORD
LwpsCfgCopyToken(
    PLWPS_CFG_TOKEN pTokenSrc,
    PLWPS_CFG_TOKEN pTokenDst
    )
{
    DWORD dwError = 0;
    
    pTokenDst->tokenType = pTokenSrc->tokenType;
    
    if (pTokenSrc->dwLen > pTokenDst->dwLen) {
        dwError = LwpsReallocMemory(
                        (PVOID)  pTokenDst->pszToken,
                        (PVOID*) &pTokenDst->pszToken,
                        (DWORD)  pTokenSrc->dwLen);
        BAIL_ON_LWPS_ERROR(dwError);
        
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
LwpsCfgFreeTokenStack(
    PLWPS_STACK* ppTokenStack
    )
{
    
    DWORD dwError = 0;
    
    PLWPS_STACK pTokenStack = *ppTokenStack;
    
    dwError = LwpsStackForeach(
            pTokenStack,
            &LwpsCfgFreeTokenInStack,
            NULL);
    BAIL_ON_LWPS_ERROR(dwError);
    
    LwpsStackFree(pTokenStack);
    
    *ppTokenStack = NULL; 
    
error:

    return dwError;
}

DWORD
LwpsCfgFreeTokenInStack(
    PVOID pToken,
    PVOID pUserData
    )
{
    if (pToken)
    {
        LwpsCfgFreeToken((PLWPS_CFG_TOKEN)pToken);
    }
    
    return 0;
}

VOID
LwpsCfgFreeToken(
    PLWPS_CFG_TOKEN pToken
    )
{
    LWPS_SAFE_FREE_MEMORY(pToken->pszToken);
    LwpsFreeMemory(pToken);
}

DWORD
LwpsCfgGetNextToken(
    PLWPS_CONFIG_PARSE_STATE pParseState,
    PLWPS_CFG_TOKEN*         ppToken
    )
{
    DWORD dwError = 0;
    LwpsCfgTokenType tokenType = LwpsCfgNone;
    LwpsCfgLexState  curLexState = LwpsLexBegin;
    PLWPS_CFG_TOKEN  pToken = NULL;
    BOOLEAN         bOwnToken = FALSE;
    
    if (LwpsStackPeek(pParseState->pLexerTokenStack) != NULL) 
    {
        PLWPS_CFG_TOKEN pToken_input = *ppToken;
        
        pToken = (PLWPS_CFG_TOKEN)LwpsStackPop(&pParseState->pLexerTokenStack);
        bOwnToken = TRUE;
        
        if (pToken_input) {
            
            dwError = LwpsCfgCopyToken(pToken, pToken_input);
            BAIL_ON_LWPS_ERROR(dwError);
            
            LwpsCfgFreeToken(pToken);
            pToken = NULL;
            bOwnToken = FALSE;
            
        }
        
        goto done;
    }
    
    pToken = *ppToken;
    
    if (!pToken) {
        dwError = LwpsCfgAllocateToken(
                        0,
                        &pToken);
        BAIL_ON_LWPS_ERROR(dwError);
        
        bOwnToken = TRUE;
    }
    else
    {
        LwpsCfgResetToken(pToken);
    }

    while (curLexState != LwpsLexEnd)
    {
        DWORD ch = LwpsCfgGetCharacter(pParseState);
        LwpsCfgLexState lexClass = LwpsCfgGetLexClass(ch);

        if (lexClass != LwpsLexEOF) {
            pParseState->dwCol++;
        }

        if (lexClass == LwpsLexNewline) {
            pParseState->dwLine++;
            pParseState->dwCol = 0;
        }

        tokenType = LwpsCfgGetTokenType(curLexState, lexClass);

        switch(LwpsCfgGetLexAction(curLexState, lexClass))
        {
            case Skip:
                
                break;
                
            case Consume:

                if (pToken->dwLen >= pToken->dwMaxLen) {
                    dwError = LwpsCfgReallocToken(
                                    pToken,
                                    pToken->dwMaxLen + LWPS_CFG_TOKEN_DEFAULT_LENGTH);
                    BAIL_ON_LWPS_ERROR(dwError);
                }
                
                pToken->pszToken[pToken->dwLen++] = (BYTE)ch;

                break;
                
            case Pushback:

	        if (lexClass == LwpsLexNewline)
	        {
                    pParseState->dwLine--;
	        }
                pParseState->dwCol--;
                dwError = LwpsCfgPushBackCharacter(
                                pParseState,
                                (BYTE)ch);
                BAIL_ON_LWPS_ERROR(dwError);
 
                break;
        }

        curLexState = LwpsCfgGetNextLexState(curLexState, lexClass);
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
        LwpsCfgFreeToken(pToken);
        *ppToken = NULL;
    }

    goto cleanup;
}

DWORD
LwpsCfgGetCharacter(
    PLWPS_CONFIG_PARSE_STATE pParseState
    )
{
    return getc(pParseState->fp);
}

LwpsCfgLexState
LwpsCfgGetLexClass(
    DWORD ch
    )
{
    
    if (ch == EOF) {
        return LwpsLexEOF;
    }
    
    if (ch == '\n') {
        return LwpsLexNewline;
    }

    if (ch == '[') {
        return LwpsLexLSqBrace;
    }

    if (ch == ']') {
        return LwpsLexRSqBrace;
    }

    if (ch == '=') {
        return LwpsLexEquals;
    }
    
    if (ch == '#') {
        return LwpsLexHash;
    }

    if (isalnum(ch) ||
        ispunct(ch) ||
        isblank(ch)) {
        return LwpsLexChar;
    }

    return LwpsLexOther;
}

DWORD
LwpsCfgPushBackCharacter(
    PLWPS_CONFIG_PARSE_STATE pParseState,
    BYTE ch
    )
{
    return ((EOF == ungetc(ch, pParseState->fp)) ? errno : 0);
}

LwpsCfgLexState
LwpsCfgGetNextLexState(
    LwpsCfgLexState currentState,
    DWORD chId
    )
{
    return (gLwpsLexStateTable[currentState][chId].nextState);
}

LwpsCfgLexAction
LwpsCfgGetLexAction(
    LwpsCfgLexState currentState,
    DWORD chId
    )
{
    return (gLwpsLexStateTable[currentState][chId].action);
}

LwpsCfgTokenType
LwpsCfgGetTokenType(
    LwpsCfgLexState currentState,
    DWORD chId
    )
{
    return (gLwpsLexStateTable[currentState][chId].tokenId);
}

#if !(HAVE_DECL_ISBLANK - 0)
static
int
isblank(int c)
{
    return c == '\t' || c == ' ';
}
#endif

