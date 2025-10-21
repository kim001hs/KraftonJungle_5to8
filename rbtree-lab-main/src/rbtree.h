#ifndef _RBTREE_H_
#define _RBTREE_H_

#include <stddef.h>

typedef enum { RBTREE_RED, RBTREE_BLACK } color_t;

typedef int key_t;

typedef struct node_t {
    color_t color;
    key_t key;
    struct node_t *parent, *left, *right;
} node_t;

typedef struct {
    node_t *root;
    node_t *nil;  // for sentinel
} rbtree;

rbtree *new_rbtree(void);
void delete_rbtree(rbtree *);

void delete_dfs(rbtree*, node_t* );//----------------------------------내가 추가함
void insert_dfs(rbtree* , node_t* , const key_t ,node_t* );//------------------
void insert_fixup(rbtree* ,node_t *);//----------------------------------
void left_rotate(rbtree* ,node_t *);
void right_rotate(rbtree* ,node_t *);
void erase_fixup(rbtree *t, node_t *);
node_t *find_dfs(const rbtree * ,node_t*, const key_t);//---------------------------------
void transplant(rbtree *, node_t *, node_t *);
void inorder_fill(node_t *, node_t *, key_t *, int *, const size_t );//------------------------------------
void print_tree(const rbtree *);
// void delete_node(node_t *node, node_t *nil);
// void left_rotate(rbtree *t, node_t *axis);
// void right_rotate(rbtree *t, node_t *axis);
// void color_flip(rbtree *t, node_t *node);
// void color_swap(rbtree *t, node_t * node);

node_t *rbtree_insert(rbtree *, const key_t);
node_t *rbtree_find(const rbtree *, const key_t);
node_t *rbtree_min(const rbtree *);
node_t *rbtree_max(const rbtree *);
int rbtree_erase(rbtree *, node_t *);

int rbtree_to_array(const rbtree *, key_t *, const size_t);

#endif  // _RBTREE_H_
