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
	unsigned char	bString;
	char			czKey[NODE_KEY_SIZE];
	edge			*pEdges;
} node;

enum {
	RADIX_T_ST_KEY_NONEXIST		= 0,
	RADIX_T_ST_KEY_EXIST,
	RADIX_T_ST_KEY_REMOVED_LEAF,
	RADIX_T_ST_KEY_DELETED	
};

enum {
	RADIX_T_ER_NONE				= 0,
	RADIX_T_ER_ALLOC_MEM_FAIL	= -1000,
	RADIX_T_ER_KEY_TOO_LONG,
};
#endif /* __RADIX_TREE_H__ */
