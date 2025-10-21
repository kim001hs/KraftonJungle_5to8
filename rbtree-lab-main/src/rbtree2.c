#include "rbtree.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Help functions
void delete_subtree(node_t *, node_t *);
void rotate_L(rbtree *, node_t *);
void rotate_R(rbtree *, node_t *);
int erase_node(rbtree *, node_t *, node_t *, node_t *);
void print_tree(const rbtree*);
void print_tree_structure(const rbtree *, node_t *, char *, int);

// Use Sentinel
rbtree *new_rbtree(void) {
  rbtree *p = (rbtree *)calloc(1, sizeof(rbtree));
  p->nil = (node_t *)malloc(sizeof(node_t));
  p->nil->color = RBTREE_BLACK;
  p->nil->key = 0;
  p->nil->parent = p->nil->left = p->nil->right = p->nil;

  p->root = p->nil;
  return p;
}

void delete_subtree(node_t *p, node_t *nil) {
  if (p->left != nil) delete_subtree(p->left, nil);
  if (p->right != nil) delete_subtree(p->right, nil);
  free(p);
  return;
}

void delete_rbtree(rbtree *t) {
  if (t->root != t->nil) delete_subtree(t->root, t->nil);
  free(t->nil);
  free(t);
}

void rotate_L(rbtree *t, node_t *node) {
  node_t *right = node->right;
  // Validation
  if (t == NULL) {
    printf("tree is NULL (rotate_L)\n");
    return;
  }

  if (node == NULL) {
    printf("node is NULL (rotate_L)\n");
    return;
  }

  if (right == t->nil) {
    printf("right is nil (rotate_L)\n");
    return;
  }

  // Connect node->parent <--> right 
  if (node == t->root) {
    t->root = right;
  } else {
    if (node->parent->left == node) node->parent->left = right;
    else node->parent->right = right;
  }
  right->parent = node->parent;

  // Connect node->right <--> right->left
  if (right->left != t->nil) right->left->parent = node;
  node->right = right->left;

  // Adjust node <--> right
  node->parent = right;
  right->left = node;
}

void rotate_R(rbtree *t, node_t *node) {
  node_t *left = node->left;
  // Validation
  if (t == NULL) {
    printf("tree is NULL (rotate_R)\n");
    return;
  }

  if (node == NULL) {
    printf("node is NULL (rotate_R)\n");
    return;
  }

  if (left == t->nil) {
    printf("left is nil (rotate_R)\n");
    return;
  }

  // Connect node->parent <--> left 
  if (node == t->root) {
    t->root = left;
  } else {
    if (node->parent->left == node) node->parent->left = left;
    else node->parent->right = left;
  }
  left->parent = node->parent;

  // Connect node->left <--> left->right
  if (left->right != t->nil) left->right->parent = node;
  node->left = left->right;

  // Adjust node <--> left
  node->parent = left;
  left->right = node;
}

node_t *rbtree_insert(rbtree *t, const key_t key) {
  node_t *cur;
  node_t *new_node = malloc(sizeof(node_t));
  node_t *child, *parent, *grand, *uncle;
  
  if (t == NULL) {
    printf("tree is NULL (rbtree_insert)\n");
    return NULL;
  }

  if (new_node == NULL) {
    printf("malloc failed (rbtree_insert)\n");
    return t->root;
  }

  new_node->color = RBTREE_RED;
  new_node->key = key;
  new_node->parent = t->nil;
  new_node->left = new_node->right = t->nil;

  cur = t->root;
  // Case: Empty tree
  if (cur == t->nil) {
    t->root = new_node;
    t->root->color = RBTREE_BLACK;
    return t->root;
  }

  // Sink
  while (1) {
    if (key < cur->key) {
      if (cur->left == t->nil) {
        cur->left = new_node;
        new_node->parent = cur;
        break;
      } else cur = cur->left;
    } else {
      if (cur->right == t->nil) {
        cur->right = new_node;
        new_node->parent = cur;
        break;
      } else cur = cur->right;
    }
  }

  // Color Fix
  child = new_node;
  parent = child->parent;
  grand = parent->parent;
  uncle = grand->left == parent ? grand->right : grand->left;
  while (parent->color == RBTREE_RED) {
    // Case: Uncle = Red > Color Swap > Propagation
    if (uncle->color == RBTREE_RED) {
      parent->color = uncle->color = RBTREE_BLACK;
      grand->color = RBTREE_RED;

      child = grand;
      parent = child->parent;
      grand = parent->parent;
      uncle = grand->left == parent ? grand->right : grand->left;
      if (child == t->root) child->color = RBTREE_BLACK;
    }
    // Case: Uncle = Black > Rotate
    else if (grand->left == parent && parent->left == child) { // Case: LL
      rotate_R(t, grand);

      grand->color = RBTREE_RED;
      parent->color = RBTREE_BLACK;
      break;
    } else if (grand->left == parent && parent->right == child) { // Case: LR
      rotate_L(t, parent);
      rotate_R(t, grand);

      child->color = RBTREE_BLACK;
      grand->color = RBTREE_RED;
      break;
    } else if (grand->right == parent && parent->left == child) { // Case: RL
      rotate_R(t, parent);
      rotate_L(t, grand);

      child->color = RBTREE_BLACK;
      grand->color = RBTREE_RED;
      break;
    } else if (grand->right == parent && parent->right == child) { // Case: RR
      rotate_L(t, grand);

      grand->color = RBTREE_RED;
      parent->color = RBTREE_BLACK;
      break;
    } else {
      printf("Edge Error (rbtree_insert)\n");
      return t->root;
    }
  }

  return t->root;
}

node_t *rbtree_find(const rbtree *t, const key_t key) {
  node_t *cur;

  if (t == NULL) {
    printf("tree is NULL (rbtree_find)\n");
    return NULL;
  }

  cur = t->root;
  if (cur == t->nil) return NULL;

  while (key != cur->key) {
    if (key < cur->key) {
      if (cur->left == t->nil) return NULL;
      cur = cur->left;
    } else {
      if (cur->right == t->nil) return NULL;
      cur = cur->right;
    }
  }

  return cur;
}

node_t *rbtree_min(const rbtree *t) {
  node_t *cur;

  if (t == NULL) {
    printf("tree is NULL (rbtree_min)\n");
    return NULL;
  }

  cur = t->root;
  if (cur == t->nil) return NULL;

  while (cur->left != t->nil) cur = cur->left;

  return cur;
}

node_t *rbtree_max(const rbtree *t) {
  node_t *cur;

  if (t == NULL) {
    printf("tree is NULL (rbtree_max)\n");
    return NULL;
  }

  cur = t->root;
  if (cur == t->nil) return NULL;

  while (cur->right != t->nil) cur = cur->right;

  return cur;
}

int erase_node(rbtree *t, node_t *child, node_t *parent, node_t *sibling) {
  node_t *victim = child, *tmp;

  // Input Validation
  if (child->left != t->nil || child->right != t->nil) {
    printf("victim is not leaf (erase_node)\n");
    return 1;
  }

  // Base Case: Root
  if (child == t->root) {
    t->root = t->nil;
    free(victim);
    return 0;
  }

  // Case: RED
  if (child->color == RBTREE_RED) {
    if (parent->right == child) parent->right = t->nil;
    else parent->left = t->nil;
    free(victim);
    return 0;
  }

  while (1) {
    // Case: BLACK - Sibling = RED
    if (sibling->color == RBTREE_RED) {
      parent->color = RBTREE_RED;
      sibling->color = RBTREE_BLACK;

      if (parent->left == sibling) {
        rotate_R(t, parent);
        sibling = parent->left;
      } else {
        rotate_L(t, parent);
        sibling = parent->right;
      }
    }
    // Case: BLACK - Sibling = BLACK - Nephew = ALL BLACK
    else if (sibling->left->color == RBTREE_BLACK && sibling->right->color == RBTREE_BLACK) {
      sibling->color = RBTREE_RED;
      if (parent->color == RBTREE_RED || parent == t->root) {
        parent->color = RBTREE_BLACK;
        // Escape Loop
        break;
      } else {
        child = child->parent;
        parent = parent->parent;
        sibling = parent->left == child ? parent->right : parent->left;
      }
    }
    // Case: BLACK - Sibling = BLACK - Nephew = LL RED 
    else if (parent->left == sibling && sibling->left->color == RBTREE_RED) {
      sibling->color = parent->color;
      parent->color = RBTREE_BLACK;
      sibling->left->color = RBTREE_BLACK;

      rotate_R(t, parent);
      // Escape Loop
      break;
    }
    // Case: BLACK - Sibling = BLACK - Nephew = RR RED
    else if (parent->right == sibling && sibling->right->color == RBTREE_RED) {
      sibling->color = parent->color;
      parent->color = RBTREE_BLACK;
      sibling->right->color = RBTREE_BLACK;

      rotate_L(t, parent);
      // Escape Loop
      break;
    }
    // Case: BLACK - Sibling = BLACK - Nephew = Only LR RED
    else if (parent->left == sibling && sibling->right->color == RBTREE_RED) {
      sibling->color = RBTREE_RED;
      sibling->right->color = RBTREE_BLACK;

      tmp = sibling->right;
      rotate_L(t, sibling);
      sibling = tmp;

      sibling->color = parent->color;
      parent->color = sibling->left->color = RBTREE_BLACK;

      rotate_R(t, parent);
      // Escape Loop
      break;
    } // Case: BLACK - Sibling = BLACK - Nephew = Only RL RED
    else if (parent->right == sibling && sibling->left->color == RBTREE_RED) {
      sibling->color = RBTREE_RED;
      sibling->left->color = RBTREE_BLACK;

      tmp = sibling->left;
      rotate_R(t, sibling);
      sibling = tmp;

      sibling->color = parent->color;
      parent->color = sibling->right->color = RBTREE_BLACK;

      rotate_L(t, parent);
      //Escape Loop
      break;
    }
  }

  if (victim->parent->right == victim) victim->parent->right = t->nil;
  else victim->parent->left = t->nil;
  free(victim);

  return 0;
}

int rbtree_erase(rbtree *t, node_t *p) {
  node_t *cur, *swap;
  node_t *child, *parent, *sibling;
  key_t tmp;

  // Input Validation
  if (t == NULL) {
    printf("tree is NULL (rbtree_erase)\n");
    return 1;
  }

  if (p == NULL) {
    printf("target is NULL (rbtree_erase)\n");
    return 1;
  }
  
  // Find lowerbound > Swap
  cur = p;
  while (cur->left != t->nil || cur->right != t->nil) {
    swap = cur;
    if (cur->right == t->nil) swap = cur->left;
    else {
      swap = cur->right;
      while (swap->left != t->nil) {
        swap = swap->left;
      }
    }
    tmp = cur->key;
    cur->key = swap->key;
    swap->key = tmp;

    cur = swap;
  }

  child = cur;
  parent = child->parent;
  sibling = parent->left == child ? parent->right : parent->left;
  return erase_node(t, child, parent, sibling);
}

void print_tree(const rbtree* t) {
  print_tree_structure(t, t->root, "", 1);
  printf("\n");
}

void print_tree_structure(const rbtree *t, node_t *p, char *prefix, int is_last) {
  /*         넓이 설정         */
  /* width = 3 -> key = 0~999 */
  int width = 3;

  if (p == NULL || p == t->nil) return;
  
  printf("%s", prefix);
  
  if (strlen(prefix) == 0) {
    printf("key: %*d | color: %c | parent: %*d | left: %*d | right: %*d\n",
           width, p->key,
           p->color == RBTREE_BLACK ? 'B' : 'R',
           width, (p->parent == NULL || p->parent == t->nil) ? 0 : p->parent->key,
           width, (p->left == NULL || p->left == t->nil) ? 0 : p->left->key,
           width, (p->right == NULL || p->right == t->nil) ? 0 : p->right->key);

  } else {
    printf("%skey: %*d | color: %c | parent: %*d | left: %*d | right: %*d\n",
           is_last ? "╚═ " : "╠═ ",
           width, p->key,
           p->color == RBTREE_BLACK ? 'B' : 'R',
           width, (p->parent == NULL || p->parent == t->nil) ? 0 : p->parent->key,
           width, (p->left == NULL || p->left == t->nil) ? 0 : p->left->key,
           width, (p->right == NULL || p->right == t->nil) ? 0 : p->right->key);
  }
  
  int has_left = (p->left != NULL && p->left != t->nil);
  int has_right = (p->right != NULL && p->right != t->nil);
  
  if (has_left || has_right) {
    char new_prefix[200];
    
    if (strlen(prefix) == 0) {
      sprintf(new_prefix, "%s", is_last ? " " : "║");
    } else {
      sprintf(new_prefix, "%s%s", prefix, is_last ? "    " : "║   ");
    }
    
    if (has_left) {
      print_tree_structure(t, p->left, new_prefix, !has_right);
    }
    
    if (has_right) {
      print_tree_structure(t, p->right, new_prefix, 1);
    }
  }
}

void in_order(const rbtree *t, key_t *arr, node_t *p, int *idx_p) {
  if (p->left != t->nil) in_order(t, arr, p->left, idx_p);
  *(arr + (*idx_p)) = p->key;
  (*idx_p)++;
  if (p->right != t->nil) in_order(t, arr, p->right, idx_p);
}

int rbtree_to_array(const rbtree *t, key_t *arr, const size_t n) {
  int idx = 0;

  if (t == NULL) {
    printf("tree is null (rbtree_to_array)\n");
    return 1;
  }

  if (arr == NULL) {
    printf("arr is null (rbtree_to_array)\n");
    return 1;
  }

  if (t->root != t->nil) in_order(t, arr, t->root, &idx);

  if (idx != n) {
    printf("Index Out of Range (rbtree_to_array)\n");
    return 1;
  }

  return 0;
}