/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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
 * license@likewise.com
*/

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        rbtree.c
 *
 * Abstract:
 *
 *        Likewise Base Library (LwBase)
 *
 *        Red Black Tree
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 * Based on "Introduction to Algorithms"
 * by Thomas H. Cormen, Charles E. Leiserson & Ronald L. Rivest
 *
 */
#include "includes.h"

#ifndef BAIL_ON_NT_STATUS
#define BAIL_ON_NT_STATUS(ntStatus) \
    if (ntStatus) goto error;
#endif

#define LWRTL_RBTREE_IS_NIL_NODE(pNode) (pNode == gpRBTreeSentinel)

#define LWRTL_RBTREE_IS_RED(pNode) \
    (pNode && (pNode->color == RBTreeNodeColor_Red))

#define LWRTL_RBTREE_IS_BLACK(pNode) \
    (pNode && (pNode->color == RBTreeNodeColor_Black))

typedef enum
{
    RBTreeNodeColor_Red = 0,
    RBTreeNodeColor_Black
} RBTreeNodeColor;

typedef struct __LWRTL_RB_TREE_NODE
{

    RBTreeNodeColor color;

    struct __LWRTL_RB_TREE_NODE* pLeft;
    struct __LWRTL_RB_TREE_NODE* pRight;
    struct __LWRTL_RB_TREE_NODE* pParent;

    PVOID           pKey;
    PVOID           pData;

} LWRTL_RB_TREE_NODE, *PLWRTL_RB_TREE_NODE;

typedef struct LWRTL_RB_TREE
{

    PFN_LWRTL_RB_TREE_COMPARE pfnCompare;
    PFN_LWRTL_RB_TREE_FREE_KEY  pfnFreeKey;
    PFN_LWRTL_RB_TREE_FREE_DATA pfnFreeData;

    PLWRTL_RB_TREE_NODE pRoot;

} LWRTL_RB_TREE;

static LWRTL_RB_TREE_NODE gRBTreeSentinel =
{
    RBTreeNodeColor_Black,
    NULL,
    NULL,
    NULL,
    NULL
};

static PLWRTL_RB_TREE_NODE gpRBTreeSentinel = &gRBTreeSentinel;

static
VOID
LwRtlRBTreeInsert(
    PLWRTL_RB_TREE      pRBTree,
    PLWRTL_RB_TREE_NODE pTreeNode
    );

static
NTSTATUS
LwRtlRBTreeTraversePreOrder(
    PLWRTL_RB_TREE_NODE     pNode,
    PFN_LWRTL_RB_TREE_VISIT pfnVisit,
    PVOID                 pUserData,
    PBOOLEAN              pbContinue
    );

static
NTSTATUS
LwRtlRBTreeTraverseInOrder(
    PLWRTL_RB_TREE_NODE     pNode,
    PFN_LWRTL_RB_TREE_VISIT pfnVisit,
    PVOID                 pUserData,
    PBOOLEAN              pbContinue
    );

static
NTSTATUS
LwRtlRBTreeTraversePostOrder(
    PLWRTL_RB_TREE_NODE     pNode,
    PFN_LWRTL_RB_TREE_VISIT pfnVisit,
    PVOID                 pUserData,
    PBOOLEAN              pbContinue
    );

static
VOID
LwRtlRBTreeRotateLeft(
    PLWRTL_RB_TREE      pRBTree,
    PLWRTL_RB_TREE_NODE pTreeNode
    );

static
VOID
LwRtlRBTreeRotateRight(
    PLWRTL_RB_TREE      pRBTree,
    PLWRTL_RB_TREE_NODE pTreeNode
    );

static
PLWRTL_RB_TREE_NODE
LwRtlRBTreeRemoveNode(
    PLWRTL_RB_TREE      pRBTree,
    PLWRTL_RB_TREE_NODE pTreeNode
    );

static
VOID
LwRtlRBTreeRemoveAllNodes(
    PLWRTL_RB_TREE      pRBTree,
    PLWRTL_RB_TREE_NODE pNode
    );

static
VOID
LwRtlRBTreeFixColors(
    PLWRTL_RB_TREE      pRBTree,
    PLWRTL_RB_TREE_NODE pTreeNode
    );

static
PLWRTL_RB_TREE_NODE
LwRtlRBTreeGetSuccessor(
    PLWRTL_RB_TREE      pRBTree,
    PLWRTL_RB_TREE_NODE pTreeNode
    );

static
PLWRTL_RB_TREE_NODE
LwRtlRBTreeGetMinimum(
    PLWRTL_RB_TREE      pRBTree,
    PLWRTL_RB_TREE_NODE pTreeNode
    );

static
PLWRTL_RB_TREE_NODE
LwRtlRBTreeFindNode(
    PLWRTL_RB_TREE pRBTree,
    PVOID   pData
    );

static
VOID
LwRtlRBTreeFreeNode(
    PLWRTL_RB_TREE      pRBTree,
    PLWRTL_RB_TREE_NODE pTreeNode
    );

// Rules
// (a) Nodes in the tree are red or black
// (b) Both children of a read node must be black
// (c) all leaf nodes must be black

NTSTATUS
LwRtlRBTreeCreate(
    PFN_LWRTL_RB_TREE_COMPARE   pfnRBTreeCompare,
    PFN_LWRTL_RB_TREE_FREE_KEY  pfnRBTreeFreeKey,
    PFN_LWRTL_RB_TREE_FREE_DATA pfnRBTreeFreeData,
    PLWRTL_RB_TREE* ppRBTree
    )
{
    NTSTATUS ntStatus = 0;
    PLWRTL_RB_TREE pRBTree = NULL;

    if (!pfnRBTreeCompare)
    {
        ntStatus = STATUS_INVALID_PARAMETER_1;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = LW_RTL_ALLOCATE(&pRBTree, LWRTL_RB_TREE, sizeof(LWRTL_RB_TREE));
    BAIL_ON_NT_STATUS(ntStatus);

    pRBTree->pfnCompare = pfnRBTreeCompare;
    pRBTree->pfnFreeKey    = pfnRBTreeFreeKey;
    pRBTree->pfnFreeData = pfnRBTreeFreeData;

    *ppRBTree = pRBTree;

cleanup:

    return ntStatus;

error:

    if (pRBTree) {
        LwRtlRBTreeFree(pRBTree);
    }

    *ppRBTree = NULL;

    goto cleanup;
}

NTSTATUS
LwRtlRBTreeFind(
    PLWRTL_RB_TREE pRBTree,
    PVOID        pKey,
    PVOID*       ppItem
    )
{
    NTSTATUS ntStatus = 0;
    PLWRTL_RB_TREE_NODE pResult = NULL;

    pResult = LwRtlRBTreeFindNode(pRBTree, pKey);
    if (!pResult)
    {
        ntStatus = STATUS_NOT_FOUND;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppItem = pResult->pData;

cleanup:

    return ntStatus;

error:

    *ppItem = NULL;

    goto cleanup;
}

static
PLWRTL_RB_TREE_NODE
LwRtlRBTreeFindNode(
    PLWRTL_RB_TREE pRBTree,
    PVOID   pKey
    )
{
    PVOID pResult = NULL;
    PLWRTL_RB_TREE_NODE pIter = NULL;

    if (pRBTree == NULL) {
	    return NULL;
    }

    pIter = pRBTree->pRoot;

    while (pIter && !LWRTL_RBTREE_IS_NIL_NODE(pIter))
    {
        int compResult = pRBTree->pfnCompare(pKey, pIter->pKey);

        if (!compResult)
        {
            pResult = pIter;
            break;
        }
        else if (compResult < 0)
        {
            pIter = pIter->pLeft;
        }
        else
        {
            pIter = pIter->pRight;
        }
    }

    return pResult;
}

NTSTATUS
LwRtlRBTreeAdd(
    PLWRTL_RB_TREE pRBTree,
    PVOID   pKey,
    PVOID   pData
    )
{
    NTSTATUS ntStatus = 0;
    PLWRTL_RB_TREE_NODE pTreeNode = NULL;
    BOOLEAN bFree = FALSE;

    if (!pKey)
    {
        ntStatus = STATUS_INVALID_PARAMETER_2;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = LW_RTL_ALLOCATE(
                    &pTreeNode,
                    LWRTL_RB_TREE_NODE,
                    sizeof(LWRTL_RB_TREE_NODE));
    BAIL_ON_NT_STATUS(ntStatus);

    pTreeNode->pKey = pKey;
    pTreeNode->pData = pData;
    pTreeNode->pRight = gpRBTreeSentinel;
    pTreeNode->pLeft = gpRBTreeSentinel;
    pTreeNode->pParent = gpRBTreeSentinel;

    bFree = TRUE;

    LwRtlRBTreeInsert(pRBTree, pTreeNode);

    pTreeNode->color = RBTreeNodeColor_Red;

    bFree = FALSE;

    while ((pTreeNode != pRBTree->pRoot) &&
            LWRTL_RBTREE_IS_RED(pTreeNode->pParent))
    {
        PLWRTL_RB_TREE_NODE pNode = NULL;

        if (pTreeNode->pParent ==
            pTreeNode->pParent->pParent->pLeft)
        {
            pNode = pTreeNode->pParent->pParent->pRight;
            if (LWRTL_RBTREE_IS_RED(pNode))
            {
                pTreeNode->pParent->color = RBTreeNodeColor_Black;
                pNode->color = RBTreeNodeColor_Black;

                pTreeNode = pTreeNode->pParent->pParent;

                if (pTreeNode) {
                    pTreeNode->color = RBTreeNodeColor_Red;
                }
            }
            else if (pTreeNode == pTreeNode->pParent->pRight)
            {
                pTreeNode = pTreeNode->pParent;

                LwRtlRBTreeRotateLeft(pRBTree, pTreeNode);
            }
            else
            {
                pTreeNode->pParent->color = RBTreeNodeColor_Black;
                pTreeNode->pParent->pParent->color = RBTreeNodeColor_Red;

                LwRtlRBTreeRotateRight(pRBTree, pTreeNode->pParent->pParent);
            }
        }
        else
        {
            pNode = pTreeNode->pParent->pParent->pLeft;
            if (LWRTL_RBTREE_IS_RED(pNode))
            {
                pTreeNode->pParent->color = RBTreeNodeColor_Black;
                pNode->color = RBTreeNodeColor_Black;

                pTreeNode = pTreeNode->pParent->pParent;

                if (pTreeNode) {
                    pTreeNode->color = RBTreeNodeColor_Red;
                }
            }
            else if (pTreeNode == pTreeNode->pParent->pLeft)
            {
                pTreeNode = pTreeNode->pParent;

                LwRtlRBTreeRotateRight(pRBTree, pTreeNode);
            }
            else
            {
                pTreeNode->pParent->color = RBTreeNodeColor_Black;
                pTreeNode->pParent->pParent->color = RBTreeNodeColor_Red;

                LwRtlRBTreeRotateLeft(pRBTree, pTreeNode->pParent->pParent);
            }
        }
    }

    pRBTree->pRoot->color = RBTreeNodeColor_Black;

cleanup:

    return ntStatus;

error:

    if (bFree && pTreeNode) {
        LwRtlRBTreeFreeNode(pRBTree, pTreeNode);
    }

    goto cleanup;
}

static
VOID
LwRtlRBTreeInsert(
    PLWRTL_RB_TREE pRBTree,
    PLWRTL_RB_TREE_NODE pTreeNode
    )
{
    PLWRTL_RB_TREE_NODE pParent = gpRBTreeSentinel;
    PLWRTL_RB_TREE_NODE pCurrent = pRBTree->pRoot;

    while (pCurrent && !LWRTL_RBTREE_IS_NIL_NODE(pCurrent))
    {
        pParent = pCurrent;

        if (pRBTree->pfnCompare(pTreeNode->pKey, pCurrent->pKey) < 0)
        {
            pCurrent = pCurrent->pLeft;
        }
        else
        {
            pCurrent = pCurrent->pRight;
        }
    }

    pTreeNode->pParent = pParent;

    if (LWRTL_RBTREE_IS_NIL_NODE(pTreeNode->pParent))
    {
        pRBTree->pRoot = pTreeNode;
    }
    else if (pRBTree->pfnCompare(pTreeNode->pKey, pParent->pKey) < 0)
    {
        pParent->pLeft = pTreeNode;
    }
    else
    {
        pParent->pRight = pTreeNode;
    }
}

static
VOID
LwRtlRBTreeRotateLeft(
    PLWRTL_RB_TREE pRBTree,
    PLWRTL_RB_TREE_NODE pTreeNode
    )
{
    PLWRTL_RB_TREE_NODE pNode = pTreeNode->pRight;

    pTreeNode->pRight = pNode->pLeft;

    if (!LWRTL_RBTREE_IS_NIL_NODE(pNode->pLeft))
    {
        pNode->pLeft->pParent = pTreeNode;
    }

    pNode->pParent = pTreeNode->pParent;

    if (LWRTL_RBTREE_IS_NIL_NODE(pTreeNode->pParent))
    {
        pRBTree->pRoot = pNode;
    }
    else if (pTreeNode == pTreeNode->pParent->pLeft)
    {
        pTreeNode->pParent->pLeft = pNode;
    }
    else
    {
        pTreeNode->pParent->pRight = pNode;
    }

    pNode->pLeft = pTreeNode;
    pTreeNode->pParent = pNode;
}

static
VOID
LwRtlRBTreeRotateRight(
    PLWRTL_RB_TREE pRBTree,
    PLWRTL_RB_TREE_NODE pTreeNode
    )
{
    PLWRTL_RB_TREE_NODE pNode = pTreeNode->pLeft;

    pTreeNode->pLeft = pNode->pRight;

    if (!LWRTL_RBTREE_IS_NIL_NODE(pNode->pRight))
    {
        pNode->pRight->pParent = pTreeNode;
    }

    pNode->pParent = pTreeNode->pParent;

    if (LWRTL_RBTREE_IS_NIL_NODE(pTreeNode->pParent))
    {
        pRBTree->pRoot = pNode;
    }
    else if (pTreeNode == pTreeNode->pParent->pRight)
    {
        pTreeNode->pParent->pRight = pNode;
    }
    else
    {
        pTreeNode->pParent->pLeft = pNode;
    }

    pNode->pRight = pTreeNode;
    pTreeNode->pParent = pNode;
}

NTSTATUS
LwRtlRBTreeTraverse(
    PLWRTL_RB_TREE pRBTree,
    LWRTL_TREE_TRAVERSAL_TYPE traversalType,
    PFN_LWRTL_RB_TREE_VISIT pfnVisit,
    PVOID pUserData
    )
{
    NTSTATUS   ntStatus = 0;
    BOOLEAN bContinue = TRUE;
    PLWRTL_RB_TREE_NODE pRootNode = pRBTree->pRoot;

    if (!pRootNode)
    {
        goto cleanup;
    }

    switch (traversalType)
    {
        case LWRTL_TREE_TRAVERSAL_TYPE_PRE_ORDER:

            ntStatus = LwRtlRBTreeTraversePreOrder(
                            pRootNode,
                            pfnVisit,
                            pUserData,
                            &bContinue);

            break;

        case LWRTL_TREE_TRAVERSAL_TYPE_IN_ORDER:

            ntStatus = LwRtlRBTreeTraverseInOrder(
                            pRootNode,
                            pfnVisit,
                            pUserData,
                            &bContinue);

            break;

        case LWRTL_TREE_TRAVERSAL_TYPE_POST_ORDER:

            ntStatus = LwRtlRBTreeTraversePostOrder(
                            pRootNode,
                            pfnVisit,
                            pUserData,
                            &bContinue);

            break;
    }

cleanup:

    return ntStatus;
}

static
NTSTATUS
LwRtlRBTreeTraversePreOrder(
    PLWRTL_RB_TREE_NODE     pNode,
    PFN_LWRTL_RB_TREE_VISIT pfnVisit,
    PVOID                 pUserData,
    PBOOLEAN              pbContinue
    )
{
    NTSTATUS ntStatus = 0;

    if (pNode && !LWRTL_RBTREE_IS_NIL_NODE(pNode))
    {
        ntStatus = pfnVisit(pNode->pKey, pNode->pData, pUserData, pbContinue);
        BAIL_ON_NT_STATUS(ntStatus);

        if (*pbContinue && pNode->pLeft)
        {
            ntStatus = LwRtlRBTreeTraversePreOrder(
                            pNode->pLeft,
                            pfnVisit,
                            pUserData,
                            pbContinue);
            BAIL_ON_NT_STATUS(ntStatus);
        }

        if (*pbContinue && pNode->pRight)
        {
            ntStatus = LwRtlRBTreeTraversePreOrder(
                            pNode->pRight,
                            pfnVisit,
                            pUserData,
                            pbContinue);
            BAIL_ON_NT_STATUS(ntStatus);
        }
    }

error:

    return ntStatus;
}

static
NTSTATUS
LwRtlRBTreeTraverseInOrder(
    PLWRTL_RB_TREE_NODE     pNode,
    PFN_LWRTL_RB_TREE_VISIT pfnVisit,
    PVOID                 pUserData,
    PBOOLEAN              pbContinue
    )
{
    NTSTATUS ntStatus = 0;

    if (pNode && !LWRTL_RBTREE_IS_NIL_NODE(pNode))
    {
        if (*pbContinue && pNode->pLeft)
        {
            ntStatus = LwRtlRBTreeTraverseInOrder(
                            pNode->pLeft,
                            pfnVisit,
                            pUserData,
                            pbContinue);
            BAIL_ON_NT_STATUS(ntStatus);
        }

        ntStatus = pfnVisit(pNode->pKey, pNode->pData, pUserData, pbContinue);
        BAIL_ON_NT_STATUS(ntStatus);

        if (*pbContinue && pNode->pRight)
        {
            ntStatus = LwRtlRBTreeTraverseInOrder(
                            pNode->pRight,
                            pfnVisit,
                            pUserData,
                            pbContinue);
            BAIL_ON_NT_STATUS(ntStatus);
        }
    }

error:

    return ntStatus;
}

static
NTSTATUS
LwRtlRBTreeTraversePostOrder(
    PLWRTL_RB_TREE_NODE     pNode,
    PFN_LWRTL_RB_TREE_VISIT pfnVisit,
    PVOID                 pUserData,
    PBOOLEAN              pbContinue
    )
{
    NTSTATUS ntStatus = 0;

    if (pNode && !LWRTL_RBTREE_IS_NIL_NODE(pNode))
    {
        if (*pbContinue && pNode->pLeft)
        {
            ntStatus = LwRtlRBTreeTraversePostOrder(
                            pNode->pLeft,
                            pfnVisit,
                            pUserData,
                            pbContinue);
            BAIL_ON_NT_STATUS(ntStatus);
        }

        if (*pbContinue && pNode->pRight)
        {
            ntStatus = LwRtlRBTreeTraversePostOrder(
                            pNode->pRight,
                            pfnVisit,
                            pUserData,
                            pbContinue);
            BAIL_ON_NT_STATUS(ntStatus);
        }

        ntStatus = pfnVisit(pNode->pKey, pNode->pData, pUserData, pbContinue);
        BAIL_ON_NT_STATUS(ntStatus);
    }

error:

    return ntStatus;
}

NTSTATUS
LwRtlRBTreeRemove(
    PLWRTL_RB_TREE pRBTree,
    PVOID   pKey)
{
    NTSTATUS ntStatus = 0;
    PLWRTL_RB_TREE_NODE pNode = NULL;

    pNode = LwRtlRBTreeFindNode(pRBTree, pKey);

    if (!pNode)
    {
        ntStatus = STATUS_NOT_FOUND;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pNode = LwRtlRBTreeRemoveNode(pRBTree, pNode);

    LwRtlRBTreeFreeNode(pRBTree, pNode);

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

VOID
LwRtlRBTreeFree(
    PLWRTL_RB_TREE pRBTree
    )
{
    PLWRTL_RB_TREE_NODE pRootNode = pRBTree->pRoot;

    if (pRootNode && !LWRTL_RBTREE_IS_NIL_NODE(pRootNode))
    {
        LwRtlRBTreeRemoveAllNodes(pRBTree, pRootNode);
    }

    LwRtlMemoryFree(pRBTree);
}

static
PLWRTL_RB_TREE_NODE
LwRtlRBTreeRemoveNode(
    PLWRTL_RB_TREE     pRBTree,
    PLWRTL_RB_TREE_NODE pTreeNode
    )
{
    PLWRTL_RB_TREE_NODE pSuccessor = NULL;
    PLWRTL_RB_TREE_NODE pTmp = NULL;

    if (LWRTL_RBTREE_IS_NIL_NODE(pTreeNode->pRight) ||
        LWRTL_RBTREE_IS_NIL_NODE(pTreeNode->pLeft))
    {
        pSuccessor = pTreeNode;
    }
    else
    {
        pSuccessor = LwRtlRBTreeGetSuccessor(pRBTree, pTreeNode);
    }

    if (!LWRTL_RBTREE_IS_NIL_NODE(pSuccessor->pLeft))
    {
        pTmp = pSuccessor->pLeft;
    }
    else
    {
        pTmp = pSuccessor->pRight;
    }

    pTmp->pParent = pSuccessor->pParent;

    if (LWRTL_RBTREE_IS_NIL_NODE(pSuccessor->pParent))
    {
        pRBTree->pRoot = pTmp;
    }
    else if (pSuccessor == pSuccessor->pParent->pLeft)
    {
        pSuccessor->pParent->pLeft = pTmp;
    }
    else
    {
        pSuccessor->pParent->pRight = pTmp;
    }

    if (pSuccessor != pTreeNode)
    {
        PVOID pTmpKey = pTreeNode->pKey;
        PVOID pTmpData = pTreeNode->pData;

        pTreeNode->pKey = pSuccessor->pKey;
        pSuccessor->pKey = pTmpKey;

        pTreeNode->pData = pSuccessor->pData;
        pSuccessor->pData = pTmpData;
    }

    if (LWRTL_RBTREE_IS_BLACK(pSuccessor))
    {
        LwRtlRBTreeFixColors(pRBTree, pTmp);
    }

    return pSuccessor;
}

static
VOID
LwRtlRBTreeRemoveAllNodes(
    PLWRTL_RB_TREE     pRBTree,
    PLWRTL_RB_TREE_NODE pNode
    )
{
    if (!LWRTL_RBTREE_IS_NIL_NODE(pNode->pLeft))
    {
        LwRtlRBTreeRemoveAllNodes(pRBTree, pNode->pLeft);
    }

    if (!LWRTL_RBTREE_IS_NIL_NODE(pNode->pRight))
    {
        LwRtlRBTreeRemoveAllNodes(pRBTree, pNode->pRight);
    }

    LwRtlRBTreeFreeNode(pRBTree, pNode);
}

static
VOID
LwRtlRBTreeFixColors(
    PLWRTL_RB_TREE     pRBTree,
    PLWRTL_RB_TREE_NODE pTreeNode
    )
{
    while ((pRBTree->pRoot != pTreeNode) &&
            LWRTL_RBTREE_IS_BLACK(pTreeNode))
    {
        if (pTreeNode == pTreeNode->pParent->pLeft)
        {
            PLWRTL_RB_TREE_NODE pTmp = pTreeNode->pParent->pRight;

            if (LWRTL_RBTREE_IS_RED(pTmp))
            {
                pTmp->color = RBTreeNodeColor_Black;
                pTreeNode->pParent->color = RBTreeNodeColor_Red;

                LwRtlRBTreeRotateLeft(pRBTree, pTreeNode->pParent);

                pTmp = pTreeNode->pParent->pRight;
            }

            if (LWRTL_RBTREE_IS_BLACK(pTmp->pLeft) &&
                LWRTL_RBTREE_IS_BLACK(pTmp->pRight))
            {
                pTmp->color = RBTreeNodeColor_Red;
                pTreeNode = pTreeNode->pParent;
            }
            else if (LWRTL_RBTREE_IS_BLACK(pTmp->pRight))
            {
                pTmp->pLeft->color = RBTreeNodeColor_Black;
                pTmp->color = RBTreeNodeColor_Red;

                LwRtlRBTreeRotateRight(pRBTree, pTmp);
                pTmp = pTreeNode->pParent->pRight;
            }
            else
            {
                pTmp->color = pTreeNode->pParent->color;
                pTreeNode->pParent->color = RBTreeNodeColor_Black;
                pTmp->pRight->color = RBTreeNodeColor_Black;

                LwRtlRBTreeRotateLeft(pRBTree, pTreeNode->pParent);

                pTreeNode = pRBTree->pRoot;
            }
        }
        else
        {
            PLWRTL_RB_TREE_NODE pTmp = pTreeNode->pParent->pLeft;

            if (LWRTL_RBTREE_IS_RED(pTmp))
            {
                pTmp->color = RBTreeNodeColor_Black;
                pTreeNode->pParent->color = RBTreeNodeColor_Red;

                LwRtlRBTreeRotateRight(pRBTree, pTreeNode->pParent);

                pTmp = pTreeNode->pParent->pLeft;
            }

            if (LWRTL_RBTREE_IS_BLACK(pTmp->pLeft) &&
                LWRTL_RBTREE_IS_BLACK(pTmp->pRight))
            {
                pTmp->color = RBTreeNodeColor_Red;
                pTreeNode = pTreeNode->pParent;
            }
            else if (LWRTL_RBTREE_IS_BLACK(pTmp->pLeft))
            {
                pTmp->pRight->color = RBTreeNodeColor_Black;
                pTmp->color = RBTreeNodeColor_Red;

                LwRtlRBTreeRotateLeft(pRBTree, pTmp);
                pTmp = pTreeNode->pParent->pLeft;
            }
            else
            {
                pTmp->color = pTreeNode->pParent->color;
                pTreeNode->pParent->color = RBTreeNodeColor_Black;
                pTmp->pLeft->color = RBTreeNodeColor_Black;

                LwRtlRBTreeRotateRight(pRBTree, pTreeNode->pParent);

                pTreeNode = pRBTree->pRoot;
            }
        }
    }

    pTreeNode->color = RBTreeNodeColor_Black;
}

static
PLWRTL_RB_TREE_NODE
LwRtlRBTreeGetSuccessor(
    PLWRTL_RB_TREE pRBTree,
    PLWRTL_RB_TREE_NODE pTreeNode
    )
{
    PLWRTL_RB_TREE_NODE pResult = NULL;

    if (!LWRTL_RBTREE_IS_NIL_NODE(pTreeNode->pRight))
    {
        pResult = LwRtlRBTreeGetMinimum(pRBTree, pTreeNode->pRight);
    }
    else
    {
        pResult = pTreeNode->pParent;

        while (pResult && !LWRTL_RBTREE_IS_NIL_NODE(pResult) && (pTreeNode == pResult->pRight))
        {
            pTreeNode = pResult;
            pResult = pTreeNode->pParent;
        }
    }

    return pResult;
}

static
PLWRTL_RB_TREE_NODE
LwRtlRBTreeGetMinimum(
    PLWRTL_RB_TREE pRBTree,
    PLWRTL_RB_TREE_NODE pTreeNode
    )
{
    while (!LWRTL_RBTREE_IS_NIL_NODE(pTreeNode->pLeft))
    {
        pTreeNode = pTreeNode->pLeft;
    }

    return pTreeNode;
}

static
VOID
LwRtlRBTreeFreeNode(
    PLWRTL_RB_TREE      pRBTree,
    PLWRTL_RB_TREE_NODE pTreeNode
    )
{
    if (pTreeNode->pKey && pRBTree->pfnFreeKey)
    {
        pRBTree->pfnFreeKey(pTreeNode->pKey);
    }

    if (pTreeNode->pData && pRBTree->pfnFreeData)
    {
        pRBTree->pfnFreeData(pTreeNode->pData);
    }

    LwRtlMemoryFree(pTreeNode);
}
