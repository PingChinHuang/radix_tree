#include "radix_tree.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static node *g_pRoot = NULL;

static node* gen_node(char *key, int *err)
{
	node *n = malloc(sizeof(node));

	if (strlen(key) >= NODE_KEY_SIZE) {
		fprintf(stderr, "Key is too long.\n");
		if (err) *err = RADIX_T_ER_KEY_TOO_LONG;
		return NULL;
	}

	if (NULL == n) {
		fprintf(stderr, "Fail to allocate memory.\n");
		if (err) *err = RADIX_T_ER_ALLOC_MEM_FAIL;
		return NULL;
	}

	strncpy(n->czKey, key, NODE_KEY_SIZE);
	n->pEdges = NULL;
	if (err) *err = RADIX_T_ER_NONE;
	return n;
}

static void free_node(node *n)
{
	if (!n) return;

	while (n->pEdges) {
		edge *e = n->pEdges;
		n->pEdges = n->pEdges->pSiblingEdge;
		if (e) {
			free_node(e->pChildNode);
			free(e);
		}
	}

	free(n);
	n = NULL;
}

static edge* gen_edge(char key, int *err)
{
	edge *e = malloc(sizeof(edge));

	if (NULL == e) {
		if (err) *err = RADIX_T_ER_ALLOC_MEM_FAIL;
		fprintf(stderr, "Fail to allocate memory.\n");
		return NULL;
	}

	e->pChildNode = NULL;
	e->pSiblingEdge = NULL;
	e->chKey = key;
	if (err) *err = RADIX_T_ER_NONE;
	return e;
}

static int insert(node **pRoot, char *key)
{
	int err, i = 0;
	node *root = *pRoot;

	/* Empty Tree */
	if (!root) {
		node *n = gen_node(key, &err);
		if (!n) return err;

		*pRoot = n;
		return RADIX_T_ER_NONE;	
	}

	/* Key existed in a node. */
	if (0 == strcmp(key, root->czKey)) {
		return RADIX_T_ER_NONE;
	}

	for (i = 0; i < strlen(root->czKey) && i < strlen(key); i++) {
		if (root->czKey[i] == key[i])
			continue; /* Get the same prefix */
		else 
			break; /* Get a different character */
	}

	/* Has Children */
	if (root->pEdges) {
		edge *e, *t = root->pEdges;
		while (t) {
			/* Have an edge matches for key. */
			if (t->chKey == key[i]) {
				return insert(&t->pChildNode, &key[i]);
			}
			t = t->pSiblingEdge;
		}

		/* No existing edge matches */
		e = gen_edge(key[i], &err);
		if (err) return err;
		err = insert(&e->pChildNode, &key[i]);
		if (err) {
			free(e);
			return err;
		}
	} else { /* Without Children */
		/* Create edges to keep remaining string */
		if (strlen(&root->czKey[i])) {
			edge *e, *t, *p;

			e = gen_edge(root->czKey[i], &err);
			if (err) return err;
			err = insert(&e->pChildNode, &root->czKey[i]);	
			if (err) {
				free(e);
				return err;
			}
		
			if (!root->pEdges) {
				root->pEdges = e;
			} else {
				p = root->pEdges; /* Previous Edge */
				t = root->pEdges; /* Current Edge */
				while (t) {
					/* Put into edge list by character order.*/
					if (e->chKey < t->chKey) {
						e->pSiblingEdge = t;
						if (p == t) root->pEdges = e; /* root->pEdges -> e -> t -> null */
						else p->pSiblingEdge = e; /* root->pEdges -> ... -> p -> e -> t -> .... */
						break;
					} else {
						if (!t->pSiblingEdge) {
							t->pSiblingEdge = e; /* root->pEdges -> .... -> t -> e -> null */
							break;
						}
						p = t;
						t = t->pSiblingEdge; 
					}
				}
			}	
		}


		if (strlen(&key[i])) {
			edge *e, *t, *p;

			e = gen_edge(key[i], &err);
			if (err) return err;
			err = insert(&e->pChildNode, &key[i]);
			if (err) {
				free(e);
				return err;
			}

			if (!root->pEdges) {
				root->pEdges = e;
			} else {
				p = root->pEdges;
				t = root->pEdges;
				while (t) { 
					if (e->chKey < t->chKey) {
						e->pSiblingEdge = t;
						if (p == t) root->pEdges = e;
						else p->pSiblingEdge = e;
						break;
					} else {
						if (!t->pSiblingEdge) {
							t->pSiblingEdge = e;
							break;
						}
						p = t;	
						t = t->pSiblingEdge;
					}
				}
			}	
		}
	
		/* Direcly let czKey terminate at i which is the differen character in two strings. */	
		root->czKey[i] = '\0';
	}

	return RADIX_T_ER_NONE;
}

static int delete(node *root, char *key)
{
	return RADIX_T_ER_NONE;
}

static int lookup(node *root, const char *key)
{
	return RADIX_T_ER_NONE;
}

static void traverse(node *root)
{
	node *n = root;
	if (n) {
		edge *e = n->pEdges;
		printf("[%s]", n->czKey);
		while (e) {
			printf("==%c==>", e->chKey);
			traverse(e->pChildNode);
			e = e->pSiblingEdge;
		}
		printf("\n");
	}
}

int main(int argc, char *argv[])
{
	printf ("%d\n", insert(&g_pRoot, argv[1]));
	printf ("%d\n", insert(&g_pRoot, argv[2]));
//printf ("%d\n", insert(&g_pRoot, argv[3]));
	//insert(g_pRoot, argv[3]);

	traverse(g_pRoot);

	free_node(g_pRoot);

	return 0;
}
