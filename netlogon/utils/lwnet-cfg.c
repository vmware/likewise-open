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
 *        lwnet-cfg.c
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
#include "includes.h"

static LWNET_CFG_LEXER_STATE gLWNetLexStateTable[][9] =
{
  /* LWNetLexBegin    := 0 */
  {
    {   LWNetLexBegin,  Consume,             LWNetCfgNone }, /* LWNetLexBegin    */
    {    LWNetLexChar,  Consume,             LWNetCfgNone }, /* LWNetLexChar     */
    {     LWNetLexEnd,  Consume,  LWNetCfgLeftSquareBrace }, /* LWNetLexLSqBrace */
    {     LWNetLexEnd,  Consume, LWNetCfgRightSquareBrace }, /* LWNetLexRSqBrace */
    {     LWNetLexEnd,  Consume,           LWNetCfgEquals }, /* LWNetLexEquals   */
    {     LWNetLexEnd,  Consume,             LWNetCfgHash }, /* LWNetLexHash     */
    {     LWNetLexEnd,  Consume,          LWNetCfgNewline }, /* LWNetLexNewline  */
    {   LWNetLexBegin,     Skip,             LWNetCfgNone }, /* LWNetLexOther    */
    {     LWNetLexEnd,  Consume,              LWNetCfgEOF }  /* LWNetLexEOF      */
  },
  /* LWNetLexChar     := 1 */
  {
    {   LWNetLexBegin,  Consume,             LWNetCfgNone }, /* LWNetLexBegin    */ 
    {    LWNetLexChar,  Consume,             LWNetCfgNone }, /* LWNetLexChar     */
    {     LWNetLexEnd, Pushback,           LWNetCfgString }, /* LWNetLexLSqBrace */
    {     LWNetLexEnd, Pushback,           LWNetCfgString }, /* LWNetLexRSqBrace */
    {     LWNetLexEnd, Pushback,           LWNetCfgString }, /* LWNetLexEquals   */
    {     LWNetLexEnd, Pushback,           LWNetCfgString }, /* LWNetLexHash     */
    {     LWNetLexEnd, Pushback,           LWNetCfgString }, /* LWNetLexNewline  */
    {     LWNetLexEnd,     Skip,           LWNetCfgString }, /* LWNetLexOther    */
    {     LWNetLexEnd,     Skip,           LWNetCfgString }  /* LWNetLexEOF      */
  },
  /* LWNetLexLSqBrace := 2 */
  {
    {   LWNetLexBegin,  Consume,             LWNetCfgNone }, /* LWNetLexBegin    */ 
    {     LWNetLexEnd, Pushback,  LWNetCfgLeftSquareBrace }, /* LWNetLexChar     */ 
    {     LWNetLexEnd, Pushback,  LWNetCfgLeftSquareBrace }, /* LWNetLexLSqBrace */
    {     LWNetLexEnd, Pushback,  LWNetCfgLeftSquareBrace }, /* LWNetLexRSqBrace */
    {     LWNetLexEnd, Pushback,  LWNetCfgLeftSquareBrace }, /* LWNetLexEquals   */
    {     LWNetLexEnd, Pushback,  LWNetCfgLeftSquareBrace }, /* LWNetLexHash     */
    {     LWNetLexEnd, Pushback,  LWNetCfgLeftSquareBrace }, /* LWNetLexNewline  */
    {     LWNetLexEnd,     Skip,  LWNetCfgLeftSquareBrace }, /* LWNetLexOther    */
    {     LWNetLexEnd,     Skip,  LWNetCfgLeftSquareBrace }  /* LWNetLexEOF      */
  },
  /* LWNetLexRSqBrace := 3 */
  {
    {   LWNetLexBegin,  Consume,             LWNetCfgNone }, /* LWNetLexBegin    */
    {     LWNetLexEnd, Pushback, LWNetCfgRightSquareBrace }, /* LWNetLexChar     */
    {     LWNetLexEnd, Pushback, LWNetCfgRightSquareBrace }, /* LWNetLexLSqBrace */
    {     LWNetLexEnd, Pushback, LWNetCfgRightSquareBrace }, /* LWNetLexRSqBrace */
    {     LWNetLexEnd, Pushback, LWNetCfgRightSquareBrace }, /* LWNetLexEquals   */
    {     LWNetLexEnd, Pushback, LWNetCfgRightSquareBrace }, /* LWNetLexHash     */
    {     LWNetLexEnd, Pushback, LWNetCfgRightSquareBrace }, /* LWNetLexNewline  */
    {     LWNetLexEnd,     Skip, LWNetCfgRightSquareBrace }, /* LWNetLexOther    */
    {     LWNetLexEnd,     Skip, LWNetCfgRightSquareBrace }  /* LWNetLexEOF      */
  },
  /* LWNetLexEquals   := 4 */
  {
    {   LWNetLexBegin,  Consume,             LWNetCfgNone }, /* LWNetLexBegin    */
    {     LWNetLexEnd, Pushback,           LWNetCfgEquals }, /* LWNetLexChar     */
    {     LWNetLexEnd, Pushback,           LWNetCfgEquals }, /* LWNetLexLSqBrace */
    {     LWNetLexEnd, Pushback,           LWNetCfgEquals }, /* LWNetLexRSqBrace */
    {     LWNetLexEnd, Pushback,           LWNetCfgEquals }, /* LWNetLexEquals   */
    {     LWNetLexEnd, Pushback,           LWNetCfgEquals }, /* LWNetLexHash     */
    {     LWNetLexEnd, Pushback,           LWNetCfgEquals }, /* LWNetLexNewline  */
    {     LWNetLexEnd,     Skip,           LWNetCfgEquals }, /* LWNetLexOther    */
    {     LWNetLexEnd,     Skip,           LWNetCfgEquals }  /* LWNetLexEOF      */
  },
  /* LWNetLexHash     := 5 */
  {
    {   LWNetLexBegin,  Consume,             LWNetCfgNone }, /* LWNetLexBegin    */
    {     LWNetLexEnd, Pushback,             LWNetCfgHash }, /* LWNetLexChar     */
    {     LWNetLexEnd, Pushback,             LWNetCfgHash }, /* LWNetLexLSqBrace */
    {     LWNetLexEnd, Pushback,             LWNetCfgHash }, /* LWNetLexRSqBrace */
    {     LWNetLexEnd, Pushback,             LWNetCfgHash }, /* LWNetLexEquals   */
    {     LWNetLexEnd, Pushback,             LWNetCfgHash }, /* LWNetLexHash     */
    {     LWNetLexEnd, Pushback,             LWNetCfgHash }, /* LWNetLexNewline  */
    {     LWNetLexEnd,     Skip,             LWNetCfgHash }, /* LWNetLexOther    */
    {     LWNetLexEnd,     Skip,             LWNetCfgHash }  /* LWNetLexEOF      */
  },
  /* LWNetLexNewline  := 6 */
  {
    {   LWNetLexBegin,  Consume,             LWNetCfgNone }, /* LWNetLexBegin    */
    {     LWNetLexEnd, Pushback,          LWNetCfgNewline }, /* LWNetLexChar     */
    {     LWNetLexEnd, Pushback,          LWNetCfgNewline }, /* LWNetLexLSqBrace */
    {     LWNetLexEnd, Pushback,          LWNetCfgNewline }, /* LWNetLexRSqBrace */
    {     LWNetLexEnd, Pushback,          LWNetCfgNewline }, /* LWNetLexEquals   */
    {     LWNetLexEnd, Pushback,          LWNetCfgNewline }, /* LWNetLexHash     */
    {     LWNetLexEnd, Pushback,          LWNetCfgNewline }, /* LWNetLexNewline  */
    {     LWNetLexEnd,     Skip,          LWNetCfgNewline }, /* LWNetLexOther    */
    {     LWNetLexEnd,     Skip,          LWNetCfgNewline }  /* LWNetLexEOF      */
  },
  /* LWNetLexOther    := 7 */
  {
    {   LWNetLexBegin,  Consume,             LWNetCfgNone }, /* LWNetLexBegin    */
    {    LWNetLexChar,  Consume,             LWNetCfgNone }, /* LWNetLexChar     */
    {LWNetLexLSqBrace,  Consume,             LWNetCfgNone }, /* LWNetLexLSqBrace */
    {LWNetLexRSqBrace,  Consume,             LWNetCfgNone }, /* LWNetLexRSqBrace */
    {  LWNetLexEquals,  Consume,             LWNetCfgNone }, /* LWNetLexEquals   */
    {    LWNetLexHash,  Consume,             LWNetCfgNone }, /* LWNetLexHash     */
    { LWNetLexNewline,  Consume,             LWNetCfgNone }, /* LWNetLexNewline  */
    {   LWNetLexOther,  Consume,             LWNetCfgNone }, /* LWNetLexOther    */
    {     LWNetLexEOF,  Consume,             LWNetCfgNone }  /* LWNetLexEOF      */
  },
  /* LWNetLexEOF      := 8 */
  {
    {   LWNetLexBegin,  Consume,             LWNetCfgNone }, /* LWNetLexBegin    */
    {     LWNetLexEnd,  Consume,             LWNetCfgNone }, /* LWNetLexChar     */
    {     LWNetLexEnd,  Consume,             LWNetCfgNone }, /* LWNetLexLSqBrace */
    {     LWNetLexEnd,  Consume,             LWNetCfgNone }, /* LWNetLexRSqBrace */
    {     LWNetLexEnd,  Consume,             LWNetCfgNone }, /* LWNetLexEquals   */
    {     LWNetLexEnd,  Consume,             LWNetCfgNone }, /* LWNetLexHash     */
    { LWNetLexNewline,  Consume,             LWNetCfgNone }, /* LWNetLexNewline  */
    {     LWNetLexEnd,  Consume,             LWNetCfgNone }, /* LWNetLexOther    */
    {     LWNetLexEnd,  Consume,             LWNetCfgNone }  /* LWNetLexEOF      */
  }
};

DWORD
LWNetParseConfigFile(
    PCSTR                     pszFilePath,
    DWORD                     dwOptions,
    PFNLWNET_CONFIG_START_SECTION   pfnStartSectionHandler,
    PFNLWNET_CONFIG_COMMENT         pfnCommentHandler,
    PFNLWNET_CONFIG_NAME_VALUE_PAIR pfnNameValuePairHandler,
    PFNLWNET_CONFIG_END_SECTION     pfnEndSectionHandler,
    PVOID                     pData
    )
{
    DWORD dwError = 0;
    PLWNET_CONFIG_PARSE_STATE pParseState = NULL;
    
    dwError = LWNetCfgInitParseState(
                    pszFilePath,
                    dwOptions, 
                    pfnStartSectionHandler,
                    pfnCommentHandler,
                    pfnNameValuePairHandler,
                    pfnEndSectionHandler,
                    pData,
                    &pParseState);
    BAIL_ON_LWNET_ERROR(dwError);
    
    dwError = LWNetCfgParse(pParseState);
    BAIL_ON_LWNET_ERROR(dwError);
    
cleanup:
    
    if (pParseState) {
        LWNetCfgFreeParseState(pParseState);
    }

    return dwError;
    
error:

    goto cleanup;
}

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
    )
{
    DWORD dwError = 0;
    PLWNET_CONFIG_PARSE_STATE pParseState = NULL;
    PLWNET_STACK pTokenStack = NULL;
    FILE* fp = NULL;

    if ((fp = fopen(pszFilePath, "r")) == NULL) {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LWNET_ERROR(dwError);
    }
    
    
    dwError = LWNetAllocateMemory(
                    sizeof(LWNET_CONFIG_PARSE_STATE),
                    (PVOID*)&pParseState);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetAllocateMemory(
                    sizeof(LWNET_STACK), 
                    (PVOID*)&pTokenStack);
    BAIL_ON_LWNET_ERROR(dwError);

    pParseState->pLexerTokenStack = pTokenStack;

    dwError = LWNetAllocateString(
                    pszFilePath,
                    &(pParseState->pszFilePath));
    BAIL_ON_LWNET_ERROR(dwError);
    
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
        LWNetCfgFreeParseState(pParseState);
    }
    
    if (fp) {
        fclose(fp);
    }

    goto cleanup;
}

VOID
LWNetCfgFreeParseState(
    PLWNET_CONFIG_PARSE_STATE pParseState
    )
{
    LWNET_SAFE_FREE_STRING(pParseState->pszFilePath);
    LWNET_SAFE_FREE_STRING(pParseState->pszSectionName);
    if (pParseState->pLexerTokenStack) {
        LWNetCfgFreeTokenStack(&pParseState->pLexerTokenStack);
    }
    
    if (pParseState->fp) {
        fclose(pParseState->fp);
    }
    LWNetFreeMemory(pParseState);
}
  

DWORD
LWNetCfgParse(
    PLWNET_CONFIG_PARSE_STATE pParseState
    )
{
    
    DWORD dwError = 0;
    PLWNET_CFG_TOKEN pToken = NULL;
    BOOLEAN bContinue = TRUE;
    PLWNET_STACK pTokenStack = NULL;
    
    do 
    {
    
        dwError = LWNetCfgGetNextToken(
                        pParseState,
                        &pToken);
        BAIL_ON_LWNET_ERROR(dwError);
        
        switch (pToken->tokenType)
        {
            case LWNetCfgHash:
            {
                dwError = LWNetCfgParseComment(
                                pParseState,
                                &bContinue);
                BAIL_ON_LWNET_ERROR(dwError);
                
                break;
            }
            case LWNetCfgNewline:
            {
                //in case of a blank line (newline),
                //interpret this as an empty comment.
                dwError = LWNetCfgProcessComment(
                                pParseState, 
                                &pTokenStack,
                                &bContinue);
                BAIL_ON_LWNET_ERROR(dwError);
                
                break;
            }
            case LWNetCfgLeftSquareBrace:
            {
                
                dwError = LWNetCfgParseSections(
                                pParseState);
                BAIL_ON_LWNET_ERROR(dwError);
                
                break;
            }
            case LWNetCfgEOF:
            {
                bContinue = FALSE;
                break;
            }
            default:
            {
                dwError = ERROR_BAD_CONFIGURATION;
                BAIL_ON_LWNET_ERROR(dwError);
            }
        }
        
    } while (bContinue);
    
cleanup:
    
    if (pToken) {
        LWNetCfgFreeToken(pToken);
    }
    
    return dwError;
    
error: 

    if (dwError == ERROR_BAD_CONFIGURATION)
    {
        if (pParseState) {                                                             
            LWNET_LOG_ERROR ("Parse error at line=%d, column=%d of file [%s]",  
                          pParseState->dwLine, 
                          pParseState->dwCol, 
                          IsNullOrEmptyString(pParseState->pszFilePath) ? 
                              "" : pParseState->pszFilePath);                  
        }
    }

    goto cleanup;
}

DWORD
LWNetCfgParseSections(
    PLWNET_CONFIG_PARSE_STATE pParseState
    )
{
    DWORD dwError = 0;
    PLWNET_CFG_TOKEN pToken = NULL;
    BOOLEAN bContinue = TRUE;
    PLWNET_STACK pTokenStack = NULL;
    
    dwError = LWNetCfgParseSectionHeader(
                    pParseState, 
                    &bContinue);
    BAIL_ON_LWNET_ERROR(dwError);
    
    while (bContinue)
    {
        dwError = LWNetCfgGetNextToken(
                        pParseState,
                        &pToken
                        );
        BAIL_ON_LWNET_ERROR(dwError);
        
        switch (pToken->tokenType)
        {
            case LWNetCfgString:
            {
                BOOLEAN bIsAllSpace = FALSE;
                
                dwError = LwStrIsAllSpace(
                                pToken->pszToken,
                                &bIsAllSpace
                                );
                BAIL_ON_LWNET_ERROR(dwError);
                                
                if (bIsAllSpace)
                {
                    continue;
                }
                            
                dwError = LWNetStackPush(pToken, &(pParseState->pLexerTokenStack));
                BAIL_ON_LWNET_ERROR(dwError);
                
                pToken = NULL;
                
                dwError = LWNetCfgParseNameValuePair(
                                pParseState,
                                &bContinue
                                );
                BAIL_ON_LWNET_ERROR(dwError);
                break;                   
            }
        
            case LWNetCfgHash:
            {
                dwError = LWNetCfgParseComment(
                                pParseState,
                                &bContinue
                                );
                BAIL_ON_LWNET_ERROR(dwError);
                break;
            }
            case LWNetCfgNewline:
            {
                //in case of a blank line (newline),
                //interpret this as an empty comment.
                dwError = LWNetCfgProcessComment(
                                pParseState,
                                &pTokenStack,
                                &bContinue);
                BAIL_ON_LWNET_ERROR(dwError);
                break;
            }
            case LWNetCfgLeftSquareBrace:
            {
                dwError = LWNetCfgParseSectionHeader(
                                pParseState, 
                                &bContinue);
                BAIL_ON_LWNET_ERROR(dwError);

                break;
            }
            case LWNetCfgEOF:
            {
                bContinue = FALSE;
                break;
            }
            default:
            {
                dwError = ERROR_BAD_CONFIGURATION;
                BAIL_ON_LWNET_ERROR(dwError);
            }
        }
    } 
    
    if (bContinue && !IsNullOrEmptyString(pParseState->pszSectionName))
    {
        dwError = LWNetCfgProcessEndSection(
                        pParseState,
                        &bContinue);
        BAIL_ON_LWNET_ERROR(dwError);
    }
    
cleanup:
    
    if (pToken) {
        LWNetCfgFreeToken(pToken);
    }
    
    return dwError;
    
error:

    goto cleanup;
}



DWORD
LWNetCfgParseComment(
    PLWNET_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN    pbContinue
    )
{
    DWORD dwError = 0;
    BOOLEAN bContinue = TRUE;
    BOOLEAN bKeepParsing = TRUE;
    PLWNET_CFG_TOKEN pToken = NULL;
    PLWNET_STACK pTokenStack = NULL;
    
    do
    {
        dwError = LWNetCfgGetNextToken(
                    pParseState,
                    &pToken);
        BAIL_ON_LWNET_ERROR(dwError);
        
        switch (pToken->tokenType)
        {
            case LWNetCfgEOF:
            {
                dwError = LWNetCfgProcessComment(
                                pParseState,
                                &pTokenStack,
                                &bContinue);
                BAIL_ON_LWNET_ERROR(dwError);
                
                bContinue = FALSE;
                
                break;
            }
            case LWNetCfgNewline:
            {
                dwError = LWNetCfgProcessComment(
                                pParseState,
                                &pTokenStack,
                                &bContinue);
                BAIL_ON_LWNET_ERROR(dwError);
                
                bKeepParsing = FALSE;
                
                break;
            }
            default:
            {
                dwError = LWNetStackPush(pToken, &pTokenStack);
                BAIL_ON_LWNET_ERROR(dwError);
                
                pToken = NULL;
                
            }
        }
        
    } while (bContinue && bKeepParsing);
    
    *pbContinue = bContinue;
    
cleanup:

    if (pToken) {
        LWNetCfgFreeToken(pToken);
    }

    return dwError;
    
error:

    *pbContinue = FALSE;
    
    goto cleanup;
}

DWORD
LWNetCfgParseSectionHeader(
    PLWNET_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    BOOLEAN bKeepParsing = TRUE;
    BOOLEAN bContinue = TRUE;
    PLWNET_CFG_TOKEN pToken = NULL;
    PLWNET_STACK pTokenStack = NULL;
    
    if(!IsNullOrEmptyString(pParseState->pszSectionName))
    {
        dwError = LWNetCfgProcessEndSection(
                        pParseState,
                        &bContinue);
        BAIL_ON_LWNET_ERROR(dwError);
        
    }
    
    if (!bContinue)
    {
        goto done;
    }
    
    pParseState->bSkipSection = FALSE;
    
    do
    {
        dwError = LWNetCfgGetNextToken(
                        pParseState,
                        &pToken);
        BAIL_ON_LWNET_ERROR(dwError);
        
        switch(pToken->tokenType)
        {
            case LWNetCfgString:
            case LWNetCfgEquals:
            case LWNetCfgOther:
            { 
                dwError = LWNetStackPush(pToken, &pTokenStack);
                BAIL_ON_LWNET_ERROR(dwError);
                
                pToken = NULL;
                break;
            }
            case LWNetCfgRightSquareBrace:
            {
                dwError = LWNetAssertWhitespaceOnly(pParseState);
                BAIL_ON_LWNET_ERROR(dwError);
                
                bKeepParsing = FALSE;
                
                break;
            }
            default:
            {
                dwError = ERROR_BAD_CONFIGURATION;
                BAIL_ON_LWNET_ERROR(dwError);
            }
        }
        
    } while (bKeepParsing);
    
    dwError = LWNetCfgGetNextToken(
                    pParseState,
                    &pToken);
    BAIL_ON_LWNET_ERROR(dwError);
    
    switch(pToken->tokenType)
    {
    
        case LWNetCfgNewline:
        {
            dwError = LWNetCfgProcessBeginSection(
                            pParseState,
                            &pTokenStack,
                            &bContinue);
            BAIL_ON_LWNET_ERROR(dwError);

            break;
        }
        case LWNetCfgEOF:
        {
            dwError = LWNetCfgProcessBeginSection(
                            pParseState,
                            &pTokenStack,
                            &bContinue);
            BAIL_ON_LWNET_ERROR(dwError);
            
            if (bContinue) {
                
                dwError = LWNetCfgProcessEndSection(
                                pParseState,
                                &bContinue);
                BAIL_ON_LWNET_ERROR(dwError);
            }
            
            bContinue = FALSE;
            
            break;     
        }
        default:
        {
            dwError = ERROR_BAD_CONFIGURATION;
            BAIL_ON_LWNET_ERROR(dwError);
        }
    }
    
done:

    *pbContinue = bContinue;
    
cleanup:

    if (pToken) {
        LWNetCfgFreeToken(pToken);
    }

    return dwError;
    
error:

    *pbContinue = FALSE;

    goto cleanup;
}

DWORD
LWNetAssertWhitespaceOnly(
    PLWNET_CONFIG_PARSE_STATE pParseState
    )
{
    DWORD dwError = 0;
    PLWNET_CFG_TOKEN pToken = NULL;
    BOOLEAN bKeepParsing = TRUE;
    
    do
    {
        dwError = LWNetCfgGetNextToken(
                        pParseState,
                        &pToken);
        BAIL_ON_LWNET_ERROR(dwError);
        
        switch(pToken->tokenType)
        {
            case LWNetCfgString:
            case LWNetCfgOther:
            {
                DWORD i = 0;
                for (; i < pToken->dwLen; i++) {
                    if (!isspace((int)pToken->pszToken[i])) {
                        dwError = ERROR_BAD_CONFIGURATION;
                        BAIL_ON_LWNET_ERROR(dwError);
                    }
                }
                break;
            }
            case LWNetCfgEOF:
            case LWNetCfgNewline:
            {
                dwError = LWNetStackPush(pToken, &pParseState->pLexerTokenStack);
                BAIL_ON_LWNET_ERROR(dwError);
                
                pToken = NULL;
                
                bKeepParsing = FALSE;
                
                break;
            }
            default:
            {
                dwError = ERROR_BAD_CONFIGURATION;
                BAIL_ON_LWNET_ERROR(dwError);
            }
        }
    } while (bKeepParsing);
    
cleanup:

    if (pToken) {
        LWNetCfgFreeToken(pToken);
    }

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LWNetCfgParseNameValuePair(
    PLWNET_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    BOOLEAN bContinue = TRUE;
    BOOLEAN bKeepParsing = TRUE;
    PLWNET_CFG_TOKEN pToken = NULL;
    PLWNET_STACK pTokenStack = NULL;
    
    //format is <str><equals><token1><token2>...<newline>
    
    //get initial <str>
    dwError = LWNetCfgGetNextToken(
                    pParseState,
                    &pToken);
    BAIL_ON_LWNET_ERROR(dwError);
    
    if(pToken->tokenType == LWNetCfgString) 
    {
        dwError = LWNetStackPush(pToken, &pTokenStack);
        BAIL_ON_LWNET_ERROR(dwError);
        pToken = NULL;
    }
    else
    {
        dwError = ERROR_BAD_CONFIGURATION;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    //get <equals>
    dwError = LWNetCfgGetNextToken(
                    pParseState,
                    &pToken);
    BAIL_ON_LWNET_ERROR(dwError);
   
    if(pToken->tokenType == LWNetCfgEquals) 
    {
        dwError = LWNetStackPush(pToken, &pTokenStack);
        BAIL_ON_LWNET_ERROR(dwError);
        pToken = NULL;
    }
    else
    {
        dwError = ERROR_BAD_CONFIGURATION;
        BAIL_ON_LWNET_ERROR(dwError);
    }


    do 
    {
        dwError = LWNetCfgGetNextToken(
                        pParseState,
                        &pToken);
        BAIL_ON_LWNET_ERROR(dwError);
        
        switch(pToken->tokenType)
        {
            case LWNetCfgString:
            case LWNetCfgEquals:
            case LWNetCfgOther:
            {
                
                dwError = LWNetStackPush(pToken, &pTokenStack);
                BAIL_ON_LWNET_ERROR(dwError);
                pToken = NULL;
                
                break;

            }
            case LWNetCfgNewline:
            {
                dwError = LWNetCfgProcessNameValuePair(
                    pParseState,
                    &pTokenStack,
                    &bContinue);
                BAIL_ON_LWNET_ERROR(dwError);
                bKeepParsing = FALSE; 
                break;
            }
            case LWNetCfgEOF: 
            {
                dwError = LWNetCfgProcessNameValuePair(
                    pParseState,
                    &pTokenStack,
                    &bContinue);
                BAIL_ON_LWNET_ERROR(dwError);
                bContinue = FALSE;
                break;
            }
            default:
            {
                dwError = ERROR_BAD_CONFIGURATION;
                BAIL_ON_LWNET_ERROR(dwError);
            }
        }
    } while(bContinue && bKeepParsing);
    
    
    *pbContinue = bContinue;
    
cleanup:

    if (pToken) {
        LWNetCfgFreeToken(pToken);
    }

    return dwError;
    
error:

    *pbContinue = FALSE;

    goto cleanup;
}

DWORD
LWNetCfgProcessComment(
    PLWNET_CONFIG_PARSE_STATE pParseState,
    PLWNET_STACK* ppTokenStack,
    PBOOLEAN    pbContinue
    )
{
    DWORD   dwError = 0;
    BOOLEAN bContinue = TRUE;
    PSTR    pszComment = NULL;
    
    dwError = LWNetCfgProcessTokenStackIntoString(
        ppTokenStack,
        &pszComment);
    BAIL_ON_LWNET_ERROR(dwError);
    
    if (pParseState->pfnCommentHandler &&
        !pParseState->bSkipSection) {
            
        dwError = pParseState->pfnCommentHandler(
                        pszComment,
                        pParseState->pData,
                        &bContinue);
        BAIL_ON_LWNET_ERROR(dwError);
    }
    
    *pbContinue = bContinue;
    
cleanup:

    LWNET_SAFE_FREE_STRING(pszComment);

    return dwError;
    
error:

    *pbContinue = TRUE;

    goto cleanup;
}

DWORD
LWNetCfgProcessBeginSection(
    PLWNET_CONFIG_PARSE_STATE pParseState,
    PLWNET_STACK* ppTokenStack,
    PBOOLEAN    pbContinue)
{
    DWORD   dwError = 0;
    PSTR    pszSectionName = NULL;
    BOOLEAN bSkipSection = FALSE;
    BOOLEAN bContinue = TRUE;
    
    dwError = LWNetCfgProcessTokenStackIntoString(
        ppTokenStack,
        &pszSectionName);
    BAIL_ON_LWNET_ERROR(dwError);
    
    if (IsNullOrEmptyString(pszSectionName)) {
        dwError = ERROR_BAD_CONFIGURATION;
        BAIL_ON_LWNET_ERROR(dwError);
    }
    
    if (pParseState->pfnStartSectionHandler) {
        
        if(pParseState->dwOptions & LWNET_CFG_OPTION_STRIP_SECTION) 
        {
            LwStripWhitespace(pszSectionName, TRUE, TRUE);
        }
        
        dwError = pParseState->pfnStartSectionHandler(
                        pszSectionName,
                        pParseState->pData,
                        &bSkipSection,
                        &bContinue);
        BAIL_ON_LWNET_ERROR(dwError);
    }
    
    pParseState->pszSectionName = pszSectionName;
    pParseState->bSkipSection = bSkipSection;
    *pbContinue = bContinue;
    
cleanup:

    return dwError;
    
error:

    LWNET_SAFE_FREE_STRING(pszSectionName);
    pParseState->pszSectionName = NULL;
    pParseState->bSkipSection = FALSE;
    *pbContinue = FALSE;

    goto cleanup;
}

DWORD
LWNetCfgProcessNameValuePair(
    PLWNET_CONFIG_PARSE_STATE pParseState,
    PLWNET_STACK*             ppTokenStack,
    PBOOLEAN                pbContinue
    )
{
    DWORD      dwError = 0;
    BOOLEAN    bContinue = TRUE;
    PSTR       pszName = NULL;
    PSTR       pszValue = NULL;
    PLWNET_CFG_TOKEN pToken = NULL;
    
    *ppTokenStack = LWNetStackReverse(*ppTokenStack);
    pToken = (PLWNET_CFG_TOKEN)LWNetStackPop(ppTokenStack);
    if (pToken && pToken->dwLen) {
        dwError = LwStrndup(
                    pToken->pszToken,
                    pToken->dwLen,
                    &pszName);
        BAIL_ON_LWNET_ERROR(dwError);
    }
    
    if (IsNullOrEmptyString(pszName)) {
        dwError = ERROR_BAD_CONFIGURATION;
        BAIL_ON_LWNET_ERROR(dwError);
    }
    
    LWNetCfgFreeToken(pToken);
    pToken = NULL;
    
    pToken = (PLWNET_CFG_TOKEN)LWNetStackPop(ppTokenStack);
    if (!pToken || pToken->tokenType != LWNetCfgEquals) 
    {
        dwError = ERROR_BAD_CONFIGURATION;
        BAIL_ON_LWNET_ERROR(dwError);
    }
    
    LWNetCfgFreeToken(pToken);
    pToken = NULL;
    
    //this will consume the token stack
    dwError = LWNetCfgProcessTokenStackIntoString(
        ppTokenStack,
        &pszValue);
    BAIL_ON_LWNET_ERROR(dwError);
    
    if (pParseState->pfnNameValuePairHandler  &&
        !pParseState->bSkipSection) {
        
        if(pParseState->dwOptions & LWNET_CFG_OPTION_STRIP_NAME_VALUE_PAIR) 
        {
            LwStripWhitespace(pszName, TRUE, TRUE);
            LwStripWhitespace(pszValue, TRUE, TRUE);
        }
        
        dwError = pParseState->pfnNameValuePairHandler( 
                        pszName,
                        pszValue,
                        pParseState->pData,
                        &bContinue);
        BAIL_ON_LWNET_ERROR(dwError);
        
    }
    
    *pbContinue = bContinue;
    
cleanup:

    if (pToken) {
        LWNetCfgFreeToken(pToken);
        pToken = NULL;
    }
    
    if(ppTokenStack && *ppTokenStack)
    {
        dwError = LWNetCfgFreeTokenStack(ppTokenStack);
    }
    
    LWNET_SAFE_FREE_STRING(pszName);
    LWNET_SAFE_FREE_STRING(pszValue);

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LWNetCfgProcessEndSection(
    PLWNET_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN                pbContinue
    )
{
    DWORD dwError = 0;
    BOOLEAN bContinue = TRUE;
    
    if (pParseState->pfnEndSectionHandler &&
        !pParseState->bSkipSection) {
        
        if(pParseState->dwOptions & LWNET_CFG_OPTION_STRIP_SECTION) 
        {
            LwStripWhitespace(pParseState->pszSectionName, TRUE, TRUE);
        }
        
        
        dwError = pParseState->pfnEndSectionHandler(
                        pParseState->pszSectionName,
                        pParseState->pData,
                        &bContinue);
        BAIL_ON_LWNET_ERROR(dwError);
    }
    
    *pbContinue = bContinue;
    
cleanup:

    LWNET_SAFE_FREE_STRING(pParseState->pszSectionName);

    return dwError;
    
error:

    *pbContinue = TRUE;

    goto cleanup;
}

DWORD
LWNetCfgDetermineTokenLength(
    PLWNET_STACK pStack
    )
{
    DWORD dwLen = 0;
    PLWNET_STACK pIter = pStack;
    
    for (; pIter; pIter = pIter->pNext)
    {
        PLWNET_CFG_TOKEN pToken = (PLWNET_CFG_TOKEN)pIter->pItem;
        if (pToken) {
            dwLen += pToken->dwLen;
        }
    }
    
    return dwLen;
}

//this will consume the token stack
DWORD
LWNetCfgProcessTokenStackIntoString(
    PLWNET_STACK* ppTokenStack,
    PSTR* ppszConcatenated
    )
{
    DWORD   dwError = 0;
    DWORD   dwRequiredTokenLen = 0;
    PSTR    pszConcatenated = NULL;
    
    dwRequiredTokenLen = LWNetCfgDetermineTokenLength(*ppTokenStack);
    
    if (dwRequiredTokenLen) {
        
        PSTR pszPos = NULL;
        PLWNET_CFG_TOKEN pToken = NULL;
        
        *ppTokenStack = LWNetStackReverse(*ppTokenStack);
        
    
        dwError = LWNetAllocateMemory(
                        (dwRequiredTokenLen + 1) * sizeof(CHAR),
                        (PVOID*)&pszConcatenated);
        BAIL_ON_LWNET_ERROR(dwError);
            

        pszPos = pszConcatenated;
        while (*ppTokenStack)
        {
            pToken = LWNetStackPop(ppTokenStack);
            if (pToken && pToken->dwLen) {
                    
                strncpy(pszPos, pToken->pszToken, pToken->dwLen);
                pszPos += pToken->dwLen;
                    
                LWNetCfgFreeToken(pToken);
                pToken = NULL;
            }
        }
    }
    
    *ppszConcatenated = pszConcatenated;
    
cleanup:

    return dwError;
    
error:

    LWNET_SAFE_FREE_STRING(pszConcatenated);
    
    *ppszConcatenated = NULL;

    goto cleanup;
}


DWORD
LWNetCfgAllocateToken(
    DWORD           dwSize,
    PLWNET_CFG_TOKEN* ppToken
    )
{
    DWORD dwError = 0;
    PLWNET_CFG_TOKEN pToken = NULL;
    DWORD dwMaxLen = (dwSize > 0 ? dwSize : LWNET_CFG_TOKEN_DEFAULT_LENGTH);
    
    
    dwError = LWNetAllocateMemory(
                    sizeof(LWNET_CFG_TOKEN),
                    (PVOID*)&pToken);
    BAIL_ON_LWNET_ERROR(dwError);
    
    dwError = LWNetAllocateMemory(
                    dwMaxLen * sizeof(CHAR),
                    (PVOID*)&pToken->pszToken);
    BAIL_ON_LWNET_ERROR(dwError);
        

    pToken->tokenType = LWNetCfgNone;
    pToken->dwMaxLen = dwMaxLen;
    
    *ppToken = pToken;
    
cleanup:
    
    return dwError;
    
error:

    *ppToken = NULL;
    
    if (pToken) {
        LWNetCfgFreeToken(pToken);
    }

    goto cleanup;
}

DWORD
LWNetCfgReallocToken(
    PLWNET_CFG_TOKEN pToken,
    DWORD          dwNewSize
    )
{
    DWORD dwError = 0;
    
    dwError = LWNetReallocMemory(
                    pToken->pszToken,
                    (PVOID*)&pToken->pszToken,
                    dwNewSize);
    BAIL_ON_LWNET_ERROR(dwError);
    
    pToken->dwMaxLen = dwNewSize;
    
cleanup:

    return dwError;
    
error:

    goto cleanup;
}

VOID
LWNetCfgResetToken(
    PLWNET_CFG_TOKEN pToken
    )
{
    if (pToken && pToken->pszToken && pToken->dwMaxLen)
    {
        memset(pToken->pszToken, 0, pToken->dwMaxLen);
        pToken->dwLen = 0;
    }
}

DWORD
LWNetCfgCopyToken(
    PLWNET_CFG_TOKEN pTokenSrc,
    PLWNET_CFG_TOKEN pTokenDst
    )
{
    DWORD dwError = 0;
    
    pTokenDst->tokenType = pTokenSrc->tokenType;
    
    if (pTokenSrc->dwLen > pTokenDst->dwLen) {
        dwError = LWNetReallocMemory(
                        (PVOID)  pTokenDst->pszToken,
                        (PVOID*) &pTokenDst->pszToken,
                        (DWORD)  pTokenSrc->dwLen);
        BAIL_ON_LWNET_ERROR(dwError);
        
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
LWNetCfgFreeTokenStack(
    PLWNET_STACK* ppTokenStack
    )
{
    
    DWORD dwError = 0;
    
    PLWNET_STACK pTokenStack = *ppTokenStack;
    
    dwError = LWNetStackForeach(
            pTokenStack,
            &LWNetCfgFreeTokenInStack,
            NULL);
    BAIL_ON_LWNET_ERROR(dwError);
    
    LWNetStackFree(pTokenStack);
    
    *ppTokenStack = NULL; 
    
error:

    return dwError;
}

DWORD
LWNetCfgFreeTokenInStack(
    PVOID pToken,
    PVOID pUserData
    )
{
    if (pToken)
    {
        LWNetCfgFreeToken((PLWNET_CFG_TOKEN)pToken);
    }
    
    return 0;
}

VOID
LWNetCfgFreeToken(
    PLWNET_CFG_TOKEN pToken
    )
{
    LWNET_SAFE_FREE_MEMORY(pToken->pszToken);
    LWNetFreeMemory(pToken);
}

DWORD
LWNetCfgGetNextToken(
    PLWNET_CONFIG_PARSE_STATE pParseState,
    PLWNET_CFG_TOKEN*         ppToken
    )
{
    DWORD dwError = 0;
    LWNetCfgTokenType tokenType = LWNetCfgNone;
    LWNetCfgLexState  curLexState = LWNetLexBegin;
    PLWNET_CFG_TOKEN  pToken = NULL;
    BOOLEAN         bOwnToken = FALSE;
    
    if (LWNetStackPeek(pParseState->pLexerTokenStack) != NULL) 
    {
        PLWNET_CFG_TOKEN pToken_input = *ppToken;
        
        pToken = (PLWNET_CFG_TOKEN)LWNetStackPop(&pParseState->pLexerTokenStack);
        bOwnToken = TRUE;
        
        if (pToken_input) {
            
            dwError = LWNetCfgCopyToken(pToken, pToken_input);
            BAIL_ON_LWNET_ERROR(dwError);
            
            LWNetCfgFreeToken(pToken);
            pToken = NULL;
            bOwnToken = FALSE;
            
        }
        
        goto done;
    }
    
    pToken = *ppToken;
    
    if (!pToken) {
        dwError = LWNetCfgAllocateToken(
                        0,
                        &pToken);
        BAIL_ON_LWNET_ERROR(dwError);
        
        bOwnToken = TRUE;
    }
    else
    {
        LWNetCfgResetToken(pToken);
    }

    while (curLexState != LWNetLexEnd)
    {
        DWORD ch = LWNetCfgGetCharacter(pParseState);
        LWNetCfgLexState lexClass = LWNetCfgGetLexClass(ch);

        if (lexClass != LWNetLexEOF) {
            pParseState->dwCol++;
        }

        if (lexClass == LWNetLexNewline) {
            pParseState->dwLine++;
            pParseState->dwCol = 0;
        }

        tokenType = LWNetCfgGetTokenType(curLexState, lexClass);

        switch(LWNetCfgGetLexAction(curLexState, lexClass))
        {
            case Skip:
                
                break;
                
            case Consume:

                if (pToken->dwLen >= pToken->dwMaxLen) {
                    dwError = LWNetCfgReallocToken(
                                    pToken,
                                    pToken->dwMaxLen + LWNET_CFG_TOKEN_DEFAULT_LENGTH);
                    BAIL_ON_LWNET_ERROR(dwError);
                }
                
                pToken->pszToken[pToken->dwLen++] = (BYTE)ch;

                break;
                
            case Pushback:

            if (lexClass == LWNetLexNewline)
            {
                    pParseState->dwLine--;
            }
                pParseState->dwCol--;
                dwError = LWNetCfgPushBackCharacter(
                                pParseState,
                                (BYTE)ch);
                BAIL_ON_LWNET_ERROR(dwError);
 
                break;
        }

        curLexState = LWNetCfgGetNextLexState(curLexState, lexClass);
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
        LWNetCfgFreeToken(pToken);
        *ppToken = NULL;
    }

    goto cleanup;
}

DWORD
LWNetCfgGetCharacter(
    PLWNET_CONFIG_PARSE_STATE pParseState
    )
{
    return getc(pParseState->fp);
}

LWNetCfgLexState
LWNetCfgGetLexClass(
    DWORD ch
    )
{
    
    if (ch == EOF) {
        return LWNetLexEOF;
    }
    
    if (ch == '\n') {
        return LWNetLexNewline;
    }

    if (ch == '[') {
        return LWNetLexLSqBrace;
    }

    if (ch == ']') {
        return LWNetLexRSqBrace;
    }

    if (ch == '=') {
        return LWNetLexEquals;
    }
    
    if (ch == '#') {
        return LWNetLexHash;
    }

    if (isalnum(ch) ||
        ispunct(ch) ||
        isblank(ch)) {
        return LWNetLexChar;
    }

    return LWNetLexOther;
}

DWORD
LWNetCfgPushBackCharacter(
    PLWNET_CONFIG_PARSE_STATE pParseState,
    BYTE ch
    )
{
    return ((EOF == ungetc(ch, pParseState->fp)) ? LwMapErrnoToLwError(errno) : 0);
}

LWNetCfgLexState
LWNetCfgGetNextLexState(
    LWNetCfgLexState currentState,
    DWORD chId
    )
{
    return (gLWNetLexStateTable[currentState][chId].nextState);
}

LWNetCfgLexAction
LWNetCfgGetLexAction(
    LWNetCfgLexState currentState,
    DWORD chId
    )
{
    return (gLWNetLexStateTable[currentState][chId].action);
}

LWNetCfgTokenType
LWNetCfgGetTokenType(
    LWNetCfgLexState currentState,
    DWORD chId
    )
{
    return (gLWNetLexStateTable[currentState][chId].tokenId);
}

