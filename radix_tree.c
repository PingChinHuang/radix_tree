#include "radix_tree.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <error.h>

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
	n->bString = 0;
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
	int err, i = 0, rootk_len, k_len;
	char prefix[256];
	char remaining[256];
	node *root = *pRoot, *n;
	edge *e, *t, *p;

	/* Empty Tree */
	if (!root) {
		n = gen_node(key, &err);
		if (!n) return err;

		*pRoot = n;
		n->bString = 1;
		return RADIX_T_ER_NONE;	
	}

	/* Key existed in a node. */
	if (0 == strcmp(key, root->czKey)) {
		/* strlen(root->czKey) = strlen(key) */
		root->bString = 1;
		return RADIX_T_ER_NONE;
	}

	rootk_len = strlen(root->czKey);
	k_len = strlen(key);
	for (i = 0; i < rootk_len && i < k_len; i++) {
		if (root->czKey[i] == key[i])
			continue; /* Get the same prefix */
		else 
			break; /* Get a different character */
	}

	if (rootk_len < k_len) {
		if (rootk_len == i) {
			/* Check existing edge */
			t = root->pEdges;
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

			if (root->pEdges == NULL) {
				root->pEdges = e;
				return RADIX_T_ER_NONE;
			}

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
		} else {
			strcpy(prefix, "");
			strcpy(remaining, "");

			strncpy(prefix, root->czKey, i);
			prefix[i] = '\0';
			n = gen_node(prefix, &err);
			if (!n) return err;
			n->bString = 0;
			n->pEdges = NULL;

			*pRoot = n;
			e = gen_edge(root->czKey[i], &err);
			if (err) return err;
			strcpy(remaining, &root->czKey[i]);
			strcpy(root->czKey, remaining);
			e->pChildNode = root; /* Original root becomes the first child of new root. */
			n->pEdges = e;

			/* handle new string */
			e = gen_edge(key[i], &err);
			if (err) return err;
			err = insert(&e->pChildNode, &key[i]);
			if (err) {
				free(e);
				return err;
			}

			if (key[i] < root->czKey[i]) {
				e->pSiblingEdge = n->pEdges;
				n->pEdges = e;
			} else {
				n->pEdges->pSiblingEdge = e;
			}
		}
	} else if (rootk_len >= k_len) {
		strcpy(prefix, "");
		strcpy(remaining, "");

		strncpy(prefix, root->czKey, i);
		prefix[i] = '\0';
		n = gen_node(prefix, &err);
		if (!n) return err;
		n->pEdges = NULL;

		*pRoot = n;
		e = gen_edge(root->czKey[i], &err);
		if (err) return err;
		strcpy(remaining, &root->czKey[i]);
		strcpy(root->czKey, remaining);
		e->pChildNode = root; /* Original root becomes the first child of new root. */
		n->pEdges = e;

		if (k_len == i) {
			n->bString = 1;
		} else {
			n->bString = 0;

			/* handle new string */
			e = gen_edge(key[i], &err);
			if (err) return err;
			err = insert(&e->pChildNode, &key[i]);
			if (err) {
				free(e);
				return err;
			}

			if (key[i] < root->czKey[i]) {
				e->pSiblingEdge = n->pEdges;
				n->pEdges = e;
			} else {
				n->pEdges->pSiblingEdge = e;
			}
		}
	}

	return RADIX_T_ER_NONE;
}

static int delete(node *root, char *key)
{
	int rootk_len, k_len;

	if (NULL == root) return RADIX_T_ST_KEY_NONEXIST;

	rootk_len = strlen(root->czKey);
	k_len = strlen(key);

	if (rootk_len) {
		if (0 == strncmp(root->czKey, key, rootk_len)) {
			if (k_len == rootk_len) {
				root->bString = 0;
				if (NULL == root->pEdges) {
					/* Leaf node, notify parent to remove this edge and concat the string back to its key. */
					return RADIX_T_ST_KEY_REMOVED_LEAF;
				}

				return RADIX_T_ST_KEY_DELETED;
			} else { /* only the case which k_len > rootk_len */
				int ret, count;
				edge *t = root->pEdges, *p = root->pEdges;
				while (t) {
					if (t->chKey == key[rootk_len]) {
						ret = delete(t->pChildNode, &key[rootk_len]);
						if (RADIX_T_ST_KEY_REMOVED_LEAF == ret) {
							/* Remove this edge, and combine child node if this node only has one child left. */
							if (p == t) root->pEdges = t->pSiblingEdge; 
							else p->pSiblingEdge = t->pSiblingEdge;
							free_node(t->pChildNode);
							free(t);
						
							/* If this node represents a string, we don't combine it with its child node. */
							if (root->bString) return RADIX_T_ST_KEY_DELETED;
							if (NULL == root->pEdges) return RADIX_T_ST_KEY_REMOVED_LEAF;

							count = 0;
							t = root->pEdges;
							while (t) {
								count++;
								if (count > 1) return RADIX_T_ST_KEY_DELETED;
								t = t->pSiblingEdge;
							}

							/* Combine the child node. And set bString to 1. */
							t = root->pEdges;
							if (t->pChildNode->bString) root->bString = 1; /* If child node is a string, this node should becomes a string node after combinination. */
							strcat(root->czKey, t->pChildNode->czKey);
							root->pEdges = t->pChildNode->pEdges;
							free(t->pChildNode);
							free(t);

							if (!root->bString && root->pEdges == NULL)
								return RADIX_T_ST_KEY_REMOVED_LEAF;
							else
								return RADIX_T_ST_KEY_DELETED;
						} else if (RADIX_T_ST_KEY_DELETED == ret) {
							return ret;
						}
						break;
					}
					p = t;
					t = t->pSiblingEdge;
				}
			}
		}
	
		return RADIX_T_ST_KEY_NONEXIST;
	} else { /* Tree root has no key. */
		int ret, count;
		edge *t = root->pEdges, *p = root->pEdges;
		while (t) {
			if (t->chKey == key[0]) {
				ret = delete(t->pChildNode, key);
				if (RADIX_T_ST_KEY_REMOVED_LEAF == ret) {
					/* Remove this edge, and combine child node if only this node only has one child left. */
					if (p == t) root->pEdges = t->pSiblingEdge; 
					else p->pSiblingEdge = t->pSiblingEdge;
					free_node(t->pChildNode);
					free(t);

					/* If this node represents a string, we don't combine it with its child node. */
					if (root->bString) return RADIX_T_ST_KEY_DELETED;
					if (NULL == root->pEdges) return RADIX_T_ST_KEY_REMOVED_LEAF;
							
					count = 0;
					t = root->pEdges;
					while (t) {
						count++;
						if (count > 1) return RADIX_T_ST_KEY_DELETED;
						t = t->pSiblingEdge;
					}

					/* Combine the child node. */
					t = root->pEdges;
					if (t->pChildNode->bString) root->bString = 1;
					strcat(root->czKey, t->pChildNode->czKey);
					root->pEdges = t->pChildNode->pEdges;
					free_node(t->pChildNode);
					free(t);

					if (!root->bString && root->pEdges == NULL)
						return RADIX_T_ST_KEY_REMOVED_LEAF;
					else
						return RADIX_T_ST_KEY_DELETED;
				} else if (RADIX_T_ST_KEY_DELETED == ret) {
					return ret;
				}
				break;
			}
			p = t;
			t = t->pSiblingEdge;
		}
	}	

	return RADIX_T_ST_KEY_NONEXIST;
}

static int lookup(node *root, const char *key)
{
	if (root) {
		int i;
		edge *t = root->pEdges;

		/* Key is the same as the node's key [exist] */
		if (0 == strcmp(key, root->czKey) && root->bString)
			return RADIX_T_ST_KEY_EXIST;

		/* Key length is shorter than or equals to node's key */
		/* EQAUL: if this key exists it should be return in previous strcmp. */
		/* SHORTER: clearly it should not have to check the children node */
		if (strlen(key) <= strlen(root->czKey))
			return RADIX_T_ST_KEY_NONEXIST;

		for (i = 0; i < strlen(root->czKey) && i < strlen(key); i++) {
			if (root->czKey[i] == key[i])
				continue; /* Get the same prefix */
			else 
				break; /* Get a different character */
		}

		/* Check the edge list */
		while (t) {
			/* Have an edge matches for key. Check child node. */
			if (t->chKey == key[i]) {
				return lookup(t->pChildNode, &key[i]);
			}
			t = t->pSiblingEdge;
		}
	}
	return RADIX_T_ST_KEY_NONEXIST;
}

static void traverse(node *root, int prev_lv_len, int lv)
{
	node *n;
	if (NULL == root) return;
	n = root;
	if (n) {
		edge *e = n->pEdges;
		printf("[%s]%s \n", n->czKey, n->bString ? "(Str)" : "");
		while (e) {
			int i, cur_lv_len;
			for (i = 0; i < lv * 2 + prev_lv_len; i++) printf(" ");
			printf(" -(%c)--> ", e->chKey);
			cur_lv_len = strlen(n->czKey) + lv * 2 + 9 + prev_lv_len;
			traverse(e->pChildNode, cur_lv_len, lv + 1);
			e = e->pSiblingEdge;
		}
		printf("\n");
	}
}

static void insert_test(const char *file)
{
	FILE *fp;
	char line[256];

	if (NULL == file) return;

	fp = fopen(file, "r");
	if (NULL == fp) {
		perror("fopen: ");
		return;	 
	}

	while (fgets(line, sizeof(line), fp)) {
		int err, len;
		printf("Insert %s", line);
		len = strlen(line);
		line[len - 1] = '\0';
		err = insert(&g_pRoot, line);
		if (err) {
			fprintf(stderr, "Fail to insert %s into the tree", line);
		}
		traverse(g_pRoot, 0, 0);
	}

	fclose(fp);
	//traverse(g_pRoot, 0, 0);
}

static void lookup_test(const char *file)
{
	FILE *fp;
	char line[256];

	if (NULL == file) return;

	fp = fopen(file, "r");
	if (NULL == fp) {
		perror("fopen: ");
		return;	 
	}

	while (fgets(line, sizeof(line), fp)) {
		int status, len;
		len = strlen(line);
		line[len - 1] = '\0';
		printf("Look for \"%s\"", line);
		status = lookup(g_pRoot, line);
		if (RADIX_T_ST_KEY_EXIST == status)
			fprintf(stdout, "[FOUND]\n");
		else if (RADIX_T_ST_KEY_NONEXIST == status)
			fprintf(stdout, "[NOT FOUND]\n");
	}

	fclose(fp);
}

static void delete_test(const char *file)
{
	FILE *fp;
	char line[256];

	if (NULL == file) return;

	fp = fopen(file, "r");
	if (NULL == fp) {
		perror("fopen: ");
		return;	 
	}

	while (fgets(line, sizeof(line), fp)) {
		int status, len;
		len = strlen(line);
		line[len - 1] = '\0';
		printf("Delete \"%s\"\n", line);
		status = delete(g_pRoot, line);
		if (RADIX_T_ST_KEY_REMOVED_LEAF == status) {
			free_node(g_pRoot);
			g_pRoot = NULL;
		}
		traverse(g_pRoot, 0, 0);
	}

	fclose(fp);
}

int main(int argc, char *argv[])
{
	insert_test(argv[1]);
	delete_test(argv[1]);
	//lookup_test(argv[2]);

	//if (g_pRoot)
	//	free_node(g_pRoot);

	return 0;
}
