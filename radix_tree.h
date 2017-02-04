#ifndef __RADIX_TREE_H__
#define __RADIX_TREE_H__

#define NODE_KEY_SIZE 256

struct _node;

typedef struct _edge {
	char			chKey;
	struct _edge	*pSiblingEdge;
	struct _node	*pChildNode;
} edge;

typedef struct _node {
	char	czKey[NODE_KEY_SIZE];
	edge	*pEdges;
} node;

enum {
	RADIX_T_ER_NONE				= 0,
	RADIX_T_ER_ALLOC_MEM_FAIL	= -1000,
	RADIX_T_ER_KEY_TOO_LONG,
};
#endif /* __RADIX_TREE_H__ */
