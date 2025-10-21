#ifndef _RBTREE_H_
#include "rbtree.h"
#endif

#include <stdio.h>
#include <string.h>

void print_tree(const rbtree* t) {
  print_tree_structure(t, t->root, "", 1, 0);
  printf("\n");
}

void print_tree_structure(const rbtree *t, node_t *p, char *prefix, int is_last, int black_depth) {
  /*         넓이 설정         */
  /* width = 3 -> key = 0~999 */
  int width = 3;

  if (p == NULL || p == t->nil) return;
  
  printf("%s", prefix);
  
  char color_char = p->color == RBTREE_BLACK ? 'B' : 'R';
  const char* color_start = (p->color == RBTREE_BLACK) ? "" : "\x1b[31m";
  const char* color_end = (p->color == RBTREE_BLACK) ? "" : "\x1b[0m";

  int has_left = (p->left != NULL && p->left != t->nil);
  int has_right = (p->right != NULL && p->right != t->nil);
  int is_leaf_path = (!has_left || !has_right);
  black_depth += p->color;

  if (strlen(prefix) == 0) {
    printf("key: %s%*d | color: %c | parent: %*d | left: %*d | right: %*d%s",
           color_start,
           width, p->key,
           color_char,
           width, (p->parent == NULL || p->parent == t->nil) ? 0 : p->parent->key,
           width, (p->left == NULL || p->left == t->nil) ? 0 : p->left->key,
           width, (p->right == NULL || p->right == t->nil) ? 0 : p->right->key,
           color_end);
    
    if (is_leaf_path) {
      printf(" | black depth: %3d", black_depth);
    }
    printf("\n");

  } else {
    printf("%s%skey: %*d | color: %c | parent: %*d | left: %*d | right: %*d",
           color_start,
           is_last ? "╚═ " : "╠═ ",
           width, p->key,
           color_char,
           width, (p->parent == NULL || p->parent == t->nil) ? 0 : p->parent->key,
           width, (p->left == NULL || p->left == t->nil) ? 0 : p->left->key,
           width, (p->right == NULL || p->right == t->nil) ? 0 : p->right->key);
    
    if (is_leaf_path) {
      printf(" | black depth: %3d", black_depth);
    }
    printf("%s\n", color_end);
  }
}
