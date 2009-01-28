/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        rbtree.c
 *
 * Abstract:
 *
 *        Likewise Server Message Block (LSMB)
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

typedef enum
{
    RBTreeNodeColor_Red = 0,
    RBTreeNodeColor_Black
} RBTreeNodeColor;

typedef struct __SMB_RB_TREE_NODE
{

    RBTreeNodeColor color;

    struct __SMB_RB_TREE_NODE* pLeft;
    struct __SMB_RB_TREE_NODE* pRight;
    struct __SMB_RB_TREE_NODE* pParent;

    PVOID           pKey;
    PVOID           pData;

} SMB_RB_TREE_NODE, *PSMB_RB_TREE_NODE;

static SMB_RB_TREE_NODE gRBTreeSentinel = { RBTreeNodeColor_Black, NULL, NULL, NULL, NULL };
static PSMB_RB_TREE_NODE gpRBTreeSentinel = &gRBTreeSentinel;

#define SMB_RBTREE_IS_NIL_NODE(pNode) (pNode == gpRBTreeSentinel)
#define SMB_RBTREE_IS_RED(pNode)    (pNode && (pNode->color == RBTreeNodeColor_Red))
#define SMB_RBTREE_IS_BLACK(pNode)  (pNode && (pNode->color == RBTreeNodeColor_Black))

static
VOID
SMBRBTreeInsert(
    PSMB_RB_TREE      pRBTree,
    PSMB_RB_TREE_NODE pTreeNode
    );

static
NTSTATUS
SMBRBTreeTraversePreOrder(
    PSMB_RB_TREE_NODE     pNode,
    PFN_SMB_RB_TREE_VISIT pfnVisit,
    PVOID                 pUserData,
    PBOOLEAN              pbContinue
    );

static
NTSTATUS
SMBRBTreeTraverseInOrder(
    PSMB_RB_TREE_NODE     pNode,
    PFN_SMB_RB_TREE_VISIT pfnVisit,
    PVOID                 pUserData,
    PBOOLEAN              pbContinue
    );

static
NTSTATUS
SMBRBTreeTraversePostOrder(
    PSMB_RB_TREE_NODE     pNode,
    PFN_SMB_RB_TREE_VISIT pfnVisit,
    PVOID                 pUserData,
    PBOOLEAN              pbContinue
    );

static
VOID
SMBRBTreeRotateLeft(
    PSMB_RB_TREE      pRBTree,
    PSMB_RB_TREE_NODE pTreeNode
    );

static
VOID
SMBRBTreeRotateRight(
    PSMB_RB_TREE      pRBTree,
    PSMB_RB_TREE_NODE pTreeNode
    );

static
PSMB_RB_TREE_NODE
SMBRBTreeRemoveNode(
    PSMB_RB_TREE      pRBTree,
    PSMB_RB_TREE_NODE pTreeNode
    );

static
VOID
SMBRBTreeRemoveAllNodes(
    PSMB_RB_TREE      pRBTree,
    PSMB_RB_TREE_NODE pNode
    );

static
VOID
SMBRBTreeFixColors(
    PSMB_RB_TREE      pRBTree,
    PSMB_RB_TREE_NODE pTreeNode
    );

static
PSMB_RB_TREE_NODE
SMBRBTreeGetSuccessor(
    PSMB_RB_TREE      pRBTree,
    PSMB_RB_TREE_NODE pTreeNode
    );

static
PSMB_RB_TREE_NODE
SMBRBTreeGetMinimum(
    PSMB_RB_TREE      pRBTree,
    PSMB_RB_TREE_NODE pTreeNode
    );

static
PSMB_RB_TREE_NODE
SMBRBTreeFindNode(
    PSMB_RB_TREE pRBTree,
    PVOID   pData
    );

static
VOID
SMBRBTreeFreeNode(
    PSMB_RB_TREE      pRBTree,
    PSMB_RB_TREE_NODE pTreeNode
    );

// Rules
// (a) Nodes in the tree are red or black
// (b) Both children of a read node must be black
// (c) all leaf nodes must be black

NTSTATUS
SMBRBTreeCreate(
    PFN_SMB_RB_TREE_COMPARE   pfnRBTreeCompare,
    PFN_SMB_RB_TREE_FREE_KEY  pfnRBTreeFreeKey,
    PFN_SMB_RB_TREE_FREE_DATA pfnRBTreeFreeData,
    PSMB_RB_TREE* ppRBTree
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_RB_TREE pRBTree = NULL;

    if (!pfnRBTreeCompare)
    {
        ntStatus = STATUS_INVALID_PARAMETER_1;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SMBAllocateMemory(
                    sizeof(SMB_RB_TREE),
                    (PVOID*)&pRBTree);
    BAIL_ON_NT_STATUS(ntStatus);

    pRBTree->pfnCompare = pfnRBTreeCompare;
    pRBTree->pfnFreeKey    = pfnRBTreeFreeKey;
    pRBTree->pfnFreeData = pfnRBTreeFreeData;

    *ppRBTree = pRBTree;

cleanup:

    return ntStatus;

error:

    if (pRBTree) {
        SMBRBTreeFree(pRBTree);
    }

    *ppRBTree = NULL;

    goto cleanup;
}

NTSTATUS
SMBRBTreeFind(
    PSMB_RB_TREE pRBTree,
    PVOID        pKey,
    PVOID*       ppItem
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_RB_TREE_NODE pResult = NULL;

    pResult = SMBRBTreeFindNode(pRBTree, pKey);
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
PSMB_RB_TREE_NODE
SMBRBTreeFindNode(
    PSMB_RB_TREE pRBTree,
    PVOID   pKey
    )
{
    PVOID pResult = NULL;
    PSMB_RB_TREE_NODE pIter = (PSMB_RB_TREE_NODE)pRBTree->hRoot;

    while (pIter && !SMB_RBTREE_IS_NIL_NODE(pIter))
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
SMBRBTreeAdd(
    PSMB_RB_TREE pRBTree,
    PVOID   pKey,
    PVOID   pData
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_RB_TREE_NODE pTreeNode = NULL;
    BOOLEAN bFree = FALSE;

    if (!pKey)
    {
        ntStatus = STATUS_INVALID_PARAMETER_2;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SMBAllocateMemory(
                    sizeof(SMB_RB_TREE_NODE),
                    (PVOID*)&pTreeNode);
    BAIL_ON_NT_STATUS(ntStatus);

    pTreeNode->pKey = pKey;
    pTreeNode->pData = pData;
    pTreeNode->pRight = gpRBTreeSentinel;
    pTreeNode->pLeft = gpRBTreeSentinel;
    pTreeNode->pParent = gpRBTreeSentinel;

    bFree = TRUE;

    SMBRBTreeInsert(pRBTree, pTreeNode);

    pTreeNode->color = RBTreeNodeColor_Red;

    bFree = FALSE;

    while ((pTreeNode != (PSMB_RB_TREE_NODE)pRBTree->hRoot) &&
            SMB_RBTREE_IS_RED(pTreeNode->pParent))
    {
        PSMB_RB_TREE_NODE pNode = NULL;

        if (pTreeNode->pParent ==
            pTreeNode->pParent->pParent->pLeft)
        {
            pNode = pTreeNode->pParent->pParent->pRight;
            if (SMB_RBTREE_IS_RED(pNode))
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

                SMBRBTreeRotateLeft(pRBTree, pTreeNode);
            }
            else
            {
                pTreeNode->pParent->color = RBTreeNodeColor_Black;
                pTreeNode->pParent->pParent->color = RBTreeNodeColor_Red;

                SMBRBTreeRotateRight(pRBTree, pTreeNode->pParent->pParent);
            }
        }
        else
        {
            pNode = pTreeNode->pParent->pParent->pLeft;
            if (SMB_RBTREE_IS_RED(pNode))
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

                SMBRBTreeRotateRight(pRBTree, pTreeNode);
            }
            else
            {
                pTreeNode->pParent->color = RBTreeNodeColor_Black;
                pTreeNode->pParent->pParent->color = RBTreeNodeColor_Red;

                SMBRBTreeRotateLeft(pRBTree, pTreeNode->pParent->pParent);
            }
        }
    }

    ((PSMB_RB_TREE_NODE)pRBTree->hRoot)->color = RBTreeNodeColor_Black;

cleanup:

    return ntStatus;

error:

    if (bFree && pTreeNode) {
        SMBRBTreeFreeNode(pRBTree, pTreeNode);
    }

    goto cleanup;
}

static
VOID
SMBRBTreeInsert(
    PSMB_RB_TREE pRBTree,
    PSMB_RB_TREE_NODE pTreeNode
    )
{
    PSMB_RB_TREE_NODE pParent = gpRBTreeSentinel;
    PSMB_RB_TREE_NODE pCurrent = (PSMB_RB_TREE_NODE)pRBTree->hRoot;

    while (pCurrent && !SMB_RBTREE_IS_NIL_NODE(pCurrent))
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

    if (SMB_RBTREE_IS_NIL_NODE(pTreeNode->pParent))
    {
        pRBTree->hRoot = (HANDLE)pTreeNode;
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
SMBRBTreeRotateLeft(
    PSMB_RB_TREE pRBTree,
    PSMB_RB_TREE_NODE pTreeNode
    )
{
    PSMB_RB_TREE_NODE pNode = pTreeNode->pRight;

    pTreeNode->pRight = pNode->pLeft;

    if (!SMB_RBTREE_IS_NIL_NODE(pNode->pLeft))
    {
        pNode->pLeft->pParent = pTreeNode;
    }

    pNode->pParent = pTreeNode->pParent;

    if (SMB_RBTREE_IS_NIL_NODE(pTreeNode->pParent))
    {
        pRBTree->hRoot = (HANDLE)pNode;
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
SMBRBTreeRotateRight(
    PSMB_RB_TREE pRBTree,
    PSMB_RB_TREE_NODE pTreeNode
    )
{
    PSMB_RB_TREE_NODE pNode = pTreeNode->pLeft;

    pTreeNode->pLeft = pNode->pRight;

    if (!SMB_RBTREE_IS_NIL_NODE(pNode->pRight))
    {
        pNode->pRight->pParent = pTreeNode;
    }

    pNode->pParent = pTreeNode->pParent;

    if (SMB_RBTREE_IS_NIL_NODE(pTreeNode->pParent))
    {
        pRBTree->hRoot = (HANDLE)pNode;
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
SMBRBTreeTraverse(
    PSMB_RB_TREE pRBTree,
    SMB_TREE_TRAVERSAL_TYPE traversalType,
    PFN_SMB_RB_TREE_VISIT pfnVisit,
    PVOID pUserData
    )
{
    NTSTATUS   ntStatus = 0;
    BOOLEAN bContinue = TRUE;
    PSMB_RB_TREE_NODE pRootNode = (PSMB_RB_TREE_NODE)pRBTree->hRoot;

    if (!pRootNode)
    {
        goto cleanup;
    }

    switch (traversalType)
    {
        case SMB_TREE_TRAVERSAL_TYPE_PRE_ORDER:

            ntStatus = SMBRBTreeTraversePreOrder(
                            pRootNode,
                            pfnVisit,
                            pUserData,
                            &bContinue);

            break;

        case SMB_TREE_TRAVERSAL_TYPE_IN_ORDER:

            ntStatus = SMBRBTreeTraverseInOrder(
                            pRootNode,
                            pfnVisit,
                            pUserData,
                            &bContinue);

            break;

        case SMB_TREE_TRAVERSAL_TYPE_POST_ORDER:

            ntStatus = SMBRBTreeTraversePostOrder(
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
SMBRBTreeTraversePreOrder(
    PSMB_RB_TREE_NODE     pNode,
    PFN_SMB_RB_TREE_VISIT pfnVisit,
    PVOID                 pUserData,
    PBOOLEAN              pbContinue
    )
{
    NTSTATUS ntStatus = 0;

    if (pNode && !SMB_RBTREE_IS_NIL_NODE(pNode))
    {
        ntStatus = pfnVisit(pNode->pKey, pNode->pData, pUserData, pbContinue);
        BAIL_ON_NT_STATUS(ntStatus);

        if (*pbContinue && pNode->pLeft)
        {
            ntStatus = SMBRBTreeTraversePreOrder(
                            pNode->pLeft,
                            pfnVisit,
                            pUserData,
                            pbContinue);
            BAIL_ON_NT_STATUS(ntStatus);
        }

        if (*pbContinue && pNode->pRight)
        {
            ntStatus = SMBRBTreeTraversePreOrder(
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
SMBRBTreeTraverseInOrder(
    PSMB_RB_TREE_NODE     pNode,
    PFN_SMB_RB_TREE_VISIT pfnVisit,
    PVOID                 pUserData,
    PBOOLEAN              pbContinue
    )
{
    NTSTATUS ntStatus = 0;

    if (pNode && !SMB_RBTREE_IS_NIL_NODE(pNode))
    {
        if (*pbContinue && pNode->pLeft)
        {
            ntStatus = SMBRBTreeTraverseInOrder(
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
            ntStatus = SMBRBTreeTraverseInOrder(
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
SMBRBTreeTraversePostOrder(
    PSMB_RB_TREE_NODE     pNode,
    PFN_SMB_RB_TREE_VISIT pfnVisit,
    PVOID                 pUserData,
    PBOOLEAN              pbContinue
    )
{
    NTSTATUS ntStatus = 0;

    if (pNode && !SMB_RBTREE_IS_NIL_NODE(pNode))
    {
        if (*pbContinue && pNode->pLeft)
        {
            ntStatus = SMBRBTreeTraversePostOrder(
                            pNode->pLeft,
                            pfnVisit,
                            pUserData,
                            pbContinue);
            BAIL_ON_NT_STATUS(ntStatus);
        }

        if (*pbContinue && pNode->pRight)
        {
            ntStatus = SMBRBTreeTraversePostOrder(
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
SMBRBTreeRemove(
    PSMB_RB_TREE pRBTree,
    PVOID   pKey)
{
    NTSTATUS ntStatus = 0;
    PSMB_RB_TREE_NODE pNode = NULL;

    pNode = SMBRBTreeFindNode(pRBTree, pKey);

    if (!pNode)
    {
        ntStatus = STATUS_NOT_FOUND;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pNode = SMBRBTreeRemoveNode(pRBTree, pNode);

    SMBRBTreeFreeNode(pRBTree, pNode);

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

VOID
SMBRBTreeFree(
    PSMB_RB_TREE pRBTree
    )
{
    PSMB_RB_TREE_NODE pRootNode = (PSMB_RB_TREE_NODE)pRBTree->hRoot;

    if (pRootNode && !SMB_RBTREE_IS_NIL_NODE(pRootNode))
    {
        SMBRBTreeRemoveAllNodes(pRBTree, pRootNode);
    }

    SMBFreeMemory(pRBTree);
}

static
PSMB_RB_TREE_NODE
SMBRBTreeRemoveNode(
    PSMB_RB_TREE     pRBTree,
    PSMB_RB_TREE_NODE pTreeNode
    )
{
    PSMB_RB_TREE_NODE pSuccessor = NULL;
    PSMB_RB_TREE_NODE pTmp = NULL;

    if (SMB_RBTREE_IS_NIL_NODE(pTreeNode->pRight) ||
        SMB_RBTREE_IS_NIL_NODE(pTreeNode->pLeft))
    {
        pSuccessor = pTreeNode;
    }
    else
    {
        pSuccessor = SMBRBTreeGetSuccessor(pRBTree, pTreeNode);
    }

    if (!SMB_RBTREE_IS_NIL_NODE(pSuccessor->pLeft))
    {
        pTmp = pSuccessor->pLeft;
    }
    else
    {
        pTmp = pSuccessor->pRight;
    }

    pTmp->pParent = pSuccessor->pParent;

    if (SMB_RBTREE_IS_NIL_NODE(pSuccessor->pParent))
    {
        pRBTree->hRoot = (HANDLE)pTmp;
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
        pTreeNode->pData = pSuccessor->pData;
        pTreeNode->pKey = pSuccessor->pKey;
    }

    if (SMB_RBTREE_IS_BLACK(pSuccessor))
    {
        SMBRBTreeFixColors(pRBTree, pTmp);
    }

    return pSuccessor;
}

static
VOID
SMBRBTreeRemoveAllNodes(
    PSMB_RB_TREE     pRBTree,
    PSMB_RB_TREE_NODE pNode
    )
{
    if (!SMB_RBTREE_IS_NIL_NODE(pNode->pLeft))
    {
        SMBRBTreeRemoveAllNodes(pRBTree, pNode->pLeft);
    }

    if (!SMB_RBTREE_IS_NIL_NODE(pNode->pRight))
    {
        SMBRBTreeRemoveAllNodes(pRBTree, pNode->pRight);
    }

    SMBRBTreeFreeNode(pRBTree, pNode);
}

static
VOID
SMBRBTreeFixColors(
    PSMB_RB_TREE     pRBTree,
    PSMB_RB_TREE_NODE pTreeNode
    )
{
    while ((((PSMB_RB_TREE_NODE)(pRBTree->hRoot)) != pTreeNode) &&
            SMB_RBTREE_IS_BLACK(pTreeNode))
    {
        if (pTreeNode == pTreeNode->pParent->pLeft)
        {
            PSMB_RB_TREE_NODE pTmp = pTreeNode->pParent->pRight;

            if (SMB_RBTREE_IS_RED(pTmp))
            {
                pTmp->color = RBTreeNodeColor_Black;
                pTreeNode->pParent->color = RBTreeNodeColor_Red;

                SMBRBTreeRotateLeft(pRBTree, pTreeNode->pParent);

                pTmp = pTreeNode->pParent->pRight;
            }

            if (SMB_RBTREE_IS_BLACK(pTmp->pLeft) &&
                SMB_RBTREE_IS_BLACK(pTmp->pRight))
            {
                pTmp->color = RBTreeNodeColor_Red;
                pTreeNode = pTreeNode->pParent;
            }
            else if (SMB_RBTREE_IS_BLACK(pTmp->pRight))
            {
                pTmp->pLeft->color = RBTreeNodeColor_Black;
                pTmp->color = RBTreeNodeColor_Red;

                SMBRBTreeRotateRight(pRBTree, pTmp);
                pTmp = pTreeNode->pParent->pRight;
            }
            else
            {
                pTmp->color = pTreeNode->pParent->color;
                pTreeNode->pParent->color = RBTreeNodeColor_Black;
                pTmp->pRight->color = RBTreeNodeColor_Black;

                SMBRBTreeRotateLeft(pRBTree, pTreeNode->pParent);

                pTreeNode = (PSMB_RB_TREE_NODE)pRBTree->hRoot;
            }
        }
        else
        {
            PSMB_RB_TREE_NODE pTmp = pTreeNode->pParent->pLeft;

            if (SMB_RBTREE_IS_RED(pTmp))
            {
                pTmp->color = RBTreeNodeColor_Black;
                pTreeNode->pParent->color = RBTreeNodeColor_Red;

                SMBRBTreeRotateRight(pRBTree, pTreeNode->pParent);

                pTmp = pTreeNode->pParent->pLeft;
            }

            if (SMB_RBTREE_IS_BLACK(pTmp->pLeft) &&
                SMB_RBTREE_IS_BLACK(pTmp->pRight))
            {
                pTmp->color = RBTreeNodeColor_Red;
                pTreeNode = pTreeNode->pParent;
            }
            else if (SMB_RBTREE_IS_BLACK(pTmp->pLeft))
            {
                pTmp->pRight->color = RBTreeNodeColor_Black;
                pTmp->color = RBTreeNodeColor_Red;

                SMBRBTreeRotateLeft(pRBTree, pTmp);
                pTmp = pTreeNode->pParent->pLeft;
            }
            else
            {
                pTmp->color = pTreeNode->pParent->color;
                pTreeNode->pParent->color = RBTreeNodeColor_Black;
                pTmp->pLeft->color = RBTreeNodeColor_Black;

                SMBRBTreeRotateRight(pRBTree, pTreeNode->pParent);

                pTreeNode = (PSMB_RB_TREE_NODE)pRBTree->hRoot;
            }
        }
    }

    pTreeNode->color = RBTreeNodeColor_Black;
}

static
PSMB_RB_TREE_NODE
SMBRBTreeGetSuccessor(
    PSMB_RB_TREE pRBTree,
    PSMB_RB_TREE_NODE pTreeNode
    )
{
    PSMB_RB_TREE_NODE pResult = NULL;

    if (!SMB_RBTREE_IS_NIL_NODE(pTreeNode->pRight))
    {
        pResult = SMBRBTreeGetMinimum(pRBTree, pTreeNode->pRight);
    }
    else
    {
        pResult = pTreeNode->pParent;

        while (pResult && !SMB_RBTREE_IS_NIL_NODE(pResult) && (pTreeNode == pResult->pRight))
        {
            pTreeNode = pResult;
            pResult = pTreeNode->pParent;
        }
    }

    return pResult;
}

static
PSMB_RB_TREE_NODE
SMBRBTreeGetMinimum(
    PSMB_RB_TREE pRBTree,
    PSMB_RB_TREE_NODE pTreeNode
    )
{
    while (!SMB_RBTREE_IS_NIL_NODE(pTreeNode->pLeft))
    {
        pTreeNode = pTreeNode->pLeft;
    }

    return pTreeNode;
}

static
VOID
SMBRBTreeFreeNode(
    PSMB_RB_TREE     pRBTree,
    PSMB_RB_TREE_NODE pTreeNode
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

    SMBFreeMemory(pTreeNode);
}
